#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Builds the store against a Boost newer than the one it pins, and reports what
# that Boost does to a database written under the certified layout.
#
# Informative on purpose. A newer Boost is not a defect, and this job going red
# would say nothing about the code in the pull request that happened to be open
# when Boost published a release. What it produces is a verdict, and the verdict
# is a thing a person reads before deciding whether `map_layout_epoch` moves.
#
# The verdicts, and what each one means:
#
#   NO_EVIDENCE         the fixtures were not there to test against. Nothing was
#                       proven, and this never reads as compatibility.
#   DOES_NOT_BUILD      the store does not compile against it. Nothing is known
#                       about the format; somebody has to look.
#   BUILDS_ONLY         it compiled and the tests could not be run here. This is
#                       *not* compatibility, and is never reported as such.
#   REFUSED_BY_IDENTITY it ran and declined to open the fixtures — the barrier
#                       working. Expected if an epoch was deliberately moved.
#   LOGICALLY_COMPATIBLE it opened the fixtures and every entry came back, digest
#                       for digest. This is the one that justifies keeping the
#                       epoch where it is.
#   LOGICAL_MISMATCH    it opened them and the contents are not what was stored.
#                       The worst outcome, and the reason this job exists.
#
# The Boost it fetches is pinned by version and checksum, or by an exact commit.
# Nothing floating: a run that cannot say which bytes it tested proves nothing
# later.

set -euo pipefail

readonly DEFAULT_VERSION="1.92.0"
readonly DEFAULT_SHA256="e2a814b3a158ab482c7a3d330f8bf5a7a8423d258a4e2fb396996e00fcee2111"

BOOST_VERSION="${BOOST_VERSION:-${DEFAULT_VERSION}}"
BOOST_SHA256="${BOOST_SHA256:-${DEFAULT_SHA256}}"
BOOST_COMMIT="${BOOST_COMMIT:-}"

# The two answers are reported separately and never merged. "It reads what we
# wrote" and "it writes what we write" fail independently, and a single word
# covering both always overstates one of them.
physical="NO_EVIDENCE"
physical_note="the writer comparison did not run."

verdict() {
    echo "verdict=$1"
    echo "physical=${physical}"
    if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
        {
            echo "### Boost compatibility probe"
            echo
            echo "| | |"
            echo "|---|---|"
            echo "| Boost | ${2} |"
            echo "| Reader (logical) | \`$1\` |"
            echo "| Writer (physical) | \`${physical}\` |"
            echo
            echo "$3"
            echo
            echo "Writer: ${physical_note}"
        } >> "${GITHUB_STEP_SUMMARY}"
    fi
    echo "$3"
    echo "writer: ${physical_note}"
}

# ---- comparing two trees -----------------------------------------------------
#
# Sets `identical`, `differing`, `only_candidate` and `only_baseline`, and prints
# a line per difference.
#
# Both sides are enumerated, not just the baseline. A candidate that writes a
# file the baseline does not — an extra generation, a file named differently —
# would otherwise never be visited, and every file that was visited would match.
compare_trees() {
    local base="$1" cand="$2"
    identical=0; differing=0; only_candidate=0; only_baseline=0

    local names="${work}/names.$$"
    { (cd "${base}" && find . -type f -name '*.dat' -print)
      (cd "${cand}" && find . -type f -name '*.dat' -print); } | sort -u > "${names}"

    local rel b c
    while IFS= read -r rel; do
        b="${base}/${rel}"
        c="${cand}/${rel}"
        if [[ ! -f "${b}" ]]; then
            only_candidate=$(( only_candidate + 1 ))
            echo "written only by this Boost: ${rel}"
        elif [[ ! -f "${c}" ]]; then
            only_baseline=$(( only_baseline + 1 ))
            echo "not written by this Boost: ${rel}"
        elif [[ "${rel##*/}" == "utxoz_config.dat" ]]; then
            # The one field that is expected to differ, and the only one that is
            # allowed to: the config records which Boost wrote it, by design and
            # for diagnosis. Normalised rather than skipped, so that a difference
            # anywhere else in the config is still a difference.
            normalise_config "${b}" "${work}/base.cfg"
            normalise_config "${c}" "${work}/cand.cfg"
            if cmp -s "${work}/base.cfg" "${work}/cand.cfg"; then
                identical=$(( identical + 1 ))
            else
                differing=$(( differing + 1 ))
                echo "the config differs in more than the Boost it records: ${rel}"
            fi
        elif cmp -s "${b}" "${c}"; then
            identical=$(( identical + 1 ))
        else
            differing=$(( differing + 1 ))
            echo "written differently by this Boost: ${rel} (first at byte $(cmp "${b}" "${c}" 2>&1 | sed -n 's/.*byte \([0-9]*\).*/\1/p'))"
        fi
    done < "${names}"
    rm -f "${names}"
}

