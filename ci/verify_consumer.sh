#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Configure, build and run a consumer against an already-installed utxoz.
#
# This is the diagnostic layer of the post-upload verification, on its own so
# that it can be pointed at something other than a release. verify_published.sh
# calls it with what came back from the remote; the pull-request job calls it
# twice against the package `conan create` just built — once with the real
# consumer, which must pass, and once with a copy whose utxoz dependency has
# been removed, which must fail.
#
# That second call is the reason this is a separate file. A verification that
# cannot fail verifies nothing, and until 0.9.1 nothing established that this
# one could: the consumer had never been run against a package it should not
# have been able to use. When it finally did fail, it failed for a header it
# should never have been including (#113).
#
# The failure modes are separate exit codes because they are separate faults,
# and the caller writes a different sentence for each:
#
#   0  configured, built and ran
#   1  did not configure — the package's CMake config is not usable
#   2  did not compile or link — headers or library are missing or broken
#   3  built, but no executable can be found to run
#   4  ran and exited non-zero — it links and does not work
#
# Usage:  verify_consumer.sh <install-dir> <consumer-dir>
#
#   install-dir   a directory holding conan_toolchain.cmake, as produced by
#                 `conan install -g CMakeToolchain -of <dir>`
#   consumer-dir  the CMake project to build; test_package by default

set -euo pipefail

readonly EXIT_OK=0
readonly EXIT_CONFIGURE=1
readonly EXIT_COMPILE=2
readonly EXIT_NO_EXECUTABLE=3
readonly EXIT_RAN_AND_FAILED=4

if [[ $# -ne 2 ]]; then
    echo "usage: $0 <install-dir> <consumer-dir>" >&2
    exit ${EXIT_CONFIGURE}
fi

readonly INSTALL_DIR="$1"
readonly CONSUMER_DIR="$2"
readonly TOOLCHAIN="${INSTALL_DIR}/conan_toolchain.cmake"

[[ -d "${CONSUMER_DIR}" ]] || { echo "FAIL: no such consumer directory: ${CONSUMER_DIR}" >&2; exit ${EXIT_CONFIGURE}; }
[[ -f "${TOOLCHAIN}" ]] || { echo "FAIL: no conan_toolchain.cmake in ${INSTALL_DIR}" >&2; exit ${EXIT_CONFIGURE}; }

readonly BUILD_DIR="${INSTALL_DIR}/consumer-build"
# A build tree of its own per consumer, so the negative control cannot pass by
# finding the previous run's executable still sitting there.
rm -rf "${BUILD_DIR}"

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT

echo "configuring ${CONSUMER_DIR}"
if ! cmake -S "${CONSUMER_DIR}" -B "${BUILD_DIR}" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" > "${work}/cmake.log" 2>&1; then
    cat "${work}/cmake.log" >&2
    echo "FAIL: the consumer does not configure against the installed package" >&2
    exit ${EXIT_CONFIGURE}
fi

echo "building ${CONSUMER_DIR}"
if ! cmake --build "${BUILD_DIR}" --parallel > "${work}/build.log" 2>&1; then
    cat "${work}/build.log" >&2
    echo "FAIL: the consumer does not compile or link against the installed package" >&2
    exit ${EXIT_COMPILE}
fi

# `find | head` reports head's status, so a find that failed — an unreadable
# directory, a bad predicate — would look exactly like "no executable was built".
# The results are collected first and the status is checked on its own.
candidates=()
while IFS= read -r -d '' candidate; do
    candidates+=("${candidate}")
done < <(find "${BUILD_DIR}" -name 'test_package*' -type f -perm -u+x -print0; \
         printf '%d' $? > "${work}/find-status")
find_status="$(cat "${work}/find-status")"
if (( find_status != 0 )); then
    echo "FAIL: cannot search the consumer build tree (find exited ${find_status})" >&2
    exit ${EXIT_NO_EXECUTABLE}
fi
if (( ${#candidates[@]} == 0 )); then
    echo "FAIL: the consumer built but produced no executable" >&2
    exit ${EXIT_NO_EXECUTABLE}
fi

# Run from a directory of its own: the consumer creates databases under its
# working directory, and two runs sharing one would open each other's files.
run=0
( cd "${work}" && "${candidates[0]}" ) || run=$?
if (( run != 0 )); then
    echo "FAIL: the consumer built but exited ${run}" >&2
    exit ${EXIT_RAN_AND_FAILED}
fi

echo "ok: the consumer configures, builds and runs"
exit ${EXIT_OK}
