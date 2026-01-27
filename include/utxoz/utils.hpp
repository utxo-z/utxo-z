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
 * @brief Print an outpoint in human-readable format
 *
 * Prints the txid in reversed hex format followed by the output index.
 * Format: "deadbeef...abcd1234:5"
 *
 * @param outpoint Outpoint to print
 */
void print_outpoint(raw_outpoint const& outpoint);

/**
 * @brief Convert outpoint to string representation
 * @param outpoint Outpoint to convert
 * @return String representation of the outpoint
 */
[[nodiscard]]
std::string outpoint_to_string(raw_outpoint const& outpoint);

/**
 * @brief Create outpoint from txid and output index
 * @param txid Transaction ID (32 bytes)
 * @param output_index Output index (4 bytes)
 * @return Constructed outpoint
 */
[[nodiscard]]
raw_outpoint make_outpoint(std::span<uint8_t const, 32> txid, uint32_t output_index);

/**
 * @brief Extract txid from outpoint
 * @param outpoint Outpoint
 * @return Span pointing to the txid portion (first 32 bytes)
 */
[[nodiscard]]
std::span<uint8_t const, 32> get_txid(raw_outpoint const& outpoint);

/**
 * @brief Extract output index from outpoint
 * @param outpoint Outpoint
 * @return Output index (last 4 bytes interpreted as uint32_t)
 */
[[nodiscard]]
uint32_t get_output_index(raw_outpoint const& outpoint);

/**
 * @brief Calculate optimal bucket count for hash table
 * @param element_count Expected number of elements
 * @param max_load_factor Maximum load factor to maintain
 * @return Optimal bucket count
 */
[[nodiscard]]
size_t calculate_optimal_buckets(size_t element_count, float max_load_factor = 0.88f);

} // namespace utxoz
