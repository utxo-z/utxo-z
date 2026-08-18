// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_lookup_telemetry.cpp
 * @brief That the read-path counters describe the read path.
 *
 * Every case here asserts an **exact** number. A counter that is merely non-zero
 * proves that something incremented it, which is the one thing nobody doubted;
 * what has to be shown is that it incremented the right number of times, for the
 * right class, and not twice.
 *
 * Three distinctions are checked separately because they were one number before
 * and the whole point of this work is that they are three:
 *
 *  - **maps probed** — how often a class was asked. `find()` stops at the first
 *    answer, so class 4 is asked only when the four before it said no.
 *  - **probe ordinal** — how many generation files a key was searched in before
 *    it was answered. The cost.
 *  - **version distance** — how far back the answering generation sits from its
 *    class's active version. The age. Compaction leaves gaps in the numbering
 *    and the cache is searched first, so this is not the ordinal.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
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
#include <utxoz/statistics.hpp>
#include <utxoz/types.hpp>

#include "detail/durability.hpp"

namespace fs = std::filesystem;
using namespace utxoz;
using utxoz::detail::failpoints;

namespace {

struct temp_db {
    temp_db() {
        static std::atomic<uint64_t> counter{0};
        auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        dir = fmt::format("./test_lookup_tel_{}_{}_{}", getpid(), ts, counter.fetch_add(1));
        fs::remove_all(dir);
    }
    ~temp_db() { std::error_code ec; fs::remove_all(dir, ec); }
    temp_db(temp_db const&) = delete;
    temp_db& operator=(temp_db const&) = delete;
    fs::path dir;
};

raw_outpoint key_of(uint64_t n) {
    raw_outpoint k{};
    std::memcpy(k.data(), &n, sizeof(n));
    return k;
}

/// A payload that lands in class `index`.
std::vector<uint8_t> payload_for(size_t index, uint8_t fill = 0x5A) {
    return std::vector<uint8_t>(index == 0 ? 8 : container_capacities[index - 1] + 1, fill);
}

/// By value, deliberately. Returning a reference invites
/// `class_of(db.get_statistics().lookups, 0)`, where the statistics are a
/// temporary and lifetime extension does not reach through a function call — a
/// dangling reference that reads correctly in isolation and wrongly once another
/// test has run. A copy of a handful of counters costs nothing and cannot do
/// that.
class_lookup_summary class_of(lookup_telemetry const& t, size_t index) {
    auto const it = std::ranges::find_if(t.classes, [&](auto const& c) {
        return c.container_class == index;
    });
    REQUIRE(it != t.classes.end());
    return *it;
}

/// Only the `lookup` level compiles these counters, so the exact numbers below
/// are claims about that build alone. Those cases SKIP rather than return: a
/// case that returns early is reported as passed, and a suite saying a hundred
/// passed when half asserted nothing is a suite lying about its own coverage.
/// The last two cases run at every level and are what pin the others.
#if UTXOZ_STATISTICS_LEVEL >= 2
constexpr bool counting = true;
#else
constexpr bool counting = false;
#endif

} // namespace

// =============================================================================
// The active map
// =============================================================================

TEST_CASE("a hit in the active map is counted once, in the class that answered",
          "[telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;

    auto db = std::move(*full_db::open_for_testing(t.dir, true));
    // One entry in class 0 and one in class 2, so that "the class that answered"
    // is a choice and not the only option.
    REQUIRE(db.insert(key_of(1), payload_for(0), 800000).has_value());
    REQUIRE(db.insert(key_of(2), payload_for(2), 800000).has_value());
    db.reset_search_stats();

    REQUIRE(db.find(key_of(1), 800001).has_value());

    auto const stats = db.get_statistics();
    auto const& lookups = stats.lookups;
    CHECK(lookups.lookups_received == 1);
    CHECK(lookups.deferred == 0);
    CHECK(lookups.absent == 0);

    // Class 0 answered, so it was asked and nothing after it was.
    CHECK(class_of(lookups, 0).active_maps_probed == 1);
    CHECK(class_of(lookups, 0).answered_from_active == 1);
    for (size_t i = 1; i < container_sizes.size(); ++i) {
        INFO("class " << i);
        CHECK(class_of(lookups, i).active_maps_probed == 0);
        CHECK(class_of(lookups, i).answered_from_active == 0);
    }
    // Nothing historical happened at all.
    CHECK(lookups.resolved_historical() == 0);
    CHECK(lookups.generations_probed() == 0);
    CHECK(lookups.files_opened() == 0);
    db.close();
}

