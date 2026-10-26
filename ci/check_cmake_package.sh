#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# The CMake package a plain `cmake --install` produces must be usable by
# `find_package(utxoz CONFIG REQUIRED)`.
#
# Every other consumer check in this repository goes through Conan, and a Conan
# build sets UTXOZ_CONAN_BUILD, which skips the config file entirely: Conan's
# CMakeDeps writes its own. So the config installed for everyone else was never
# loaded by anything, and it could not have been. It included a targets file
# under another project's name, asked for compiled Boost components the library
# does not link, checked another package's components, and recorded an empty
# version. This script is the regression, and it fails on that template at its
# first consumer, before reaching the rest.
#
# Nothing here uses Conan's toolchain. Conan only fetches the dependencies, the
# way a system package manager would, and they reach CMake through
# CMAKE_PREFIX_PATH and nothing else. Neither the library build nor the consumer
# is told anything a non-Conan user would not tell it.
#
# Run from the repository root:  ci/check_cmake_package.sh [work-dir]

set -euo pipefail

readonly ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly WORK="${1:-$(mktemp -d)}"

# Numeric, unlike the other check scripts' versions, so that a consumer can ask
# for it: find_package() takes a version request in digits only.
readonly VERSION="1.2.3"

readonly CONAN_SETTINGS=(-pr:h "${ROOT}/ci/conan-cpp23" -s build_type=Release)

# The recipe's CMakeToolchain would otherwise write CMakeUserPresets.json into
# the source tree, which every build directory shares. Ignored by git, but still
# state that this script has no business leaving behind.
readonly CONAN_CONF=(-c "tools.cmake.cmaketoolchain:user_presets=")

mkdir -p "${WORK}"
cd "${ROOT}"

fail() { echo "FAIL: $*" >&2; exit 1; }
pass() { echo "ok: $*"; }

# Dependencies for one log backend, as CMake config files in a directory.
fetch_dependencies() {
    local backend="$1"
    local out="${WORK}/deps-${backend}"
    conan install . "${CONAN_SETTINGS[@]}" "${CONAN_CONF[@]}" -of "${out}" \
        --version="${VERSION}" --build=missing \
        -o log="${backend}" -o with_tests=False -o with_benchmarks=False \
        -o with_large_benchmarks=False -o with_examples=False \
        > "${out}.log" 2>&1 \
        || { cat "${out}.log"; fail "fetching the dependencies for log=${backend}"; }
    echo "${out}/build/Release/generators"
}

# Build and install the library without Conan, then build and run the Conan test
# package's consumer against the install — the same source, through plain CMake.
# The consumer names one package, utxoz, so whatever else it needs has to arrive
# through utxoz's config.
check_case() {
    local name="$1" backend="$2" shared="$3" deps="$4"
    local build="${WORK}/${name}/build"
    local prefix="${WORK}/${name}/prefix"
    local consumer="${WORK}/${name}/consumer"
    local run="${WORK}/${name}/run"

    cmake -S "${ROOT}" -B "${build}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="${deps}" \
        -DCMAKE_INSTALL_PREFIX="${prefix}" \
        -DBUILD_SHARED_LIBS="${shared}" \
        -DUTXOZ_VERSION="${VERSION}" \
        -DUTXOZ_LOG_BACKEND="${backend}" \
        -DUTXOZ_BUILD_TESTS=OFF \
        -DUTXOZ_BUILD_EXAMPLES=OFF \
        -DUTXOZ_BUILD_BENCHMARKS=OFF \
        > "${WORK}/${name}-configure.log" 2>&1 \
        || { cat "${WORK}/${name}-configure.log"; fail "${name}: configuring the library"; }
    cmake --build "${build}" --parallel > "${WORK}/${name}-build.log" 2>&1 \
        || { cat "${WORK}/${name}-build.log"; fail "${name}: building the library"; }
    cmake --install "${build}" > "${WORK}/${name}-install.log" 2>&1 \
        || { cat "${WORK}/${name}-install.log"; fail "${name}: installing the library"; }

    local config
    config="$(find "${prefix}" -path '*/cmake/utxoz/utxoz-config.cmake' -print -quit)"
    [[ -n "${config}" ]] || fail "${name}: the install has no utxoz-config.cmake"

    cmake -S "${ROOT}/test_package" -B "${consumer}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="${prefix};${deps}" \
        > "${WORK}/${name}-consumer-configure.log" 2>&1 \
        || { cat "${WORK}/${name}-consumer-configure.log"; fail "${name}: find_package(utxoz CONFIG REQUIRED) did not configure"; }
    cmake --build "${consumer}" --parallel > "${WORK}/${name}-consumer-build.log" 2>&1 \
        || { cat "${WORK}/${name}-consumer-build.log"; fail "${name}: the consumer did not compile or link"; }

    # From a directory of its own: the consumer creates its databases under the
    # working directory.
    mkdir -p "${run}"
    (cd "${run}" && "${consumer}/test_package") > "${WORK}/${name}-consumer-run.log" 2>&1 \
        || { cat "${WORK}/${name}-consumer-run.log"; fail "${name}: the consumer ran and failed"; }
    grep -q "^utxoz ${VERSION}\$" "${WORK}/${name}-consumer-run.log" \
        || { cat "${WORK}/${name}-consumer-run.log"; fail "${name}: the consumer does not report version ${VERSION}"; }
    pass "${name}: installed without Conan, found by find_package, linked and ran"
}

