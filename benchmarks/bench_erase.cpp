// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench_common.hpp"

namespace bench {

void register_erase_benchmarks(ankerl::nanobench::Bench& bench) {
    // Erase hit (pre-populated with enough entries for many iterations)
    {
        BenchFixture f;
        f.populate(100'000);
        uint32_t id = 0;
        bench.run("erase hit", [&] {
            ankerl::nanobench::doNotOptimizeAway(
                f.db.erase(make_test_key(id++, 0), 200)
            );
        });
    }

    // Erase miss
    {
        BenchFixture f;
        f.populate(10'000);
        uint32_t id = 100'000;
        bench.run("erase miss", [&] {
            ankerl::nanobench::doNotOptimizeAway(
                f.db.erase(make_test_key(id++, 0), 200)
            );
        });
    }

    // Erase + process_pending_deletions cycle
    {
        BenchFixture f;
        f.populate(100'000);
        uint32_t id = 0;
        bench.run("erase + process_pending_deletions (100 entries)", [&] {
            for (uint32_t i = 0; i < 100; ++i) {
                (void)f.db.erase(make_test_key(id++, 0), 200);
            }
            ankerl::nanobench::doNotOptimizeAway(f.db.process_pending_deletions());
        });
    }

    // Batch erase
    {
        BenchFixture f;
        f.populate(100'000);
        uint32_t id = 0;
        bench.run("batch erase 1K", [&] {
            for (uint32_t i = 0; i < 1000; ++i) {
                (void)f.db.erase(make_test_key(id++, 0), 200);
            }
        });
    }
}

} // namespace bench
