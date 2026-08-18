# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required(VERSION 3.20)

# Project definition
project(utxo-z
    DESCRIPTION "High-performance UTXO Database for Bitcoin Cash"
    LANGUAGES CXX
)

# Version: provided via -DUTXOZ_VERSION=x.y.z from Conan
if(NOT DEFINED UTXOZ_VERSION)
    message(FATAL_ERROR "UTXOZ_VERSION not defined. Pass -DUTXOZ_VERSION=x.y.z or use conan install with --version.")
endif()
message(STATUS "utxoz version: ${UTXOZ_VERSION}")

# Generated headers go in the build tree, never in the source tree.
#
# The source tree is shared by every build directory. A header generated into it
# is shared state: configuring a second build overwrites what the first one
# generated, and the first then compiles against the second's options with no
# signal that it happened — no error, no reconfiguration, and a CMakeCache that
# still reports the options it was given. Under ${CMAKE_CURRENT_BINARY_DIR} each
# build directory has its own, and configurations stop leaking between them.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utxoz/version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/utxoz/version.hpp
    @ONLY
)

# C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(UTXOZ_BUILD_TESTS "Build tests" ON)
option(UTXOZ_BUILD_EXAMPLES "Build examples" ON)
option(UTXOZ_BUILD_BENCHMARKS "Build benchmarks" ON)
option(UTXOZ_BUILD_LARGE_BENCHMARKS "Build large-scale benchmarks (production file sizes, requires significant disk/RAM)" ON)
option(UTXOZ_BUILD_DOCS "Build documentation" OFF)
# Operational tools, meaning tools an operator runs against a real database
# rather than instruments the project uses on itself. They use the public API
# only, they are installed with the library, and they are deliberately not tied
# to UTXOZ_BUILD_TESTS: a node operator who builds with tests off still needs to
# be able to ask what is in their database.
option(UTXOZ_BUILD_TOOLS "Build operational tools (utxoz_census, utxoz_verify_unique)" ON)
option(UTXOZ_CONAN_BUILD "Building via Conan (skip cmake config generation)" OFF)

# Log backend: custom, spdlog, none
if(NOT DEFINED UTXOZ_LOG_BACKEND)
    set(UTXOZ_LOG_BACKEND "custom")
endif()
message(STATUS "Log backend: ${UTXOZ_LOG_BACKEND}")

# Statistics: one cumulative level rather than a switch per counter set.
#
#   off     nothing is compiled
#   basic   the counters that describe what this process did: inserts, probes,
#           resolutions, lifetimes
#   lookup  basic, plus the per-class read-path telemetry
#
# `lookup` is not the default and is not meant to be. It costs about eleven per
# cent of a find() that hits, measured, on the path a node spends most of its
# time in; it is for measurement builds run against a real database. It also
# widens its own counters, which is why it is a level and not a flag: at `basic`
# nothing of it is compiled and the older counters keep the size they had. The
# sharded read-path counters come to 16 KiB per open database at `basic` and
# 96 KiB at `lookup` — a subtotal of three of them, not the size of every
# statistic. See doc/statistics-levels.md, which is where these figures are kept
# so that they are kept in one place.
#
# Names, not numbers, because a build log that says "2" says nothing. The numeric
# form exists only inside the generated header, for `#if` to compare against.
set(UTXOZ_STATISTICS_LEVEL "basic" CACHE STRING
    "Statistics compiled in: off, basic, or lookup")
set_property(CACHE UTXOZ_STATISTICS_LEVEL PROPERTY STRINGS off basic lookup)

if(UTXOZ_STATISTICS_LEVEL STREQUAL "off")
    set(UTXOZ_STATISTICS_LEVEL_VALUE 0)
elseif(UTXOZ_STATISTICS_LEVEL STREQUAL "basic")
    set(UTXOZ_STATISTICS_LEVEL_VALUE 1)
elseif(UTXOZ_STATISTICS_LEVEL STREQUAL "lookup")
    set(UTXOZ_STATISTICS_LEVEL_VALUE 2)
else()
    # Refused rather than defaulted. A typo that silently produced `basic` would
    # be a build nobody asked for, and the person who asked for `lookups` would
    # go on to read counters that were never compiled.
    message(FATAL_ERROR
        "UTXOZ_STATISTICS_LEVEL is '${UTXOZ_STATISTICS_LEVEL}'. "
        "It has to be one of: off, basic, lookup.")
endif()
message(STATUS "Statistics level: ${UTXOZ_STATISTICS_LEVEL} (${UTXOZ_STATISTICS_LEVEL_VALUE})")

# Set variables for config.hpp generation
set(UTXOZ_LOG_CUSTOM OFF)
set(UTXOZ_LOG_SPDLOG OFF)
set(UTXOZ_LOG_NONE OFF)
if(UTXOZ_LOG_BACKEND STREQUAL "custom")
    set(UTXOZ_LOG_CUSTOM ON)
