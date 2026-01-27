// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file utils.hpp
 * @brief Utility functions for UTXO database
 */

#pragma once

#include <string>

#include <utxoz/types.hpp>

namespace utxoz {

/**
 * @brief Print a UTXO key in human-readable format
 * 
 * Prints the transaction hash in reversed hex format followed by the output index.
 * Format: "deadbeef...abcd1234:5"
 * 
 * @param key UTXO key to print
 */
void print_key(key_t const& key);

/**
 * @brief Convert UTXO key to string representation
 * @param key UTXO key to convert
 * @return String representation of the key
 */
[[nodiscard]]
std::string key_to_string(key_t const& key);

/**
 * @brief Create UTXO key from transaction hash and output index
 * @param tx_hash Transaction hash (32 bytes)
 * @param output_index Output index (4 bytes)
 * @return Constructed UTXO key
 */
[[nodiscard]]
key_t make_key(std::span<uint8_t const, 32> tx_hash, uint32_t output_index);

/**
 * @brief Extract transaction hash from UTXO key
 * @param key UTXO key
 * @return Span pointing to the transaction hash portion (first 32 bytes)
 */
[[nodiscard]]
std::span<uint8_t const, 32> get_tx_hash(key_t const& key);

/**
 * @brief Extract output index from UTXO key
 * @param key UTXO key
 * @return Output index (last 4 bytes interpreted as uint32_t)
 */
[[nodiscard]]
uint32_t get_output_index(key_t const& key);

/**
 * @brief Calculate optimal bucket count for hash table
 * @param element_count Expected number of elements
 * @param max_load_factor Maximum load factor to maintain
 * @return Optimal bucket count
 */
[[nodiscard]]
size_t calculate_optimal_buckets(size_t element_count, float max_load_factor = 0.88f);

} // namespace utxoz
