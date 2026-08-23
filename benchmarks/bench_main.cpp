// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include <fstream>
#include <stdexcept>

#include <fmt/format.h>

#include "bench_common.hpp"

int main() {
    // Parsed before the Bench exists, so a bad value stops the process instead
    // of producing a session that measured something nobody asked for. Reported
    // as an exit code rather than an abort: the harness reads exit codes, and a
    // SIGABRT tells it only that something died.
    uint32_t synthetic_lookups = 0;
    try {
        synthetic_lookups = bench::synthetic_lookups_from_environment();
    } catch (std::exception const& e) {
        fmt::println(stderr, "{}", e.what());
        return 2;
    }
    fmt::println("synthetic control: {} lookups", synthetic_lookups);

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
    bench::register_lookup_telemetry_benchmarks(bench);

    // Last. Its extra work touches caches, the page cache and the allocator, so
    // anything measured after it would meet a machine it had helped change.
    bench::register_synthetic_control(bench, synthetic_lookups);

    std::ofstream json_file("benchmark_results.json");
    bench.render(ankerl::nanobench::templates::json(), json_file);

    fmt::println("Benchmark results written to benchmark_results.json");

    bench::run_storage_overhead_report();

    return 0;
}
