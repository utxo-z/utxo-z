// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file types.hpp
 * @brief Core type definitions for the UTXO database
 */

#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>
#include <optional>
#include <chrono>

#include <boost/container_hash/hash.hpp>

namespace utxoz {

/**
 * @brief Size of outpoint in bytes (32 bytes txid + 4 bytes output index)
 */
inline constexpr size_t outpoint_size = 36;

/**
 * @brief Raw outpoint - 36-byte array representing a Bitcoin outpoint
 *
 * Layout: [txid: 32 bytes][output_index: 4 bytes]
 *
 * Typical encoding:
 * - txid: Internal byte order (as stored in Bitcoin's internal representation)
 * - output_index: Little-endian uint32_t
 *
 * However, the database is agnostic to the specific encoding used.
 * Users may encode the outpoint in any format, as long as the same
 * encoding is used consistently across all operations.
 *
 * @see make_outpoint() for constructing outpoints
 * @see get_txid(), get_output_index() for extracting components
 */
using raw_outpoint = std::array<uint8_t, outpoint_size>;

/**
 * @brief Span of bytes representing UTXO output data
 *
 * Typically contains the serialized transaction output:
 * - Amount in satoshis (8 bytes, little-endian)
 * - scriptPubKey (variable length)
 *
 * The database stores this data as-is without interpreting its contents.
 */
using output_data_span = std::span<uint8_t const>;

/**
 * @brief Container sizes for different UTXO value sizes
 */
inline constexpr std::array<size_t, 4> container_sizes = {44, 128, 512, 10240};

/**
 * @brief File sizes for each container type (production)
 */
inline constexpr std::array<size_t, 4> file_sizes = {
    2ULL * 1024 * 1024 * 1024, // 2 GiB
    2ULL * 1024 * 1024 * 1024, // 2 GiB
    100ULL * 1024 * 1024,      // 100 MiB
    50ULL * 1024 * 1024        // 50 MiB
};

/**
 * @brief File sizes for each container type (testing - much smaller)
 */
inline constexpr std::array<size_t, 4> test_file_sizes = {
    10ULL * 1024 * 1024,       // 10 MiB
    10ULL * 1024 * 1024,       // 10 MiB
    10ULL * 1024 * 1024,       // 10 MiB
    10ULL * 1024 * 1024        // 10 MiB
};

/**
 * @brief Number of container types
 */
inline constexpr size_t container_count = container_sizes.size();

/**
 * @brief Memory size literals
 */
namespace literals {
    constexpr size_t operator""_mib(unsigned long long n) {
        return static_cast<size_t>(n) * 1024 * 1024;
    }

    constexpr size_t operator""_gib(unsigned long long n) {
        return static_cast<size_t>(n) * 1024 * 1024 * 1024;
    }
} // namespace literals

/**
 * @brief Search operation record for performance tracking
 */
struct search_record {
    uint32_t access_height;     ///< Block height when accessed
    uint32_t insertion_height;  ///< Block height when inserted
    uint32_t depth;            ///< Search depth (0 = current version, >0 = previous versions)
    bool is_cache_hit;         ///< Whether this was a cache hit
    bool found;                ///< Whether the UTXO was found
    char operation;            ///< Operation type ('f' = find, 'e' = erase)
    
    /**
     * @brief Calculate UTXO age in blocks
     * @return Age in blocks, or 0 if not found or invalid
     */
    uint32_t get_utxo_age() const {
        return found && access_height >= insertion_height ? 
               access_height - insertion_height : 0;
    }
};

/**
 * @brief Deferred deletion entry
 */
struct deferred_deletion_entry {
    raw_outpoint key;         ///< UTXO key to delete
    uint32_t height;   ///< Block height when deletion was requested

    deferred_deletion_entry(raw_outpoint const& k, uint32_t h)
        : key(k), height(h) {}

    bool operator==(deferred_deletion_entry const& other) const {
        return key == other.key;
    }

    friend std::size_t hash_value(deferred_deletion_entry const& entry) {
        return boost::hash<raw_outpoint>{}(entry.key);
    }
};

/**
 * @brief Deferred lookup entry
 */
struct deferred_lookup_entry {
    raw_outpoint key;         ///< UTXO key to lookup
    uint32_t height;   ///< Block height when lookup was requested

    deferred_lookup_entry(raw_outpoint const& k, uint32_t h)
        : key(k), height(h) {}

    bool operator==(deferred_lookup_entry const& other) const {
        return key == other.key;
    }

    friend std::size_t hash_value(deferred_lookup_entry const& entry) {
        return boost::hash<raw_outpoint>{}(entry.key);
    }
};

} // namespace utxoz

/**
 * @brief Hash function for UTXO key
 */
template<>
struct std::hash<utxoz::raw_outpoint> {
    std::size_t operator()(utxoz::raw_outpoint const& key) const noexcept {
        // Simple hash combination
        std::size_t seed = 0;
        for (auto byte : key) {
            seed ^= std::hash<uint8_t>{}(byte) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

/**
 * @brief Hash function for deferred_deletion_entry
 */
template<>
struct std::hash<utxoz::deferred_deletion_entry> {
    std::size_t operator()(utxoz::deferred_deletion_entry const& entry) const noexcept {
        return std::hash<utxoz::raw_outpoint>{}(entry.key);
    }
};

/**
 * @brief Hash function for deferred_lookup_entry
 */
template<>
struct std::hash<utxoz::deferred_lookup_entry> {
    std::size_t operator()(utxoz::deferred_lookup_entry const& entry) const noexcept {
        return std::hash<utxoz::raw_outpoint>{}(entry.key);
    }
};
