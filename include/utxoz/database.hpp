// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file database.hpp
 * @brief Main database interface — db_base, full_db, compact_db
 */

#pragma once

#include <memory>
#include <string_view>
#include <utility>

#include <utxoz/aliases.hpp>
#include <utxoz/statistics.hpp>
#include <utxoz/types.hpp>

namespace utxoz {

namespace detail {
struct database_impl;
} // namespace detail

// =============================================================================
// db_base — shared methods for both storage modes
// =============================================================================

/**
 * @brief Base class with methods common to both storage modes.
 *
 * Not intended to be instantiated directly — use full_db or compact_db.
 * Non-virtual, protected constructors.
 */
struct db_base {
    ~db_base();

    // Non-copyable
    db_base(db_base const&) = delete;
    db_base& operator=(db_base const&) = delete;

    // Movable
    db_base(db_base&&) noexcept;
    db_base& operator=(db_base&&) noexcept;

    /**
     * @brief Close the database and flush all data
     */
    void close();

    /**
     * @brief Get the total number of UTXOs in the database
     */
    [[nodiscard]]
    size_t size() const;

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
    [[nodiscard]]
    size_t erase(raw_outpoint const& key, uint32_t height);

    /**
     * @brief Process all pending deferred deletions
     * @return Pair of (successful_deletions_count, failed_deletions)
     */
    [[nodiscard]]
    std::pair<uint32_t, std::vector<deferred_deletion_entry>> process_pending_deletions();

    /**
     * @brief Get the number of pending deferred deletions
     */
    [[nodiscard]]
    size_t deferred_deletions_size() const;

    /**
     * @brief Get the number of pending deferred lookups
     */
    [[nodiscard]]
    size_t deferred_lookups_size() const;

    /**
     * @brief Compact all containers
     */
    void compact_all();

    /**
     * @brief Iterate over all keys in the database
     *
     * @param f Callable with signature void(raw_outpoint const&)
     */
    template<typename F>
    void for_each_key(F&& f) const {
        for_each_key_impl([](void* ctx, raw_outpoint const& key) {
            (*static_cast<std::remove_reference_t<F>*>(ctx))(key);
        }, &f);
    }

    // Statistics
    [[nodiscard]] database_statistics get_statistics();
    void print_statistics();
    [[nodiscard]] sizing_report get_sizing_report() const;
    void print_sizing_report() const;
    void print_height_range_stats() const;
    void reset_all_statistics();
    [[nodiscard]] search_stats const& get_search_stats() const;
    void reset_search_stats();
    [[nodiscard]] float get_cache_hit_rate() const;
    [[nodiscard]] std::vector<std::pair<size_t, size_t>> get_cached_file_info() const;

protected:
    db_base();
    void for_each_key_impl(void(*cb)(void*, raw_outpoint const&), void* ctx) const;
    std::unique_ptr<detail::database_impl> impl_;
};

// =============================================================================
// full_db — full storage mode (variable-size values)
// =============================================================================

/**
 * @brief Full-mode UTXO Database
 *
 * Stores complete UTXO output data (scriptPubKey + amount) across 5 size-tiered
 * containers. Use this when you need the full transaction output data.
 */
struct full_db : db_base {
    full_db();
    ~full_db();

    full_db(full_db&&) noexcept;
    full_db& operator=(full_db&&) noexcept;

    /**
     * @brief Configure and open the database in full mode
     * @param path Database directory path
     * @param remove_existing If true, remove existing database files
     */
    void configure(std::string_view path, bool remove_existing = false);

    /**
     * @brief Configure for testing with smaller file sizes (full mode)
     */
    void configure_for_testing(std::string_view path, bool remove_existing = false);

    /**
     * @brief Insert a new UTXO with variable-size data
     * @param key UTXO key (transaction hash + output index)
     * @param value UTXO value data
     * @param height Block height where this UTXO was created
     * @return true if inserted successfully, false if already exists
     */
    [[nodiscard]]
    bool insert(raw_outpoint const& key, output_data_span value, uint32_t height);

