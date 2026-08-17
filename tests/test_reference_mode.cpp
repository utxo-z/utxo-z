// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_reference_mode.cpp
 * @brief Tests for reference storage mode using reference_db
 */

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
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

inline std::atomic<uint64_t> reference_test_counter{0};

utxoz::raw_outpoint make_test_key(uint32_t tx_id, uint32_t output_index) {
    utxoz::raw_outpoint key{};
    for (size_t i = 0; i < 32; ++i) {
        key[i] = static_cast<uint8_t>((tx_id >> (i % 4 * 8)) & 0xFF);
    }
    // Little-endian encoding of output_index
    key[32] = static_cast<uint8_t>(output_index);
    key[33] = static_cast<uint8_t>(output_index >> 8);
    key[34] = static_cast<uint8_t>(output_index >> 16);
    key[35] = static_cast<uint8_t>(output_index >> 24);
    return key;
}

struct ReferenceFixture {
    ReferenceFixture() {
        auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto unique_id = reference_test_counter.fetch_add(1);
        test_path_ = fmt::format("./test_reference_db_{}_{}_{}", getpid(), ts, unique_id);

        if (std::filesystem::exists(test_path_)) {
            std::filesystem::remove_all(test_path_);
        }
        auto r = utxoz::reference_db::open_for_testing(test_path_, true);
        if (!r) throw std::runtime_error("Failed to open test database");
        db_.emplace(std::move(*r));
    }

    ~ReferenceFixture() {
        db_.reset();
        if (std::filesystem::exists(test_path_)) {
            std::filesystem::remove_all(test_path_);
        }
    }

    std::optional<utxoz::reference_db> db_;
    std::string test_path_;
};

// Helper to create a fresh path without auto-configuring
std::string make_fresh_path() {
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto unique_id = reference_test_counter.fetch_add(1);
    auto path = fmt::format("./test_reference_db_{}_{}_{}", getpid(), ts, unique_id);
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
    return path;
}

} // anonymous namespace

TEST_CASE_METHOD(ReferenceFixture, "Reference: basic insert and find", "[reference]") {
    auto key = make_test_key(1, 0);
    uint32_t file_number = 42;
    uint32_t offset = 12345;
    uint32_t height = 100;

    CHECK(db_->insert(key, file_number, offset, height).value());
    CHECK(db_->size() == 1);

    auto result = db_->find(key, height);
    REQUIRE(result.has_value());
    CHECK(result->file_number == file_number);
    CHECK(result->offset == offset);
    CHECK(result->block_height == height);

    // Duplicate insert should fail
    CHECK_FALSE(db_->insert(key, file_number, offset, height).value());
    CHECK(db_->size() == 1);
}

TEST_CASE_METHOD(ReferenceFixture, "Reference: multiple inserts with typed fields", "[reference]") {
    for (int i = 0; i < 10; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        uint32_t file_number = static_cast<uint32_t>(i + 1);
        uint32_t offset = static_cast<uint32_t>(i * 1000);
        CHECK(db_->insert(key, file_number, offset, 100).value());

        auto result = db_->find(key, 100);
        REQUIRE(result.has_value());
        CHECK(result->file_number == file_number);
        CHECK(result->offset == offset);
    }
    CHECK(db_->size() == 10);
}

TEST_CASE_METHOD(ReferenceFixture, "Reference: erase operations", "[reference]") {
    auto key = make_test_key(1, 0);
    uint32_t height = 100;

    CHECK(db_->insert(key, 1, 100, height).value());
    CHECK(db_->size() == 1);

    // Stored and in the active version, so the batch applies it outright.
    auto const erased = db_->apply_deletes(std::vector<utxoz::deferred_deletion_entry>{{key, height}});
    CHECK(erased.erased.size() == 1);
    CHECK(erased.absent.empty());
    CHECK(erased.unresolved.empty());

    auto result = db_->find(key, height);
    CHECK_FALSE(result.has_value());

    // A key that was never stored. One version exists, so it was looked for
    // everywhere it could have been: this is absence, not an unfinished batch.
    auto key2 = make_test_key(2, 0);
    auto const missing = db_->apply_deletes(std::vector<utxoz::deferred_deletion_entry>{{key2, height}});
    CHECK(missing.erased.empty());
    CHECK(missing.absent.size() == 1);
    CHECK(missing.unresolved.empty());
    CHECK_FALSE(missing.error.has_value());
}

