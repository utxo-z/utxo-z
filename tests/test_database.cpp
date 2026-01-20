/**
 * @file test_database.cpp
 * @brief Database functionality tests
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

// Atomic counter for unique test directories
inline std::atomic<uint64_t> test_counter{0};

utxoz::key_t make_test_key(uint32_t tx_id, uint32_t output_index) {
    utxoz::key_t key{};
    for (size_t i = 0; i < 32; ++i) {
        key[i] = static_cast<uint8_t>((tx_id >> (i % 4 * 8)) & 0xFF);
    }
    std::memcpy(key.data() + 32, &output_index, 4);
    return key;
}

std::vector<uint8_t> make_test_value(size_t size) {
    std::vector<uint8_t> value(size);
    std::iota(value.begin(), value.end(), 0);
    return value;
}

struct DatabaseFixture {
    DatabaseFixture() {
        // Use truly unique directory per test using timestamp + atomic counter
        auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto unique_id = test_counter.fetch_add(1);
        test_path_ = fmt::format("./test_utxo_db_{}_{}_{}", getpid(), ts, unique_id);

        if (std::filesystem::exists(test_path_)) {
            std::filesystem::remove_all(test_path_);
        }
        db_.configure_for_testing(test_path_, true);
    }

    ~DatabaseFixture() {
        db_.close();
        if (std::filesystem::exists(test_path_)) {
            std::filesystem::remove_all(test_path_);
        }
    }

    utxoz::db db_;
    std::string test_path_;
};

} // anonymous namespace

TEST_CASE_METHOD(DatabaseFixture, "Basic insert and find", "[database]") {
    auto key = make_test_key(1, 0);
    auto value = make_test_value(50);
    uint32_t height = 100;

    // Insert
    CHECK(db_.insert(key, value, height));
    CHECK(db_.size() == 1);

    // Find
    auto result = db_.find(key, height);
    REQUIRE(result.has_value());
    CHECK(result->size() == value.size());
    CHECK(*result == value);

    // Duplicate insert should fail
    CHECK_FALSE(db_.insert(key, value, height));
    CHECK(db_.size() == 1);
}

TEST_CASE_METHOD(DatabaseFixture, "Multiple containers by value size", "[database]") {
    struct TestCase {
        size_t value_size;
        size_t expected_container;
    };

    std::vector<TestCase> test_cases = {
        {30, 0},   // Should go to container 0 (44 bytes)
        {100, 1},  // Should go to container 1 (128 bytes)
        {400, 2},  // Should go to container 2 (512 bytes)
        {8000, 3}  // Should go to container 3 (10240 bytes)
    };

    for (size_t i = 0; i < test_cases.size(); ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i + 1), 0);
        auto value = make_test_value(test_cases[i].value_size);

        CHECK(db_.insert(key, value, static_cast<uint32_t>(100 + i)));

        auto result = db_.find(key, static_cast<uint32_t>(100 + i));
        REQUIRE(result.has_value());
        CHECK(result->size() == value.size());
    }

    CHECK(db_.size() == test_cases.size());
}

TEST_CASE_METHOD(DatabaseFixture, "Erase operations", "[database]") {
    auto key = make_test_key(1, 0);
    auto value = make_test_value(50);
    uint32_t height = 100;

    // Insert and verify
    CHECK(db_.insert(key, value, height));
    CHECK(db_.size() == 1);

    // Erase
    CHECK(db_.erase(key, height) == 1);

    // Verify not found
    auto result = db_.find(key, height);
    CHECK_FALSE(result.has_value());

    // Erase non-existent key
    auto key2 = make_test_key(2, 0);
    CHECK(db_.erase(key2, height) == 0);
}

TEST_CASE_METHOD(DatabaseFixture, "Deferred deletions", "[database]") {
    // Insert multiple UTXOs
    std::vector<utxoz::key_t> keys;
    for (int i = 0; i < 10; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        auto value = make_test_value(50);
        keys.push_back(key);
        CHECK(db_.insert(key, value, 100));
    }

    CHECK(db_.size() == 10);

    // Erase some - should find them in latest version
    for (int i = 0; i < 5; ++i) {
        CHECK(db_.erase(keys[i], 200) == 1);
    }

    // Process pending deletions
    auto [deleted, failed] = db_.process_pending_deletions();
    CHECK(failed.size() == 0); // No failures expected

    // Verify deletions
    for (int i = 0; i < 5; ++i) {
        auto result = db_.find(keys[i], 200);
        CHECK_FALSE(result.has_value());
    }

    // Verify remaining UTXOs
    for (int i = 5; i < 10; ++i) {
        auto result = db_.find(keys[i], 200);
        CHECK(result.has_value());
    }
}

TEST_CASE_METHOD(DatabaseFixture, "Statistics", "[database]") {
    // Insert some data
    for (int i = 0; i < 100; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        auto value = make_test_value(50 + i % 50);
        CHECK(db_.insert(key, value, static_cast<uint32_t>(100 + i)));
    }

    auto stats = db_.get_statistics();
    CHECK(stats.total_entries == 100);
    CHECK(stats.total_inserts >= 100);

    // Test search stats
    auto key = make_test_key(0, 0);
    CHECK(db_.find(key, 200).has_value());

    auto search_stats = db_.get_search_stats().get_summary();
    CHECK(search_stats.total_operations > 0);
}

TEST_CASE("Key utilities", "[utils]") {
    std::array<uint8_t, 32> tx_hash;
    std::iota(tx_hash.begin(), tx_hash.end(), 0);
    uint32_t output_index = 42;

    auto key = utxoz::make_key(tx_hash, output_index);

    auto extracted_hash = utxoz::get_tx_hash(key);
    auto extracted_index = utxoz::get_output_index(key);

    CHECK(std::equal(tx_hash.begin(), tx_hash.end(), extracted_hash.begin()));
    CHECK(extracted_index == output_index);
}

TEST_CASE_METHOD(DatabaseFixture, "Large data set", "[database][.slow]") {
    constexpr size_t num_utxos = 1000;
    std::vector<utxoz::key_t> keys;
    keys.reserve(num_utxos);

    // Insert large number of UTXOs
    for (size_t i = 0; i < num_utxos; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), static_cast<uint32_t>(i % 10));
        auto value = make_test_value(50 + (i % 100));
        keys.push_back(key);

        CHECK(db_.insert(key, value, static_cast<uint32_t>(100 + i)));
    }

    CHECK(db_.size() == num_utxos);

    // Verify random subset
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, num_utxos - 1);

    for (int test = 0; test < 100; ++test) {
        size_t index = dist(gen);
        auto result = db_.find(keys[index], static_cast<uint32_t>(100 + num_utxos));
        CHECK(result.has_value());
    }

    // Compact and verify still works
    db_.compact_all();

    // Verify after compaction
    for (int test = 0; test < 50; ++test) {
        size_t index = dist(gen);
        auto result = db_.find(keys[index], static_cast<uint32_t>(100 + num_utxos));
        CHECK(result.has_value());
    }
}
