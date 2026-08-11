// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_lookup_ownership.cpp
 * @brief A resolution answers the batch it was handed, and nothing else.
 *
 * UTXO-Z used to hold the pending lookups itself: find() wrote the key into a
 * database-wide set and process_pending_lookups() emptied all of it, so the
 * answer went to whoever swept next rather than to whoever asked. With one
 * component doing both, that is invisible. With two it is a time-of-check to
 * time-of-use race across two calls that were never atomic with respect to each
 * other — one caller's sweep resolves and clears another caller's keys, and the
 * second caller gets back a set that does not mention them. The mirror case is
 * worse: it hands a caller absences *proven on somebody else's behalf* (#116).
 *
 * The queue is gone. resolve(span) reads the caller's batch and keeps nothing,
 * so ownership is expressed by the argument instead of by a rule in the README.
 *
 * What that buys has to be checked rather than asserted, and most of these cases
 * are built so that a reappearing shared container fails them:
 *
 *  - a resolution returns only the keys its own batch named, however many finds
 *    ran first (a drained global queue would return those too);
 *  - two batches with disjoint keys each get their own results and only those;
 *  - two threads, each owning a batch, neither steal nor lose requests;
 *  - found and absent come back exactly, in the right lists;
 *  - a failure returns no lists at all, and the same vector retries whole;
 *  - full and reference behave identically, case for case.
 *
 * The fixtures rotate containers, which costs a few hundred thousand inserts, so
 * they are built once and shared. Nothing touches the shared ones — resolve()
 * and find() only read — so sharing is safe and the cases stay independent. The
 * one case that writes, the deletion counterpart below, builds its own database
 * rather than deleting out of a fixture the others are reading.
 */

#include <algorithm>
#include <array>
#include <atomic>
#include <barrier>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <numeric>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>

#include "detail/durability.hpp"

using utxoz::detail::failpoints;
using utxoz::lookup_request;

namespace {

std::string make_unique_path(std::string_view tag) {
    auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fmt::format("./test_own_{}_{}_{}", tag, getpid(), ts);
}

utxoz::raw_outpoint outpoint_of(uint64_t n, uint32_t index) {
    utxoz::raw_outpoint key{};
    std::memcpy(key.data(), &n, sizeof(n));
    key[20] = 0x7C;
    std::memcpy(key.data() + 32, &index, sizeof(index));
    return key;
}

std::vector<uint8_t> value_of(size_t size, uint8_t seed) {
    std::vector<uint8_t> v(size);
    std::iota(v.begin(), v.end(), seed);
    return v;
}

/// Keys that end up below the active version, and keys that were never stored.
///
/// Two disjoint groups, A and B, so a case can hand one batch to one owner and
/// the other to another and check that neither sees the other's keys.
struct corpus {
    static constexpr size_t group_size = 6;

    std::array<utxoz::raw_outpoint, group_size> a_stored{};
    std::array<utxoz::raw_outpoint, group_size> b_stored{};
    std::array<utxoz::raw_outpoint, group_size> a_absent{};
    std::array<utxoz::raw_outpoint, group_size> b_absent{};

    /// Inserted after the fill loop, so it is in the ACTIVE version and find()
    /// can answer it without touching a file. The find-vs-resolve cases need a
    /// key that exercises find()'s hit path, not only its miss path.
    utxoz::raw_outpoint active_witness{};

