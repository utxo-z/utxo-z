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
#include <optional>
#include <string_view>
#include <utility>

#include <utxoz/aliases.hpp>
#include <utxoz/census.hpp>
#include <utxoz/uniqueness.hpp>
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
// Together these hold one entry per DISTINCT key of the request span, not one
// per request: duplicates are collapsed, keeping the first occurrence. Match by
// key, not by position.

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
 * @brief What a batch of deletions actually did.
 *
 * Three lists, and every distinct key of the request span appears in exactly
 * one of them. They are separate because they are separate facts, and a caller
 * acts on each differently:
 *
 *  - `erased`     — the deletion was applied during this call.
 *  - `absent`     — the key is not stored, established after every version that
 *                   could hold it was read. It never stands for a file that
 *                   could not be opened, a catalogue that could not be listed,
 *                   or any other operational fault; those are `unresolved`.
 *  - `unresolved` — the obligation could not be completed. This is the only
 *                   category that may be resent.
 *
 * Not a `result<>`. A deletion mutates files as it goes, so a call that meets an
 * unreadable version has already applied part of its batch, and that part is a
 * fact the caller needs. Returning `unexpected` would hide it behind an error
 * and leave the caller unable to tell which keys are gone. `error` carries the
 * cause when there is one; the lists are always readable.
 *
 * @par The partition is over distinct keys, not requests
 * The batch is deduplicated by key, keeping the **first** occurrence of each —
 * its height included, so a caller that sent one outpoint at two heights gets
 * back the earlier one. Therefore
 *
 *     erased.size() + absent.size() + unresolved.size()
 *
 * is the number of **distinct keys** in the span, not `requests.size()`. Match
 * results back **by key**: the lists are not positional, are not parallel to the
 * span, and their sizes do not add up to the number of requests submitted.
 *
 * This holds on every path, including the ones that apply nothing: a closed or
 * recovery-latched instance returns the same deduplicated batch in
 * `unresolved`, because a refusal is still an answer about the same set of keys.
 *
 * @par erased is a fact, even when error is set
 * A deletion writes as it walks, so an error partway through leaves earlier
 * deletions applied. Those are enumerated exactly, in `erased`, and stay there:
 * an exception raised after a key left the map does not move it to `unresolved`
 * and does not drop it from the report. `erased` and `unresolved` never name the
 * same key.
 *
 * @par Resending
 * Resend `unresolved`, and **only** `unresolved`. This library keeps no record of
 * what a caller has already asked for, so an entry taken from `erased` and sent
 * again describes a key that is now genuinely not stored, and comes back in
 * `absent`. That is `absent` reporting the state of the database correctly; it
 * is not a signal that the earlier deletion failed. What a caller concludes from
 * `absent` is the caller's policy, and depends on how it recovers from an
 * interrupted operation.
 */
struct deletion_progress {
    std::vector<deferred_deletion_entry> erased;      ///< Applied during this call
    std::vector<deferred_deletion_entry> absent;      ///< Proven not stored
    std::vector<deferred_deletion_entry> unresolved;  ///< Not completed; resend these
    std::optional<error_code> error;                  ///< Why, when something stopped it
};

