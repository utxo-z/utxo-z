// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_unresolved_vs_absent.cpp
 * @brief The `absent` list from resolve() means absent, and only absent.
 *
 * A consumer decides whether a block is valid by asking whether its inputs
 * exist. If a key it merely failed to look up — because a version file would
 * not open — comes back beside keys that exist nowhere, the consumer cannot
 * tell them apart. It reports missing_previous_output and rejects a block that
 * may be perfectly valid, and the cause is a local storage fault it never hears
 * about.
 *
 * So resolution is fail-closed: every version below the current one is read, or
 * the call returns an error and no lists at all. What survives into `absent` was
 * looked for everywhere it could have been.
 *
 * The failure is injected rather than arranged on disk. Corrupting a file and
 * hoping produces a test that passes for reasons nobody chose; the failpoint
 * names one version and fails exactly that open, so each case has one outcome.
 *
 * The retry is the assertion that carries the most weight. It is not enough that
 * the second call succeeds: it has to return *both* resolvable keys and the one
 * genuine absence, from the same vector the caller passed the first time. Since
 * the batch is the caller's (#116), that is now a property of the API rather
 * than of a rollback the library has to get right — and the case is kept
 * precisely to pin that it stayed true when the rollback was deleted.
 */

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <numeric>
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

namespace {

inline std::atomic<uint64_t> case_counter{0};

std::string make_unique_path(std::string_view tag) {
    auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fmt::format("./test_unres_{}_{}_{}_{}", tag, getpid(), ts, case_counter.fetch_add(1));
}

utxoz::raw_outpoint outpoint_of(uint64_t n, uint32_t index) {
    utxoz::raw_outpoint key{};
    std::memcpy(key.data(), &n, sizeof(n));
    key[20] = 0x5A;
    std::memcpy(key.data() + 32, &index, sizeof(index));
    return key;
}

std::vector<uint8_t> value_of(size_t size, uint8_t seed) {
    std::vector<uint8_t> v(size);
    std::iota(v.begin(), v.end(), seed);
    return v;
}

/// Keys that end up in a previous version, plus one that was never stored.
struct fixture {
    std::string path;
    utxoz::raw_outpoint in_container_0;
    utxoz::raw_outpoint in_container_1;
    utxoz::raw_outpoint never_stored;
};

/// Fills until container 0 has rotated twice and container 1 once, so the sweep
/// has several previous versions to walk and the failpoint has a version to
/// fail that is neither the first nor the only one.
///
/// The probe that established these numbers took 260'100 inserts; the loop below
/// stops on the rotation counters rather than on a count, so it stays correct if
/// the capacities change.
fixture build_full_fixture(std::string_view tag) {
    fixture f{std::string(make_unique_path(tag)),
              outpoint_of(1, 0), outpoint_of(2, 0), outpoint_of(999'999'999ULL, 7)};

    auto opened = utxoz::full_db::open_for_testing(f.path, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    REQUIRE(db.insert(f.in_container_0, utxoz::output_data_span{value_of(33, 1)}, 1).value());
    REQUIRE(db.insert(f.in_container_1, utxoz::output_data_span{value_of(70, 2)}, 1).value());

    uint64_t n = 100;
    uint32_t height = 2;
    for (int batch = 0; batch < 400; ++batch) {
        auto const stats = db.get_statistics();
        if (stats.rotations_per_container[0] >= 2 && stats.rotations_per_container[1] >= 1) break;
        for (int i = 0; i < 5000; ++i, ++n) {
            (void) db.insert(outpoint_of(n, 0), utxoz::output_data_span{value_of(33, 3)}, height);
            if (i % 5 == 0) {
                (void) db.insert(outpoint_of(n, 1), utxoz::output_data_span{value_of(70, 4)}, height);
            }
        }
        ++height;
    }

    auto const stats = db.get_statistics();
    REQUIRE(stats.rotations_per_container[0] >= 2);
    REQUIRE(stats.rotations_per_container[1] >= 1);

    db.close();
    return f;
}

/// Probes the three keys and keeps the ones the active versions could not
/// answer. Reopening first leaves the file cache cold, so the resolution's order
/// is the catalogue's and not a leftover of how the fixture was filled.
///
/// The batch is built here and belongs to the caller from here on. Nothing in
/// the database remembers these keys, which is the whole change: the answer can
/// only come back to whoever holds this vector.
std::vector<utxoz::lookup_request> batch_of(utxoz::full_db& db, fixture const& f) {
    std::vector<utxoz::lookup_request> batch;
    for (auto const& key : {f.in_container_0, f.in_container_1, f.never_stored}) {
        auto const probed = db.find(key, 100'000);
        REQUIRE_FALSE(probed.has_value());
        // not_resolved, never not_found: the active versions were all that was
        // consulted, so this cannot be absence.
        REQUIRE(probed.error() == utxoz::error_code::not_resolved);
        batch.emplace_back(key, 100'000);
    }
    REQUIRE(batch.size() == 3);
    return batch;
}

/// Exactly what a complete resolution must produce for this fixture.
void check_complete_resolution(
    utxoz::result<utxoz::full_resolution> const& resolved,
    fixture const& f,
    std::vector<utxoz::lookup_request> const& batch) {
    REQUIRE(resolved.has_value());

    auto const& [found, absent] = *resolved;

    CHECK(found.size() == 2);
    REQUIRE(found.contains(f.in_container_0));
    REQUIRE(found.contains(f.in_container_1));
    CHECK(found.at(f.in_container_0).data == value_of(33, 1));
    CHECK(found.at(f.in_container_1).data == value_of(70, 2));

    REQUIRE(absent.size() == 1);
    CHECK(absent[0].key == f.never_stored);

    // The batch survives the call unchanged. resolve() borrowed it; it did not
    // take it, so a caller can resolve the same batch again if it wants to.
    CHECK(batch.size() == 3);
}

struct failpoint_guard {
    failpoint_guard() { failpoints::clear(); }
    ~failpoint_guard() { failpoints::clear(); }
};

} // namespace

// =============================================================================
// full mode
// =============================================================================

TEST_CASE("full: a complete sweep resolves what is there and proves what is not",
          "[unresolved][full]") {
    failpoint_guard guard;
    auto const f = build_full_fixture("full_complete");
    {
        auto opened = utxoz::full_db::open_for_testing(f.path, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        auto const batch = batch_of(db, f);
        check_complete_resolution(db.resolve(batch), f, batch);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("full: a version that will not open is an error, and the retry loses nothing",
          "[unresolved][full][negative]") {
    failpoint_guard guard;
    auto const f = build_full_fixture("full_unreadable");
    {
        auto opened = utxoz::full_db::open_for_testing(f.path, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        auto const batch = batch_of(db, f);

        // Version 1: container 0 has versions 0 and 1 below its current, so the
        // sweep reads one of them before it reaches this one. The failure
        // therefore lands after entries have already been consumed, which is the
        // case the restore exists for.
        failpoints::fail_lookup_open_version.store(1, std::memory_order_relaxed);

        auto const failed = db.resolve(batch);

        REQUIRE_FALSE(failed.has_value());
        CHECK(failed.error() == utxoz::error_code::version_unreadable);

        // Nothing was consumed: all three are still queued, so the retry needs
        // no requeueing.
        // Untouched: a failed resolution consumes nothing, because it was never
        // given anything to consume. The retry below needs no rebuilding.
        CHECK(batch.size() == 3);

        failpoints::clear();

        // The retry returns both resolvable keys and the one genuine absence.
        // Returning only the keys the failed sweep never reached would mean the
        // consumed entries were dropped, and that is what this pins down.
        check_complete_resolution(db.resolve(batch), f, batch);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("full: a failure before anything is resolved restores the queue the same way",
          "[unresolved][full][negative]") {
    failpoint_guard guard;
    auto const f = build_full_fixture("full_early");
    {
        auto opened = utxoz::full_db::open_for_testing(f.path, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        auto const batch = batch_of(db, f);

        // Version 0 is the oldest, reached first.
        failpoints::fail_lookup_open_version.store(0, std::memory_order_relaxed);

        auto const failed = db.resolve(batch);
        REQUIRE_FALSE(failed.has_value());
        CHECK(failed.error() == utxoz::error_code::version_unreadable);
        // Untouched: a failed resolution consumes nothing, because it was never
        // given anything to consume. The retry below needs no rebuilding.
        CHECK(batch.size() == 3);

        failpoints::clear();
        check_complete_resolution(db.resolve(batch), f, batch);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("full: a catalogue that cannot be listed keeps its own cause",
          "[unresolved][full][negative]") {
    failpoint_guard guard;
    auto const f = build_full_fixture("full_catalog");
    {
        auto opened = utxoz::full_db::open_for_testing(f.path, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        auto const batch = batch_of(db, f);

        failpoints::fail_lookup_catalog.store(true, std::memory_order_relaxed);

        auto const failed = db.resolve(batch);

        REQUIRE_FALSE(failed.has_value());
        // Not version_unreadable: not knowing which files exist sends an
        // operator somewhere else than a file that will not open.
        CHECK(failed.error() == utxoz::error_code::catalog_unreadable);
        // Untouched: a failed resolution consumes nothing, because it was never
        // given anything to consume. The retry below needs no rebuilding.
        CHECK(batch.size() == 3);

        failpoints::clear();
        check_complete_resolution(db.resolve(batch), f, batch);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("full: clearing the failpoint removes the failure", "[unresolved][full]") {
    failpoint_guard guard;
    auto const f = build_full_fixture("full_clear");
    {
        auto opened = utxoz::full_db::open_for_testing(f.path, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        auto const batch = batch_of(db, f);
        failpoints::fail_lookup_open_version.store(1, std::memory_order_relaxed);
        REQUIRE_FALSE(db.resolve(batch).has_value());

        failpoints::clear();

        check_complete_resolution(db.resolve(batch), f, batch);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

// A sweep that is rolled back must not leave its work in the numbers. The retry
// does it all again, so an attempt that was abandoned would be counted twice —
// and these are what an operator reads to decide whether the deferred path is
// behaving.
TEST_CASE("full: a failed sweep publishes no statistics, and the retry counts once",
          "[unresolved][full][statistics]") {
    failpoint_guard guard;
    auto const f = build_full_fixture("full_stats");
    {
        auto opened = utxoz::full_db::open_for_testing(f.path, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        auto const batch = batch_of(db, f);

        auto const before = db.get_statistics();

        failpoints::fail_lookup_open_version.store(1, std::memory_order_relaxed);
        REQUIRE_FALSE(db.resolve(batch).has_value());

        auto const after_failure = db.get_statistics();

        // Exactly unchanged. The failed attempt read files and resolved entries,
        // and none of it may be observable.
        CHECK(after_failure.deferred.processing_runs == before.deferred.processing_runs);
        CHECK(after_failure.deferred.successfully_processed == before.deferred.successfully_processed);
        CHECK(after_failure.deferred.failed_to_delete == before.deferred.failed_to_delete);
        CHECK(after_failure.resolution.files_visited == before.resolution.files_visited);
        CHECK(after_failure.resolution.cache_hits == before.resolution.cache_hits);
        CHECK(after_failure.resolution.resolved == before.resolution.resolved);
        CHECK(after_failure.resolution.unresolved == before.resolution.unresolved);

        failpoints::clear();
        check_complete_resolution(db.resolve(batch), f, batch);

        auto const after_retry = db.get_statistics();

#ifdef UTXOZ_STATISTICS_ENABLED
        // One run, two lookups resolved, one left unresolved — each counted once.
        // Only meaningful where the counters exist; with statistics off they are
        // all zero and the checks above still hold, which is the point of
        // running this case in both builds.
        CHECK(after_retry.deferred.processing_runs == before.deferred.processing_runs + 1);
        CHECK(after_retry.resolution.resolved == before.resolution.resolved + 2);
        CHECK(after_retry.resolution.unresolved == before.resolution.unresolved + 1);
#else
        CHECK(after_retry.deferred.processing_runs == before.deferred.processing_runs);
#endif

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

// =============================================================================
// reference mode — the same contract, exercised the same way
// =============================================================================

namespace {

struct reference_fixture {
    std::string path;
    utxoz::raw_outpoint stored;
    utxoz::raw_outpoint never_stored;
};

reference_fixture build_reference_fixture(std::string_view tag) {
    reference_fixture f{std::string(make_unique_path(tag)),
                        outpoint_of(11, 0), outpoint_of(888'888'888ULL, 9)};

    auto opened = utxoz::reference_db::open_for_testing(f.path, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    REQUIRE(db.insert(f.stored, 3, 44, 1).value());

    uint64_t n = 100;
    uint32_t height = 2;
    for (int batch = 0; batch < 400; ++batch) {
        if (db.get_statistics().rotations_per_container[0] >= 2) break;
        for (int i = 0; i < 5000; ++i, ++n) {
            (void) db.insert(outpoint_of(n, 0), static_cast<uint32_t>(n % 1000),
                             static_cast<uint32_t>(n), height);
        }
        ++height;
    }
    REQUIRE(db.get_statistics().rotations_per_container[0] >= 2);

    db.close();
    return f;
}

std::vector<utxoz::lookup_request> reference_batch_of(utxoz::reference_db& db,
                                                     reference_fixture const& f) {
    std::vector<utxoz::lookup_request> batch;
    for (auto const& key : {f.stored, f.never_stored}) {
        auto const probed = db.find(key, 100'000);
        REQUIRE_FALSE(probed.has_value());
        REQUIRE(probed.error() == utxoz::error_code::not_resolved);
        batch.emplace_back(key, 100'000);
    }
    REQUIRE(batch.size() == 2);
    return batch;
}

void check_reference_resolution(
    utxoz::result<utxoz::reference_resolution> const& resolved,
    reference_fixture const& f,
    std::vector<utxoz::lookup_request> const& batch) {
    REQUIRE(resolved.has_value());

    auto const& [found, absent] = *resolved;

    CHECK(found.size() == 1);
    REQUIRE(found.contains(f.stored));
    CHECK(found.at(f.stored).file_number == 3u);
    CHECK(found.at(f.stored).offset == 44u);

    REQUIRE(absent.size() == 1);
    CHECK(absent[0].key == f.never_stored);

    CHECK(batch.size() == 2);
}

} // namespace

TEST_CASE("reference: a complete sweep resolves what is there and proves what is not",
          "[unresolved][reference]") {
    failpoint_guard guard;
    auto const f = build_reference_fixture("ref_complete");
    {
        auto opened = utxoz::reference_db::open_for_testing(f.path, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        auto const batch = reference_batch_of(db, f);
        check_reference_resolution(db.resolve(batch), f, batch);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("reference: a version that will not open is an error, and the retry loses nothing",
          "[unresolved][reference][negative]") {
    failpoint_guard guard;
    auto const f = build_reference_fixture("ref_unreadable");
    {
        auto opened = utxoz::reference_db::open_for_testing(f.path, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        auto const batch = reference_batch_of(db, f);

        failpoints::fail_lookup_open_version.store(0, std::memory_order_relaxed);

        auto const failed = db.resolve(batch);

        REQUIRE_FALSE(failed.has_value());
        CHECK(failed.error() == utxoz::error_code::version_unreadable);
        // Untouched: the batch was borrowed, not taken.
        CHECK(batch.size() == 2);

        failpoints::clear();
        check_reference_resolution(db.resolve(batch), f, batch);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("reference: a catalogue that cannot be listed keeps its own cause",
          "[unresolved][reference][negative]") {
    failpoint_guard guard;
    auto const f = build_reference_fixture("ref_catalog");
    {
        auto opened = utxoz::reference_db::open_for_testing(f.path, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        auto const batch = reference_batch_of(db, f);

        failpoints::fail_lookup_catalog.store(true, std::memory_order_relaxed);

        auto const failed = db.resolve(batch);
        REQUIRE_FALSE(failed.has_value());
        CHECK(failed.error() == utxoz::error_code::catalog_unreadable);
        // Untouched: the batch was borrowed, not taken.
        CHECK(batch.size() == 2);

        failpoints::clear();
        check_reference_resolution(db.resolve(batch), f, batch);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("reference: a failed sweep publishes no statistics, and the retry counts once",
          "[unresolved][reference][statistics]") {
    failpoint_guard guard;
    auto const f = build_reference_fixture("ref_stats");
    {
        auto opened = utxoz::reference_db::open_for_testing(f.path, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        auto const batch = reference_batch_of(db, f);

        auto const before = db.get_statistics();

        failpoints::fail_lookup_open_version.store(0, std::memory_order_relaxed);
        REQUIRE_FALSE(db.resolve(batch).has_value());

        auto const after_failure = db.get_statistics();
        CHECK(after_failure.deferred.processing_runs == before.deferred.processing_runs);
        CHECK(after_failure.resolution.files_visited == before.resolution.files_visited);
        CHECK(after_failure.resolution.cache_hits == before.resolution.cache_hits);
        CHECK(after_failure.resolution.resolved == before.resolution.resolved);
        CHECK(after_failure.resolution.unresolved == before.resolution.unresolved);

        failpoints::clear();
        check_reference_resolution(db.resolve(batch), f, batch);

        auto const after_retry = db.get_statistics();
#ifdef UTXOZ_STATISTICS_ENABLED
        CHECK(after_retry.deferred.processing_runs == before.deferred.processing_runs + 1);
        CHECK(after_retry.resolution.resolved == before.resolution.resolved + 1);
        CHECK(after_retry.resolution.unresolved == before.resolution.unresolved + 1);
#else
        CHECK(after_retry.deferred.processing_runs == before.deferred.processing_runs);
#endif

        db.close();
    }
    std::filesystem::remove_all(f.path);
}
