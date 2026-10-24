# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required(VERSION 3.20)

project(utxoz-benchmarks)

find_package(nanobench REQUIRED)

add_executable(utxoz_benchmarks
    bench_main.cpp
    bench_synthetic_control.cpp
    bench_insert.cpp
    bench_find.cpp
    bench_erase.cpp
    bench_mixed_workload.cpp
    bench_storage.cpp
    bench_lookup_telemetry.cpp
    storage_overhead_report.cpp
)

target_link_libraries(utxoz_benchmarks
    PRIVATE
        utxoz::utxoz
        nanobench::nanobench
)

# The telemetry benchmark drives rotations through the failpoints, which live
# in an internal header. The rest of the suite uses the public API only.
target_include_directories(utxoz_benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# Large-scale benchmarks (production file sizes, 2GB containers)
if(UTXOZ_BUILD_LARGE_BENCHMARKS)
    add_executable(utxoz_benchmarks_large
        bench_large_limits.hpp
        bench_large_main.cpp
        bench_large_ibd.cpp
        bench_large_ops.cpp
    )

    target_link_libraries(utxoz_benchmarks_large
        PRIVATE
            utxoz::utxoz
            nanobench::nanobench
    )

    # The same internal headers the regular suite needs. `bench_common.hpp`
    # includes detail/durability.hpp, so this target never compiled without it —
    # it is only noticed when the option is on, which the project's own build
    # configuration leaves off.
    target_include_directories(utxoz_benchmarks_large
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
endif()

# The calibration harness.
#
# A separate target from the benchmark suites on purpose: it answers a different
# question — how well can this host resolve a difference at all — and it must
# stay small enough to read in one sitting. It uses the public API only.
add_executable(utxoz_calib calib_stationary.cpp)
target_link_libraries(utxoz_calib PRIVATE utxoz::utxoz)

# The same question asked where the history is real: a lookup the active
# containers cannot answer, going through find() -> resolve() as published. A
# separate file so that calib_stationary.cpp stays what the accepted A/A
# confirmation measured. The measured version (sha256 7e18fc2c...) is kept
# with that evidence; the current file differs from it only in argument and
# output handling: whole-number options are parsed strictly (calib_args.hpp),
# and --out is written with checks, its directory checked early, and a self-test;
# and in naming the headers it uses directly (<unistd.h> for getpid).
add_executable(utxoz_calib_hist calib_historical.cpp)
target_link_libraries(utxoz_calib_hist PRIVATE utxoz::utxoz)

# Insertion and compaction on a private, verified copy of an image built by
# utxoz_calib_hist. Mutating, so every run gets its own copy and the source is
# never written.
add_executable(utxoz_calib_mut calib_mutation.cpp)
target_link_libraries(utxoz_calib_mut PRIVATE utxoz::utxoz)
