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
 * Not intended to be instantiated directly — use full_db::open() or compact_db::open().
 */
struct db_base {
    // Non-copyable
    db_base(db_base const&) = delete;
    db_base& operator=(db_base const&) = delete;

    // Movable
    db_base(db_base&&) noexcept;
    db_base& operator=(db_base&&) noexcept;

    /**
     * @brief Close the database and flush all data.
     * Safe to call multiple times. Also called by the destructor.
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
     * @warning A return value of 0 is NOT authoritative. Like find(), erase()
     * only looks at the currently mapped (latest) version plus the cached
     * files; anything else is queued as a deferred deletion. The definitive
     * answer comes from process_pending_deletions() — only keys reported there
     * as failed were really absent.
     *
     * @param key UTXO key to erase
     * @param height Current block height
     * @return 1 if erased right away, 0 if the deletion was deferred (or the
     *         key does not exist)
     * @see process_pending_deletions()
     */
    [[nodiscard]]
    size_t erase(raw_outpoint const& key, uint32_t height);

    /**
     * @brief Process all pending deferred deletions
     *
     * Sweeps the cached files and every previous version, applying the
     * deletions queued by erase(). Drains the queue: after this call nothing is
     * pending, so the returned values are the only report you get.
     *
     * @return Pair of (successful_deletions_count, failed_deletions). The failed
     *         entries are the keys that exist in no version of the database.
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
    ~db_base();
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
 *
 * Create via full_db::open() or full_db::open_for_testing().
 */
struct full_db : db_base {
    ~full_db();

    full_db(full_db&&) noexcept;
    full_db& operator=(full_db&&) noexcept;

    /**
     * @brief Open or create a database in full mode
     * @param path Database directory path
     * @param remove_existing If true, remove existing database files
     * @return full_db on success, error on failure
     */
    [[nodiscard]]
    static result<full_db> open(std::string_view path, bool remove_existing = false);

    /**
     * @brief Open for testing with smaller file sizes (full mode)
     */
    [[nodiscard]]
    static result<full_db> open_for_testing(std::string_view path, bool remove_existing = false);

    /**
     * @brief Insert a new UTXO with variable-size data
     * @param key UTXO key (transaction hash + output index)
     * @param value UTXO value data
     * @param height Block height where this UTXO was created
     * @return true if inserted, false if duplicate, error on failure
     */
    [[nodiscard]]
    result<bool> insert(raw_outpoint const& key, output_data_span value, uint32_t height);

    /**
     * @brief Find a UTXO by key
     *
     * @warning A not_found result is NOT authoritative. find() only searches the
     * currently mapped (latest) version of each container. Once a container has
     * rotated to a new file, entries left behind in previous versions are not
     * visible here: the lookup is queued as a deferred lookup and not_found is
     * returned. The definitive answer comes from process_pending_lookups(),
     * which sweeps the cached files and every previous version.
     *
     * Per-batch usage, mirroring the erase()/process_pending_deletions() pair:
     * @code
     * for (auto const& op : outpoints) {
     *     if (auto r = db.find(op, height)) { use(*r); }   // resolved right away
     *     // otherwise: queued, do NOT conclude "does not exist" yet
     * }
     * auto [found, missing] = db.process_pending_lookups();
     * // `found` resolves the queued lookups; only `missing` keys truly do not
     * // exist in any version.
     * @endcode
     *
     * Call process_pending_lookups() before process_pending_deletions(): the
     * latter removes entries from the previous-version files that the pending
     * lookups still need to read.
     *
     * @param key UTXO key to search for
     * @param height Current block height (for statistics)
     * @return full_find_result if found in the latest version; error not_found
     *         if the lookup was deferred (or the key does not exist)
     * @see process_pending_lookups()
     */
    [[nodiscard]]
    result<full_find_result> find(raw_outpoint const& key, uint32_t height) const;

    /**
     * @brief Process all pending deferred lookups
     *
     * Sweeps the cached files and every previous version, resolving the lookups
     * queued by find(). This is where a find() that returned not_found gets its
     * definitive answer. Drains the queue: after this call nothing is pending,
     * so whatever you do not read from the returned map is lost.
     *
     * Call this before process_pending_deletions(), which removes entries from
     * the very files the pending lookups still need to read.
     *
     * @return Pair of (successful_lookups_map, failed_lookups). The failed
     *         entries are the keys that exist in no version of the database.
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
    full_db();
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
 *
 * Create via compact_db::open() or compact_db::open_for_testing().
 */
struct compact_db : db_base {
    ~compact_db();

    compact_db(compact_db&&) noexcept;
    compact_db& operator=(compact_db&&) noexcept;

    /**
     * @brief Open or create a database in compact mode
     * @param path Database directory path
     * @param remove_existing If true, remove existing database files
     * @return compact_db on success, error on failure
     */
    [[nodiscard]]
    static result<compact_db> open(std::string_view path, bool remove_existing = false);

    /**
     * @brief Open for testing with smaller file sizes (compact mode)
     */
    [[nodiscard]]
    static result<compact_db> open_for_testing(std::string_view path, bool remove_existing = false);

    /**
     * @brief Insert a new UTXO with typed compact fields
     * @param key UTXO key (transaction hash + output index)
     * @param file_number Block file number
     * @param offset Offset within the block file
     * @param height Block height where this UTXO was created
     * @return true if inserted, false if duplicate, error on failure
     */
    [[nodiscard]]
    result<bool> insert(raw_outpoint const& key, uint32_t file_number, uint32_t offset, uint32_t height);

    /**
     * @brief Find a UTXO by key
     *
     * @warning A not_found result is NOT authoritative — see full_db::find().
     * Only the currently mapped (latest) version is searched; anything left in
     * a previous version is queued as a deferred lookup and resolved by
     * process_pending_lookups().
     *
     * @param key UTXO key to search for
     * @param height Current block height (for statistics)
     * @return compact_find_result if found in the latest version; error
     *         not_found if the lookup was deferred (or the key does not exist)
     * @see process_pending_lookups()
     */
    [[nodiscard]]
    result<compact_find_result> find(raw_outpoint const& key, uint32_t height) const;

    /**
     * @brief Process all pending deferred lookups
     *
     * Definitive answer for the lookups queued by find() — see
     * full_db::process_pending_lookups(). Drains the queue, and must run before
     * process_pending_deletions().
     *
     * @return Pair of (successful_lookups_map, failed_lookups). The failed
     *         entries are the keys that exist in no version of the database.
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
    compact_db();
    void for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, uint32_t, uint32_t), void* ctx) const;
};

// =============================================================================
// Backward-compatible alias
// =============================================================================

using db = full_db;

} // namespace utxoz
