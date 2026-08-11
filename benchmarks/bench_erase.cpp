// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench_common.hpp"

namespace bench {

void register_erase_benchmarks(ankerl::nanobench::Bench& bench) {
    // One-key batch, hit (pre-populated with enough entries for many iterations)
    {
        BenchFixture f;
        f.populate(100'000);
        uint32_t id = 0;
        std::vector<utxoz::deferred_deletion_entry> one(1, {make_test_key(0, 0), 200});
        bench.run("apply_deletes hit (1 entry)", [&] {
            one[0] = utxoz::deferred_deletion_entry{make_test_key(id++, 0), 200};
            ankerl::nanobench::doNotOptimizeAway(f.db->apply_deletes(one));
        });
    }

    // One-key batch, miss
    {
        BenchFixture f;
        f.populate(10'000);
        uint32_t id = 100'000;
        std::vector<utxoz::deferred_deletion_entry> one(1, {make_test_key(0, 0), 200});
        bench.run("apply_deletes miss (1 entry)", [&] {
            one[0] = utxoz::deferred_deletion_entry{make_test_key(id++, 0), 200};
            ankerl::nanobench::doNotOptimizeAway(f.db->apply_deletes(one));
        });
    }

    // One batch of deletions, built by the caller and applied in one call.
    {
        BenchFixture f;
        f.populate(100'000);
        uint32_t id = 0;
        std::vector<utxoz::deferred_deletion_entry> batch;
        bench.run("apply_deletes (100 entries)", [&] {
            batch.clear();
            for (uint32_t i = 0; i < 100; ++i) {
                batch.emplace_back(make_test_key(id++, 0), 200);
            }
            ankerl::nanobench::doNotOptimizeAway(f.db->apply_deletes(batch));
        });
    }

    // Batch erase
    {
        BenchFixture f;
        f.populate(100'000);
        uint32_t id = 0;
        std::vector<utxoz::deferred_deletion_entry> batch;
        bench.run("apply_deletes 1K", [&] {
            batch.clear();
            for (uint32_t i = 0; i < 1000; ++i) batch.emplace_back(make_test_key(id++, 0), 200);
            ankerl::nanobench::doNotOptimizeAway(f.db->apply_deletes(batch));
        });
    }
}

} // namespace bench
