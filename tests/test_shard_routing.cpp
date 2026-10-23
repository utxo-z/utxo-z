// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_shard_routing.cpp
 * @brief That the historical walks look for a key only where it can be.
 *
 * Inside the sharded class a key can live in one shard only — the one its own
 * hash names — and every other shard's files are work with no possible result.
 * The walks used to search every file of every shard with the whole batch. That
 * was correct in what it returned and wrong in everything else: a miss at N=8
 * read eight histories instead of one, the counters charged every key to every
 * file, and a file nobody needed could fail a lookup that never had to touch it.
 *
 * What stays exactly as it was: a request carries a key and not its size class,
 * so every class remains a candidate. Only the shard is ruled out, and only
 * inside the sharded class.
 *
 * The expected counters come from a small model of the routed walk rather than
 * from literals, so the same case is exact at N=1 — where routing changes
 * nothing — and at every sharded build. Cases that can only mean something with
 * two shards or more say so and skip at N=1.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
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
#include "shard_paths.hpp"

namespace fs = std::filesystem;
using namespace utxoz;
using utxoz::detail::failpoints;
using utxoz::testing::generations_in_shard;
using utxoz::testing::key_in_shard;
using utxoz::testing::class_file;

namespace {

struct temp_db {
    temp_db() {
        static std::atomic<uint64_t> counter{0};
        auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        dir = fmt::format("./test_shard_routing_{}_{}_{}", getpid(), ts, counter.fetch_add(1));
        fs::remove_all(dir);
    }
    ~temp_db() { std::error_code ec; fs::remove_all(dir, ec); }
    temp_db(temp_db const&) = delete;
    temp_db& operator=(temp_db const&) = delete;
    fs::path dir;
};

#if UTXOZ_STATISTICS_LEVEL >= 2
constexpr bool counting = true;
#else
constexpr bool counting = false;
#endif

constexpr uint32_t shards = utxoz::shard_count;
constexpr uint32_t last_shard = shards - 1;
constexpr uint32_t query_height = 900000;

/// A payload that lands in class `index`.
std::vector<uint8_t> payload_for(size_t index, uint8_t fill = 0x5A) {
    return std::vector<uint8_t>(index == 0 ? 8 : container_capacities[index - 1] + 1, fill);
}

/// Every shard of class 0 gets three generations: v0 and v1 historical, v2
/// active. hist[sh][v] is the key stored in version v of shard sh, at height
/// 800000 + 10·sh + v so that an answer carrying another key's entry shows.
///
/// Every shard gets a history, not only the ones a case asks about. The defect
/// was walking the others, and a fixture where the others are empty cannot show
/// it.
struct layered {
    std::vector<std::array<raw_outpoint, 2>> hist;
};

uint32_t height_of(uint32_t shard, size_t version) {
    return uint32_t(800000 + 10 * shard + version);
}

layered build_layered(fs::path const& dir) {
    layered out;
    out.hist.resize(shards);
    auto db = std::move(*full_db::open_for_testing(dir, true));
    for (uint32_t sh = 0; sh != shards; ++sh) {
        for (size_t v = 0; v != 2; ++v) {
            auto const key = key_in_shard(0, sh, 100 + 16 * sh + v);
            REQUIRE(db.insert(key, payload_for(0), height_of(sh, v)).has_value());
            out.hist[sh][v] = key;
            // The insert that triggers the rotation lands in the new version, in
            // the same shard, so it is a filler and not one of the placed keys.
            failpoints::force_rotations.store(1, std::memory_order_relaxed);
            REQUIRE(db.insert(key_in_shard(0, sh, 5000 + 16 * sh + v), payload_for(0),
                              height_of(sh, v)).has_value());
        }
    }
    db.close();
    for (uint32_t sh = 0; sh != shards; ++sh) {
        INFO("shard " << sh);
        REQUIRE(generations_in_shard(dir, 0, sh) == 3);
    }
    return out;
}

class_lookup_summary class_of(lookup_telemetry const& t, size_t index) {
    auto const it = std::ranges::find_if(t.classes, [&](auto const& c) {
        return c.container_class == index;
    });
    REQUIRE(it != t.classes.end());
    return *it;
}

/// What the routed walk should cost, for a fresh instance (nothing cached) over
/// the layered fixture. Each request is (shard, version it lives in), with
/// version -1 for a key stored nowhere. Duplicates are the caller's to remove,
/// as the library removes them.
///
/// The walk visits shards in order and each shard's history newest first. A file
/// is opened only when some pending key could be in it, and charged one probe
/// per such key. That is the whole specification being tested.
struct cost { uint64_t files = 0; uint64_t probes = 0; };

cost routed_cost(std::vector<std::pair<uint32_t, int>> pending) {
    cost c;
    for (uint32_t sh = 0; sh != shards; ++sh) {
        for (int v = 1; v >= 0; --v) {
            uint64_t eligible = 0;
            for (auto const& [s, _] : pending) eligible += (s == sh);
            if (eligible == 0) break;
            ++c.files;
            c.probes += eligible;
            std::erase_if(pending, [&](auto const& p) { return p.first == sh && p.second == v; });
        }
    }
    return c;
}

std::set<raw_outpoint> found_keys(full_resolution const& r) {
    std::set<raw_outpoint> keys;
    for (auto const& [k, _] : r.found) keys.insert(k);
    return keys;
}

std::set<raw_outpoint> absent_keys(full_resolution const& r) {
    std::set<raw_outpoint> keys;
    for (auto const& req : r.absent) keys.insert(req.key);
    return keys;
}

} // namespace

