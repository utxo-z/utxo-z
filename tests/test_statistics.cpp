// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_statistics.cpp
 * @brief Search statistics: aggregates, and recording them from many threads.
 *
 * The counters are what let find() be called concurrently. Recording used to
 * append to a shared std::vector, which both raced and grew without bound; these
 * tests pin the replacement — exact totals, no losses under concurrency, and a
 * reset that actually zeroes.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <numeric>
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

namespace {

inline std::atomic<uint64_t> stats_counter{0};

std::string make_unique_path(std::string_view tag) {
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fmt::format("./test_stats_{}_{}_{}_{}", tag, getpid(), ts, stats_counter.fetch_add(1));
}

utxoz::raw_outpoint make_key(uint64_t n) {
    utxoz::raw_outpoint key{};
    std::memcpy(key.data(), &n, sizeof(n));
    return key;
}

std::vector<uint8_t> make_value(size_t size) {
    std::vector<uint8_t> v(size);
    std::iota(v.begin(), v.end(), 7);
    return v;
}

} // anonymous namespace

TEST_CASE("search statistics count every lookup exactly once", "[statistics]") {
    auto const path = make_unique_path("counts");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        constexpr size_t entries = 500;
        for (size_t i = 0; i < entries; ++i) {
            REQUIRE(db.insert(make_key(i), make_value(33), 100).value());
        }

        // Inserts do not record searches.
        REQUIRE(db.get_statistics().search.total_operations == 0);

        // Every one of these hits the active map: depth 0, found.
        for (size_t i = 0; i < entries; ++i) {
            REQUIRE(db.find(make_key(i), 200).has_value());
        }

        auto const hits = db.get_statistics().search;
        CHECK(hits.total_operations == entries);
        CHECK(hits.current_version_hits == entries);
        // Note: every call site records found=true — a lookup that resolves
        // nothing records nothing at all — so found_operations tracks
        // total_operations and hit_rate is structurally 1.0. Pinned as the
        // current behaviour, not as a meaningful hit rate.
        CHECK(hits.found_operations == entries);
        CHECK(hits.hit_rate == 1.0);
        CHECK(hits.avg_depth == 0.0);
        CHECK(hits.avg_utxo_age == 100.0);  // accessed at 200, inserted at 100

        // Misses in a single-version database record nothing: find() only
        // records when it resolves, and the sweep finds no file to search.
        for (size_t i = entries; i < entries + 100; ++i) {
            CHECK_FALSE(db.find(make_key(i), 200).has_value());
        }
        auto const [resolved, unresolved] = db.process_pending_lookups();
        CHECK(resolved.empty());
        CHECK(unresolved.size() == 100);

        db.reset_search_stats();
        auto const cleared = db.get_statistics().search;
        CHECK(cleared.total_operations == 0);
        CHECK(cleared.found_operations == 0);
        CHECK(cleared.hit_rate == 0.0);

        db.close();
    }

    std::filesystem::remove_all(path);
}

/**
 * Recording from several threads at once.
 *
 * With the old shared vector this raced on every append; the totals below are
 * exact, so a single lost or duplicated increment fails the test.
 *
 * This exercises the shape KTH relies on: one admission holding its lock, then
 * fanning the inputs out across N reader threads that all call find().
 */
TEST_CASE("search statistics survive concurrent recorders", "[statistics][concurrency]") {
    auto const path = make_unique_path("threads");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        constexpr size_t entries = 2'000;
        for (size_t i = 0; i < entries; ++i) {
            REQUIRE(db.insert(make_key(i), make_value(33), 100).value());
        }
        db.reset_search_stats();

        constexpr size_t thread_count = 8;
        constexpr size_t per_thread = 5'000;

        auto const& const_db = db;
        std::vector<size_t> misses(thread_count, 0);
        std::vector<std::thread> workers;
        workers.reserve(thread_count);

        for (size_t t = 0; t < thread_count; ++t) {
            workers.emplace_back([&, t] {
                for (size_t i = 0; i < per_thread; ++i) {
                    auto const key = make_key((i * 7 + t) % entries);
                    if (!const_db.find(key, 200)) ++misses[t];
                }
            });
        }
        for (auto& w : workers) w.join();

        CHECK(std::accumulate(misses.begin(), misses.end(), size_t{0}) == 0);

        // Exact: no increments lost to a race, none double counted.
        auto const summary = db.get_statistics().search;
        CHECK(summary.total_operations == thread_count * per_thread);
        CHECK(summary.found_operations == thread_count * per_thread);
        CHECK(summary.current_version_hits == thread_count * per_thread);
        CHECK(summary.hit_rate == 1.0);
        CHECK(summary.avg_depth == 0.0);

        db.close();
    }

    std::filesystem::remove_all(path);
}

