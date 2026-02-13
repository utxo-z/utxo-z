// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench_common.hpp"

namespace bench {

void register_find_benchmarks(ankerl::nanobench::Bench& bench) {
    // Find in latest version (recently inserted UTXOs)
    bench.run("find hit (latest version)", [&] {
        BenchFixture f;
        f.populate(1000, 50);
        auto key = make_test_key(500, 0);
        ankerl::nanobench::doNotOptimizeAway(f.db.find(key, 200));
    });

    // Find miss (key not present)
    bench.run("find miss", [&] {
        BenchFixture f;
        f.populate(1000, 50);
        auto key = make_test_key(999'999, 0);
        ankerl::nanobench::doNotOptimizeAway(f.db.find(key, 200));
    });

    // Find across multiple value sizes
    bench.run("find hit (mixed sizes)", [&] {
        BenchFixture f;
        size_t const sizes[] = {30, 100, 400, 8000};
        for (uint32_t i = 0; i < 1000; ++i) {
            auto value = make_test_value(sizes[i % 4]);
            (void)f.db.insert(make_test_key(i, 0), value, 100);
        }
        uint32_t id = 0;
        auto key = make_test_key(id++, 0);
        ankerl::nanobench::doNotOptimizeAway(f.db.find(key, 200));
    });

    // Batch of finds after populating
    bench.run("batch find 1K hits", [&] {
        BenchFixture f;
        f.populate(5000, 50);
        for (uint32_t i = 0; i < 1000; ++i) {
            auto key = make_test_key(i, 0);
            ankerl::nanobench::doNotOptimizeAway(f.db.find(key, 200));
        }
    });
}

} // namespace bench
