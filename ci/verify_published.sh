#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# After the upload: prove the package is actually retrievable from the remote.
#
# A successful `conan upload` is not that proof. utxoz/0.8.1 was uploaded on
# 2026-07-31 — three publish jobs, each printing a complete upload summary with
# `(Uploaded)` — and it is not on the remote today; every recipe revision those
# jobs reported answers 404. The release was declared done on the strength of
# the log. See #92.
#
# Three questions are asked, and they are not the same question. A release is
# published when its exact recipe revision is on the remote and comes back from
# an empty cache. A prebuilt binary for this platform is an acceleration: a
# consumer without one builds from the recipe. A consumer that compiles and runs
# is a diagnostic about the package's usability. Ranking them wrongly is how
# 0.9.1 came to be reported as a failed release: the recipe published, the
# Windows binary downloaded, and a consumer that included a header from a
# package it never declared failed to compile — so the run went red as though
# nothing had been published at all (#113).
#
# So each layer has its own exit code and its own line in the verdict, and no
# failure below the first one is allowed to describe the first one:
#
#   0  required gate passed, and both layers below it held
#   3  REQUIRED: the remote's own answer is that it does not hold the revision
#   4  REQUIRED: no usable answer — this is not absence, and it is not a pass
#   5  REQUIRED: the remote lists the revision but it does not come back
#   6  OPTIONAL: no binary for this platform came back under --build=never
#   7  DIAGNOSTIC: the consumer did not configure, compile, link or run
#
# 3 and 4 are kept apart because "it did not publish" and "we could not tell"
# call for different things next: the first is a fault to fix, the second is a
# question to re-ask. Collapsing them invents a fact, which is what #92 is made
# of. 5, 6 and 7 all leave the release published, and say so in the verdict even
# while exiting non-zero.
#
# Every code above 0 is a red exit. A quiet failure is not the alternative being
# offered here — the alternative is a red failure that says which layer failed.
#
# All of it against an empty cache, so nothing can be answered by what
# `conan create` left behind locally. And the remote is read anonymously: what
# this verifies is what a consumer sees, not what an authenticated session can
# reach.
#
# --build=never is load-bearing for layer 2. Without it a missing binary is
# papered over by rebuilding from the recipe, and the layer passes while
# measuring nothing.
#
# Reads:
#   UTXOZ_BUILD_VERSION    the version that was just uploaded
#   UTXOZ_RECIPE_REVISION  the revision create_package.sh reported; required
#   UTXOZ_REMOTE           remote name, default `kth-verify`
#   UTXOZ_REMOTE_URL       remote URL; added when set
#   UTXOZ_CONSUMER_DIR     the consumer to build, default test_package. Exists
#                          so the negative control can point this at a copy with
#                          the utxoz dependency removed and prove that layer 3
#                          fails when it should — a check that cannot fail is
#                          not a check.
#   UTXOZ_VERIFY_RESULT_FILE  where to write the three verdicts as key=value
#
# Run from the repository root:  ci/verify_published.sh

set -euo pipefail

readonly EXIT_OK=0
readonly EXIT_RECIPE_ABSENT=3
readonly EXIT_RECIPE_UNKNOWN=4
readonly EXIT_RECIPE_UNRETRIEVABLE=5
readonly EXIT_BINARY_MISSING=6
readonly EXIT_CONSUMER_FAILED=7

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT
readonly VERSION="${UTXOZ_BUILD_VERSION:?UTXOZ_BUILD_VERSION is required}"
readonly REVISION="${UTXOZ_RECIPE_REVISION:?UTXOZ_RECIPE_REVISION is required}"
readonly REMOTE="${UTXOZ_REMOTE:-kth-verify}"
readonly REFERENCE="utxoz/${VERSION}"
readonly CONSUMER_DIR="${UTXOZ_CONSUMER_DIR:-${ROOT}/test_package}"

cd "${ROOT}"

fail() { echo "FAIL: $*" >&2; exit 1; }

# What each layer established. They start as the honest thing to say about a
# layer that has not run: nothing.
recipe_state="not established"
binary_state="not attempted"
consumer_state="not attempted"

