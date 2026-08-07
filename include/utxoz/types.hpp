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
#include <cstring>
#include <expected>
#include <span>
#include <vector>
#include <optional>
#include <chrono>

#include <utxoz/literals.hpp>

namespace utxoz {

using namespace literals;

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
 * @brief Fast hash for raw_outpoint.
 *
 * Since the first 32 bytes are a SHA256 txid (uniformly distributed),
 * we just grab the first 8 bytes as size_t and mix in the 4-byte output index.
 * This is O(1) instead of the O(36) byte-by-byte hash_combine approach.
 */
inline size_t hash_outpoint(raw_outpoint const& k) noexcept {
    size_t seed;
    std::memcpy(&seed, k.data(), sizeof(seed));
    uint32_t idx;
    std::memcpy(&idx, k.data() + 32, sizeof(idx));
    seed ^= size_t(idx) * 0x9e3779b97f4a7c15ULL;
    return seed;
}

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
inline constexpr std::array<size_t, 5> container_sizes = {48, 94, 128, 256, 10240};

/**
 * @brief Effective data capacity for a given container size
 *
 * Each utxo_value<Size> stores: block_height (4 bytes) + actual_size field + data.
 * The actual_size field is uint8_t for Size <= 255, uint16_t otherwise.
 */
inline constexpr size_t data_capacity(size_t container_size) {
    size_t size_field = container_size <= 255 ? 1 : 2;
    return container_size - sizeof(uint32_t) - size_field;
}

/**
 * @brief Pre-computed data capacities for each container
 */
inline constexpr auto container_capacities = [] {
    std::array<size_t, container_sizes.size()> caps{};
    for (size_t i = 0; i < container_sizes.size(); ++i) {
        caps[i] = data_capacity(container_sizes[i]);
    }
    return caps;
}();

/**
 * @brief File sizes for each container type (production)
 */
inline constexpr std::array<size_t, 5> file_sizes = {2_gib, 500_mib, 1_gib, 50_mib, 10_mib};

/**
 * @brief File sizes for each container type (testing - much smaller)
 */
inline constexpr std::array<size_t, 5> test_file_sizes = {10_mib, 10_mib, 10_mib, 10_mib, 10_mib};

/**
 * @brief Number of container types
 */
inline constexpr size_t container_count = container_sizes.size();

/**
 * @brief Storage mode selection
 *
 * - full: Stores complete UTXO output data (scriptPubKey + amount) across 5 size-tiered containers
 * - compact: Stores only a small fixed-size reference (e.g. block file position) in a single container
 */
enum class storage_mode : uint8_t { full = 0, compact = 1 };

/**
 * @brief Error codes for database operations
 */
enum class error_code : uint8_t {
    not_found,              ///< Key not found in the database (may be deferred)
    closed,                 ///< Operation on a closed or moved-from database
    storage_mode_mismatch,  ///< Existing database has a different storage mode
    config_file_corrupt,    ///< Config file is invalid, truncated, or has bad magic
    value_too_large,        ///< Value exceeds maximum container capacity
    duplicate_key,          ///< Two stored entries share a key: the database is locally inconsistent
    catalog_unreadable,     ///< The set of version files could not be read; absence cannot be assumed
    /// A drained version file, or its metadata, could not be removed. Two
    /// states reach here: the data file is still present and still catalogued,
    /// or the data file went and a stale metadata record was left behind.
    removal_failed,
    sync_unsupported,       ///< This platform has no such durability barrier; nothing was promised
    sync_failed,            ///< A durability barrier was attempted and failed
    rename_failed,          ///< A file could not be moved onto its final name
    /// Another instance holds this database. Reserved for exactly that: the
    /// lock was attempted and someone else has it.
    database_in_use,
    /// The claim could not be attempted at all — no permission, a filesystem
    /// without locking, a lock file that is not a regular file. Distinct from
    /// database_in_use because they send an operator looking in different
    /// places.
    database_lock_unavailable,
    entropy_unavailable,    ///< The system's cryptographic generator would not produce bytes
    file_open_failed,       ///< A version file could not be created or opened
    identity_collision,     ///< A version identity about to be published is already taken
    insufficient_space,     ///< Not enough room for the file an operation must build
    /// The instance published a merge and could not finish retiring what it
    /// superseded. Several canonical files hold the same keys, so it refuses
    /// every further operation: close it and reopen, which runs recovery.
    recovery_required,
    /// Recovery found evidence of an interrupted operation that it cannot act
    /// on. The database does not open; nothing is guessed at or repaired.
    recovery_failed,
    metadata_write_failed,  ///< A derived metadata record could not be published
};

/**
 * @brief What sync() can promise on this platform.
 *
 * A caller that needs the difference has to be able to ask for it. `sync()`
 * returning success under `contents_only` means the entries are on the disk and
 * the directory entries that name them are not — the ordering between a
 * rotation and the data it publishes is then weaker than POSIX gives, and a
 * caller recording a checkpoint on the strength of it should know that.
 */
enum class durability_level : uint8_t {
    full,           ///< File contents and directory entries can both be made durable.
    contents_only,  ///< Contents can; directory entries have no exposed barrier.
    none,           ///< A virtual filesystem: there is no stable storage to reach.
};

/// What this build's platform can promise. Constant, and safe to branch on.
[[nodiscard]]
durability_level platform_durability() noexcept;

/**
 * @brief Result type for operations that can fail
 */
template<typename T = void>
using result = std::expected<T, error_code>;

/// Result of full_db::find(): data bytes + block height at which the UTXO was inserted.
struct full_find_result {
    std::vector<uint8_t> data;
    uint32_t block_height;
};

/// Result of compact_db::find(): typed compact fields + block height.
struct compact_find_result {
    uint32_t block_height;
    uint32_t file_number;
    uint32_t offset;
};

/// Backward-compatible alias.
using find_result = full_find_result;

/**
 * @brief File size for compact mode (production)
 */
inline constexpr size_t compact_file_size = 4_gib;

/**
 * @brief File size for compact mode (testing)
 */
inline constexpr size_t compact_test_file_size = 10_mib;

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
        return hash_outpoint(entry.key);
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
        return hash_outpoint(entry.key);
    }
};

} // namespace utxoz

/**
 * @brief Hash function for UTXO key
 */
template<>
struct std::hash<utxoz::raw_outpoint> {
    std::size_t operator()(utxoz::raw_outpoint const& key) const noexcept {
        return utxoz::hash_outpoint(key);
    }
};

/**
 * @brief Hash function for deferred_deletion_entry
 */
template<>
struct std::hash<utxoz::deferred_deletion_entry> {
    std::size_t operator()(utxoz::deferred_deletion_entry const& entry) const noexcept {
        return utxoz::hash_outpoint(entry.key);
    }
};

/**
 * @brief Hash function for deferred_lookup_entry
 */
template<>
struct std::hash<utxoz::deferred_lookup_entry> {
    std::size_t operator()(utxoz::deferred_lookup_entry const& entry) const noexcept {
        return utxoz::hash_outpoint(entry.key);
    }
};
