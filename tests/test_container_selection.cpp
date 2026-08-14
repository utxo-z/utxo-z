// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_container_selection.cpp
 * @brief Which container a value lands in, at the sizes where it changes.
 *
 * The classes are coarse on purpose, so almost every payload sits somewhere in
 * the middle of one and proves nothing about where the line is. These are the
 * sizes on the line.
 *
 * Geometry 2 moved one of those lines. The second class is 96 rather than 94 —
 * the object always occupied 96, because 94 rounds up, so the two bytes it gained
 * were padding no caller could reach. Naming the class 96 turns them into
 * payload and costs nothing: same object, same stored pair, same bytes per slot.
 * What it changes is where a 90- or 91-byte output lives, and that is a format
 * change, which is why `geometry_id` moved with it.
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

#include "detail/file_cache.hpp"
#include "detail/segment_open.hpp"
#include "detail/utxo_value.hpp"

namespace fs = std::filesystem;

namespace {

struct temp_db {
    temp_db() {
        static std::atomic<uint64_t> counter{0};
        auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        dir = fmt::format("./test_selection_{}_{}_{}", getpid(), ts, counter.fetch_add(1));
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

utxoz::raw_outpoint key_of(uint32_t n) {
    utxoz::raw_outpoint key{};
    std::memcpy(key.data(), &n, sizeof(n));
    return key;
}

/// A payload whose every byte is distinguishable, so a value recovered one byte
/// short — or one byte long — is a failure rather than a coincidence.
std::vector<uint8_t> payload_of(size_t size) {
    std::vector<uint8_t> v(size);
    std::iota(v.begin(), v.end(), uint8_t(1));
    return v;
}

/// Whether a key is in container `Index`'s map, asked of the file.
///
/// The public `find()` searches every container, so it answers "the store has
/// this" and never "it is here". And the mere existence of `cont_N_v00000.dat`
/// says only that something went there — in a test that fills several containers,
/// every file exists and the check is vacuous. This opens the segment and looks
/// in the named map with the type that container actually holds.
template <size_t Index>
bool key_in_container(fs::path const& dir, utxoz::raw_outpoint const& key) {
    constexpr size_t Size = utxoz::container_sizes[Index];
    auto const file = dir / fmt::format(utxoz::detail::data_file_format, Index, 0);
    if ( ! fs::exists(file)) return false;

    auto opened = utxoz::detail::open_existing_segment(file);
    REQUIRE(opened.has_value());
    auto const found = utxoz::detail::find_single_named<utxoz::detail::utxo_map<Size>>(
        **opened, utxoz::detail::map_object_name, file);
    REQUIRE(found.has_value());
    return (**found).find(key) != (**found).end();
}

/// The payload stored for `key` in container `Index`, straight out of the map.
template <size_t Index>
std::vector<uint8_t> stored_payload(fs::path const& dir, utxoz::raw_outpoint const& key) {
    constexpr size_t Size = utxoz::container_sizes[Index];
    auto const file = dir / fmt::format(utxoz::detail::data_file_format, Index, 0);
    auto opened = utxoz::detail::open_existing_segment(file);
    REQUIRE(opened.has_value());
    auto const found = utxoz::detail::find_single_named<utxoz::detail::utxo_map<Size>>(
        **opened, utxoz::detail::map_object_name, file);
    REQUIRE(found.has_value());
    auto const it = (**found).find(key);
    REQUIRE(it != (**found).end());
    auto const data = it->second.get_data();
    return {data.begin(), data.end()};
}

/// Dispatches the two above over a runtime index, so a loop over the geometry can
/// ask about any container without the callers unrolling it by hand.
bool key_in(fs::path const& dir, size_t index, utxoz::raw_outpoint const& key) {
    switch (index) {
        case 0: return key_in_container<0>(dir, key);
        case 1: return key_in_container<1>(dir, key);
        case 2: return key_in_container<2>(dir, key);
        case 3: return key_in_container<3>(dir, key);
        case 4: return key_in_container<4>(dir, key);
    }
    FAIL("no such container: " << index);
    return false;
}

std::vector<uint8_t> payload_in(fs::path const& dir, size_t index, utxoz::raw_outpoint const& key) {
    switch (index) {
        case 0: return stored_payload<0>(dir, key);
        case 1: return stored_payload<1>(dir, key);
        case 2: return stored_payload<2>(dir, key);
        case 3: return stored_payload<3>(dir, key);
        case 4: return stored_payload<4>(dir, key);
    }
    FAIL("no such container: " << index);
    return {};
}

} // namespace

TEST_CASE("the capacities follow the classes", "[geometry]") {
    STATIC_REQUIRE(utxoz::container_sizes == std::array<size_t, 5>{48, 96, 128, 256, 10240});
    // The dispatch above unrolls the containers by hand, so a sixth one has to
    // come through here rather than silently falling off the end of a switch.
    STATIC_REQUIRE(utxoz::container_sizes.size() == 5);

    // Derived, not listed twice: a class changed above has to move these.
    STATIC_REQUIRE(utxoz::container_capacities[0] == 43);
    STATIC_REQUIRE(utxoz::container_capacities[1] == 91);
    STATIC_REQUIRE(utxoz::container_capacities[2] == 123);
    STATIC_REQUIRE(utxoz::container_capacities[3] == 250);
    STATIC_REQUIRE(utxoz::container_capacities[4] == 10234);
}

TEST_CASE("the sizes around the second class land where geometry 2 says", "[geometry]") {
    // 89 was the old capacity and 91 is the new one. Under geometry 1 the two
    // sizes between them went to container 2; under geometry 2 they do not, and
    // 92 still does. Four payloads, one line moved.
    temp_db t;
    struct entry { size_t payload; size_t container; utxoz::raw_outpoint key; };
    std::vector<entry> cases{{89, 1, {}}, {90, 1, {}}, {91, 1, {}}, {92, 2, {}}};

    {
        auto opened = utxoz::full_db::open_for_testing(t.dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        uint32_t n = 0;
        for (auto& c : cases) {
            INFO("payload " << c.payload << " bytes");
            c.key = key_of(++n);
            REQUIRE(db.insert(c.key, payload_of(c.payload), 800000).has_value());

            auto const found = db.find(c.key, 800001);
            REQUIRE(found.has_value());
            CHECK(found->data.size() == c.payload);
            CHECK(std::ranges::equal(found->data, payload_of(c.payload)));
        }
        db.close();
    }

    // Where each key actually is, asked of the maps. Both containers hold
    // something here, so the files existing proves nothing about any one key —
    // and `find()` answers "the store has it", never "it is here".
    for (auto const& c : cases) {
        INFO("payload " << c.payload << " bytes -> container " << c.container);
        CHECK(key_in(t.dir, c.container, c.key));
        CHECK_FALSE(key_in(t.dir, c.container == 1 ? 2 : 1, c.key));
        CHECK(std::ranges::equal(payload_in(t.dir, c.container, c.key), payload_of(c.payload)));
    }
}

TEST_CASE("every class takes its capacity and refuses one byte more", "[geometry]") {
    // The two sizes that decide each line: the last a class accepts, and the
    // first it does not. Inserting only the capacity would leave the upper edge
    // untested, and the upper edge is the half that moves when a class changes.
    temp_db t;
    struct entry { size_t payload; size_t container; utxoz::raw_outpoint key; };
    std::vector<entry> placed;

    auto const last = utxoz::container_sizes.size() - 1;
    utxoz::raw_outpoint too_large_key{};
    std::vector<uint8_t> const too_large = payload_of(utxoz::container_capacities[last] + 1);

    {
        auto opened = utxoz::full_db::open_for_testing(t.dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        uint32_t n = 0;
        for (size_t i = 0; i < utxoz::container_sizes.size(); ++i) {
            auto const capacity = utxoz::container_capacities[i];
            INFO("container " << i << ", capacity " << capacity);

            placed.push_back({capacity, i, key_of(++n)});
            REQUIRE(db.insert(placed.back().key, payload_of(capacity), 800000).has_value());

            if (i + 1 < utxoz::container_sizes.size()) {
                placed.push_back({capacity + 1, i + 1, key_of(++n)});
                REQUIRE(db.insert(placed.back().key, payload_of(capacity + 1), 800000).has_value());
            }
        }

        // One byte past the largest class fits nowhere, and says so rather than
        // storing a value with its end cut off.
        too_large_key = key_of(++n);
        auto const refused = db.insert(too_large_key, too_large, 800000);
        REQUIRE_FALSE(refused.has_value());
        CHECK(refused.error() == utxoz::error_code::value_too_large);
        CHECK_FALSE(db.find(too_large_key, 800001).has_value());

        db.close();
    }

    for (auto const& p : placed) {
        INFO("payload " << p.payload << " bytes -> container " << p.container);
        CHECK(key_in(t.dir, p.container, p.key));
        CHECK(std::ranges::equal(payload_in(t.dir, p.container, p.key), payload_of(p.payload)));

        // And nowhere else: a value one byte over a capacity that stayed put
        // would still read back correctly from the class it was too big for.
        for (size_t other = 0; other < utxoz::container_sizes.size(); ++other) {
            if (other == p.container) continue;
            INFO("must not be in container " << other);
            CHECK_FALSE(key_in(t.dir, other, p.key));
        }
    }

    // The refused value is in none of them.
    for (size_t i = 0; i < utxoz::container_sizes.size(); ++i) {
        INFO("a value too large for any class must not be in container " << i);
        CHECK_FALSE(key_in(t.dir, i, too_large_key));
    }
}
