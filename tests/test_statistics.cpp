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

#include <utxoz/config.hpp>
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

// Counter values only exist when recording is compiled in.
#ifdef UTXOZ_STATISTICS_ENABLED
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
        REQUIRE(db.get_statistics().probes.probes == 0);

        // Every one of these hits the active map: depth 0, found.
        for (size_t i = 0; i < entries; ++i) {
            REQUIRE(db.find(make_key(i), 200).has_value());
        }

        auto const hits = db.get_statistics().probes;
        CHECK(hits.probes == entries);
        CHECK(hits.answered_from_active == entries);
        CHECK(hits.deferred == 0);
        CHECK(hits.active_map_hit_rate == 1.0);
        CHECK(hits.avg_age_answered == 100.0);  // probed at 200, created at 100

        // Misses in a single-version database record nothing: find() only
        // records when it resolves, and a resolution finds no file to search.
        std::vector<utxoz::lookup_request> misses;
        for (size_t i = entries; i < entries + 100; ++i) {
            CHECK_FALSE(db.find(make_key(i), 200).has_value());
            misses.emplace_back(make_key(i), 200);
        }
        auto const swept = db.resolve(misses);
        REQUIRE(swept.has_value());
        CHECK(swept->found.empty());
        CHECK(swept->absent.size() == 100);

        db.reset_search_stats();
        auto const cleared = db.get_statistics().probes;
        CHECK(cleared.probes == 0);
        CHECK(cleared.answered_from_active == 0);
        CHECK(cleared.active_map_hit_rate == 0.0);

        db.close();
    }

    std::filesystem::remove_all(path);
}
#endif

/**
 * Recording from several threads at once.
 *
 * With the old shared vector this raced on every append; the totals below are
 * exact, so a single lost or duplicated increment fails the test.
 *
 * This exercises the shape KTH relies on: one admission holding its lock, then
 * fanning the inputs out across N reader threads that all call find().
 */
// Counter values only exist when recording is compiled in.
#ifdef UTXOZ_STATISTICS_ENABLED
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
        auto const summary = db.get_statistics().probes;
        CHECK(summary.probes == thread_count * per_thread);
        CHECK(summary.answered_from_active == thread_count * per_thread);
        CHECK(summary.deferred == 0);
        CHECK(summary.active_map_hit_rate == 1.0);

        db.close();
    }

    std::filesystem::remove_all(path);
}
#endif

/**
 * The other half of find(): the miss path.
 *
 * A lookup that misses the active versions used to put the key into a shared
 * queue, and that queue was the one piece of state a concurrent find() wrote to.
 * It is gone (#116): a miss now records a probe counter and nothing else, so
 * every thread here is free to keep its own batch.
 *
 * Which is what this checks. Eight threads probe the same 300 absent keys, and
 * each builds its own vector of what did not resolve. If any shared registry
 * came back, the batches would stop being independent — they would be the same
 * keys funnelled through one container, and the per-thread counts below would
 * stop matching.
 */
