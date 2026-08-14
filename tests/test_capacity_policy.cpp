// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_capacity_policy.cpp
 * @brief That a generation's bucket count never changes, and where the line is.
 *
 * The store's answer to a container filling up is a new generation; compaction
 * deals with the cost afterwards. A map that grew instead is a defect even when
 * there was room for it, because the file it wrote is not the file the policy
 * describes and the growth copied every entry to get there.
 *
 * Three levels, because they fail separately:
 *
 *  - the boundary as arithmetic, where an off-by-one lives;
 *  - the boundary as behaviour, driven through the ordinary insert path at a size
 *    small enough for every CI run;
 *  - the invariant itself, which no test can assert away: a counter the store
 *    keeps whatever it was built with.
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
#include <utxoz/types.hpp>

#include <boost/unordered/unordered_flat_map.hpp>

#include "detail/capacity_policy.hpp"
#include "detail/file_cache.hpp"
#include "detail/segment_open.hpp"
#include "detail/utxo_value.hpp"

namespace fs = std::filesystem;
using namespace utxoz::detail;

namespace {

struct temp_db {
    temp_db() {
        static std::atomic<uint64_t> counter{0};
        auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        dir = fmt::format("./test_capacity_{}_{}_{}", getpid(), ts, counter.fetch_add(1));
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
    ~temp_db() {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
    temp_db(temp_db const&) = delete;
    temp_db& operator=(temp_db const&) = delete;
    fs::path dir;
};

utxoz::raw_outpoint key_of(uint64_t n) {
    utxoz::raw_outpoint k{};
    std::memcpy(k.data(), &n, sizeof(n));
    return k;
}

/// The bucket count and entry count of one generation, read from its file.
template <size_t Index>
std::pair<size_t, size_t> generation_of(fs::path const& dir, size_t version) {
    constexpr size_t S = utxoz::container_sizes[Index];
    auto const file = dir / fmt::format(data_file_format, Index, version);
    REQUIRE(fs::exists(file));
    auto seg = open_existing_segment(file);
    REQUIRE(seg.has_value());
    auto found = find_single_named<utxo_map<S>>(**seg, map_object_name, file);
    REQUIRE(found.has_value());
    return {(**found).bucket_count(), (**found).size()};
}

/// How far a scan looks for generations of one container. One number rather than
/// a bound per loop: a case that scanned fewer than another would miss a
/// generation and report a smaller database, silently and only sometimes.
constexpr size_t max_versions_scanned = 64;

/// The ladder as arithmetic, in a type wide enough for every step these tests
/// name. `bucket_step` returns `size_t`, and the overflow checks below reach past
/// what a 32-bit `size_t` holds on purpose — the point of them is that
/// `max_entries_for` takes a `uint64_t` and stays exact there. Shifting a
/// `size_t` by 35 is undefined where `size_t` is 32 bits, so the shift is done in
/// the wider type and tied to the real one where both can represent the answer.
constexpr uint64_t wide_bucket_step(unsigned k) { return (uint64_t(15) << k) - 1; }
static_assert(wide_bucket_step(20) == bucket_step(20));

size_t generations(fs::path const& dir, size_t index) {
    size_t n = 0;
    for (size_t v = 0; v < max_versions_scanned; ++v) {
        if (fs::exists(dir / fmt::format(data_file_format, index, v))) ++n;
    }
    return n;
}

} // namespace

// =============================================================================
// A. The boundary as arithmetic
// =============================================================================

TEST_CASE("the rotation threshold is 133/160 of the buckets, in integers", "[capacity]") {
    // 0.875 is where Boost grows; 0.95 of that is where the store rotates. The
    // product is exact in integers and is not in a float, which is the reason it
    // is written as a fraction.
    STATIC_REQUIRE(rotation_numerator == 133);
    STATIC_REQUIRE(rotation_denominator == 160);

    // Inclusive: this many entries may be present. The next insert rotates.
    STATIC_REQUIRE(max_entries_for(15728639) == 13074431);   // container 0, production
    STATIC_REQUIRE(max_entries_for(7864319) == 6537215);     // a generation from before
    STATIC_REQUIRE(max_entries_for(122879) == 102143);       // the fixtures' container 0
    STATIC_REQUIRE(max_entries_for(959) == 797);             // the smallest, used below

    // Always strictly below where Boost would grow, at every step of the ladder.
    for (unsigned k = 5; k < 30; ++k) {
        auto const buckets = wide_bucket_step(k);
        INFO("step " << k << ", " << buckets << " buckets");
        CHECK(max_entries_for(buckets) < uint64_t(double(buckets) * 0.875));
    }

    // And it does not overflow: a bucket count far larger than anything here could
    // ask for, times 133, is still inside 64 bits.
    CHECK(max_entries_for(wide_bucket_step(35)) > 0);
    CHECK(max_entries_for(wide_bucket_step(35)) < wide_bucket_step(35));
}

TEST_CASE("the policy states container 0 exactly", "[capacity]") {
    // Not "at least". These are the numbers a new production generation gets.
    STATIC_REQUIRE(production_capacity[0].file_size == 1405091840);      // 1340 MiB
    STATIC_REQUIRE(production_capacity[0].bucket_count == 15728639);
    STATIC_REQUIRE(next_bucket_step(production_capacity[0].bucket_count) == 31457279);
    STATIC_REQUIRE(max_entries_for(production_capacity[0].bucket_count) == 13074431);
    STATIC_REQUIRE(production_capacity[0].certified);

    // The other four are carried over untouched, and say so.
    STATIC_REQUIRE(production_capacity[1].bucket_count == 3932159);
    STATIC_REQUIRE(production_capacity[2].bucket_count == 3932159);
    STATIC_REQUIRE(production_capacity[3].bucket_count == 122879);
    STATIC_REQUIRE(production_capacity[4].bucket_count == 959);
    STATIC_REQUIRE_FALSE(production_capacity[1].certified);
    STATIC_REQUIRE_FALSE(production_reference.certified);
}

TEST_CASE("the test profile keeps the counts the fixtures were written with",
          "[capacity]") {
    // A formula that produced anything else here would change every compatibility
    // fixture for a reason that has nothing to do with the format. Container 0's
    // floor at 10 MiB leaves 0.27% of slack, so the production margin does not
    // apply to this profile — these are measured, not derived.
    STATIC_REQUIRE(testing_capacity[0].bucket_count == 122879);
    STATIC_REQUIRE(testing_capacity[1].bucket_count == 61439);
    STATIC_REQUIRE(testing_capacity[2].bucket_count == 61439);
    STATIC_REQUIRE(testing_capacity[3].bucket_count == 30719);
    STATIC_REQUIRE(testing_capacity[4].bucket_count == 959);
    STATIC_REQUIRE(testing_reference.bucket_count == 122879);
    for (auto const& e : testing_capacity) {
        CHECK(e.file_size == 10u * 1024 * 1024);
    }
}

// =============================================================================
// B. The boundary as behaviour, cheaply
// =============================================================================

TEST_CASE("a container rotates on the insert after its last, and never grows",
          "[capacity]") {
    // Container 4 in the test profile: 959 buckets, full at 797 entries. Small
    // enough to run in every CI job, and it goes through the same guard, the same
    // rotation and the same new-generation path as container 0 at thirteen
    // million. What is under test is the mechanism, not the number.
    temp_db t;
    auto const before = rehashes_observed.load(std::memory_order_relaxed);

    constexpr size_t index = 4;
    auto const limit = max_entries_for(testing_capacity[index].bucket_count);
    REQUIRE(limit == 797);

    std::vector<uint8_t> const value(utxoz::container_capacities[index], 0x5A);
    {
        auto opened = utxoz::full_db::open_for_testing(t.dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        for (uint64_t i = 0; i < limit; ++i) {
            REQUIRE(db.insert(key_of(i), value, 800000).has_value());
        }
        db.close();
    }

    // Everything still in one generation, at the count it was created with.
    CHECK(generations(t.dir, index) == 1);
    {
        auto const [buckets, entries] = generation_of<index>(t.dir, 0);
        CHECK(buckets == testing_capacity[index].bucket_count);
        CHECK(entries == limit);
    }

    // One more, and it goes somewhere new.
    {
        auto opened = utxoz::full_db::open_for_testing(t.dir, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        REQUIRE(db.insert(key_of(limit), value, 800000).has_value());
        CHECK(db.size() == limit + 1);
        db.close();
    }

    CHECK(generations(t.dir, index) == 2);
    {
        auto const [buckets, entries] = generation_of<index>(t.dir, 0);
        INFO("the generation that filled up");
        CHECK(buckets == testing_capacity[index].bucket_count);   // did not grow
        CHECK(entries == limit);                                  // and kept its entries
    }
    {
        auto const [buckets, entries] = generation_of<index>(t.dir, 1);
        INFO("the generation that was made for the next entry");
        CHECK(buckets == testing_capacity[index].bucket_count);   // its own, from the policy
        CHECK(entries == 1);
    }

    // Nothing lost, nothing duplicated, and both generations reachable.
    //
    // Through resolve() for the ones that were left behind: since #116 find()
    // answers from the newest generation and defers the rest rather than
    // pretending they are absent, so an entry in a rotated-out generation comes
    // back from the batch and not from find().
    auto reopened = utxoz::full_db::open_for_testing(t.dir, false);
    REQUIRE(reopened.has_value());
    CHECK(reopened->size() == limit + 1);

    // The one in the new generation is answered inline.
    {
        auto const found = reopened->find(key_of(limit), 800001);
        REQUIRE(found.has_value());
        CHECK(std::ranges::equal(found->data, value));
    }

    // The ones in the generation that filled up are all still there.
    std::vector<utxoz::lookup_request> batch;
    for (uint64_t i : {uint64_t(0), limit / 2, limit - 1}) batch.push_back({key_of(i), 800001});
    auto const swept = reopened->resolve(batch);
    REQUIRE(swept.has_value());
    CHECK(swept->absent.empty());
    for (auto const& r : batch) {
        INFO("an entry from the generation that filled up");
        REQUIRE(swept->found.contains(r.key));
        CHECK(std::ranges::equal(swept->found.at(r.key).data, value));
    }
    reopened->close();

    // And the store never grew a map to get there.
    CHECK(rehashes_observed.load(std::memory_order_relaxed) == before);
}

TEST_CASE("reference rotates the same way, and never grows", "[capacity]") {
    // Reference has one container and its own guard, so it needs its own pass
    // through the boundary rather than an argument by analogy.
    temp_db t;
    auto const before = rehashes_observed.load(std::memory_order_relaxed);
    auto const limit = max_entries_for(testing_reference.bucket_count);
    REQUIRE(limit == 102143);

    {
        auto opened = utxoz::reference_db::open_for_testing(t.dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        for (uint64_t i = 0; i < limit; ++i) {
            REQUIRE(db.insert(key_of(i), uint32_t(i % 1000), uint32_t(i), 800000).has_value());
        }
        db.close();
    }
    CHECK(fs::exists(t.dir / fmt::format(reference_data_file_format, 0)));
    CHECK_FALSE(fs::exists(t.dir / fmt::format(reference_data_file_format, 1)));

    {
        auto opened = utxoz::reference_db::open_for_testing(t.dir, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        REQUIRE(db.insert(key_of(limit), 1, 1, 800000).has_value());
        CHECK(db.size() == limit + 1);
        db.close();
    }
    CHECK(fs::exists(t.dir / fmt::format(reference_data_file_format, 1)));

    auto reopened = utxoz::reference_db::open_for_testing(t.dir, false);
    REQUIRE(reopened.has_value());
    CHECK(reopened->size() == limit + 1);

    // The newest generation answers inline; the rotated-out one answers through
    // the batch, for the same reason as in full mode.
    CHECK(reopened->find(key_of(limit), 800001).has_value());
    std::vector<utxoz::lookup_request> const batch{{key_of(0), 800001}};
    auto const swept = reopened->resolve(batch);
    REQUIRE(swept.has_value());
    CHECK(swept->absent.empty());
    CHECK(swept->found.contains(key_of(0)));
    reopened->close();

    CHECK(rehashes_observed.load(std::memory_order_relaxed) == before);
}

// =============================================================================
// Compaction is held to the same invariant
// =============================================================================

TEST_CASE("compaction merges what fits and refuses what would grow the target",
          "[capacity]") {
    // Deterministic on both sides, because a test that accepts either outcome
    // cannot tell the guard from its absence — which is exactly what an earlier
    // version of this case did.
    //
    // The rotation seam makes generations of a chosen size instead of full ones,
    // so the totals can be put either side of the line on purpose.
    constexpr size_t index = 4;
    auto const buckets = testing_capacity[index].bucket_count;     // 959
    auto const growth = max_size_without_rehash(buckets);          // 839
    std::vector<uint8_t> const value(utxoz::container_capacities[index], 0x5A);

    auto build = [&](fs::path const& dir, size_t generations_wanted, size_t per_generation) {
        failpoints::scoped_reset const disarm;
        auto opened = utxoz::full_db::open_for_testing(dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        uint64_t n = 0;
        for (size_t g = 0; g < generations_wanted; ++g) {
            for (size_t i = 0; i < per_generation; ++i) {
                REQUIRE(db.insert(key_of(++n), value, 800000).has_value());
            }
            if (g + 1 < generations_wanted) {
                failpoints::force_rotations.store(1, std::memory_order_relaxed);
                REQUIRE(db.insert(key_of(++n), value, 800000).has_value());
            }
        }
        db.close();
        return n;
    };

    SECTION("three small generations fit in one target and are merged") {
        temp_db t;
        auto const before = rehashes_observed.load(std::memory_order_relaxed);
        auto const inserted = build(t.dir, 3, 200);          // ~600, under 839
        auto const files_before = generations(t.dir, index);
        REQUIRE(files_before >= 3);

        auto opened = utxoz::full_db::open_for_testing(t.dir, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        REQUIRE(db.compact_all().has_value());
        CHECK(db.size() == inserted);
        db.close();

        CHECK(generations(t.dir, index) < files_before);
        CHECK(rehashes_observed.load(std::memory_order_relaxed) == before);
        for (size_t v = 0; v < max_versions_scanned; ++v) {
            if ( ! fs::exists(t.dir / fmt::format(data_file_format, index, v))) continue;
            auto const [b, entries] = generation_of<index>(t.dir, v);
            INFO("generation " << v << " holds " << entries);
            CHECK(b == buckets);
            CHECK(entries <= growth);
        }
    }

    SECTION("generations that would overflow the target are refused, not grown") {
        temp_db t;
        auto const before = rehashes_observed.load(std::memory_order_relaxed);
        auto const inserted = build(t.dir, 3, 500);          // ~1500, well over 839
        auto const files_before = generations(t.dir, index);

        auto opened = utxoz::full_db::open_for_testing(t.dir, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        auto const outcome = db.compact_all();
        // Required, not allowed either way: a group that does not fit is refused
        // inside the walk, which retries with fewer sources and reports that it
        // finished. `insufficient_space` never reaches the caller from here — the
        // refusal is read in the files, and the measured return value is success.
        // `duplicate_key` is the one refusal that does come back, and it has its
        // own case.
        REQUIRE(outcome.has_value());
        CHECK(db.size() == inserted);
        db.close();

        // Whatever it merged, nothing grew and nothing exceeded the line.
        CHECK(rehashes_observed.load(std::memory_order_relaxed) == before);
        size_t surviving = 0;
        for (size_t v = 0; v < max_versions_scanned; ++v) {
            if ( ! fs::exists(t.dir / fmt::format(data_file_format, index, v))) continue;
            ++surviving;
            auto const [b, entries] = generation_of<index>(t.dir, v);
            INFO("generation " << v << " holds " << entries);
            CHECK(b == buckets);
            CHECK(entries <= growth);
        }
        // Two generations of 500 cannot become one, so some had to survive.
        CHECK(surviving >= 2);
        CHECK(surviving <= files_before);

        for (auto const& e : fs::directory_iterator(t.dir)) {
            INFO(e.path().filename().string());
            CHECK(e.path().extension() != ".building");
        }
    }
}

TEST_CASE("with room to grow, it is the guard that refuses and not the allocator",
          "[capacity]") {
    // The case the other compaction tests cannot reach. At the ten-megabyte
    // profile a container cannot grow at all — 959 buckets to 1919 needs 18.8 MiB
    // — so `bad_alloc` refuses first and a guard that had been deleted would look
    // exactly like one that had not.
    //
    // A small capacity in the same file leaves room for the step above, so Boost
    // *can* grow here. What refuses is then the policy, and removing it is
    // visible.
    constexpr size_t index = 0;
    constexpr size_t small = 15359;                     // step 10
    auto const growth = max_size_without_rehash(small); // 13439
    std::vector<uint8_t> const value(utxoz::container_capacities[index], 0x11);

    failpoints::scoped_reset const disarm;
    failpoints::forced_capacity.store(small, std::memory_order_relaxed);
    failpoints::forced_capacity_index.store(index, std::memory_order_relaxed);

    // The control, first and in this same segment: Boost really does grow here.
    // Without it the rest would pass against a map that could never have grown,
    // which is the mistake this whole case exists to avoid.
    {
        temp_db probe;
        {
            auto opened = utxoz::full_db::open_for_testing(probe.dir, true);
            REQUIRE(opened.has_value());
            auto db = std::move(*opened);
            db.close();
        }
        auto const file = probe.dir / fmt::format(data_file_format, index, 0);
        REQUIRE(fs::exists(file));
        auto seg = open_existing_segment(file);
        REQUIRE(seg.has_value());
        auto found = find_single_named<utxo_map<utxoz::container_sizes[index]>>(
            **seg, map_object_name, file);
        REQUIRE(found.has_value());
        REQUIRE((**found).bucket_count() == small);

        utxo_value<utxoz::container_sizes[index]> v{};
        v.set_data(value);
        for (uint64_t i = 0; i <= growth; ++i) {
            (**found).emplace(key_of(i), v);
        }
        INFO("the segment must have room for the step above, or this proves nothing");
        REQUIRE((**found).bucket_count() == next_bucket_step(small));
    }

    auto const before = rehashes_observed.load(std::memory_order_relaxed);
    temp_db t;
    uint64_t n = 0;

    // Two generations that together come to exactly the limit, and a third entry
    // beyond it in a third generation.
    {
        auto opened = utxoz::full_db::open_for_testing(t.dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        for (int g = 0; g < 3; ++g) {
            for (uint64_t i = 0; i < growth / 2 + 1; ++i) {
                REQUIRE(db.insert(key_of(++n), value, 800000).has_value());
            }
            failpoints::force_rotations.store(1, std::memory_order_relaxed);
            REQUIRE(db.insert(key_of(++n), value, 800000).has_value());
        }
        db.close();
    }
    auto const files_before = generations(t.dir, index);
    REQUIRE(files_before >= 3);

    auto opened = utxoz::full_db::open_for_testing(t.dir, false);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);
    auto const size_before = db.size();

    auto const outcome = db.compact_all();
    // Required, not allowed either way: a group that does not fit is refused inside
    // the walk, which retries with fewer sources and reports that it finished. The
    // refusal is read in the files below, and the measured return value is success.
    REQUIRE(outcome.has_value());
    CHECK(db.size() == size_before);
    db.close();

    // Nothing grew — and here that is the guard's doing, because the room was
    // there.
    //
    // Zero, not "few": the assertion is equality with the count before, so any
    // growth at all fails it. Removing the guard produces several rather than one,
    // because `compact_all` tries the largest group and shrinks on refusal, and
    // without the guard each attempt grows its own target before the per-entry
    // detector stops it. The number depends on how many groups get tried and is
    // not part of the scenario; what is asserted is that there are none.
    CHECK(rehashes_observed.load(std::memory_order_relaxed) == before);
    for (size_t v = 0; v < max_versions_scanned; ++v) {
        if ( ! fs::exists(t.dir / fmt::format(data_file_format, index, v))) continue;
        INFO("generation " << v);
        auto const [b, entries] = generation_of<index>(t.dir, v);
        CHECK(b == small);
        CHECK(entries <= growth);
    }

    // Sources intact, nothing half-built, and a smaller group is still available
    // to try.
    CHECK(generations(t.dir, index) >= 2);
    for (auto const& e : fs::directory_iterator(t.dir)) {
        INFO(e.path().filename().string());
        CHECK(e.path().extension() != ".building");
    }
}

TEST_CASE("at the limit, a duplicate is a duplicate and a new key is not",
          "[capacity]") {
    // The target has to be *at* the line for this to mean anything, and the key
    // that arrives there has to be the one under test — so the last source holds
    // exactly one entry. Iteration order inside a hash map is arbitrary; the size
    // of the source is not.
    //
    // A generation rotates at 133/160 and a target fills to 7/8, so one source can
    // never fill a target on its own. Two do, and a third of one entry decides
    // what is reported.
    constexpr size_t index = 0;
    constexpr size_t small = 15359;
    auto const limit = max_size_without_rehash(small);        // 13439
    auto const per_generation = max_entries_for(small);        // 12767
    REQUIRE(limit > per_generation);
    auto const remainder = limit - per_generation;             // 672
    std::vector<uint8_t> const value(utxoz::container_capacities[index], 0x33);

    // `last` is the single entry of the third source: either a key the first
    // source already holds, or one nothing holds.
    auto const build = [&](fs::path const& dir, uint64_t last) {
        failpoints::scoped_reset const disarm;
        failpoints::forced_capacity.store(small, std::memory_order_relaxed);
        failpoints::forced_capacity_index.store(index, std::memory_order_relaxed);

        auto opened = utxoz::full_db::open_for_testing(dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        uint64_t n = 0;
        for (uint64_t i = 0; i < per_generation; ++i) {         // v0, exactly full
            REQUIRE(db.insert(key_of(++n), value, 800000).has_value());
        }
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        for (uint64_t i = 0; i < remainder; ++i) {              // v1, the remainder
            REQUIRE(db.insert(key_of(++n), value, 800000).has_value());
        }
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(last), value, 800001).has_value());   // v2, one entry
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(1000000), value, 800000).has_value()); // v3, active
        db.close();
    };

    auto const no_building = [](fs::path const& dir) {
        for (auto const& e : fs::directory_iterator(dir)) {
            if (e.path().extension() == ".building") return false;
        }
        return true;
    };

    auto const check_sources_intact = [&](fs::path const& dir, size_t expected_files) {
        CHECK(generations(dir, index) == expected_files);
        auto const [b0, e0] = generation_of<index>(dir, 0);
        CHECK(b0 == small);
        CHECK(e0 == per_generation);
        auto const [b1, e1] = generation_of<index>(dir, 1);
        CHECK(b1 == small);
        CHECK(e1 == remainder);
        for (auto const& e : fs::directory_iterator(dir)) {
            INFO(e.path().filename().string());
            CHECK(e.path().extension() != ".building");
        }
    };

    SECTION("a key another source already holds is reported as a duplicate") {
        temp_db t;
        auto const before = rehashes_observed.load(std::memory_order_relaxed);
        build(t.dir, 1);                       // key 1 is in v0
        auto const files = generations(t.dir, index);
        REQUIRE(files == 4);

        failpoints::scoped_reset const disarm;
        failpoints::forced_capacity.store(small, std::memory_order_relaxed);
        failpoints::forced_capacity_index.store(index, std::memory_order_relaxed);

        auto opened = utxoz::full_db::open_for_testing(t.dir, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        auto const outcome = db.compact_all();
        db.close();

        // The target was full when that key arrived, and it is still a duplicate.
        // Reporting "no room" here would send an operator to look for a sizing
        // problem in a database that is inconsistent.
        REQUIRE_FALSE(outcome.has_value());
        CHECK(outcome.error() == utxoz::error_code::duplicate_key);
        CHECK(rehashes_observed.load(std::memory_order_relaxed) == before);
        check_sources_intact(t.dir, files);
    }

    SECTION("a key nothing holds is reported as no room") {
        temp_db t;
        auto const before = rehashes_observed.load(std::memory_order_relaxed);
        build(t.dir, 900000);                  // a key no source holds
        auto const files = generations(t.dir, index);
        REQUIRE(files == 4);

        failpoints::scoped_reset const disarm;
        failpoints::forced_capacity.store(small, std::memory_order_relaxed);
        failpoints::forced_capacity_index.store(index, std::memory_order_relaxed);

        auto opened = utxoz::full_db::open_for_testing(t.dir, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        auto const outcome = db.compact_all();
        db.close();

        // Same target, same fullness, different key: a sizing answer rather than an
        // inconsistency, and the caller may retry with fewer sources — which is
        // what happens here. The group of three is refused and the group of two
        // fits exactly at the limit, so those sources are legitimately consumed
        // and the file count goes down. Asserting they survive would be asserting
        // that the retry does not work.
        CHECK(rehashes_observed.load(std::memory_order_relaxed) == before);
        // Required, not allowed either way: a group that does not fit is refused
        // inside the walk, which retries with fewer sources and reports that it
        // finished. `insufficient_space` never reaches the caller from here — the
        // refusal is read in the files, and the measured return value is success.
        // `duplicate_key` is the one refusal that does come back, and it has its
        // own case.
        REQUIRE(outcome.has_value());
        CHECK(generations(t.dir, index) < files);
        CHECK(no_building(t.dir));

        // And whatever was merged sits at or below the line, never past it.
        for (size_t v = 0; v < max_versions_scanned; ++v) {
            if ( ! fs::exists(t.dir / fmt::format(data_file_format, index, v))) continue;
            INFO("generation " << v);
            auto const [b, entries] = generation_of<index>(t.dir, v);
            CHECK(b == small);
            CHECK(entries <= limit);
        }

        // Nothing was lost on the way.
        failpoints::forced_capacity.store(small, std::memory_order_relaxed);
        failpoints::forced_capacity_index.store(index, std::memory_order_relaxed);
        auto reopened = utxoz::full_db::open_for_testing(t.dir, false);
        REQUIRE(reopened.has_value());
        CHECK(reopened->size() == per_generation + remainder + 2);
        reopened->close();
    }
}

TEST_CASE("reference compaction is held to it too", "[capacity]") {
    // Reference is the case that could actually have grown: its production file
    // is eleven times its map. Driven here at the small profile, through the same
    // guard.
    temp_db t;
    auto const before = rehashes_observed.load(std::memory_order_relaxed);
    auto const limit = max_entries_for(testing_reference.bucket_count);

    uint64_t n = 0;
    {
        auto opened = utxoz::reference_db::open_for_testing(t.dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        for (int generation = 0; generation < 2; ++generation) {
            for (uint64_t i = 0; i < limit; ++i) {
                REQUIRE(db.insert(key_of(++n), uint32_t(i % 1000), uint32_t(i), 800000)
                            .has_value());
            }
        }
        REQUIRE(db.insert(key_of(++n), 1, 1, 800000).has_value());
        db.close();
    }

    auto opened = utxoz::reference_db::open_for_testing(t.dir, false);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);
    auto const total_before = db.size();

    auto const outcome = db.compact_all();
    // Required, not allowed either way: see the full-mode cases above. Reference
    // mode gets its own pass rather than an argument by analogy.
    REQUIRE(outcome.has_value());
    CHECK(db.size() == total_before);
    db.close();

    CHECK(rehashes_observed.load(std::memory_order_relaxed) == before);
    for (auto const& e : fs::directory_iterator(t.dir)) {
        INFO(e.path().filename().string());
        CHECK(e.path().extension() != ".building");
    }
}

TEST_CASE("a compacted target may sit above the operating threshold, and the next "
          "insert rotates instead of touching it", "[capacity]") {
    // The case the two thresholds create, built on purpose rather than hoped for.
    // A sealed target may be packed past the reserve a live container keeps; the
    // moment it becomes the active map an insert must make a new generation
    // instead of pushing it towards the growth point.
    constexpr size_t index = 0;
    constexpr size_t small = 15359;
    auto const operating = max_entries_for(small);          // 12767
    auto const growth = max_size_without_rehash(small);     // 13439
    REQUIRE(operating < growth);
    std::vector<uint8_t> const value(utxoz::container_capacities[index], 0x44);

    failpoints::scoped_reset const disarm;
    failpoints::forced_capacity.store(small, std::memory_order_relaxed);
    failpoints::forced_capacity_index.store(index, std::memory_order_relaxed);

    temp_db t;
    auto const before = rehashes_observed.load(std::memory_order_relaxed);
    uint64_t n = 0;

    // The packed generation has to land *strictly between* the two thresholds,
    // and that is the whole difficulty of building this state. A target packed to
    // exactly `growth` is refused by both numbers alike: the case would stay green
    // with the operating threshold replaced by the growth point, and would be
    // proving that something refuses the insert rather than that the reserve does.
    // Half the reserve, so the merged target sits inside it with room on each side.
    auto const surplus = (growth - operating) / 2;
    REQUIRE(surplus > 0);

    // Two sources that come to the operating threshold plus that surplus, and an
    // active generation behind them.
    {
        auto opened = utxoz::full_db::open_for_testing(t.dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        for (uint64_t i = 0; i < operating; ++i) {
            REQUIRE(db.insert(key_of(++n), value, 800000).has_value());
        }
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        for (uint64_t i = 0; i < surplus; ++i) {
            REQUIRE(db.insert(key_of(++n), value, 800000).has_value());
        }
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(++n), value, 800000).has_value());
        db.close();
    }
    auto const inserted = n;

    {
        auto opened = utxoz::full_db::open_for_testing(t.dir, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        REQUIRE(db.compact_all().has_value());
        CHECK(db.size() == inserted);
        db.close();
    }

    // The state under test, required rather than looked for: one generation
    // strictly between the two thresholds.
    size_t packed_version = 0;
    size_t packed_entries = 0;
    bool found_packed = false;
    for (size_t v = 0; v < max_versions_scanned; ++v) {
        if ( ! fs::exists(t.dir / fmt::format(data_file_format, index, v))) continue;
        auto const [b, entries] = generation_of<index>(t.dir, v);
        CHECK(b == small);
        CHECK(entries <= growth);
        if (entries > operating) {
            found_packed = true;
            packed_version = v;
            packed_entries = entries;
        }
    }
    REQUIRE(found_packed);
    REQUIRE(packed_entries > operating);
    // Strictly below, for the reason given above: at the growth point the two
    // thresholds agree and what follows would not tell them apart.
    REQUIRE(packed_entries < growth);

    // One more entry. The packed generation is already past the threshold, so it
    // must not be touched: a new generation takes the entry.
    size_t const files_before = generations(t.dir, index);
    {
        auto opened = utxoz::full_db::open_for_testing(t.dir, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        REQUIRE(db.insert(key_of(++n), value, 800000).has_value());
        CHECK(db.size() == inserted + 1);
        db.close();
    }

    CHECK(generations(t.dir, index) > files_before);

    // The packed generation is exactly as it was: same bucket count, same
    // entries, untouched by the insert that arrived after it.
    {
        auto const [b, entries] = generation_of<index>(t.dir, packed_version);
        CHECK(b == small);
        CHECK(entries == packed_entries);
    }

    // And the new key is reachable, from wherever it went.
    {
        auto opened = utxoz::full_db::open_for_testing(t.dir, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        auto const found = db.find(key_of(n), 800001);
        REQUIRE(found.has_value());
        CHECK(std::ranges::equal(found->data, value));
        db.close();
    }

    CHECK(rehashes_observed.load(std::memory_order_relaxed) == before);
}

// =============================================================================
// The counter counts a growth once, not once per insert after it
// =============================================================================

TEST_CASE("a growth is reported once, and a second one again", "[capacity]") {
    // `detect_rehash` rather than `note_rehash_if_grown`: the two are separate so
    // that the counting can be tested in a build whose assertion would otherwise
    // stop the program. Detecting and stopping are different decisions and only
    // the first is observable from here.
    auto const before = rehashes_observed.load(std::memory_order_relaxed);
    rehash_watch watch;
    watch.reset(959);

    // No transition, however many times it is asked.
    CHECK_FALSE(detect_rehash(0, watch, 959));
    CHECK_FALSE(detect_rehash(0, watch, 959));
    CHECK(rehashes_observed.load(std::memory_order_relaxed) == before);

    // The first growth is a transition, and counts once.
    CHECK(detect_rehash(0, watch, 1919));
    CHECK(rehashes_observed.load(std::memory_order_relaxed) == before + 1);

    // Every insert afterwards sees the same count. Nothing changed, so nothing is
    // reported — comparing against the opening value instead would count the same
    // defect again on every one of them, turning it into a figure that measures
    // how much was written afterwards.
    for (int i = 0; i < 100; ++i) CHECK_FALSE(detect_rehash(0, watch, 1919));
    CHECK(rehashes_observed.load(std::memory_order_relaxed) == before + 1);

    // A second, different growth is a second violation.
    CHECK(detect_rehash(0, watch, 3839));
    CHECK(rehashes_observed.load(std::memory_order_relaxed) == before + 2);
    for (int i = 0; i < 10; ++i) CHECK_FALSE(detect_rehash(0, watch, 3839));
    CHECK(rehashes_observed.load(std::memory_order_relaxed) == before + 2);

    // A generation opened afresh starts clean.
    watch.reset(3839);
    CHECK_FALSE(detect_rehash(0, watch, 3839));
    CHECK(rehashes_observed.load(std::memory_order_relaxed) == before + 2);
}

TEST_CASE("where Boost grows, found by growing it", "[capacity]") {
    // The formula is `7 * buckets / 8`. That is a claim about Boost, so it is
    // checked against Boost: real maps, filled one entry at a time, and the size
    // at which the bucket count moves.
    //
    // Three points around the line, and the third is the control: without it the
    // other two would pass against a map that never grows at all.
    for (unsigned k = 5; k <= 14; ++k) {
        auto const buckets = bucket_step(k);
        auto const limit = max_size_without_rehash(buckets);
        INFO("step " << k << ", " << buckets << " buckets, limit " << limit);

        boost::unordered_flat_map<uint64_t, uint64_t> m(buckets);
        REQUIRE(m.bucket_count() == buckets);

        // Up to one below the limit: nothing moves.
        for (uint64_t i = 0; i < limit - 1; ++i) m.emplace(i, i);
        CHECK(m.size() == limit - 1);
        CHECK(m.bucket_count() == buckets);

        // At the limit: still nothing.
        m.emplace(limit - 1, limit - 1);
        CHECK(m.size() == limit);
        CHECK(m.bucket_count() == buckets);

        // One past it: this is the entry that grows the map. If it did not, the
        // two checks above would be measuring a boundary that is somewhere else.
        m.emplace(limit, limit);
        CHECK(m.size() == limit + 1);
        CHECK(m.bucket_count() == next_bucket_step(buckets));
    }
}

TEST_CASE("the operating threshold leaves room below the growth point",
          "[capacity]") {
    // Two different numbers for two different questions: a live container keeps
    // five per cent of reserve because it is still receiving inserts; a sealed
    // compaction target does not, because it is not.
    for (unsigned k = 5; k <= 25; ++k) {
        auto const buckets = bucket_step(k);
        INFO("step " << k);
        CHECK(max_entries_for(buckets) < max_size_without_rehash(buckets));
    }
    CHECK(max_entries_for(122879) == 102143);
    CHECK(max_size_without_rehash(122879) == 107519);
    CHECK(max_entries_for(15728639) == 13074431);
    CHECK(max_size_without_rehash(15728639) == 13762559);
}

// =============================================================================
// C. The invariant, whatever the build
// =============================================================================

TEST_CASE("no generation's bucket count moves, in either configuration", "[capacity]") {
    // The counter is compiled in every build, statistics or not, because a rehash
    // is not a statistic: it is the one thing the rotation exists to prevent, and
    // an operator running without statistics needs to know it did not happen.
    auto const before = rehashes_observed.load(std::memory_order_relaxed);

    temp_db t;
    auto opened = utxoz::full_db::open_for_testing(t.dir, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    // Every class, across a rotation, with the sizes that reach each container.
    uint64_t n = 0;
    for (int round = 0; round < 3; ++round) {
        for (size_t index = 0; index < utxoz::container_sizes.size(); ++index) {
            std::vector<uint8_t> const value(utxoz::container_capacities[index], uint8_t(index));
            for (int i = 0; i < 300; ++i) {
                REQUIRE(db.insert(key_of(++n), value, 800000).has_value());
            }
        }
    }
    db.close();

    CHECK(rehashes_observed.load(std::memory_order_relaxed) == before);
}
