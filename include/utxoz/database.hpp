// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file database.hpp
 * @brief Main database interface
 */

#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <utxoz/statistics.hpp>
#include <utxoz/types.hpp>

namespace utxoz {

namespace detail {
struct database_impl;
} // namespace detail

/**
 * @brief Main UTXO Database interface
 *
 * This class provides a high-performance UTXO database with the following features:
 * - Multi-container architecture optimized for different value sizes
 * - Memory-mapped file storage with automatic rotation
 * - Deferred deletion for optimal write performance
 * - Comprehensive statistics and performance monitoring
 * - File-based caching for historical data access
 * - Database compaction and optimization
 */
struct db {
    db();
    ~db();

    // Non-copyable
    db(db const&) = delete;
    db& operator=(db const&) = delete;

    // Movable - allows returning from factory functions and storing in containers
    db(db&&) noexcept;
    db& operator=(db&&) noexcept;

    /**
     * @brief Configure and open the database
     * @param path Database directory path
     * @param remove_existing If true, remove existing database files
     * @throws std::runtime_error on configuration failure
     */
    void configure(std::string_view path, bool remove_existing = false);

    /**
     * @brief Configure for testing with smaller file sizes
     * @param path Database directory path
     * @param remove_existing If true, remove existing database files
     * @throws std::runtime_error on configuration failure
     */
    void configure_for_testing(std::string_view path, bool remove_existing = false);

    /**
     * @brief Close the database and flush all data
     */
    void close();

    /**
     * @brief Get the total number of UTXOs in the database
     */
    [[nodiscard]] size_t size() const;

    /**
     * @brief Insert a new UTXO
     * @param key UTXO key (transaction hash + output index)
     * @param value UTXO value data
     * @param height Block height where this UTXO was created
     * @return true if inserted successfully, false if already exists
     */
    [[nodiscard]] bool insert(key_t const& key, value_span_t value, uint32_t height);

    /**
     * @brief Find a UTXO by key
     * @param key UTXO key to search for
     * @param height Current block height (for statistics)
     * @return UTXO value if found, std::nullopt otherwise
     */
    [[nodiscard]] std::optional<std::vector<uint8_t>> find(key_t const& key, uint32_t height);

    /**
     * @brief Erase a UTXO by key
     *
     * Deletion may be deferred for performance reasons. Use process_pending_deletions()
     * to ensure all deletions are completed.
     *
     * @param key UTXO key to erase
     * @param height Current block height
     * @return Number of UTXOs erased (0 if not found, 1 if found and erased)
     */
    [[nodiscard]] size_t erase(key_t const& key, uint32_t height);

    /**
     * @brief Process all pending deferred deletions
     * @return Pair of (successful_deletions_count, failed_deletions)
     *         Each failed deletion includes the key and the block height that requested it
     */
    [[nodiscard]] std::pair<uint32_t, std::vector<deferred_deletion_entry>> process_pending_deletions();

    /**
     * @brief Get the number of pending deferred deletions
     */
    [[nodiscard]] size_t deferred_deletions_size() const;

    /**
     * @brief Compact all containers
     *
     * Merges files, removes empty ones, and optimizes storage.
     * Should be called periodically for optimal performance.
     */
    void compact_all();

    /**
     * @brief Get comprehensive database statistics
     */
    [[nodiscard]] database_statistics get_statistics();

    /**
     * @brief Print formatted statistics to log
     */
    void print_statistics();

    /**
     * @brief Reset all statistics counters
     */
    void reset_all_statistics();

    /**
     * @brief Get search performance statistics
     */
    [[nodiscard]] search_stats const& get_search_stats() const;

    /**
     * @brief Reset search statistics
     */
    void reset_search_stats();

    /**
     * @brief Get file cache hit rate
     * @return Cache hit rate (0.0 to 1.0)
     */
    [[nodiscard]] float get_cache_hit_rate() const;

    /**
     * @brief Get information about currently cached files
     * @return Vector of (container_index, version) pairs
     */
    [[nodiscard]] std::vector<std::pair<size_t, size_t>> get_cached_file_info() const;

private:
    std::unique_ptr<detail::database_impl> impl_;
};

} // namespace utxoz