    /**
     * @brief Find a UTXO by key
     * @param key UTXO key to search for
     * @param height Current block height (for statistics)
     * @return full_find_result {data, block_height} if found, std::nullopt otherwise
     */
    [[nodiscard]]
    std::optional<full_find_result> find(raw_outpoint const& key, uint32_t height) const;

    /**
     * @brief Process all pending deferred lookups
     * @return Pair of (successful_lookups_map, failed_lookups)
     */
    [[nodiscard]]
    std::pair<flat_map<raw_outpoint, full_find_result>, std::vector<deferred_lookup_entry>> process_pending_lookups();

    /**
     * @brief Iterate over all entries (key + value) in the database
     *
     * @param f Callable with signature void(raw_outpoint const&, uint32_t block_height, std::span<uint8_t const> data)
     */
    template<typename F>
    void for_each_entry(F&& f) const {
        for_each_entry_impl([](void* ctx, raw_outpoint const& key, uint32_t height, std::span<uint8_t const> data) {
            (*static_cast<std::remove_reference_t<F>*>(ctx))(key, height, data);
        }, &f);
    }

private:
    void for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, std::span<uint8_t const>), void* ctx) const;
};

// =============================================================================
// compact_db — compact storage mode (fixed-size reference)
// =============================================================================

/**
 * @brief Compact-mode UTXO Database
 *
 * Stores only a small fixed-size reference (block_height, file_number, offset)
 * in a single container. Use this when the node stores full block data on disk
 * and only needs to track which file/offset each UTXO lives at.
 */
struct compact_db : db_base {
    compact_db();
    ~compact_db();

    compact_db(compact_db&&) noexcept;
    compact_db& operator=(compact_db&&) noexcept;

    /**
     * @brief Configure and open the database in compact mode
     * @param path Database directory path
     * @param remove_existing If true, remove existing database files
     */
    void configure(std::string_view path, bool remove_existing = false);

    /**
     * @brief Configure for testing with smaller file sizes (compact mode)
     */
    void configure_for_testing(std::string_view path, bool remove_existing = false);

    /**
     * @brief Insert a new UTXO with typed compact fields
     * @param key UTXO key (transaction hash + output index)
     * @param file_number Block file number
     * @param offset Offset within the block file
     * @param height Block height where this UTXO was created
     * @return true if inserted successfully, false if already exists
     */
    [[nodiscard]]
    bool insert(raw_outpoint const& key, uint32_t file_number, uint32_t offset, uint32_t height);

    /**
     * @brief Find a UTXO by key
     * @param key UTXO key to search for
     * @param height Current block height (for statistics)
     * @return compact_find_result {block_height, file_number, offset} if found, std::nullopt otherwise
     */
    [[nodiscard]]
    std::optional<compact_find_result> find(raw_outpoint const& key, uint32_t height) const;

    /**
     * @brief Process all pending deferred lookups
     * @return Pair of (successful_lookups_map, failed_lookups)
     */
    [[nodiscard]]
    std::pair<flat_map<raw_outpoint, compact_find_result>, std::vector<deferred_lookup_entry>> process_pending_lookups();

    /**
     * @brief Iterate over all entries (key + compact fields) in the database
     *
     * @param f Callable with signature void(raw_outpoint const&, uint32_t height, uint32_t file_number, uint32_t offset)
     */
    template<typename F>
    void for_each_entry(F&& f) const {
        for_each_entry_impl([](void* ctx, raw_outpoint const& key, uint32_t height,
                               uint32_t file_number, uint32_t offset) {
            (*static_cast<std::remove_reference_t<F>*>(ctx))(key, height, file_number, offset);
        }, &f);
    }

private:
    void for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, uint32_t, uint32_t), void* ctx) const;
};

// =============================================================================
// Backward-compatible alias
// =============================================================================

using db = full_db;

} // namespace utxoz