// =============================================================================
// Lookups
// =============================================================================

TEST_CASE("a historical hit and a miss in one shard are searched in that shard only",
          "[routing][shard]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const fx = build_layered(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    db.reset_search_stats();

    auto const hit = fx.hist[last_shard][0];                      // the oldest version
    auto const miss = key_in_shard(0, last_shard, 900000);
    std::vector<lookup_request> const request{{hit, query_height}, {miss, query_height}};
    auto const resolved = db.resolve(request);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->found.size() == 1);
    REQUIRE(resolved->found.contains(hit));
    CHECK(resolved->found.at(hit).block_height == height_of(last_shard, 0));
    CHECK(absent_keys(*resolved) == std::set<raw_outpoint>{miss});

    if constexpr (counting) {
        auto const expected = routed_cost({{last_shard, 0}, {last_shard, -1}});
        auto const zero = class_of(db.get_statistics().lookups, 0);
        CHECK(zero.files_opened == expected.files);           // 2 at every N
        CHECK(zero.generations_probed == expected.probes);    // 4 at every N
        CHECK(zero.resolved_historical == 1);
    }
    db.close();
}

TEST_CASE("keys of several shards in one batch are each searched in their own shard",
          "[routing][shard]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const fx = build_layered(t.dir);

    uint32_t const mid = shards / 2;
    uint32_t const miss_shard = shards > 1 ? 1 : 0;
    auto const miss = key_in_shard(0, miss_shard, 900001);

    // A duplicate, three shards, both versions and a miss, in an order that is
    // not the walk's.
    std::vector<lookup_request> request{
        {fx.hist[last_shard][1], query_height},
        {miss, query_height},
        {fx.hist[0][0], query_height},
        {fx.hist[mid][0], query_height},
        {fx.hist[0][0], query_height},
    };
    std::set<raw_outpoint> const want_found{fx.hist[last_shard][1], fx.hist[0][0], fx.hist[mid][0]};

    // The model wants distinct keys, as the library deduplicates them. At N=1
    // `mid` is shard 0 and its key is the same one as hist[0][0].
    std::map<raw_outpoint, std::pair<uint32_t, int>> distinct{
        {fx.hist[last_shard][1], {last_shard, 1}}, {miss, {miss_shard, -1}},
        {fx.hist[0][0], {0, 0}}, {fx.hist[mid][0], {mid, 0}}};
    std::vector<std::pair<uint32_t, int>> model;
    for (auto const& [_, where] : distinct) model.push_back(where);
    auto const expected = routed_cost(model);

    for (int pass = 0; pass != 2; ++pass) {
        INFO("pass " << pass << (pass ? " (reversed)" : ""));
        if (pass == 1) std::ranges::reverse(request);
        auto db = std::move(*full_db::open_for_testing(t.dir, false));
        db.reset_search_stats();

        auto const resolved = db.resolve(request);
        REQUIRE(resolved.has_value());
        CHECK(found_keys(*resolved) == want_found);
        CHECK(absent_keys(*resolved) == std::set<raw_outpoint>{miss});
        CHECK(resolved->found.at(fx.hist[last_shard][1]).block_height == height_of(last_shard, 1));
        CHECK(resolved->found.at(fx.hist[0][0]).block_height == height_of(0, 0));
        CHECK(resolved->found.at(fx.hist[mid][0]).block_height == height_of(mid, 0));

        if constexpr (counting) {
            auto const zero = class_of(db.get_statistics().lookups, 0);
            CHECK(zero.files_opened == expected.files);
            CHECK(zero.generations_probed == expected.probes);
            CHECK(zero.resolved_historical == want_found.size());
        }
        db.close();
    }
}

