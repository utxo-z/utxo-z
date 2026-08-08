#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# What determine_version.sh accepts, and what it refuses.
#
# The refusals are the point. Part of the ref is a branch name written by
# whoever opened the pull request, and the derived string is spliced into
# `conan create --version=…` in five jobs. Each case below is a branch name
# somebody can actually push.
#
# Run from anywhere:  ci/test_determine_version.sh

set -eu

readonly SCRIPT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/determine_version.sh"

failures=0

# Fixed so the expectations do not depend on the repository's tags.
export LAST_TAG="0.8.1"
export RUN_NUMBER="42"

accepts() {
    local ref="$1" expected="$2" actual status
    set +e
    actual="$(GITHUB_REF_INPUT="${ref}" "${SCRIPT}" 2>/dev/null)"
    status=$?
    set -e
    if [[ ${status} -ne 0 ]]; then
        printf 'FAIL  %-46q rejected, expected %s\n' "${ref}" "${expected}"
        failures=$((failures + 1))
    elif [[ "${actual}" != "${expected}" ]]; then
        printf 'FAIL  %-46q gave %q, expected %s\n' "${ref}" "${actual}" "${expected}"
        failures=$((failures + 1))
    else
        printf 'ok    %-46q -> %s\n' "${ref}" "${actual}"
    fi
}

refuses() {
    local ref="$1" what="$2" actual status
    set +e
    actual="$(GITHUB_REF_INPUT="${ref}" "${SCRIPT}" 2>/dev/null)"
    status=$?
    set -e
    if [[ ${status} -eq 0 ]]; then
        printf 'FAIL  %-46q accepted as %q — %s\n' "${ref}" "${actual}" "${what}"
        failures=$((failures + 1))
    elif [[ -n "${actual}" ]]; then
        # A refusal that still prints to stdout would be captured by the caller
        # and used as a version.
        printf 'FAIL  %-46q refused but wrote %q to stdout\n' "${ref}" "${actual}"
        failures=$((failures + 1))
    else
        printf 'ok    %-46q refused (%s)\n' "${ref}" "${what}"
    fi
}

echo "== accepted =="
accepts "refs/heads/release/0.9.0"   "0.9.0"
accepts "refs/heads/hotfix/0.9.1"    "0.9.1"
accepts "release/0.9.0"              "0.9.0"          # bare branch name, as github.head_ref arrives
accepts "refs/heads/release/1.2.3-rc1" "1.2.3-rc1"
accepts "refs/heads/master"          "0.8.1-commit.42"
accepts "refs/heads/main"            "0.8.1-commit.42"
accepts "refs/heads/feature/anything" "0.8.1-commit.42"
accepts "refs/tags/v0.9.0"           "0.8.1-commit.42"  # unchanged behaviour, see the script
accepts "refs/pull/86/merge"         "0.8.1-commit.42"

echo
echo "== refused =="
refuses 'refs/heads/release/0.9.0 --build=never'  "a space"
refuses 'refs/heads/release/0.9.0; whoami'        "a semicolon"
refuses 'refs/heads/release/$(id)'                "command substitution"
refuses 'refs/heads/release/`id`'                 "backquotes"
refuses "$(printf 'refs/heads/release/0.9.0\nversion=evil')" "a newline"
refuses 'refs/heads/release/v0.9.0'               "a leading v"
refuses 'refs/heads/release/'                     "empty"
refuses 'refs/heads/release/0.9'                  "not three components"
refuses 'refs/heads/release/../../etc/passwd'     "path traversal"
refuses 'refs/heads/release/0.9.0&&id'            "a shell operator"
refuses 'refs/heads/release/0.9.0|id'             "a pipe"
refuses 'refs/heads/release/--version'            "an option"

# The same hostile names as an actual branch name rather than a full ref, which
# is the form a pull request delivers.
refuses 'release/0.9.0; whoami'                   "a semicolon, bare branch"
refuses 'release/$(id)'                           "command substitution, bare branch"

# A tag that is not a version poisons every development version derived from it.
echo
echo "== a non-version newest tag =="
LAST_TAG='not-a-version' refuses "refs/heads/master" "the newest tag is not a version"
LAST_TAG='0.0.0; id'     refuses "refs/heads/master" "the newest tag carries a semicolon"

# ---------------------------------------------------------------------------
# Reading the tags: a repository with none, and a repository that cannot be
# read, are different answers and must not produce the same one.
#
# These run with LAST_TAG unset, so the script actually asks git — the rest of
# the cases above deliberately do not, so their expectations do not depend on
# this repository's tags.
# ---------------------------------------------------------------------------
echo
echo "== reading the newest tag =="

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT

git init -q "${work}/no-tags"
git -C "${work}/no-tags" -c user.email=ci@utxoz -c user.name=ci commit -q --allow-empty -m "no tags here"
mkdir -p "${work}/not-a-repo"

# A successful query that finds nothing is a real answer: start at zero.
set +e
actual="$(cd "${work}/no-tags" && GITHUB_REF_INPUT=refs/heads/master RUN_NUMBER=42 \
          env -u LAST_TAG "${SCRIPT}" 2>/dev/null)"
status=$?
set -e
if [[ ${status} -ne 0 || "${actual}" != "0.0.0-commit.42" ]]; then
    printf 'FAIL  a repository with no tags gave rc=%d %q, expected 0.0.0-commit.42\n' "${status}" "${actual}"
    failures=$((failures + 1))
else
    printf 'ok    a repository with no tags               -> %s\n' "${actual}"
fi

# A failed query is not an answer at all. Before this, `git tag | head` reported
# head's status, so git failing looked exactly like a repository with no tags
# and 0.0.0-commit.<run> was published off the back of it.
set +e
actual="$(cd "${work}/not-a-repo" && GITHUB_REF_INPUT=refs/heads/master RUN_NUMBER=42 \
          env -u LAST_TAG "${SCRIPT}" 2>"${work}/stderr")"
status=$?
set -e
if [[ ${status} -eq 0 ]]; then
    printf 'FAIL  git failing was accepted as %q\n' "${actual}"
    failures=$((failures + 1))
elif [[ -n "${actual}" ]]; then
    printf 'FAIL  git failing wrote %q to stdout\n' "${actual}"
    failures=$((failures + 1))
elif [[ ! -s "${work}/stderr" ]]; then
    printf 'FAIL  git failing said nothing on stderr\n'
    failures=$((failures + 1))
else
    printf 'ok    git failing to read the tags          -> refused, nothing on stdout\n'
fi

rm -rf "${work}"
trap - EXIT

echo
if [[ ${failures} -ne 0 ]]; then
    printf '%d failed\n' "${failures}" >&2
    exit 1
fi
echo "all cases behaved as expected"
