# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required(VERSION 3.20)

project(utxoz-tests)

# Find Catch2
find_package(Catch2 REQUIRED)

# Test executable
add_executable(utxoz_tests
    test_database.cpp
    test_storage_correctness.cpp
)

target_link_libraries(utxoz_tests
    PRIVATE
        utxoz::utxoz
        Catch2::Catch2WithMain
)

# Discover tests
include(Catch)
catch_discover_tests(utxoz_tests)
