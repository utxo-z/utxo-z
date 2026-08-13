// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file utxo_value.hpp
 * @brief UTXO value storage types for memory-mapped containers
 * @internal
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <ranges>
#include <span>
#include <type_traits>

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

#include <utxoz/types.hpp>

namespace utxoz::detail {

namespace bip = boost::interprocess;

// =============================================================================
// Type aliases
// =============================================================================

using segment_manager_t = bip::managed_mapped_file::segment_manager;

struct outpoint_hash {
    size_t operator()(raw_outpoint const& k) const noexcept {
        return hash_outpoint(k);
    }
};

using outpoint_equal = std::equal_to<raw_outpoint>;

// =============================================================================
// UTXO value storage
// =============================================================================

template<size_t Size>
using size_type = std::conditional_t<Size <= 255, uint8_t, uint16_t>;

template<size_t Size>
struct utxo_value {
    uint32_t block_height;
    size_type<Size> actual_size;
    std::array<uint8_t, Size - sizeof(uint32_t) - sizeof(size_type<Size>)> data;

    void set_data(std::span<uint8_t const> output_data) {
        actual_size = static_cast<size_type<Size>>(
            std::min(output_data.size(), data.size())
        );
        std::ranges::copy(output_data.first(actual_size), data.begin());

        // Everything past the payload is persisted too. This object is stored
        // inline in a memory-mapped file, so any byte left unwritten here is
        // whatever this storage last held — for a value built on the stack, the
        // caller's stack — and it goes to disk. Nothing reads it back, which is
        // exactly why it went unnoticed.
        //
        // Two regions, and they have to be cleared separately even though they
        // are adjacent. The tail of `data` is the large part: a 10240 container
        // storing a 200-byte output persists just over 10 KiB of it. The second
        // is the padding `sizeof` adds when the size class is not a multiple of
        // the alignment — 94 occupies 96 — which no member names and which is
        // copied and written along with the rest.
        //
        // One `fill` from inside `data` to the end of the object would be
        // undefined: the padding is not part of that array, and forming a pointer
        // past its one-past-the-end is not made legal by the padding happening to
        // sit there. So the tail is cleared as the array it belongs to, and the
        // padding through the object representation, which is the sequence of
        // unsigned chars a standard-layout object is allowed to be viewed as.
        std::fill(data.begin() + actual_size, data.end(), uint8_t{0});

        static_assert(std::is_standard_layout_v<utxo_value>,
                      "offsetof below, and the object-representation access, need this");
        static_assert(std::is_trivially_copyable_v<utxo_value>,
                      "the value is copied into the mapping as bytes, padding included");

        constexpr size_t members_end = offsetof(utxo_value, data) + sizeof(data);
        static_assert(members_end <= sizeof(utxo_value));
        if constexpr (members_end < sizeof(utxo_value)) {
            std::memset(reinterpret_cast<unsigned char*>(this) + members_end, 0,
                        sizeof(utxo_value) - members_end);
        }
    }

    std::span<uint8_t const> get_data() const {
        return {data.data(), actual_size};
    }
};

// What `sizeof` actually reserves for each container, pinned. 94 is the one that
// rounds up, and the two bytes it gains are the padding set_data() defines above.
// Written out rather than derived so that a change to the members shows up here
// as a failing build instead of as a quietly different file on disk.
static_assert(sizeof(utxo_value<48>) == 48);
static_assert(sizeof(utxo_value<94>) == 96);
static_assert(sizeof(utxo_value<128>) == 128);
static_assert(sizeof(utxo_value<256>) == 256);
static_assert(sizeof(utxo_value<10240>) == 10240);

/// The name the entry map is stored under inside a segment.
///
/// One spelling, because every reader has to use the one the writer used, and a
/// literal repeated at fifteen call sites is one typo away from a file whose
/// map nothing can find — which reads as an empty version rather than as an
/// error.
inline constexpr char const* map_object_name = "db_map";

template<size_t Size>
using utxo_map = boost::unordered_flat_map<
    raw_outpoint,
    utxo_value<Size>,
    outpoint_hash,
    outpoint_equal,
    bip::allocator<std::pair<raw_outpoint const, utxo_value<Size>>, segment_manager_t>
>;

// =============================================================================
// Reference mode value storage
// =============================================================================

struct reference_value {
    uint32_t height;
    uint32_t file_number;
    uint32_t offset;
};

using reference_map_t = boost::unordered_flat_map<
    raw_outpoint,
    reference_value,
    outpoint_hash,
    outpoint_equal,
    bip::allocator<std::pair<raw_outpoint const, reference_value>, segment_manager_t>
>;

} // namespace utxoz::detail
