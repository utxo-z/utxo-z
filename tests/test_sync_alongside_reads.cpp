// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * The second half of the barrier while find() and resolve() run.
 *
 * A caller that has finished a batch of mutations owes the barrier before it
 * may call the batch durable, and the barrier is most of what the batch costs:
 * it writes every page the batch touched. Nothing a lookup reads changes while
 * it does, so the lookups of whatever comes next do not have to wait for it —
 * except for the one structure the two share, the file cache. So the barrier
 * comes in two calls, and only the first, which flushes through that cache,
 * excludes them.
 *
 * Each round here mutates and calls begin_sync() alone, as the contract
 * requires, and then starts the second half and the readers together. The mutation is there so the barrier has
 * something to do in every part of it: inserts dirty the active containers,
 * deletions dirty older generations through the cache, and a resolution over
 * several generations evicts from that cache while the barrier is out.
 *
 * Run under ThreadSanitizer. Nothing in these cases takes a lock.
 */

#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <latch>
#include <numeric>
#include <string>
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
#include "detail/scope_exit.hpp"
#include "detail/shard.hpp"
#include "support/sync_barrier.hpp"

using utxoz::detail::failpoints;
using utxoz::detail::scope_exit;
using utxoz::lookup_request;

namespace {

std::string unique_path(std::string_view tag) {
    auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fmt::format("./test_syncread_{}_{}_{}", tag, getpid(), ts);
}

utxoz::raw_outpoint outpoint_of(uint64_t n) {
    utxoz::raw_outpoint key{};
    std::memcpy(key.data(), &n, sizeof(n));
    key[20] = 0x5A;
    return key;
}

std::vector<uint8_t> value_of(uint8_t seed) {
    std::vector<uint8_t> v(33);
    std::iota(v.begin(), v.end(), seed);
    return v;
}

// Keys n in [first, first + count) with n % stride == residue.
std::vector<uint64_t> every(uint64_t first, uint64_t count, uint64_t stride, uint64_t residue) {
    std::vector<uint64_t> out;
    for (uint64_t n = first; n < first + count; ++n) {
        if (n % stride == residue) out.push_back(n);
    }
    return out;
}

// How far the lookups of the current round have gone, for the barrier to
// wait on. File scope because the hook is a plain function pointer.
std::atomic<uint64_t> g_finds_this_round{0};
std::atomic<uint64_t> g_resolves_this_round{0};
std::atomic<uint64_t> g_barriers_held{0};

// Holds the barrier, part done, until a find() and a resolve() have each
// started and completed inside it.
void hold_until_both_lookups_have_gone_round() {
    auto const finds = g_finds_this_round.load();
    auto const resolves = g_resolves_this_round.load();
    // Bounded: if the barrier ever came to block the lookups, they would be
    // waiting for it while it waited for them here. Letting go unheld leaves
    // the count below short, and that is what reports it.
    auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    // Two more of each: the first may have started before the barrier did.
    while (g_finds_this_round.load() < finds + 2 || g_resolves_this_round.load() < resolves + 2) {
        if (std::chrono::steady_clock::now() >= deadline) return;
        std::this_thread::yield();
    }
    ++g_barriers_held;
}

// The same answer, entry for entry: which keys were found, with what, and
// which were proven absent.
bool same_resolution(utxoz::full_resolution const& got, utxoz::full_resolution const& expected) {
    if (got.found.size() != expected.found.size()) return false;
    for (auto const& [key, entry] : expected.found) {
        auto const it = got.found.find(key);
        if (it == got.found.end()) return false;
        if (it->second.data != entry.data || it->second.block_height != entry.block_height) return false;
    }
    return got.absent == expected.absent;
}

} // namespace

