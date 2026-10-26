// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_delete_allocation.cpp
 * @brief apply_deletes under an allocation that fails, at every allocation it makes.
 *
 * A deletion writes as it goes, so the promise is progress, not atomicity:
 * whatever was erased is in `erased`, exactly, on the failure path too, and the
 * entry count agrees with the maps. An allocation failing after the first
 * erase must not escape the call — that would take the report with it, leaving
 * the caller with deletions it cannot see — and one failing between an erase and
 * its bookkeeping must not leave the key erased and uncounted.
 *
 * Nothing in the library is marked for this. operator new is replaced for the
 * whole program, which is why this is a binary of its own, and while a case is
 * armed it fails the N-th allocation made on the calling thread. N runs from 1
 * up, each time on a fresh database, until a call makes no allocation that
 * fails: every allocation the call makes is the failing one exactly once.
 *
 * The workload is built to make the statistics allocate inside the deletion:
 * every key is spent at an age and in a height range nothing has recorded yet.
 */

#include <array>
#include <atomic>
#include <cstdlib>
#include <map>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <utxoz/config.hpp>

#include "detail/durability.hpp"
#include "support/batch_workload.hpp"

using namespace utxoz;
using namespace utxoz::testing::batch;
using utxoz::testing::spread_key;

namespace {

// Only the thread that armed a case counts, and only while it is armed.
thread_local bool armed_here = false;
thread_local size_t allocations_seen = 0;
thread_local size_t fail_at = 0;
std::atomic<size_t> injected{0};

void* allocate(std::size_t n) {
    if (armed_here && ++allocations_seen == fail_at) {
        injected.fetch_add(1, std::memory_order_relaxed);
        throw std::bad_alloc();
    }
    if (void* p = std::malloc(n == 0 ? 1 : n)) return p;
    throw std::bad_alloc();
}

} // namespace

// Every unaligned form, new and delete alike, so that under a sanitizer each
// allocation is released by the same family that made it.
void* operator new(std::size_t n) { return allocate(n); }
void* operator new[](std::size_t n) { return allocate(n); }
void* operator new(std::size_t n, std::nothrow_t const&) noexcept {
    try { return allocate(n); } catch (...) { return nullptr; }
}
void* operator new[](std::size_t n, std::nothrow_t const&) noexcept {
    try { return allocate(n); } catch (...) { return nullptr; }
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::nothrow_t const&) noexcept { std::free(p); }
void operator delete[](void* p, std::nothrow_t const&) noexcept { std::free(p); }

