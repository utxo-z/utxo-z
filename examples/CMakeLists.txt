# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required(VERSION 3.20)

# Examples
project(utxoz-examples)

# Basic example
add_executable(basic_usage basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE utxoz::utxoz)

# Blockchain processing example
add_executable(blockchain_processing blockchain_processing.cpp)
target_link_libraries(blockchain_processing PRIVATE utxoz::utxoz)

# Adapted benchmark
add_executable(adapted_benchmark adapted_benchmark.cpp)
target_link_libraries(adapted_benchmark PRIVATE utxoz::utxoz)

# Logging example
add_executable(logging_example logging_example.cpp)
target_link_libraries(logging_example PRIVATE utxoz::utxoz)
