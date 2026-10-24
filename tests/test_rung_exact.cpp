// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_rung_exact.cpp
 * @brief The experimental rung-exact testing profile, as the containers see it.
 *
 * UTXOZ_TESTING_RUNG_EXACT gives each shard of the sharded class the N=1 rung
 * divided by N, in the smallest file the derived segment bound says holds it.
 * It exists to compare shard counts on the same history: under `keep` the
 * class's capacity grows with N, and under `divide` the ten megabytes fall one
 * rung short at N=8. The table is checked at compile time in
 * capacity_policy.hpp; these cases check what a real store does with it — that
 * a shard is created at that rung and rotates at that rung's threshold, which
 * is the part a static assertion cannot see.
 *
 * Every case skips in a build without the profile. At N=1 the profile is the
 * testing profile, unchanged, and that is asserted rather than assumed.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/census.hpp>
#include <utxoz/database.hpp>
#include <utxoz/types.hpp>

#include "detail/capacity_policy.hpp"
#include "shard_paths.hpp"

namespace fs = std::filesystem;
using namespace utxoz;
using utxoz::detail::testing_slot_capacity;
using utxoz::detail::testing_capacity;
using utxoz::testing::generations_in_shard;
using utxoz::testing::key_in_shard;

namespace {

struct temp_db {
    temp_db() {
        static std::atomic<uint64_t> counter{0};
        auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        dir = fmt::format("./test_rung_exact_{}_{}_{}", getpid(), ts, counter.fetch_add(1));
        fs::remove_all(dir);
    }
    ~temp_db() { std::error_code ec; fs::remove_all(dir, ec); }
    temp_db(temp_db const&) = delete;
    temp_db& operator=(temp_db const&) = delete;
    fs::path dir;
};

constexpr bool profile = utxoz::detail::testing_rung_exact;
constexpr uint32_t shards = utxoz::shard_count;

/// The N=1 rung divided by N, written out rather than taken from the library.
constexpr size_t expected_rung() {
    switch (shards) {
        case 1:  return 122879;
        case 2:  return 61439;
        case 4:  return 30719;
        case 8:  return 15359;
        case 16: return 7679;
        case 32: return 3839;
        default: return 0;
    }
}

} // namespace

TEST_CASE("the rung-exact profile gives each shard the N=1 rung divided by N",
          "[capacity][shard][profile]") {
    if constexpr ( ! profile) SKIP("built without UTXOZ_TESTING_RUNG_EXACT");

    for (uint32_t sh = 0; sh != shards; ++sh) {
        INFO("shard " << sh);
        auto const& e = testing_slot_capacity[sh];
        CHECK(e.bucket_count == expected_rung());
        CHECK(e.capacity == expected_rung());
        if constexpr (shards == 1) {
            // Unchanged: the testing profile's own entry, file size included.
            CHECK(e == testing_capacity[0]);
        } else {
            // The smallest file the derived bound says holds the rung.
            CHECK(e.file_size == utxoz::detail::segment_bytes_needed(
                                     expected_rung(), utxoz::detail::sharded_value_type_bytes));
        }
    }
    // The other classes are not sharded and are not touched.
    for (size_t cls = 1; cls < container_count; ++cls) {
        INFO("class " << cls);
        CHECK(testing_slot_capacity[shards + cls - 1] == testing_capacity[cls]);
    }
}

TEST_CASE("under the rung-exact profile a shard is created at its rung and rotates "
          "at that rung's threshold",
          "[capacity][shard][profile][rotation]") {
    if constexpr ( ! profile) SKIP("built without UTXOZ_TESTING_RUNG_EXACT");
    temp_db t;

    uint32_t const target = shards - 1;
    auto const threshold = utxoz::detail::max_entries_for(expected_rung());
    std::vector<uint8_t> const payload(8, 0x5A);   // lands in the sharded class

    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t n = 0; n != threshold; ++n) {
            REQUIRE(db.insert(key_in_shard(0, target, n), payload, 800000).has_value());
        }
        db.close();
    }
    // Full to the threshold and not rotated: the threshold is inclusive.
    CHECK(generations_in_shard(t.dir, 0, target) == 1);

    {
        auto db = std::move(*full_db::open_for_testing(t.dir, false));
        REQUIRE(db.insert(key_in_shard(0, target, threshold), payload, 800000).has_value());
        db.close();
    }
    CHECK(generations_in_shard(t.dir, 0, target) == 2);
    for (uint32_t sh = 0; sh != shards; ++sh) {
        if (sh == target) continue;
        INFO("untouched shard " << sh);
        CHECK(generations_in_shard(t.dir, 0, sh) == 1);
    }

    // What the store reports about the rotated generation: the rung it was built
    // at, the entries it holds, and a segment with room left in it - which is the
    // real check on the derived file size.
    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto const census = db.census();
    REQUIRE(census.has_value());
    auto const it = std::ranges::find_if(census->classes, [](auto const& c) {
        return c.container_class == 0;
    });
    REQUIRE(it != census->classes.end());
    bool seen = false;
    for (auto const& g : it->generations_detail) {
        if (g.shard != target || g.active) continue;
        seen = true;
        CHECK(g.bucket_count == expected_rung());
        CHECK(g.entries == threshold);
        CHECK(g.segment_free_bytes > 0);
        if constexpr (shards > 1) {
            CHECK(g.logical_file_bytes == testing_slot_capacity[target].file_size);
        }
    }
    CHECK(seen);
    db.close();
}
