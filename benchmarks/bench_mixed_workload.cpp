// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench_common.hpp"

namespace bench {

void register_mixed_workload_benchmarks(ankerl::nanobench::Bench& bench) {
    // Simulated IBD pattern: batch of inserts + finds + erases per "block"
    // Uses chain-realistic value size distribution.
    // State accumulates across iterations (realistic).
    {
        BenchFixture f;
        auto val_43  = make_test_value(43);
        auto val_41  = make_test_value(41);
        auto val_123 = make_test_value(123);
        auto val_89  = make_test_value(89);
        uint32_t next_id = 0;
        uint32_t spent_id = 0;
        uint32_t block_num = 0;

        bench.run("simulated IBD (100 blocks)", [&] {
            for (uint32_t block = 0; block < 100; ++block) {
                uint32_t height = 100 + block_num++;

                // Each block: 20 new outputs, 10 spends, 5 lookups
                for (uint32_t i = 0; i < 20; ++i) {
                    auto sz = chain_value_size((next_id + i) % 100);
                    utxoz::output_data_span value;
                    switch (sz) {
                        case 43:  value = val_43;  break;
                        case 41:  value = val_41;  break;
                        case 123: value = val_123; break;
                        default:  value = val_89;  break;
                    }
                    (void)f.db.insert(make_test_key(next_id++, 0), value, height);
                }

                // Spend some older UTXOs (if available)
                for (uint32_t i = 0; i < 10 && spent_id < next_id - 20; ++i) {
                    (void)f.db.erase(make_test_key(spent_id++, 0), height);
                }

                // Lookup some random UTXOs
                for (uint32_t i = 0; i < 5; ++i) {
                    uint32_t lookup_id = spent_id + (i * 3) % (next_id - spent_id);
                    ankerl::nanobench::doNotOptimizeAway(
                        f.db.find(make_test_key(lookup_id, 0), height)
                    );
                }

                // Process pending deletions periodically
                if (block % 10 == 9) {
                    (void)f.db.process_pending_deletions();
                }
            }
        });
    }

    // Insert-heavy workload with chain distribution
    {
        BenchFixture f;
        auto val_43  = make_test_value(43);
        auto val_41  = make_test_value(41);
        auto val_123 = make_test_value(123);
        auto val_89  = make_test_value(89);
        uint32_t next_id = 0;

        bench.run("insert-heavy workload (1K inserts, 100 finds)", [&] {
            uint32_t base = next_id;
            for (uint32_t i = 0; i < 1000; ++i) {
                auto sz = chain_value_size(next_id % 100);
                utxoz::output_data_span value;
                switch (sz) {
                    case 43:  value = val_43;  break;
                    case 41:  value = val_41;  break;
                    case 123: value = val_123; break;
                    default:  value = val_89;  break;
                }
                (void)f.db.insert(make_test_key(next_id++, 0), value, 100);
            }
            for (uint32_t i = 0; i < 100; ++i) {
                ankerl::nanobench::doNotOptimizeAway(
                    f.db.find(make_test_key(base + i * 10, 0), 200)
                );
            }
        });
    }

    // Read-heavy workload (like serving queries)
    {
        BenchFixture f;
        f.populate(1000);

        bench.run("read-heavy workload (5K finds on 1K entries)", [&] {
            for (uint32_t i = 0; i < 5000; ++i) {
                auto key = make_test_key(i % 1000, 0);
                ankerl::nanobench::doNotOptimizeAway(f.db.find(key, 200));
            }
        });
    }
}

} // namespace bench
