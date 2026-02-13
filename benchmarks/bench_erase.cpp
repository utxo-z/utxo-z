// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench_common.hpp"

namespace bench {

void register_erase_benchmarks(ankerl::nanobench::Bench& bench) {
    // Erase throughput
    bench.run("erase hit", [&] {
        BenchFixture f;
        f.populate(1000, 50);
        uint32_t id = 0;
        auto key = make_test_key(id++, 0);
        ankerl::nanobench::doNotOptimizeAway(f.db.erase(key, 200));
    });

    // Erase miss
    bench.run("erase miss", [&] {
        BenchFixture f;
        f.populate(1000, 50);
        auto key = make_test_key(999'999, 0);
        ankerl::nanobench::doNotOptimizeAway(f.db.erase(key, 200));
    });

    // Erase + process_pending_deletions cycle
    bench.run("erase + process_pending_deletions (100 entries)", [&] {
        BenchFixture f;
        f.populate(1000, 50);
        for (uint32_t i = 0; i < 100; ++i) {
            (void)f.db.erase(make_test_key(i, 0), 200);
        }
        ankerl::nanobench::doNotOptimizeAway(f.db.process_pending_deletions());
    });

    // Batch erase
    bench.run("batch erase 1K", [&] {
        BenchFixture f;
        f.populate(5000, 50);
        for (uint32_t i = 0; i < 1000; ++i) {
            (void)f.db.erase(make_test_key(i, 0), 200);
        }
    });
}

} // namespace bench