namespace {

/// Fails the n-th allocation on this thread while it lives.
struct fail_nth {
    explicit fail_nth(size_t n) {
        injected = 0;
        allocations_seen = 0;
        fail_at = n;
        armed_here = true;
    }
    ~fail_nth() { armed_here = false; }
};

// Enough that the statistics have to grow partway through the batch, after
// earlier keys have been erased. A Boost flat map's first allocation makes room
// for 29 entries, so a batch smaller than that grows them, if at all, on its
// first key — before anything has been erased — and never reaches the window.
constexpr size_t stored_keys = 64;

/// Stored at heights 1..24, all in one height range; spent at heights that put
/// every key in a range and at an age of its own.
uint32_t stored_height(size_t i) { return uint32_t(1 + i); }
uint32_t spend_height(size_t i) { return uint32_t(30000 + i * 20000 + 7 * i); }

/// The batch: every stored key, two that were never stored, and one stored key
/// twice. Distinct keys: stored_keys + 2.
std::vector<deferred_deletion_entry> deletion_batch() {
    std::vector<deferred_deletion_entry> batch;
    for (size_t i = 0; i != stored_keys; ++i) batch.emplace_back(spread_key(i), spend_height(i));
    batch.emplace_back(spread_key(1000), spend_height(1));
    batch.emplace_back(spread_key(5), spend_height(5));
    batch.emplace_back(spread_key(1001), spend_height(2));
    return batch;
}

/// With `history`, the first half goes in before a forced rotation and the
/// second half after it, so the batch reaches a historical generation as well as
/// the active one. With `cached` as well, the historical file is opened once
/// beforehand, by a deletion of a key nobody stored, so that the walk meets it in
/// the file cache; without it, the walk has to open the file itself, and every
/// allocation that opening makes is one the sweep fails.
template <typename Db, typename Insert>
Db populate(fs::path const& dir, bool history, bool cached, Insert const& insert) {
    utxoz::detail::failpoints::scoped_reset const disarm;
    auto opened = Db::open_for_testing(dir, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);
    for (size_t i = 0; i != stored_keys; ++i) {
        if (history && i == stored_keys / 2) {
            utxoz::detail::failpoints::force_rotations.store(1, std::memory_order_relaxed);
        }
        insert(db, i);
    }
    if (history && cached) {
        std::vector<deferred_deletion_entry> const warm{{spread_key(9999), 1}};
        auto const r = db.apply_deletes(warm);
        REQUIRE_FALSE(r.error.has_value());
        REQUIRE(r.absent.size() == 1);
        REQUIRE(db.get_statistics().cached_files_count > 0);
    } else {
        REQUIRE(db.get_statistics().cached_files_count == 0);
    }
    return db;
}

full_db make_full(fs::path const& dir, bool history, bool cached) {
    return populate<full_db>(dir, history, cached, [](full_db& db, size_t i) {
        // Classes 0 to 2, so the active phase visits more than one container.
        REQUIRE(db.insert(spread_key(i), payload(i % 3, i), stored_height(i)).value());
    });
}

reference_db make_reference(fs::path const& dir, bool history, bool cached) {
    return populate<reference_db>(dir, history, cached, [](reference_db& db, size_t i) {
        REQUIRE(db.insert(spread_key(i), uint32_t(i + 1), uint32_t(i * 10), stored_height(i)).value());
    });
}

bool is_stored(raw_outpoint const& key) {
    for (size_t i = 0; i != stored_keys; ++i) if (spread_key(i) == key) return true;
    return false;
}

/// Whether the database still holds `key` anywhere: the active versions, then
/// the older ones.
template <typename Db>
bool holds(Db const& db, size_t i) {
    if (db.find(spread_key(i), spend_height(i)).has_value()) return true;
    std::vector<lookup_request> const ask{{spread_key(i), spend_height(i)}};
    auto const resolved = db.resolve(ask);
    REQUIRE(resolved.has_value());
    return resolved->found.contains(spread_key(i));
}

/// What one armed call came to.
struct sweep_totals {
    size_t points = 0;              // allocations the call made, each failed once
    size_t threw = 0;               // ... of which escaped the call
    size_t threw_after_erasing = 0; // ... after a key had already left its map
    size_t returned_with_error = 0;
    std::map<error_code, size_t> errors;   // what those errors were
};

/// What the database looked like just before the armed call.
struct before_call {
    size_t size = 0;
    std::set<raw_outpoint> historical;   // stored keys not in an active version
    bool already_dirty = false;          // the historical version was owed a barrier already
    size_t active_barriers = 0;          // what a sync owes when nothing else is owed
};

/// The checks every armed call is held to, whatever it returned or threw.
template <typename Db>
void check_after(Db& db, fs::path const& dir, bool history,
                 std::optional<deletion_progress> const& progress, before_call const& before,
                 sweep_totals& totals, Db (*reopen)(fs::path const&)) {
    using utxoz::detail::failpoints;
    size_t const size_before = before.size;
    auto const batch = deletion_batch();
    std::set<raw_outpoint> distinct;
    for (auto const& r : batch) distinct.insert(r.key);

    // Which keys are still stored, asked of the database itself.
    std::set<raw_outpoint> still_stored;
    for (size_t i = 0; i != stored_keys; ++i) {
        if (holds(db, i)) still_stored.insert(spread_key(i));
    }
    size_t const gone = stored_keys - still_stored.size();

    if ( ! progress) {
        // An exception is only an answer if nothing happened: then everything is
        // still owed, and resending the batch is right. After an erase it is a
        // report thrown away.
        ++totals.threw;
        if (gone != 0) ++totals.threw_after_erasing;
        CHECK(gone == 0);
        CHECK(db.size() == size_before);
    } else {
        // The only failure in these calls is the allocation the harness failed,
        // so an error is that and nothing else: not a file that would not open,
        // not a catalogue that would not list. An allocation the call absorbed
        // without consequence — in logging, which swallows its own — needs no
        // error, and invents none.
        if (progress->error) {
            ++totals.returned_with_error;
            CHECK(*progress->error == error_code::out_of_memory);
        }

        // Every distinct key exactly once, across the three lists.
        std::multiset<raw_outpoint> named;
        for (auto const* list : {&progress->erased, &progress->absent, &progress->unresolved})
            for (auto const& e : *list) named.insert(e.key);
        CHECK(named.size() == distinct.size());
        CHECK(std::set<raw_outpoint>(named.begin(), named.end()) == distinct);

        // erased is a fact: those keys are gone, and nothing else is.
        CHECK(progress->erased.size() == gone);
        for (auto const& e : progress->erased) CHECK_FALSE(still_stored.contains(e.key));
        // unresolved is owed: a stored key in it has not been touched.
        for (auto const& e : progress->unresolved) {
            if (is_stored(e.key)) CHECK(still_stored.contains(e.key));
        }
        // absent is a fact too: never stored, or not without an error.
        for (auto const& e : progress->absent) CHECK_FALSE(is_stored(e.key));
        if ( ! progress->error) CHECK(progress->unresolved.empty());

        CHECK(db.size() == size_before - progress->erased.size());
    }

    // Of what is gone, how much came out of the historical generation.
    size_t historical_gone = 0;
    for (auto const& key : before.historical) historical_gone += ! still_stored.contains(key);

    // Every deletion from a historical file updated that file's catalogue record,
    // and nothing else did.
    CHECK(failpoints::full_metadata_deletes.load() + failpoints::reference_metadata_deletes.load()
          == historical_gone);

#if UTXOZ_STATISTICS_LEVEL >= 1
    // The counters describe the deletions that happened, no more and no fewer.
    // Lifetimes are recorded by the active phase only; depths by the historical
    // walk only.
    auto const stats = db.get_statistics();
    CHECK(stats.total_entries == db.size());
    CHECK(stats.total_deletes == gone);
    CHECK(stats.lifetime.total_spent == gone - historical_gone);
    size_t by_depth = 0;
    for (auto const& [depth, count] : stats.deferred.deletions_by_depth) by_depth += count;
    CHECK(by_depth == historical_gone);
#endif

    // The obligation a sync has to meet: every active container, and the
    // historical version if it was written to — by this call, or before it and
    // not synced since. A sync that does fewer barriers returns success with a
    // modified file it never flushed. The count is of file barriers requested,
    // not of what reached the disk.
    if (history) {
        failpoints::sync_file_calls.store(0);
        REQUIRE(db.sync());
        bool const owed = before.already_dirty || historical_gone != 0;
        CHECK(failpoints::sync_file_calls.load() == before.active_barriers + (owed ? 1 : 0));
    }

    // The count the instance kept is the count the maps hold.
    size_t const kept = db.size();
    db.close();
    auto again = reopen(dir);
    CHECK(again.size() == kept);
    again.close();
}

/// `synced` decides what the historical version owes before the call. Rotated
/// out and not synced since, it is already on record and the call cannot lose
/// that; synced, it owes nothing, and only the call's own bookkeeping can put
/// it back on record.
template <typename Db>
sweep_totals sweep(Db (*make)(fs::path const&, bool, bool), Db (*reopen)(fs::path const&),
                   bool history, bool synced, bool cached) {
    using utxoz::detail::failpoints;
    auto const batch = deletion_batch();
    sweep_totals totals;
    for (size_t n = 1; n != 100000; ++n) {
        CAPTURE(n);
        temp_db t;
        auto db = make(t.dir, history, cached);

        before_call before;
        before.size = db.size();
        REQUIRE(before.size == stored_keys);
        for (size_t i = 0; i != stored_keys; ++i) {
            if ( ! db.find(spread_key(i), spend_height(i)).has_value()) {
                before.historical.insert(spread_key(i));
            }
        }
        REQUIRE(history == ! before.historical.empty());
        before.active_barriers = std::is_same_v<Db, full_db> ? utxoz::detail::slot_count : 1;
        before.already_dirty = history && ! synced;
        if (synced) REQUIRE(db.sync());
        failpoints::full_metadata_deletes.store(0);
        failpoints::reference_metadata_deletes.store(0);

        std::optional<deletion_progress> progress;
        {
            fail_nth const armed(n);
            try {
                progress = db.apply_deletes(batch);
            } catch (std::bad_alloc const&) {
            }
        }

        if (injected == 0) {
            // The call made fewer than n allocations: this is the run with none
            // failing, and it must be the plain answer.
            REQUIRE(progress.has_value());
            CHECK_FALSE(progress->error.has_value());
            CHECK(progress->erased.size() == stored_keys);
            CHECK(progress->absent.size() == 2);
            CHECK(progress->unresolved.empty());
            check_after(db, t.dir, history, progress, before, totals, reopen);
            return totals;
        }
        ++totals.points;
        if (progress && progress->error) ++totals.errors[*progress->error];
        check_after(db, t.dir, history, progress, before, totals, reopen);
    }
    FAIL("the sweep did not reach a call with no failing allocation");
    return totals;
}

full_db reopen_full(fs::path const& dir) {
    auto opened = full_db::open_for_testing(dir, false);
    REQUIRE(opened.has_value());
    return std::move(*opened);
}

reference_db reopen_reference(fs::path const& dir) {
    auto opened = reference_db::open_for_testing(dir, false);
    REQUIRE(opened.has_value());
    return std::move(*opened);
}

void report(char const* mode, sweep_totals const& t) {
    std::string errors;
    for (auto const& [code, count] : t.errors) {
        errors += " error " + std::to_string(int(code)) + " x" + std::to_string(count);
    }
    WARN(mode << ": " << t.points << " allocation points, " << t.threw << " threw ("
                  << t.threw_after_erasing << " after erasing), " << t.returned_with_error
                  << " returned with an error;" << errors);
}

} // namespace

