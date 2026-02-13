// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

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

#include <fmt/format.h>
#include <nanobench.h>

#include <utxoz/database.hpp>
#include <utxoz/types.hpp>
#include <utxoz/utils.hpp>

namespace bench {

inline std::atomic<uint64_t> bench_counter{0};

inline
utxoz::raw_outpoint make_test_key(uint32_t tx_id, uint32_t output_index) {
    utxoz::raw_outpoint key{};
    for (size_t i = 0; i < 32; ++i) {
        key[i] = static_cast<uint8_t>((tx_id >> (i % 4 * 8)) & 0xFF);
    }
    std::memcpy(key.data() + 32, &output_index, 4);
    return key;
}

inline
std::vector<uint8_t> make_test_value(size_t size) {
    std::vector<uint8_t> value(size);
    std::iota(value.begin(), value.end(), 0);
    return value;
}

struct BenchFixture {
    BenchFixture() {
        auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto unique_id = bench_counter.fetch_add(1);
        path = fmt::format("./bench_utxo_db_{}_{}_{}", getpid(), ts, unique_id);

        if (std::filesystem::exists(path)) {
            std::filesystem::remove_all(path);
        }
        db.configure_for_testing(path, true);
    }

    ~BenchFixture() {
        db.close();
        if (std::filesystem::exists(path)) {
            std::filesystem::remove_all(path);
        }
    }

    BenchFixture(BenchFixture const&) = delete;
    BenchFixture& operator=(BenchFixture const&) = delete;

    void populate(size_t n, size_t value_size = 50) {
        auto value = make_test_value(value_size);
        for (size_t i = 0; i < n; ++i) {
            auto key = make_test_key(static_cast<uint32_t>(i), 0);
            (void)db.insert(key, value, 100);
        }
    }

    utxoz::db db;
    std::string path;
};

// Benchmark registration functions (defined in each bench_*.cpp)
void register_insert_benchmarks(ankerl::nanobench::Bench& bench);
void register_find_benchmarks(ankerl::nanobench::Bench& bench);
void register_erase_benchmarks(ankerl::nanobench::Bench& bench);
void register_mixed_workload_benchmarks(ankerl::nanobench::Bench& bench);
void register_storage_benchmarks(ankerl::nanobench::Bench& bench);
void run_storage_overhead_report();

} // namespace bench