TEST_CASE("a hit in a later class shows the classes before it being asked",
          "[telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;

    auto db = std::move(*full_db::open_for_testing(t.dir, true));
    REQUIRE(db.insert(key_of(7), payload_for(3), 800000).has_value());
    db.reset_search_stats();

    REQUIRE(db.find(key_of(7), 800001).has_value());

    auto const stats = db.get_statistics();
    auto const& lookups = stats.lookups;
    CHECK(lookups.lookups_received == 1);

    // Asked 0, 1, 2, 3 — and stopped. Class 4 was never reached, which is the
    // measurement: it says the search is ordered and short-circuits.
    for (size_t i = 0; i <= 3; ++i) {
        INFO("class " << i);
        CHECK(class_of(lookups, i).active_maps_probed == 1);
    }
    CHECK(class_of(lookups, 4).active_maps_probed == 0);

    CHECK(class_of(lookups, 3).answered_from_active == 1);
    CHECK(lookups.answered_from_active() == 1);
    db.close();
}

TEST_CASE("a miss asks every class and is counted once as deferred", "[telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;

    auto db = std::move(*full_db::open_for_testing(t.dir, true));
    REQUIRE(db.insert(key_of(1), payload_for(0), 800000).has_value());
    db.reset_search_stats();

    auto const missing = db.find(key_of(999), 800001);
    REQUIRE_FALSE(missing.has_value());

    auto const stats = db.get_statistics();
    auto const& lookups = stats.lookups;
    CHECK(lookups.lookups_received == 1);
    CHECK(lookups.deferred == 1);
    CHECK(lookups.answered_from_active() == 0);
    for (size_t i = 0; i < container_sizes.size(); ++i) {
        INFO("class " << i);
        CHECK(class_of(lookups, i).active_maps_probed == 1);
    }
    db.close();
}

// =============================================================================
// The historical sweep
// =============================================================================

namespace {

/// A database whose class 0 has `generations` generations, one entry in each,
/// keyed 1..generations with the oldest first. Returns the keys in the order
/// they were written, so keys.front() is in the oldest generation.
std::vector<raw_outpoint> layered(fs::path const& dir, size_t generations) {
    auto db = std::move(*full_db::open_for_testing(dir, true));
    std::vector<raw_outpoint> keys;
    for (size_t i = 0; i < generations; ++i) {
        auto const key = key_of(i + 1);
        REQUIRE(db.insert(key, payload_for(0), 800000).has_value());
        keys.push_back(key);
        if (i + 1 < generations) {
            failpoints::force_rotations.store(1, std::memory_order_relaxed);
            // The insert that triggers the rotation goes to the new generation,
            // so it is a key of its own and not one of the ones being placed.
            REQUIRE(db.insert(key_of(1000 + i), payload_for(0), 800000).has_value());
        }
    }
    db.close();
    return keys;
}

} // namespace

TEST_CASE("a key in the newest historical generation is found at ordinal one",
          "[telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const keys = layered(t.dir, 3);   // generations 0, 1, 2; active is 2

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    db.reset_search_stats();

    // keys[1] went into generation 1, which is the newest historical one.
    std::vector<lookup_request> const request{{keys[1], 800001}};
    auto const resolved = db.resolve(request);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->found.size() == 1);

    auto const stats = db.get_statistics();
    auto const& zero = class_of(stats.lookups, 0);
    CHECK(zero.resolved_historical == 1);
    // One file searched: the newest historical generation, which is where it was.
    CHECK(zero.generations_probed == 1);
    CHECK(zero.files_opened == 1);
    CHECK(zero.probe_ordinal_histogram[0] == 1);     // bucket "1"
    CHECK(zero.avg_probe_ordinal == 1.0);
    // And one version back from the active generation.
    CHECK(zero.version_distance_histogram[0] == 1);
    CHECK(zero.avg_version_distance == 1.0);
    db.close();
}

