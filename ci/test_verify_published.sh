#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Which layer of the post-upload verification failed, and what it is allowed to
# say about the others.
#
# The 0.9.1 release published its recipe, uploaded a Windows binary, retrieved
# both from a clean cache, and then went red because the verification consumer
# included a header from a package it had not declared. Reading that run, the
# failure was indistinguishable from a release that never published (#113). So
# verify_published.sh now answers three questions separately, and the property
# under test here is the separation itself: an optional or diagnostic failure
# must never claim the required gate failed, and a gate that could not be
# established must never be reported as a gate that failed.
#
# None of that can be produced against a real remote on demand — a remote cannot
# be asked to serve a recipe and withhold a binary, and a release cannot be held
# open to find out. So conan and cmake are replaced by scripts that answer
# exactly what each case needs, and verify_published.sh runs its real logic
# against them: the real await_published.sh, the real remote_revisions.py, the
# real branching, the real verdict.
#
# The fake cmake is not a stub that always succeeds. It reads the consumer's
# CMakeLists.txt and refuses to configure one that does not link utxoz, which is
# what makes the negation case below mean something: a consumer with the utxoz
# dependency removed has to fail, or the diagnostic layer is measuring nothing.
# The same negation is run against a real toolchain in CI; this is the cheap
# copy of it that runs on every pull request.
#
# Run from anywhere:  ci/test_verify_published.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT
readonly SCRIPT="${ROOT}/ci/verify_published.sh"

readonly EXIT_OK=0
readonly EXIT_RECIPE_ABSENT=3
readonly EXIT_RECIPE_UNKNOWN=4
readonly EXIT_RECIPE_UNRETRIEVABLE=5
readonly EXIT_BINARY_MISSING=6
readonly EXIT_CONSUMER_FAILED=7

readonly VERSION="0.9.1"
readonly REVISION="cf72ca058a4808769921fffccd4ee5f0"
readonly OTHER_REVISION="544d3847f25a1b54b0fa518f1656381c"
readonly REMOTE="kth-verify"

failures=0

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT
mkdir -p "${work}/bin"
export PATH="${work}/bin:${PATH}"

# ---------------------------------------------------------------------------
# The doubles.
#
# One case directory holds one file per decision. A missing file means "behave",
# so a case only writes down the thing it is about.
# ---------------------------------------------------------------------------

cat > "${work}/bin/conan" <<'FAKE'
#!/usr/bin/env bash
# A conan that answers from the case directory. Only the verbs
# verify_published.sh actually uses are implemented; anything else is a test
# harness bug rather than a silent pass.
case_dir="${FAKE_CASE:?FAKE_CASE is required}"
verb="$1"
sub="${2:-}"

behaviour() { cat "${case_dir}/$1" 2>/dev/null || printf 'ok'; }

case "${verb}" in
    profile|remote)
        exit 0
        ;;
    list)
        # remote_revisions.py asks twice: existence, then `ref#*`. Both are
        # answered from the same file, so a case is one line rather than two.
        case "${sub}" in
            *'#*') cat "${case_dir}/list" ;;
            *)     cat "${case_dir}/list" ;;
        esac
        exit 0
        ;;
    download)
        if [[ "$(behaviour download)" != "ok" ]]; then
            echo "ERROR: Recipe not found or there are no matching packages" >&2
            exit 1
        fi
        echo "Downloading recipe"
        exit 0
        ;;
    cache)
        if [[ "$(behaviour cachepath)" != "ok" ]]; then
            echo "ERROR: Recipe 'utxoz' not found in the cache" >&2
            exit 1
        fi
        echo "/fake/cache/recipe"
        exit 0
        ;;
    install)
        if [[ "$(behaviour install)" != "ok" ]]; then
            echo "ERROR: Missing binary: utxoz/VERSION" >&2
            echo "ERROR: Try to build from sources with '--build=utxoz'" >&2
            exit 1
        fi
        # The output folder is what the consumer configures against, so the
        # toolchain file has to exist for the next layer to be reached at all.
        out=""
        while [[ $# -gt 0 ]]; do
            case "$1" in
                -of) out="$2"; shift 2 ;;
                *) shift ;;
            esac
        done
        [[ -n "${out}" ]] || { echo "the install had no -of" >&2; exit 70; }
        mkdir -p "${out}"
        echo "# fake toolchain" > "${out}/conan_toolchain.cmake"
        echo "Install finished successfully"
        exit 0
        ;;
    *)
        echo "the fake conan was asked for '${verb}', which it does not model" >&2
        exit 70
        ;;
