// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_storage_correctness.cpp
 * @brief Storage correctness tests: persistence, rotation, cross-version lookups
 *
 * These tests verify that the storage layer correctly persists data across
 * close/reopen cycles, handles file rotation, and serves lookups/deletes
 * across multiple file versions.
 */

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <numeric>
#include <random>
#include <map>
#include <set>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>
#include <utxoz/utils.hpp>

namespace {

inline std::atomic<uint64_t> test_counter{0};

std::string make_unique_path() {
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto unique_id = test_counter.fetch_add(1);
    return fmt::format("./test_storage_{}_{}_{}", getpid(), ts, unique_id);
}

utxoz::raw_outpoint make_test_key(uint32_t tx_id, uint32_t output_index) {
    utxoz::raw_outpoint key{};
    for (size_t i = 0; i < 32; ++i) {
        key[i] = static_cast<uint8_t>((tx_id >> (i % 4 * 8)) & 0xFF);
    }
    std::memcpy(key.data() + 32, &output_index, 4);
    return key;
}

std::vector<uint8_t> make_test_value(size_t size, uint8_t seed = 0) {
    std::vector<uint8_t> value(size);
    std::iota(value.begin(), value.end(), seed);
    return value;
}

struct ScopedTestDir {
    std::string path;

    ScopedTestDir() : path(make_unique_path()) {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove_all(path);
        }
    }

    ~ScopedTestDir() {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove_all(path);
        }
    }
};

} // anonymous namespace

// =============================================================================
// Persistence: close + reopen
// =============================================================================

TEST_CASE("Reopen: data persists after close and reopen", "[storage][persistence]") {
    ScopedTestDir dir;
    constexpr size_t N = 500;

    std::vector<utxoz::raw_outpoint> keys;
    std::vector<std::vector<uint8_t>> values;

    // Phase 1: insert data and close
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);

        for (size_t i = 0; i < N; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), 0);
            auto val = make_test_value(30, static_cast<uint8_t>(i & 0xFF));
            keys.push_back(key);
            values.push_back(val);
            REQUIRE(db.insert(key, val, static_cast<uint32_t>(100 + i)));
        }

        CHECK(db.size() == N);
        db.close();
    }

    // Phase 2: reopen and verify all data
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);  // do NOT remove existing

        for (size_t i = 0; i < N; ++i) {
            auto result = db.find(keys[i], 1000);
            REQUIRE(result.has_value());
            CHECK(*result == values[i]);
        }

        db.close();
    }
}

TEST_CASE("Reopen: multiple close/reopen cycles accumulate data", "[storage][persistence]") {
    ScopedTestDir dir;
    constexpr size_t entries_per_cycle = 100;
    constexpr size_t cycles = 5;

    std::vector<utxoz::raw_outpoint> all_keys;
    std::vector<std::vector<uint8_t>> all_values;

    for (size_t c = 0; c < cycles; ++c) {
        utxoz::db db;
        if (c == 0) {
            db.configure_for_testing(dir.path, true);
        } else {
            db.configure_for_testing(dir.path, false);
        }

        // Insert new batch
        for (size_t i = 0; i < entries_per_cycle; ++i) {
            uint32_t id = static_cast<uint32_t>(c * entries_per_cycle + i);
            auto key = make_test_key(id, 0);
            auto val = make_test_value(30, static_cast<uint8_t>(id & 0xFF));
            all_keys.push_back(key);
            all_values.push_back(val);
            REQUIRE(db.insert(key, val, id + 100));
        }

        // Verify ALL data from all cycles so far
        for (size_t i = 0; i < all_keys.size(); ++i) {
            auto result = db.find(all_keys[i], 9999);
            REQUIRE(result.has_value());
            CHECK(*result == all_values[i]);
        }

        db.close();
    }

    CHECK(all_keys.size() == cycles * entries_per_cycle);
}

TEST_CASE("Reopen: all container sizes persist correctly", "[storage][persistence]") {
    ScopedTestDir dir;

    struct TestEntry {
        utxoz::raw_outpoint key;
        std::vector<uint8_t> value;
    };

    std::vector<TestEntry> entries;

    // Insert one entry per container size
    // Capacities: 43, 89, 123, 250, 10234
    std::array<size_t, 5> value_sizes = {30, 50, 100, 200, 8000};

    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);

        for (size_t i = 0; i < value_sizes.size(); ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i + 1), 0);
            auto val = make_test_value(value_sizes[i], static_cast<uint8_t>(i * 42));
            entries.push_back({key, val});
            REQUIRE(db.insert(key, val, static_cast<uint32_t>(100 + i)));
        }

        db.close();
    }

    // Reopen and verify
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);

        for (auto const& [key, val] : entries) {
            auto result = db.find(key, 9999);
            REQUIRE(result.has_value());
            CHECK(*result == val);
        }

        db.close();
    }
}

// =============================================================================
// File rotation
// =============================================================================

TEST_CASE("Rotation: data accessible after file rotation", "[storage][rotation]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // Insert enough entries to trigger at least one rotation.
    // With 10 MiB test files and container 0 (44B values), rotation happens
    // when load factor approaches the limit (~100K+ entries).
    // We check statistics to confirm rotation occurred.
    constexpr size_t N = 200'000;
    std::vector<utxoz::raw_outpoint> keys;
    keys.reserve(N);

    for (size_t i = 0; i < N; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
        keys.push_back(key);
        auto val = make_test_value(30, static_cast<uint8_t>(i & 0xFF));
        REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
    }

    // Verify at least one rotation happened
    auto stats = db.get_statistics();
    bool any_rotation = false;
    for (size_t i = 0; i < utxoz::container_count; ++i) {
        if (stats.rotations_per_container[i] > 0) {
            any_rotation = true;
            break;
        }
    }
    REQUIRE(any_rotation);

    // Verify a random sample of entries (including ones that ended up in older versions)
    std::mt19937 gen(42);
    std::uniform_int_distribution<size_t> dist(0, N - 1);

    size_t found_in_latest = 0;
    size_t deferred = 0;

    for (int test = 0; test < 1000; ++test) {
        size_t idx = dist(gen);
        auto result = db.find(keys[idx], static_cast<uint32_t>(N));
        if (result.has_value()) {
            ++found_in_latest;
        } else {
            ++deferred;
        }
    }

    // Some should be found immediately (latest version), rest are deferred
    CHECK(found_in_latest > 0);

    // Process deferred lookups and verify they succeed
    if (deferred > 0) {
        auto [successful, failed] = db.process_pending_lookups();
        // Most deferred lookups should succeed (the data is there, just in older files)
        CHECK(successful.size() + failed.size() > 0);
    }

    db.close();
}

// =============================================================================
// Cross-version erase (deferred deletions)
// =============================================================================

