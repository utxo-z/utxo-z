#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Wait for a recipe revision to become retrievable from a remote, or report why
# it did not.
#
# Publication is not synchronous. On the remote this project publishes to, an
# upload's HTTP 200 means the files reached a staging directory; the commit that
# actually publishes them happens after a server-side timer expires and takes as
# long as the commit takes. Asking once, immediately after `conan upload`
# returns, asks a question the remote cannot yet answer — and the answer it gives
# is "not found", which is indistinguishable from the package having been lost.
#
# That is not a hypothesis about the release that failed. The publish job for
# utxoz/0.9.0 finished uploading at 19:40:12.37 and asked at 19:40:12.98: 1.3
# seconds later, against a remote that had not begun to publish. It would have
# reported failure for a healthy upload just as readily. See #92.
#
# So the question is asked repeatedly, with a growing gap, until either the
# revision appears or a deadline passes. Three outcomes, and they are not
# interchangeable:
#
#   0  the remote holds the exact revision
#   3  the deadline passed and the remote's own answer was that it does not hold
#      it — an absence the remote asserted, not one inferred from silence
#   4  the deadline passed without a usable answer, or the last word was that the
#      remote could not be asked
#
# 4 is never absence. A release blocked by 3 is a release that did not publish; a
# release blocked by 4 is one we cannot speak about. Collapsing them invents a
# fact, which is the failure mode #92 is made of.
#
# Waiting longer never turns a 0 into a 3: the loop stops the moment the revision
# is there. The deadline only bounds how long a negative answer is disbelieved.
#
# Environment, all optional:
#   UTXOZ_PUBLISH_TIMEOUT       seconds to keep asking; default 300
#   UTXOZ_PUBLISH_POLL_INITIAL  first gap between attempts; default 2
#   UTXOZ_PUBLISH_POLL_MAX      largest gap between attempts; default 30
#   UTXOZ_PUBLISH_MIN_QUERY     least time an attempt may be given; default 5
#   UTXOZ_PUBLISH_CLOCK         command printing monotonic seconds; default
#                               ci/monotonic_seconds.py
#   UTXOZ_PUBLISH_SLEEP         command that waits; default `sleep`
#
# The clock and the sleep are injectable so the tests can drive the deadline and
# the backoff without spending the time they describe. Nothing else reads them.
#
# Usage:  await_published.sh <remote> <name/version> <recipe-revision>

set -euo pipefail

readonly EXIT_OK=0
readonly EXIT_ABSENT=3
readonly EXIT_UNKNOWN=4

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT

if [[ $# -ne 3 ]]; then
    echo "usage: $0 <remote> <name/version> <recipe-revision>" >&2
    exit ${EXIT_UNKNOWN}
fi

readonly REMOTE="$1"
readonly REFERENCE="$2"
readonly REVISION="$3"

readonly TIMEOUT="${UTXOZ_PUBLISH_TIMEOUT:-300}"
readonly POLL_INITIAL="${UTXOZ_PUBLISH_POLL_INITIAL:-2}"
readonly POLL_MAX="${UTXOZ_PUBLISH_POLL_MAX:-30}"
# Monotonic, not `date +%s`. The civil clock steps — NTP correcting a drifting
# runner moves it in either direction — and a deadline built by subtracting two
# of its readings is not a deadline: a backwards step lengthens the wait by
# however far it stepped, silently, and the 300s this script announces stops
# being the 300s it waits. The attempt bound below keeps that finite, but finite
# is not the promise being made.
readonly CLOCK="${UTXOZ_PUBLISH_CLOCK:-python3 ${ROOT}/ci/monotonic_seconds.py}"
readonly SLEEP="${UTXOZ_PUBLISH_SLEEP:-sleep}"

# remote_revisions.py's own bound, read here so each attempt can be given the
# smaller of it and what is left of the window. Its default is repeated rather
# than inherited, because this script has to know the number to clamp it.
readonly QUERY_TIMEOUT="${UTXOZ_REMOTE_QUERY_TIMEOUT:-120}"

# The least time a query needs to stand a chance of answering. Attempts that
# cannot be given this much are not made at all: a starved query times out, a
# timeout is Unknown, and Unknown arriving last would throw away the definite
# answers that came before it.
readonly MIN_QUERY="${UTXOZ_PUBLISH_MIN_QUERY:-5}"

# A non-numeric setting would make every arithmetic comparison below silently
# false, which turns the bound into no bound at all.
whole_seconds() {
    if [[ ! "$2" =~ ^[0-9]+$ ]]; then
        echo "$1 is not a whole number of seconds: $2" >&2
        exit ${EXIT_UNKNOWN}
    fi
}
whole_seconds UTXOZ_PUBLISH_TIMEOUT "${TIMEOUT}"
whole_seconds UTXOZ_PUBLISH_POLL_INITIAL "${POLL_INITIAL}"
whole_seconds UTXOZ_PUBLISH_POLL_MAX "${POLL_MAX}"
whole_seconds UTXOZ_REMOTE_QUERY_TIMEOUT "${QUERY_TIMEOUT}"
whole_seconds UTXOZ_PUBLISH_MIN_QUERY "${MIN_QUERY}"

# A gap of zero is a busy loop against somebody else's server, and it also
# defeats the attempt bound below, which counts on a gap costing at least a
# second.
if (( POLL_INITIAL < 1 )); then
    echo "UTXOZ_PUBLISH_POLL_INITIAL must be at least 1 second: ${POLL_INITIAL}" >&2
    exit ${EXIT_UNKNOWN}
fi
if (( POLL_MAX < POLL_INITIAL )); then
    echo "UTXOZ_PUBLISH_POLL_MAX (${POLL_MAX}) is below UTXOZ_PUBLISH_POLL_INITIAL (${POLL_INITIAL})" >&2
    exit ${EXIT_UNKNOWN}
fi

# Zero is not a smaller floor, it is no floor: it turns the guard below back into
# `remaining <= 0` and hands the last attempt whatever scraps are left, which is
# the starved query that reported an established absence as Unknown. A setting
# that silently restores a fixed bug is worse than one that is rejected.
if (( MIN_QUERY < 1 )); then
    echo "UTXOZ_PUBLISH_MIN_QUERY must be at least 1 second: ${MIN_QUERY}" >&2
    echo "  zero would let an attempt be made with no time to answer in, and a" >&2
    echo "  query that times out is Unknown even when the remote already answered" >&2
    exit ${EXIT_UNKNOWN}
fi

# A clock that goes backwards, or stops, would let the loop run forever. Every
# reading is checked rather than trusted, and an unreadable one ends the wait as
# Unknown — which is what it is.
now() {
    local reading
    if ! reading="$(${CLOCK})"; then
        echo "the clock could not be read" >&2
        return 1
    fi
    if [[ ! "${reading}" =~ ^[0-9]+$ ]]; then
        echo "the clock did not print a whole number of seconds: ${reading}" >&2
        return 1
    fi
    printf '%s' "${reading}"
}

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT

# What the last attempt established. `unknown` until something is established,
# so a run that never gets an answer ends as Unknown rather than as absence.
state="unknown"
detail="no attempt completed"

if ! started="$(now)"; then
    echo "could not determine whether ${REMOTE} has ${REFERENCE}#${REVISION}" >&2
    echo "  the clock is unusable, so the wait cannot be bounded" >&2
    exit ${EXIT_UNKNOWN}
fi
readonly deadline=$(( started + TIMEOUT ))

# A second bound, on attempts rather than on time, because the first one trusts
# the clock. A clock that stops — a container's, a mocked one, one that goes
# backwards — leaves `remaining` permanently positive and the loop with no way
# to end, and an unbounded wait inside a release is worse than either answer it
# was trying to choose between. The gaps are whole seconds and at least one, so
# a working clock can never reach this; only a broken one can.
readonly MAX_ATTEMPTS=$(( TIMEOUT + 2 ))

echo "waiting for ${REFERENCE}#${REVISION} on ${REMOTE} (up to ${TIMEOUT}s)"

attempt=0
definite=0
gap="${POLL_INITIAL}"

while :; do
    attempt=$(( attempt + 1 ))

    if ! before="$(now)"; then
        state="unknown"
        detail="the clock became unreadable while waiting"
        break
    fi

    # Bound the query by what is left of the window.
    #
    # Without this the deadline describes only the gaps between attempts, while
    # each attempt can block for the query's own timeout — 120s by default. A
    # 300s wait against a remote that accepts connections and never answers
    # takes 300s of gaps plus a 120s query, and the number this script printed
    # when it started was not true. The promise is about elapsed time, so the
    # query gets what is left of it and no more.
    #
    # Never below one second: a query given zero time cannot answer, and the
    # first attempt has to happen even when the window is already spent —
    # the package may be there already.
    budget=$(( deadline - before ))
    if (( budget > QUERY_TIMEOUT )); then
        budget="${QUERY_TIMEOUT}"
    fi
    if (( budget < 1 )); then
        budget=1
    fi

    lookup=0
    UTXOZ_REMOTE_QUERY_TIMEOUT="${budget}" \
        "${ROOT}/ci/remote_revisions.py" "${REMOTE}" "${REFERENCE}" \
        > "${work}/revisions" 2> "${work}/why" || lookup=$?

    case ${lookup} in
        0)
            # The reference is there. Whether *this* revision is among the ones
            # listed is a separate question, and the interesting intermediate
            # state: a previous version publishes, ours has not yet.
            grep_status=0
            grep -qxF -- "${REVISION}" "${work}/revisions" || grep_status=$?
            case ${grep_status} in
                0)
                    elapsed=0
                    if current="$(now)"; then
                        elapsed=$(( current - started ))
                    fi
                    printf '  attempt %d: published\n' "${attempt}"
                    echo "ok: ${REFERENCE}#${REVISION} is on ${REMOTE} after ${elapsed}s and ${attempt} attempt(s)"
                    exit ${EXIT_OK}
                    ;;
                1)
                    state="absent"
                    definite=$(( definite + 1 ))
                    detail="${REFERENCE} is on ${REMOTE}, but not revision ${REVISION}; it lists: $(tr '\n' ' ' < "${work}/revisions")"
                    ;;
                *)
                    # grep could not read the list. Not "the revision is
                    # missing" — nothing was compared.
                    state="unknown"
                    detail="cannot read the revision list back (grep exited ${grep_status})"
                    ;;
            esac
            ;;
        3)
            state="absent"
            definite=$(( definite + 1 ))
            detail="the remote does not have ${REFERENCE}"
            ;;
        4)
            state="unknown"
            detail="$(tr '\n' ' ' < "${work}/why")"
            ;;
        *)
            state="unknown"
            detail="the revision lookup failed in an unexpected way (${lookup})"
            ;;
    esac

    if ! current="$(now)"; then
        state="unknown"
        detail="the clock became unreadable while waiting"
        break
    fi

    # Another attempt is worth making only if the window can still give it a fair
    # chance to answer, plus a second to wait beforehand.
    #
    # Without this the last attempt is always the most starved one — the clamp
    # above hands it whatever scraps are left — and a query given a second times
    # out on a remote that is working perfectly. That timeout is Unknown, and
    # Unknown at the end discards every definite answer that came before it. A
    # run that asked three times, was told three times that the package is not
    # there, and then starved its own fourth query reported that it could not
    # establish anything. It had established it.
    #
    # So the window is spent on questions that can be answered, and when it can
    # no longer buy one the run ends on the last real answer it got.
    remaining=$(( deadline - current ))
    if (( remaining < MIN_QUERY + 1 )); then
        printf '  attempt %d: %s\n' "${attempt}" "${detail}"
        break
    fi

    if (( attempt >= MAX_ATTEMPTS )); then
        # More attempts than the window has seconds. Time is not passing the way
        # the clock reports it, so how long this actually waited is unknown —
        # and an absence established over an unknown window is not established.
        state="unknown"
        detail="the clock is not advancing; ${attempt} attempts inside a ${TIMEOUT}s window"
        printf '  attempt %d: %s\n' "${attempt}" "${detail}"
        break
    fi

    # Never sleep into the time the next attempt needs. Trimming to `remaining`
    # alone would land the next query at the deadline with nothing left to run
    # in, which is the starvation this loop just refused to cause.
    this_gap="${gap}"
    headroom=$(( remaining - MIN_QUERY ))
    if (( this_gap > headroom )); then
        this_gap="${headroom}"
    fi

    printf '  attempt %d: %s — asking again in %ds\n' "${attempt}" "${detail}" "${this_gap}"

    # A wait that fails is not a wait that happened, and under `set -e` it would
    # leave here carrying the sleep's own exit code — 127 for a command that is
    # not installed, anything at all for one that is. The caller reads 0, 3 and
    # 4 and nothing else, so a fourth value is not a worse answer than Unknown;
    # it is an answer with no meaning, which the release would then act on.
    sleep_status=0
    ${SLEEP} "${this_gap}" || sleep_status=$?
    if (( sleep_status != 0 )); then
        state="unknown"
        detail="the wait command (${SLEEP}) exited ${sleep_status}, so the window was not honoured"
        break
    fi

    gap=$(( gap * 2 ))
    if (( gap > POLL_MAX )); then
        gap="${POLL_MAX}"
    fi
done

if [[ "${state}" == "absent" ]]; then
    echo "FAIL: ${REFERENCE}#${REVISION} did not appear on ${REMOTE} within ${TIMEOUT}s" >&2
    echo "  ${attempt} attempt(s), ${definite} of them a definite answer from the remote" >&2
    echo "  last answer: ${detail}" >&2
    exit ${EXIT_ABSENT}
fi

echo "FAIL: could not establish whether ${REMOTE} has ${REFERENCE}#${REVISION}" >&2
echo "  ${attempt} attempt(s) in ${TIMEOUT}s, ${definite} of them a definite answer from the remote" >&2
echo "  last answer: ${detail}" >&2
echo "  this is not absence, and it is not a pass" >&2
exit ${EXIT_UNKNOWN}