esac
FAKE
chmod +x "${work}/bin/conan"

cat > "${work}/bin/cmake" <<'FAKE'
#!/usr/bin/env bash
# A cmake that models one property: a consumer configures when it declares the
# utxoz package, and does not when it does not. Everything else it is asked
# about, it agrees to — the compiler is not what these cases are about.
case_dir="${FAKE_CASE:?FAKE_CASE is required}"
behaviour() { cat "${case_dir}/$1" 2>/dev/null || printf 'ok'; }

if [[ "$1" == "--build" ]]; then
    build_dir="$2"
    case "$(behaviour compile)" in
        fail) echo "error: the consumer does not compile" >&2; exit 1 ;;
        noexe) echo "built nothing"; exit 0 ;;
    esac
    # Not `behaviour`: the default here is an exit status, not the word "ok".
    run_status="$(cat "${case_dir}/run_status" 2>/dev/null || printf '0')"
    printf '#!/usr/bin/env bash\nexit %s\n' "${run_status}" > "${build_dir}/test_package"
    chmod +x "${build_dir}/test_package"
    echo "built the consumer"
    exit 0
fi

# Configure. -S is the consumer, -B is where it goes.
source_dir="" build_dir=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        -S) source_dir="$2"; shift 2 ;;
        -B) build_dir="$2"; shift 2 ;;
        *) shift ;;
    esac
done
[[ -n "${source_dir}" && -n "${build_dir}" ]] || { echo "configure without -S/-B" >&2; exit 70; }

if [[ "$(behaviour configure)" != "ok" ]]; then
    echo "error: the consumer does not configure" >&2
    exit 1
fi

# The part that makes the negation case non-vacuous: a consumer that does not
# ask for utxoz cannot be configured against it.
if ! grep -q 'utxoz::utxoz' "${source_dir}/CMakeLists.txt"; then
    echo "error: no target links utxoz::utxoz; nothing declares the package under test" >&2
    exit 1
fi
if ! grep -q 'find_package(utxoz' "${source_dir}/CMakeLists.txt"; then
    echo "error: the consumer never calls find_package(utxoz)" >&2
    exit 1
fi

mkdir -p "${build_dir}"
echo "configured the consumer"
exit 0
FAKE
chmod +x "${work}/bin/cmake"

# ---------------------------------------------------------------------------
# The remote's replies.
# ---------------------------------------------------------------------------
readonly PUBLISHED='{"'"${REMOTE}"'":{"utxoz/'"${VERSION}"'":{"revisions":{"'"${REVISION}"'":{"timestamp":1}}}}}'
readonly ABSENT='{"'"${REMOTE}"'":{"error":"Recipe '"'"'utxoz/'"${VERSION}"''"'"' not found"}}'
readonly UNREACHABLE='{"'"${REMOTE}"'":{"error":"HTTPSConnectionPool: Max retries exceeded"}}'
readonly OTHER_ONLY='{"'"${REMOTE}"'":{"utxoz/'"${VERSION}"'":{"revisions":{"'"${OTHER_REVISION}"'":{"timestamp":1}}}}}'

# A consumer that declares what it uses, and one that does not.
mkdir -p "${work}/consumer-ok"
cat > "${work}/consumer-ok/CMakeLists.txt" <<'CM'
cmake_minimum_required(VERSION 3.20)
project(test_package LANGUAGES CXX)
find_package(utxoz CONFIG REQUIRED)
add_executable(test_package src/test_package.cpp)
target_link_libraries(test_package PRIVATE utxoz::utxoz)
CM

mkdir -p "${work}/consumer-no-utxoz"
cat > "${work}/consumer-no-utxoz/CMakeLists.txt" <<'CM'
cmake_minimum_required(VERSION 3.20)
project(test_package LANGUAGES CXX)
add_executable(test_package src/test_package.cpp)
CM

# ---------------------------------------------------------------------------
# The harness.
# ---------------------------------------------------------------------------