# Zeroes bytes 44..51 of a config: `boost_version`, documented in
# src/detail/store_config_io.hpp as diagnostic and never compared by the reader,
# and the checksum that follows it.
#
# The checksum has to go with it. It covers everything before it, so a config
# recording a different Boost differs twice — in the field and in the checksum of
# the field — and normalising only the first leaves the second looking like a
# finding. Nothing is lost by dropping it: it is a function of bytes 0..47, and
# those are compared directly, so a difference it could reveal is one already
# visible in the field that caused it.
normalise_config() {
    cp "$1" "$2"
    printf '\0\0\0\0\0\0\0\0' | dd of="$2" bs=1 seek=44 count=8 conv=notrunc status=none 2>/dev/null
}

# ---- the comparison, checked against known answers ---------------------------
#
# A comparison is only worth what it can fail on. `--self-test` builds trees whose
# answers are known and requires the classification to match, so that a mistake in
# the loop above shows up here rather than as a confident "identical".
if [[ "${1:-}" == "--self-test" ]]; then
    work="$(mktemp -d)"
    trap 'rm -rf "${work}"' EXIT
    failures=0

    expect() {
        local what="$1" want="$2" got="$3"
        if [[ "${want}" != "${got}" ]]; then
            echo "SELF-TEST FAILED: ${what}: expected ${want}, got ${got}" >&2
            failures=$(( failures + 1 ))
        else
            echo "ok: ${what}"
        fi
    }

    make_tree() {
        mkdir -p "$1/fixture"
        head -c 200 /dev/zero > "$1/fixture/cont_0_v00000.dat"
        head -c 52 /dev/zero > "$1/fixture/utxoz_config.dat"
    }

    rm -rf "${work}/a" "${work}/b"; make_tree "${work}/a"; make_tree "${work}/b"
    compare_trees "${work}/a" "${work}/b" > /dev/null
    expect "identical trees" "2 0 0 0" "${identical} ${differing} ${only_candidate} ${only_baseline}"

    # The one this exists for: a file only the candidate writes.
    head -c 10 /dev/zero > "${work}/b/fixture/cont_9_v00000.dat"
    compare_trees "${work}/a" "${work}/b" > /dev/null
    expect "a file only the candidate writes" "2 0 1 0" \
        "${identical} ${differing} ${only_candidate} ${only_baseline}"
    rm -f "${work}/b/fixture/cont_9_v00000.dat"

    rm -f "${work}/b/fixture/cont_0_v00000.dat"
    compare_trees "${work}/a" "${work}/b" > /dev/null
    expect "a file the candidate does not write" "1 0 0 1" \
        "${identical} ${differing} ${only_candidate} ${only_baseline}"
    head -c 200 /dev/zero > "${work}/b/fixture/cont_0_v00000.dat"

    printf 'x' | dd of="${work}/b/fixture/cont_0_v00000.dat" bs=1 seek=100 conv=notrunc status=none
    compare_trees "${work}/a" "${work}/b" > /dev/null
    expect "a container written differently" "1 1 0 0" \
        "${identical} ${differing} ${only_candidate} ${only_baseline}"
    head -c 200 /dev/zero > "${work}/b/fixture/cont_0_v00000.dat"

    # A config recording another Boost differs twice: in boost_version at 44 and
    # in the checksum at 48 that covers it. Both, because normalising only the
    # first is exactly the mistake that reported two clean configs as findings.
    printf '\xAA\xBB\xCC\xDD' | dd of="${work}/b/fixture/utxoz_config.dat" bs=1 seek=44 \
        conv=notrunc status=none
    printf '\x11\x22\x33\x44' | dd of="${work}/b/fixture/utxoz_config.dat" bs=1 seek=48 \
        conv=notrunc status=none
    compare_trees "${work}/a" "${work}/b" > /dev/null
    expect "a config recording another Boost, checksum and all" "2 0 0 0" \
        "${identical} ${differing} ${only_candidate} ${only_baseline}"

    # Anything else must still count, and it also moves the checksum — so this is
    # the case that says the normalisation did not swallow the whole tail.
    printf '\xAA' | dd of="${work}/b/fixture/utxoz_config.dat" bs=1 seek=8 conv=notrunc status=none
    compare_trees "${work}/a" "${work}/b" > /dev/null
    expect "a config differing elsewhere" "1 1 0 0" \
        "${identical} ${differing} ${only_candidate} ${only_baseline}"

    if (( failures )); then echo "${failures} self-test failures" >&2; exit 1; fi
    echo "the comparison classifies all six cases correctly"
    exit 0
