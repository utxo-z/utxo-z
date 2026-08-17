// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file bench_lookup_telemetry.cpp
 * @brief What the read-path counters cost.
 *
 * The comparison is between two builds of the same source, not between two code
 * paths in one binary: run this from a build with statistics and from a build
 * without, and the difference is the overhead. That is the only honest way to
 * measure it, because with statistics compiled out there is nothing left to
 * branch on — the counters are not skipped at run time, they are absent.
 *
 * The workload is stated rather than left to be inferred from the name, because
 * the answer depends entirely on it: a hit in the first active map touches one
 * counter, and a key resolved from the third generation of a sweep touches a
 * handful of stack fields and one publication at the end. Reporting "the
 * overhead" without the distribution would be reporting a number about a
 * workload nobody named.
 */

#include "bench_common.hpp"

#include <vector>

namespace bench {

namespace {

/// A database with one class holding several generations, so that a sweep has
/// somewhere to walk. Returns keys that live in the oldest generation.
std::vector<utxoz::raw_outpoint> layered_keys(BenchFixture& f, size_t per_generation,
                                              size_t generations) {
    std::vector<utxoz::raw_outpoint> oldest;
    auto const value = make_test_value(43);
    uint32_t id = 0;
    for (size_t g = 0; g < generations; ++g) {
        for (size_t i = 0; i < per_generation; ++i) {
            auto const key = make_test_key(id++, 0);
            (void) f.db->insert(key, value, 100);
            if (g == 0) oldest.push_back(key);
        }
        if (g + 1 < generations) {
            utxoz::detail::failpoints::force_rotations.store(1, std::memory_order_relaxed);
            (void) f.db->insert(make_test_key(900000 + uint32_t(g), 0), value, 100);
        }
    }
    return oldest;
}

} // namespace

void register_lookup_telemetry_benchmarks(ankerl::nanobench::Bench& bench) {
    // Every lookup answered by the first active map probed: the cheapest shape,
    // and the one a node spends most of its time in. Two counters per call.
    {
        BenchFixture f;
        f.populate(10'000);
        uint32_t id = 0;
        bench.run("telemetry: active hit, first class", [&] {
            auto key = make_test_key(id++ % 10'000, 0);
            ankerl::nanobench::doNotOptimizeAway(f.db->find(key, 200));
        });
    }

    // Every lookup missing everywhere: five active maps probed and none answers,
    // which is the most counters a find() can touch.
    {
        BenchFixture f;
        f.populate(10'000);
        uint32_t id = 500'000;
        bench.run("telemetry: miss, every class probed", [&] {
            auto key = make_test_key(id++, 0);
            ankerl::nanobench::doNotOptimizeAway(f.db->find(key, 200));
        });
    }

    // A sweep over history. The per-key work is on the stack and the publication
    // happens once for the whole batch, so this is where the shape of the cost
    // differs most from the per-call paths above.
    {
        BenchFixture f;
        auto const oldest = layered_keys(f, 2'000, 3);
        std::vector<utxoz::lookup_request> batch;
        batch.reserve(256);
        size_t offset = 0;
        bench.run("telemetry: sweep of 256 keys over three generations", [&] {
            batch.clear();
            for (size_t i = 0; i < 256; ++i) {
                batch.push_back({oldest[(offset + i) % oldest.size()], 200});
            }
            offset += 256;
            ankerl::nanobench::doNotOptimizeAway(f.db->resolve(batch));
        });
    }

    // A mix, stated: nine in ten answered by an active map, one in ten deferred
    // and then resolved from history. Closer to a node than any of the three
    // above on its own, and the number to quote if only one is quoted.
    {
        BenchFixture f;
        auto const oldest = layered_keys(f, 2'000, 3);
        std::vector<utxoz::lookup_request> batch;
        uint32_t id = 0;
        bench.run("telemetry: mixed 9 active hits to 1 historical", [&] {
            for (int i = 0; i < 9; ++i) {
                auto key = make_test_key(4'000 + (id++ % 2'000), 0);
                ankerl::nanobench::doNotOptimizeAway(f.db->find(key, 200));
            }
            batch.clear();
            batch.push_back({oldest[id % oldest.size()], 200});
            ankerl::nanobench::doNotOptimizeAway(f.db->resolve(batch));
        });
    }
}

} // namespace bench