/**
 * @brief Base class with methods common to both storage modes.
 *
 * Not intended to be instantiated directly — use full_db::open() or reference_db::open().
 *
 * @par Threading
 * A database instance supports ONE mutating operation at a time, with no other
 * operation of any kind in flight. Mutating means insert(), apply_deletes(),
 * compact_all() and close(). Serialising those is the caller's job.
 *
 * The read path is different, and only the read path:
 *
 * - **resolve() may be called concurrently.** The library serialises
 *   resolutions against each other with a lock of its own, held for the whole
 *   call rather than around the cache lookups — long enough to cover the
 *   lifetime of every mapping reference the call obtains, because the file
 *   cache hands out references into segments it destroys on eviction and a
 *   second resolution evicting one mid-read is a use-after-unmap. Callers
 *   arrange nothing.
 * - **find() may run alongside resolve().** They touch disjoint state: find()
 *   reads the active containers and writes only its own sharded probe counters,
 *   while a resolution reads the older versions through the file cache and
 *   writes only the resolution counters. Eviction inside the cache cannot reach
 *   the active containers, which are separate mappings. Demonstrated rather than
 *   assumed — see the ThreadSanitizer cases in tests/test_lookup_ownership.cpp,
 *   which run both pairings with no lock of the caller's.
 *
 * apply_deletes() is on neither list. It mutates: it erases from the active
 * containers and writes through the cache's mappings, so it needs exclusion from
 * resolve(), find(), insert(), compaction and close() alike. The resolve lock
 * does not extend to it — that lock serialises resolutions against each other
 * and knows nothing about a deletion writing through the same segments.
 *
 * That is the whole of it. The lock covers resolve-vs-resolve; it does not make
 * the database thread-safe. Nothing above permits running either read
 * concurrently with insert(), a deletion, compaction, close(), or anything else
 * that mutates the active maps or writes through the cache's mappings — a
 * rotation inside insert() unmaps the active segment outright, and
 * apply_deletes() writes through the very mappings a resolution reads.
 *
 * Statistics are operations too, not free reads. get_statistics() is not const
 * — it recomputes the fragmentation counters as it goes — and
 * reset_search_stats() / reset_all_statistics() write by definition; the const
 * accessors read plain counters that insert() and apply_deletes() write. All of them
 * may overlap with find(), which writes nothing they look at beyond its own
 * sharded counters, but not with any mutation, and get_statistics() and the
 * reset calls not with each other either. A summary taken while find() is
 * recording is also not consistent across fields; see probe_stats.
 *
 * The restriction on everything else is structural, not incidental:
 * - The LRU file cache has no synchronisation of its own, and it owns the memory
 *   mappings. Evicting an entry unmaps the segment, so a second thread reading a
 *   previously returned map is a use-after-unmap — a crash, not a torn read.
 *   resolve() is safe against another resolve() because it holds the lock above
 *   across its whole use of those references. apply_deletes() writes through the
 *   same mappings and is not covered by that lock, so it stays the caller's to
 *   exclude.
 * - The entry count, the per-container statistics and the file metadata are
 *   plain members mutated without atomics.
 * - A rotation (triggered from inside insert()) unmaps the whole active segment
 *   and briefly leaves the container pointer null. A concurrent find() would be
 *   reading unmapped memory — which is why "no mutation in flight" is a
 *   condition of the exception above, not a nicety.
 *
 * @par Ownership
 * Neither lookups nor deletions have an ownership rule any more, because neither
 * has shared state to own. resolve() and apply_deletes() each answer the batch
 * they are handed and nothing else, so two components can keep their own batches
 * without arranging which of them is allowed to sweep, and neither can receive
 * or consume the other's requests.
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
    /// error_code::recovery_required instead; this one and the statistics do
    /// not, because they exist to describe a database that is in trouble.
    size_t size() const;

    /**
     * @brief Apply a caller's batch of deletions
     *
     * The one mutating entry point for removing entries. Each request is tried
     * against the active versions first, then the cached files, then every
     * version below the current one, and keys are dropped from the working set
     * as they are applied — so each further file is searched for fewer of them.
     *
     * The batch belongs to the caller throughout. This call reads the span and
     * keeps nothing: no request survives the return, so two components can each
     * apply their own batch without agreeing which of them is allowed to, and
     * neither can receive — or consume — a request the other made.
     *
     * See deletion_progress for what the three lists mean and which of them may
     * be resent. In short: every distinct key is classified exactly once,
     * `absent` is established only after full coverage and never stands for an
     * operational fault, and `unresolved` is the only category to send again.
     *
     * @warning Unlike resolve(), this cannot be transactional. It writes as it
     * goes, so a fault partway through leaves earlier deletions applied. That is
     * why progress is returned rather than an error: the applied part is
     * enumerated in `erased`, exactly, including on the failure path.
     *
     * @param requests The caller's batch; borrowed for the duration of the call
     * @return What was applied, what is proven absent, and what is still owed
     */
    [[nodiscard]]
    deletion_progress apply_deletes(std::span<deferred_deletion_entry const> requests);

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
     * deletions reached through the file cache, and the directory
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

    /**
     * @brief Count what is stored, by walking the files.
     *
     * Not a statistic: the counters in `get_statistics()` describe what this
     * process did since it opened the database, and this describes what is in it.
     * The cost is a full pass over every generation of every class, and the
     * report says how long it took and how much it read.
     *
     * `const` is a statement about this object and not about safety. It requires
     * the exclusive directory claim and no concurrent mutation. See census.hpp.
     */
    [[nodiscard]] result<census_report> census(census_options const& options = {}) const;

    /**
     * @brief Is any outpoint stored more than once, across every class and
     *        every generation?
     *
     * A separate operation and not a mode of `census()`: a census describes what
     * is there, and this decides whether what is there is allowed to be. The
     * report carries the verdict, and a database with duplicates is answered
     * rather than refused — see uniqueness.hpp.
     *
     * Considerably more expensive than a census: the data is re-read once per
     * partition group, inside `verify_options::memory_budget`, and the report
     * says how many passes that took.
     *
     * Same contract as `census()`: `const` is a statement about this object and
     * not about safety. It requires the exclusive directory claim and no
     * concurrent mutation.
     *
     * Fails with `insufficient_space` when the budget or the pass limit cannot
     * accommodate the walk — including when a single partition holds more copies
     * than the budget can hold at once, which no number of passes can help; with
     * `file_open_failed` or `entry_corrupt` when a generation cannot be read; with
     * `recovery_required` on a latched store, because an interrupted compaction
     * can leave a merged generation beside its sources and a verdict rendered on
     * that would accuse the database of something the interruption did; and with
     * `closed` on a closed object. There is deliberately **no** error for a
     * duplicate: that is a successful verification with `unique == false`.
     */
    [[nodiscard]] result<uniqueness_report> verify_unique_outpoints(
        verify_options const& options = {}) const;

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
     * @brief Open a database to look at it, creating nothing at all.
     *
     * `open()` makes a database where there is none, which is what a store should
     * do and what an instrument must not: pointed at a mistyped path it would
     * leave a new empty database behind and then report that it holds nothing.
     *
     * "Creates nothing" is meant literally, and that is what makes this a
     * different kind of open rather than a flag on the usual one. `open()` also
     * creates the active container of a **class that has no generations**, which
     * compaction can produce by draining one completely — an ordinary state of a
     * working database. For a store about to receive inserts, giving it version
     * zero back is right. For an instrument it is not: a ten-megabyte file made
     * on the way to measuring is a file the measurement then reports.
     *
     * So a class with no generations stays absent here, and is censused as zero
     * generations and zero entries. The consequence is the contract: **this
     * object supports `census()`, `verify_unique_outpoints()` and `close()`, and
     * refuses everything else with `inspection_only`** — there is no map for the
     * rest to work on, and making one is the thing being avoided.
     *
     * Whether a database exists is asked **under the directory claim**, so there
     * is no window between deciding and opening. A caller who checked for
     * themselves beforehand could only have that window.
     *
     * Fails with `database_not_found` when the directory is absent, when there is
     * no database in it, or when there is a config with no generations behind it
     * at all. Nothing is created — with one documented exception: taking the
     * claim creates `.utxoz.lock` if it is not there, and that file is permanent
     * by design (see database_lock). A lock file is not a database and is never
     * read as one, so what is left behind is a directory this call still refuses.
     *
     * A database that *is* there and is wrong returns the error saying so — a
     * corrupt config is `config_file_corrupt`, not `database_not_found`.
     */
    static result<full_db> open_for_inspection(std::filesystem::path path);
    static result<full_db> open_for_inspection_for_testing(std::filesystem::path path);

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
     * Per-batch usage. A complete function, so the return types agree and it
     * compiles as written:
     * @code
     * utxoz::result<void> validate(utxoz::full_db const& db,
     *                              std::span<utxoz::raw_outpoint const> outpoints,
     *                              uint32_t height) {
     *     std::vector<utxoz::lookup_request> pending;
     *     for (auto const& op : outpoints) {
     *         auto r = db.find(op, height);
     *         if (r) { use(*r); continue; }                 // resolved right away
     *         if (r.error() != utxoz::error_code::not_resolved) {
     *             return std::unexpected(r.error());
     *         }
     *         pending.emplace_back(op, height);             // mine, and still mine
     *     }
     *
     *     auto resolved = db.resolve(pending);
     *     if ( ! resolved) {
     *         // Could not read something it needed. `pending` is untouched — it
     *         // was never handed over, only borrowed — so retry it later and
     *         // treat nothing as missing in the meantime.
     *         return std::unexpected(resolved.error());
     *     }
     *
     *     // Matched back by key. The lists are not positional and are not
     *     // parallel to `pending`.
     *     for (auto const& [key, value] : resolved->found) { use_resolved(key, value); }
     *     for (auto const& missing : resolved->absent)     { reject(missing.key); }
     *     return {};
     * }
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
     * Duplicate keys collapse, and the results are not positional. The batch is
     * deduplicated by key, keeping the first occurrence of each — so a span
     * naming one outpoint three times asks one question and is answered once,
     * and `found.size() + absent.size()` equals the number of **distinct** keys
     * rather than `requests.size()`. Match results back by key; never by
     * position against the span, and never by count.
     *
     * A caller that sent the same outpoint at two heights gets one entry back,
     * carrying the height of the first occurrence.
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
     * Call this before apply_deletes(), which removes entries from
     * the very files a resolution still needs to read.
     *
     * @warning const means it does not change what is stored — it does move the
     * LRU file cache. Two threads may call it at once regardless: the library
     * serialises resolutions internally, for the whole call. That does not
     * extend to running it alongside insert(), a deletion, compaction or
     * close(). See the threading notes on db_base.
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

    /// Open a reference database to look at it, creating nothing. Supports
    /// census(), verify_unique_outpoints() and close(), and refuses the rest; see
    /// full_db::open_for_inspection().
    static result<reference_db> open_for_inspection(std::filesystem::path path);
    static result<reference_db> open_for_inspection_for_testing(std::filesystem::path path);

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
     * @warning const means it does not change what is stored. Two threads may
     * call it at once — resolutions are serialised internally — but not
     * alongside insert(), a deletion, compaction or close().
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