fi

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT

# ---- what the verdict will be about ------------------------------------------
#
# First, because everything below is only meaningful against the fixtures. Absent
# them the compatibility cases fail on the missing files, which would be reported
# as the format having changed — the one thing this job must never say when it
# has not looked.
readonly fixtures="tests/fixtures/epoch1-lp64"
if [[ ! -f "${fixtures}/manifest.json" ]]; then
    verdict "NO_EVIDENCE" "not tested" \
        "There are no fixtures at ${fixtures}, so nothing was built and nothing is claimed. This is not a compatibility result."
    exit 0
fi

# ---- fetch, pinned -----------------------------------------------------------
#
# Every step here can fail for reasons that say nothing about the format: DNS, a
# proxy, a rate limit, a cancelled run. Under `set -e` those would end the script
# where they happened, with no verdict at all — an informative job going red and
# reporting nothing, which is the one outcome it is built to avoid. So each is
# guarded, and a transport failure produces NO_EVIDENCE.
#
# The checksum is not in that category and keeps its own verdict: bytes that do
# not match the pin are a finding, not a network.
cannot_fetch() {
    physical="NO_EVIDENCE"
    physical_note="nothing was fetched, so nothing was written to compare."
    verdict "NO_EVIDENCE" "${described:-not fetched}" \
        "$1 Nothing was built and nothing is claimed about this Boost."
    exit 0
}

if [[ -n "${BOOST_COMMIT}" ]]; then
    described="boostorg/boost@${BOOST_COMMIT}"
    echo "fetching ${described}"
    git clone -q --recurse-submodules --shallow-submodules \
        https://github.com/boostorg/boost.git "${work}/boost-src" \
        || cannot_fetch "The Boost repository could not be cloned."
    git -C "${work}/boost-src" checkout -q "${BOOST_COMMIT}" \
        || cannot_fetch "Commit ${BOOST_COMMIT} could not be checked out."
    git -C "${work}/boost-src" submodule update -q --init --recursive \
        || cannot_fetch "The submodules could not be initialised."
    ( cd "${work}/boost-src" && ./bootstrap.sh > /dev/null && ./b2 headers > /dev/null ) \
        || cannot_fetch "The header tree could not be generated."
    headers="${work}/boost-src"
else
    described="${BOOST_VERSION} (sha256 ${BOOST_SHA256:0:16}…)"
    url="https://github.com/boostorg/boost/releases/download/boost-${BOOST_VERSION}/boost-${BOOST_VERSION}-b2-nodocs.tar.gz"
    echo "fetching ${url}"

    # -f so an HTTP error is a failure rather than an error page saved as an
    # archive, and a finite timeout so a hung transfer cannot become the six
    # hours of silence this job already learned about once.
    curl -fsSL --retry 3 --retry-delay 5 --connect-timeout 30 --max-time 900 \
        -o "${work}/boost.tar.gz" "${url}" \
        || cannot_fetch "The archive could not be downloaded from ${url}."

    actual="$(sha256sum "${work}/boost.tar.gz" | cut -d' ' -f1)"
    if [[ "${actual}" != "${BOOST_SHA256}" ]]; then
        verdict "DOES_NOT_BUILD" "${described}" \
            "The archive does not match the pinned checksum (got ${actual}). Nothing was built."
        exit 0
    fi
    tar -xzf "${work}/boost.tar.gz" -C "${work}" --wildcards "boost-${BOOST_VERSION}/boost/*" \
        || cannot_fetch "The archive matched its checksum and could not be unpacked."
    headers="${work}/boost-${BOOST_VERSION}"
fi

# ---- build against it --------------------------------------------------------
#
# Compiled directly rather than through the project's CMake, because the point is
# to replace exactly one dependency and leave everything else as it is. Boost is
# header-only here, so this is an include path and nothing more.
# `-print -quit` rather than a pipe into head: this script runs under `set -o
# pipefail`, where head closing the pipe kills find with SIGPIPE and the failed
# pipeline takes the assignment — and the script — with it.
fmt_include="$(find "${HOME}/.conan2/p" -maxdepth 4 -type d -path '*fmt*/p/include' -print -quit)"
if [[ -z "${fmt_include}" ]]; then
    verdict "DOES_NOT_BUILD" "${described}" "fmt headers were not found; nothing was built."
    exit 0
