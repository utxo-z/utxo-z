// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench_common.hpp"

namespace bench {

void register_storage_benchmarks(ankerl::nanobench::Bench& bench) {
    // =========================================================================
    // Close + Reopen cost
    // =========================================================================
    // Measures the cost of serializing/deserializing the map on close/reopen.
    // On master (managed_mapped_file), close is just msync + unmap and reopen
    // is mmap + find named object. With serialization, close writes all entries
    // sequentially and reopen reads + re-inserts.

    struct CloseReopenCase {
        char const* name;
        size_t count;
        size_t value_size;
    };

    CloseReopenCase const cr_cases[] = {
        {"close+reopen 1K (44B)",     1'000,  30},
        {"close+reopen 10K (44B)",   10'000,  30},
        {"close+reopen 50K (44B)",   50'000,  30},
        {"close+reopen 100K (44B)", 100'000,  30},
        {"close+reopen 10K (128B)",  10'000, 100},
        {"close+reopen 50K (128B)",  50'000, 100},
    };

    for (auto const& c : cr_cases) {
        BenchFixture f;
        f.populate(c.count, c.value_size);
        auto path = f.path;

        bench.minEpochIterations(3).run(c.name, [&] {
            f.db.close();
            f.db.configure_for_testing(path, false);
        });
    }
}

} // namespace bench
