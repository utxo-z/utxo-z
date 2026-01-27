// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file utils.cpp
 * @brief Utility functions implementation
 */

#include <utxoz/utils.hpp>

#include <algorithm>
#include <bit>
#include <cstring>

#include <fmt/format.h>

#include "detail/log.hpp"

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