TEST_CASE("a key of an unsharded class is still searched in every class, and counted there",
          "[routing][shard]") {
    // The request does not say which class a key is in, so routing must not rule
    // a class out — only a shard inside the sharded one. And the per-class
    // counters have to land on the class that answered, not on whichever slot
    // happens to share its number.
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const fx = build_layered(t.dir);

    // A class-1 entry whose key hashes to the last shard of class 0, rotated into
    // class 1's history.
    auto const wide = key_in_shard(0, last_shard, 700);
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, false));
        REQUIRE(db.insert(wide, payload_for(1), 850000).has_value());
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_in_shard(0, 0, 701), payload_for(1), 850000).has_value());
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    db.reset_search_stats();
    std::vector<lookup_request> const request{{wide, query_height}};
    auto const resolved = db.resolve(request);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->found.contains(wide));
    CHECK(resolved->found.at(wide).block_height == 850000);
    CHECK(resolved->found.at(wide).data.size() == payload_for(1).size());

    if constexpr (counting) {
        auto const lookups = db.get_statistics().lookups;
        auto const zero = class_of(lookups, 0);
        auto const one = class_of(lookups, 1);
        // Searched in its own class-0 shard's two historical files, then class 1.
        CHECK(zero.files_opened == 2);
        CHECK(zero.generations_probed == 2);
        CHECK(zero.resolved_historical == 0);
        CHECK(one.files_opened == 1);
        CHECK(one.generations_probed == 1);
        CHECK(one.resolved_historical == 1);
        for (size_t c = 2; c < container_sizes.size(); ++c) {
            INFO("class " << c);
            CHECK(class_of(lookups, c).resolved_historical == 0);
            CHECK(class_of(lookups, c).files_opened == 0);
        }
    }
    db.close();
}

TEST_CASE("a file already in the cache is still used for a key of its own shard",
          "[routing][shard]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const fx = build_layered(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    std::vector<lookup_request> const request{{fx.hist[0][0], query_height}};
    REQUIRE(db.resolve(request).has_value());   // leaves shard 0's v0 in the cache

    db.reset_search_stats();
    auto const again = db.resolve(request);
    REQUIRE(again.has_value());
    REQUIRE(again->found.contains(fx.hist[0][0]));
    auto const zero = class_of(db.get_statistics().lookups, 0);
    CHECK(zero.files_opened == 1);
    CHECK(zero.cache_hits == 1);
    CHECK(zero.generations_probed == 1);
    db.close();
}

