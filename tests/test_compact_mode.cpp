// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_compact_mode.cpp
 * @brief Tests for compact storage mode using compact_db
 */

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <numeric>
#include <random>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>
#include <utxoz/utils.hpp>

namespace {

inline std::atomic<uint64_t> compact_test_counter{0};

utxoz::raw_outpoint make_test_key(uint32_t tx_id, uint32_t output_index) {
    utxoz::raw_outpoint key{};
    for (size_t i = 0; i < 32; ++i) {
        key[i] = static_cast<uint8_t>((tx_id >> (i % 4 * 8)) & 0xFF);
    }
    std::memcpy(key.data() + 32, &output_index, 4);
    return key;
}

struct CompactFixture {
    CompactFixture() {
        auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto unique_id = compact_test_counter.fetch_add(1);
        test_path_ = fmt::format("./test_compact_db_{}_{}_{}", getpid(), ts, unique_id);

        if (std::filesystem::exists(test_path_)) {
            std::filesystem::remove_all(test_path_);
        }
        db_.configure_for_testing(test_path_, true);
    }

    ~CompactFixture() {
        db_.close();
        if (std::filesystem::exists(test_path_)) {
            std::filesystem::remove_all(test_path_);
        }
    }

    utxoz::compact_db db_;
    std::string test_path_;
};

// Helper to create a fresh path without auto-configuring
std::string make_fresh_path() {
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto unique_id = compact_test_counter.fetch_add(1);
    auto path = fmt::format("./test_compact_db_{}_{}_{}", getpid(), ts, unique_id);
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
    return path;
}

} // anonymous namespace

TEST_CASE_METHOD(CompactFixture, "Compact: basic insert and find", "[compact]") {
    auto key = make_test_key(1, 0);
    uint32_t file_number = 42;
    uint32_t offset = 12345;
    uint32_t height = 100;

    CHECK(db_.insert(key, file_number, offset, height));
    CHECK(db_.size() == 1);

    auto result = db_.find(key, height);
    REQUIRE(result.has_value());
    CHECK(result->file_number == file_number);
    CHECK(result->offset == offset);
    CHECK(result->block_height == height);

    // Duplicate insert should fail
    CHECK_FALSE(db_.insert(key, file_number, offset, height));
    CHECK(db_.size() == 1);
}

TEST_CASE_METHOD(CompactFixture, "Compact: multiple inserts with typed fields", "[compact]") {
    for (int i = 0; i < 10; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        uint32_t file_number = static_cast<uint32_t>(i + 1);
        uint32_t offset = static_cast<uint32_t>(i * 1000);
        CHECK(db_.insert(key, file_number, offset, 100));

        auto result = db_.find(key, 100);
        REQUIRE(result.has_value());
        CHECK(result->file_number == file_number);
        CHECK(result->offset == offset);
    }
    CHECK(db_.size() == 10);
}

TEST_CASE_METHOD(CompactFixture, "Compact: erase operations", "[compact]") {
    auto key = make_test_key(1, 0);
    uint32_t height = 100;

    CHECK(db_.insert(key, 1, 100, height));
    CHECK(db_.size() == 1);

    CHECK(db_.erase(key, height) == 1);

    auto result = db_.find(key, height);
    CHECK_FALSE(result.has_value());

    // Erase non-existent key
    auto key2 = make_test_key(2, 0);
    CHECK(db_.erase(key2, height) == 0);
}

TEST_CASE("Compact: close and reopen", "[compact]") {
    auto path = make_fresh_path();

    // Insert data
    {
        utxoz::compact_db db;
        db.configure_for_testing(path, true);

        for (int i = 0; i < 50; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), 0);
            CHECK(db.insert(key, static_cast<uint32_t>(i + 1), static_cast<uint32_t>(i * 100), 100));
        }
        CHECK(db.size() == 50);
        db.close();
    }

    // Reopen and verify
    {
        utxoz::compact_db db;
        db.configure_for_testing(path, false);
        CHECK(db.size() == 50);

        for (int i = 0; i < 50; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), 0);
            auto result = db.find(key, 200);
            REQUIRE(result.has_value());
            CHECK(result->file_number == static_cast<uint32_t>(i + 1));
            CHECK(result->offset == static_cast<uint32_t>(i * 100));
        }
        db.close();
    }

    std::filesystem::remove_all(path);
}

