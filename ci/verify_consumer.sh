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

# Both directories are made absolute before anything is derived from them.
#
# BUILD_DIR comes from INSTALL_DIR, and `find` prints paths that begin with the
# directory it was given — so a relative INSTALL_DIR yields a relative candidate.
# The run below deliberately happens from a scratch directory, because the
# consumer creates databases under its working directory, and a relative path
# resolved from there names nothing. The exec fails with 127 and arrives as
# EXIT_RAN_AND_FAILED: "it built and then did not work", about a consumer that
# was never started.
#
# `cd && pwd` rather than `realpath`, which is GNU coreutils and not on a stock
# macOS. It also needs the directory to exist, which is why the checks come
# first and keep their own diagnostics.
[[ -d "$1" ]] || { echo "FAIL: no such install directory: $1" >&2; exit ${EXIT_CONFIGURE}; }
[[ -d "$2" ]] || { echo "FAIL: no such consumer directory: $2" >&2; exit ${EXIT_CONFIGURE}; }

INSTALL_DIR="$(cd "$1" && pwd)"
CONSUMER_DIR="$(cd "$2" && pwd)"
readonly INSTALL_DIR CONSUMER_DIR
readonly TOOLCHAIN="${INSTALL_DIR}/conan_toolchain.cmake"

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
# --config Release, not just CMAKE_BUILD_TYPE at configure time.
#
# On a multi-config generator — which is what CMake picks by default on Windows —
# CMAKE_BUILD_TYPE is ignored and `cmake --build` with no --config builds Debug.
# The package installed above has Release binaries only, so CMakeDeps has no data
# file for the Debug config: the utxoz target comes out with no include
# directories and the consumer fails on `utxoz/config.hpp`, which reads exactly
# like a package that ships no headers. It ships them; this was asking for the
# wrong configuration. Single-config generators ignore the flag.
if ! cmake --build "${BUILD_DIR}" --config Release --parallel > "${work}/build.log" 2>&1; then
    cat "${work}/build.log" >&2
    echo "FAIL: the consumer does not compile or link against the installed package" >&2
    exit ${EXIT_COMPILE}
fi

# A search that failed and a search that found nothing are different faults, and
# only one of them is the package's. So the results go to a file, the status is
# taken from `find` itself, and the two are read separately.
#
# Not a process substitution. The subshell inherits `set -e`, so a `find` that
# exits non-zero — an unreadable directory, a bad predicate — kills the subshell
# at that command and the status is never written. Reading the missing file then
# fails, `set -e` exits 1, and the caller maps 1 to "does not configure": the
# exact conflation this block exists to prevent, reintroduced by the machinery
# meant to prevent it. Redirecting to a file keeps `find` in this shell, where
# `|| find_status=$?` is what stops errexit rather than something that has to
# survive it.
#
# The name is exact. `test_package*` also matches what the build leaves beside
# the executable — `test_package.cpp.o`, `test_package.dir`, import libraries,
# PDBs — and on a filesystem that reports every file as executable, which is what
# Git Bash on Windows does, one of those is picked and run instead. The CMake
# target is `test_package`; the only platform variation is the `.exe` suffix.
find_status=0
find "${BUILD_DIR}" \
     \( -name 'test_package' -o -name 'test_package.exe' \) \
     -type f -perm -u+x -print0 > "${work}/candidates" 2>"${work}/find-errors" \
    || find_status=$?

if (( find_status != 0 )); then
    cat "${work}/find-errors" >&2
    echo "FAIL: cannot search the consumer build tree (find exited ${find_status})" >&2
    exit ${EXIT_NO_EXECUTABLE}
fi

candidates=()
while IFS= read -r -d '' candidate; do
    candidates+=("${candidate}")
done < "${work}/candidates"

if (( ${#candidates[@]} == 0 )); then
    echo "FAIL: the consumer built but produced no executable" >&2
    exit ${EXIT_NO_EXECUTABLE}
fi

# More than one is not a worse version of one. A build tree holding two things
# called `test_package` means the search is not identifying what it thinks it is,
# and picking the first would hide that behind a pass.
if (( ${#candidates[@]} > 1 )); then
    echo "FAIL: the consumer build tree holds ${#candidates[@]} executables named test_package:" >&2
    printf '  %s\n' "${candidates[@]}" >&2
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
