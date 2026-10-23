// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include <fstream>
#include <string>
#include <cstdlib>
#include <cstdio>

#include <fmt/format.h>

#include "bench_common.hpp"
#include "bench_large_limits.hpp"

namespace bench_large {
void run_ibd_simulation(large_limits const& limits);
void run_large_ops(ankerl::nanobench::Bench& bench, large_limits const& limits);
} // namespace bench_large

int main(int argc, char** argv) {
    // --smoke exercises every path at a size that proves the harness works and
    // measures nothing. --max-entries bounds each fixture; it only has meaning
    // with --smoke, because the official run's sizes are the measurement.
    bool smoke = false;
    size_t cap = 40'000;
    for (int i = 1; i < argc; ++i) {
        std::string const arg = argv[i];
        if (arg == "--smoke") {
            smoke = true;
        } else if (arg.rfind("--max-entries=", 0) == 0) {
            cap = std::strtoull(arg.c_str() + 14, nullptr, 10);
            if (cap == 0) { fmt::println(stderr, "--max-entries must be positive"); return 2; }
        } else if (arg == "--help" || arg == "-h") {
            fmt::println("usage: utxoz_benchmarks_large [--smoke [--max-entries=N]]");
            fmt::println("  no option   the official run: {} IBD inserts, {} and {} entry fixtures",
                         bench_large::production_limits().ibd_inserts,
                         bench_large::production_limits().single_gen_entries,
                         bench_large::production_limits().multi_gen_entries);
            fmt::println("  --smoke     every path, bounded by --max-entries (default 40000)");
            return 0;
        } else {
            fmt::println(stderr, "unknown argument: {}", arg);
            return 2;
        }
    }

    auto const limits = smoke ? bench_large::smoke_limits(cap)
                              : bench_large::production_limits();

    fmt::println("{:=^80}", " utxo-z Large-Scale Benchmarks ");
    // Printed and flushed before any work, so that a run which then takes
    // longer than expected can be read rather than guessed at.
    fmt::println("mode:              {}", limits.smoke ? "SMOKE (measures nothing)" : "official");
    fmt::println("IBD inserts:       {:L}", limits.ibd_inserts);
    fmt::println("single-gen entries:{:L}", limits.single_gen_entries);
    fmt::println("multi-gen entries: {:L}", limits.multi_gen_entries);
    fmt::println("delete batch:      {:L}", limits.delete_batch);
    fmt::println("find lookups:      {:L}", limits.find_lookups);
    fmt::println("deferred interval: {:L}", limits.deferred_interval());
    fmt::println("suites:            run_ibd_simulation, run_large_ops");
    std::fflush(stdout);

    bench_large::run_ibd_simulation(limits);

    ankerl::nanobench::Bench bench;
    bench.title("utxo-z large-scale ops")
         .warmup(1)
         .relative(false)
         .minEpochIterations(1);

    bench_large::run_large_ops(bench, limits);

    std::ofstream json_file("benchmark_results_large.json");
    bench.render(ankerl::nanobench::templates::json(), json_file);

    fmt::println("\nResults written to benchmark_results_large.json");
    return 0;
}
