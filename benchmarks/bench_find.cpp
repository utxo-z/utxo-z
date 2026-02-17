// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench_common.hpp"

namespace bench {

void register_find_benchmarks(ankerl::nanobench::Bench& bench) {
    // Find in latest version (recently inserted P2PKH UTXOs)
    {
        BenchFixture f;
        f.populate(10'000);
        uint32_t id = 0;
        bench.run("find hit (latest version)", [&] {
            auto key = make_test_key(id++ % 10'000, 0);
            ankerl::nanobench::doNotOptimizeAway(f.db.find(key, 200));
        });
    }

    // Find miss (key not present)
    {
        BenchFixture f;
        f.populate(10'000);
        uint32_t id = 100'000;
        bench.run("find miss", [&] {
            auto key = make_test_key(id++, 0);
            ankerl::nanobench::doNotOptimizeAway(f.db.find(key, 200));
        });
    }

    // Find across chain-realistic value sizes
    {
        BenchFixture f;
        f.populate_chain_mix(10'000);
        uint32_t id = 0;
        bench.run("find hit (chain mix)", [&] {
            auto key = make_test_key(id++ % 10'000, 0);
            ankerl::nanobench::doNotOptimizeAway(f.db.find(key, 200));
        });
    }

    // Batch of finds
    {
        BenchFixture f;
        f.populate(10'000);
        uint32_t batch_start = 0;
        bench.run("batch find 1K hits", [&] {
            for (uint32_t i = 0; i < 1000; ++i) {
                auto key = make_test_key((batch_start + i) % 10'000, 0);
                ankerl::nanobench::doNotOptimizeAway(f.db.find(key, 200));
            }
            batch_start += 1000;
        });
    }
}

} // namespace bench