fi

mkdir -p "${work}/obj"
build_failed=0
for src in src/database.cpp src/database_impl.cpp src/statistics.cpp src/utils.cpp src/log.cpp; do
    if ! g++ -std=c++23 -O2 -c -I include -I build/build/Release/include -I src \
        -DFMT_HEADER_ONLY \
        -DBOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC=10u \
        -isystem "${headers}" -isystem "${fmt_include}" \
        -o "${work}/obj/$(basename "${src}" .cpp).o" "${src}" 2> "${work}/build.log"; then
        build_failed=1
        break
    fi
done

if (( build_failed )); then
    tail -40 "${work}/build.log" >&2
    verdict "DOES_NOT_BUILD" "${described}" \
        "The store does not compile against this Boost. Nothing is known about the format until somebody looks at why."
    exit 0
fi

catch_root="$(find "${HOME}/.conan2/p/b" -maxdepth 3 -type d -name 'p' -path '*catch*' -print -quit 2>/dev/null)"
if [[ -z "${catch_root}" || ! -f "${catch_root}/lib/libCatch2Main.a" ]]; then
    verdict "BUILDS_ONLY" "${described}" \
        "The library compiled against this Boost; Catch2 was not found, so the compatibility cases could not be run. Compilation is not compatibility."
    exit 0
fi

# The suite itself, not a second implementation of it. Building the real cases
# against the other Boost is what makes this cover both storage modes, the
# per-segment digests and the write-and-reopen — a bespoke probe would cover
# whatever somebody remembered to put in it, and would drift.
if ! g++ -std=c++23 -O2 -I include -I build/build/Release/include -I src -I tests \
    -DFMT_HEADER_ONLY \
    -DBOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC=10u \
    -DUTXOZ_FIXTURES_DIR="\"$(pwd)/tests/fixtures/epoch1-lp64\"" \
    -isystem "${headers}" -isystem "${fmt_include}" -isystem "${catch_root}/include" \
    -o "${work}/suite" tests/test_format_compatibility.cpp "${work}"/obj/*.o \
    "${catch_root}/lib/libCatch2Main.a" "${catch_root}/lib/libCatch2.a" -pthread \
    2> "${work}/link.log"; then
    tail -40 "${work}/link.log" >&2
    verdict "DOES_NOT_BUILD" "${described}" "The compatibility cases do not build against this Boost."
    exit 0
fi

if ! g++ -std=c++23 -O2 -I include -I build/build/Release/include -I src -I tests \
    -DFMT_HEADER_ONLY \
    -DBOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC=10u \
    -DUTXOZ_FIXTURES_DIR="\"$(pwd)/tests/fixtures/epoch1-lp64\"" \
    -isystem "${headers}" -isystem "${fmt_include}" \
    -o "${work}/probe" ci/boost_latest_probe.cpp "${work}"/obj/*.o -pthread 2>> "${work}/link.log"; then
    tail -40 "${work}/link.log" >&2
    verdict "DOES_NOT_BUILD" "${described}" "The refusal probe does not build against this Boost."
    exit 0
fi

# ---- run it ------------------------------------------------------------------
#
# The probe first, because it is the only thing that can tell "the barrier
# declined" apart from "it read the wrong data": a Catch2 run reports both as
# failures.
set +e
"${work}/probe" > "${work}/probe.txt" 2>&1
probe_status=$?
set -e
cat "${work}/probe.txt"

if (( probe_status == 3 )); then
    verdict "REFUSED_BY_IDENTITY" "${described}" \
        "It ran and declined to open the fixtures. The barrier working — expected when an epoch was deliberately moved, and a finding otherwise."
    exit 0
fi

set +e
"${work}/suite" "[compat]" --reporter compact > "${work}/suite.txt" 2>&1
suite_status=$?
set -e
tail -30 "${work}/suite.txt"

if (( suite_status != 0 )); then
    verdict "LOGICAL_MISMATCH" "${described}" \
        "It opened the fixtures and the compatibility cases did not pass. This is what this job exists to catch: do not move any epoch until somebody has understood it."
    exit 0
fi

# A pass is only worth what it covered. A filter that stopped selecting the
# reference cases would leave this green while testing one storage mode, and the
# verdict below claims both — so the run is asked how many cases it actually
# executed rather than trusted to have executed the ones it was pointed at.
# Both wordings Catch2's compact reporter has used — "N assertions in M test
# cases" and "Passed all M test cases with N assertions" — because the count is
# what the verdict below rests on, and a summary this failed to recognise would
# report NO_EVIDENCE for a run that went perfectly well.
ran="$(sed -n -e 's/.*assertions in \([0-9]\+\) test case.*/\1/p' \
              -e 's/.*all \([0-9]\+\) test cases\? with .*assertion.*/\1/p' \
       "${work}/suite.txt" | tail -1)"