TEST_CASE("Reference: close and reopen", "[reference]") {
    auto path = make_fresh_path();

    // Insert data
    {
        auto r_db = utxoz::reference_db::open_for_testing(path, true);
        REQUIRE(r_db);
        auto db = std::move(*r_db);

        for (int i = 0; i < 50; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), 0);
            CHECK(db.insert(key, static_cast<uint32_t>(i + 1), static_cast<uint32_t>(i * 100), 100).value());
        }
        CHECK(db.size() == 50);
        db.close();
    }

    // Reopen and verify
    {
        auto r_db = utxoz::reference_db::open_for_testing(path, false);
        REQUIRE(r_db);
        auto db = std::move(*r_db);
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

TEST_CASE("Reference: mode mismatch detection", "[reference]") {
    auto path = make_fresh_path();

    // Create as reference
    {
        auto r_db = utxoz::reference_db::open_for_testing(path, true);
        REQUIRE(r_db);
        auto db = std::move(*r_db);
        auto key = make_test_key(1, 0);
        db.insert(key, 1, 100, 100).value();
        db.close();
    }

    // Try to open as full - should return error
    {
        auto r = utxoz::full_db::open_for_testing(path, false);
        REQUIRE_FALSE(r.has_value());
        CHECK(r.error() == utxoz::error_code::storage_mode_mismatch);
    }

    // Create as full in another path
    auto path2 = make_fresh_path();
    {
        auto r_db = utxoz::full_db::open_for_testing(path2, true);
        REQUIRE(r_db);
        auto db = std::move(*r_db);
        auto key = make_test_key(1, 0);
        auto value = std::vector<uint8_t>(30, 0x42);
        db.insert(key, value, 100).value();
        db.close();
    }

    // Try to open as reference - should return error
    {
        auto r = utxoz::reference_db::open_for_testing(path2, false);
        REQUIRE_FALSE(r.has_value());
        CHECK(r.error() == utxoz::error_code::storage_mode_mismatch);
    }

    std::filesystem::remove_all(path);
    std::filesystem::remove_all(path2);
}

TEST_CASE_METHOD(ReferenceFixture, "Reference: deferred deletions", "[reference]") {
    std::vector<utxoz::raw_outpoint> keys;
    for (int i = 0; i < 10; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        keys.push_back(key);
        CHECK(db_->insert(key, static_cast<uint32_t>(i), static_cast<uint32_t>(i * 10), 100).value());
    }

    CHECK(db_->size() == 10);

    std::vector<utxoz::deferred_deletion_entry> batch;
    for (int i = 0; i < 5; ++i) batch.emplace_back(keys[i], 200);

    auto const progress = db_->apply_deletes(batch);
    CHECK(progress.erased.size() == 5);
    CHECK(progress.absent.empty());
    CHECK(progress.unresolved.empty());

    for (int i = 0; i < 5; ++i) {
        auto result = db_->find(keys[i], 200);
        CHECK_FALSE(result.has_value());
    }

    for (int i = 5; i < 10; ++i) {
        auto result = db_->find(keys[i], 200);
        CHECK(result.has_value());
    }
}

TEST_CASE_METHOD(ReferenceFixture, "Reference: for_each_key", "[reference]") {
    for (int i = 0; i < 20; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        db_->insert(key, static_cast<uint32_t>(i), 0, 100).value();
    }

    size_t count = 0;
    REQUIRE(db_->for_each_key([&](utxoz::raw_outpoint const&) {
        ++count;
    }));

    CHECK(count == 20);
}

TEST_CASE_METHOD(ReferenceFixture, "Reference: for_each_entry", "[reference]") {
    for (int i = 0; i < 20; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        db_->insert(key, static_cast<uint32_t>(i + 1), static_cast<uint32_t>(i * 100),
                   static_cast<uint32_t>(100 + i)).value();
    }

    size_t count = 0;
    REQUIRE(db_->for_each_entry([&](utxoz::raw_outpoint const&, uint32_t height,
                           uint32_t file_number, uint32_t offset) {
        CHECK(height >= 100);
        CHECK(file_number >= 1);
        ++count;
    }));

    CHECK(count == 20);
}

TEST_CASE_METHOD(ReferenceFixture, "Reference: statistics", "[reference]") {
    for (int i = 0; i < 50; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        db_->insert(key, static_cast<uint32_t>(i), 0, static_cast<uint32_t>(100 + i)).value();
    }

    auto stats = db_->get_statistics();
    CHECK(stats.mode == utxoz::storage_mode::reference);
    CHECK(stats.total_entries == 50);

    // Counters only exist when recording is compiled in; the mode and the entry
    // count above are kept by the database itself and hold either way. Same
    // split as the full-mode test in test_database.cpp — this one was left
    // behind when that convention was applied.
#if UTXOZ_STATISTICS_LEVEL >= 1
    CHECK(stats.total_inserts >= 50);
#endif
}

TEST_CASE_METHOD(ReferenceFixture, "Reference: compaction", "[reference]") {
    // Insert enough data to verify compaction works
    for (int i = 0; i < 100; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        db_->insert(key, static_cast<uint32_t>(i), 0, 100).value();
    }

    // Erase half
    std::vector<utxoz::deferred_deletion_entry> batch;
    for (int i = 0; i < 50; ++i) {
        batch.emplace_back(make_test_key(static_cast<uint32_t>(i), 0), 200);
    }
    auto const progress = db_->apply_deletes(batch);
    CHECK(progress.erased.size() == 50);
    CHECK(progress.absent.empty());
    CHECK(progress.unresolved.empty());

    // Compact
    REQUIRE(db_->compact_all().has_value());

    // Verify remaining entries
    for (int i = 50; i < 100; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        auto result = db_->find(key, 300);
        CHECK(result.has_value());
    }
}

TEST_CASE("Reference: file naming uses compact_v prefix", "[reference]") {
    auto path = make_fresh_path();

    {
        auto r_db = utxoz::reference_db::open_for_testing(path, true);
        REQUIRE(r_db);
        auto db = std::move(*r_db);
        auto key = make_test_key(1, 0);
        db.insert(key, 1, 100, 100).value();
        db.close();
    }

    // Verify reference file exists
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

TEST_CASE("Reference: config file is created", "[reference]") {
    auto path = make_fresh_path();

    {
        auto r_db = utxoz::reference_db::open_for_testing(path, true);
        REQUIRE(r_db);
        auto db = std::move(*r_db);
        db.close();
    }

    CHECK(std::filesystem::exists(path + "/utxoz_config.dat"));

    std::filesystem::remove_all(path);
}

/**
 * The mode was renamed; the database was not.
 *
 * `compact_db` became `reference_db` to stop one word meaning two things — the
 * storage mode, and the operation that merges version files. Nothing about what
 * is written changed, and this is what says so: the files a reference database
 * creates still carry the names they always did, and the byte the config stores
 * for the mode is still the same byte.
 *
 * A database written by 0.8 has to open here, and one written here has to open
 * there. Renaming the files or renumbering the enumerator would break both —
 * silently in the second case, since that byte is what tells a reopen which
 * kind of database it is looking at.
 */
TEST_CASE("renaming the mode did not rename anything on disk", "[reference][format]") {
    auto const path = make_fresh_path();

    {
        auto opened = utxoz::reference_db::open_for_testing(path, true);
        REQUIRE(opened);
        auto db = std::move(*opened);
        REQUIRE(db.insert(make_test_key(1, 0), 7, 42, 100).value());
        db.close();
    }

    // The names, unchanged from before the rename.
    CHECK(std::filesystem::exists(fmt::format("{}/compact_v00000.dat", path)));
    CHECK(std::filesystem::exists(fmt::format("{}/utxoz_config.dat", path)));

    // And the mode byte the config carries.
    {
        std::ifstream ifs(fmt::format("{}/utxoz_config.dat", path), std::ios::binary);
        REQUIRE(ifs);

        char magic[4]{};
        uint32_t format = 0;
        uint8_t mode_byte = 0xFF;
        ifs.read(magic, 4);
        ifs.read(reinterpret_cast<char*>(&format), sizeof(format));
        ifs.read(reinterpret_cast<char*>(&mode_byte), sizeof(mode_byte));
        REQUIRE(ifs);

        // Format 2 records what the database was written under; the mode still
        // sits immediately after the format field, which is all this case reads.
        CHECK(std::string(magic, 4) == "UTXO");
        CHECK(format == 2);
        CHECK(mode_byte == 1);
        CHECK(mode_byte == static_cast<uint8_t>(utxoz::storage_mode::reference));
    }

    // It reopens, and what was written is there.
    auto reopened = utxoz::reference_db::open_for_testing(path, false);
    REQUIRE(reopened);
    auto db = std::move(*reopened);
    auto const found = db.find(make_test_key(1, 0), 200);
    REQUIRE(found);
    CHECK(found->file_number == 7);
    CHECK(found->offset == 42);
    db.close();

    std::filesystem::remove_all(path);
}
