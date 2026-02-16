// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include <fstream>

#include <fmt/format.h>

#include "bench_common.hpp"

namespace bench_large {
void run_ibd_simulation();
void run_large_ops(ankerl::nanobench::Bench& bench);
} // namespace bench_large

int main() {
    fmt::println("{:=^80}", " utxo-z Large-Scale Benchmarks ");
    fmt::println("Using production file sizes (2 GiB containers)");
    fmt::println("Estimated time: 5-10 minutes depending on hardware\n");

    bench_large::run_ibd_simulation();

    ankerl::nanobench::Bench bench;
    bench.title("utxo-z large-scale ops")
         .warmup(1)
         .relative(false)
         .minEpochIterations(1);

    bench_large::run_large_ops(bench);

    std::ofstream json_file("benchmark_results_large.json");
    bench.render(ankerl::nanobench::templates::json(), json_file);

    fmt::println("\nResults written to benchmark_results_large.json");
    return 0;
}
