# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required(VERSION 3.20)

# Project definition
project(utxo-z
    DESCRIPTION "High-performance UTXO Database for Bitcoin Cash"
    LANGUAGES CXX
)

# Version: must be provided via -DUTXOZ_VERSION=x.y.z or from Conan
if(NOT DEFINED UTXOZ_VERSION)
    message(FATAL_ERROR "UTXOZ_VERSION not defined. Pass -DUTXOZ_VERSION=x.y.z to cmake.")
endif()
message(STATUS "utxoz version: ${UTXOZ_VERSION}")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utxoz/version.hpp.in
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utxoz/version.hpp
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
option(UTXOZ_BUILD_DOCS "Build documentation" OFF)

# Log backend: custom, spdlog, none
if(NOT DEFINED UTXOZ_LOG_BACKEND)
    set(UTXOZ_LOG_BACKEND "custom")
endif()
message(STATUS "Log backend: ${UTXOZ_LOG_BACKEND}")

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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utxoz/config.hpp
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
        src/statistics.cpp
        src/utils.cpp
        src/log.cpp
        src/database_impl.cpp
)

# Public headers
target_include_directories(utxoz
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

# Link dependencies
target_link_libraries(utxoz
    PUBLIC
        Boost::headers
        fmt::fmt
)

# Log backend configuration (defines are in config.hpp)
if(UTXOZ_LOG_BACKEND STREQUAL "spdlog")
    target_link_libraries(utxoz PRIVATE spdlog::spdlog)
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

# Examples
if(UTXOZ_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(UTXOZ_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
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

# Install headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)

# Install CMake config files
install(EXPORT utxoz-targets
    FILE utxoz-targets.cmake
    NAMESPACE utxoz::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/utxoz
)

# Create config file
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

# Package config
set(CPACK_PACKAGE_NAME "utxoz")
set(CPACK_PACKAGE_VERSION ${UTXOZ_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "Fernando Pelliccioni")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)