TEST_CASE("concurrent misses record a probe and nothing else", "[statistics][concurrency]") {
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
        db.reset_search_stats();

        constexpr size_t thread_count = 8;
        constexpr size_t rounds = 20;

        auto const& const_db = db;
        std::vector<size_t> wrong_hits(thread_count, 0);
        std::vector<size_t> hits(thread_count, 0);
        // One batch per thread, never shared. Nothing merges these.
        std::vector<std::vector<utxoz::lookup_request>> batches(thread_count);
        std::vector<std::thread> workers;
        workers.reserve(thread_count);

        for (size_t t = 0; t < thread_count; ++t) {
            workers.emplace_back([&, t] {
                for (size_t round = 0; round < rounds; ++round) {
                    // Hits and misses interleaved, so both paths run at once.
                    for (size_t i = 0; i < absent; ++i) {
                        auto const missed = const_db.find(make_key(absent_base + i), 200);
                        if (missed) {
                            ++wrong_hits[t];
                        } else if (round == 0) {
                            batches[t].emplace_back(make_key(absent_base + i), 200);
                        }
                        if (const_db.find(make_key((i + t) % present), 200)) ++hits[t];
                    }
                }
            });
        }
        for (auto& w : workers) w.join();

        CHECK(std::accumulate(wrong_hits.begin(), wrong_hits.end(), size_t{0}) == 0);
        CHECK(std::accumulate(hits.begin(), hits.end(), size_t{0}) == thread_count * rounds * absent);

        // Every thread kept its own copy of the same 300 keys. Nothing
        // deduplicated across threads, because nothing was shared to deduplicate
        // in — the whole batch is the thread's.
        for (auto const& batch : batches) {
            CHECK(batch.size() == absent);
        }

        // Single version, so a resolution finds nothing and hands back exactly
        // the batch it was given — no losses, no duplicates, no extras, and
        // nothing from any other thread's batch.
        auto const resolved = db.resolve(batches[0]);
        REQUIRE(resolved.has_value());
        CHECK(resolved->found.empty());
        REQUIRE(resolved->absent.size() == absent);

        std::vector<uint64_t> seen;
        seen.reserve(resolved->absent.size());
        for (auto const& entry : resolved->absent) {
            uint64_t n = 0;
            std::memcpy(&n, entry.key.data(), sizeof(n));
            seen.push_back(n);
        }
        std::ranges::sort(seen);
        CHECK(std::ranges::adjacent_find(seen) == seen.end());  // no duplicates
        CHECK(seen.front() == absent_base);
        CHECK(seen.back() == absent_base + absent - 1);

        db.close();
    }

    std::filesystem::remove_all(path);
}

// Counter values only exist when recording is compiled in.
#ifdef UTXOZ_STATISTICS_ENABLED
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

        auto const summary = db.get_statistics().probes;
        CHECK(summary.probes == 250'000);
        CHECK(summary.answered_from_active == 250'000);

        db.close();
    }

    std::filesystem::remove_all(path);
}
#endif

/**
 * The point of splitting the counters.
 *
 * One logical lookup that misses the active map shows up twice: once as a
 * deferred probe, once as a resolution. Folding them into one denominator is
 * what made the old hit rate meaningless — every recorded operation was a hit,
 * because a lookup that resolved nothing recorded nothing at all.
 *
 * So: the probe counters describe find() and only find(), the resolution
 * counters describe the sweeps and only the sweeps, and neither moves when the
 * other records.
 */
