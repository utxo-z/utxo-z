# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required(VERSION 3.20)

project(utxoz-benchmarks)

find_package(nanobench REQUIRED)

add_executable(utxoz_benchmarks
    bench_main.cpp
    bench_insert.cpp
    bench_find.cpp
    bench_erase.cpp
    bench_mixed_workload.cpp
    bench_storage.cpp
    storage_overhead_report.cpp
)

target_link_libraries(utxoz_benchmarks
    PRIVATE
        utxoz::utxoz
        nanobench::nanobench
)

# Large-scale benchmarks (production file sizes, 2GB containers)
if(UTXOZ_BUILD_LARGE_BENCHMARKS)
    add_executable(utxoz_benchmarks_large
        bench_large_main.cpp
        bench_large_ibd.cpp
        bench_large_ops.cpp
    )

    target_link_libraries(utxoz_benchmarks_large
        PRIVATE
            utxoz::utxoz
            nanobench::nanobench
    )
endif()
