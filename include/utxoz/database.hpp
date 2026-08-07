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
 *
 * @par Threading
 * A database instance supports ONE mutating operation at a time, with no other
 * operation of any kind in flight. Mutating means insert(), erase(),
 * process_pending_deletions(), process_pending_lookups(), compact_all() and
 * close(). Serialising them is the caller's job.
 *
 * find() is the one exception, and only a partial one: concurrent find() calls
 * are permitted strictly while no insert, erase, rotation, sweep, compaction or
 * cache operation can run. Providing that reader/writer barrier is the caller's
 * responsibility — this library has no lock to lean on.
 *
 * What makes find() eligible at all is that every piece of shared state it
 * touches is itself thread-safe: it reads the active maps, records into sharded
 * atomic counters, and queues into a concurrent set. That removes the internal
 * writer; it does NOT make the active map safe against modification. Nothing
 * here does.
 *
 * Statistics are operations too, not free reads. get_statistics() is not const
 * — it recomputes the fragmentation counters as it goes — and
 * reset_search_stats() / reset_all_statistics() write by definition; the const
 * accessors read plain counters that insert() and erase() write. All of them
 * may overlap with find(), which writes nothing they look at beyond its own
 * sharded counters, but not with any mutation, and get_statistics() and the
 * reset calls not with each other either. A summary taken while find() is
 * recording is also not consistent across fields; see probe_stats.
 *
 * The restriction on everything else is structural, not incidental:
 * - The LRU file cache has no synchronisation, and it owns the memory mappings.
 *   Evicting an entry unmaps the segment, so a second thread reading a
 *   previously returned map is a use-after-unmap — a crash, not a torn read.
 *   The cache is touched by erase(), process_pending_deletions() and
 *   process_pending_lookups().
 * - The entry count, per-container statistics, deferred deletions and the file
 *   metadata are plain members mutated without atomics.
 * - A rotation (triggered from inside insert()) unmaps the whole active segment
 *   and briefly leaves the container pointer null. A concurrent find() would be
 *   reading unmapped memory — which is why "no mutation in flight" is a
 *   condition of the exception above, not a nicety.
 *
 * Separately from threading, the deferred-lookup queue is drained wholesale
 * (see process_pending_lookups()), so two *callers* steal each other's keys
 * even when properly serialised. That one is an ownership rule.
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
    /// @warning Diagnostic, not authoritative access to the stored state. It
    /// keeps answering after a failed cleanup has latched the instance, and in
    /// that state it counts entries that several files hold at once. Every
    /// operation that reads or changes what is stored reports
    /// error_code::recovery_required instead; this one, the queue sizes and the
    /// statistics do not, because they exist to describe a database that is in
    /// trouble.
    size_t size() const;

    /**
     * @brief Erase a UTXO by key
     *
     * @warning A return value of 0 is NOT authoritative. Like find(), erase()
     * only looks at the currently mapped (latest) version plus the cached
     * files; anything else is queued as a deferred deletion, and
     * process_pending_deletions() is what applies it.
     *
     * @warning The keys that call reports as failed are UNRESOLVED, not proven
     * absent: a version file that could not be read is logged, skipped, and its
     * keys land in the same list. Absence is only established when the sweep
     * read every version, which today you can tell apart only from the log.
     *
     * @param key UTXO key to erase
     * @param height Current block height
     * @return 1 if erased right away, 0 if the deletion was deferred (or the
     *         key does not exist)
     * @see process_pending_deletions()
     */
    [[nodiscard]]
    result<size_t> erase(raw_outpoint const& key, uint32_t height);

    /**
     * @brief Process all pending deferred deletions
     *
     * Sweeps the cached files and every previous version, applying the
     * deletions queued by erase(). Drains the queue: after this call nothing is
     * pending, so the returned values are the only report you get.
     *
     * @warning Single owner. The sweep is not partitioned per caller: it takes
     * the entire queue, including keys queued by other threads, and reports
     * them to whoever called. Exactly one component may own this call.
     *
     * @warning A version file that cannot be read is logged and skipped, so its
     * keys come back as failed — indistinguishable from genuinely absent. Do
     * not treat "failed" as proof of absence if the log shows read errors.
     *
     * @return Pair of (successful_deletions_count, failed_deletions).
     */
    [[nodiscard]]
    result<std::pair<uint32_t, std::vector<deferred_deletion_entry>>> process_pending_deletions();

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
    /**
     * @brief Compact all containers
     *
     * Merges entries out of older version files and removes the ones left
     * empty, so the number of files a sweep has to open goes down.
     *
     * @warning Fails with error_code::duplicate_key if it finds two stored
     * entries sharing a key. A stored key is unique across the whole database;
     * two entries for one key mean the database is locally inconsistent, which
     * compaction can see because it holds two version files open at once.
     * It reports rather than repairs: nothing is chosen between the copies and
     * neither is removed, because picking one would hide a corrupt database
     * behind a plausible answer. Treat it as fatal — the entry counter is left
     * untouched too, so it still describes what is on disk.
     *
     * @note A drained version file is removed and nothing is renumbered to close
     * the gap, so version numbers are not dense: they are identities, not
     * positions. Do not assume the files run 0..n, and do not derive a file
     * count from the highest version.
     *
     * @return empty on success, error otherwise
     */
    [[nodiscard]]
    result<> compact_all();

    /**
     * @brief Puts everything written so far on stable storage.
     *
     * Returns once the entries this database holds have reached the disk: the
     * active container of each size class, the older generations a batch's
     * deferred deletions reached through the file cache, and the directory
     * entries that name them. Until it returns, a power cut can lose writes
     * that every earlier call reported as successful — that is not a defect,
     * it is what buffered I/O is, and this is the call that ends it.
     *
     * @par How often
     * Not decided here. The store has no idea what a caller is willing to lose,
     * and a policy baked into it would be wrong for everyone: a node syncing
     * per block pays for durability it may not need below a checkpoint, and one
     * syncing per hour may not be able to answer what it has. The caller knows
     * the answer and calls this when it wants the guarantee.
     *
     * @par What it does not cover
     * Derived metadata. Losing a record costs a rescan and nothing else — an
     * absent or damaged one degrades to "unknown", which every consumer already
     * handles — so a barrier per record would buy nothing usable. What this
     * promises is that the entries are there.
     *
     * @par Retrying
     * A failure discharges nothing: every barrier this call owed is still owed
     * afterwards, so calling again is well defined and attempts all of them —
     * including the ones that had already succeeded, since a partial result is
     * not recorded. What retrying cannot do is make a disk that refused to
     * flush agree to. Treat a failure as fatal to whatever depended on the
     * guarantee rather than as a transient to spin on; a caller that does retry
     * is not left with a half-kept promise, which is the point.
     *
     * @par On a closed database
     * error_code::closed. There is nothing mapped and nothing this object still
     * holds, so reporting a successful sync would suggest a guarantee about a
     * database it has let go of. Note that closing does **not** make anything
     * durable: sync() before close() if the guarantee is wanted.
     *
     * @par Per platform
     * On POSIX the file and directory barriers both exist and this is a full
     * guarantee. On Windows the file contents are flushed and there is no
     * directory barrier, so the ordering between a rotation and the data it
     * publishes is weaker than POSIX gives — documented rather than assumed
     * equivalent. Under Emscripten the filesystem is virtual and there is no
     * stable storage to reach, so this returns error_code::sync_unsupported
     * rather than a success it cannot honour.
     *
     * Ask platform_durability() for that difference rather than inferring it:
     * success here means every barrier this platform *has* was crossed, which
     * under durability_level::contents_only is not the same as every barrier
     * the guarantee would need. A caller recording a checkpoint on the strength
     * of a sync should know which of the two it got.
     *
     * @return empty on success; sync_failed if a barrier was attempted and
     *         failed; sync_unsupported where the platform has none;
     *         recovery_required if this instance is latched.
     */
    [[nodiscard]]
    result<> sync();

    /**
     * @brief Iterate over all keys in the database
     *
     * @param f Callable with signature void(raw_outpoint const&)
     */
    template<typename F>
    [[nodiscard]]
    result<> for_each_key(F&& f) const {
        return for_each_key_impl([](void* ctx, raw_outpoint const& key) {
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
    void reset_search_stats();  ///< Clears the probe and resolution counters
    [[nodiscard]] float get_cache_hit_rate() const;
    [[nodiscard]] std::vector<std::pair<size_t, size_t>> get_cached_file_info() const;

protected:
    db_base();
    ~db_base();
    [[nodiscard]]
    result<> for_each_key_impl(void(*cb)(void*, raw_outpoint const&), void* ctx) const;
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
     * auto [found, unresolved] = db.process_pending_lookups();
     * // `found` resolves the queued lookups. `unresolved` is NOT proof of
     * // absence: a version file that could not be read lands its keys here too.
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
     * queued by find(). This is where a find() that returned not_found gets
     * resolved. Drains the queue: after this call nothing is pending, so
     * whatever you do not read from the returned map is lost.
     *
     * Call this before process_pending_deletions(), which removes entries from
     * the very files the pending lookups still need to read.
     *
     * @warning Single owner. The queue is global, not per caller: this call
     * takes every pending key — including any queued by a different caller —
     * and reports them all to whoever called it. Exactly one component may own
     * the sweep, and it is responsible for routing results back to whoever
     * asked. (This is about ownership, not threads: per the class contract,
     * operations are serialised anyway.)
     *
     * @warning The second element is UNRESOLVED, not absent. A version file that
     * cannot be read is logged and skipped, and its keys come back in that same
     * list, indistinguishable from keys that exist nowhere. Absence is only
     * established if the sweep read every version — which today you can tell
     * apart only from the log.
     *
     * @return Pair of (resolved_lookups_map, unresolved_lookups).
     */
    [[nodiscard]]
    result<std::pair<flat_map<raw_outpoint, full_find_result>, std::vector<deferred_lookup_entry>>> process_pending_lookups();

    /**
     * @brief Iterate over all entries (key + value) in the database
     *
     * @param f Callable with signature void(raw_outpoint const&, uint32_t block_height, std::span<uint8_t const> data)
     */
    template<typename F>
    [[nodiscard]]
    result<> for_each_entry(F&& f) const {
        return for_each_entry_impl([](void* ctx, raw_outpoint const& key, uint32_t height, std::span<uint8_t const> data) {
            (*static_cast<std::remove_reference_t<F>*>(ctx))(key, height, data);
        }, &f);
    }

private:
    full_db();
    [[nodiscard]]
    result<> for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, std::span<uint8_t const>), void* ctx) const;
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
     * Resolves the lookups queued by find() — see
     * full_db::process_pending_lookups() for the full contract: single owner,
     * drains the queue, and must run before process_pending_deletions().
     *
     * @warning The second element is UNRESOLVED, not absent: a version file that
     * could not be read lands its keys there too.
     *
     * @return Pair of (resolved_lookups_map, unresolved_lookups).
     */
    [[nodiscard]]
    result<std::pair<flat_map<raw_outpoint, compact_find_result>, std::vector<deferred_lookup_entry>>> process_pending_lookups();

    /**
     * @brief Iterate over all entries (key + compact fields) in the database
     *
     * @param f Callable with signature void(raw_outpoint const&, uint32_t height, uint32_t file_number, uint32_t offset)
     */
    template<typename F>
    [[nodiscard]]
    result<> for_each_entry(F&& f) const {
        return for_each_entry_impl([](void* ctx, raw_outpoint const& key, uint32_t height,
                               uint32_t file_number, uint32_t offset) {
            (*static_cast<std::remove_reference_t<F>*>(ctx))(key, height, file_number, offset);
        }, &f);
    }

private:
    compact_db();
    [[nodiscard]]
    result<> for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, uint32_t, uint32_t), void* ctx) const;
};

// =============================================================================
// Backward-compatible alias
// =============================================================================

using db = full_db;

} // namespace utxoz