TEST_CASE("a file of another shard in the cache is neither searched nor counted",
          "[routing][shard]") {
    if constexpr (shards == 1) SKIP("one shard: there is no other shard's file to cache");
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const fx = build_layered(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    std::vector<lookup_request> const first{{fx.hist[0][0], query_height}};
    REQUIRE(db.resolve(first).has_value());     // shard 0's v0 is now cached

    db.reset_search_stats();
    std::vector<lookup_request> const second{{fx.hist[last_shard][0], query_height}};
    auto const resolved = db.resolve(second);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->found.contains(fx.hist[last_shard][0]));
    auto const zero = class_of(db.get_statistics().lookups, 0);
    CHECK(zero.files_opened == 2);
    CHECK(zero.cache_hits == 0);
    CHECK(zero.generations_probed == 2);
    db.close();
}

TEST_CASE("a file that will not open fails only the lookups that needed it",
          "[routing][shard][failpoint]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const fx = build_layered(t.dir);

    // Every shard's v0 refuses to open. The failpoint names a version, not a
    // shard, which is exactly what makes the case: before routing, a key living
    // in the last shard's v1 had to get past shard 0's v0 first.
    failpoints::fail_historical_open_version.store(0, std::memory_order_relaxed);

    SECTION("a key answered before any refusing file it could live in is answered") {
        auto db = std::move(*full_db::open_for_testing(t.dir, false));
        std::vector<lookup_request> const request{{fx.hist[last_shard][1], query_height}};
        auto const resolved = db.resolve(request);
        REQUIRE(resolved.has_value());
        CHECK(resolved->found.contains(fx.hist[last_shard][1]));
        db.close();
    }
    SECTION("a key whose own file refuses is not called absent") {
        auto db = std::move(*full_db::open_for_testing(t.dir, false));
        std::vector<lookup_request> const request{{fx.hist[last_shard][0], query_height}};
        auto const resolved = db.resolve(request);
        REQUIRE_FALSE(resolved.has_value());
        CHECK(resolved.error() == error_code::version_unreadable);
        db.close();
    }
    SECTION("a miss still has to read every file of its own shard") {
        auto db = std::move(*full_db::open_for_testing(t.dir, false));
        std::vector<lookup_request> const request{
            {key_in_shard(0, last_shard, 900002), query_height}};
        auto const resolved = db.resolve(request);
        REQUIRE_FALSE(resolved.has_value());
        CHECK(resolved.error() == error_code::version_unreadable);
        db.close();
    }
    SECTION("one key that needed the refusing file fails the whole batch") {
        auto db = std::move(*full_db::open_for_testing(t.dir, false));
        std::vector<lookup_request> const request{
            {fx.hist[last_shard][1], query_height}, {fx.hist[0][0], query_height}};
        auto const resolved = db.resolve(request);
        REQUIRE_FALSE(resolved.has_value());
        CHECK(resolved.error() == error_code::version_unreadable);
        db.close();
    }
}

TEST_CASE("a damaged file of a shard nobody asked for does not fail a lookup",
          "[routing][shard]") {
    if constexpr (shards == 1) SKIP("one shard: every file is a file somebody may need");
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const fx = build_layered(t.dir);

    // Damaged after the open, while nothing holds it: opening validates every
    // version and refuses a store with a damaged one (file_open_failed), which
    // is the open doing its job and not what this case is about. What is: a file
    // that goes bad underneath a running instance, in a shard a lookup never had
    // to read.
    auto opened = full_db::open_for_testing(t.dir, false);
    if ( ! opened) FAIL("open refused: error_code=" << int(opened.error()));
    auto db = std::move(*opened);

    // Shard 0's oldest file, overwritten with zeros at the same size: still in
    // the catalogue, unreadable as a segment.
    auto const damaged = t.dir / class_file(0, 0, 0);
    REQUIRE(fs::exists(damaged));
    {
        auto const size = fs::file_size(damaged);
        std::ofstream out(damaged, std::ios::binary | std::ios::trunc);
        std::vector<char> zeros(size, 0);
        out.write(zeros.data(), std::streamsize(zeros.size()));
    }

    {
        std::vector<lookup_request> const request{{fx.hist[last_shard][0], query_height}};
        auto const resolved = db.resolve(request);
        REQUIRE(resolved.has_value());
        CHECK(resolved->found.contains(fx.hist[last_shard][0]));
    }
    {
        // The shard whose file is damaged still fails closed.
        std::vector<lookup_request> const request{{fx.hist[0][0], query_height}};
        auto const resolved = db.resolve(request);
        REQUIRE_FALSE(resolved.has_value());
        CHECK(resolved.error() == error_code::version_unreadable);
    }
    db.close();
}

