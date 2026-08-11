// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file database.hpp
 * @brief Main database interface — db_base, full_db, reference_db
 */

#pragma once

#include <filesystem>
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

/**
 * @page utxoz_path_contract Database paths
 *
 * `open()` and `open_for_testing()` take a `std::filesystem::path` by value,
 * and the path reaches the operating system in its native form. On Windows that
 * means `wchar_t`; nothing in this library converts a path through the active
 * code page, so a database directory named in any script the filesystem accepts
 * opens, and reopens, as itself.
 *
 * It used to take `std::string_view`. On POSIX that was exact — a path is bytes
 * and `path::string()` hands those bytes back — but on Windows a
 * `std::filesystem::path` holds `wchar_t`, and `path::string()` converts through
 * the active code page: characters outside that page are replaced or the
 * conversion throws. A user whose data directory was named outside the machine's
 * code page could not open the database, and the failure arrived as a path that
 * did not exist rather than as an encoding problem (#109).
 *
 * @par What callers pass
 * `std::filesystem::path` is implicitly constructible from string literals,
 * `const char*`, `std::string` and `std::string_view`, so every call that
 * compiled against the old signature still compiles. There is deliberately no
 * second overload: a `std::string_view` overload alongside a
 * `std::filesystem::path` one makes `open("literal")` ambiguous, which would
 * break exactly the callers this is trying not to break.
 *
 * @par Migrating a caller that holds a path
 * Code that had to narrow a path to satisfy the old signature should stop:
 *
 * @code
 * db.open(dir.string());   // before — lossy on Windows outside the code page
 * db.open(dir);            // after  — the path is carried natively
 * @endcode
 *
 * @par Narrow strings
 * A `std::string` or `const char*` is still interpreted the way
 * `std::filesystem::path` interprets it, which on Windows is the native narrow
 * encoding rather than UTF-8. That is unchanged behaviour, and it is why a
 * caller holding a real path should pass the path rather than a string made
 * from it. To pass UTF-8 explicitly, build the path with
 * `std::filesystem::path(std::u8string(...))`.
 */

// =============================================================================
// db_base — shared methods for both storage modes
// =============================================================================

/**
 * @brief A batch of lookups resolved against the older versions (full mode).
 *
 * Two lists, and the difference between them is the contract: `found` holds the
 * entries that exist, `absent` holds the requests proven not to exist anywhere.
 * A request that could not be answered appears in neither, because a resolution
 * that could not cover everything returns an error and no lists at all.
 */
struct full_resolution {
    flat_map<raw_outpoint, full_find_result> found;  ///< Requests that resolved to an entry
    std::vector<lookup_request> absent;              ///< Requests proven absent
};

/**
 * @brief A batch of lookups resolved against the older versions (reference mode).
 *
 * Same contract as full_resolution, with the typed reference fields.
 */
struct reference_resolution {
    flat_map<raw_outpoint, reference_find_result> found;  ///< Requests that resolved to an entry
    std::vector<lookup_request> absent;                   ///< Requests proven absent
};

