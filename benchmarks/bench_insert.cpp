// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench_common.hpp"

namespace bench {

void register_insert_benchmarks(ankerl::nanobench::Bench& bench) {
    // Insert throughput for realistic BCH value sizes.
    // Fixture created once per case; entries accumulate across iterations.
    struct SizeCase {
        char const* name;
        size_t value_size;
    };

    SizeCase const cases[] = {
        {"insert P2PKH (43B)",  43},
        {"insert P2SH (41B)",   41},
        {"insert 123B",        123},
        {"insert 89B",          89},
    };

    for (auto const& c : cases) {
        BenchFixture f;
        auto value = make_test_value(c.value_size);
        uint32_t id = 0;
        bench.run(c.name, [&] {
            ankerl::nanobench::doNotOptimizeAway(
                f.db.insert(make_test_key(id++, 0), value, 100)
            );
        });
    }

    // Bulk insert: 10K P2PKH entries per iteration (entries accumulate)
    {
        BenchFixture f;
        auto value = make_test_value(43);
        uint32_t next_id = 0;
        bench.run("bulk insert 10K (P2PKH)", [&] {
            for (uint32_t i = 0; i < 10'000; ++i) {
                (void)f.db.insert(make_test_key(next_id++, 0), value, 100);
            }
        });
    }

    // Bulk insert: 10K entries with chain-realistic distribution
    {
        BenchFixture f;
        auto val_43  = make_test_value(43);
        auto val_41  = make_test_value(41);
        auto val_123 = make_test_value(123);
        auto val_89  = make_test_value(89);
        uint32_t next_id = 0;
        bench.run("bulk insert 10K (chain mix)", [&] {
            for (uint32_t i = 0; i < 10'000; ++i) {
                auto sz = chain_value_size((next_id + i) % 100);
                utxoz::output_data_span value;
                switch (sz) {
                    case 43:  value = val_43;  break;
                    case 41:  value = val_41;  break;
                    case 123: value = val_123; break;
                    default:  value = val_89;  break;
                }
                (void)f.db.insert(make_test_key(next_id++, 0), value, 100);
            }
        });
    }
}

} // namespace bench
