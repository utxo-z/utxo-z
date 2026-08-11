#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# How verify_consumer.sh decides what it built, and what it refuses to run.
#
# The consumer check reports four distinct faults through four exit codes, and a
# release reads them differently: 1 and 2 are the package's problem, 3 says the
# build produced nothing runnable, 4 says it ran and failed. Two ways of losing
# that distinction were found by review of #113 and are pinned here.
#
# The first is that `find`'s status was collected inside a process substitution,
# which inherits `set -e`. A `find` that exited non-zero killed the subshell
# before the status was written, the read of the missing file failed, and the
# script exited 1 — which the caller maps to "does not configure". A search
# problem was reported as a broken package.
#
# The second is that the search matched `test_package*`. The build leaves plenty
# beside the executable that starts with that name, and Git Bash on Windows
# reports ordinary files as executable, so the check could pick an object file
# and "run" it.
#
# cmake is replaced by a script that emits whatever a case wants in the build
# tree. That is the whole point: these cases are about what the search does with
# a directory, not about compiling anything.
#
# Run from anywhere:  ci/test_verify_consumer.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT
readonly SCRIPT="${ROOT}/ci/verify_consumer.sh"

readonly EXIT_OK=0
readonly EXIT_CONFIGURE=1
readonly EXIT_COMPILE=2
readonly EXIT_NO_EXECUTABLE=3
readonly EXIT_RAN_AND_FAILED=4

failures=0

work="$(mktemp -d)"
trap 'chmod -R u+rwx "${work}" 2>/dev/null || true; rm -rf "${work}"' EXIT
mkdir -p "${work}/bin"
export PATH="${work}/bin:${PATH}"

# A cmake that configures, and on `--build` lays out exactly the files the case
# asked for. `EMIT` is a space-separated list of `name:mode` pairs.
cat > "${work}/bin/cmake" <<'FAKE'
#!/usr/bin/env bash
if [[ "$1" == "--build" ]]; then
    build_dir="$2"
    mkdir -p "${build_dir}"
    if [[ -n "${BLOCK_DIR:-}" ]]; then
        mkdir -p "${build_dir}/blocked"
        : > "${build_dir}/blocked/hidden"
        chmod 000 "${build_dir}/blocked"
    fi
    for spec in ${EMIT:-}; do
        name="${spec%%:*}"
        mode="${spec##*:}"
        path="${build_dir}/${name}"
        mkdir -p "$(dirname "${path}")"
        printf '#!/usr/bin/env bash\nexit %s\n' "${RUN_STATUS:-0}" > "${path}"
        chmod "${mode}" "${path}"
    done
    exit 0
fi
# configure
build_dir=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        -B) build_dir="$2"; shift 2 ;;
        *) shift ;;
    esac
done
mkdir -p "${build_dir}"
exit 0
FAKE
chmod +x "${work}/bin/cmake"

mkdir -p "${work}/consumer"
: > "${work}/consumer/CMakeLists.txt"

# run <emit-spec> [run-status] — one invocation against a fresh install dir.
run() {
    local emit="$1" run_status="${2:-0}"
    rm -rf "${work}/install"
    mkdir -p "${work}/install"
    : > "${work}/install/conan_toolchain.cmake"
    env EMIT="${emit}" RUN_STATUS="${run_status}" \
        "${SCRIPT}" "${work}/install" "${work}/consumer" 2>&1
}

# check <name> <expected code> <must say> <emit> [run-status]
check() {
    local name="$1" expect="$2" must_say="$3"; shift 3
    local out status=0
    out="$(run "$@")" || status=$?

    if [[ ${status} -ne ${expect} ]]; then
        printf 'FAIL  %-52s exited %d, expected %d\n' "${name}" "${status}" "${expect}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
        return
    fi
    if [[ -n "${must_say}" && "${out}" != *"${must_say}"* ]]; then
        printf 'FAIL  %-52s exited %d but did not say %q\n' "${name}" "${status}" "${must_say}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
        return
    fi
    printf 'ok    %-52s rc=%d\n' "${name}" "${status}"
}

echo "== the executable is found and run =="

check "exactly one executable is found and run" ${EXIT_OK} "configures, builds and runs" \
      "test_package:755"

# What Windows produces. The same search has to accept it, or the release check
# passes on Linux and macOS and fails on the platform #113 was about.
check "the Windows name is accepted" ${EXIT_OK} "configures, builds and runs" \
      "test_package.exe:755"

check "a consumer that runs and fails is its own fault" ${EXIT_RAN_AND_FAILED} "exited 3" \
      "test_package:755" 3