TEST_CASE("Deferred erase: delete entries from previous versions", "[storage][rotation][deferred]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // Insert enough to trigger rotation
    constexpr size_t N = 200'000;
    std::vector<utxoz::raw_outpoint> keys;
    keys.reserve(N);

    for (size_t i = 0; i < N; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
        keys.push_back(key);
        auto val = make_test_value(30, static_cast<uint8_t>(i & 0xFF));
        REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
    }

    // Confirm rotation
    auto stats = db.get_statistics();
    bool any_rotation = false;
    for (size_t i = 0; i < utxoz::container_count; ++i) {
        if (stats.rotations_per_container[i] > 0) any_rotation = true;
    }
    REQUIRE(any_rotation);

    // Erase the first 1000 entries (likely in older versions → deferred)
    size_t immediate_erases = 0;
    size_t deferred_erases = 0;

    for (size_t i = 0; i < 1000; ++i) {
        auto result = db.erase(keys[i], static_cast<uint32_t>(N + 1));
        if (result > 0) {
            ++immediate_erases;
        } else {
            ++deferred_erases;
        }
    }

    // Process deferred deletions
    auto [deleted_count, failed] = db.process_pending_deletions();

    // Total successful should equal 1000 (immediate + deferred)
    size_t total_deleted = immediate_erases + deleted_count;
    CHECK(total_deleted == 1000);
    CHECK(failed.empty());

    db.close();
}

// =============================================================================
// Cross-version lookups (deferred lookups)
// =============================================================================

TEST_CASE("Deferred lookups: find entries in previous versions", "[storage][rotation][deferred]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // Insert enough to trigger rotation
    constexpr size_t N = 200'000;
    std::vector<utxoz::raw_outpoint> keys;
    std::vector<std::vector<uint8_t>> values;
    keys.reserve(N);
    values.reserve(N);

    for (size_t i = 0; i < N; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
        auto val = make_test_value(30, static_cast<uint8_t>(i & 0xFF));
        keys.push_back(key);
        values.push_back(val);
        REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
    }

    // Lookup entries that are definitely in old versions (first inserted)
    size_t deferred_count = 0;
    for (size_t i = 0; i < 500; ++i) {
        auto result = db.find(keys[i], static_cast<uint32_t>(N));
        if (!result.has_value()) {
            ++deferred_count;
        }
    }

    // Process deferred lookups
    auto [successful, failed] = db.process_pending_lookups();

    // Verify the found values are correct
    for (auto const& [key, found_value] : successful) {
        // Find the original index for this key
        for (size_t i = 0; i < 500; ++i) {
            if (keys[i] == key) {
                CHECK(found_value == values[i]);
                break;
            }
        }
    }

    // All lookups should eventually succeed (immediate + deferred)
    CHECK(failed.empty());

    db.close();
}

// =============================================================================
// Compaction after rotation
// =============================================================================

TEST_CASE("Compaction: data integrity preserved after compact_all", "[storage][compaction]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // Insert enough to get multiple versions
    constexpr size_t N = 200'000;
    std::vector<utxoz::raw_outpoint> keys;
    keys.reserve(N);

    for (size_t i = 0; i < N; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
        keys.push_back(key);
        auto val = make_test_value(30, static_cast<uint8_t>(i & 0xFF));
        REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
    }

    // Erase some entries before compaction
    for (size_t i = 0; i < 1000; ++i) {
        (void)db.erase(keys[i], static_cast<uint32_t>(N + 1));
    }
    (void)db.process_pending_deletions();

    // Compact
    db.compact_all();

    // Verify remaining entries (1000..N) are accessible
    // After compaction there may still be multiple files, so some finds
    // may be deferred. We process deferred lookups to verify them.
    std::mt19937 gen(42);
    std::uniform_int_distribution<size_t> dist(1000, N - 1);

    size_t found_immediate = 0;
    for (int test = 0; test < 500; ++test) {
        size_t idx = dist(gen);
        auto result = db.find(keys[idx], static_cast<uint32_t>(N + 2));
        if (result.has_value()) ++found_immediate;
    }

    auto [successful, failed_lookups] = db.process_pending_lookups();
    size_t total_found = found_immediate + successful.size();

    // All 500 sampled entries should be found (immediate or deferred)
    CHECK(total_found == 500);
    CHECK(failed_lookups.empty());

    // Verify erased entries are gone (these should not be found anywhere)
    for (size_t i = 0; i < 100; ++i) {
        auto result = db.find(keys[i], static_cast<uint32_t>(N + 2));
        CHECK_FALSE(result.has_value());
    }
    // Process any deferred lookups for erased entries — they should all fail
    auto [found_erased, failed_erased] = db.process_pending_lookups();
    CHECK(found_erased.empty());

    db.close();
}

// =============================================================================
// Persistence after rotation
// =============================================================================

TEST_CASE("Reopen after rotation: all versions survive close/reopen", "[storage][persistence][rotation]") {
    ScopedTestDir dir;

    constexpr size_t N = 200'000;
    std::vector<utxoz::raw_outpoint> keys;
    keys.reserve(N);

    // Phase 1: insert enough to rotate, then close
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);

        for (size_t i = 0; i < N; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
            keys.push_back(key);
            auto val = make_test_value(30, static_cast<uint8_t>(i & 0xFF));
            REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
        }

        auto stats = db.get_statistics();
        bool any_rotation = false;
        for (size_t i = 0; i < utxoz::container_count; ++i) {
            if (stats.rotations_per_container[i] > 0) any_rotation = true;
        }
        REQUIRE(any_rotation);

        db.close();
    }

    // Phase 2: reopen and verify data from latest version
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);

        // Recent entries (in latest version) should be found directly
        size_t found = 0;
        for (size_t i = N - 1000; i < N; ++i) {
            auto result = db.find(keys[i], static_cast<uint32_t>(N));
            if (result.has_value()) ++found;
        }

        // Most recent entries should be in the latest version
        CHECK(found > 0);

        db.close();
    }
}

// =============================================================================
// Value data integrity across all sizes
// =============================================================================