TEST_CASE("the harness fails exactly the allocation it is aimed at", "[deletion][allocation]") {
    {
        fail_nth const armed(2);
        // Direct allocation-function calls cannot be elided like new-expressions.
        ::operator delete(::operator new(sizeof(int)));
        CHECK_THROWS_AS(::operator delete(::operator new(sizeof(int))), std::bad_alloc);
        ::operator delete(::operator new(sizeof(int)));
    }
    CHECK(injected == 1);
    CHECK_NOTHROW(::operator delete(::operator new(sizeof(int))));   // disarmed
}

namespace {

/// No history; history whose version is already owed a barrier; history synced
/// clean, so that the call's own bookkeeping is all that can owe one; and
/// history the walk has to open for itself rather than find in the cache.
struct shape { bool history; bool synced; bool cached; char const* name; };
constexpr std::array<shape, 4> shapes{{
    {false, true,  false, "no history"},
    {true,  false, true,  "history, already owed"},
    {true,  true,  true,  "history, synced clean"},
    {true,  true,  false, "history, not cached"},
}};

/// The batch with the historical file refusing to open and the catalogue
/// refusing to list, both at once. The file is in the cache, so the walk meets it
/// first; what the call reports is that, and the catalogue is never reached.
template <typename Db>
void check_first_failure_wins(Db (*make)(fs::path const&, bool, bool)) {
    using utxoz::detail::failpoints;
    temp_db t;
    auto db = make(t.dir, true, true);
    std::set<raw_outpoint> historical;
    for (size_t i = 0; i != stored_keys; ++i) {
        if ( ! db.find(spread_key(i), spend_height(i)).has_value()) historical.insert(spread_key(i));
    }
    REQUIRE_FALSE(historical.empty());

    failpoints::scoped_reset const disarm;
    failpoints::fail_historical_open_version.store(0);   // the one rotated-out version
    failpoints::fail_historical_catalog.store(true);
    auto const progress = db.apply_deletes(deletion_batch());

    REQUIRE(progress.error.has_value());
    CHECK(*progress.error == error_code::version_unreadable);
    // The active keys were erased; nothing historical was, and nothing is absent.
    CHECK(progress.erased.size() == stored_keys - historical.size());
    CHECK(progress.absent.empty());
    CHECK(progress.unresolved.size() == historical.size() + 2);
    for (auto const& e : progress.erased) CHECK_FALSE(historical.contains(e.key));
    CHECK(failpoints::full_metadata_deletes.load() + failpoints::reference_metadata_deletes.load()
          == 0);
    db.close();
}

} // namespace

