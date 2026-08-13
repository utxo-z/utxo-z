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
    test_reference_mode.cpp
    test_find_across_rotations.cpp
    test_statistics.cpp
    test_unresolved_vs_absent.cpp
    test_lookup_ownership.cpp
    test_deletion_ownership.cpp
    test_compaction_invariant.cpp
    test_version_catalog.cpp
    test_file_metadata_io.cpp
    test_compaction_recovery.cpp
    test_database_lock.cpp
    test_sync.cpp
    test_path_encoding.cpp
    test_named_object_guard.cpp
)

target_link_libraries(utxoz_tests
    PRIVATE
        utxoz::utxoz
        Catch2::Catch2WithMain
)

# The same cap the library is built with. src/detail/segment_open.hpp defines an
# inline function that bakes this value in, so a target including that header
# without the definition would link a second, slower copy of it; the header
# static_asserts on the value so the mistake is a compile error rather than a
# five-minute wait on one platform. See CMakeLists.txt in the project root.
target_compile_definitions(utxoz_tests
    PRIVATE BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC=10u)

# Two tests exercise internal headers directly, because what they pin is not
# reachable from the public API: test_version_catalog.cpp for fail-closed
# enumeration and sparseness, and test_named_object_guard.cpp for the three
# answers a named-object lookup can give, only one of which the public API can
# be made to produce.
target_include_directories(utxoz_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# Discover tests
include(Catch)
catch_discover_tests(utxoz_tests)