TEST_CASE("Value integrity: exact byte content preserved for all container sizes", "[storage][integrity]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // Test with values that fit within each container's data capacity.
    // utxo_value<Size> overhead: sizeof(uint32_t) + sizeof(size_type<Size>)
    //   Container 0 (48B):    max data = 43
    //   Container 1 (94B):    max data = 89
    //   Container 2 (128B):   max data = 123
    //   Container 3 (256B):   max data = 250
    //   Container 4 (10240B): max data = 10234
    std::vector<size_t> test_sizes = {1, 20, 43, 50, 89, 100, 123, 200, 250, 1000, 5000, 10234};

    struct Entry {
        utxoz::raw_outpoint key;
        std::vector<uint8_t> value;
    };
    std::vector<Entry> entries;

    for (size_t i = 0; i < test_sizes.size(); ++i) {
        size_t sz = test_sizes[i];
        // Skip sizes that exceed max container
        if (sz > 10240) continue;

        auto key = make_test_key(static_cast<uint32_t>(i + 1), 0);

        // Create a value with recognizable pattern
        std::vector<uint8_t> val(sz);
        for (size_t j = 0; j < sz; ++j) {
            val[j] = static_cast<uint8_t>((i * 31 + j * 7) & 0xFF);
        }

        entries.push_back({key, val});
        REQUIRE(db.insert(key, val, static_cast<uint32_t>(100 + i)));
    }

    // Verify all values byte-for-byte
    for (auto const& [key, expected] : entries) {
        auto result = db.find(key, 9999);
        REQUIRE(result.has_value());
        REQUIRE(result->size() == expected.size());
        CHECK(*result == expected);
    }

    db.close();

    // Reopen and verify again
    db.configure_for_testing(dir.path, false);

    for (auto const& [key, expected] : entries) {
        auto result = db.find(key, 9999);
        REQUIRE(result.has_value());
        REQUIRE(result->size() == expected.size());
        CHECK(*result == expected);
    }

    db.close();
}

// =============================================================================
// Erase + reopen: deletions persist
// =============================================================================

TEST_CASE("Erase persistence: erased entries stay gone after reopen", "[storage][persistence]") {
    ScopedTestDir dir;

    constexpr size_t N = 200;

    std::vector<utxoz::raw_outpoint> keys;

    // Phase 1: insert, erase half, close
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);

        for (size_t i = 0; i < N; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), 0);
            keys.push_back(key);
            auto val = make_test_value(30);
            REQUIRE(db.insert(key, val, 100));
        }

        // Erase first half
        for (size_t i = 0; i < N / 2; ++i) {
            CHECK(db.erase(keys[i], 200) == 1);
        }

        db.close();
    }

    // Phase 2: reopen, erased entries should be gone, rest should be present
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);

        for (size_t i = 0; i < N / 2; ++i) {
            auto result = db.find(keys[i], 300);
            CHECK_FALSE(result.has_value());
        }

        for (size_t i = N / 2; i < N; ++i) {
            auto result = db.find(keys[i], 300);
            CHECK(result.has_value());
        }

        db.close();
    }
}

// =============================================================================
// Edge cases: empty DB, single entry
// =============================================================================

TEST_CASE("Empty DB: close and reopen preserves empty state", "[storage][persistence][edge]") {
    ScopedTestDir dir;

    // Phase 1: create empty DB and close
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);
        CHECK(db.size() == 0);
        db.close();
    }

    // Phase 2: reopen, should still be empty and functional
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);
        CHECK(db.size() == 0);

        // Should be able to insert after reopening empty DB
        auto key = make_test_key(1, 0);
        auto val = make_test_value(30);
        REQUIRE(db.insert(key, val, 100));
        CHECK(db.size() == 1);

        auto result = db.find(key, 200);
        REQUIRE(result.has_value());
        CHECK(*result == val);

        db.close();
    }
}

TEST_CASE("Single entry: close and reopen per container size", "[storage][persistence][edge]") {
    std::array<size_t, 5> value_sizes = {30, 50, 100, 200, 8000};

    for (size_t vs : value_sizes) {
        ScopedTestDir dir;
        auto key = make_test_key(42, 7);
        auto val = make_test_value(vs, 0xAB);

        {
            utxoz::db db;
            db.configure_for_testing(dir.path, true);
            REQUIRE(db.insert(key, val, 100));
            CHECK(db.size() == 1);
            db.close();
        }

        {
            utxoz::db db;
            db.configure_for_testing(dir.path, false);
            CHECK(db.size() == 1);

            auto result = db.find(key, 200);
            REQUIRE(result.has_value());
            CHECK(*result == val);
            db.close();
        }
    }
}

// =============================================================================
// Erase all + close/reopen
// =============================================================================

TEST_CASE("Erase all: empty state persists after reopen", "[storage][persistence][edge]") {
    ScopedTestDir dir;
    constexpr size_t N = 100;

    std::vector<utxoz::raw_outpoint> keys;

    // Phase 1: insert N entries, erase all, close
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);

        for (size_t i = 0; i < N; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), 0);
            keys.push_back(key);
            auto val = make_test_value(30, static_cast<uint8_t>(i));
            REQUIRE(db.insert(key, val, 100));
        }
        CHECK(db.size() == N);

        for (size_t i = 0; i < N; ++i) {
            CHECK(db.erase(keys[i], 200) == 1);
        }
        CHECK(db.size() == 0);

        db.close();
    }

    // Phase 2: reopen, should be empty
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);
        CHECK(db.size() == 0);

        // None of the erased entries should be found
        for (size_t i = 0; i < N; ++i) {
            auto result = db.find(keys[i], 300);
            CHECK_FALSE(result.has_value());
        }

        // Should be able to re-insert after erase all + reopen
        auto val = make_test_value(30, 0xFF);
        REQUIRE(db.insert(keys[0], val, 300));
        auto result = db.find(keys[0], 400);
        REQUIRE(result.has_value());
        CHECK(*result == val);

        db.close();
    }
}

// =============================================================================
// Multi-cycle with interleaved deletes
// =============================================================================

TEST_CASE("Multi-cycle: insert, delete, close, insert, close, verify", "[storage][persistence]") {
    ScopedTestDir dir;

    std::vector<utxoz::raw_outpoint> keys;
    std::vector<std::vector<uint8_t>> values;

    // Cycle 1: insert 200 entries
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);

        for (size_t i = 0; i < 200; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), 0);
            auto val = make_test_value(30, static_cast<uint8_t>(i));
            keys.push_back(key);
            values.push_back(val);
            REQUIRE(db.insert(key, val, 100));
        }

        // Delete entries 50..99
        for (size_t i = 50; i < 100; ++i) {
            CHECK(db.erase(keys[i], 200) == 1);
        }

        db.close();
    }

    // Cycle 2: reopen, insert 100 more, delete entries 150..174
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);

        for (size_t i = 200; i < 300; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), 0);
            auto val = make_test_value(30, static_cast<uint8_t>(i));
            keys.push_back(key);
            values.push_back(val);
            REQUIRE(db.insert(key, val, 300));
        }

        for (size_t i = 150; i < 175; ++i) {
            CHECK(db.erase(keys[i], 400) == 1);
        }

        db.close();
    }

    // Cycle 3: reopen and verify final state
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);

        // Entries 0..49: should exist
        for (size_t i = 0; i < 50; ++i) {
            auto result = db.find(keys[i], 500);
            REQUIRE(result.has_value());
            CHECK(*result == values[i]);
        }

        // Entries 50..99: deleted in cycle 1
        for (size_t i = 50; i < 100; ++i) {
            auto result = db.find(keys[i], 500);
            CHECK_FALSE(result.has_value());
        }

        // Entries 100..149: should exist
        for (size_t i = 100; i < 150; ++i) {
            auto result = db.find(keys[i], 500);
            REQUIRE(result.has_value());
            CHECK(*result == values[i]);
        }

        // Entries 150..174: deleted in cycle 2
        for (size_t i = 150; i < 175; ++i) {
            auto result = db.find(keys[i], 500);
            CHECK_FALSE(result.has_value());
        }

        // Entries 175..299: should exist
        for (size_t i = 175; i < 300; ++i) {
            auto result = db.find(keys[i], 500);
            REQUIRE(result.has_value());
            CHECK(*result == values[i]);
        }

        db.close();
    }
}