/**
 * @brief Base class with methods common to both storage modes.
 *
 * Not intended to be instantiated directly — use full_db::open() or reference_db::open().
 *
 * @par Threading
 * A database instance supports ONE mutating operation at a time, with no other
 * operation of any kind in flight. Mutating means insert(), erase(),
 * process_pending_deletions(), resolve(), compact_all() and close().
 * Serialising them is the caller's job.
 *
 * resolve() is const and still belongs on that list. Const here means it does
 * not change what is stored; it does move the LRU file cache, which owns the
 * memory mappings and has no synchronisation. Two threads resolving at once is
 * a use-after-unmap, not a slow path. What resolve() being caller-owned buys is
 * that batches cannot mix — not that they may run concurrently.
 *
 * find() is the one exception, and only a partial one: concurrent find() calls
 * are permitted strictly while no insert, erase, rotation, resolution,
 * compaction or cache operation can run. Providing that reader/writer barrier is
 * the caller's responsibility — this library has no lock to lean on.
 *
 * What makes find() eligible at all is that it reads the active maps and writes
 * nothing but sharded atomic counters. It holds no queue and registers no key:
 * a miss is reported to the caller and forgotten. That removes the internal
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
 *   The cache is touched by erase(), process_pending_deletions() and resolve().
 * - The entry count, per-container statistics, deferred deletions and the file
 *   metadata are plain members mutated without atomics.
 * - A rotation (triggered from inside insert()) unmaps the whole active segment
 *   and briefly leaves the container pointer null. A concurrent find() would be
 *   reading unmapped memory — which is why "no mutation in flight" is a
 *   condition of the exception above, not a nicety.
 *
 * @par Ownership
 * Lookups have no ownership rule any more, because they have no shared state to
 * own. resolve() answers the batch it is handed and nothing else, so two
 * components can each keep their own batch without arranging which of them is
 * allowed to sweep. Deferred *deletions* still work the old way: the queue is
 * global and process_pending_deletions() drains all of it, so that one call
 * still needs a single owner.
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
     * absent: a version file that could not be read is logged and skipped, and
     * its keys land in the same list. This is the deletion path, and it still
     * carries that ambiguity — resolve() no longer does.
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
     * @param path Database directory path. See @ref utxoz_path_contract.
     * @param remove_existing If true, remove existing database files
     * @return full_db on success, error on failure
     */
    [[nodiscard]]
    static result<full_db> open(std::filesystem::path path, bool remove_existing = false);

    /**
     * @brief Open for testing with smaller file sizes (full mode)
     * @param path Database directory path. See @ref utxoz_path_contract.
     */
    [[nodiscard]]
    static result<full_db> open_for_testing(std::filesystem::path path, bool remove_existing = false);

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
     * @brief Find a UTXO by key, in the active versions only
     *
     * @warning An error result is NOT absence. find() searches the currently
     * mapped (latest) version of each container and nothing else. Once a
     * container has rotated, entries left behind in previous versions are not
     * visible here, and the answer is error_code::not_resolved — which says
     * exactly that: not answerable from the active versions. resolve() is what
     * reaches the older ones.
     *
     * find() records nothing. It does not remember the key, does not queue it,
     * and does not expect a later call to pick it up; a caller that drops a
     * not_resolved on the floor has simply not asked the question. Keeping the
     * request is the caller's job, and keeping it is what makes the answer come
     * back to the caller that asked (#116).
     *
     * Per-batch usage:
     * @code
     * std::vector<utxoz::lookup_request> pending;
     * for (auto const& op : outpoints) {
     *     auto r = db.find(op, height);
     *     if (r) { use(*r); continue; }                    // resolved right away
     *     if (r.error() != utxoz::error_code::not_resolved) return r.error();
     *     pending.emplace_back(op, height);                // mine, and still mine
     * }
     *
     * auto resolved = db.resolve(pending);
     * if ( ! resolved) {
     *     // Could not read something it needed. `pending` is untouched — it was
     *     // never handed over, only borrowed — so retry it later and treat
     *     // nothing as missing in the meantime.
     *     return;
     * }
     * auto& [found, absent] = *resolved;
     * @endcode
     *
     * @param key UTXO key to search for
     * @param height Current block height (for statistics)
     * @return full_find_result if the active versions hold it; error
     *         error_code::not_resolved otherwise
     * @see resolve()
     */
    [[nodiscard]]
    result<full_find_result> find(raw_outpoint const& key, uint32_t height) const;

    /**
     * @brief Resolve a caller's batch of lookups against the older versions
     *
     * Walks the cached files and every version below the current one, answering
     * exactly the requests it was handed. Keys are dropped from its working set
     * as they are found, so each further file is searched for fewer of them.
     *
     * The batch belongs to the caller throughout. This call reads the span and
     * keeps nothing: no request survives the return, so two components can each
     * resolve their own batch without agreeing which of them is allowed to, and
     * neither can receive — or consume — a request the other made. That is the
     * whole reason this takes an argument instead of draining a queue (#116).
     *
     * Duplicate keys collapse. A batch naming the same outpoint twice asks one
     * question and gets one answer, in exactly one of the two lists.
     *
     * `absent` is ABSENT, and only that. A request reaches it exactly when every
     * version below the current one was read and the key was in none of them, so
     * it is a fact about the database and not about how the resolution happened
     * to go.
     *
     * If the resolution could not cover everything it needed to, this returns an
     * error and no lists at all, and the cause is kept rather than flattened:
     *
     *   - error_code::version_unreadable — a version file that the catalogue
     *     names could not be opened or read;
     *   - error_code::catalog_unreadable — the set of version files could not be
     *     enumerated, so which files exist is not known.
     *
     * They send an operator to different places, which is why they are not the
     * same code. On either, retry with the same span once the storage fault is
     * dealt with: nothing was consumed, so nothing has to be rebuilt.
     *
     * That distinction is the point. A caller that cannot tell "this outpoint
     * does not exist" from "this outpoint could not be looked up" turns a local
     * storage fault into a missing input, and rejects a block that may be
     * perfectly valid.
     *
     * Call this before process_pending_deletions(), which removes entries from
     * the very files a resolution still needs to read.
     *
     * @warning const means it does not change what is stored. It is not
     * concurrently callable: it moves the unsynchronised LRU file cache. See the
     * threading notes on db_base.
     *
     * @param requests The caller's batch; borrowed for the duration of the call
     * @return full_resolution, or error_code::version_unreadable /
     *         error_code::catalog_unreadable if it could not cover everything
     */
    [[nodiscard]]
    result<full_resolution> resolve(std::span<lookup_request const> requests) const;

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
// reference_db — reference storage mode (fixed-size reference)
// =============================================================================

/**
 * @brief Reference-mode UTXO Database
 *
 * Stores only a small fixed-size reference (block_height, file_number, offset)
 * in a single container. Use this when the node stores full block data on disk
 * and only needs to track which file/offset each UTXO lives at.
 *
 * Create via reference_db::open() or reference_db::open_for_testing().
 */
struct reference_db : db_base {
    ~reference_db();

    reference_db(reference_db&&) noexcept;
    reference_db& operator=(reference_db&&) noexcept;

    /**
     * @brief Open or create a database in reference mode
     * @param path Database directory path. See @ref utxoz_path_contract.
     * @param remove_existing If true, remove existing database files
     * @return reference_db on success, error on failure
     */
    [[nodiscard]]
    static result<reference_db> open(std::filesystem::path path, bool remove_existing = false);

    /**
     * @brief Open for testing with smaller file sizes (reference mode)
     * @param path Database directory path. See @ref utxoz_path_contract.
     */
    [[nodiscard]]
    static result<reference_db> open_for_testing(std::filesystem::path path, bool remove_existing = false);

    /**
     * @brief Insert a new UTXO with typed reference fields
     * @param key UTXO key (transaction hash + output index)
     * @param file_number Block file number
     * @param offset Offset within the block file
     * @param height Block height where this UTXO was created
     * @return true if inserted, false if duplicate, error on failure
     */
    [[nodiscard]]
    result<bool> insert(raw_outpoint const& key, uint32_t file_number, uint32_t offset, uint32_t height);

    /**
     * @brief Find a UTXO by key, in the active version only
     *
     * @warning An error result is NOT absence — see full_db::find(). Only the
     * currently mapped (latest) version is searched; anything left in a previous
     * version comes back as error_code::not_resolved, and resolve() is what
     * answers it. Nothing is recorded: keeping the request is the caller's job.
     *
     * @param key UTXO key to search for
     * @param height Current block height (for statistics)
     * @return reference_find_result if the active version holds it; error
     *         error_code::not_resolved otherwise
     * @see resolve()
     */
    [[nodiscard]]
    result<reference_find_result> find(raw_outpoint const& key, uint32_t height) const;

    /**
     * @brief Resolve a caller's batch of lookups against the older versions
     *
     * The same contract as full_db::resolve(), case for case, with the typed
     * reference fields: the batch stays the caller's, duplicate keys collapse,
     * `absent` means proven absent, and version_unreadable / catalog_unreadable
     * return no lists at all so the same span can simply be retried.
     *
     * @warning const means it does not change what is stored. It is not
     * concurrently callable: it moves the unsynchronised LRU file cache.
     *
     * @param requests The caller's batch; borrowed for the duration of the call
     * @return reference_resolution, or error_code::version_unreadable /
     *         error_code::catalog_unreadable.
     */
    [[nodiscard]]
    result<reference_resolution> resolve(std::span<lookup_request const> requests) const;

    /**
     * @brief Iterate over all entries (key + reference fields) in the database
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
    reference_db();
    [[nodiscard]]
    result<> for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, uint32_t, uint32_t), void* ctx) const;
};

// =============================================================================
// Backward-compatible alias
// =============================================================================

using db = full_db;

} // namespace utxoz
