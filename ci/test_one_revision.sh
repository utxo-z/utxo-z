#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Exactly one recipe revision, or a diagnostic — never a guess.
#
# The value this prints is what the publish job then demands the remote holds.
# A wrong one sends the verification looking for a revision that was never
# built; a silently-chosen one out of several pins the release to whichever
# iteration order produced. So the cases here are mostly the ways a listing can
# be surprising rather than the way it usually looks.
#
# Run from anywhere:  ci/test_one_revision.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT
readonly SCRIPT="${ROOT}/ci/one_revision.py"
readonly REFERENCE="utxoz/0.9.0"

failures=0

# name, expected exit, expected message fragment, listing on stdin
check() {
    local name="$1" expect_code="$2" expect_text="$3" listing="$4"
    local out status=0
    out="$(printf '%s' "${listing}" | "${SCRIPT}" "${REFERENCE}" 2>&1)" || status=$?

    if [[ ${status} -ne ${expect_code} ]]; then
        printf 'FAIL  %-32s exited %d, expected %d\n' "${name}" "${status}" "${expect_code}"
        printf '        %s\n' "${out}"
        failures=$((failures + 1))
    elif [[ "${out}" != *"${expect_text}"* ]]; then
        printf 'FAIL  %-32s exited %d but did not say %q\n' "${name}" "${status}" "${expect_text}"
        printf '        %s\n' "${out}"
        failures=$((failures + 1))
    else
        printf 'ok    %-32s rc=%d  %s\n' "${name}" "${status}" "${expect_text}"
    fi
}

echo "== the one good case, and the two that are not =="
check "exactly one revision" 0 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
      '{"Local Cache":{"utxoz/0.9.0":{"revisions":{"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa":{}}}}}'
check "no revision" 2 "no recipe revision" \
      '{"Local Cache":{"utxoz/0.9.0":{"revisions":{}}}}'
check "two revisions" 2 "has 2 recipe revisions" \
      '{"Local Cache":{"utxoz/0.9.0":{"revisions":{"aaaa":{},"bbbb":{}}}}}'

echo
echo "== listings that are not answers =="
check "conan reported an error" 2 "reported an error" \
      '{"Local Cache":{"error":"boom"}}'
check "not JSON" 2 "not JSON" 'not json at all'
check "empty input" 2 "no output to parse" ''
check "reference absent" 2 "does not list" \
      '{"Local Cache":{"utxoz/0.8.0":{"revisions":{"aaaa":{}}}}}'
check "no Local Cache" 2 "no Local Cache" '{"kth":{}}'

echo
echo "== JSON of the wrong shape =="
# Each of these used to fail in a way that looked like an answer: a list raised
# AttributeError, and a string under "revisions" yielded its first character as
# a revision.
check "payload is a list" 2 "not an object" '[]'
check "Local Cache is a list" 2 "not an object" '{"Local Cache":[]}'
check "entry is a string" 2 "not an object" \
      '{"Local Cache":{"utxoz/0.9.0":"x"}}'
check "revisions is a string" 2 "not an object" \
      '{"Local Cache":{"utxoz/0.9.0":{"revisions":"aaaa"}}}'

echo
if (( failures != 0 )); then
    printf '%d failed\n' "${failures}" >&2
    exit 1
fi
echo "all cases behaved as expected"
