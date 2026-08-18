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
    test_format_barrier.cpp
    test_container_selection.cpp
    test_capacity_policy.cpp
    test_sizing_tool.cpp
    support/boost_json.cpp
    test_value_initialisation.cpp
    test_failpoint_reset.cpp
    test_fixture_generator.cpp
    test_census.cpp
    test_uniqueness.cpp
    test_lookup_telemetry.cpp
    test_open_for_inspection.cpp
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
#
# BOOST_ALL_NO_LIB is there for a different reason: the compatibility cases parse
# the manifest with Boost.JSON, which includes Boost.Container's pmr headers,
# which ask MSVC for a Boost binary this project does not link. Also explained in
# the root CMakeLists.txt.
target_compile_definitions(utxoz_tests
    PRIVATE BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC=10u
            BOOST_ALL_NO_LIB)

# Where the compatibility fixtures live. Passed in rather than discovered, so the
# tests do not depend on the working directory ctest happens to give them.
# The compatibility suite needs the fixtures, which are deliberately not part of
# what the recipe exports: they are this repository's evidence about what earlier
# builds wrote, not something a consumer building from source needs. Where they
# are absent the cases are not built at all — a suite that silently skipped them
# would report the same green as one that checked them.
# The gate below is evaluated once, at configure time. Without this, adding or
# removing the fixtures leaves an already-configured build deciding on what was
# true when it was first configured — the compatibility cases quietly absent, or
# quietly still listed, with nothing to say so.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/epoch1-lp64/manifest.json")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/epoch1-lp64/manifest.json")
    target_sources(utxoz_tests PRIVATE test_format_compatibility.cpp)
    target_compile_definitions(utxoz_tests
        PRIVATE UTXOZ_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/epoch1-lp64")
else()
    message(STATUS "Format compatibility fixtures absent: those cases are not built")
endif()

# Two tests exercise internal headers directly, because what they pin is not
# reachable from the public API: test_version_catalog.cpp for fail-closed
# enumeration and sparseness, and test_named_object_guard.cpp for the three
# answers a named-object lookup can give, only one of which the public API can
# be made to produce.
target_include_directories(utxoz_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# Where the fixture generator ends up. test_fixture_generator.cpp runs it as a
# process, because what it checks is what the tool does on its way out.
target_compile_definitions(utxoz_tests
    PRIVATE UTXOZ_FIXTURE_GENERATOR="$<TARGET_FILE:utxoz_make_format_fixtures>"
            UTXOZ_SIZING_TOOL="$<TARGET_FILE:utxoz_sizing>")

# The census tool is built by UTXOZ_BUILD_TOOLS, which is a separate switch: it
# is an operational tool and does not belong to the test lifecycle. So the case
# that runs it is compiled only when it exists, and says so when it does not
# rather than testing nothing quietly.
if(TARGET utxoz_verify_unique)
    target_compile_definitions(utxoz_tests
        PRIVATE UTXOZ_VERIFY_TOOL="$<TARGET_FILE:utxoz_verify_unique>")
else()
    message(STATUS "utxoz_verify_unique not built: the case that runs it is not compiled")
endif()

if(TARGET utxoz_census)
    target_compile_definitions(utxoz_tests
        PRIVATE UTXOZ_CENSUS_TOOL="$<TARGET_FILE:utxoz_census>")
else()
    message(STATUS "utxoz_census not built: the case that runs it is not compiled")
endif()

# A contract that can only be checked by failing to compile.
#
# An unrecognised platform has no fallback tag, and an explicit one has a floor
# beneath it. Both refusals happen before there is a program, so no running test
# can observe them — these targets are built by ctest and required to fail.
foreach(bad_tag 0 999)
    add_library(utxoz_platform_tag_${bad_tag} OBJECT EXCLUDE_FROM_ALL
        ${CMAKE_CURRENT_SOURCE_DIR}/platform_tag_refusal.cpp)
    target_link_libraries(utxoz_platform_tag_${bad_tag} PRIVATE utxoz::utxoz)
    target_include_directories(utxoz_platform_tag_${bad_tag}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
    target_compile_definitions(utxoz_platform_tag_${bad_tag}
        PRIVATE BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC=10u
                BOOST_ALL_NO_LIB
                UTXOZ_PLATFORM_TAG=${bad_tag})

    add_test(NAME "a platform tag of ${bad_tag} does not compile"
             COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
                     --target utxoz_platform_tag_${bad_tag} --config $<CONFIG>)
    set_tests_properties("a platform tag of ${bad_tag} does not compile"
                         PROPERTIES WILL_FAIL TRUE TIMEOUT 300)
endforeach()

# The control, and it is not optional. A WILL_FAIL test passes when the build
# fails for *any* reason — a mistyped target, a missing include, a broken
# toolchain — so on its own it proves nothing about the assertion it is supposed
# to be reaching. This one compiles the same file with a tag that is allowed, and
# has to succeed.
add_library(utxoz_platform_tag_ok OBJECT EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/platform_tag_refusal.cpp)
target_link_libraries(utxoz_platform_tag_ok PRIVATE utxoz::utxoz)
target_include_directories(utxoz_platform_tag_ok PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_compile_definitions(utxoz_platform_tag_ok
    PRIVATE BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC=10u
            BOOST_ALL_NO_LIB
            UTXOZ_PLATFORM_TAG=1000)

add_test(NAME "a platform tag at the floor compiles"
         COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
                 --target utxoz_platform_tag_ok --config $<CONFIG>)
set_tests_properties("a platform tag at the floor compiles" PROPERTIES TIMEOUT 300)

# Discover tests.
#
# With a timeout, because the alternative is not a slow suite but a stuck one: a
# test that blocks takes the whole platform's job with it and CI reports nothing
# except six hours of silence, naming nothing. The slowest legitimate case is the
# one that waits out the mapped-file initialisation timeout, at eleven seconds;
# three hundred leaves room for a loaded runner and still fails in minutes rather
# than never — and names the test when it does.
include(Catch)
#
# The budget is per configuration, not per project: the slowest legitimate case
# takes eleven seconds in an optimised build and over three hundred under
# address and undefined-behaviour sanitizers, where everything is instrumented
# and unoptimised. One number for both would either fail honest work or stop
# catching anything.
if(UTXOZ_SANITIZER OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(UTXOZ_TEST_TIMEOUT 1200)
else()
    set(UTXOZ_TEST_TIMEOUT 300)
endif()
message(STATUS "Per-test timeout: ${UTXOZ_TEST_TIMEOUT}s")
catch_discover_tests(utxoz_tests PROPERTIES TIMEOUT ${UTXOZ_TEST_TIMEOUT})