// =============================================================================
// Compaction + close/reopen
// =============================================================================

TEST_CASE("Compaction persistence: data survives compact + close + reopen", "[storage][compaction][persistence]") {
    ScopedTestDir dir;

    constexpr size_t N = 200'000;
    std::vector<utxoz::raw_outpoint> keys;
    keys.reserve(N);

    // Phase 1: insert, erase some, compact, close
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);

        for (size_t i = 0; i < N; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
            keys.push_back(key);
            auto val = make_test_value(30, static_cast<uint8_t>(i & 0xFF));
            REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
        }

        // Erase first 500
        for (size_t i = 0; i < 500; ++i) {
            (void)db.erase(keys[i], static_cast<uint32_t>(N + 1));
        }
        (void)db.process_pending_deletions();

        db.compact_all();
        db.close();
    }

    // Phase 2: reopen and verify
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);

        // Erased entries should be gone
        for (size_t i = 0; i < 100; ++i) {
            auto result = db.find(keys[i], static_cast<uint32_t>(N + 2));
            CHECK_FALSE(result.has_value());
        }
        // Drain deferred lookups for erased entries before the next batch
        auto [found_erased, failed_erased] = db.process_pending_lookups();
        CHECK(found_erased.empty());

        // Check a sequential range of remaining entries (no duplicates)
        size_t found_immediate = 0;
        size_t not_found = 0;
        for (size_t i = 500; i < 1000; ++i) {
            auto expected = make_test_value(30, static_cast<uint8_t>(i & 0xFF));
            auto result = db.find(keys[i], static_cast<uint32_t>(N + 2));
            if (result.has_value()) {
                CHECK(*result == expected);
                ++found_immediate;
            } else {
                ++not_found;
            }
        }

        // Process deferred lookups
        auto [successful, failed] = db.process_pending_lookups();
        CHECK(failed.empty());

        // All 500 entries should be found: immediately or via deferred
        CHECK(found_immediate + successful.size() == 500);

        db.close();
    }
}

// =============================================================================
// Many entries per container with close/reopen
// =============================================================================

TEST_CASE("High fill: close/reopen near rotation point per container", "[storage][persistence]") {
    // Insert many entries of each container size (but not enough to rotate),
    // close, reopen, and verify all entries.
    struct Case {
        size_t value_size;
        size_t count;
    };

    // Counts chosen to be well under rotation point but substantial
    Case const cases[] = {
        {30,   50'000},  // 48B container
        {50,   25'000},  // 94B container
        {100,  15'000},  // 128B container
        {200,   5'000},  // 256B container
        {8000,    500},  // 10KB container
    };

    for (auto const& c : cases) {
        ScopedTestDir dir;

        // Phase 1: insert and close
        {
            utxoz::db db;
            db.configure_for_testing(dir.path, true);

            for (size_t i = 0; i < c.count; ++i) {
                auto key = make_test_key(static_cast<uint32_t>(i), 0);
                auto val = make_test_value(c.value_size, static_cast<uint8_t>(i & 0xFF));
                REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
            }

            CHECK(db.size() == c.count);
            db.close();
        }

        // Phase 2: reopen and verify all entries
        {
            utxoz::db db;
            db.configure_for_testing(dir.path, false);
            CHECK(db.size() == c.count);

            for (size_t i = 0; i < c.count; ++i) {
                auto key = make_test_key(static_cast<uint32_t>(i), 0);
                auto expected = make_test_value(c.value_size, static_cast<uint8_t>(i & 0xFF));
                auto result = db.find(key, 99999);
                REQUIRE(result.has_value());
                CHECK(*result == expected);
            }

            db.close();
        }
    }
}

// =============================================================================
// Rotation + close/reopen with full verification
// =============================================================================

TEST_CASE("Reopen after rotation: all entries verified across versions", "[storage][persistence][rotation]") {
    ScopedTestDir dir;

    constexpr size_t N = 200'000;
    std::vector<utxoz::raw_outpoint> keys;
    std::vector<std::vector<uint8_t>> values;
    keys.reserve(N);
    values.reserve(N);

    // Phase 1: insert enough to rotate, close
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);

        for (size_t i = 0; i < N; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
            auto val = make_test_value(30, static_cast<uint8_t>(i & 0xFF));
            keys.push_back(key);
            values.push_back(val);
            REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
        }

        auto stats = db.get_statistics();
        bool any_rotation = false;
        for (size_t i = 0; i < utxoz::container_count; ++i) {
            if (stats.rotations_per_container[i] > 0) any_rotation = true;
        }
        REQUIRE(any_rotation);

        db.close();
    }

    // Phase 2: reopen and verify entries from ALL versions (not just latest)
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);

        // Check latest entries (should be immediate)
        for (size_t i = N - 100; i < N; ++i) {
            auto result = db.find(keys[i], static_cast<uint32_t>(N));
            REQUIRE(result.has_value());
            CHECK(*result == values[i]);
        }

        // Check oldest entries (likely deferred to previous versions)
        size_t deferred_count = 0;
        for (size_t i = 0; i < 100; ++i) {
            auto result = db.find(keys[i], static_cast<uint32_t>(N));
            if (!result.has_value()) {
                ++deferred_count;
            } else {
                CHECK(*result == values[i]);
            }
        }

        // Process deferred lookups and verify values
        if (deferred_count > 0) {
            auto [successful, failed] = db.process_pending_lookups();
            CHECK(failed.empty());
            for (auto const& [key, found_value] : successful) {
                for (size_t i = 0; i < 100; ++i) {
                    if (keys[i] == key) {
                        CHECK(found_value == values[i]);
                        break;
                    }
                }
            }
        }

        db.close();
    }
}

// =============================================================================
// Insert duplicate key after reopen
// =============================================================================