# run <list-reply> <consumer-dir> [key=value...] — one invocation of the script.
run() {
    local list_reply="$1" consumer="$2"; shift 2
    local dir="${work}/case"
    rm -rf "${dir}"; mkdir -p "${dir}"
    printf '%s' "${list_reply}" > "${dir}/list"

    local spec
    for spec in "$@"; do
        printf '%s' "${spec#*=}" > "${dir}/${spec%%=*}"
    done

    env FAKE_CASE="${dir}" \
        UTXOZ_BUILD_VERSION="${VERSION}" \
        UTXOZ_RECIPE_REVISION="${REVISION}" \
        UTXOZ_REMOTE="${REMOTE}" \
        UTXOZ_CONSUMER_DIR="${consumer}" \
        UTXOZ_VERIFY_RESULT_FILE="${work}/result" \
        UTXOZ_PUBLISH_TIMEOUT=1 \
        "${SCRIPT}" 2>&1
}

# check <name> <expected code> <must say> <must not say> <run args...>
#
# The "must not say" half is the whole point of the suite. Every case below that
# fails a layer asserts that the verdict does not contain the word the release
# would be read from — a binary failure that prints ABSENT anywhere is the #113
# bug returning under a new name.
check() {
    local name="$1" expect_code="$2" must_say="$3" must_not_say="$4"; shift 4
    local out status=0
    out="$(run "$@")" || status=$?

    if [[ ${status} -ne ${expect_code} ]]; then
        printf 'FAIL  %-46s exited %d, expected %d\n' "${name}" "${status}" "${expect_code}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
        return
    fi
    if [[ "${out}" != *"${must_say}"* ]]; then
        printf 'FAIL  %-46s exited %d but did not say %q\n' "${name}" "${status}" "${must_say}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
        return
    fi
    if [[ -n "${must_not_say}" && "${out}" == *"${must_not_say}"* ]]; then
        printf 'FAIL  %-46s exited %d and wrongly said %q\n' "${name}" "${status}" "${must_not_say}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
        return
    fi
    printf 'ok    %-46s rc=%d\n' "${name}" "${status}"
}

# check_result <name> <key> <expected substring> — reads the machine-readable
# verdict left by the run immediately before it.
check_result() {
    local name="$1" key="$2" expected="$3"
    local line
    line="$(grep "^${key}=" "${work}/result" || true)"
    if [[ "${line}" != *"${expected}"* ]]; then
        printf 'FAIL  %-46s %s was %q, expected to contain %q\n' \
               "${name}" "${key}" "${line#*=}" "${expected}"
        failures=$((failures + 1))
        return
    fi
    printf 'ok    %-46s %s\n' "${name}" "${line}"
}

echo "== everything holds =="

check "all three layers pass" ${EXIT_OK} "ok: utxoz/${VERSION}#${REVISION} is on" "" \
      "${PUBLISHED}" "${work}/consumer-ok"
check_result "all three layers pass" recipe "PUBLISHED"
check_result "all three layers pass" binary "retrieved with --build=never"
check_result "all three layers pass" consumer "compiles, links and runs"

echo
echo "== the required gate =="

# The remote's own words. This is the only case allowed to say the release did
# not publish.
check "recipe absent is absence" ${EXIT_RECIPE_ABSENT} "ABSENT" "" \
      "${ABSENT}" "${work}/consumer-ok"
check_result "recipe absent is absence" binary "not attempted"

# The reference is there from an earlier release and this revision is not. Still
# absence — the remote answered — but about the revision, not the name.
check "wrong revision only is absence" ${EXIT_RECIPE_ABSENT} "ABSENT" "" \
      "${OTHER_ONLY}" "${work}/consumer-ok"

# The distinction #92 was made of, and the one this suite exists to keep: a
# remote that cannot be reached has not told us anything.
check "unreachable is not absence" ${EXIT_RECIPE_UNKNOWN} "UNDETERMINED" "ABSENT" \
      "${UNREACHABLE}" "${work}/consumer-ok"
check_result "unreachable is not absence" recipe "UNDETERMINED"

# Listed and not retrievable. The gate fails, and it is still not absence: the
# remote named the revision.
check "listed but not retrievable" ${EXIT_RECIPE_UNRETRIEVABLE} "NOT RETRIEVABLE" "ABSENT" \
      "${PUBLISHED}" "${work}/consumer-ok" "download=fail"

