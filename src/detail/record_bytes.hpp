// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file record_bytes.hpp
 * @brief The encoding every fixed-layout record in this store shares.
 * @internal
 *
 * Field by field, fixed widths, little-endian, checksum last. Nothing here ever
 * writes a struct through a cast: a struct carries padding the compiler chooses,
 * and a record that means one thing on the machine that wrote it and another on
 * the machine that reads it is exactly what these records exist to prevent.
 *
 * Little-endian because every target the store builds for is; the assertion is
 * what makes a big-endian port fail loudly rather than quietly write a different
 * encoding under the same marker.
 */

#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>
#include <vector>

namespace utxoz::detail::record_bytes {

static_assert(std::endian::native == std::endian::little,
              "these records are little-endian; a big-endian port must convert and bump "
              "the format version of every record that uses this encoding");

/// FNV-1a, 32-bit. Enough to catch a torn or truncated write, which is all it is
/// for — it is not a defence against deliberate tampering and is not presented
/// as one.
[[nodiscard]]
inline uint32_t checksum(std::span<uint8_t const> bytes) noexcept {
    uint32_t hash = 2166136261u;
    for (auto const b : bytes) {
        hash ^= b;
        hash *= 16777619u;
    }
    return hash;
}

/// Scalars only. A struct would carry whatever padding the compiler chose, and
/// copying that verbatim is the thing these records exist not to do.
template <typename T>
    requires std::is_scalar_v<T>
void put(std::vector<uint8_t>& out, T const& value) {
    auto const* src = reinterpret_cast<uint8_t const*>(&value);
    out.insert(out.end(), src, src + sizeof(T));
}

template <typename T>
    requires std::is_scalar_v<T>
void get(uint8_t const*& cursor, T& value) {
    std::memcpy(&value, cursor, sizeof(T));
    cursor += sizeof(T);
}

} // namespace utxoz::detail::record_bytes