// =============================================================================
// The probe ordinal is a key's own cost
// =============================================================================
//
// `probe_ordinal` says how many files a key was searched in before it was
// answered. With routing, files opened for other shards' keys are not searched
// for this one, so they are not part of its cost. The expectations below are
// written per key, by hand, from where each key lives - not from any count of
// files the batch opened.

TEST_CASE("each key's probe ordinal counts only the files it was searched in",
          "[routing][shard][telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const fx = build_layered(t.dir);

    auto const early = fx.hist[0][1];              // shard 0, newest history: 1st file it meets
    auto const late = fx.hist[last_shard][0];      // last shard, oldest: 2nd file of its shard
    auto const miss = key_in_shard(0, shards > 1 ? 1 : 0, 900010);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    db.reset_search_stats();
    std::vector<lookup_request> const request{
        {late, query_height}, {miss, query_height}, {early, query_height}};
    auto const resolved = db.resolve(request);
    REQUIRE(resolved.has_value());
    REQUIRE(found_keys(*resolved) == std::set<raw_outpoint>{early, late});
    REQUIRE(absent_keys(*resolved) == std::set<raw_outpoint>{miss});

    auto const zero = class_of(db.get_statistics().lookups, 0);
    CHECK(zero.resolved_historical == 2);
    // early: answered by the first file it was searched in. late: by the second
    // file of its own shard, whatever the batch opened for other shards first.
    CHECK(zero.probe_ordinal_histogram[0] == 1);   // ordinal 1
    CHECK(zero.probe_ordinal_histogram[1] == 1);   // ordinal 2
    for (size_t b = 2; b < zero.probe_ordinal_histogram.size(); ++b) {
        INFO("ordinal bucket " << b);
        CHECK(zero.probe_ordinal_histogram[b] == 0);
    }
    CHECK(zero.avg_probe_ordinal == 1.5);
    // The age is a separate number and does not move: one and two versions back.
    CHECK(zero.version_distance_histogram[0] == 1);
    CHECK(zero.version_distance_histogram[1] == 1);
    CHECK(zero.avg_version_distance == 1.5);
    // Searches made: early 1, late 2, the miss 2 (both files of its shard). At
    // N=1 the same five, as three keys in v1 and two in v0.
    CHECK(zero.generations_probed == 5);
    db.close();
}