    static corpus make() {
        corpus c;
        for (size_t i = 0; i < group_size; ++i) {
            c.a_stored[i] = outpoint_of(1'000 + i, 0);
            c.b_stored[i] = outpoint_of(2'000 + i, 0);
            c.a_absent[i] = outpoint_of(900'000'000ULL + i, 3);
            c.b_absent[i] = outpoint_of(950'000'000ULL + i, 4);
        }
        c.active_witness = outpoint_of(3'000, 0);
        return c;
    }
};

std::vector<lookup_request> batch_of(std::span<utxoz::raw_outpoint const> keys, uint32_t height) {
    std::vector<lookup_request> batch;
    batch.reserve(keys.size());
    for (auto const& k : keys) batch.emplace_back(k, height);
    return batch;
}

/// Removes its directory when the process ends, so a shared fixture does not
/// leak a database into the working tree.
struct scoped_path {
    std::string path;
    explicit scoped_path(std::string p) : path(std::move(p)) {}
    scoped_path(scoped_path const&) = delete;
    scoped_path& operator=(scoped_path const&) = delete;
    // Movable so a builder lambda can return one. The moved-from path is cleared
    // rather than left behind, or the temporary's destructor would delete the
    // directory the surviving object still names.
    scoped_path(scoped_path&& other) noexcept : path(std::move(other.path)) { other.path.clear(); }
    ~scoped_path() {
        if (path.empty()) return;
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

// -----------------------------------------------------------------------------
// full mode
// -----------------------------------------------------------------------------

corpus const& keys() {
    static corpus const c = corpus::make();
    return c;
}

/// Fills with filler entries until container 0 has rotated `rotations` times, so
/// everything inserted before the call sits below the active version.
///
/// Shared, because a case that must not touch the read-only fixture needs its own
/// database and should not restate this. The filler keys are the only thing it
/// writes, and no case names them.
void fill_until_rotations(utxoz::full_db& db, size_t rotations) {
    uint64_t n = 10'000;
    uint32_t height = 2;
    for (int round = 0; round < 400; ++round) {
        if (db.get_statistics().rotations_per_container[0] >= rotations) break;
        for (int i = 0; i < 5'000; ++i, ++n) {
            (void) db.insert(outpoint_of(n, 0), utxoz::output_data_span{value_of(33, 7)}, height);
        }
        ++height;
    }
    REQUIRE(db.get_statistics().rotations_per_container[0] >= rotations);
}

/// Fills until container 0 has rotated twice, so every corpus key sits below the
/// active version and resolve() has several files to walk.
scoped_path const& full_fixture() {
    static scoped_path const fixture = [] {
        scoped_path p{make_unique_path("full")};
        auto opened = utxoz::full_db::open_for_testing(p.path, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        auto const& c = keys();
        for (size_t i = 0; i < corpus::group_size; ++i) {
            REQUIRE(db.insert(c.a_stored[i], utxoz::output_data_span{value_of(33, static_cast<uint8_t>(i))}, 1).value());
            REQUIRE(db.insert(c.b_stored[i], utxoz::output_data_span{value_of(33, static_cast<uint8_t>(100 + i))}, 1).value());
        }

        fill_until_rotations(db, 2);

        // Last, so it is in the active version.
        REQUIRE(db.insert(c.active_witness, utxoz::output_data_span{value_of(33, 0xC3)}, 9'999).value());

        db.close();
        return p;
    }();
    return fixture;
}

utxoz::full_db open_full() {
    auto opened = utxoz::full_db::open_for_testing(full_fixture().path, false);
    REQUIRE(opened.has_value());
    return std::move(*opened);
}

struct failpoint_guard {
    failpoint_guard() { failpoints::clear(); }
    ~failpoint_guard() { failpoints::clear(); }
};

} // namespace

// =============================================================================
// 1 & 10. find() keeps nothing, so a resolution cannot inherit anybody's probes
// =============================================================================

/**
 * The control that would fail first if a shared queue came back.
 *
 * Every corpus key is probed — 24 misses — and then a batch naming exactly one
 * of them is resolved. With the old global queue, that call drained all 24 and
 * returned them; here it must return one. The assertion is on the totals, so a
 * queue that merely leaked a few keys fails it too.
 */
TEST_CASE("full: a resolution returns only its own batch, however many finds ran first",
          "[ownership][full][negative]") {
    failpoint_guard guard;
    auto db = open_full();
    auto const& c = keys();

    size_t probed = 0;
    for (auto const* group : {&c.a_stored, &c.b_stored, &c.a_absent, &c.b_absent}) {
        for (auto const& k : *group) {
            auto const r = db.find(k, 100'000);
            REQUIRE_FALSE(r.has_value());
            REQUIRE(r.error() == utxoz::error_code::not_resolved);
            ++probed;
        }
    }
    REQUIRE(probed == corpus::group_size * 4);

    // One key. Not one of the twenty-four that were just probed and dropped.
    std::vector<lookup_request> const one{{c.a_stored[0], 100'000}};
    auto const resolved = db.resolve(one);
    REQUIRE(resolved.has_value());

    CHECK(resolved->found.size() == 1);
    CHECK(resolved->absent.empty());
    REQUIRE(resolved->found.contains(c.a_stored[0]));

    // And an empty batch is empty, not "everything pending".
    auto const nothing = db.resolve(std::span<lookup_request const>{});
    REQUIRE(nothing.has_value());
    CHECK(nothing->found.empty());
    CHECK(nothing->absent.empty());

    db.close();
}

// =============================================================================
// 2. Two batches, disjoint keys, no leakage either way
// =============================================================================

TEST_CASE("full: independent batches return only their own results",
          "[ownership][full]") {
    failpoint_guard guard;
    auto db = open_full();
    auto const& c = keys();

    auto const batch_a = batch_of(c.a_stored, 100'000);
    auto const batch_b = batch_of(c.b_stored, 100'000);

    auto const a = db.resolve(batch_a);
    auto const b = db.resolve(batch_b);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());

    CHECK(a->found.size() == corpus::group_size);
    CHECK(b->found.size() == corpus::group_size);

    for (size_t i = 0; i < corpus::group_size; ++i) {
        // Each holds all of its own...
        CHECK(a->found.contains(c.a_stored[i]));
        CHECK(b->found.contains(c.b_stored[i]));
        // ...and none of the other's. This is the assertion the old queue could
        // not have satisfied: it answered with whatever was pending.
        CHECK_FALSE(a->found.contains(c.b_stored[i]));
        CHECK_FALSE(b->found.contains(c.a_stored[i]));
    }

    // B resolving did not consume A's requests: A resolves again, identically.
    auto const a_again = db.resolve(batch_a);
    REQUIRE(a_again.has_value());
    CHECK(a_again->found.size() == corpus::group_size);
    CHECK(batch_a.size() == corpus::group_size);

    db.close();
}

// =============================================================================
// 3. Two owners on two threads
// =============================================================================

/**
 * Two threads, each owning a batch, resolving over and over.
 *
 * The mutex is deliberate and is not what is under test. resolve() is const in
 * the sense that it does not change what is stored, but it moves the LRU file
 * cache, which has no synchronisation — the class contract has always required
 * the caller to serialise anything that touches it. What this checks is the
 * property that serialisation does *not* give you: that two owners interleaved
 * in an order nobody controls still each receive exactly their own batch. Under
 * the old queue this failed regardless of locking, because the sweep took
 * everything pending no matter who had queued it.
 */
TEST_CASE("full: two threads resolving their own batches neither mix nor steal",
          "[ownership][full][concurrency]") {
    failpoint_guard guard;
    auto db = open_full();
    auto const& c = keys();

    auto const batch_a = batch_of(c.a_stored, 100'000);
    auto const batch_b = batch_of(c.b_stored, 100'000);

    constexpr int rounds = 25;
    std::mutex db_mutex;
    std::barrier sync(2);
    std::atomic<int> mismatches{0};

    auto worker = [&](std::vector<lookup_request> const& mine,
                      std::array<utxoz::raw_outpoint, corpus::group_size> const& expected,
                      std::array<utxoz::raw_outpoint, corpus::group_size> const& foreign) {
        for (int round = 0; round < rounds; ++round) {
            sync.arrive_and_wait();   // maximise the overlap between the two owners
            utxoz::result<utxoz::full_resolution> got = utxoz::full_resolution{};
            {
                std::scoped_lock lock(db_mutex);
                got = db.resolve(mine);
            }
            if ( ! got) { ++mismatches; continue; }
            if (got->found.size() != corpus::group_size) ++mismatches;
            if ( ! got->absent.empty()) ++mismatches;
            for (auto const& k : expected) {
                if ( ! got->found.contains(k)) ++mismatches;
            }
            for (auto const& k : foreign) {
                if (got->found.contains(k)) ++mismatches;   // stolen from the other owner
            }
        }
    };

    std::thread ta(worker, std::cref(batch_a), std::cref(c.a_stored), std::cref(c.b_stored));
    std::thread tb(worker, std::cref(batch_b), std::cref(c.b_stored), std::cref(c.a_stored));
    ta.join();
    tb.join();

    CHECK(mismatches.load() == 0);

    // Neither batch was consumed by all that resolving.
    CHECK(batch_a.size() == corpus::group_size);
    CHECK(batch_b.size() == corpus::group_size);

    db.close();
}

// =============================================================================
// 4. Found and absent, exactly
// =============================================================================

TEST_CASE("full: a mixed batch returns both categories exactly", "[ownership][full]") {
    failpoint_guard guard;
    auto db = open_full();
    auto const& c = keys();

    std::vector<lookup_request> batch;
    for (size_t i = 0; i < corpus::group_size; ++i) {
        batch.emplace_back(c.a_stored[i], 100'000);
        batch.emplace_back(c.a_absent[i], 100'000);
    }

    auto const resolved = db.resolve(batch);
    REQUIRE(resolved.has_value());

    REQUIRE(resolved->found.size() == corpus::group_size);
    REQUIRE(resolved->absent.size() == corpus::group_size);

    for (size_t i = 0; i < corpus::group_size; ++i) {
        CHECK(resolved->found.contains(c.a_stored[i]));
        CHECK(resolved->found.at(c.a_stored[i]).data == value_of(33, static_cast<uint8_t>(i)));
    }

    std::vector<utxoz::raw_outpoint> absent_keys;
    for (auto const& e : resolved->absent) absent_keys.push_back(e.key);
    for (size_t i = 0; i < corpus::group_size; ++i) {
        CHECK(std::ranges::find(absent_keys, c.a_absent[i]) != absent_keys.end());
        // A stored key must never be called absent.
        CHECK(std::ranges::find(absent_keys, c.a_stored[i]) == absent_keys.end());
    }

    // Every request landed in exactly one list, once.
    CHECK(resolved->found.size() + resolved->absent.size() == batch.size());

    db.close();
}

TEST_CASE("full: a duplicated key is one question and one answer", "[ownership][full]") {
    failpoint_guard guard;
    auto db = open_full();
    auto const& c = keys();

    std::vector<lookup_request> const batch{
        {c.a_stored[0], 100'000}, {c.a_stored[0], 100'001},
        {c.a_absent[0], 100'000}, {c.a_absent[0], 100'002}, {c.a_absent[0], 100'003},
    };

    auto const resolved = db.resolve(batch);
    REQUIRE(resolved.has_value());
    CHECK(resolved->found.size() == 1);
    // Not three. A repeated key that came back repeatedly would read as three
    // missing inputs where the caller asked about one.
    CHECK(resolved->absent.size() == 1);
    CHECK(resolved->absent[0].key == c.a_absent[0]);

    db.close();
}

// =============================================================================
// 5, 6, 7 & 9. Failure returns nothing, and the same vector retries whole
// =============================================================================

TEST_CASE("full: an unreadable version yields no partial results and no absences",
          "[ownership][full][negative]") {
    failpoint_guard guard;
    auto db = open_full();
    auto const& c = keys();

    std::vector<lookup_request> batch;
    for (size_t i = 0; i < corpus::group_size; ++i) {
        batch.emplace_back(c.a_stored[i], 100'000);
        batch.emplace_back(c.a_absent[i], 100'000);
    }
    auto const size_before = batch.size();

    auto const before = db.get_statistics();

    // Version 1 is neither the first file walked nor the only one, so the
    // failure lands after entries have already been resolved into the local
    // result. None of that may escape.
    failpoints::fail_historical_open_version.store(1, std::memory_order_relaxed);

    auto const failed = db.resolve(batch);
    REQUIRE_FALSE(failed.has_value());
    CHECK(failed.error() == utxoz::error_code::version_unreadable);

    // No lists at all: there is no partial result to misread as absence.
    // (result<> carries the error instead of a value, so this is structural —
    // the check that matters is that the batch is intact and the numbers moved
    // nowhere.)
    CHECK(batch.size() == size_before);

    auto const after_failure = db.get_statistics();
    CHECK(after_failure.deferred.processing_runs == before.deferred.processing_runs);
    CHECK(after_failure.deferred.successfully_processed == before.deferred.successfully_processed);
    CHECK(after_failure.resolution.files_visited == before.resolution.files_visited);
    CHECK(after_failure.resolution.cache_hits == before.resolution.cache_hits);
    CHECK(after_failure.resolution.resolved == before.resolution.resolved);
    CHECK(after_failure.resolution.absent == before.resolution.absent);

    // The retry uses the same vector the caller has been holding all along —
    // nothing to rebuild, because nothing was taken — and is complete.
    failpoints::clear();
    auto const retried = db.resolve(batch);
    REQUIRE(retried.has_value());
    CHECK(retried->found.size() == corpus::group_size);
    CHECK(retried->absent.size() == corpus::group_size);

    auto const after_retry = db.get_statistics();

    // A resolution never touches the deletion counters, in either build.
    CHECK(after_retry.deferred.processing_runs == before.deferred.processing_runs);
    CHECK(after_retry.deferred.successfully_processed == before.deferred.successfully_processed);
    CHECK(after_retry.deferred.failed_to_delete == before.deferred.failed_to_delete);

#ifdef UTXOZ_STATISTICS_ENABLED
    // Counted once: the abandoned attempt left nothing behind.
    CHECK(after_retry.resolution.resolved == before.resolution.resolved + corpus::group_size);
    CHECK(after_retry.resolution.absent == before.resolution.absent + corpus::group_size);
#endif

    db.close();
}

TEST_CASE("full: an unlistable catalogue keeps its own cause and consumes nothing",
          "[ownership][full][negative]") {
    failpoint_guard guard;
    auto db = open_full();
    auto const& c = keys();

    auto const batch = batch_of(c.a_stored, 100'000);

    failpoints::fail_historical_catalog.store(true, std::memory_order_relaxed);
    auto const failed = db.resolve(batch);
    REQUIRE_FALSE(failed.has_value());
    // Not version_unreadable: not knowing which files exist sends an operator
    // somewhere else than a file that will not open.
    CHECK(failed.error() == utxoz::error_code::catalog_unreadable);
    CHECK(batch.size() == corpus::group_size);

    failpoints::clear();
    auto const retried = db.resolve(batch);
    REQUIRE(retried.has_value());
    CHECK(retried->found.size() == corpus::group_size);
    CHECK(retried->absent.empty());

    db.close();
}

/// A failure for one owner is not a failure for the other's data: once the fault
/// is cleared, the second batch resolves whole. Under the old queue the first
/// caller's failed sweep and the second caller's keys shared one container, so
/// this pairing could not even be expressed.
TEST_CASE("full: one owner's failed resolution leaves another's batch resolvable",
          "[ownership][full][negative]") {
    failpoint_guard guard;
    auto db = open_full();
    auto const& c = keys();

    auto const batch_a = batch_of(c.a_stored, 100'000);
    auto const batch_b = batch_of(c.b_stored, 100'000);

    failpoints::fail_historical_open_version.store(1, std::memory_order_relaxed);
    REQUIRE_FALSE(db.resolve(batch_a).has_value());
    failpoints::clear();

    auto const b = db.resolve(batch_b);
    REQUIRE(b.has_value());
    CHECK(b->found.size() == corpus::group_size);
    for (size_t i = 0; i < corpus::group_size; ++i) {
        CHECK(b->found.contains(c.b_stored[i]));
        CHECK_FALSE(b->found.contains(c.a_stored[i]));
    }

    auto const a = db.resolve(batch_a);
    REQUIRE(a.has_value());
    CHECK(a->found.size() == corpus::group_size);

    db.close();
}

// =============================================================================
// 8. reference mode — the same contract, case for case
// =============================================================================

namespace {

scoped_path const& reference_fixture() {
    static scoped_path const fixture = [] {
        scoped_path p{make_unique_path("ref")};
        auto opened = utxoz::reference_db::open_for_testing(p.path, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        auto const& c = keys();
        for (size_t i = 0; i < corpus::group_size; ++i) {
            REQUIRE(db.insert(c.a_stored[i], static_cast<uint32_t>(i), static_cast<uint32_t>(10 + i), 1).value());
            REQUIRE(db.insert(c.b_stored[i], static_cast<uint32_t>(100 + i), static_cast<uint32_t>(20 + i), 1).value());
        }

        uint64_t n = 10'000;
        uint32_t height = 2;
        for (int round = 0; round < 400; ++round) {
            if (db.get_statistics().rotations_per_container[0] >= 2) break;
            for (int i = 0; i < 5'000; ++i, ++n) {
                (void) db.insert(outpoint_of(n, 0), static_cast<uint32_t>(n % 1000),
                                 static_cast<uint32_t>(n), height);
            }
            ++height;
        }
        REQUIRE(db.get_statistics().rotations_per_container[0] >= 2);

        REQUIRE(db.insert(c.active_witness, 777, 888, 9'999).value());

        db.close();
        return p;
    }();
    return fixture;
}

utxoz::reference_db open_reference() {
    auto opened = utxoz::reference_db::open_for_testing(reference_fixture().path, false);
    REQUIRE(opened.has_value());
    return std::move(*opened);
}

} // namespace

TEST_CASE("reference: a resolution returns only its own batch, however many finds ran first",
          "[ownership][reference][negative]") {
    failpoint_guard guard;
    auto db = open_reference();
    auto const& c = keys();

    for (auto const* group : {&c.a_stored, &c.b_stored, &c.a_absent, &c.b_absent}) {
        for (auto const& k : *group) {
            auto const r = db.find(k, 100'000);
            REQUIRE_FALSE(r.has_value());
            REQUIRE(r.error() == utxoz::error_code::not_resolved);
        }
    }

    std::vector<lookup_request> const one{{c.a_stored[0], 100'000}};
    auto const resolved = db.resolve(one);
    REQUIRE(resolved.has_value());
    CHECK(resolved->found.size() == 1);
    CHECK(resolved->absent.empty());
    REQUIRE(resolved->found.contains(c.a_stored[0]));
    CHECK(resolved->found.at(c.a_stored[0]).file_number == 0u);
    CHECK(resolved->found.at(c.a_stored[0]).offset == 10u);

    db.close();
}

TEST_CASE("reference: independent batches return only their own results",
          "[ownership][reference]") {
    failpoint_guard guard;
    auto db = open_reference();
    auto const& c = keys();

    auto const batch_a = batch_of(c.a_stored, 100'000);
    auto const batch_b = batch_of(c.b_stored, 100'000);

    auto const a = db.resolve(batch_a);
    auto const b = db.resolve(batch_b);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());

    CHECK(a->found.size() == corpus::group_size);
    CHECK(b->found.size() == corpus::group_size);
    for (size_t i = 0; i < corpus::group_size; ++i) {
        CHECK(a->found.contains(c.a_stored[i]));
        CHECK_FALSE(a->found.contains(c.b_stored[i]));
        CHECK(b->found.contains(c.b_stored[i]));
        CHECK_FALSE(b->found.contains(c.a_stored[i]));
    }

    db.close();
}

TEST_CASE("reference: a mixed batch returns both categories exactly", "[ownership][reference]") {
    failpoint_guard guard;
    auto db = open_reference();
    auto const& c = keys();

    std::vector<lookup_request> batch;
    for (size_t i = 0; i < corpus::group_size; ++i) {
        batch.emplace_back(c.b_stored[i], 100'000);
        batch.emplace_back(c.b_absent[i], 100'000);
    }

    auto const resolved = db.resolve(batch);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->found.size() == corpus::group_size);
    REQUIRE(resolved->absent.size() == corpus::group_size);
    CHECK(resolved->found.size() + resolved->absent.size() == batch.size());

    for (size_t i = 0; i < corpus::group_size; ++i) {
        REQUIRE(resolved->found.contains(c.b_stored[i]));
        CHECK(resolved->found.at(c.b_stored[i]).file_number == static_cast<uint32_t>(100 + i));
        CHECK(resolved->found.at(c.b_stored[i]).offset == static_cast<uint32_t>(20 + i));
    }

    db.close();
}

TEST_CASE("reference: an unreadable version yields no partial results, and the retry is whole",
          "[ownership][reference][negative]") {
    failpoint_guard guard;
    auto db = open_reference();
    auto const& c = keys();

    std::vector<lookup_request> batch;
    for (size_t i = 0; i < corpus::group_size; ++i) {
        batch.emplace_back(c.a_stored[i], 100'000);
        batch.emplace_back(c.a_absent[i], 100'000);
    }

    auto const before = db.get_statistics();

    failpoints::fail_historical_open_version.store(1, std::memory_order_relaxed);
    auto const failed = db.resolve(batch);
    REQUIRE_FALSE(failed.has_value());
    CHECK(failed.error() == utxoz::error_code::version_unreadable);

    auto const after_failure = db.get_statistics();
    CHECK(after_failure.deferred.processing_runs == before.deferred.processing_runs);
    CHECK(after_failure.resolution.resolved == before.resolution.resolved);
    CHECK(after_failure.resolution.absent == before.resolution.absent);

    failpoints::clear();
    auto const retried = db.resolve(batch);
    REQUIRE(retried.has_value());
    CHECK(retried->found.size() == corpus::group_size);
    CHECK(retried->absent.size() == corpus::group_size);

    db.close();
}

TEST_CASE("reference: an unlistable catalogue keeps its own cause and consumes nothing",
          "[ownership][reference][negative]") {
    failpoint_guard guard;
    auto db = open_reference();
    auto const& c = keys();

    auto const batch = batch_of(c.a_stored, 100'000);

    failpoints::fail_historical_catalog.store(true, std::memory_order_relaxed);
    auto const failed = db.resolve(batch);
    REQUIRE_FALSE(failed.has_value());
    CHECK(failed.error() == utxoz::error_code::catalog_unreadable);
    CHECK(batch.size() == corpus::group_size);

    failpoints::clear();
    auto const retried = db.resolve(batch);
    REQUIRE(retried.has_value());
    CHECK(retried->found.size() == corpus::group_size);

    db.close();
}

// =============================================================================
// The two statistics families do not touch each other
// =============================================================================

/**
 * A resolution writes resolution_stats and nothing else; a deletion writes
 * deferred_stats and nothing else.
 *
 * They used to share. resolve() incremented deferred_stats.processing_runs,
 * added its found keys to successfully_processed and its absences to
 * failed_to_delete, and wrote a per-depth histogram into lookups_by_depth —
 * fields belonging to the deletion path. An operator reading
 * `failed_to_delete` saw deletions that failed plus outpoints that were looked
 * up and are legitimately not stored, which are unrelated events, and
 * `successfully_processed` moved for both too. Neither number described
 * anything (#118).
 *
 * Each case below moves one path and asserts the other family is byte-identical,
 * so a single write across the boundary fails it.
 */
TEST_CASE("full: a resolution moves only the resolution counters",
          "[ownership][full][statistics]") {
    failpoint_guard guard;
    auto db = open_full();
    auto const& c = keys();

    auto const before = db.get_statistics();

    // One key that resolves and one proven absent, so both outcomes are
    // exercised in a single completed resolution.
    std::vector<lookup_request> const batch{{c.a_stored[0], 100'000},
                                            {c.a_absent[0], 100'000}};
    auto const resolved = db.resolve(batch);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->found.size() == 1);
    REQUIRE(resolved->absent.size() == 1);

    auto const after = db.get_statistics();

#ifdef UTXOZ_STATISTICS_ENABLED
    CHECK(after.resolution.resolved == before.resolution.resolved + 1);
    CHECK(after.resolution.absent == before.resolution.absent + 1);
    CHECK(after.resolution.files_visited > before.resolution.files_visited);
#endif

    // Not one of these may have moved. They belong to deletions.
    CHECK(after.deferred.processing_runs == before.deferred.processing_runs);
    CHECK(after.deferred.successfully_processed == before.deferred.successfully_processed);
    CHECK(after.deferred.failed_to_delete == before.deferred.failed_to_delete);
    CHECK(after.deferred.total_processing_time == before.deferred.total_processing_time);
    CHECK(after.deferred.deletions_by_depth == before.deferred.deletions_by_depth);

    db.close();
}

TEST_CASE("full: a deletion moves only the deletion counters",
          "[ownership][full][statistics]") {
    failpoint_guard guard;

    // Its own database, not the shared fixture. This is the one case here that
    // writes, and the fixture the others share is documented as read-only —
    // deleting out of it would make their independence a matter of ordering.
    // One rotation is enough and costs half of what the shared one does.
    scoped_path own{make_unique_path("del")};
    auto opened = utxoz::full_db::open_for_testing(own.path, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    // Inserted before the fill, so the rotation leaves it below the active
    // version. That is deliberate: a key still in the active version is applied
    // by the batch's first phase without opening a file, so the historical walk
    // would have nothing to do and its counters would not move — the case would
    // pass for the wrong reason.
    auto const doomed = outpoint_of(7'777'001, 2);
    REQUIRE(db.insert(doomed, utxoz::output_data_span{value_of(33, 0x11)}, 1).value());
    fill_until_rotations(db, 1);

    auto const before = db.get_statistics();

    std::vector<utxoz::deferred_deletion_entry> const batch{{doomed, 70'001}};
    auto const progress = db.apply_deletes(batch);
    REQUIRE(progress.erased.size() == 1);   // the walk actually applied it
    REQUIRE(progress.absent.empty());
    REQUIRE(progress.unresolved.empty());

    auto const after = db.get_statistics();

#ifdef UTXOZ_STATISTICS_ENABLED
    // The deletion path moved — at least one of its counters has to have
    // changed, or this case would be
    // asserting that the resolution family stayed still while nothing happened
    // at all. With statistics off every counter is zero by construction and
    // there is nothing to move, which is why this half is guarded and the half
    // below is not.
    CHECK((after.deferred.processing_runs != before.deferred.processing_runs
           || after.deferred.successfully_processed != before.deferred.successfully_processed));
#endif

    // And the resolution family did not. No resolve() ran.
    CHECK(after.resolution.resolved == before.resolution.resolved);
    CHECK(after.resolution.absent == before.resolution.absent);
    CHECK(after.resolution.files_visited == before.resolution.files_visited);
    CHECK(after.resolution.cache_hits == before.resolution.cache_hits);

    db.close();
}

TEST_CASE("full: an incomplete resolution moves neither family",
          "[ownership][full][statistics][negative]") {
    failpoint_guard guard;
    auto db = open_full();
    auto const& c = keys();

    auto const before = db.get_statistics();

    auto const batch = batch_of(c.a_stored, 100'000);
    failpoints::fail_historical_open_version.store(1, std::memory_order_relaxed);
    REQUIRE_FALSE(db.resolve(batch).has_value());

    auto const after = db.get_statistics();

    // Nothing at all. An attempt that was abandoned did work — it opened files
    // and resolved keys — and none of it may be observable, because the retry
    // does all of it again and would otherwise count twice.
    CHECK(after.resolution.resolved == before.resolution.resolved);
    CHECK(after.resolution.absent == before.resolution.absent);
    CHECK(after.resolution.files_visited == before.resolution.files_visited);
    CHECK(after.resolution.cache_hits == before.resolution.cache_hits);

    CHECK(after.deferred.processing_runs == before.deferred.processing_runs);
    CHECK(after.deferred.successfully_processed == before.deferred.successfully_processed);
    CHECK(after.deferred.failed_to_delete == before.deferred.failed_to_delete);
    CHECK(after.deferred.total_processing_time == before.deferred.total_processing_time);

    db.close();
}

TEST_CASE("reference: a resolution moves only the resolution counters",
          "[ownership][reference][statistics]") {
    failpoint_guard guard;
    auto db = open_reference();
    auto const& c = keys();

    auto const before = db.get_statistics();

    std::vector<lookup_request> const batch{{c.a_stored[0], 100'000},
                                            {c.a_absent[0], 100'000}};
    auto const resolved = db.resolve(batch);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->found.size() == 1);
    REQUIRE(resolved->absent.size() == 1);

    auto const after = db.get_statistics();

#ifdef UTXOZ_STATISTICS_ENABLED
    CHECK(after.resolution.resolved == before.resolution.resolved + 1);
    CHECK(after.resolution.absent == before.resolution.absent + 1);
#endif

    CHECK(after.deferred.processing_runs == before.deferred.processing_runs);
    CHECK(after.deferred.successfully_processed == before.deferred.successfully_processed);
    CHECK(after.deferred.failed_to_delete == before.deferred.failed_to_delete);
    CHECK(after.deferred.total_processing_time == before.deferred.total_processing_time);

    db.close();
}

// =============================================================================
// Concurrency: resolve() may be called from two threads at once
// =============================================================================

/**
 * Two owners, two threads, and nothing arranging that they do not overlap.
 *
 * This is the case the mutex in database_impl exists for, and the one the
 * "[concurrency]" cases above cannot make: those hold a lock of their own, so
 * they prove that batches stay separate and say nothing about whether the call
 * is safe to enter twice at once. Before #120 this crashed — 90 ThreadSanitizer
 * races in file_cache::get_or_open_file and exit 139 — because the cache hands
 * out a reference into a mapping it destroys on eviction.
 *
 * Meant to be run under ThreadSanitizer as well as plain. The barrier is inside
 * the round loop so the two threads enter resolve() together every time rather
 * than drifting apart after the first one.
 */
TEST_CASE("full: resolve() may be called concurrently, with no lock of the caller's",
          "[ownership][full][concurrency][unguarded]") {
    failpoint_guard guard;
    auto db = open_full();
    auto const& c = keys();

    auto const batch_a = batch_of(c.a_stored, 100'000);
    auto const batch_b = batch_of(c.b_stored, 100'000);

    constexpr int rounds = 40;
    std::barrier sync(2);
    std::atomic<int> mismatches{0};
    std::atomic<int> failures{0};

    auto worker = [&](std::vector<lookup_request> const& mine,
                      std::array<utxoz::raw_outpoint, corpus::group_size> const& expected,
                      std::array<utxoz::raw_outpoint, corpus::group_size> const& foreign) {
        for (int round = 0; round < rounds; ++round) {
            sync.arrive_and_wait();
            auto const got = db.resolve(mine);   // no lock: that is the case
            if ( ! got) { ++failures; continue; }
            if (got->found.size() != corpus::group_size) ++mismatches;
            if ( ! got->absent.empty()) ++mismatches;
            for (auto const& k : expected) {
                if ( ! got->found.contains(k)) ++mismatches;
            }
            for (auto const& k : foreign) {
                if (got->found.contains(k)) ++mismatches;
            }
        }
    };

    std::thread ta(worker, std::cref(batch_a), std::cref(c.a_stored), std::cref(c.b_stored));
    std::thread tb(worker, std::cref(batch_b), std::cref(c.b_stored), std::cref(c.a_stored));
    ta.join();
    tb.join();

    CHECK(failures.load() == 0);
    CHECK(mismatches.load() == 0);
    CHECK(batch_a.size() == corpus::group_size);
    CHECK(batch_b.size() == corpus::group_size);

    db.close();
}

TEST_CASE("reference: resolve() may be called concurrently, with no lock of the caller's",
          "[ownership][reference][concurrency][unguarded]") {
    failpoint_guard guard;
    auto db = open_reference();
    auto const& c = keys();

    auto const batch_a = batch_of(c.a_stored, 100'000);
    auto const batch_b = batch_of(c.b_stored, 100'000);

    constexpr int rounds = 40;
    std::barrier sync(2);
    std::atomic<int> mismatches{0};
    std::atomic<int> failures{0};

    auto worker = [&](std::vector<lookup_request> const& mine,
                      std::array<utxoz::raw_outpoint, corpus::group_size> const& expected,
                      std::array<utxoz::raw_outpoint, corpus::group_size> const& foreign) {
        for (int round = 0; round < rounds; ++round) {
            sync.arrive_and_wait();
            auto const got = db.resolve(mine);
            if ( ! got) { ++failures; continue; }
            if (got->found.size() != corpus::group_size) ++mismatches;
            for (auto const& k : expected) if ( ! got->found.contains(k)) ++mismatches;
            for (auto const& k : foreign) if (got->found.contains(k)) ++mismatches;
        }
    };

    std::thread ta(worker, std::cref(batch_a), std::cref(c.a_stored), std::cref(c.b_stored));
    std::thread tb(worker, std::cref(batch_b), std::cref(c.b_stored), std::cref(c.a_stored));
    ta.join();
    tb.join();

    CHECK(failures.load() == 0);
    CHECK(mismatches.load() == 0);

    db.close();
}

/**
 * The positive control the concurrent cases are measured against.
 *
 * The same two batches, the same number of resolutions, on one thread. If this
 * ever disagrees with the concurrent cases above, the disagreement is the
 * concurrency and not the batches — which is the only way to read those results
 * without wondering whether the fixture itself is flaky.
 */
TEST_CASE("full: the same batches resolved sequentially agree with the concurrent run",
          "[ownership][full][concurrency]") {
    failpoint_guard guard;
    auto db = open_full();
    auto const& c = keys();

    auto const batch_a = batch_of(c.a_stored, 100'000);
    auto const batch_b = batch_of(c.b_stored, 100'000);

    constexpr int rounds = 40;
    for (int round = 0; round < rounds; ++round) {
        auto const a = db.resolve(batch_a);
        auto const b = db.resolve(batch_b);
        REQUIRE(a.has_value());
        REQUIRE(b.has_value());
        REQUIRE(a->found.size() == corpus::group_size);
        REQUIRE(b->found.size() == corpus::group_size);
        REQUIRE(a->absent.empty());
        REQUIRE(b->absent.empty());
        for (size_t i = 0; i < corpus::group_size; ++i) {
            REQUIRE(a->found.contains(c.a_stored[i]));
            REQUIRE_FALSE(a->found.contains(c.b_stored[i]));
            REQUIRE(b->found.contains(c.b_stored[i]));
            REQUIRE_FALSE(b->found.contains(c.a_stored[i]));
        }
    }

    db.close();
}

/**
 * The client's real shape: one thread reading, another resolving.
 *
 * resolve_mutex_ serialises resolutions against each other. It does nothing
 * about find(), and the contract this replaces forbade exactly this pairing —
 * "concurrent find() calls are permitted strictly while no ... sweep or cache
 * operation can run". KTH has a lock-free path that calls find(), and it cannot
 * be held off while another component resolves, so whether these two may
 * overlap has to be demonstrated rather than reasoned about.
 *
 * The reader exercises both of find()'s paths: a key in the active version,
 * which it must answer with the stored value every single time, and keys below
 * it, which must always come back not_resolved. The resolver is given a batch
 * spanning both size classes so it opens and evicts repeatedly — the cache holds
 * one file by default, so eviction is not an edge case here, it is every round.
 *
 * Run under ThreadSanitizer. Nothing in this case takes a lock.
 */
TEST_CASE("full: find() and resolve() may run at the same time",
          "[ownership][full][concurrency][unguarded]") {
    failpoint_guard guard;
    auto db = open_full();
    auto const& c = keys();

    // Establish, single-threaded, what the reader must keep seeing.
    auto const baseline = db.find(c.active_witness, 100'000);
    REQUIRE(baseline.has_value());
    auto const expected_value = baseline->data;

    auto const batch = batch_of(c.a_stored, 100'000);

    constexpr int rounds = 200;
    std::barrier sync(2);
    std::atomic<int> reader_wrong{0};
    std::atomic<int> resolver_wrong{0};
    std::atomic<bool> stop{false};

    auto const& const_db = db;
    std::thread reader([&] {
        for (int round = 0; round < rounds; ++round) {
            sync.arrive_and_wait();
            // The hit path: an active-version key, answered without any file.
            auto const hit = const_db.find(c.active_witness, 100'000);
            if ( ! hit || hit->data != expected_value) ++reader_wrong;
            // The miss path: keys that live below the active version, which is
            // where the resolver is working.
            for (auto const& k : c.a_stored) {
                auto const miss = const_db.find(k, 100'000);
                if (miss || miss.error() != utxoz::error_code::not_resolved) ++reader_wrong;
            }
        }
        stop = true;
    });

    std::thread resolver([&] {
        for (int round = 0; round < rounds; ++round) {
            sync.arrive_and_wait();
            auto const got = db.resolve(batch);
            if ( ! got || got->found.size() != corpus::group_size) ++resolver_wrong;
        }
    });

    reader.join();
    resolver.join();

    CHECK(reader_wrong.load() == 0);
    CHECK(resolver_wrong.load() == 0);
    CHECK(stop.load());

    db.close();
}

TEST_CASE("reference: find() and resolve() may run at the same time",
          "[ownership][reference][concurrency][unguarded]") {
    failpoint_guard guard;
    auto db = open_reference();
    auto const& c = keys();

    auto const baseline = db.find(c.active_witness, 100'000);
    REQUIRE(baseline.has_value());
    CHECK(baseline->file_number == 777u);
    CHECK(baseline->offset == 888u);

    auto const batch = batch_of(c.a_stored, 100'000);

    constexpr int rounds = 200;
    std::barrier sync(2);
    std::atomic<int> reader_wrong{0};
    std::atomic<int> resolver_wrong{0};

    auto const& const_db = db;
    std::thread reader([&] {
        for (int round = 0; round < rounds; ++round) {
            sync.arrive_and_wait();
            auto const hit = const_db.find(c.active_witness, 100'000);
            if ( ! hit || hit->file_number != 777u || hit->offset != 888u) ++reader_wrong;
            for (auto const& k : c.a_stored) {
                auto const miss = const_db.find(k, 100'000);
                if (miss || miss.error() != utxoz::error_code::not_resolved) ++reader_wrong;
            }
        }
    });

    std::thread resolver([&] {
        for (int round = 0; round < rounds; ++round) {
            sync.arrive_and_wait();
            auto const got = db.resolve(batch);
            if ( ! got || got->found.size() != corpus::group_size) ++resolver_wrong;
        }
    });

    reader.join();
    resolver.join();

    CHECK(reader_wrong.load() == 0);
    CHECK(resolver_wrong.load() == 0);

    db.close();
}

// =============================================================================
// What the handoff costs
// =============================================================================

/**
 * The price of passing the batch in, separated from the price of answering it.
 *
 * Taking a span means resolve() has to turn the caller's requests into a working
 * set before it can walk anything: one pass to drop duplicate keys and record an
 * index per distinct request. The requests themselves are never copied — they
 * stay in the caller's vector and are addressed by index — but the pass is real
 * and somebody should know what it costs before it is on the path of every
 * block.
 *
 * It is isolated by resolving against a database that has never rotated. There
 * is nothing below the active version, so both walk phases find no file to open
 * and what is left is exactly the handoff plus building the `absent` list. The
 * rotated fixture then gives the same batch a real resolution to compare with.
 *
 * Hidden by default. Run with: utxoz_tests "[.measure]"
 */
TEST_CASE("the batch handoff is a rounding error next to the file walk",
          "[ownership][.measure]") {
    failpoint_guard guard;

    scoped_path flat{make_unique_path("flat")};
    {
        auto opened = utxoz::full_db::open_for_testing(flat.path, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        for (uint64_t i = 0; i < 1'000; ++i) {
            REQUIRE(db.insert(outpoint_of(500'000 + i, 0),
                              utxoz::output_data_span{value_of(33, 2)}, 1).value());
        }
        REQUIRE(db.get_statistics().rotations_per_container[0] == 0);
        db.close();
    }

    auto const bench = [](auto& db, std::vector<lookup_request> const& batch, int reps) {
        auto const start = std::chrono::steady_clock::now();
        size_t sink = 0;
        for (int i = 0; i < reps; ++i) {
            auto const r = db.resolve(batch);
            REQUIRE(r.has_value());
            sink += r->found.size() + r->absent.size();
        }
        auto const elapsed = std::chrono::steady_clock::now() - start;
        REQUIRE(sink > 0);
        return std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count()
             / static_cast<double>(reps) / static_cast<double>(batch.size());
    };

    for (size_t n : {size_t{1'000}, size_t{5'000}, size_t{20'000}}) {
        std::vector<lookup_request> batch;
        batch.reserve(n);
        for (uint64_t i = 0; i < n; ++i) batch.emplace_back(outpoint_of(700'000'000ULL + i, 1), 100'000);

        double handoff = 0.0;
        {
            auto opened = utxoz::full_db::open_for_testing(flat.path, false);
            REQUIRE(opened.has_value());
            auto db = std::move(*opened);
            handoff = bench(db, batch, 20);
            db.close();
        }

        double whole = 0.0;
        {
            auto db = open_full();
            whole = bench(db, batch, 3);
            db.close();
        }

        WARN(fmt::format("batch {:>6}: handoff {:6.1f} ns/request, whole resolution {:8.1f} "
                         "ns/request ({:.1f}x)", n, handoff, whole,
                         handoff > 0.0 ? whole / handoff : 0.0));

        // Not a threshold on absolute speed — that would fail on a loaded runner.
        // The claim is structural: the handoff cannot be the expensive part,
        // because answering means opening and probing every version file.
        CHECK(handoff < whole);
    }
}

TEST_CASE("reference: two threads resolving their own batches neither mix nor steal",
          "[ownership][reference][concurrency]") {
    failpoint_guard guard;
    auto db = open_reference();
    auto const& c = keys();

    auto const batch_a = batch_of(c.a_stored, 100'000);
    auto const batch_b = batch_of(c.b_stored, 100'000);

    constexpr int rounds = 25;
    std::mutex db_mutex;
    std::barrier sync(2);
    std::atomic<int> mismatches{0};

    auto worker = [&](std::vector<lookup_request> const& mine,
                      std::array<utxoz::raw_outpoint, corpus::group_size> const& expected,
                      std::array<utxoz::raw_outpoint, corpus::group_size> const& foreign) {
        for (int round = 0; round < rounds; ++round) {
            sync.arrive_and_wait();
            utxoz::result<utxoz::reference_resolution> got = utxoz::reference_resolution{};
            {
                std::scoped_lock lock(db_mutex);
                got = db.resolve(mine);
            }
            if ( ! got) { ++mismatches; continue; }
            if (got->found.size() != corpus::group_size) ++mismatches;
            if ( ! got->absent.empty()) ++mismatches;
            for (auto const& k : expected) if ( ! got->found.contains(k)) ++mismatches;
            for (auto const& k : foreign) if (got->found.contains(k)) ++mismatches;
        }
    };

    std::thread ta(worker, std::cref(batch_a), std::cref(c.a_stored), std::cref(c.b_stored));
    std::thread tb(worker, std::cref(batch_b), std::cref(c.b_stored), std::cref(c.a_stored));
    ta.join();
    tb.join();

    CHECK(mismatches.load() == 0);
    db.close();
}
