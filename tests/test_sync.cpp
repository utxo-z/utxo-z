// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_sync.cpp
 * @brief What sync() covers, and what it refuses to claim.
 *
 * Durability against power loss cannot be tested here. Killing a process leaves
 * the page cache intact, so a file whose barrier was skipped is still there
 * afterwards; proving the barriers reach the platter needs an apparatus that
 * can cut the power, and nothing in this file has one.
 *
 * What can be tested is everything else, and it is where the mistakes live:
 * that every barrier is attempted rather than some of them, that a failure
 * arrives as a value instead of being swallowed, that the platforms without a
 * given barrier say so, and above all that the *set* of things covered is the
 * right one. A sync that flushed the active containers and stopped would look
 * perfectly correct and would leave a batch's deletions to older generations
 * nowhere but memory.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <numeric>
#include <string>
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
#include "detail/scope_exit.hpp"

namespace fs = std::filesystem;
using utxoz::detail::failpoints;
using utxoz::detail::platform_sync_support;
using utxoz::durability_level;
using utxoz::detail::scope_exit;
using utxoz::detail::sync_support;

namespace {

inline std::atomic<uint64_t> sync_counter{0};

std::string unique_path(std::string_view tag) {
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fmt::format("./test_sync_{}_{}_{}_{}", tag, getpid(), ts, sync_counter.fetch_add(1));
}

utxoz::raw_outpoint make_key(uint64_t n) {
    utxoz::raw_outpoint key{};
    std::memcpy(key.data(), &n, sizeof(n));
    return key;
}

std::vector<uint8_t> make_value(size_t size, uint8_t seed) {
    std::vector<uint8_t> v(size);
    std::iota(v.begin(), v.end(), seed);
    return v;
}

size_t count_data_files(std::string const& path) {
    size_t n = 0;
    for (auto const& entry : fs::directory_iterator(path)) {
        auto const name = entry.path().filename().string();
        if (name.rfind("cont_0_v", 0) == 0 && name.ends_with(".dat")) ++n;
    }
    return n;
}

} // anonymous namespace

