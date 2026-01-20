// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file file_metadata.hpp
 * @brief Metadata tracking for database files
 * @internal
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

#include <utxoz/types.hpp>

namespace utxoz::detail {

/**
 * @brief Metadata for tracking contents of a database file
 *
 * Stores range information to enable efficient filtering during searches.
 */
struct file_metadata {
    uint32_t min_block_height = std::numeric_limits<uint32_t>::max();
    uint32_t max_block_height = 0;
    key_t min_key;
    key_t max_key;
    size_t entry_count = 0;
    size_t container_index = 0;
    size_t version = 0;

    file_metadata() {
        min_key.fill(0xFF);
        max_key.fill(0x00);
    }

    bool key_in_range(key_t const& key) const {
        return entry_count > 0 && key >= min_key && key <= max_key;
    }

    bool block_in_range(uint32_t height) const {
        return entry_count > 0 &&
               height >= min_block_height && height <= max_block_height;
    }

    void update_on_insert(key_t const& key, uint32_t height) {
        if (entry_count == 0) {
            min_key = max_key = key;
            min_block_height = max_block_height = height;
        } else {
            if (key < min_key) min_key = key;
            if (key > max_key) max_key = key;
            min_block_height = std::min(min_block_height, height);
            max_block_height = std::max(max_block_height, height);
        }
        ++entry_count;
    }

    void update_on_delete() {
        if (entry_count > 0) --entry_count;
    }
};

} // namespace utxoz::detail
