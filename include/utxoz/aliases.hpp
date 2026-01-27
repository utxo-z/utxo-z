// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file aliases.hpp
 * @brief Common type aliases for cleaner code
 */

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>

namespace utxoz {

// Byte containers
using bytes = std::vector<uint8_t>;
using byte_span = std::span<uint8_t const>;

// Optional types
using bytes_opt = std::optional<bytes>;

// Common map types
template<typename K, typename V>
using flat_map = boost::unordered_flat_map<K, V>;

} // namespace utxoz