TEST_CASE("a key answered from the cache has ordinal one, and the cache does not "
          "inflate another shard's key",
          "[routing][shard][telemetry]") {
    if constexpr ( ! counting) SKIP("statistics are compiled out");
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const fx = build_layered(t.dir);

    auto const cached = fx.hist[last_shard][0];
    auto const early = fx.hist[0][1];
    auto const miss = key_in_shard(0, shards > 1 ? 1 : 0, 900011);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    std::vector<lookup_request> const prime{{cached, query_height}};
    REQUIRE(db.resolve(prime).has_value());   // the last shard's v0 is now cached

    db.reset_search_stats();
    std::vector<lookup_request> const request{
        {early, query_height}, {cached, query_height}, {miss, query_height}};
    auto const resolved = db.resolve(request);
    REQUIRE(resolved.has_value());
    REQUIRE(found_keys(*resolved) == std::set<raw_outpoint>{early, cached});

    auto const zero = class_of(db.get_statistics().lookups, 0);
    CHECK(zero.resolved_historical == 2);
    CHECK(zero.cache_hits == 1);
    if constexpr (shards > 1) {
        // `cached` is found in the cached file, the first it was searched in.
        // `early` was never searched in that file - another shard's - so its
        // first search is shard 0's v1, where it is.
        CHECK(zero.probe_ordinal_histogram[0] == 2);
        CHECK(zero.probe_ordinal_histogram[1] == 0);
        CHECK(zero.avg_probe_ordinal == 1.0);
        // cached 1 + early 1 + the miss 2 (its own shard's two files).
        CHECK(zero.generations_probed == 4);
    } else {
        // One shard: the cached v0 is `cached`'s home and also a file `early`
        // can be in, so `early` really is searched there first, then in v1.
        CHECK(zero.probe_ordinal_histogram[0] == 1);
        CHECK(zero.probe_ordinal_histogram[1] == 1);
        CHECK(zero.avg_probe_ordinal == 1.5);
        // v0 (cached): three keys searched; v1: early and the miss.
        CHECK(zero.generations_probed == 5);
    }
    db.close();
}

// =============================================================================
// Deletions walk the same history, and route the same way
// =============================================================================

TEST_CASE("a historical deletion walks only its key's shard", "[routing][shard][deletion]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const fx = build_layered(t.dir);

    SECTION("keys of several shards and a miss, nothing failing") {
        auto db = std::move(*full_db::open_for_testing(t.dir, false));
        auto const miss = key_in_shard(0, shards > 1 ? 1 : 0, 900003);
        std::vector<deferred_deletion_entry> const batch{
            {fx.hist[last_shard][1], query_height}, {fx.hist[0][0], query_height},
            {miss, query_height}, {fx.hist[0][0], query_height}};
        auto const progress = db.apply_deletes(batch);
        CHECK_FALSE(progress.error.has_value());
        CHECK(progress.erased.size() == 2);
        REQUIRE(progress.absent.size() == 1);
        CHECK(progress.absent.front().key == miss);
        CHECK(progress.unresolved.empty());

        // And they are gone, while a neighbour is not. The neighbour is the
        // last shard's filler in its v1: no batch here names it, and at N=1 the
        // placed keys of "the last shard" are the ones just deleted.
        auto const neighbour = key_in_shard(0, last_shard, 5000 + 16 * last_shard);
        std::vector<lookup_request> const check{
            {fx.hist[last_shard][1], query_height}, {fx.hist[0][0], query_height},
            {neighbour, query_height}};
        auto const resolved = db.resolve(check);
        REQUIRE(resolved.has_value());
        CHECK(found_keys(*resolved) == std::set<raw_outpoint>{neighbour});
        db.close();
    }
    SECTION("a refusing file of a shard the batch does not touch does not stop it") {
        failpoints::fail_historical_open_version.store(0, std::memory_order_relaxed);
        auto db = std::move(*full_db::open_for_testing(t.dir, false));
        std::vector<deferred_deletion_entry> const batch{{fx.hist[last_shard][1], query_height}};
        auto const progress = db.apply_deletes(batch);
        CHECK_FALSE(progress.error.has_value());
        CHECK(progress.erased.size() == 1);
        CHECK(progress.unresolved.empty());
        db.close();
    }
    SECTION("a key whose own file refuses stays owed, never absent") {
        failpoints::fail_historical_open_version.store(0, std::memory_order_relaxed);
        auto db = std::move(*full_db::open_for_testing(t.dir, false));
        std::vector<deferred_deletion_entry> const batch{{fx.hist[last_shard][0], query_height}};
        auto const progress = db.apply_deletes(batch);
        REQUIRE(progress.error.has_value());
        CHECK(*progress.error == error_code::version_unreadable);
        CHECK(progress.erased.empty());
        CHECK(progress.absent.empty());
        CHECK(progress.unresolved.size() == 1);
        db.close();
    }
}
