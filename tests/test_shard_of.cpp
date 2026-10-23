// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "detail/format_identity.hpp"
#include "detail/shard.hpp"

using namespace utxoz;
using utxoz::detail::shard_of;
using utxoz::detail::valid_shard_count;

// =============================================================================
// shard_of is part of the persisted format
// =============================================================================
//
// A key's shard decides which file holds it. Change this function and every
// entry moves, so what is pinned here is not "a reasonable distribution" but
// EXACT OUTPUTS. The golden vectors below are the contract; a change that alters
// any of them requires a new geometry_id and a rebuilt database, and this file
// is where that is noticed.

namespace {

constexpr raw_outpoint make(std::array<uint8_t, 32> const& txid, uint32_t index) {
    raw_outpoint key{};
    for (size_t i = 0; i != 32; ++i) key[i] = txid[i];
    key[32] = uint8_t(index);
    key[33] = uint8_t(index >> 8);
    key[34] = uint8_t(index >> 16);
    key[35] = uint8_t(index >> 24);
    return key;
}
constexpr std::array<uint8_t, 32> filled(uint8_t v) {
    std::array<uint8_t, 32> a{};
    for (auto& b : a) b = v;
    return a;
}
constexpr std::array<uint8_t, 32> ramp() {
    std::array<uint8_t, 32> a{};
    for (size_t i = 0; i != 32; ++i) a[i] = uint8_t(i);
    return a;
}

} // namespace

// -- the domain of N ---------------------------------------------------------

static_assert(valid_shard_count(1));
static_assert(valid_shard_count(2));
static_assert(valid_shard_count(4));
static_assert(valid_shard_count(8));
static_assert(valid_shard_count(16));
static_assert(valid_shard_count(32));
static_assert( ! valid_shard_count(0),  "zero shards is not a geometry");
static_assert( ! valid_shard_count(3),  "only powers of two: the reduction is a mask");
static_assert( ! valid_shard_count(64), "32 is the ceiling this format admits");

// -- N = 1 collapses to a single shard ---------------------------------------

static_assert(shard_of(make(filled(0x00), 0), 1) == 0);
static_assert(shard_of(make(filled(0xFF), 0xFFFFFFFFu), 1) == 0);
static_assert(shard_of(make(ramp(), 65535), 1) == 0);

// -- the golden vectors, fixed at compile time -------------------------------
//
// Twenty-two outpoints across N = 2, 4, 8, 16 and 32. If any of these changes,
// the format changed.

static_assert(shard_of(make(filled(0x00), 0), 2) == 0);
static_assert(shard_of(make(filled(0x00), 0), 4) == 0);
static_assert(shard_of(make(filled(0x00), 0), 8) == 0);
static_assert(shard_of(make(filled(0x00), 0), 16) == 0);
static_assert(shard_of(make(filled(0x00), 0), 32) == 0);

static_assert(shard_of(make(filled(0x00), 1), 2) == 1);
static_assert(shard_of(make(filled(0x00), 1), 4) == 3);
static_assert(shard_of(make(filled(0x00), 1), 8) == 7);
static_assert(shard_of(make(filled(0x00), 1), 16) == 15);
static_assert(shard_of(make(filled(0x00), 1), 32) == 15);

static_assert(shard_of(make(filled(0x00), 255), 8) == 6);
static_assert(shard_of(make(filled(0x00), 256), 8) == 6);
static_assert(shard_of(make(filled(0x00), 0xFFFFFFFFu), 8) == 1);

static_assert(shard_of(make(filled(0xFF), 0), 8) == 3);
static_assert(shard_of(make(filled(0xFF), 1), 8) == 4);
static_assert(shard_of(make(filled(0xFF), 255), 8) == 6);
static_assert(shard_of(make(filled(0xFF), 256), 8) == 0);
static_assert(shard_of(make(filled(0xFF), 0xFFFFFFFFu), 8) == 2);

static_assert(shard_of(make(ramp(), 0), 8) == 4);
static_assert(shard_of(make(ramp(), 1), 8) == 2);
static_assert(shard_of(make(ramp(), 2), 8) == 5);
static_assert(shard_of(make(ramp(), 3), 8) == 1);
static_assert(shard_of(make(ramp(), 7), 8) == 1);
static_assert(shard_of(make(ramp(), 255), 8) == 6);
static_assert(shard_of(make(ramp(), 65535), 8) == 5);
static_assert(shard_of(make(ramp(), 0xFFFFFFFFu), 8) == 2);

static_assert(shard_of(make(filled(0x80), 0), 8) == 4);
static_assert(shard_of(make(filled(0x7F), 0), 8) == 7);
static_assert(shard_of(make(filled(0x01), 0), 8) == 2);
static_assert(shard_of(make(filled(0xFE), 0), 8) == 4);

TEST_CASE("shard_of is bounded by N", "[shard]") {
    for (uint32_t n : {1u, 2u, 4u, 8u, 16u, 32u}) {
        for (uint32_t i = 0; i != 512; ++i) {
            auto const key = make(ramp(), i);
            CHECK(shard_of(key, n) < n);
        }
    }
}

TEST_CASE("the outputs of one transaction do not land in one shard", "[shard]") {
    // The failure mode this function exists to avoid. A txid-only function put
    // every output of a transaction together, and one real transaction has
    // 31,243 live outputs.
    for (uint32_t n : {4u, 8u, 16u}) {
        std::map<uint32_t, int> occupancy;
        for (uint32_t i = 0; i != 256; ++i) ++occupancy[shard_of(make(ramp(), i), n)];
        CHECK(occupancy.size() == n);
        // No shard takes more than twice its share of 256 outputs.
        for (auto const& [shard, count] : occupancy) {
            CHECK(count <= int(2 * 256 / n));
        }
    }
}