if [[ -z "${ran}" || "${ran}" -lt 8 ]]; then
    verdict "NO_EVIDENCE" "${described}" \
        "The compatibility cases reported ${ran:-no} cases run, which is not the suite. Whatever passed, it is not what this verdict would have claimed."
    exit 0
fi
echo "compatibility cases run against this Boost: ${ran}"

# ---- and what it writes ------------------------------------------------------
#
# Reader compatibility is settled above. This is the other half — whether the same
# inputs still produce the same bytes — measured against a baseline written by the
# *pinned* Boost rather than against the committed fixtures. That is the only
# comparison that isolates the variable: the fixtures were written by another
# build at another time, so a difference against them says nothing about Boost.
#
# The files are byte-reproducible, so equality is the criterion and there is no
# mask. That was not always true: the unused tail of every stored value used to be
# whatever the writer's stack held, which changed run to run and made any
# comparison meaningless. One field is still expected to differ, `boost_version`
# in the config, and it is normalised rather than skipped.
fixed_id="00112233445566778899aabbccddeeff"
readonly pinned_generator="build/build/Release/utxoz_make_format_fixtures"

if [[ ! -x "${pinned_generator}" ]]; then
    # Not a footnote. This job says it measures the writer, and without the
    # pinned generator there is nothing to measure against — so it says so
    # instead of quietly reporting on the reader alone.
    physical="NO_EVIDENCE"
    physical_note="${pinned_generator} was not built, so there was no baseline to compare against and nothing is claimed about what this Boost writes. Build the utxoz_make_format_fixtures target before running this."
elif ! g++ -std=c++23 -O2 -I include -I build/build/Release/include -I src -I tests/support -I tests \
        -DFMT_HEADER_ONLY \
        -DBOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC=10u \
        -isystem "${headers}" -isystem "${fmt_include}" \
        -o "${work}/gen" tools/make_format_fixtures.cpp "${work}"/obj/*.o -pthread \
        2>> "${work}/link.log"; then
    physical="NO_EVIDENCE"
    physical_note="the generator does not build against this Boost, so nothing was written to compare."
else
    rm -rf "${work}/candidate" "${work}/baseline" "${work}/baseline2"
    if ! "${work}/gen" "${work}/candidate" "${fixed_id}" > /dev/null 2>&1 \
       || ! "${pinned_generator}" "${work}/baseline" "${fixed_id}" > /dev/null 2>&1 \
       || ! "${pinned_generator}" "${work}/baseline2" "${fixed_id}" > /dev/null 2>&1; then
        physical="NO_EVIDENCE"
        physical_note="a generator run failed, so there is nothing to compare."
    else
        # The measurement checks itself first. If the pinned build does not agree
        # with its own second run, then equality is not the criterion here and any
        # difference found below would be noise wearing the costume of a finding.
        compare_trees "${work}/baseline" "${work}/baseline2" > "${work}/selfcheck.txt" 2>&1
        if (( differing || only_candidate || only_baseline )); then
            physical="NO_EVIDENCE"
            physical_note="the pinned build does not reproduce its own output on this machine, so no comparison here isolates Boost. $(head -3 "${work}/selfcheck.txt" | tr '\n' ' ')"
        else
            compare_trees "${work}/baseline" "${work}/candidate"
            total=$(( identical + differing + only_candidate + only_baseline ))
            if (( differing == 0 && only_candidate == 0 && only_baseline == 0 )); then
                physical="PHYSICALLY_IDENTICAL"
                physical_note="all ${identical} files are byte-identical to what the pinned Boost writes from the same inputs, once the config's diagnostic boost_version is normalised."
            else
                physical="PHYSICALLY_DIFFERENT"
                physical_note="of ${total} files, ${identical} are byte-identical, ${differing} differ, ${only_candidate} were written only by this Boost and ${only_baseline} only by the pinned one. The lines above name them. Nothing here is expected: read every one before moving an epoch."
            fi
        fi
    fi
fi

verdict "LOGICALLY_COMPATIBLE" "${described}" \
    "It opened both fixtures, matched every per-segment and global digest, and survived a write and reopen — the whole compatibility suite, ${ran} cases, built against this Boost."
