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
#include <map>

#include <boost/container_hash/hash.hpp>

namespace utxoz {

/**
 * @brief Size of UTXO key in bytes (32 bytes transaction hash + 4 bytes output index)
 */
inline constexpr size_t key_size = 36;

/**
 * @brief UTXO key type - consists of transaction hash + output index
 */
using key_t = std::array<uint8_t, key_size>;

/**
 * @brief Span of bytes for value data
 */
using value_span_t = std::span<uint8_t const>;

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
    key_t key;         ///< UTXO key to delete
    uint32_t height;   ///< Block height when deletion was requested
    
    deferred_deletion_entry(key_t const& k, uint32_t h) 
        : key(k), height(h) {}
    
    bool operator==(deferred_deletion_entry const& other) const {
        return key == other.key;
    }

    friend std::size_t hash_value(deferred_deletion_entry const& entry) {
        return boost::hash<key_t>{}(entry.key);
    }
};

} // namespace utxoz

/**
 * @brief Hash function for UTXO key
 */
template<>
struct std::hash<utxoz::key_t> {
    std::size_t operator()(utxoz::key_t const& key) const noexcept {
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
        return std::hash<utxoz::key_t>{}(entry.key);
    }
};