TEST_CASE("only the bytes the contract names are read", "[shard]") {
    // Bytes 0..23 must not change the answer: they are the map hash's, not ours.
    auto base = make(ramp(), 7);
    auto const expected = shard_of(base, 8);
    for (size_t i = 0; i != 24; ++i) {
        auto altered = base;
        altered[i] = uint8_t(~altered[i]);
        CHECK(shard_of(altered, 8) == expected);
    }
    // And bytes 24..35 must: flipping any of them is allowed to move the key,
    // and at least one of them does.
    bool moved = false;
    for (size_t i = 24; i != 36; ++i) {
        auto altered = base;
        altered[i] = uint8_t(~altered[i]);
        if (shard_of(altered, 8) != expected) moved = true;
    }
    CHECK(moved);
}

TEST_CASE("the reduction is a mask, and N=1 is the identity", "[shard]") {
    for (uint32_t i = 0; i != 1024; ++i) {
        auto const key = make(filled(uint8_t(i)), i);
        CHECK(shard_of(key, 1) == 0);
        // Nesting: a key's shard under 2N determines its shard under N.
        CHECK(shard_of(key, 4) % 2 == shard_of(key, 2));
        CHECK(shard_of(key, 8) % 4 == shard_of(key, 4));
        CHECK(shard_of(key, 16) % 8 == shard_of(key, 8));
    }
}

// =============================================================================
// Names: no old file may be adopted as a shard, and no shard as another
// =============================================================================
//
// The prefixes have to be disjoint in both directions, and not merely different.
// `enumerate_versions` matches on a prefix, so if one name were a prefix of
// another, a recovery or a catalogue scan would pick up files belonging to a
// different container and treat their versions as its own.

TEST_CASE("an unsharded name is not a prefix of a sharded one, or the reverse",
          "[shard][names]") {
    using namespace utxoz::detail;

    // What a build with one shard writes for class 0, spelled out rather than
    // computed, because the point is that this exact string keeps its meaning.
    std::string const unsharded = "cont_0_v";

    for (uint32_t n : {2u, 4u, 8u, 16u, 32u}) {
        for (uint32_t sh = 0; sh != n; ++sh) {
            auto const sharded = "cont_0s" + std::to_string(sh) + "_v";
            CAPTURE(n, sh, sharded);

            // Neither name may begin with the other. The dangerous direction is
            // the second: a store written before sharding must not be swept up
            // by shard zero, which would read another geometry's files as its
            // own history.
            CHECK_FALSE(sharded.starts_with(unsharded));
            CHECK_FALSE(unsharded.starts_with(sharded));
        }
    }
}

TEST_CASE("no two slots share a prefix, and none is a prefix of another",
          "[shard][names]") {
    using namespace utxoz::detail;

    std::vector<std::string> prefixes;
    for (size_t slot = 0; slot != slot_count; ++slot) {
        prefixes.push_back(container_prefix(slot));
    }

    for (size_t i = 0; i != prefixes.size(); ++i) {
        for (size_t j = 0; j != prefixes.size(); ++j) {
            if (i == j) continue;
            CAPTURE(i, j, prefixes[i], prefixes[j]);
            // Equality is the obvious failure; the prefix relation is the one
            // that would go unnoticed, because the files still look plausible.
            CHECK_FALSE(prefixes[i].starts_with(prefixes[j]));
        }
    }
}

TEST_CASE("a file name says which slot it belongs to, and only that one",
          "[shard][names]") {
    using namespace utxoz::detail;

    for (size_t slot = 0; slot != slot_count; ++slot) {
        auto const name = container_file_name(slot, 7);
        CAPTURE(slot, name);

        size_t matches = 0;
        for (size_t other = 0; other != slot_count; ++other) {
            if (name.starts_with(container_prefix(other))) ++matches;
        }
        CHECK(matches == 1);
    }
}

// =============================================================================
// The shard count is part of the geometry
// =============================================================================

TEST_CASE("production is eight shards and geometry 4; every other count is experimental",
          "[shard][format]") {
    using namespace utxoz::detail;

    // A production build has no choice: eight shards, geometry 4, and a production
    // id. An experimental build has another count and an id in the experimental
    // namespace. Nothing in between.
    if ( ! utxoz::experimental_build) {
        CHECK(utxoz::shard_count == 8);
        CHECK(utxoz::shard_count == utxoz::production_shard_count);
        CHECK(geometry_id == 4);
        CHECK(geometry_id == production_geometry_id);
    } else {
        CHECK(utxoz::shard_count != 8);
        CHECK((geometry_id & 0x80000000u) != 0);
        CHECK(geometry_id == experimental_geometry_id_for_shards(utxoz::shard_count));
    }

    // Every experimental count names a distinct id, none of them a production one
    // and none of them geometry 3 - so no experimental build, the unsharded one
    // included, writes a database that looks like one from before 1.0. Two that
    // collided would let a store written under one open under the other: the same
    // key resolves to a different container, and every lookup asks the wrong file.
    std::set<uint32_t> ids{production_geometry_id, 1u, 2u, 3u};
    for (uint32_t n : {1u, 2u, 4u, 16u, 32u}) {
        auto const id = experimental_geometry_id_for_shards(n);
        CAPTURE(n, id);
        CHECK((id & 0x80000000u) != 0);
        CHECK(ids.insert(id).second);
    }
    CHECK(ids.size() == 9);
    // Eight has no experimental id: it is production.
    CHECK(experimental_geometry_id_for_shards(8) == 0);
}