TEST_CASE("Reopen: duplicate insert fails for existing entries", "[storage][persistence]") {
    ScopedTestDir dir;

    auto key = make_test_key(1, 0);
    auto val1 = make_test_value(30, 0xAA);
    auto val2 = make_test_value(30, 0xBB);

    // Phase 1: insert and close
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);
        REQUIRE(db.insert(key, val1, 100));
        db.close();
    }

    // Phase 2: reopen and try to insert same key
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);

        // Duplicate insert should fail
        CHECK_FALSE(db.insert(key, val2, 200));

        // Original value should be preserved
        auto result = db.find(key, 300);
        REQUIRE(result.has_value());
        CHECK(*result == val1);

        db.close();
    }
}

// =============================================================================
// Mixed container sizes with close/reopen
// =============================================================================

TEST_CASE("Reopen: mixed container sizes with many entries each", "[storage][persistence]") {
    ScopedTestDir dir;

    struct EntryInfo {
        utxoz::raw_outpoint key;
        std::vector<uint8_t> value;
    };
    std::vector<EntryInfo> all_entries;

    // Insert entries across all 5 containers
    // Capacities: 43, 89, 123, 250, 10234
    std::array<size_t, 5> value_sizes = {30, 50, 100, 200, 8000};
    constexpr size_t entries_per_size = 50;

    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);

        uint32_t id = 0;
        for (size_t vs : value_sizes) {
            for (size_t i = 0; i < entries_per_size; ++i) {
                auto key = make_test_key(id, 0);
                auto val = make_test_value(vs, static_cast<uint8_t>(id & 0xFF));
                all_entries.push_back({key, val});
                REQUIRE(db.insert(key, val, id + 100));
                ++id;
            }
        }

        CHECK(db.size() == value_sizes.size() * entries_per_size);
        db.close();
    }

    // Reopen and verify every single entry
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);

        CHECK(db.size() == all_entries.size());

        for (auto const& [key, expected] : all_entries) {
            auto result = db.find(key, 99999);
            REQUIRE(result.has_value());
            CHECK(*result == expected);
        }

        db.close();
    }
}

// =============================================================================
// Size consistency across close/reopen
// =============================================================================

TEST_CASE("Reopen: size() is consistent across close/reopen cycles", "[storage][persistence]") {
    ScopedTestDir dir;

    // Cycle 1: insert 100
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);
        for (size_t i = 0; i < 100; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), 0);
            auto val = make_test_value(30);
            REQUIRE(db.insert(key, val, 100));
        }
        CHECK(db.size() == 100);
        db.close();
    }

    // Cycle 2: reopen, verify size, insert 50 more, erase 25
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);
        CHECK(db.size() == 100);

        for (size_t i = 100; i < 150; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), 0);
            auto val = make_test_value(30);
            REQUIRE(db.insert(key, val, 200));
        }
        CHECK(db.size() == 150);

        for (size_t i = 0; i < 25; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), 0);
            CHECK(db.erase(key, 300) == 1);
        }
        CHECK(db.size() == 125);

        db.close();
    }

    // Cycle 3: reopen, verify final size
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);
        CHECK(db.size() == 125);
        db.close();
    }
}

// =============================================================================
// Compaction with multiple version files (regression tests)
// =============================================================================

TEST_CASE("Compaction: survives 3+ rotations without crash", "[storage][compaction][regression]") {
    // Container 4 (10KB values) is used for speed: ~840 entries per rotation.
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // Insert enough 1000-byte values (-> container 4, 10KB) to cause 3+ rotations.
    // ~840 entries per rotation, so ~3400 entries gives us 4 files.
    constexpr size_t N = 3400;
    std::vector<utxoz::raw_outpoint> keys;
    keys.reserve(N);

    for (size_t i = 0; i < N; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
        keys.push_back(key);
        auto val = make_test_value(1000, static_cast<uint8_t>(i & 0xFF));
        REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
    }

    // Verify we have multiple rotations (4+ files for container 4, the 10KB container)
    auto stats = db.get_statistics();
    REQUIRE(stats.rotations_per_container[4] >= 3);

    db.compact_all();

    // Verify data integrity after compaction: sample entries from the latest version
    size_t found = 0;
    for (size_t i = N - 100; i < N; ++i) {
        auto result = db.find(keys[i], static_cast<uint32_t>(N + 1));
        if (result.has_value()) ++found;
    }

    auto [successful, failed] = db.process_pending_lookups();
    CHECK(found + successful.size() == 100);
    CHECK(failed.empty());

    db.close();
}

TEST_CASE("Compaction: data integrity with many versions + close/reopen", "[storage][compaction][persistence][regression]") {
    ScopedTestDir dir;

    constexpr size_t N = 3400;
    std::vector<utxoz::raw_outpoint> keys;
    keys.reserve(N);

    // Phase 1: insert, compact, close
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);

        for (size_t i = 0; i < N; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
            keys.push_back(key);
            auto val = make_test_value(1000, static_cast<uint8_t>(i & 0xFF));
            REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
        }

        auto stats = db.get_statistics();
        REQUIRE(stats.rotations_per_container[4] >= 3);

        db.compact_all();
        db.close();
    }

    // Phase 2: reopen and verify all entries
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);

        // Check recent entries (should be in latest version)
        size_t found_immediate = 0;
        for (size_t i = N - 200; i < N; ++i) {
            auto expected = make_test_value(1000, static_cast<uint8_t>(i & 0xFF));
            auto result = db.find(keys[i], static_cast<uint32_t>(N + 1));
            if (result.has_value()) {
                CHECK(*result == expected);
                ++found_immediate;
            }
        }

        auto [successful, failed] = db.process_pending_lookups();
        CHECK(found_immediate + successful.size() == 200);
        CHECK(failed.empty());

        db.close();
    }
}

TEST_CASE("Compaction: with deletions across multiple versions", "[storage][compaction][regression]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    constexpr size_t N = 3400;
    std::vector<utxoz::raw_outpoint> keys;
    keys.reserve(N);

    for (size_t i = 0; i < N; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
        keys.push_back(key);
        auto val = make_test_value(1000, static_cast<uint8_t>(i & 0xFF));
        REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
    }

    // Erase a large portion of entries to create sparse older files
    for (size_t i = 0; i < N / 2; ++i) {
        (void)db.erase(keys[i], static_cast<uint32_t>(N + 1));
    }
    (void)db.process_pending_deletions();

    // Compact — with many deletions, some source files may become empty
    db.compact_all();

    // Verify surviving entries
    size_t found = 0;
    for (size_t i = N / 2; i < N; ++i) {
        auto result = db.find(keys[i], static_cast<uint32_t>(N + 2));
        if (result.has_value()) ++found;
    }
    auto [successful, failed] = db.process_pending_lookups();
    CHECK(found + successful.size() == N / 2);
    CHECK(failed.empty());

    // Verify deleted entries are gone
    for (size_t i = 0; i < 100; ++i) {
        auto result = db.find(keys[i], static_cast<uint32_t>(N + 2));
        CHECK_FALSE(result.has_value());
    }
    auto [found_deleted, failed_deleted] = db.process_pending_lookups();
    CHECK(found_deleted.empty());

    db.close();
}