# The verdict, printed on every exit path including the failing ones.
#
# The point of printing all three every time is that a reader — or a release
# script — never has to infer one layer's result from another's. A run that
# exits 7 says, in the same breath, that the recipe published.
report() {
    local code="$1"

    echo
    echo "── ${REFERENCE}#${REVISION} on ${REMOTE} ──"
    echo "REQUIRED   recipe published and retrievable : ${recipe_state}"
    echo "OPTIONAL   binary for this platform         : ${binary_state}"
    echo "DIAGNOSTIC consumer compiles and runs       : ${consumer_state}"

    if [[ -n "${UTXOZ_VERIFY_RESULT_FILE:-}" ]]; then
        {
            printf 'recipe=%s\n' "${recipe_state}"
            printf 'binary=%s\n' "${binary_state}"
            printf 'consumer=%s\n' "${consumer_state}"
            printf 'exit=%s\n' "${code}"
        } > "${UTXOZ_VERIFY_RESULT_FILE}"
    fi

    if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
        {
            printf "### Post-upload verification: \`%s#%s\`\n\n" "${REFERENCE}" "${REVISION}"
            printf '| Layer | Check | Result |\n|---|---|---|\n'
            printf '| **Required** | recipe published and retrievable | %s |\n' "${recipe_state}"
            printf '| Optional | binary for this platform | %s |\n' "${binary_state}"
            printf '| Diagnostic | consumer compiles and runs | %s |\n\n' "${consumer_state}"
            if (( code == EXIT_RECIPE_ABSENT || code == EXIT_RECIPE_UNKNOWN || code == EXIT_RECIPE_UNRETRIEVABLE )); then
                printf 'The release gate did **not** pass.\n'
            elif (( code != EXIT_OK )); then
                printf 'The release gate passed: the recipe revision is published and retrievable. '
                printf 'An optional or diagnostic layer failed and is reported above.\n'
            else
                printf 'All three layers passed.\n'
            fi
        } >> "${GITHUB_STEP_SUMMARY}"
    fi

    exit "${code}"
}

readonly PROFILE="${ROOT}/ci/conan-cpp23"
[[ -f "${PROFILE}" ]] || fail "the shared C++23 profile is missing: ${PROFILE}"
[[ -d "${CONSUMER_DIR}" ]] || fail "the consumer directory does not exist: ${CONSUMER_DIR}"

# The same options the package was created with. They are part of the package
# id, so asking with different ones asks for a binary nobody built — a failure
# about this script rather than about the release.
#
# Scoped to `utxoz/*` rather than bare. Bare is ambiguous once the root is a
# requirement rather than the recipe in this directory: Conan warns that it
# cannot tell which package in the graph the option is meant for, and the
# warning is right to. `-o with_tests=False` reads as an instruction to whatever
# recipe happens to answer to it. Scoping says the thing that was meant, and
# keeps the package id identical to the one create_package.sh built (#113).
readonly PACKAGE_OPTIONS=(
    -pr:h "${PROFILE}"
    -s build_type=Release
    -o "utxoz/*:with_tests=False"
    -o "utxoz/*:with_examples=False"
    -o "utxoz/*:with_benchmarks=False"
)

# A cache of its own, holding nothing. Verifying against the cache that just
# built the package would pass whether or not the upload arrived.
work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT
export CONAN_HOME="${work}/conan"
conan profile detect --force > /dev/null

if [[ -n "${UTXOZ_REMOTE_URL:-}" ]]; then
    # This server publishes Knuth packages; it is not a mirror of ConanCenter.
    # Restricting `conan install` with `-r ${REMOTE}` made every transitive
    # dependency use it too, so a correctly published utxoz failed verification
    # when fmt was (correctly) absent there. Give the publishing remote priority
    # and limit it to utxoz: the root can only come from the server under test,
    # while fmt, Boost and the other public dependencies use ConanCenter.
    conan remote add conancenter https://center2.conan.io --force > /dev/null
    conan remote add "${REMOTE}" "${UTXOZ_REMOTE_URL}" --force --index=0 \
        --allowed-packages="utxoz/*" > /dev/null
fi

# ---------------------------------------------------------------------------
# REQUIRED, part 1. The remote holds it, and holds the revision just made.
#
# Waited for rather than asked once. The remote publishes an upload some time
# after accepting it, so the first answer to this question is routinely "not
# found" on a release that is perfectly healthy — and treating that first answer
# as the verdict fails the release for the delay rather than for the fault.
#
# The three outcomes come back as three exit codes and are passed through as
# three, because "it did not publish" and "we could not tell" call for different
# things next: the first is a fault to fix, the second is a question to re-ask.
# ---------------------------------------------------------------------------
lookup=0
"${ROOT}/ci/await_published.sh" "${REMOTE}" "${REFERENCE}" "${REVISION}" || lookup=$?

case ${lookup} in
    0) ;;
    3) recipe_state="ABSENT — the remote says it does not hold this revision"
       report ${EXIT_RECIPE_ABSENT} ;;
    4) recipe_state="UNDETERMINED — the remote could not be asked; this is not absence"
       report ${EXIT_RECIPE_UNKNOWN} ;;
    *) recipe_state="UNDETERMINED — the publication wait failed unexpectedly (${lookup})"
       report ${EXIT_RECIPE_UNKNOWN} ;;
esac