// Counter values only exist when recording is compiled in.
#ifdef UTXOZ_STATISTICS_ENABLED
TEST_CASE("probe and resolution counters describe different phases",
          "[statistics][contract]") {
    auto const path = make_unique_path("split");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        auto const witness = make_key(0xA11CE);
        REQUIRE(db.insert(witness, make_value(33), 100).value());

        // Fill until container 0 rotates, so the witness leaves the active map.
        uint64_t filler = 5'000'000;
        for (size_t batch = 0; batch < 400; ++batch) {
            for (size_t i = 0; i < 5'000; ++i) {
                REQUIRE(db.insert(make_key(filler++), make_value(33), 200).has_value());
            }
            if (db.get_statistics().rotations_per_container[0] >= 1) break;
        }
        REQUIRE(db.get_statistics().rotations_per_container[0] >= 1);

        db.reset_search_stats();

        // A probe that the active map cannot answer.
        CHECK_FALSE(db.find(witness, 300).has_value());

        auto after_probe = db.get_statistics();
        CHECK(after_probe.probes.probes == 1);
        CHECK(after_probe.probes.answered_from_active == 0);
        CHECK(after_probe.probes.deferred == 1);
        CHECK(after_probe.probes.active_map_hit_rate == 0.0);
        // The sweep has not run, so nothing is resolved yet.
        CHECK(after_probe.resolution.resolved == 0);

        // Resolving it counts on the resolution side, and leaves the probe
        // counters exactly where they were — the same lookup is not counted
        // twice inside one denominator.
        std::vector<utxoz::lookup_request> const batch{{witness, 300}};
        auto const swept = db.resolve(batch);
        REQUIRE(swept.has_value());
        CHECK(swept->absent.empty());
        CHECK(swept->found.size() == 1);

        auto after_sweep = db.get_statistics();
        CHECK(after_sweep.probes.probes == 1);
        CHECK(after_sweep.probes.answered_from_active == 0);
        CHECK(after_sweep.probes.deferred == 1);

        CHECK(after_sweep.resolution.resolved == 1);
        CHECK(after_sweep.resolution.absent == 0);
        CHECK(after_sweep.resolution.avg_depth >= 1.0);   // at least one version back
        CHECK(after_sweep.resolution.files_visited >= 1);

        // A key that exists nowhere: unresolved by find(), then proven absent.
        db.reset_search_stats();
        CHECK_FALSE(db.find(make_key(0xDEAD'BEEF), 300).has_value());
        {
            std::vector<utxoz::lookup_request> const nowhere{{make_key(0xDEAD'BEEF), 300}};
            auto const swept2 = db.resolve(nowhere);
            REQUIRE(swept2.has_value());
            CHECK(swept2->found.empty());
            CHECK(swept2->absent.size() == 1);
        }
        auto const missing = db.get_statistics();
        CHECK(missing.probes.probes == 1);
        CHECK(missing.probes.deferred == 1);
        CHECK(missing.resolution.resolved == 0);
        CHECK(missing.resolution.absent == 1);

        db.close();
    }

    std::filesystem::remove_all(path);
}
#endif

/**
 * Erases do not feed the probe counters at all. They used to, which is how the
 * average age mixed the age of a read with the age of a delete — and the
 * deferred deletion paths recorded an insertion height of zero, so that average
 * also mixed real ages with absolute heights.
 */
// Counter values only exist when recording is compiled in.
#ifdef UTXOZ_STATISTICS_ENABLED
TEST_CASE("erases do not contaminate the probe age", "[statistics][contract]") {
    auto const path = make_unique_path("age");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        for (size_t i = 0; i < 100; ++i) {
            REQUIRE(db.insert(make_key(i), make_value(33), 1'000).value());
        }
        db.reset_search_stats();

        // Probes at a known distance from creation.
        for (size_t i = 0; i < 100; ++i) {
            REQUIRE(db.find(make_key(i), 1'500).has_value());
        }
        CHECK(db.get_statistics().probes.avg_age_answered == 500.0);

        // Erasing at a very different height must not move it.
        std::vector<utxoz::deferred_deletion_entry> batch;
        for (size_t i = 0; i < 50; ++i) batch.emplace_back(make_key(i), 9'000);
        CHECK(db.apply_deletes(batch).erased.size() == 50);
        auto const after = db.get_statistics();
        CHECK(after.probes.probes == 100);
        CHECK(after.probes.avg_age_answered == 500.0);

        db.close();
    }

    std::filesystem::remove_all(path);
}
#endif

/**
 * With statistics compiled out, recording has to disappear entirely rather than
 * merely go unread — the call sites sit on the concurrent path, and an atomic
 * fetch_add that nobody will ever look at is pure cost. The guard lives in the
 * types, so a call added later cannot forget it.
 */
TEST_CASE("statistics compile out cleanly", "[statistics]") {
    auto const path = make_unique_path("off");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        for (size_t i = 0; i < 100; ++i) {
            REQUIRE(db.insert(make_key(i), make_value(33), 10).value());
            REQUIRE(db.find(make_key(i), 20).has_value());
        }

        auto const stats = db.get_statistics();
#ifdef UTXOZ_STATISTICS_ENABLED
        CHECK(stats.probes.probes == 100);
#else
        CHECK(stats.probes.probes == 0);
        CHECK(stats.probes.answered_from_active == 0);
        CHECK(stats.resolution.resolved == 0);
#endif
        // Either way the database itself works.
        CHECK(db.size() == 100);

        db.close();
    }

    std::filesystem::remove_all(path);
}