TEST_CASE("sync() succeeds on a database that has been written to", "[database][sync]") {
    auto const path = unique_path("basic");
    fs::remove_all(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    auto opened = utxoz::full_db::open_for_testing(path, true);
    REQUIRE(opened);
    auto db = std::move(*opened);

    for (uint64_t i = 0; i < 1000; ++i) {
        REQUIRE(db.insert(make_key(i), make_value(33, 1), 100).value());
    }

    auto const synced = db.sync();

    if constexpr (platform_sync_support() == sync_support::none) {
        // Nothing here can reach stable storage, and saying otherwise would be
        // a promise the platform cannot keep.
        REQUIRE_FALSE(synced);
        CHECK(synced.error() == utxoz::error_code::sync_unsupported);
    } else {
        REQUIRE(synced);

        // And everything is still there afterwards.
        for (uint64_t i = 0; i < 1000; ++i) {
            REQUIRE(db.find(make_key(i), 200));
        }
    }

    db.close();
}

TEST_CASE("sync() on an untouched database is not an error", "[database][sync]") {
    auto const path = unique_path("clean");
    fs::remove_all(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    auto opened = utxoz::full_db::open_for_testing(path, true);
    REQUIRE(opened);
    auto db = std::move(*opened);

    if constexpr (platform_sync_support() != sync_support::none) {
        REQUIRE(db.sync());
        REQUIRE(db.sync());   // and again, with nothing to do
    }

    db.close();
}

TEST_CASE("sync() covers every generation written to, not the ones still mapped",
          "[database][sync][failpoint]") {
    if (utxoz::platform_durability() == utxoz::durability_level::none) return;

    // The case that matters, and the one the first version of this file got
    // wrong. Historical resolution erases entries in older generations through
    // the file cache — which holds **one** mapping by default. A sweep that
    // touches three generations has evicted the first two before it ends, and
    // unmapping is not a barrier: those pages are still dirty and nothing has
    // asked the kernel to write them.
    //
    // A sync driven by what the cache is holding would flush the one survivor
    // and report the database durable. So the obligation is recorded by
    // identity when a file is written to, and outlives the mapping.
    auto const path = unique_path("everygeneration");
    fs::remove_all(path);
    scope_exit const cleanup([&] {
        failpoints::clear();
        std::error_code ec;
        fs::remove_all(path, ec);
    });

    auto opened = utxoz::full_db::open_for_testing(path, true);
    REQUIRE(opened);
    auto db = std::move(*opened);

    // Four generations, so a sweep has three historical ones to reach.
    uint64_t next = 0;
    while (count_data_files(path) < 4) {
        REQUIRE(db.insert(make_key(next++), make_value(8, 1), 100).value());
        REQUIRE(next < 2'000'000);
    }
    auto const files_before = count_data_files(path);
    REQUIRE(files_before == 4);
    REQUIRE(db.sync());

    // Erase across all of them. These defer and the sweep applies them file by
    // file, evicting as it goes.
    std::vector<utxoz::deferred_deletion_entry> batch;
    for (uint64_t i = 0; i < next; i += 2) batch.emplace_back(make_key(i), 400);
    auto const progress = db.apply_deletes(batch);
    INFO("erased " << progress.erased.size()
         << ", absent " << progress.absent.size()
         << ", unresolved " << progress.unresolved.size());
    REQUIRE(progress.unresolved.empty());
    REQUIRE(progress.erased.size() > 0);

    // The cache is holding at most one of them — the default size — which is
    // exactly why walking it is not enough.
    auto const still_mapped = db.get_cached_file_info();
    INFO("mappings still resident: " << still_mapped.size());
    REQUIRE(still_mapped.size() <= 1);

    // Count the file barriers a sync performs. There has to be one for every
    // generation that was written to, not one for the survivor.
    failpoints::sync_file_calls.store(0, std::memory_order_relaxed);
    REQUIRE(db.sync());
    auto const barriers = failpoints::sync_file_calls.load(std::memory_order_relaxed);

    // Exactly one barrier per distinct thing that owes one: the active
    // container of every size class, plus every historical generation the
    // sweep wrote to. "More than the cache could have done" would be satisfied
    // by covering one extra generation and missing the rest.
    //
    // The historical ones are every version of container 0 below the active:
    // the erases span all the keys, which span all the generations.
    auto const historical = files_before - 1;
    INFO("file barriers: " << barriers << ", active containers: " << utxoz::container_count
         << ", historical generations: " << historical
         << ", resident mappings: " << still_mapped.size());
    CHECK(barriers == utxoz::container_count + historical);

    // And a second sync has nothing left to do: the obligation was discharged.
    failpoints::sync_file_calls.store(0, std::memory_order_relaxed);
    REQUIRE(db.sync());
    auto const after = failpoints::sync_file_calls.load(std::memory_order_relaxed);
    INFO("file barriers on the second sync: " << after);
    CHECK(after < barriers);

    db.close();
}

TEST_CASE("a failed sync discharges nothing", "[database][sync][failpoint]") {
    if (utxoz::platform_durability() == utxoz::durability_level::none) return;

    auto const path = unique_path("nodischarge");
    fs::remove_all(path);
    scope_exit const cleanup([&] {
        failpoints::clear();
        std::error_code ec;
        fs::remove_all(path, ec);
    });

    auto opened = utxoz::full_db::open_for_testing(path, true);
    REQUIRE(opened);
    auto db = std::move(*opened);

    uint64_t next = 0;
    while (count_data_files(path) < 3) {
        REQUIRE(db.insert(make_key(next++), make_value(8, 1), 100).value());
        REQUIRE(next < 2'000'000);
    }
    REQUIRE(db.sync());

    std::vector<utxoz::deferred_deletion_entry> batch;
    for (uint64_t i = 0; i < next; i += 2) batch.emplace_back(make_key(i), 400);
    REQUIRE(db.apply_deletes(batch).unresolved.empty());

    failpoints::fail_sync_file.store(true, std::memory_order_relaxed);
    auto const failed_sync = db.sync();
    failpoints::clear();
    REQUIRE_FALSE(failed_sync);

    // An obligation half met is an obligation. The next sync has to attempt all
    // of them again, so it performs at least as many barriers as it would have.
    failpoints::sync_file_calls.store(0, std::memory_order_relaxed);
    REQUIRE(db.sync());
    auto const retried = failpoints::sync_file_calls.load(std::memory_order_relaxed);
    INFO("barriers on the sync after the failure: " << retried);
    CHECK(retried >= 3);

    db.close();
}

TEST_CASE("a barrier that fails comes back as a value", "[database][sync][failpoint]") {
    if constexpr (platform_sync_support() == sync_support::none) return;

    auto const path = unique_path("barrierfails");
    fs::remove_all(path);
    scope_exit const cleanup([&] {
        failpoints::clear();
        std::error_code ec;
        fs::remove_all(path, ec);
    });

    auto opened = utxoz::full_db::open_for_testing(path, true);
    REQUIRE(opened);
    auto db = std::move(*opened);
    REQUIRE(db.insert(make_key(1), make_value(33, 1), 100).value());

    SECTION("the page barrier") {
        failpoints::fail_sync_mapped_region.store(true, std::memory_order_relaxed);
        auto const synced = db.sync();
        failpoints::clear();

        REQUIRE_FALSE(synced);
        CHECK(synced.error() == utxoz::error_code::sync_failed);
    }

    SECTION("the file barrier") {
        failpoints::fail_sync_file.store(true, std::memory_order_relaxed);
        auto const synced = db.sync();
        failpoints::clear();

        REQUIRE_FALSE(synced);
        CHECK(synced.error() == utxoz::error_code::sync_failed);
    }

    SECTION("the directory barrier") {
        if constexpr (platform_sync_support() == sync_support::full) {
            failpoints::fail_sync_directory.store(true, std::memory_order_relaxed);
            auto const synced = db.sync();
            failpoints::clear();

            REQUIRE_FALSE(synced);
            CHECK(synced.error() == utxoz::error_code::sync_failed);
        }
    }

    db.close();
}

TEST_CASE("sync() works the same way in reference mode", "[database][sync][reference]") {
    auto const path = unique_path("reference");
    fs::remove_all(path);
    scope_exit const cleanup([&] {
        failpoints::clear();
        std::error_code ec;
        fs::remove_all(path, ec);
    });

    auto opened = utxoz::reference_db::open_for_testing(path, true);
    REQUIRE(opened);
    auto db = std::move(*opened);

    for (uint64_t i = 0; i < 1000; ++i) {
        REQUIRE(db.insert(make_key(i), uint32_t(i % 100), uint32_t(i), 100).value());
    }

    auto const synced = db.sync();

    if constexpr (platform_sync_support() == sync_support::none) {
        REQUIRE_FALSE(synced);
        CHECK(synced.error() == utxoz::error_code::sync_unsupported);
    } else {
        REQUIRE(synced);

        failpoints::fail_sync_file.store(true, std::memory_order_relaxed);
        auto const failing = db.sync();
        failpoints::clear();

        REQUIRE_FALSE(failing);
        CHECK(failing.error() == utxoz::error_code::sync_failed);
    }

    db.close();
}

TEST_CASE("sync() on a closed database says so", "[database][sync]") {
    auto const path = unique_path("closed");
    fs::remove_all(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    auto opened = utxoz::full_db::open_for_testing(path, true);
    REQUIRE(opened);
    auto db = std::move(*opened);
    REQUIRE(db.insert(make_key(1), make_value(33, 1), 100).value());

    db.close();

    // Not success. There is nothing mapped and nothing this object still holds,
    // so reporting a successful sync would suggest a guarantee about a database
    // it has let go of. The answer is the same one every other operation gives.
    auto const synced = db.sync();
    REQUIRE_FALSE(synced);
    CHECK(synced.error() == utxoz::error_code::closed);
}

TEST_CASE("a version that rotated out still gets a barrier", "[database][sync][failpoint]") {
    if (utxoz::platform_durability() == durability_level::none) return;

    // The other way a version ends up owing a barrier, and the one with no
    // deletions in it at all. A version receives inserts, fills, and rotates.
    // From that moment it is not an active container any more, so the active
    // barriers do not reach it — and closing it only calls the asynchronous
    // flush, which schedules writeback and promises nothing.
    //
    // Nothing here erases anything, so the file cache is never involved. If
    // coverage came only from historical deletions, every one of these rotated
    // versions would be lost.
    auto const path = unique_path("rotatedout");
    fs::remove_all(path);
    scope_exit const cleanup([&] {
        failpoints::clear();
        std::error_code ec;
        fs::remove_all(path, ec);
    });

    auto opened = utxoz::full_db::open_for_testing(path, true);
    REQUIRE(opened);
    auto db = std::move(*opened);

    REQUIRE(db.sync());   // a clean start, so the count below is only this batch

    uint64_t next = 0;
    while (count_data_files(path) < 4) {
        REQUIRE(db.insert(make_key(next++), make_value(8, 1), 100).value());
        REQUIRE(next < 2'000'000);
    }

    // Nothing was deleted and nothing is cached: under a design that tracked
    // only historical deletions there would be nothing to flush beyond the
    // active containers.
    REQUIRE(db.get_cached_file_info().empty());

    failpoints::sync_file_calls.store(0, std::memory_order_relaxed);
    REQUIRE(db.sync());
    auto const barriers = failpoints::sync_file_calls.load(std::memory_order_relaxed);

    auto const rotated_out = count_data_files(path) - 1;
    REQUIRE(rotated_out == 3);
    INFO("file barriers: " << barriers << ", active containers: " << utxoz::container_count
         << ", rotated out: " << rotated_out);
    CHECK(barriers == utxoz::container_count + rotated_out);

    // Discharged: a second sync covers the actives and nothing else.
    failpoints::sync_file_calls.store(0, std::memory_order_relaxed);
    REQUIRE(db.sync());
    CHECK(failpoints::sync_file_calls.load(std::memory_order_relaxed) == utxoz::container_count);

    db.close();
}

TEST_CASE("a barrier failing partway through keeps every obligation",
          "[database][sync][failpoint]") {
    if (utxoz::platform_durability() == durability_level::none) return;

    // Blanket failure cannot reach past the first barrier a sync performs, so
    // it cannot show what happens when one in the *middle* fails — which is
    // where a partial result could be recorded. This fails one historical
    // barrier, after the active containers have already succeeded.
    auto const path = unique_path("partway");
    fs::remove_all(path);
    scope_exit const cleanup([&] {
        failpoints::clear();
        std::error_code ec;
        fs::remove_all(path, ec);
    });

    auto opened = utxoz::full_db::open_for_testing(path, true);
    REQUIRE(opened);
    auto db = std::move(*opened);
    REQUIRE(db.sync());

    uint64_t next = 0;
    while (count_data_files(path) < 4) {
        REQUIRE(db.insert(make_key(next++), make_value(8, 1), 100).value());
        REQUIRE(next < 2'000'000);
    }
    auto const rotated_out = count_data_files(path) - 1;
    REQUIRE(rotated_out == 3);

    auto const expected = utxoz::container_count + rotated_out;

    // The barrier after the last active container: the first historical one.
    failpoints::sync_file_calls.store(0, std::memory_order_relaxed);
    failpoints::fail_sync_file_at.store(utxoz::container_count + 1, std::memory_order_relaxed);

    auto const failed_sync = db.sync();
    auto const attempted = failpoints::sync_file_calls.load(std::memory_order_relaxed);
    failpoints::clear();

    REQUIRE_FALSE(failed_sync);
    CHECK(failed_sync.error() == utxoz::error_code::sync_failed);

    // It got as far as the failing one and stopped there.
    INFO("barriers attempted before the failure: " << attempted);
    CHECK(attempted == utxoz::container_count + 1);

    // And nothing was discharged — not the actives that succeeded, not the
    // historical ones that were never reached. The next sync owes all of them.
    failpoints::sync_file_calls.store(0, std::memory_order_relaxed);
    REQUIRE(db.sync());
    auto const retried = failpoints::sync_file_calls.load(std::memory_order_relaxed);
    INFO("barriers on the retry: " << retried << ", expected " << expected);
    CHECK(retried == expected);

    db.close();
}

TEST_CASE("a config that cannot be made durable stops the open", "[database][sync][failpoint]") {
    if (utxoz::platform_durability() == durability_level::none) return;

    // The config says whether a database is full or reference, so sync() leaves
    // it out of its promise on the grounds that creation made it durable. If
    // that did not happen, creation is the only place anyone can be told.
    SECTION("the file barrier") {
        auto const path = unique_path("configfilebarrier");
        fs::remove_all(path);
        scope_exit const cleanup([&] {
            failpoints::clear();
            std::error_code ec;
            fs::remove_all(path, ec);
        });

        failpoints::fail_sync_file.store(true, std::memory_order_relaxed);
        auto const opened = utxoz::full_db::open_for_testing(path, true);
        failpoints::clear();

        REQUIRE_FALSE(opened);
        CHECK(opened.error() == utxoz::error_code::sync_failed);

        // Published atomically, so a config that could not be made durable was
        // never given its name: there is no half-written one to read later.
        CHECK_FALSE(fs::exists(fs::path(path) / "utxoz_config.dat"));
        CHECK_FALSE(fs::exists(fs::path(path) / "utxoz_config.dat.tmp"));
    }

    SECTION("the directory barrier") {
        if (utxoz::platform_durability() != durability_level::full) return;

        auto const path = unique_path("configdirbarrier");
        fs::remove_all(path);
        scope_exit const cleanup([&] {
            failpoints::clear();
            std::error_code ec;
            fs::remove_all(path, ec);
        });

        failpoints::fail_sync_directory.store(true, std::memory_order_relaxed);
        auto const opened = utxoz::full_db::open_for_testing(path, true);
        failpoints::clear();

        REQUIRE_FALSE(opened);
        CHECK(opened.error() == utxoz::error_code::sync_failed);
    }
}