# A download that reports success and leaves nothing in the cache is the same
# failure wearing a success message — #92 in miniature.
check "downloaded but not in the cache" ${EXIT_RECIPE_UNRETRIEVABLE} "not in the cache" "ABSENT" \
      "${PUBLISHED}" "${work}/consumer-ok" "cachepath=fail"

echo
echo "== the optional binary, which is not the gate =="

check "no binary under --build=never" ${EXIT_BINARY_MISSING} "NOT RETRIEVED" "ABSENT" \
      "${PUBLISHED}" "${work}/consumer-ok" "install=fail"
# The layer that matters: the release still published, and the verdict says so.
check_result "no binary: recipe still published" recipe "PUBLISHED"
check_result "no binary: consumer not reached" consumer "not attempted"

echo
echo "== the diagnostic consumer, which is not the gate either =="

check "consumer will not configure" ${EXIT_CONSUMER_FAILED} "does not configure" "ABSENT" \
      "${PUBLISHED}" "${work}/consumer-ok" "configure=fail"
check_result "consumer will not configure" recipe "PUBLISHED"
check_result "consumer will not configure" binary "retrieved"

check "consumer will not compile" ${EXIT_CONSUMER_FAILED} "does not compile or link" "ABSENT" \
      "${PUBLISHED}" "${work}/consumer-ok" "compile=fail"
check_result "consumer will not compile" recipe "PUBLISHED"

check "consumer builds no executable" ${EXIT_CONSUMER_FAILED} "produced no executable" "ABSENT" \
      "${PUBLISHED}" "${work}/consumer-ok" "compile=noexe"

# A consumer that compiles and then fails at run time is a broken package too:
# 0.9.1's would have linked and died opening a database, and that has to be as
# red as a compile error.
check "consumer runs and fails" ${EXIT_CONSUMER_FAILED} "exited 1" "ABSENT" \
      "${PUBLISHED}" "${work}/consumer-ok" "run_status=1"
check_result "consumer runs and fails" recipe "PUBLISHED"

echo
echo "== the negation: the diagnostic can fail =="

# Without this the suite above proves only that a check which always passes,
# passes. A consumer with the utxoz dependency removed must fail — and must fail
# in the consumer layer, exit 7, with the recipe still reported as published.
check "a consumer without utxoz fails" ${EXIT_CONSUMER_FAILED} "does not configure" "ABSENT" \
      "${PUBLISHED}" "${work}/consumer-no-utxoz"
check_result "a consumer without utxoz fails" recipe "PUBLISHED"
check_result "a consumer without utxoz fails" consumer "FAILED"

echo
echo "== the real consumer declares what it includes =="

# The 0.9.1 failure, asserted against the file rather than against a rebuild of
# it: every include in the verification consumer comes from utxoz or from the
# standard library, because CMakeLists.txt declares utxoz and nothing else.
consumer_source="${ROOT}/test_package/src/test_package.cpp"
undeclared="$(grep -o '#include <[a-z0-9_]*/[^>]*>' "${consumer_source}" | grep -v '<utxoz/' || true)"
if [[ -n "${undeclared}" ]]; then
    printf 'FAIL  %-46s includes a package it does not declare:\n' "consumer includes only what it declares"
    printf '%s\n' "${undeclared}" | sed 's/^/        /'
    failures=$((failures + 1))
else
    printf 'ok    %-46s only utxoz and the standard library\n' "consumer includes only what it declares"
fi

# And the declaration itself is still there to be relied on.
if grep -q 'find_package(utxoz' "${ROOT}/test_package/CMakeLists.txt" \
   && grep -q 'utxoz::utxoz' "${ROOT}/test_package/CMakeLists.txt"; then
    printf 'ok    %-46s find_package(utxoz) and utxoz::utxoz\n' "consumer declares utxoz"
else
    printf 'FAIL  %-46s the consumer no longer declares utxoz\n' "consumer declares utxoz"
    failures=$((failures + 1))
fi

echo
if (( failures > 0 )); then
    echo "${failures} case(s) failed"
    exit 1
fi
echo "all cases passed"
