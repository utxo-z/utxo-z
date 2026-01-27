// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file literals.hpp
 * @brief User-defined literals for memory sizes
 */

#pragma once

#include <cstddef>

namespace utxoz::literals {

/**
 * @brief Mebibyte literal (1 MiB = 1024 * 1024 bytes)
 * @param n Number of mebibytes
 * @return Size in bytes
 *
 * Example: 100_mib -> 104857600
 */
constexpr size_t operator""_mib(unsigned long long n) {
    return static_cast<size_t>(n) * 1024 * 1024;
}

/**
 * @brief Gibibyte literal (1 GiB = 1024 * 1024 * 1024 bytes)
 * @param n Number of gibibytes
 * @return Size in bytes
 *
 * Example: 2_gib -> 2147483648
 */
constexpr size_t operator""_gib(unsigned long long n) {
    return static_cast<size_t>(n) * 1024 * 1024 * 1024;
}

} // namespace utxoz::literals