TEST_CASE("Compaction: size() is correct after compaction", "[storage][compaction][regression]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    constexpr size_t N = 3400;
    for (size_t i = 0; i < N; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
        auto val = make_test_value(1000, static_cast<uint8_t>(i & 0xFF));
        REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
    }

    size_t size_before = db.size();
    CHECK(size_before == N);

    db.compact_all();

    // size() should remain the same — compaction moves entries, doesn't add or remove them
    CHECK(db.size() == size_before);

    db.close();
}

// =============================================================================
// Metadata persistence
// =============================================================================

TEST_CASE("Metadata: files are created on disk after close", "[storage][metadata]") {
    ScopedTestDir dir;

    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);

        for (size_t i = 0; i < 100; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), 0);
            auto val = make_test_value(30, static_cast<uint8_t>(i));
            REQUIRE(db.insert(key, val, static_cast<uint32_t>(i + 100)));
        }

        db.close();
    }

    // Metadata file for container 0, version 0 should exist
    auto meta_path = std::filesystem::path(dir.path) / "meta_0_v00000.dat";
    CHECK(std::filesystem::exists(meta_path));
    CHECK(std::filesystem::file_size(meta_path) > 0);
}

TEST_CASE("Metadata: files created for all versions on rotation", "[storage][metadata]") {
    ScopedTestDir dir;

    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);

        // Insert enough to cause rotation in container 0
        constexpr size_t N = 200'000;
        for (size_t i = 0; i < N; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
            auto val = make_test_value(30, static_cast<uint8_t>(i & 0xFF));
            REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
        }

        auto stats = db.get_statistics();
        REQUIRE(stats.rotations_per_container[0] > 0);

        db.close();
    }

    // Metadata files should exist for both version 0 and version 1
    CHECK(std::filesystem::exists(std::filesystem::path(dir.path) / "meta_0_v00000.dat"));
    CHECK(std::filesystem::exists(std::filesystem::path(dir.path) / "meta_0_v00001.dat"));
}

TEST_CASE("Metadata: rebuilt correctly after compaction", "[storage][metadata][compaction]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // Insert enough 1000-byte values (-> container 4, 10KB) to cause 3+ rotations.
    constexpr size_t N = 3400;
    uint32_t min_height = 1000;
    uint32_t max_height = min_height + N - 1;

    for (size_t i = 0; i < N; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
        auto val = make_test_value(1000, static_cast<uint8_t>(i & 0xFF));
        REQUIRE(db.insert(key, val, static_cast<uint32_t>(min_height + i)));
    }

    auto stats = db.get_statistics();
    size_t container_idx = utxoz::container_count - 1; // 10KB container
    REQUIRE(stats.rotations_per_container[container_idx] >= 3);

    // Count metadata files before compaction
    size_t meta_before = 0;
    for (size_t v = 0; v < 20; ++v) {
        auto meta_path = std::filesystem::path(dir.path) /
            fmt::format("meta_{}_v{:05}.dat", container_idx, v);
        if (std::filesystem::exists(meta_path)) ++meta_before;
    }
    REQUIRE(meta_before >= 4); // at least 4 files (3 rotations + 1)

    db.compact_all();

    // Count data files and metadata files after compaction - they must match
    size_t data_after = 0;
    size_t meta_after = 0;
    for (size_t v = 0; v < 20; ++v) {
        auto data_path = std::filesystem::path(dir.path) /
            fmt::format("cont_{}_v{:05}.dat", container_idx, v);
        auto meta_path = std::filesystem::path(dir.path) /
            fmt::format("meta_{}_v{:05}.dat", container_idx, v);
        if (std::filesystem::exists(data_path)) ++data_after;
        if (std::filesystem::exists(meta_path)) ++meta_after;
    }

    CHECK(data_after > 0);
    CHECK(meta_after == data_after);

    // No orphaned metadata files beyond the data file range
    for (size_t v = data_after; v < 20; ++v) {
        auto meta_path = std::filesystem::path(dir.path) /
            fmt::format("meta_{}_v{:05}.dat", container_idx, v);
        CHECK_FALSE(std::filesystem::exists(meta_path));
    }

    // Verify metadata is usable after close + reopen
    db.close();

    utxoz::db db2;
    db2.configure_for_testing(dir.path, false);

    // All entries should still be findable
    size_t found = 0;
    for (size_t i = 0; i < N; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
        auto result = db2.find(key, max_height + 1);
        if (result.has_value()) ++found;
    }

    auto [successful, failed] = db2.process_pending_lookups();
    CHECK(found + successful.size() == N);
    CHECK(failed.empty());

    db2.close();
}

// =============================================================================
// No-truncation: values up to container_sizes[i] must round-trip exactly
// =============================================================================

TEST_CASE("No truncation: P2PKH-sized values (43 bytes) survive round-trip", "[storage][truncation]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // 43 bytes is the most common UTXO value size on BCH (P2PKH outputs).
    // It should fit in container 0 (max 48 bytes, capacity 43) without any truncation.
    auto key = make_test_key(1, 0);
    auto val = make_test_value(43, 0xAB);
    REQUIRE(db.insert(key, val, 100));

    auto result = db.find(key, 200);
    REQUIRE(result.has_value());
    CHECK(result->size() == 43);
    CHECK(*result == val);

    db.close();
}

TEST_CASE("No truncation: P2SH-sized values (41 bytes) survive round-trip", "[storage][truncation]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // 41 bytes is the second most common size (P2SH outputs, 13.3% of chain).
    auto key = make_test_key(1, 0);
    auto val = make_test_value(41, 0xCD);
    REQUIRE(db.insert(key, val, 100));

    auto result = db.find(key, 200);
    REQUIRE(result.has_value());
    CHECK(result->size() == 41);
    CHECK(*result == val);

    db.close();
}

TEST_CASE("No truncation: max value for each container survives round-trip", "[storage][truncation]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // For each container, insert a value at exactly its data capacity.
    // This is the maximum value size that should fit without truncation.
    for (size_t i = 0; i < utxoz::container_sizes.size(); ++i) {
        size_t val_size = utxoz::container_capacities[i];
        auto key = make_test_key(static_cast<uint32_t>(i + 1), 0);
        auto val = make_test_value(val_size, static_cast<uint8_t>(i * 37));

        INFO("container " << i << ", capacity = " << val_size);
        REQUIRE(db.insert(key, val, static_cast<uint32_t>(100 + i)));

        auto result = db.find(key, 200);
        REQUIRE(result.has_value());
        REQUIRE(result->size() == val_size);
        CHECK(*result == val);
    }

    db.close();
}

