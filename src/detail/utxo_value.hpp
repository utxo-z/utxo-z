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
#include <ranges>
#include <span>
#include <type_traits>

#include <utxoz/types.hpp>

#include <utxoz/flatmap/mmap_flat_map.hpp>

namespace utxoz::detail {

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
    }

    std::span<uint8_t const> get_data() const {
        return {data.data(), actual_size};
    }
};

/// The internal map type — lives directly in an mmap'd buffer.
/// No allocator needed; groups + elements are the file's table buffer.
template<size_t Size>
using utxo_map = utxoz::flatmap::mmap_flat_map<
    raw_outpoint,
    utxo_value<Size>,
    outpoint_hash,
    outpoint_equal
>;

} // namespace utxoz::detail
