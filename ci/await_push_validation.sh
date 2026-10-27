#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# The release pull request's validation: the push run of the same commit.
#
# A release branch is pushed, and that push run is the release — the full matrix,
# then publication and its verification. The pull request opened for the same
# branch used to run the full matrix again over the same tree. It now runs this
# instead (see ci/release_authority.sh for when): wait for the push run of the
# pull request's head commit, on that branch, in this repository, and pass only
# if it succeeded. A pull request whose push run failed, never ran, or is still
# running when the wait ends is red, never green and never pending forever.
#
#   0  the push run of this commit succeeded
#   1  it completed without success — failed, cancelled, timed out
#   3  no push run of this commit appeared before the deadline
#   4  could not be determined: still running at the deadline, or the API could
#      not be read
#
# A re-run of the push run is seen here, because a re-run is the same run. A
# pull request that went red because the push run had failed is re-run, after
# the push run has been fixed, like any other check.
#
# Environment:
#   REPOSITORY   owner/name                                     (required)
#   HEAD_SHA     the pull request's head commit                 (required)
#   HEAD_REF     the pull request's head branch, e.g. release/1.2.3 (required)
#   WORKFLOW     the workflow file; default ci.yml
#   UTXOZ_GATE_TIMEOUT  seconds to wait; default 7200, which is what
#                       scripts/release.sh waits for the same run
#   UTXOZ_GATE_POLL     seconds between questions; default 30
#   UTXOZ_GATE_CLOCK    command printing monotonic seconds;
#                       default ci/monotonic_seconds.py
#   UTXOZ_GATE_SLEEP    command that waits; default `sleep`
# `gh` must be authenticated (GH_TOKEN) with read access to Actions.
#
# The clock and the sleep are injectable so the tests can drive the deadline
# without spending it. Nothing else reads them.

set -euo pipefail

readonly EXIT_OK=0
readonly EXIT_FAILED=1
readonly EXIT_ABSENT=3
readonly EXIT_UNKNOWN=4

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT

readonly REPO="${REPOSITORY:?REPOSITORY is required}"
readonly SHA="${HEAD_SHA:?HEAD_SHA is required}"
readonly BRANCH="${HEAD_REF:?HEAD_REF is required}"
readonly WORKFLOW_FILE="${WORKFLOW:-ci.yml}"
readonly TIMEOUT="${UTXOZ_GATE_TIMEOUT:-7200}"
readonly POLL="${UTXOZ_GATE_POLL:-30}"
readonly CLOCK="${UTXOZ_GATE_CLOCK:-python3 ${ROOT}/ci/monotonic_seconds.py}"
readonly SLEEP="${UTXOZ_GATE_SLEEP:-sleep}"

for setting in "UTXOZ_GATE_TIMEOUT=${TIMEOUT}" "UTXOZ_GATE_POLL=${POLL}"; do
    if [[ ! "${setting#*=}" =~ ^[0-9]+$ ]]; then
        echo "${setting%%=*} is not a whole number of seconds: ${setting#*=}" >&2
        exit ${EXIT_UNKNOWN}
    fi
done
if (( POLL < 1 )); then
    echo "UTXOZ_GATE_POLL must be at least 1 second" >&2
    exit ${EXIT_UNKNOWN}
fi
# A commit is forty hex digits; anything else would be a query about something else.
if [[ ! "${SHA}" =~ ^[0-9a-f]{40}$ ]]; then
    echo "HEAD_SHA is not a commit id: ${SHA}" >&2
    exit ${EXIT_UNKNOWN}
fi

now() {
    local reading
    reading="$(${CLOCK})" || return 1
    [[ "${reading}" =~ ^[0-9]+$ ]] || return 1
    printf '%s' "${reading}"
}

# The newest push run of this workflow for exactly this commit, on exactly this
# branch, in exactly this repository: "status conclusion url", or "none". The
# filters are applied here as well as in the query, so that an API that ignored
# a parameter could not hand back somebody else's run.
#
# Passed under names of their own: the script's are readonly, and a readonly
# variable cannot be assigned even as a prefix — bash refuses, runs the command
# anyway, and the child never sees the value.
newest_matching_run() {
    GATE_REPO="${REPO}" GATE_SHA="${SHA}" GATE_BRANCH="${BRANCH}" python3 -c '
import json, os, sys
runs = json.load(sys.stdin).get("workflow_runs", [])
mine = [r for r in runs
        if r.get("head_sha") == os.environ["GATE_SHA"]
        and r.get("event") == "push"
        and r.get("head_branch") == os.environ["GATE_BRANCH"]
        and (r.get("head_repository") or {}).get("full_name") == os.environ["GATE_REPO"]]
if not mine:
    print("none")
else:
    r = max(mine, key=lambda r: r.get("id", 0))
    print(r.get("status") or "unknown", r.get("conclusion") or "none", r.get("html_url") or "")
'
}

if ! started="$(now)"; then
    echo "the clock is unusable, so the wait cannot be bounded" >&2
    exit ${EXIT_UNKNOWN}
fi
readonly deadline=$(( started + TIMEOUT ))
# A clock that stops would otherwise keep this waiting for ever.
readonly max_attempts=$(( TIMEOUT / POLL + 2 ))

echo "waiting for the push run of ${SHA} on ${BRANCH} (up to ${TIMEOUT}s)"

state="unknown"
detail="no question was answered"
attempt=0
while :; do
    attempt=$(( attempt + 1 ))
    query="repos/${REPO}/actions/workflows/${WORKFLOW_FILE}/runs?event=push&branch=${BRANCH}&head_sha=${SHA}&per_page=50"
    if payload="$(gh api -H 'Accept: application/vnd.github+json' "${query}" 2>/dev/null)" \
       && answer="$(printf '%s' "${payload}" | newest_matching_run 2>/dev/null)"; then
        read -r status conclusion url <<<"${answer}"
        if [[ "${status}" == "none" ]]; then
            state="absent"
            detail="no push run of ${SHA} on ${BRANCH} yet"
        elif [[ "${status}" == "completed" ]]; then
            if [[ "${conclusion}" == "success" ]]; then
                echo "ok: the push run of ${SHA} succeeded: ${url}"
                exit ${EXIT_OK}
            fi
            echo "::error title=Release revision not validated::the push run of ${SHA} on ${BRANCH} concluded '${conclusion}': ${url}" >&2
            exit ${EXIT_FAILED}
        else
            state="running"
            detail="the push run of ${SHA} is ${status}: ${url}"
        fi
    else
        state="unknown"
        detail="the Actions API could not be read"
    fi
    echo "  attempt ${attempt}: ${detail}"

    if ! current="$(now)"; then
        state="unknown"; detail="the clock became unreadable"; break
    fi
    if (( current >= deadline || attempt >= max_attempts )); then
        break
    fi
    ${SLEEP} "${POLL}"
done

case "${state}" in
    absent)
        echo "::error title=Release revision not validated::no push run of ${SHA} on ${BRANCH} appeared within ${TIMEOUT}s" >&2
        exit ${EXIT_ABSENT}
        ;;
    *)
        echo "::error title=Release validation undetermined::${detail}. This is not a pass; re-run once the push run has finished." >&2
        exit ${EXIT_UNKNOWN}
        ;;
esac
