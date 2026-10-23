// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench_common.hpp"

#include "bench_large_limits.hpp"
#include <string>
#include <vector>
#include <cstdlib>

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
        auto r = utxoz::db::open(path, true);
        if (!r) throw std::runtime_error("Failed to open benchmark database");
        db.emplace(std::move(*r));
    }

    ~LargeBenchFixture() {
        db.reset();
        if (fs::exists(path)) {
            fs::remove_all(path);
        }
    }

    LargeBenchFixture(LargeBenchFixture const&) = delete;
    LargeBenchFixture& operator=(LargeBenchFixture const&) = delete;

    void populate(size_t n, size_t value_size = 43) {
        auto value = bench::make_test_value(value_size);
        for (size_t i = 0; i < n; ++i) {
            auto key = bench::make_test_key(static_cast<uint32_t>(i), 0);
            (void)db->insert(key, value, 100);
        }
    }

    std::optional<utxoz::db> db;
    std::string path;
};

} // anonymous namespace

void run_large_ops(ankerl::nanobench::Bench& bench, large_limits const& limits) {
    fmt::println("{:=^80}", " Large-Scale Operation Benchmarks ");

    // =========================================================================
    // Fixture 1: Single generation (~15M P2PKH entries in one 2GB file)
    // =========================================================================
    {
        auto const single_gen_entries = limits.single_gen_entries;
        fmt::println("\n  Populating single-generation fixture ({:L} entries)...", single_gen_entries);
        auto t0 = std::chrono::high_resolution_clock::now();
        LargeBenchFixture f;
        f.populate(single_gen_entries);
        auto t1 = std::chrono::high_resolution_clock::now();
        fmt::println("  Done in {:.1f}s (db size: {:L})\n",
            std::chrono::duration<double>(t1 - t0).count(), f.db->size());

        // Find — random access pattern to stress TLB and cache
        std::mt19937 rng(42);
        std::uniform_int_distribution<uint32_t> dist(0, single_gen_entries - 1);
        bench.run("find in 2GB map (random)", [&] {
            auto key = bench::make_test_key(dist(rng), 0);
            ankerl::nanobench::doNotOptimizeAway(f.db->find(key, 500));
        });

        // Insert — append into already-populated map
        auto value = bench::make_test_value(43);
        uint32_t next_insert = single_gen_entries;
        bench.run("insert into populated 2GB map", [&] {
            auto key = bench::make_test_key(next_insert++, 0);
            ankerl::nanobench::doNotOptimizeAway(f.db->insert(key, value, 500));
        });

        // Deletion.
        //
        // DIAGNOSTIC, and not usable as an equivalence gate. `erase` per key is
        // gone; `apply_deletes` takes a batch and answers with what it applied,
        // what it proved absent and what is still owed. Repeating one call under
        // nanobench would delete already-deleted keys, which move from `erased`
        // to `absent` — a different operation each epoch, so the dispersion
        // would describe the fixture's decay and not the code's cost.
        //
        // So it is timed once over a prepared batch, and the outcome is checked:
        // a figure for an operation that half failed is worse than no figure.
        {
            size_t const batch_size = std::min(single_gen_entries, limits.delete_batch);
            std::vector<utxoz::deferred_deletion_entry> batch;
            batch.reserve(batch_size);
            for (size_t i = 0; i < batch_size; ++i) {
                batch.emplace_back(bench::make_test_key(static_cast<uint32_t>(i), 0), 500);
            }

            auto const t_start = std::chrono::high_resolution_clock::now();
            auto const progress = f.db->apply_deletes(batch);
            auto const t_end = std::chrono::high_resolution_clock::now();
            auto const seconds = std::chrono::duration<double>(t_end - t_start).count();

            bool const clean = ! progress.error.has_value()
                               && progress.unresolved.empty()
                               && progress.absent.empty()
                               && progress.erased.size() == batch_size;
            fmt::println("  apply_deletes over {:L} keys: {:.1f} ns/entry  "
                         "[erased {:L}, absent {:L}, unresolved {:L}, error {}]  {}",
                         batch_size, seconds * 1e9 / double(batch_size),
                         progress.erased.size(), progress.absent.size(),
                         progress.unresolved.size(),
                         progress.error.has_value()
                             ? std::to_string(static_cast<int>(*progress.error)) : "none",
                         clean ? "diagnostic" : "DIAGNOSTIC, AND THE BATCH DID NOT COMPLETE");
        }
    }
    fmt::println("");

    // =========================================================================
    // Fixture 2: Multiple generations (~25M entries, file rotation triggered)
    // =========================================================================
    {
        auto const multi_gen_entries = limits.multi_gen_entries;
        fmt::println("  Populating multi-generation fixture ({:L} entries)...", multi_gen_entries);
        fmt::println("  (triggers file rotation past 2GB boundary)");
        auto t0 = std::chrono::high_resolution_clock::now();
        LargeBenchFixture f;
        f.populate(multi_gen_entries);
        auto t1 = std::chrono::high_resolution_clock::now();
        fmt::println("  Done in {:.1f}s (db size: {:L})\n",
            std::chrono::duration<double>(t1 - t0).count(), f.db->size());

        // Find in previous generation — keys 0..5M are in the first .dat file
        std::mt19937 rng(123);
        std::uniform_int_distribution<uint32_t> dist(0, 5'000'000);
        bench.run("find in previous generation", [&] {
            auto key = bench::make_test_key(dist(rng), 0);
            ankerl::nanobench::doNotOptimizeAway(f.db->find(key, 500));
        });

        // Close + reopen — measures the O(1) mmap advantage at 2GB+ scale
        auto path = f.path;
        bench.minEpochIterations(1).run("close+reopen 2GB+ map", [&] {
            f.db.reset();
            auto r = utxoz::db::open(path, false);
            if (!r) throw std::runtime_error("Failed to reopen database");
            f.db.emplace(std::move(*r));
        });
    }

    fmt::println("{:=^80}\n", "");
}

} // namespace bench_large