TEST_CASE("No truncation: boundary values at each container capacity", "[storage][truncation]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // Test sizes around each container's data capacity boundary.
    // Values at capacity should fit exactly; values below should have room to spare.
    std::vector<size_t> boundary_sizes;
    for (size_t cap : utxoz::container_capacities) {
        for (size_t delta = 0; delta <= 5 && delta < cap; ++delta) {
            boundary_sizes.push_back(cap - delta);
        }
    }

    for (size_t i = 0; i < boundary_sizes.size(); ++i) {
        size_t val_size = boundary_sizes[i];
        auto key = make_test_key(static_cast<uint32_t>(i + 1), 0);
        auto val = make_test_value(val_size, static_cast<uint8_t>(i * 13));

        INFO("value size = " << val_size);
        REQUIRE(db.insert(key, val, static_cast<uint32_t>(100 + i)));

        auto result = db.find(key, 200);
        REQUIRE(result.has_value());
        REQUIRE(result->size() == val_size);
        CHECK(*result == val);
    }

    db.close();
}

// =============================================================================
// Sizing report
// =============================================================================

TEST_CASE("Sizing report: histogram and waste calculations are correct", "[storage][sizing]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // Insert values of different sizes into different containers.
    // Routing uses container_capacities (43, 89, 123, 250, 10234), not container_sizes.
    //
    // Container 0 capacity: 43B  (container_size 48 - 5 overhead)
    // Container 1 capacity: 89B  (container_size 94 - 5 overhead)
    // Container 2 capacity: 123B (container_size 128 - 5 overhead)

    // 10 entries of 30 bytes -> container 0 (cap 43), waste = (48-30)*10 = 180
    for (size_t i = 0; i < 10; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        auto val = make_test_value(30, static_cast<uint8_t>(i));
        REQUIRE(db.insert(key, val, 100));
    }

    // 5 entries of 50 bytes -> container 1 (50 > cap 43), waste = (94-50)*5 = 220
    for (size_t i = 10; i < 15; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        auto val = make_test_value(50, static_cast<uint8_t>(i));
        REQUIRE(db.insert(key, val, 100));
    }

    // 3 entries of 100 bytes -> container 2 (100 > cap 89), waste = (128-100)*3 = 84
    for (size_t i = 15; i < 18; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        auto val = make_test_value(100, static_cast<uint8_t>(i));
        REQUIRE(db.insert(key, val, 100));
    }

    auto report = db.get_sizing_report();

    // Container 0: only the 30-byte entries (10 entries)
    CHECK(report.containers[0].container_size == 48);
    CHECK(report.containers[0].historical_inserts == 10);
    CHECK(report.containers[0].current_entries == 10);
    CHECK(report.containers[0].total_wasted_bytes == 180);
    CHECK(report.containers[0].avg_waste_per_entry == Catch::Approx(18.0));

    // Container 1: 50-byte entries (5 entries)
    CHECK(report.containers[1].container_size == 94);
    CHECK(report.containers[1].historical_inserts == 5);
    CHECK(report.containers[1].current_entries == 5);
    CHECK(report.containers[1].total_wasted_bytes == 220);
    CHECK(report.containers[1].avg_waste_per_entry == Catch::Approx(44.0));

    // Container 2: 100-byte entries (3 entries)
    CHECK(report.containers[2].container_size == 128);
    CHECK(report.containers[2].historical_inserts == 3);
    CHECK(report.containers[2].current_entries == 3);
    CHECK(report.containers[2].total_wasted_bytes == 84);
    CHECK(report.containers[2].avg_waste_per_entry == Catch::Approx(28.0));

    // Container 3 and 4: no inserts
    CHECK(report.containers[3].historical_inserts == 0);
    CHECK(report.containers[4].historical_inserts == 0);

    // Global histogram checks
    CHECK(report.global_value_size_histogram.size() == 3);  // 30, 50, 100
    CHECK(report.global_value_size_histogram[30] == 10);
    CHECK(report.global_value_size_histogram[50] == 5);
    CHECK(report.global_value_size_histogram[100] == 3);

    // file_count should be at least 1 for all containers
    for (size_t i = 0; i < utxoz::container_count; ++i) {
        CHECK(report.containers[i].file_count >= 1);
    }

    // Erase some entries and verify deletes are tracked (keys 0-4 are 30B -> container 0)
    for (size_t i = 0; i < 5; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        (void)db.erase(key, 200);
    }

    auto report2 = db.get_sizing_report();
    CHECK(report2.containers[0].historical_deletes == 5);
    CHECK(report2.containers[0].current_entries == 5);

    // print_sizing_report should not crash
    db.print_sizing_report();

    db.close();
}

TEST_CASE("Sizing report: empty database returns zeroed report", "[storage][sizing]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    auto report = db.get_sizing_report();

    for (size_t i = 0; i < utxoz::container_count; ++i) {
        CHECK(report.containers[i].container_size == utxoz::container_sizes[i]);
        CHECK(report.containers[i].historical_inserts == 0);
        CHECK(report.containers[i].current_entries == 0);
        CHECK(report.containers[i].total_wasted_bytes == 0);
        CHECK(report.containers[i].avg_waste_per_entry == 0.0);
    }

    CHECK(report.global_value_size_histogram.empty());

    db.close();
}

TEST_CASE("Sizing report: unconfigured database returns zeroed report", "[storage][sizing]") {
    utxoz::db db;
    auto report = db.get_sizing_report();

    for (size_t i = 0; i < utxoz::container_count; ++i) {
        CHECK(report.containers[i].historical_inserts == 0);
        CHECK(report.containers[i].current_entries == 0);
    }

    CHECK(report.global_value_size_histogram.empty());
}

// =============================================================================
// Metadata persistence
// =============================================================================

TEST_CASE("Metadata: key ranges are correct after reopen", "[storage][metadata]") {
    ScopedTestDir dir;

    constexpr size_t N = 200'000;
    std::vector<utxoz::raw_outpoint> keys;
    keys.reserve(N);

    // Phase 1: insert enough to rotate, close
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, true);

        for (size_t i = 0; i < N; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
            keys.push_back(key);
            auto val = make_test_value(30, static_cast<uint8_t>(i & 0xFF));
            REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
        }

        auto stats = db.get_statistics();
        REQUIRE(stats.rotations_per_container[0] > 0);
        db.close();
    }

    // Phase 2: reopen — metadata should be loaded from disk.
    // Without metadata, find_in_prev_versions searches all files (works but slow).
    // With metadata, it can skip files where the key is out of range.
    // We verify correctness: entries in old versions should still be found.
    {
        utxoz::db db;
        db.configure_for_testing(dir.path, false);

        // Look up entries that are likely in version 0 (earliest inserts)
        size_t deferred = 0;
        for (size_t i = 0; i < 200; ++i) {
            auto result = db.find(keys[i], static_cast<uint32_t>(N + 1));
            if (!result.has_value()) ++deferred;
        }

        auto [successful, failed] = db.process_pending_lookups();
        // All should be found — either immediately or via deferred
        CHECK(failed.empty());
        CHECK((deferred == 0 || !successful.empty()));

        db.close();
    }
}