TEST_CASE("a key in a deeper generation counts every file it was searched in",
          "[telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const keys = layered(t.dir, 3);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    db.reset_search_stats();

    // keys[0] is in generation 0, the oldest. The sweep searches generation 1
    // first and generation 0 second.
    std::vector<lookup_request> const request{{keys[0], 800001}};
    auto const resolved = db.resolve(request);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->found.size() == 1);

    auto const stats = db.get_statistics();
    auto const& zero = class_of(stats.lookups, 0);
    CHECK(zero.resolved_historical == 1);
    CHECK(zero.generations_probed == 2);             // searched in two files
    CHECK(zero.files_opened == 2);
    CHECK(zero.probe_ordinal_histogram[1] == 1);     // bucket "2": the second file
    CHECK(zero.avg_probe_ordinal == 2.0);
    CHECK(zero.version_distance_histogram[1] == 1);  // two versions back
    CHECK(zero.avg_version_distance == 2.0);
    db.close();
}

TEST_CASE("an absent key counts every generation it was actually searched in",
          "[telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;
    layered(t.dir, 3);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    db.reset_search_stats();

    std::vector<lookup_request> const request{{key_of(999999), 800001}};
    auto const resolved = db.resolve(request);
    REQUIRE(resolved.has_value());
    CHECK(resolved->found.empty());
    REQUIRE(resolved->absent.size() == 1);

    auto const stats = db.get_statistics();
    auto const& lookups = stats.lookups;
    // One absent lookup, however many files it took to establish that.
    CHECK(lookups.absent == 1);
    CHECK(lookups.resolved_historical() == 0);
    // Two historical generations of class 0, both searched.
    CHECK(class_of(lookups, 0).generations_probed == 2);
    CHECK(class_of(lookups, 0).files_opened == 2);
    // And no ordinal was recorded, because nothing was answered.
    for (auto const count : class_of(lookups, 0).probe_ordinal_histogram) CHECK(count == 0);
    db.close();
}

TEST_CASE("a sweep of several keys counts a probe per key per file", "[telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    // The case that tells "generations probed" apart from "files opened". With
    // one key in a batch the two are the same number, which is why every earlier
    // case here could not see the difference.
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const keys = layered(t.dir, 3);   // historical generations 0 and 1

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    db.reset_search_stats();

    // Three keys: one in the newer historical generation, one in the older, one
    // nowhere at all.
    std::vector<lookup_request> const request{
        {keys[1], 800001}, {keys[0], 800001}, {key_of(777777), 800001}};
    auto const resolved = db.resolve(request);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->found.size() == 2);
    REQUIRE(resolved->absent.size() == 1);

    auto const zero = class_of(db.get_statistics().lookups, 0);
    CHECK(zero.files_opened == 2);
    // Three keys against the first file, then two against the second: the one
    // found in the first stopped being searched for, and the absent one was
    // searched for in both. Five, not two.
    CHECK(zero.generations_probed == 5);
    CHECK(zero.resolved_historical == 2);
    CHECK(zero.probe_ordinal_histogram[0] == 1);   // found in the first file searched
    CHECK(zero.probe_ordinal_histogram[1] == 1);   // and in the second
    CHECK(zero.avg_probe_ordinal == 1.5);
    db.close();
}

