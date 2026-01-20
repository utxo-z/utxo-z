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

#include <boost/container_hash/hash.hpp>
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
using key_hash = boost::hash<key_t>;
using key_equal = std::equal_to<key_t>;

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

template<size_t Size>
using utxo_map = boost::unordered_flat_map<
    key_t,
    utxo_value<Size>,
    key_hash,
    key_equal,
    bip::allocator<std::pair<key_t const, utxo_value<Size>>, segment_manager_t>
>;

} // namespace utxoz::detail