elseif(UTXOZ_LOG_BACKEND STREQUAL "spdlog")
    set(UTXOZ_LOG_SPDLOG ON)
elseif(UTXOZ_LOG_BACKEND STREQUAL "none")
    set(UTXOZ_LOG_NONE ON)
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utxoz/config.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/utxoz/config.hpp
)

# Find dependencies
find_package(Boost REQUIRED)
find_package(fmt REQUIRED)

# Conditionally find spdlog
if(UTXOZ_LOG_BACKEND STREQUAL "spdlog")
    find_package(spdlog REQUIRED)
endif()

# Create main library
add_library(utxoz)
add_library(utxoz::utxoz ALIAS utxoz)

# Library sources
target_sources(utxoz
    PRIVATE
        src/database.cpp
        src/census.cpp
        src/uniqueness.cpp
        src/statistics.cpp
        src/statistics_json.cpp
        src/utils.cpp
        src/log.cpp
        src/database_impl.cpp
)

# Public headers
#
# The generated directory comes first, and the order is load-bearing rather than
# stylistic. Working trees that predate this change still hold a leftover
# include/utxoz/config.hpp and include/utxoz/version.hpp, ignored by git and
# regenerated by nobody. Searched first, the source tree would hand those to the
# compiler instead of the ones this build just wrote — the same bug with a longer
# fuse, since a stale header that nothing updates is worse than one that at least
# changes when somebody configures.
target_include_directories(utxoz
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

# Link dependencies
target_link_libraries(utxoz
    PUBLIC
        Boost::headers
        fmt::fmt-header-only
)

# The merge identifier comes from the system CSPRNG, which on Windows lives in
# bcrypt. std::random_device is deliberately not used: the standard allows it to
# be deterministic.
#
# PUBLIC, not PRIVATE. utxoz is a static library, so linking bcrypt while
# building it resolves nothing: BCryptGenRandom stays an undefined symbol in
# utxoz.lib and whoever links the archive has to supply bcrypt themselves.
# PRIVATE says they do not have to, and they find out at link time that they do.
if(WIN32)
    target_link_libraries(utxoz PUBLIC bcrypt)
endif()

# Given a mapped file whose header does not read as initialised, Boost assumes
# another process is still creating it and spins for this long before reporting
# corruption — five minutes by default. Nothing here waits on another process
# creating a segment: one instance per database is the documented contract, and
# a precondition nothing yet enforces (see #71), but either way this store never
# waits for such a creator. So a corrupt or foreign file should be reported
# promptly rather than five minutes later.
# Private: Boost.Interprocess is an implementation detail and this must not
# change the behaviour of anything a consumer compiles.
target_compile_definitions(utxoz
    PRIVATE BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC=10u)

# This project links Boost::headers and no Boost binary. On MSVC a Boost header
# can ask the linker for one anyway, through a `#pragma comment(lib, ...)` that
# fires on inclusion whether or not a symbol from that library is ever needed —
# so the request arrives with no library path to satisfy it and the link fails
# over a dependency nothing uses. Boost.Container's pmr headers do this, and
# Boost.JSON includes them.
#
# Private, and repeated on our own targets rather than made PUBLIC: a consumer
# may link a compiled Boost of their own, and which libraries their linker goes
# looking for is their decision. This changes no inline function — only that.
target_compile_definitions(utxoz PRIVATE BOOST_ALL_NO_LIB)

# Log backend configuration (defines are in config.hpp)
if(UTXOZ_LOG_BACKEND STREQUAL "spdlog")
    target_link_libraries(utxoz PRIVATE spdlog::spdlog_header_only)
elseif(NOT UTXOZ_LOG_BACKEND STREQUAL "custom" AND NOT UTXOZ_LOG_BACKEND STREQUAL "none")
    message(FATAL_ERROR "Invalid UTXOZ_LOG_BACKEND: ${UTXOZ_LOG_BACKEND}. Must be custom, spdlog, or none.")
endif()

# Compiler settings
target_compile_features(utxoz PUBLIC cxx_std_23)

if(MSVC)
    target_compile_options(utxoz PRIVATE /W4)
else()
    target_compile_options(utxoz PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
    )
endif()

# Sanitizers (e.g. -DUTXOZ_SANITIZER=address,undefined)
set(UTXOZ_SANITIZER "" CACHE STRING "Sanitizer to enable (address, undefined, thread, address,undefined)")
if(UTXOZ_SANITIZER)
    message(STATUS "Sanitizer: ${UTXOZ_SANITIZER}")
    target_compile_options(utxoz PUBLIC -fsanitize=${UTXOZ_SANITIZER} -fno-omit-frame-pointer)
    target_link_options(utxoz PUBLIC -fsanitize=${UTXOZ_SANITIZER})
endif()

# Examples
if(UTXOZ_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
# Operational tools
#
# Defined before the tests, so that the suite can see whether the target exists
# and compile the case that runs it. The dependency goes that way and only that
# way: the tool knows nothing about the tests.
#
# Outside the test block on purpose. `utxoz_census` answers "what is in this
# database", which is a question an operator asks about a production directory,
# so tying it to the test lifecycle would mean a node build could not produce it.
# It links the public API and nothing else: no internal headers, no test support,
# no dependency on the fixtures generator or the sizing instrument.
if(UTXOZ_BUILD_TOOLS)
    add_executable(utxoz_census tools/census.cpp)
    target_link_libraries(utxoz_census PRIVATE utxoz)
    # Only because Boost.Interprocess reaches MSVC's auto-link pragmas through
    # the public headers; see the note in the test CMakeLists.
    target_compile_definitions(utxoz_census PRIVATE BOOST_ALL_NO_LIB)

    # A second binary rather than a flag on the first: they answer different
    # questions and, more to the point, they exit differently. A census that
    # started returning 2 because of what it found would break every caller that
    # treats a non-zero status as "the tool failed".
    add_executable(utxoz_verify_unique tools/verify_unique.cpp)
    target_link_libraries(utxoz_verify_unique PRIVATE utxoz)
    target_compile_definitions(utxoz_verify_unique PRIVATE BOOST_ALL_NO_LIB)
endif()

if(UTXOZ_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)

    # The compatibility-fixture generator. Built with the tests because it uses
    # the same internal headers, and never installed: it is a tool a person runs
    # by hand to produce candidates, not part of what a consumer gets.
    add_executable(utxoz_make_format_fixtures tools/make_format_fixtures.cpp)
    target_link_libraries(utxoz_make_format_fixtures PRIVATE utxoz)
    target_include_directories(utxoz_make_format_fixtures
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/tests/support)
    # See src/detail/segment_open.hpp: this value is baked into an inline
    # function, so every target that includes that header must agree on it.
    target_compile_definitions(utxoz_make_format_fixtures
        PRIVATE BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC=10u
                BOOST_ALL_NO_LIB)

    # The sizing instrument. Built with the tests because it uses the same
    # internal headers, and never installed: it answers a question a person asks
    # before choosing a file size, and nothing a database does depends on it.
    add_executable(utxoz_sizing tools/sizing.cpp)
    target_link_libraries(utxoz_sizing PRIVATE utxoz)
    target_include_directories(utxoz_sizing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(utxoz_sizing
        PRIVATE BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC=10u
                BOOST_ALL_NO_LIB)

    # The suite runs the generator, so it has to exist before the suite does.
    add_dependencies(utxoz_tests utxoz_make_format_fixtures utxoz_sizing)
    if(TARGET utxoz_census)
        add_dependencies(utxoz_tests utxoz_census)
    endif()
    if(TARGET utxoz_verify_unique)
        add_dependencies(utxoz_tests utxoz_verify_unique)
    endif()
endif()

# Benchmarks
if(UTXOZ_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Documentation
if(UTXOZ_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    endif()
endif()

# Installation
include(GNUInstallDirs)

# Install library
install(TARGETS utxoz
    EXPORT utxoz-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install the operational tools. They are installed rather than merely built,
# because the database worth censusing belongs to a node and the person asking
# is on that machine, not in this source tree.
if(UTXOZ_BUILD_TOOLS)
    install(TARGETS utxoz_census utxoz_verify_unique
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# Install headers
#
# Two directories, and the generated one is not optional. The source-tree install
# used to carry config.hpp and version.hpp only because they happened to be
# sitting there; now that they are not, installing from the source tree alone
# would produce a package missing both. That failure does not surface here — it
# surfaces in whoever consumes the package, as a broken #include rather than a
# CMake error. So the generated directory is installed explicitly, and the two
# names are excluded from the source-tree install so a leftover copy in somebody's
# working tree can never be the one that ships.
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "config.hpp" EXCLUDE
        PATTERN "version.hpp" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)

# Install CMake config files
install(EXPORT utxoz-targets
    FILE utxoz-targets.cmake
    NAMESPACE utxoz::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/utxoz
)

# Create config file (only for non-Conan builds)
if(NOT UTXOZ_CONAN_BUILD)
    include(CMakePackageConfigHelpers)

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/utxoz-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/utxoz-config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/utxoz
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/utxoz-config-version.cmake"
        VERSION ${UTXOZ_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/utxoz-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/utxoz-config-version.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/utxoz
    )
endif()

# Package config
set(CPACK_PACKAGE_NAME "utxoz")
set(CPACK_PACKAGE_VERSION ${UTXOZ_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "Fernando Pelliccioni")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)
