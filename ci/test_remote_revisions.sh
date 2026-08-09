#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# What remote_revisions.py answers, and how it answers when it cannot.
#
# The distinction it exists to make — "the package is not there" versus "we could
# not ask" — is the one a real remote will not reproduce on demand: an outage
# cannot be scheduled, and a corrupt reply even less so. So conan is replaced by
# a script that returns exactly the reply each case needs, and every outcome is
# checked on both its exit code and its message.
#
# It reached production once already: `utxoz/0.8.1` was uploaded, is not on the
# remote, and the publish job went green (#92). A verification that reports a
# network failure as absence, or absence as a pass, is how that happens twice.
#
# Run from anywhere:  ci/test_remote_revisions.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT
readonly SCRIPT="${ROOT}/ci/remote_revisions.py"

readonly EXIT_OK=0
readonly EXIT_ABSENT=3
readonly EXIT_UNKNOWN=4

failures=0

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT
mkdir -p "${work}/bin"
export PATH="${work}/bin:${PATH}"

# A conan that replies from a file rather than a network. The reply is chosen per
# case; when the file holds the word `hang`, it sleeps past any sane timeout.
cat > "${work}/bin/conan" <<'FAKE'
#!/usr/bin/env bash
reply_dir="${FAKE_CONAN_REPLIES:?FAKE_CONAN_REPLIES is required}"
# Distinguish the existence query from the revisions query by the pattern, which
# is the second argument: `conan list <pattern> -r <remote> --format=json`.
pattern="$2"
case "${pattern}" in
    *'#*') reply="${reply_dir}/revisions" ;;
    *)     reply="${reply_dir}/existence" ;;
esac
[[ -f "${reply}" ]] || { echo "no fake reply at ${reply}" >&2; exit 70; }
if [[ "$(head -c 4 "${reply}")" == "hang" ]]; then
    sleep 120
fi
if [[ "$(head -c 4 "${reply}")" == "fail" ]]; then
    echo "simulated conan failure" >&2
    exit 1
fi
cat "${reply}"
FAKE
chmod +x "${work}/bin/conan"

# name, expected exit, expected message fragment, existence reply, revisions reply
check() {
    local name="$1" expect_code="$2" expect_text="$3" existence="$4" revisions="$5"
    local dir="${work}/case"
    rm -rf "${dir}"; mkdir -p "${dir}"
    printf '%s' "${existence}" > "${dir}/existence"
    printf '%s' "${revisions}" > "${dir}/revisions"

    local out status=0
    out="$(FAKE_CONAN_REPLIES="${dir}" UTXOZ_REMOTE_QUERY_TIMEOUT=3 \
           "${SCRIPT}" kth utxoz/0.9.0 2>&1)" || status=$?

    if [[ ${status} -ne ${expect_code} ]]; then
        printf 'FAIL  %-34s exited %d, expected %d\n' "${name}" "${status}" "${expect_code}"
        printf '        %s\n' "${out}"
        failures=$((failures + 1))
        return
    fi
    if [[ "${out}" != *"${expect_text}"* ]]; then
        printf 'FAIL  %-34s exited %d but did not say %q\n' "${name}" "${status}" "${expect_text}"
        printf '        %s\n' "${out}"
        failures=$((failures + 1))
        return
    fi
    printf 'ok    %-34s rc=%d  %s\n' "${name}" "${status}" "${expect_text}"
}

readonly PRESENT='{"kth":{"utxoz/0.9.0":{}}}'
readonly ONE_REVISION='{"kth":{"utxoz/0.9.0":{"revisions":{"79617824a559f6e45a47ae8e93f017c1":{"timestamp":1}}}}}'

echo "== the three outcomes =="

# Present, and the revision comes back exactly. The publish check pins on this
# value, so a wrong one is as bad as none.
check "present, exact revision" ${EXIT_OK} "79617824a559f6e45a47ae8e93f017c1" \
      "${PRESENT}" "${ONE_REVISION}"

# Absence, claimed only on the remote's own words.
check "absent" ${EXIT_ABSENT} "does not have utxoz/0.9.0" \
      '{"kth":{"error":"Recipe '"'"'utxoz/0.9.0'"'"' not found"}}' ''

# An error that is not a definite answer must not become absence.
check "error, not absence" ${EXIT_UNKNOWN} "could not determine" \
      '{"kth":{"error":"HTTPSConnectionPool: Max retries exceeded"}}' ''

# Conan itself failing.
check "conan exits non-zero" ${EXIT_UNKNOWN} "could not determine" \
      'fail' ''

# A reply that is not JSON at all.
check "invalid JSON" ${EXIT_UNKNOWN} "not JSON" \
      'not json at all' ''

# A reply that is JSON but says nothing about this remote.
check "JSON without the remote" ${EXIT_UNKNOWN} "says nothing about remote" \
      '{"other":{}}' ''

# A query that never returns. Bounded, and reported as not knowing.
check "timeout" ${EXIT_UNKNOWN} "did not return within" \
      'hang' ''

echo
echo "== ambiguity and half-answers =="

# The reference exists and the revisions query then fails: by then absence is
# ruled out, so this is a failure to read rather than a missing package.
check "revisions query fails" ${EXIT_UNKNOWN} "could not determine" \
      "${PRESENT}" '{"kth":{"error":"b'"''"'. [Remote: kth]"}}'

# Listed, with no revision at all: nothing to install.
check "listed with no revision" ${EXIT_ABSENT} "no recipe revision" \
      "${PRESENT}" '{"kth":{"utxoz/0.9.0":{"revisions":{}}}}'

# Several revisions is not an error here — the caller checks whether its own is
# among them — but all of them must come out, or that check is against a
# truncated list.
check "several revisions" ${EXIT_OK} "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
      "${PRESENT}" '{"kth":{"utxoz/0.9.0":{"revisions":{"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa":{},"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb":{}}}}}'
check "several revisions, all listed" ${EXIT_OK} "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb" \
      "${PRESENT}" '{"kth":{"utxoz/0.9.0":{"revisions":{"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa":{},"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb":{}}}}}'

# The remote answers, and lists something else entirely. Not absence: it said
# nothing about the reference asked for, and silence is not a denial.
check "answers about another package" ${EXIT_UNKNOWN} "did not mention utxoz/0.9.0" \
      '{"kth":{"utxoz/0.8.0":{}}}' ''

echo
echo "== replies that are JSON but not a listing =="

# Valid JSON of the wrong shape. Before the type checks, this one was reported
# as absence: `"error" in "invalid"` and `"utxoz/0.9.0" not in "invalid"` are
# both substring tests on a string, and both happened to give the wrong answer.
check "remote entry is a string" ${EXIT_UNKNOWN} "not an object" \
      '{"kth":"invalid"}' ''
check "payload is a list" ${EXIT_UNKNOWN} "not an object" \
      '[]' ''
check "reference entry is a string" ${EXIT_UNKNOWN} "not an object" \
      "${PRESENT}" '{"kth":{"utxoz/0.9.0":"x"}}'
check "revisions is a string" ${EXIT_UNKNOWN} "not an object" \
      "${PRESENT}" '{"kth":{"utxoz/0.9.0":{"revisions":"aaaa"}}}'

echo
if (( failures != 0 )); then
    printf '%d failed\n' "${failures}" >&2
    exit 1
fi
echo "all cases behaved as expected"