TEST_CASE("full: the second half of a barrier may run while find() and resolve() do",
          "[database][sync][concurrency][unguarded]") {
    if (utxoz::platform_durability() == utxoz::durability_level::none) return;

    failpoints::clear();
    auto const path = unique_path("full");
    std::filesystem::remove_all(path);
    scope_exit const cleanup([&] {
        failpoints::clear();
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    });

    auto opened = utxoz::full_db::open_for_testing(path, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    // Three generations of one class, so a resolution has two older files to
    // walk and the cache, which holds one, evicts on the way.
    constexpr uint64_t first = 10'000;
    uint64_t next = first;
    while (db.get_statistics().rotations_per_container[0] < 2) {
        for (int i = 0; i < 5'000; ++i, ++next) {
            REQUIRE(db.insert(outpoint_of(next), utxoz::output_data_span{value_of(7)}, 2).has_value());
        }
        REQUIRE(next < 4'000'000);
    }
    uint64_t const stored = next - first;

    REQUIRE(utxoz::testing::sync_barrier(db));

    constexpr uint64_t stride = 997;
    // Residue 0 is what the resolver asks for and nothing ever deletes.
    std::vector<lookup_request> batch;
    for (auto const n : every(first, stored, stride, 0)) batch.emplace_back(outpoint_of(n), 100'000);
    REQUIRE(batch.size() > 8);

    auto const& const_db = db;

    constexpr int rounds = 24;
    g_barriers_held = 0;
    std::atomic<int> sync_failed{0};
    std::atomic<int> reader_wrong{0};
    std::atomic<int> resolver_wrong{0};
    std::atomic<uint64_t> finds_during{0};
    std::atomic<uint64_t> resolves_during{0};

    for (int round = 0; round < rounds; ++round) {
        // Alone: what the barrier of this round is for.
        for (uint64_t i = 0; i < 400; ++i) {
            auto const n = 500'000'000ULL + uint64_t(round) * 1'000 + i;
            REQUIRE(db.insert(outpoint_of(n), utxoz::output_data_span{value_of(uint8_t(round))}, 3).has_value());
        }
        std::vector<utxoz::deferred_deletion_entry> owed;
        for (auto const n : every(first, stored, stride, uint64_t(round) + 1)) {
            owed.emplace_back(outpoint_of(n), 400);
        }
        auto const progress = db.apply_deletes(owed);
        REQUIRE(progress.unresolved.empty());
        REQUIRE( ! progress.error);
        REQUIRE(progress.erased.size() == owed.size());

        // What the lookups of this round must keep answering, established
        // alone: a key this round inserted, which the active generation holds
        // unless the round's own inserts rotated it out, and the batch.
        auto const witness = outpoint_of(500'000'000ULL + uint64_t(round) * 1'000);
        auto const expected_witness = const_db.find(witness, 100'000);
        auto const baseline = const_db.resolve(batch);
        REQUIRE(baseline.has_value());
        auto const expected_found = baseline->found.size();
        REQUIRE(expected_found > 0);

        // The half that excludes the lookups, before any of them starts.
        auto pending = db.begin_sync();
        REQUIRE(pending.has_value());

        g_finds_this_round = 0;
        g_resolves_this_round = 0;
        failpoints::inside_finish_sync.store(&hold_until_both_lookups_have_gone_round);

        std::latch start(3);
        std::atomic<bool> barrier_returned{false};

        std::thread syncer([&] {
            start.count_down();
            start.wait();
            if ( ! std::move(*pending).finish()) ++sync_failed;
            barrier_returned = true;
        });
        std::thread reader([&] {
            start.count_down();
            start.wait();
            // At least once, and for as long as the barrier is out.
            do {
                auto const hit = const_db.find(witness, 100'000);
                if (hit.has_value() != expected_witness.has_value()
                        || (hit && hit->data != expected_witness->data)) {
                    ++reader_wrong;
                }
                // Below the active generation: a miss here, a resolution's to answer.
                auto const miss = const_db.find(batch.front().key, 100'000);
                if (miss || miss.error() != utxoz::error_code::not_resolved) ++reader_wrong;
                ++finds_during;
                ++g_finds_this_round;
            } while ( ! barrier_returned.load());
        });
        std::thread resolver([&] {
            start.count_down();
            start.wait();
            do {
                auto const got = const_db.resolve(batch);
                if ( ! got || ! same_resolution(*got, *baseline)) ++resolver_wrong;
                ++resolves_during;
                ++g_resolves_this_round;
            } while ( ! barrier_returned.load());
        });

        syncer.join();
        reader.join();
        resolver.join();
        failpoints::inside_finish_sync.store(nullptr);
    }

    // Every round's barrier was held open across whole lookups of both kinds:
    // the overlap is arranged, not left to the scheduler.
    CHECK(g_barriers_held.load() == uint64_t(rounds));

    INFO("finds " << finds_during.load() << ", resolutions " << resolves_during.load());
    CHECK(sync_failed.load() == 0);
    CHECK(reader_wrong.load() == 0);
    CHECK(resolver_wrong.load() == 0);

    // The barriers were whole ones: every obligation the rounds recorded was
    // discharged by the sync of its round, so one more has only the active
    // containers left to cover.
    failpoints::sync_file_calls.store(0, std::memory_order_relaxed);
    REQUIRE(utxoz::testing::sync_barrier(db));
    CHECK(failpoints::sync_file_calls.load(std::memory_order_relaxed) == utxoz::detail::slot_count);

    db.close();
}

TEST_CASE("full: a pending barrier refuses to skip a deletion's pages",
          "[database][sync]") {
    if (utxoz::platform_durability() == utxoz::durability_level::none) return;

    failpoints::clear();
    auto const path = unique_path("order");
    std::filesystem::remove_all(path);
    scope_exit const cleanup([&] {
        failpoints::clear();
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    });

    auto opened = utxoz::full_db::open_for_testing(path, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    constexpr uint64_t first = 10'000;
    uint64_t next = first;
    while (db.get_statistics().rotations_per_container[0] < 1) {
        for (int i = 0; i < 5'000; ++i, ++next) {
            REQUIRE(db.insert(outpoint_of(next), utxoz::output_data_span{value_of(7)}, 2).has_value());
        }
        REQUIRE(next < 4'000'000);
    }
    REQUIRE(utxoz::testing::sync_barrier(db));

    // A deletion in the oldest generation writes through the cache.
    std::vector<utxoz::deferred_deletion_entry> owed{{outpoint_of(first), 400}};
    auto const progress = db.apply_deletes(owed);
    REQUIRE(progress.erased.size() == 1);

    // In order, it is the whole barrier: every active container and the
    // generation the deletion wrote to.
    failpoints::sync_file_calls.store(0, std::memory_order_relaxed);
    {
        auto pending = db.begin_sync();
        REQUIRE(pending.has_value());
        REQUIRE(std::move(*pending).finish());
        CHECK(failpoints::sync_file_calls.load(std::memory_order_relaxed) == utxoz::detail::slot_count + 1);

        // Consumed: there is nothing left for a second call to finish.
        auto const twice = std::move(*pending).finish();
        REQUIRE_FALSE(twice.has_value());
        CHECK(twice.error() == utxoz::error_code::sync_failed);
    }

    // An insert between the halves writes only an active container, which the
    // second half covers.
    {
        auto pending = db.begin_sync();
        REQUIRE(pending.has_value());
        REQUIRE(db.insert(outpoint_of(700'000'000), utxoz::output_data_span{value_of(9)}, 5).has_value());
        CHECK(std::move(*pending).finish());
    }

    // A deletion between them writes pages the first half did not flush. The
    // second refuses, and makes no file barrier over them.
    {
        std::vector<utxoz::deferred_deletion_entry> again{{outpoint_of(first + 1), 401}};
        auto pending = db.begin_sync();
        REQUIRE(pending.has_value());
        REQUIRE(db.apply_deletes(again).erased.size() == 1);
        failpoints::sync_file_calls.store(0, std::memory_order_relaxed);
        auto const skipped = std::move(*pending).finish();
        REQUIRE_FALSE(skipped.has_value());
        CHECK(skipped.error() == utxoz::error_code::sync_failed);
        CHECK(failpoints::sync_file_calls.load(std::memory_order_relaxed) == 0);
    }

    // One dropped unfinished makes nothing durable and breaks nothing: the
    // next barrier is a whole one.
    {
        auto dropped = db.begin_sync();
        REQUIRE(dropped.has_value());
    }
    CHECK(utxoz::testing::sync_barrier(db));

    // It does not keep the database open.
    auto late = db.begin_sync();
    REQUIRE(late.has_value());
    db.close();
    auto const closed = std::move(*late).finish();
    REQUIRE_FALSE(closed.has_value());
    CHECK(closed.error() == utxoz::error_code::closed);
}
