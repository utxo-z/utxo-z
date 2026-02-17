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
    // Uses realistic BCH value sizes (P2PKH 43B, 123B).

    struct CloseReopenCase {
        char const* name;
        size_t count;
        size_t value_size;
    };

    CloseReopenCase const cr_cases[] = {
        {"close+reopen 1K (P2PKH)",     1'000,  43},
        {"close+reopen 10K (P2PKH)",   10'000,  43},
        {"close+reopen 50K (P2PKH)",   50'000,  43},
        {"close+reopen 100K (P2PKH)", 100'000,  43},
        {"close+reopen 10K (123B)",    10'000, 123},
        {"close+reopen 50K (123B)",    50'000, 123},
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