TEST_CASE("two classes are counted apart", "[telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;

    // One historical generation in class 0 and one in class 2, each holding one
    // key, so an attribution mistake shows as a number in the wrong row.
    std::vector<raw_outpoint> keys;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(key_of(1), payload_for(0), 800000).has_value());
        REQUIRE(db.insert(key_of(2), payload_for(2), 800000).has_value());
        keys = {key_of(1), key_of(2)};
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(50), payload_for(0), 800000).has_value());
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(51), payload_for(2), 800000).has_value());
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    db.reset_search_stats();

    std::vector<lookup_request> const request{{keys[0], 800001}, {keys[1], 800001}};
    auto const resolved = db.resolve(request);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->found.size() == 2);

    auto const stats_snapshot = db.get_statistics();
    auto const& lookups = stats_snapshot.lookups;
    CHECK(class_of(lookups, 0).resolved_historical == 1);
    CHECK(class_of(lookups, 2).resolved_historical == 1);
    CHECK(class_of(lookups, 1).resolved_historical == 0);
    CHECK(class_of(lookups, 3).resolved_historical == 0);
    CHECK(class_of(lookups, 4).resolved_historical == 0);
    CHECK(lookups.resolved_historical() == 2);
    db.close();
}

// =============================================================================
// Repetition, and the totals
// =============================================================================

TEST_CASE("repeating a lookup moves the counters exactly once each time",
          "[telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;

    auto db = std::move(*full_db::open_for_testing(t.dir, true));
    REQUIRE(db.insert(key_of(1), payload_for(0), 800000).has_value());
    db.reset_search_stats();

    for (int i = 0; i < 7; ++i) REQUIRE(db.find(key_of(1), 800001).has_value());

    auto const stats_snapshot = db.get_statistics();
    auto const& lookups = stats_snapshot.lookups;
    CHECK(lookups.lookups_received == 7);
    CHECK(class_of(lookups, 0).active_maps_probed == 7);
    CHECK(class_of(lookups, 0).answered_from_active == 7);
    CHECK(lookups.answered_from_active() == 7);
    CHECK(lookups.deferred == 0);
    db.close();
}

TEST_CASE("the derived totals are the sum of the classes, always", "[telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const keys = layered(t.dir, 3);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    db.reset_search_stats();
    REQUIRE(db.find(key_of(1000), 800001).has_value() == false);   // a miss, then a sweep
    std::vector<lookup_request> const request{{keys[0], 800001}, {key_of(424242), 800001}};
    REQUIRE(db.resolve(request).has_value());

    auto const stats_snapshot = db.get_statistics();
    auto const& lookups = stats_snapshot.lookups;
    size_t answered = 0, historical = 0, probes = 0, files = 0, hits = 0;
    for (auto const& c : lookups.classes) {
        answered += c.answered_from_active;
        historical += c.resolved_historical;
        probes += c.generations_probed;
        files += c.files_opened;
        hits += c.cache_hits;
    }
    CHECK(answered == lookups.answered_from_active());
    CHECK(historical == lookups.resolved_historical());
    CHECK(probes == lookups.generations_probed());
    CHECK(files == lookups.files_opened());
    CHECK(hits == lookups.cache_hits());

    // And the one relationship that has to hold between the two structures: what
    // the per-class counters say was answered from an active map is what the
    // global probe summary says.
    auto const stats = db.get_statistics();
    CHECK(stats.probes.answered_from_active == lookups.answered_from_active());
    db.close();
}

// =============================================================================
// The cache
// =============================================================================