# ---------------------------------------------------------------------------
# REQUIRED, part 2. And it comes back.
#
# Listing is not retrieval. The remote naming a revision in an index it serves
# and the remote handing over the recipe files under that revision are two
# different claims, and a release rests on the second. Downloading the recipe
# alone — not the binary — is exactly the required gate: this is what a consumer
# on a platform nobody built for still gets, and what they build from.
# ---------------------------------------------------------------------------
echo
echo "retrieving the recipe for ${REFERENCE}#${REVISION} from ${REMOTE} into an empty cache"
download=0
conan download "${REFERENCE}#${REVISION}" -r "${REMOTE}" --only-recipe \
    > "${work}/download.log" 2>&1 || download=$?
if (( download != 0 )); then
    cat "${work}/download.log" >&2
    recipe_state="NOT RETRIEVABLE — listed on ${REMOTE}, but the recipe did not come back (conan download exited ${download})"
    report ${EXIT_RECIPE_UNRETRIEVABLE}
fi

# And it is in this cache under that revision, rather than merely reported as
# downloaded. `conan download` succeeding is a claim about a transfer; this is
# the artefact.
if ! conan cache path "${REFERENCE}#${REVISION}" > "${work}/cache-path" 2>&1; then
    cat "${work}/cache-path" >&2
    recipe_state="NOT RETRIEVABLE — the download reported success but the revision is not in the cache"
    report ${EXIT_RECIPE_UNRETRIEVABLE}
fi

recipe_state="PUBLISHED and retrievable from an empty cache"
echo "the recipe is in the empty cache at: $(cat "${work}/cache-path")"

# ---------------------------------------------------------------------------
# OPTIONAL. A binary for this platform installs, pinned to that revision, with
# no permission to build.
#
# Optional as a release gate, not optional as information: a consumer without a
# binary falls back to building from the recipe, which works and is slow. What
# must not happen is this layer's failure being read as the recipe's.
# ---------------------------------------------------------------------------
echo
echo "installing ${REFERENCE}#${REVISION} from ${REMOTE}, with public dependencies from ConanCenter and --build=never"
readonly CONSUMER_BUILD="${work}/consumer"
mkdir -p "${CONSUMER_BUILD}"
# --lockfile-partial keeps the repository's pinned dependencies while admitting
# utxoz itself as a new root; dropping the lockfile lets them float and changes
# the package id, asking for a binary nobody built.
install=0
conan install --requires="${REFERENCE}#${REVISION}" --build=never --lockfile-partial \
    "${PACKAGE_OPTIONS[@]}" -g CMakeDeps -g CMakeToolchain -of "${CONSUMER_BUILD}" \
    > "${work}/install.log" 2>&1 || install=$?
cat "${work}/install.log"
if (( install != 0 )); then
    binary_state="NOT RETRIEVED — no prebuilt binary came back under --build=never (conan install exited ${install})"
    report ${EXIT_BINARY_MISSING}
fi
binary_state="retrieved with --build=never"

# ---------------------------------------------------------------------------
# DIAGNOSTIC. A consumer compiles and runs against what came back.
#
# The same consumer `conan create` uses, so this exercises both storage modes
# and both generated headers. It declares one package and includes headers from
# that package and the standard library only; the 0.9.1 Windows failure was this
# consumer reaching for a header belonging to a package it had not asked for
# (#113).
# ---------------------------------------------------------------------------
echo
echo "building a consumer (${CONSUMER_DIR}) against the downloaded package"
# ci/verify_consumer.sh rather than the steps inline, so the pull-request job can
# run this same layer against a locally created package — once with the real
# consumer and once with its utxoz dependency removed. A check nobody has seen
# fail is not known to be able to.
consumer=0
"${ROOT}/ci/verify_consumer.sh" "${CONSUMER_BUILD}" "${CONSUMER_DIR}" || consumer=$?
case ${consumer} in
    0) consumer_state="compiles, links and runs in both storage modes" ;;
    1) consumer_state="FAILED — does not configure against the published package"
       report ${EXIT_CONSUMER_FAILED} ;;
    2) consumer_state="FAILED — does not compile or link against the published package"
       report ${EXIT_CONSUMER_FAILED} ;;
    3) consumer_state="FAILED — built but produced no executable to run"
       report ${EXIT_CONSUMER_FAILED} ;;
    4) consumer_state="FAILED — built and linked, but the consumer exited non-zero"
       report ${EXIT_CONSUMER_FAILED} ;;
    *) consumer_state="FAILED — the consumer check failed unexpectedly (${consumer})"
       report ${EXIT_CONSUMER_FAILED} ;;
esac

echo
echo "ok: ${REFERENCE}#${REVISION} is on ${REMOTE} and a consumer can install and run it"
report ${EXIT_OK}