echo
echo "== what must not be picked =="

# The bug the exact-name match exists for. Every one of these begins with
# `test_package` and every one is executable — which is what Git Bash reports for
# ordinary files on Windows. None of them is the consumer.
check "an object file is not the consumer" ${EXIT_NO_EXECUTABLE} "produced no executable" \
      "test_package.cpp.o:755"
check "an import library is not the consumer" ${EXIT_NO_EXECUTABLE} "produced no executable" \
      "test_package.lib:755"
check "a PDB is not the consumer" ${EXIT_NO_EXECUTABLE} "produced no executable" \
      "test_package.pdb:755"
check "a build subdirectory is not the consumer" ${EXIT_NO_EXECUTABLE} "produced no executable" \
      "test_package.dir/test_package.cpp.obj:755"

# All of them at once, plus the real thing: the real one must be chosen, and the
# run must be the real one's.
check "the real executable is chosen from among them" ${EXIT_OK} "configures, builds and runs" \
      "test_package.cpp.o:755 test_package.lib:755 test_package.pdb:755 test_package:755"

echo
echo "== two candidates is not one =="

# Ambiguity is a fault, not a coin toss. Picking the first would report a pass
# for a tree the search cannot identify.
check "two executables named test_package are refused" ${EXIT_NO_EXECUTABLE} "holds 2 executables" \
      "test_package:755 sub/test_package:755"

echo
echo "== a search that fails keeps its own code and says why =="

# The #113 review finding. An unreadable build tree makes `find` exit non-zero;
# that has to arrive as 3 with the search diagnostic, never as 1, which the
# caller reads as "the package's CMake config is not usable".
#
# Skipped when the check would be vacuous: root ignores the permission bits, and
# a filesystem that does not enforce them (Windows) never fails the search.
unreadable_is_enforced() {
    local probe="${work}/probe"
    rm -rf "${probe}"; mkdir -p "${probe}/blocked"
    : > "${probe}/blocked/file"
    chmod 000 "${probe}/blocked" 2>/dev/null || return 1
    local ok=1
    find "${probe}" -name file > /dev/null 2>&1 && ok=0
    chmod 755 "${probe}/blocked" 2>/dev/null || true
    (( ok == 1 ))
}

if unreadable_is_enforced; then
    # The build tree is arranged by the fake cmake, after verify_consumer.sh has
    # cleared and configured it — a directory chmod'd 000 beforehand would only
    # make its own `rm -rf` fail, which is a different fault.
    status=0
    out="$(rm -rf "${work}/install" \
           && mkdir -p "${work}/install" \
           && : > "${work}/install/conan_toolchain.cmake" \
           && env EMIT="test_package:755" BLOCK_DIR=1 RUN_STATUS=0 \
                  "${SCRIPT}" "${work}/install" "${work}/consumer" 2>&1)" || status=$?
    chmod -R u+rwx "${work}/install" 2>/dev/null || true

    if [[ ${status} -eq ${EXIT_CONFIGURE} || ${status} -eq ${EXIT_COMPILE} ]]; then
        # Named rather than merely "not 3": these two are the package's fault,
        # and a search that could not run is not the package's fault. Reporting
        # it as either is what sends a release to look at the wrong thing.
        printf 'FAIL  %-52s exited %d, blaming the package for a search fault\n' \
               "an unreadable tree is a search fault" "${status}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
    elif [[ ${status} -ne ${EXIT_NO_EXECUTABLE} ]]; then
        printf 'FAIL  %-52s exited %d, expected %d\n' \
               "an unreadable tree is a search fault" "${status}" "${EXIT_NO_EXECUTABLE}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
    elif [[ "${out}" != *"cannot search the consumer build tree"* ]]; then
        printf 'FAIL  %-52s exited %d without the search diagnostic\n' \
               "an unreadable tree is a search fault" "${status}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
    elif [[ "${out}" == *"does not configure"* || "${out}" == *"produced no executable"* ]]; then
        # Both would be the conflation this case exists for: a search that could
        # not run reported as a package that does not build, or as one that
        # built nothing.
        printf 'FAIL  %-52s reported a search fault as something else\n' \
               "an unreadable tree is a search fault"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
    else
        printf 'ok    %-52s rc=%d\n' "an unreadable tree is a search fault" "${status}"
    fi
else
    printf 'skip  %-52s permissions are not enforced here\n' "an unreadable tree is a search fault"
fi

echo
if (( failures > 0 )); then
    echo "${failures} case(s) failed"
    exit 1
fi
echo "all cases passed"