/**
 * The other half of find(): the miss path.
 *
 * A lookup that misses the active map queues the key instead of answering, and
 * that queue is the one piece of shared state a concurrent find() writes to. The
 * hit path never touches it, so the test above does not cover it at all.
 *
 * Every thread here asks for the same absent keys, so the queue also has to
 * deduplicate: one entry per distinct key no matter how many threads asked. The
 * database has a single version, so the sweep afterwards can resolve nothing and
 * must hand back exactly the set that was asked for.
 */
TEST_CASE("concurrent misses queue exactly once per key", "[statistics][concurrency]") {
    auto const path = make_unique_path("misses");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        constexpr size_t present = 500;
        constexpr size_t absent = 300;
        constexpr uint64_t absent_base = 1'000'000;

        for (size_t i = 0; i < present; ++i) {
            REQUIRE(db.insert(make_key(i), make_value(33), 100).value());
        }
        REQUIRE(db.deferred_lookups_size() == 0);
        db.reset_search_stats();

        constexpr size_t thread_count = 8;
        constexpr size_t rounds = 20;

        auto const& const_db = db;
        std::vector<size_t> wrong_hits(thread_count, 0);
        std::vector<size_t> hits(thread_count, 0);
        std::vector<std::thread> workers;
        workers.reserve(thread_count);

        for (size_t t = 0; t < thread_count; ++t) {
            workers.emplace_back([&, t] {
                for (size_t round = 0; round < rounds; ++round) {
                    // Hits and misses interleaved, so both paths run at once.
                    for (size_t i = 0; i < absent; ++i) {
                        if (const_db.find(make_key(absent_base + i), 200)) ++wrong_hits[t];
                        if (const_db.find(make_key((i + t) % present), 200)) ++hits[t];
                    }
                }
            });
        }
        for (auto& w : workers) w.join();

        CHECK(std::accumulate(wrong_hits.begin(), wrong_hits.end(), size_t{0}) == 0);
        CHECK(std::accumulate(hits.begin(), hits.end(), size_t{0}) == thread_count * rounds * absent);

        // Deduplicated by key: every thread asked for the same 300 outpoints.
        CHECK(db.deferred_lookups_size() == absent);

        // Single version, so the sweep resolves nothing and returns exactly the
        // set that was queued — no losses, no duplicates, no extras.
        auto const [resolved, unresolved] = db.process_pending_lookups();
        CHECK(resolved.empty());
        REQUIRE(unresolved.size() == absent);

        std::vector<uint64_t> seen;
        seen.reserve(unresolved.size());
        for (auto const& entry : unresolved) {
            uint64_t n = 0;
            std::memcpy(&n, entry.key.data(), sizeof(n));
            seen.push_back(n);
        }
        std::ranges::sort(seen);
        CHECK(std::ranges::adjacent_find(seen) == seen.end());  // no duplicates
        CHECK(seen.front() == absent_base);
        CHECK(seen.back() == absent_base + absent - 1);

        CHECK(db.deferred_lookups_size() == 0);

        db.close();
    }

    std::filesystem::remove_all(path);
}

TEST_CASE("search statistics stay bounded", "[statistics]") {
    auto const path = make_unique_path("bounded");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        REQUIRE(db.insert(make_key(1), make_value(33), 1).value());

        // Recording is O(1) in space: a quarter of a million lookups used to be
        // a quarter of a million retained records.
        for (size_t i = 0; i < 250'000; ++i) {
            (void)db.find(make_key(1), 2);
        }

        auto const summary = db.get_statistics().search;
        CHECK(summary.total_operations == 250'000);
        CHECK(summary.found_operations == 250'000);

        db.close();
    }

    std::filesystem::remove_all(path);
}
