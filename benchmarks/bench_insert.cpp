// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench_common.hpp"

namespace bench {

void register_insert_benchmarks(ankerl::nanobench::Bench& bench) {
    // Insert throughput for each container size
    struct SizeCase {
        char const* name;
        size_t value_size;
    };

    SizeCase const cases[] = {
        {"insert 44B value",    30},
        {"insert 128B value",  100},
        {"insert 512B value",  400},
        {"insert 10KB value", 8000},
    };

    for (auto const& c : cases) {
        bench.run(c.name, [&] {
            BenchFixture f;
            auto value = make_test_value(c.value_size);
            uint32_t id = 0;
            ankerl::nanobench::doNotOptimizeAway(
                f.db.insert(make_test_key(id++, 0), value, 100)
            );
        });
    }

    // Bulk insert: 10K entries with 44B values
    bench.run("bulk insert 10K (44B)", [&] {
        BenchFixture f;
        auto value = make_test_value(30);
        for (uint32_t i = 0; i < 10'000; ++i) {
            (void)f.db.insert(make_test_key(i, 0), value, 100);
        }
    });

    // Bulk insert: 10K entries with mixed sizes
    bench.run("bulk insert 10K (mixed sizes)", [&] {
        BenchFixture f;
        size_t const sizes[] = {30, 100, 400, 8000};
        for (uint32_t i = 0; i < 10'000; ++i) {
            auto value = make_test_value(sizes[i % 4]);
            (void)f.db.insert(make_test_key(i, 0), value, 100);
        }
    });
}

} // namespace bench
