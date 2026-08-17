// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file utils.cpp
 * @brief Utility functions implementation
 */

#include <cassert>

#include "detail/capacity_policy.hpp"
#include "detail/log.hpp"
#include <utxoz/utils.hpp>

#include <algorithm>
#include <bit>
#include <cstring>

#include <fmt/format.h>

namespace utxoz {

void print_outpoint(raw_outpoint const& outpoint) {
    log::debug("{}", outpoint_to_string(outpoint));
}

std::string outpoint_to_string(raw_outpoint const& outpoint) {
    std::string result;
    result.reserve(64 + 10); // 64 hex chars + ":" + up to 10 digits for index

    // txid in reversed hex (display order)
    for (size_t i = 0; i < 32; ++i) {
        result += fmt::format("{:02x}", outpoint[31 - i]);
    }

    // Output index
    uint32_t output_index = 0;
    std::memcpy(&output_index, outpoint.data() + 32, sizeof(uint32_t));
    result += fmt::format(":{}", output_index);

    return result;
}

raw_outpoint make_outpoint(std::span<uint8_t const, 32> txid, uint32_t output_index) {
    raw_outpoint outpoint;
    std::copy(txid.begin(), txid.end(), outpoint.begin());
    std::memcpy(outpoint.data() + 32, &output_index, sizeof(uint32_t));
    return outpoint;
}

std::span<uint8_t const, 32> get_txid(raw_outpoint const& outpoint) {
    return std::span<uint8_t const, 32>{outpoint.data(), 32};
}

uint32_t get_output_index(raw_outpoint const& outpoint) {
    uint32_t output_index = 0;
    std::memcpy(&output_index, outpoint.data() + 32, sizeof(uint32_t));
    return output_index;
}

size_t calculate_optimal_buckets(size_t element_count, float max_load_factor) {
    if (element_count == 0) {
        return 0;
    }
    
    if (element_count < 15) {
        return 29;
    }
    
    constexpr float margin = 0.95f;
    float safe_load = max_load_factor * margin;
    
    size_t required_buckets = static_cast<size_t>(std::ceil(element_count / safe_load));
    size_t k = (required_buckets + 1) / 15;
    size_t ceil_pow2 = std::bit_ceil(k);
    
    return ceil_pow2 * 15 - 1;
}

} // namespace utxoz

namespace utxoz::detail {

bool detect_rehash(uint32_t container_kind, rehash_watch& watch, size_t now) {
    if (watch.at_open == 0 || now == watch.last_reported) return false;

    size_t const was = watch.last_reported;
    watch.last_reported = now;
    rehashes_observed.fetch_add(1, std::memory_order_relaxed);
    log::error("container {} grew to {} buckets, from {} and originally {}. A "
               "generation's bucket count is not supposed to change: the store makes a "
               "new generation when one fills up, and compaction deals with the cost "
               "later. The entry was written; this is a defect in the guard, not a "
               "failure of this insert.",
               container_kind, now, was, watch.at_open);
    return true;
}

bool note_rehash_if_grown(uint32_t container_kind, rehash_watch& watch, size_t now) {
    bool const grown = detect_rehash(container_kind, watch, now);

    // Not in a release build, where the alternative to continuing is losing a
    // node. In a build with assertions it stops here, because continuing to write
    // into a store that has started rehashing under itself buries the evidence.
    assert( ! grown && "a generation's bucket count changed");
    return grown;
}

} // namespace utxoz::detail