# A configure-only consumer that asks for a version, so the version file is read
# and what it records is compared. Arguments after the third go to cmake as they
# are. Returns the status of the configure; its output is in <dir>/configure.log.
#
# The directory has to be new. A probe that found its directory already there
# would either read a previous run's cache or have to delete what it found, and
# this script deletes nothing it was not the one to create.
probe() {
    local dir="$1" request="$2" prefix_path="$3"
    shift 3
    if ! mkdir "${dir}"; then
        echo "FAIL: ${dir} already exists; a probe needs a directory of its own" >&2
        exit 1
    fi
    mkdir "${dir}/src"
    cat > "${dir}/src/CMakeLists.txt" <<PROBE
cmake_minimum_required(VERSION 3.20)
project(utxoz_package_probe LANGUAGES CXX)
find_package(utxoz ${request} CONFIG REQUIRED)
if(NOT utxoz_VERSION STREQUAL "${VERSION}")
    message(FATAL_ERROR "utxoz_VERSION is '\${utxoz_VERSION}', expected '${VERSION}'")
endif()
if(NOT TARGET utxoz::utxoz)
    message(FATAL_ERROR "the package defines no utxoz::utxoz")
endif()
PROBE
    cmake -S "${dir}/src" -B "${dir}/build" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="${prefix_path}" "$@" > "${dir}/configure.log" 2>&1
}

custom_deps="$(fetch_dependencies custom)"
spdlog_deps="$(fetch_dependencies spdlog)"
none_deps="$(fetch_dependencies none)"

# ---------------------------------------------------------------------------
# 1. Every log backend, and the shared library the recipe also offers.
#
# spdlog is the case a rename alone would miss. The library links it privately,
# but a static archive's private links are exported as $<LINK_ONLY:...>, so the
# consumer needs spdlog's target to exist without ever having asked for spdlog.
# ---------------------------------------------------------------------------
check_case custom        custom OFF "${custom_deps}"
check_case spdlog        spdlog OFF "${spdlog_deps}"
check_case none          none   OFF "${none_deps}"
check_case custom-shared custom ON  "${custom_deps}"

# ---------------------------------------------------------------------------
# 2. The version file records UTXOZ_VERSION, and SameMajorVersion means it.
# ---------------------------------------------------------------------------
probe "${WORK}/probe-same-major" 1.2 "${WORK}/custom/prefix;${custom_deps}" \
    || { cat "${WORK}/probe-same-major/configure.log"; fail "find_package(utxoz 1.2) refused version ${VERSION}"; }
pass "find_package(utxoz 1.2) accepts ${VERSION} and reports it as utxoz_VERSION"

if probe "${WORK}/probe-other-major" 2 "${WORK}/custom/prefix;${custom_deps}"; then
    fail "find_package(utxoz 2) accepted version ${VERSION}"
fi
grep -q "${VERSION}" "${WORK}/probe-other-major/configure.log" \
    || { cat "${WORK}/probe-other-major/configure.log"; fail "find_package(utxoz 2) failed, but not over the version"; }
pass "find_package(utxoz 2) refuses ${VERSION}"

# ---------------------------------------------------------------------------
# 3. The spdlog package asks for spdlog itself.
#
# Without it, the failure is a generate-time "target not found" in the
# consumer's build, about a target the consumer never named. With it, the
# package says which dependency is missing.
#
# spdlog is made missing by CMAKE_DISABLE_FIND_PACKAGE_spdlog rather than by
# leaving it off the search path. Leaving it off proves nothing on a host that
# has spdlog installed system-wide or in a package registry: it is found there,
# and the probe fails to fail. Disabled, find_package(spdlog) cannot succeed
# from anywhere — and spdlog stays on the path, so a package that never asks for
# it configures regardless, which is the failure this exists to report.
#
# The control is the same probe without the disable. It has to configure, so
# that the refusal below is the disable's doing and not something else broken.
# ---------------------------------------------------------------------------
readonly SPDLOG_PROBE_PATH="${WORK}/spdlog/prefix;${spdlog_deps}"

probe "${WORK}/probe-spdlog-control" "" "${SPDLOG_PROBE_PATH}" \
    || { cat "${WORK}/probe-spdlog-control/configure.log"; fail "the spdlog package did not configure with spdlog available"; }

if probe "${WORK}/probe-spdlog-disabled" "" "${SPDLOG_PROBE_PATH}" -DCMAKE_DISABLE_FIND_PACKAGE_spdlog=ON; then
    fail "the spdlog package configured with find_package(spdlog) disabled: it does not ask for spdlog"
fi
# The error CMake gives when a REQUIRED lookup meets the disable, and not just
# the variable's name: that also appears in the warning a project gets for a
# variable it never read, which is what a package that skips spdlog produces.
grep -q 'find_package for module spdlog called with REQUIRED' "${WORK}/probe-spdlog-disabled/configure.log" \
    || { cat "${WORK}/probe-spdlog-disabled/configure.log"; fail "the spdlog package failed, but not over spdlog"; }
pass "the spdlog package asks for spdlog, and fails over it when it cannot be found"

echo "all checks passed"