TEST_CASE("a warm sweep reports cache hits and a cold one does not", "[telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const keys = layered(t.dir, 3);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    db.reset_search_stats();

    std::vector<lookup_request> const request{{keys[0], 800001}};
    REQUIRE(db.resolve(request).has_value());

    auto const cold = class_of(db.get_statistics().lookups, 0);
    // Nothing was cached before the first sweep, so both files were opened and
    // neither was a hit.
    CHECK(cold.files_opened == 2);
    CHECK(cold.cache_hits == 0);

    REQUIRE(db.resolve(request).has_value());
    auto const warm = class_of(db.get_statistics().lookups, 0);

    // The second sweep opened **one** file, not two, and that one was a hit.
    //
    // That is the LRU showing through, and it is worth stating rather than
    // asserting around: the sweep consults cached files before it walks the
    // catalogue, so once the answering generation is cached it is reached first
    // and the other file is never opened. The counters describe what the search
    // did, not what a fixed order would have done — which is the behaviour this
    // measurement exists to reveal, and is the same LRU dependence recorded in
    // issue #136 for a different consequence.
    CHECK(warm.files_opened - cold.files_opened == 1);
    CHECK(warm.cache_hits - cold.cache_hits == 1);
    CHECK(warm.cache_hits <= warm.files_opened);

    // And this is where the two numbers come apart, which is the whole reason
    // they are two numbers.
    //
    // The key is in the oldest generation, two versions behind the active one,
    // and that never changes. What changes is the cost: the cold sweep searched
    // two files to reach it, the warm one searched a single cached file. So the
    // ordinals are 2 and then 1, while both distances are 2. A report that
    // carried one number could say the search got cheaper or that the data got
    // younger, and those are different facts.
    CHECK(cold.probe_ordinal_histogram[1] == 1);      // cold: found in the 2nd file
    CHECK(warm.probe_ordinal_histogram[0] == 1);      // warm: found in the 1st
    CHECK(warm.probe_ordinal_histogram[1] == 1);      // and the cold one still counted
    CHECK(cold.version_distance_histogram[1] == 1);   // two versions back
    CHECK(warm.version_distance_histogram[1] == 2);   // still two, both times
    CHECK(warm.version_distance_histogram[0] == 0);
    db.close();
}

// =============================================================================
// Reference mode
// =============================================================================

TEST_CASE("reference reports one class, labelled reference", "[telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;

    {
        auto db = std::move(*reference_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(key_of(1), 3, 4, 800000).has_value());
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(2), 5, 6, 800000).has_value());
        db.close();
    }

    auto db = std::move(*reference_db::open_for_testing(t.dir, false));
    db.reset_search_stats();

    // Active hit.
    REQUIRE(db.find(key_of(2), 800001).has_value());
    // Historical: not in the active map, then found by the sweep.
    REQUIRE_FALSE(db.find(key_of(1), 800001).has_value());
    std::vector<lookup_request> const request{{key_of(1), 800001}};
    auto const resolved = db.resolve(request);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->found.size() == 1);

    auto const stats_snapshot = db.get_statistics();
    auto const& lookups = stats_snapshot.lookups;
    REQUIRE(lookups.classes.size() == 1);
    CHECK(lookups.mode == storage_mode::reference);
    CHECK(lookups.classes[0].container_class == reference_class);
    CHECK(lookups.classes[0].active_maps_probed == 2);
    CHECK(lookups.classes[0].answered_from_active == 1);
    CHECK(lookups.classes[0].resolved_historical == 1);
    CHECK(lookups.classes[0].generations_probed == 1);
    CHECK(lookups.lookups_received == 2);
    CHECK(lookups.deferred == 1);
    db.close();
}

// =============================================================================
// Rotation and compaction do not change what the numbers mean
// =============================================================================

TEST_CASE("after compaction the ordinal still counts files and the distance still "
          "counts versions", "[telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    // The two numbers come apart exactly here. Compaction merges generations and
    // removes the drained ones, so the surviving version numbers have gaps: a
    // key found in the first file searched can sit several versions back.
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const keys = layered(t.dir, 4);

    {
        auto db = std::move(*full_db::open_for_testing(t.dir, false));
        REQUIRE(db.compact_all().has_value());
        // Compaction merges into a generation that may well be the active one,
        // and resolve() searches only below the active. One forced rotation puts
        // the merged generation behind the active one, which is the state this
        // case is about.
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(90001), payload_for(0), 800000).has_value());
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    db.reset_search_stats();
    std::vector<lookup_request> const request{{keys[0], 800001}};
    auto const resolved = db.resolve(request);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->found.size() == 1);

    auto const zero = class_of(db.get_statistics().lookups, 0);
    REQUIRE(zero.resolved_historical == 1);
    // Whatever compaction left, the ordinal is the number of files searched and
    // cannot exceed the number opened.
    CHECK(zero.avg_probe_ordinal >= 1.0);
    CHECK(zero.avg_probe_ordinal <= double(zero.files_opened));
    CHECK(zero.generations_probed == zero.files_opened);   // one key, so one probe per file
    // And the histograms account for that one key exactly once each.
    size_t ordinals = 0, distances = 0;
    for (size_t b = 0; b < zero.probe_ordinal_histogram.size(); ++b) {
        ordinals += zero.probe_ordinal_histogram[b];
        distances += zero.version_distance_histogram[b];
    }
    CHECK(ordinals == 1);
    CHECK(distances == 1);
    db.close();
}

