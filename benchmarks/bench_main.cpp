// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include <fstream>

#include <fmt/format.h>

#include "bench_common.hpp"

int main() {
    ankerl::nanobench::Bench bench;
    bench.title("utxo-z benchmarks")
         .warmup(3)
         .relative(false)
         .minEpochIterations(5);

    bench::register_insert_benchmarks(bench);
    bench::register_find_benchmarks(bench);
    bench::register_erase_benchmarks(bench);
    bench::register_mixed_workload_benchmarks(bench);
    bench::register_storage_benchmarks(bench);

    std::ofstream json_file("benchmark_results.json");
    bench.render(ankerl::nanobench::templates::json(), json_file);

    fmt::println("Benchmark results written to benchmark_results.json");

    bench::run_storage_overhead_report();

    return 0;
}