TEST_CASE("error codes keep their numbers, and out_of_memory is appended",
          "[deletion][allocation]") {
    // Callers persist and compare these; none may move.
    static_assert(int(error_code::not_found) == 0);
    static_assert(int(error_code::storage_mode_mismatch) == 3);
    static_assert(int(error_code::catalog_unreadable) == 7);
    static_assert(int(error_code::version_unreadable) == 8);
    static_assert(int(error_code::file_open_failed) == 16);
    static_assert(int(error_code::recovery_failed) == 20);
    static_assert(int(error_code::format_unsupported) == 22);
    static_assert(int(error_code::database_identity_mismatch) == 28);
    static_assert(int(error_code::inspection_only) == 32);
    static_assert(int(error_code::database_not_found) == 33);
    static_assert(int(error_code::entry_corrupt) == 34);
    static_assert(int(error_code::config_missing) == 35);
    static_assert(int(error_code::out_of_memory) == 36);
    SUCCEED();
}

TEST_CASE("full: the first failure is the one reported, and the walk stops there",
          "[deletion][allocation]") {
    check_first_failure_wins<full_db>(&make_full);
}

TEST_CASE("reference: the first failure is the one reported, and the walk stops there",
          "[deletion][allocation]") {
    check_first_failure_wins<reference_db>(&make_reference);
}

TEST_CASE("full: a failing allocation anywhere in apply_deletes loses no progress",
          "[deletion][allocation]") {
    for (auto const& s : shapes) {
        DYNAMIC_SECTION(s.name) {
            auto const totals = sweep<full_db>(&make_full, &reopen_full, s.history, s.synced,
                                               s.cached);
            report((std::string("full, ") + s.name).c_str(), totals);
            CHECK(totals.points > 0);
            CHECK(totals.threw_after_erasing == 0);
        }
    }
}

TEST_CASE("reference: a failing allocation anywhere in apply_deletes loses no progress",
          "[deletion][allocation]") {
    for (auto const& s : shapes) {
        DYNAMIC_SECTION(s.name) {
            auto const totals = sweep<reference_db>(&make_reference, &reopen_reference,
                                                    s.history, s.synced, s.cached);
            report((std::string("reference, ") + s.name).c_str(), totals);
            CHECK(totals.points > 0);
            CHECK(totals.threw_after_erasing == 0);
        }
    }
}