// =============================================================================
// The two presentations, and the build without statistics
// =============================================================================

TEST_CASE("the telemetry JSON carries the schema, the mode and the classes",
          "[telemetry]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto db = std::move(*full_db::open_for_testing(t.dir, true));
    REQUIRE(db.insert(key_of(1), payload_for(0), 800000).has_value());
    REQUIRE(db.find(key_of(1), 800001).has_value());

    auto const json = to_json(db.get_statistics().lookups);
    INFO(json);
    // The frame is the same at every level: a consumer parses one document, not
    // three.
    CHECK(json.find("\"schema_version\": 1") != std::string::npos);
    CHECK(json.find("\"storage_mode\": \"full\"") != std::string::npos);
    CHECK(json.find("\"unattributed\"") != std::string::npos);
    CHECK(json.find("\"derived_totals\"") != std::string::npos);

    // The classes are there only where they were collected. At `basic` the list
    // is empty rather than five rows of zeros, which would read as a database
    // nobody queried.
    if constexpr (counting) {
        CHECK(json.find("\"container_class\": 0") != std::string::npos);
        CHECK(json.find("\"probe_ordinal_histogram\"") != std::string::npos);
        CHECK(json.find("\"version_distance_histogram\"") != std::string::npos);
    } else {
        CHECK(json.find("\"container_class\"") == std::string::npos);
    }

    // The flag says which kind of document this is, so a reader cannot mistake a
    // build that counted nothing for a database that did nothing.
    if constexpr (counting) {
        CHECK(json.find("\"statistics_level\": \"lookup\"") != std::string::npos);
    } else {
        // "basic" or "off": either way this build did not collect these, and the
        // document says which rather than leaving a page of zeros unexplained.
        CHECK(json.find("\"statistics_level\": \"lookup\"") == std::string::npos);
        CHECK(json.find("\"statistics_level\"") != std::string::npos);
    }
    db.close();
}

TEST_CASE("without statistics the API is unchanged and every figure is zero",
          "[telemetry]") {
    // The contract for the other configuration, asserted from the same source so
    // that it cannot drift: the types exist, the calls compile, the report is
    // produced, and it says why it is empty.
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto db = std::move(*full_db::open_for_testing(t.dir, true));
    REQUIRE(db.insert(key_of(1), payload_for(0), 800000).has_value());
    REQUIRE(db.find(key_of(1), 800001).has_value());

    auto const stats_snapshot = db.get_statistics();
    auto const& lookups = stats_snapshot.lookups;
    if constexpr (counting) {
        CHECK(lookups.statistics_level == "lookup");
        CHECK(lookups.lookups_received == 1);
        REQUIRE(lookups.classes.size() == container_sizes.size());
    } else {
        // At `basic` the older counters still work — the lookup was received —
        // and the per-class telemetry does not exist. Reporting no classes is the
        // honest answer: five rows of zeros would read as a database nobody
        // queried.
        CHECK(lookups.statistics_level != "lookup");
        CHECK(lookups.classes.empty());
        CHECK(lookups.answered_from_active() == 0);
        CHECK(lookups.generations_probed() == 0);
#if UTXOZ_STATISTICS_LEVEL >= 1
        CHECK(lookups.statistics_level == "basic");
        CHECK(lookups.lookups_received == 1);
#else
        CHECK(lookups.statistics_level == "off");
        CHECK(lookups.lookups_received == 0);
#endif
    }
    db.close();
}