TEST_CASE("Compact: mode mismatch detection", "[compact]") {
    auto path = make_fresh_path();

    // Create as compact
    {
        utxoz::compact_db db;
        db.configure_for_testing(path, true);
        auto key = make_test_key(1, 0);
        db.insert(key, 1, 100, 100);
        db.close();
    }

    // Try to open as full - should throw
    {
        utxoz::full_db db;
        CHECK_THROWS_AS(
            db.configure_for_testing(path, false),
            std::runtime_error);
    }

    // Create as full in another path
    auto path2 = make_fresh_path();
    {
        utxoz::full_db db;
        db.configure_for_testing(path2, true);
        auto key = make_test_key(1, 0);
        auto value = std::vector<uint8_t>(30, 0x42);
        db.insert(key, value, 100);
        db.close();
    }

    // Try to open as compact - should throw
    {
        utxoz::compact_db db;
        CHECK_THROWS_AS(
            db.configure_for_testing(path2, false),
            std::runtime_error);
    }

    std::filesystem::remove_all(path);
    std::filesystem::remove_all(path2);
}

TEST_CASE_METHOD(CompactFixture, "Compact: deferred deletions", "[compact]") {
    std::vector<utxoz::raw_outpoint> keys;
    for (int i = 0; i < 10; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        keys.push_back(key);
        CHECK(db_.insert(key, static_cast<uint32_t>(i), static_cast<uint32_t>(i * 10), 100));
    }

    CHECK(db_.size() == 10);

    // Erase some - should find them in latest version
    for (int i = 0; i < 5; ++i) {
        CHECK(db_.erase(keys[i], 200) == 1);
    }

    auto [deleted, failed] = db_.process_pending_deletions();
    CHECK(failed.size() == 0);

    for (int i = 0; i < 5; ++i) {
        auto result = db_.find(keys[i], 200);
        CHECK_FALSE(result.has_value());
    }

    for (int i = 5; i < 10; ++i) {
        auto result = db_.find(keys[i], 200);
        CHECK(result.has_value());
    }
}

TEST_CASE_METHOD(CompactFixture, "Compact: for_each_key", "[compact]") {
    for (int i = 0; i < 20; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        db_.insert(key, static_cast<uint32_t>(i), 0, 100);
    }

    size_t count = 0;
    db_.for_each_key([&](utxoz::raw_outpoint const&) {
        ++count;
    });

    CHECK(count == 20);
}

TEST_CASE_METHOD(CompactFixture, "Compact: for_each_entry", "[compact]") {
    for (int i = 0; i < 20; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        db_.insert(key, static_cast<uint32_t>(i + 1), static_cast<uint32_t>(i * 100),
                   static_cast<uint32_t>(100 + i));
    }

    size_t count = 0;
    db_.for_each_entry([&](utxoz::raw_outpoint const&, uint32_t height,
                           uint32_t file_number, uint32_t offset) {
        CHECK(height >= 100);
        CHECK(file_number >= 1);
        ++count;
    });

    CHECK(count == 20);
}

TEST_CASE_METHOD(CompactFixture, "Compact: statistics", "[compact]") {
    for (int i = 0; i < 50; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        db_.insert(key, static_cast<uint32_t>(i), 0, static_cast<uint32_t>(100 + i));
    }

    auto stats = db_.get_statistics();
    CHECK(stats.mode == utxoz::storage_mode::compact);
    CHECK(stats.total_entries == 50);
    CHECK(stats.total_inserts >= 50);
}

TEST_CASE_METHOD(CompactFixture, "Compact: compaction", "[compact]") {
    // Insert enough data to verify compaction works
    for (int i = 0; i < 100; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        db_.insert(key, static_cast<uint32_t>(i), 0, 100);
    }

    // Erase half
    for (int i = 0; i < 50; ++i) {
        db_.erase(make_test_key(static_cast<uint32_t>(i), 0), 200);
    }
    db_.process_pending_deletions();

    // Compact
    db_.compact_all();

    // Verify remaining entries
    for (int i = 50; i < 100; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        auto result = db_.find(key, 300);
        CHECK(result.has_value());
    }
}

TEST_CASE("Compact: file naming uses compact_v prefix", "[compact]") {
    auto path = make_fresh_path();

    {
        utxoz::compact_db db;
        db.configure_for_testing(path, true);
        auto key = make_test_key(1, 0);
        db.insert(key, 1, 100, 100);
        db.close();
    }

    // Verify compact file exists
    CHECK(std::filesystem::exists(path + "/compact_v00000.dat"));

    // Verify no cont_ files exist
    bool has_cont_files = false;
    for (auto const& entry : std::filesystem::directory_iterator(path)) {
        auto filename = entry.path().filename().string();
        if (filename.starts_with("cont_")) {
            has_cont_files = true;
        }
    }
    CHECK_FALSE(has_cont_files);

    std::filesystem::remove_all(path);
}

TEST_CASE("Compact: config file is created", "[compact]") {
    auto path = make_fresh_path();

    {
        utxoz::compact_db db;
        db.configure_for_testing(path, true);
        db.close();
    }

    CHECK(std::filesystem::exists(path + "/utxoz_config.dat"));

    std::filesystem::remove_all(path);
}
