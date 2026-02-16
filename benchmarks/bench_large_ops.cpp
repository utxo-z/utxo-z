// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench_common.hpp"

#include <chrono>
#include <filesystem>
#include <random>

#include <fmt/format.h>

namespace fs = std::filesystem;

namespace bench_large {

namespace {

struct LargeBenchFixture {
    LargeBenchFixture() {
        auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto unique_id = bench::bench_counter.fetch_add(1);
        path = fmt::format("./bench_large_ops_{}_{}_{}", getpid(), ts, unique_id);
        if (fs::exists(path)) {
            fs::remove_all(path);
        }
        db.configure(path, true);
    }

    ~LargeBenchFixture() {
        db.close();
        if (fs::exists(path)) {
            fs::remove_all(path);
        }
    }

    LargeBenchFixture(LargeBenchFixture const&) = delete;
    LargeBenchFixture& operator=(LargeBenchFixture const&) = delete;

    void populate(size_t n, size_t value_size = 33) {
        auto value = bench::make_test_value(value_size);
        for (size_t i = 0; i < n; ++i) {
            auto key = bench::make_test_key(static_cast<uint32_t>(i), 0);
            (void)db.insert(key, value, 100);
        }
    }

    utxoz::db db;
    std::string path;
};

// ~15M entries fills container 0 to ~70% of a 2GB file (one generation)
constexpr size_t single_gen_entries = 15'000'000;

// ~25M entries triggers file rotation in container 0 (~20M per 2GB file)
constexpr size_t multi_gen_entries = 25'000'000;

} // anonymous namespace

void run_large_ops(ankerl::nanobench::Bench& bench) {
    fmt::println("{:=^80}", " Large-Scale Operation Benchmarks ");

    // =========================================================================
    // Fixture 1: Single generation (~15M entries in one 2GB container file)
    // Benchmarks: find (random), insert (append), erase (sequential)
    // =========================================================================
    {
        fmt::println("\n  Populating single-generation fixture ({:L} entries)...", single_gen_entries);
        auto t0 = std::chrono::high_resolution_clock::now();
        LargeBenchFixture f;
        f.populate(single_gen_entries);
        auto t1 = std::chrono::high_resolution_clock::now();
        fmt::println("  Done in {:.1f}s (db size: {:L})\n",
            std::chrono::duration<double>(t1 - t0).count(), f.db.size());

        // Find — random access pattern to stress TLB and cache
        std::mt19937 rng(42);
        std::uniform_int_distribution<uint32_t> dist(0, single_gen_entries - 1);
        bench.run("find in 2GB map (random)", [&] {
            auto key = bench::make_test_key(dist(rng), 0);
            ankerl::nanobench::doNotOptimizeAway(f.db.find(key, 500));
        });

        // Insert — append into already-populated map
        auto value = bench::make_test_value(33);
        uint32_t next_insert = single_gen_entries;
        bench.run("insert into populated 2GB map", [&] {
            auto key = bench::make_test_key(next_insert++, 0);
            ankerl::nanobench::doNotOptimizeAway(f.db.insert(key, value, 500));
        });

        // Erase — sequential from the beginning (all keys in latest version)
        uint32_t next_erase = 0;
        bench.run("erase from 2GB map", [&] {
            auto key = bench::make_test_key(next_erase++, 0);
            ankerl::nanobench::doNotOptimizeAway(f.db.erase(key, 500));
        });
    }
    fmt::println("");

    // =========================================================================
    // Fixture 2: Multiple generations (~25M entries, file rotation triggered)
    // Benchmarks: find in previous generation, close+reopen at scale
    // =========================================================================
    {
        fmt::println("  Populating multi-generation fixture ({:L} entries)...", multi_gen_entries);
        fmt::println("  (triggers file rotation past 2GB boundary)");
        auto t0 = std::chrono::high_resolution_clock::now();
        LargeBenchFixture f;
        f.populate(multi_gen_entries);
        auto t1 = std::chrono::high_resolution_clock::now();
        fmt::println("  Done in {:.1f}s (db size: {:L})\n",
            std::chrono::duration<double>(t1 - t0).count(), f.db.size());

        // Find in previous generation — keys 0..5M are in the first .dat file
        // (before rotation at ~20M). Forces lookup across file boundaries.
        std::mt19937 rng(123);
        std::uniform_int_distribution<uint32_t> dist(0, 5'000'000);
        bench.run("find in previous generation", [&] {
            auto key = bench::make_test_key(dist(rng), 0);
            ankerl::nanobench::doNotOptimizeAway(f.db.find(key, 500));
        });

        // Close + reopen — measures the O(1) mmap advantage at 2GB+ scale
        auto path = f.path;
        bench.minEpochIterations(1).run("close+reopen 2GB+ map", [&] {
            f.db.close();
            f.db.configure(path, false);
        });
    }

    fmt::println("{:=^80}\n", "");
}

} // namespace bench_large