// =============================================================================
// for_each_key
// =============================================================================

TEST_CASE("for_each_key: visits all keys exactly once", "[storage][iteration]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    constexpr size_t N = 500;
    std::set<utxoz::raw_outpoint> inserted_keys;

    for (size_t i = 0; i < N; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        auto val = make_test_value(30, static_cast<uint8_t>(i & 0xFF));
        REQUIRE(db.insert(key, val, static_cast<uint32_t>(i + 100)));
        inserted_keys.insert(key);
    }

    std::set<utxoz::raw_outpoint> visited_keys;
    db.for_each_key([&](utxoz::raw_outpoint const& key) {
        visited_keys.insert(key);
    });

    CHECK(visited_keys.size() == N);
    CHECK(visited_keys == inserted_keys);

    db.close();
}

TEST_CASE("for_each_key: visits keys across all container sizes", "[storage][iteration]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // Capacities: 43, 89, 123, 250, 10234
    std::array<size_t, 5> value_sizes = {30, 50, 100, 200, 8000};
    std::set<utxoz::raw_outpoint> inserted_keys;

    uint32_t id = 0;
    for (size_t vs : value_sizes) {
        for (size_t i = 0; i < 10; ++i) {
            auto key = make_test_key(id, 0);
            auto val = make_test_value(vs, static_cast<uint8_t>(id));
            REQUIRE(db.insert(key, val, id + 100));
            inserted_keys.insert(key);
            ++id;
        }
    }

    std::set<utxoz::raw_outpoint> visited_keys;
    db.for_each_key([&](utxoz::raw_outpoint const& key) {
        visited_keys.insert(key);
    });

    CHECK(visited_keys.size() == 50);
    CHECK(visited_keys == inserted_keys);

    db.close();
}

TEST_CASE("for_each_key: visits keys in previous versions after rotation", "[storage][iteration][rotation]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // Insert enough to trigger rotation
    constexpr size_t N = 200'000;
    std::set<utxoz::raw_outpoint> inserted_keys;

    for (size_t i = 0; i < N; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i >> 16));
        auto val = make_test_value(30, static_cast<uint8_t>(i & 0xFF));
        REQUIRE(db.insert(key, val, static_cast<uint32_t>(i)));
        inserted_keys.insert(key);
    }

    // Confirm rotation happened
    auto stats = db.get_statistics();
    bool any_rotation = false;
    for (size_t i = 0; i < utxoz::container_count; ++i) {
        if (stats.rotations_per_container[i] > 0) any_rotation = true;
    }
    REQUIRE(any_rotation);

    // for_each_key should visit ALL keys, including those in old versions
    size_t count = 0;
    db.for_each_key([&](utxoz::raw_outpoint const& key) {
        ++count;
        CHECK(inserted_keys.contains(key));
    });

    CHECK(count == N);

    db.close();
}

TEST_CASE("for_each_key: empty database visits nothing", "[storage][iteration][edge]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    size_t count = 0;
    db.for_each_key([&](utxoz::raw_outpoint const&) {
        ++count;
    });

    CHECK(count == 0);

    db.close();
}

TEST_CASE("for_each_key: skips erased entries", "[storage][iteration]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    constexpr size_t N = 200;
    std::vector<utxoz::raw_outpoint> keys;

    for (size_t i = 0; i < N; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        keys.push_back(key);
        auto val = make_test_value(30, static_cast<uint8_t>(i));
        REQUIRE(db.insert(key, val, 100));
    }

    // Erase first half
    for (size_t i = 0; i < N / 2; ++i) {
        CHECK(db.erase(keys[i], 200) == 1);
    }

    size_t count = 0;
    db.for_each_key([&](utxoz::raw_outpoint const&) {
        ++count;
    });

    CHECK(count == N / 2);

    db.close();
}

// =============================================================================
// for_each_entry tests
// =============================================================================

TEST_CASE("for_each_entry: visits all entries with correct key, height and data", "[storage][iteration]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    constexpr size_t N = 500;
    std::map<utxoz::raw_outpoint, std::pair<uint32_t, std::vector<uint8_t>>> expected;

    for (size_t i = 0; i < N; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        auto val = make_test_value(30, static_cast<uint8_t>(i));
        uint32_t height = static_cast<uint32_t>(1000 + i);
        REQUIRE(db.insert(key, val, height));
        expected[key] = {height, {val.begin(), val.end()}};
    }

    size_t count = 0;
    db.for_each_entry([&](utxoz::raw_outpoint const& key, uint32_t height, std::span<uint8_t const> data) {
        auto it = expected.find(key);
        REQUIRE(it != expected.end());
        CHECK(height == it->second.first);
        CHECK(std::vector<uint8_t>(data.begin(), data.end()) == it->second.second);
        ++count;
    });

    CHECK(count == N);

    db.close();
}

TEST_CASE("for_each_entry: visits entries across all container sizes", "[storage][iteration]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    // One entry per container size
    std::array<size_t, 5> value_sizes = {30, 50, 100, 200, 8000};
    std::map<utxoz::raw_outpoint, std::pair<uint32_t, size_t>> expected; // key -> (height, data_size)

    for (size_t c = 0; c < value_sizes.size(); ++c) {
        for (size_t i = 0; i < 10; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(c * 100 + i), 0);
            auto val = make_test_value(value_sizes[c], static_cast<uint8_t>(c));
            uint32_t height = static_cast<uint32_t>(c * 1000 + i);
            REQUIRE(db.insert(key, val, height));
            expected[key] = {height, value_sizes[c]};
        }
    }

    size_t count = 0;
    db.for_each_entry([&](utxoz::raw_outpoint const& key, uint32_t height, std::span<uint8_t const> data) {
        auto it = expected.find(key);
        REQUIRE(it != expected.end());
        CHECK(height == it->second.first);
        CHECK(data.size() == it->second.second);
        ++count;
    });

    CHECK(count == 50);

    db.close();
}

TEST_CASE("for_each_entry: empty database visits nothing", "[storage][iteration][edge]") {
    ScopedTestDir dir;

    utxoz::db db;
    db.configure_for_testing(dir.path, true);

    size_t count = 0;
    db.for_each_entry([&](utxoz::raw_outpoint const&, uint32_t, std::span<uint8_t const>) {
        ++count;
    });

    CHECK(count == 0);

    db.close();
}
