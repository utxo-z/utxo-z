#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# How await_published.sh behaves while a remote is still publishing.
#
# The delay this exists for cannot be produced on demand: a real remote publishes
# when it publishes, and a release cannot be held open to find out. So the remote
# is replaced by a conan that answers a scripted sequence — not found, not found,
# then found — and the clock and the sleep are replaced too, which is what makes
# the timing assertions deterministic and the suite instant. The script under
# test runs its real loop, calls the real remote_revisions.py, and never learns
# that any of it is fake.
#
# What it is checking is a distinction that has already cost one release. #92
# asked once, 1.3 seconds after uploading, and read the delay as a lost package.
# The cases below fix the three answers in place: appeared (however late), did
# not appear within the window, and could not be established — the last of which
# is not the middle one.
#
# Run from anywhere:  ci/test_await_published.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT
readonly SCRIPT="${ROOT}/ci/await_published.sh"

readonly EXIT_OK=0
readonly EXIT_ABSENT=3
readonly EXIT_UNKNOWN=4

readonly REVISION="79617824a559f6e45a47ae8e93f017c1"
readonly OTHER_REVISION="544d3847f25a1b54b0fa518f1656381c"

failures=0

# A bound on a command's running time, built from `kill -0` and a shell loop
# rather than from `timeout`.
#
# `timeout` is coreutils, and this suite runs on the three platforms the release
# does: macOS does not ship it at all, and neither do the shells of every Linux
# it has to pass on. A test harness that is itself missing on a platform reports
# the platform as broken, which is worse than not testing there.
#
# Returns 124 when the limit is reached, mirroring what `timeout` would have
# said, and otherwise the command's own status.
run_bounded() {
    local limit="$1"; shift
    "$@" > "${work}/bounded.out" 2>&1 &
    local pid=$! waited=0 status=0

    while kill -0 "${pid}" 2>/dev/null; do
        if (( waited >= limit )); then
            kill -9 "${pid}" 2>/dev/null || true
            wait "${pid}" 2>/dev/null || true
            return 124
        fi
        sleep 1
        waited=$(( waited + 1 ))
    done

    wait "${pid}" || status=$?
    return ${status}
}

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT
mkdir -p "${work}/bin"
export PATH="${work}/bin:${PATH}"

# A conan that answers differently each time it is asked, which is the whole
# point: a remote that is still publishing says one thing and later says another.
# Attempts are counted on disk because each call is a new process.
#
# The reply for attempt N is the file `N`, or `default` once the script runs off
# the end of the list. remote_revisions.py asks twice per attempt — existence,
# then revisions — and both are answered from the same file, so a case is one
# line rather than two.
cat > "${work}/bin/conan" <<'FAKE'
#!/usr/bin/env bash
replies="${FAKE_CONAN_REPLIES:?FAKE_CONAN_REPLIES is required}"
pattern="$2"

# Only the existence query advances the counter, so one attempt of the caller is
# one step of the sequence regardless of how many queries it takes.
case "${pattern}" in
    *'#*') step="$(cat "${replies}/step")" ;;
    *)     step="$(( $(cat "${replies}/step") + 1 ))"; printf '%s' "${step}" > "${replies}/step" ;;
esac

reply="${replies}/${step}"
[[ -f "${reply}" ]] || reply="${replies}/default"
[[ -f "${reply}" ]] || { echo "no fake reply for step ${step}" >&2; exit 70; }

# A remote that accepts the connection and then never answers. Longer than any
# window a case gives it, so what ends the query is always the bound under test.
if [[ "$(head -c 4 "${reply}")" == "hang" ]]; then
    sleep 120
fi
if [[ "$(head -c 4 "${reply}")" == "fail" ]]; then
    echo "simulated conan failure" >&2
    exit 1
fi
# `slow N` on the first line: a remote that answers, but not instantly. Only the
# existence query pays it — the budget is shared between the two anyway, and one
# sleep per attempt is enough to starve a query that was given too little.
if [[ "$(head -c 4 "${reply}")" == "slow" ]]; then
    case "${pattern}" in
        *'#*') : ;;
        *) sleep "$(head -n 1 "${reply}" | cut -d' ' -f2)" ;;
    esac
    tail -n +2 "${reply}"
    exit 0
fi
cat "${reply}"
FAKE
chmod +x "${work}/bin/conan"

# A clock that advances by a fixed step every time it is read — the cost of one
# query — so the deadline arrives after a known number of readings instead of
# after real seconds.
cat > "${work}/bin/fakeclock" <<'CLOCK'
#!/usr/bin/env bash
state="${FAKE_CLOCK_STATE:?FAKE_CLOCK_STATE is required}"
step="${FAKE_CLOCK_STEP:-1}"
current="$(cat "${state}" 2>/dev/null || printf '0')"
printf '%s' "$(( current + step ))" > "${state}"
printf '%s' "${current}"
CLOCK
chmod +x "${work}/bin/fakeclock"

# A sleep that records what it was asked to wait, advances the clock by that
# much, and returns at once.
#
# Advancing the clock is the part that matters. A sleep that costs no time makes
# the deadline unreachable by waiting, so every assertion about the gaps near the
# deadline would describe a run that cannot happen — the suite would be precise
# about fiction. Here waiting consumes the window exactly as it does in a
# release, and only the real seconds are skipped.
cat > "${work}/bin/fakesleep" <<'SLEEP'
#!/usr/bin/env bash
printf '%s\n' "$1" >> "${FAKE_SLEEP_LOG:?FAKE_SLEEP_LOG is required}"
state="${FAKE_CLOCK_STATE:?FAKE_CLOCK_STATE is required}"
current="$(cat "${state}" 2>/dev/null || printf '0')"
printf '%s' "$(( current + $1 ))" > "${state}"
SLEEP
chmod +x "${work}/bin/fakesleep"

readonly ABSENT='{"kth":{"error":"Recipe '"'"'utxoz/0.9.0'"'"' not found"}}'
readonly UNREACHABLE='{"kth":{"error":"HTTPSConnectionPool: Max retries exceeded"}}'
readonly PUBLISHED='{"kth":{"utxoz/0.9.0":{"revisions":{"'"${REVISION}"'":{"timestamp":1}}}}}'
readonly OTHER_ONLY='{"kth":{"utxoz/0.9.0":{"revisions":{"'"${OTHER_REVISION}"'":{"timestamp":1}}}}}'

# run <case-dir-setup...> — sets up replies then runs the script under test.
# Each reply is "step:json"; `default:json` answers every step past the last.
run() {
    local timeout="$1" clock_step="$2"; shift 2
    local dir="${work}/case"
    rm -rf "${dir}"; mkdir -p "${dir}"
    printf '0' > "${dir}/step"
    : > "${work}/sleeps"

    local spec
    for spec in "$@"; do
        printf '%s' "${spec#*:}" > "${dir}/${spec%%:*}"
    done

    env FAKE_CONAN_REPLIES="${dir}" \
        FAKE_CLOCK_STATE="${dir}/clock" \
        FAKE_CLOCK_STEP="${clock_step}" \
        FAKE_SLEEP_LOG="${work}/sleeps" \
        UTXOZ_PUBLISH_TIMEOUT="${timeout}" \
        UTXOZ_PUBLISH_POLL_INITIAL=2 \
        UTXOZ_PUBLISH_POLL_MAX=8 \
        UTXOZ_PUBLISH_MIN_QUERY=1 \
        UTXOZ_PUBLISH_CLOCK=fakeclock \
        UTXOZ_PUBLISH_SLEEP=fakesleep \
        UTXOZ_REMOTE_QUERY_TIMEOUT=3 \
        "${SCRIPT}" kth utxoz/0.9.0 "${REVISION}" 2>&1
}

# check <name> <expected code> <expected fragment> <timeout> <clock step> <replies...>
check() {
    local name="$1" expect_code="$2" expect_text="$3"; shift 3
    local out status=0
    out="$(run "$@")" || status=$?

    if [[ ${status} -ne ${expect_code} ]]; then
        printf 'FAIL  %-40s exited %d, expected %d\n' "${name}" "${status}" "${expect_code}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
        return
    fi
    if [[ "${out}" != *"${expect_text}"* ]]; then
        printf 'FAIL  %-40s exited %d but did not say %q\n' "${name}" "${status}" "${expect_text}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
        return
    fi
    printf 'ok    %-40s rc=%d  %s\n' "${name}" "${status}" "${expect_text}"
}

# check_gaps <name> <expected gaps, comma separated>
# Reads the sleep log left by the run immediately before it.
check_gaps() {
    local name="$1" expected="$2"
    local actual
    actual="$(tr '\n' ',' < "${work}/sleeps" | sed 's/,$//')"
    if [[ "${actual}" != "${expected}" ]]; then
        printf 'FAIL  %-40s waited [%s], expected [%s]\n' "${name}" "${actual}" "${expected}"
        failures=$((failures + 1))
        return
    fi
    printf 'ok    %-40s waited [%s]\n' "${name}" "${actual}"
}

# The bound the whole design rests on: however the gaps come out, their total
# never exceeds the window the caller allowed. A release that asked to wait five
# minutes waits five minutes.
check_within_window() {
    local name="$1" allowed="$2" total=0 gap
    while read -r gap; do
        total=$(( total + gap ))
    done < "${work}/sleeps"
    if (( total > allowed )); then
        printf 'FAIL  %-40s waited %ds in total, allowed %ds\n' "${name}" "${total}" "${allowed}"
        failures=$((failures + 1))
        return
    fi
    printf 'ok    %-40s waited %ds of %ds allowed\n' "${name}" "${total}" "${allowed}"
}

echo "== the package appears, sooner or later =="

# The healthy case that used to fail. Nothing is wrong; the remote simply had
# not published yet when it was first asked.
check "published immediately" ${EXIT_OK} "after" \
      60 1 "default:${PUBLISHED}"
check_gaps "published immediately, never waited" ""

check "published on the third attempt" ${EXIT_OK} "attempt 3: published" \
      60 1 "1:${ABSENT}" "2:${ABSENT}" "default:${PUBLISHED}"

# A blip in the middle is not a verdict. The release is not failed for a network
# error that the next attempt disproves.
check "unreachable, then published" ${EXIT_OK} "attempt 3: published" \
      60 1 "1:${UNREACHABLE}" "2:${UNREACHABLE}" "default:${PUBLISHED}"

# The intermediate state a versioned remote really produces: the reference is
# there from an earlier release, and this revision has not landed yet.
check "reference there, revision lands later" ${EXIT_OK} "attempt 3: published" \
      60 1 "1:${OTHER_ONLY}" "2:${OTHER_ONLY}" "default:${PUBLISHED}"

echo
echo "== the gaps grow, and stop growing =="

# Doubling from 2, capped at 8. Six attempts means five waits.
check "backoff doubles and is capped" ${EXIT_OK} "attempt 6: published" \
      60 1 "1:${ABSENT}" "2:${ABSENT}" "3:${ABSENT}" "4:${ABSENT}" "5:${ABSENT}" "default:${PUBLISHED}"
check_gaps "backoff doubles and is capped" "2,4,8,8,8"

# Near the deadline the gap is trimmed to what is left, so the last attempt lands
# inside the window rather than after it — the doubling gives way to the bound.
check "the last gap is trimmed to the deadline" ${EXIT_ABSENT} "did not appear" \
      5 1 "default:${ABSENT}"
check_gaps "the last gap is trimmed" "2"
check_within_window "waiting never outlasts the window" 5

echo
echo "== it did not publish =="

# Absent for the whole window, and said so by the remote every time. This is a
# fault, and it is reported as one.
check "absent throughout" ${EXIT_ABSENT} "did not appear on kth within" \
      20 4 "default:${ABSENT}"
check "absent throughout, counts its answers" ${EXIT_ABSENT} "a definite answer from the remote" \
      20 4 "default:${ABSENT}"

# The reference publishes and this revision never does — a different fault from
# the reference being missing, and named differently.
check "reference published, revision never" ${EXIT_ABSENT} "not revision ${REVISION}" \
      20 4 "default:${OTHER_ONLY}"

# A window of zero still asks once: the answer may already be there.
check "zero window still asks once" ${EXIT_ABSENT} "1 attempt(s)" \
      0 1 "default:${ABSENT}"
check "zero window, published already" ${EXIT_OK} "attempt 1: published" \
      0 1 "default:${PUBLISHED}"

echo
echo "== it could not be established =="

# Never a usable answer. Not absence — nothing was ever established, and a
# release blocked on this is blocked on a question, not on a fault.
check "unreachable throughout" ${EXIT_UNKNOWN} "could not establish" \
      20 4 "default:${UNREACHABLE}"
check "unreachable is not absence" ${EXIT_UNKNOWN} "this is not absence" \
      20 4 "default:${UNREACHABLE}"

# Absent for most of the window and unreachable at the end. The last word is
# that we could not ask, so the run cannot claim the package is missing.
check "absent, then unreachable at the deadline" ${EXIT_UNKNOWN} "could not establish" \
      12 1 "1:${ABSENT}" "2:${ABSENT}" "default:${UNREACHABLE}"

check "conan itself fails" ${EXIT_UNKNOWN} "could not establish" \
      20 4 "default:fail"
check "replies are not JSON" ${EXIT_UNKNOWN} "could not establish" \
      20 4 "default:not json at all"

echo
echo "== the window covers the queries, not just the gaps =="

# A query that would outlast what is left of the window.
#
# The fake conan sleeps far longer than any window here, so whatever ends the
# query is the bound being tested. With the query's own timeout left at its
# default, only the remaining-window clamp can produce a small number — so the
# reported figure is the assertion: it names the budget the query was actually
# given.
#
# Without the clamp this case takes two minutes and reports 120s. That is the
# bug: a script that announced a 4s window blocking for 120s inside one attempt.
budget_case() {
    local name="$1" timeout="$2" expect_text="$3"
    local dir="${work}/case" status=0 out
    rm -rf "${dir}"; mkdir -p "${dir}"
    printf '0' > "${dir}/step"
    printf 'hang' > "${dir}/default"
    : > "${work}/sleeps"

    run_bounded 60 env FAKE_CONAN_REPLIES="${dir}" FAKE_SLEEP_LOG="${work}/sleeps" \
        FAKE_CLOCK_STATE="${dir}/clock" FAKE_CLOCK_STEP=1 \
        UTXOZ_PUBLISH_TIMEOUT="${timeout}" UTXOZ_PUBLISH_CLOCK=fakeclock \
        UTXOZ_PUBLISH_SLEEP=fakesleep \
        "${SCRIPT}" kth utxoz/0.9.0 "${REVISION}" || status=$?
    out="$(cat "${work}/bounded.out")"

    if [[ ${status} -ne ${EXIT_UNKNOWN} || "${out}" != *"${expect_text}"* ]]; then
        printf 'FAIL  %-40s exited %d, expected %d saying %q\n' \
               "${name}" "${status}" "${EXIT_UNKNOWN}" "${expect_text}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
        return
    fi
    printf 'ok    %-40s rc=%d  %s\n' "${name}" "${status}" "${expect_text}"
}

# Window of 4, one second gone reading the clock: the query gets the 3 that are
# left, not the 120 it would take by default.
budget_case "a query cannot outlast the window" 4 "did not return within 3s"

# And the same at the very edge: a window already spent still asks once, with
# the one second that makes a query possible at all.
budget_case "a spent window still asks, briefly" 0 "did not return within 1s"

echo
echo "== a dying window does not erase what the remote already said =="

# The regression this suite did not have, and CI found instead.
#
# Clamping each query to what is left of the window makes the last attempt the
# most starved one — it gets the scraps. A query given a second times out on a
# remote that is answering perfectly well, that timeout is Unknown, and Unknown
# arriving last discards every definite answer before it. The publish dry run
# asked three times, was told three times that the package is not there, starved
# its own fourth query, and reported that it could not establish anything.
#
# The remote here answers in two seconds. A window that cannot spare that much
# has to end the run on the last real answer rather than buy one more question
# it cannot afford. The clock does not tick per reading, so only the waits spend
# the window and the arithmetic is the script's own.
starving_case() {
    local name="$1" expect_code="$2" expect_text="$3" window="$4" min_query="$5"
    local dir="${work}/case" status=0 out
    rm -rf "${dir}"; mkdir -p "${dir}"
    printf '0' > "${dir}/step"
    printf '%s' "$(printf 'slow 2\n%s' "${ABSENT}")" > "${dir}/default"
    : > "${work}/sleeps"

    run_bounded 60 env FAKE_CONAN_REPLIES="${dir}" FAKE_SLEEP_LOG="${work}/sleeps" \
        FAKE_CLOCK_STATE="${dir}/clock" FAKE_CLOCK_STEP=0 \
        UTXOZ_PUBLISH_TIMEOUT="${window}" UTXOZ_PUBLISH_MIN_QUERY="${min_query}" \
        UTXOZ_PUBLISH_CLOCK=fakeclock UTXOZ_PUBLISH_SLEEP=fakesleep \
        "${SCRIPT}" kth utxoz/0.9.0 "${REVISION}" || status=$?
    out="$(cat "${work}/bounded.out")"

    if [[ ${status} -ne ${expect_code} || "${out}" != *"${expect_text}"* ]]; then
        printf 'FAIL  %-40s exited %d, expected %d saying %q\n' \
               "${name}" "${status}" "${expect_code}" "${expect_text}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
        return
    fi
    printf 'ok    %-40s rc=%d  %s\n' "${name}" "${status}" "${expect_text}"
}

# Absence stays absence. Two attempts answer, the window can no longer buy a
# third, and the verdict is the one the remote actually gave.
starving_case "absence survives a dying window" ${EXIT_ABSENT} \
              "2 of them a definite answer" 7 5

# And it is the window that stops it, not the deadline being reached: the run
# ends with time still on the clock, deliberately unspent.
starving_case "it stops before starving a query" ${EXIT_ABSENT} \
              "did not appear on kth within 7s" 7 5

echo
echo "== the wait itself can fail =="

# `sleep` is a command, and commands fail: not installed, killed, out of
# resources. Under `set -e` that status used to leave here as the script's own,
# which is neither 0, 3 nor 4 — a value the caller has no reading for.
sleep_case() {
    local name="$1" sleeper="$2"
    local dir="${work}/case" status=0 out
    rm -rf "${dir}"; mkdir -p "${dir}"
    printf '0' > "${dir}/step"
    printf '%s' "${ABSENT}" > "${dir}/default"

    run_bounded 30 env FAKE_CONAN_REPLIES="${dir}" FAKE_CLOCK_STATE="${dir}/clock" \
        FAKE_CLOCK_STEP=1 \
        UTXOZ_PUBLISH_TIMEOUT=60 UTXOZ_PUBLISH_CLOCK=fakeclock \
        UTXOZ_PUBLISH_SLEEP="${sleeper}" UTXOZ_REMOTE_QUERY_TIMEOUT=3 \
        "${SCRIPT}" kth utxoz/0.9.0 "${REVISION}" || status=$?
    out="$(cat "${work}/bounded.out")"

    if [[ ${status} -ne ${EXIT_UNKNOWN} || "${out}" != *"the wait command"* ]]; then
        printf 'FAIL  %-40s exited %d, expected %d blaming the wait command\n' \
               "${name}" "${status}" "${EXIT_UNKNOWN}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
        return
    fi
    printf 'ok    %-40s rc=%d  the wait command is blamed\n' "${name}" "${status}"
}

sleep_case "a wait that refuses is Unknown" false
sleep_case "a wait that does not exist is Unknown" no-such-sleep-command

echo
echo "== the wait is bounded even when its own settings are not =="

# A clock that never advances would leave the loop with no way to end.
cat > "${work}/bin/stuckclock" <<'STUCK'
#!/usr/bin/env bash
printf '0'
STUCK
chmod +x "${work}/bin/stuckclock"

# A clock that cannot be read at all.
cat > "${work}/bin/wordclock" <<'WORDS'
#!/usr/bin/env bash
printf 'soon'
WORDS
chmod +x "${work}/bin/wordclock"

# A clock that runs backwards — what `date +%s` does when NTP corrects a runner
# that has drifted forwards. The deadline recedes with every reading, so the
# window never closes and the wait would run until something outside killed it.
# This is the hazard the monotonic default exists to remove, and the bound has to
# hold even when a caller injects a clock that has it.
cat > "${work}/bin/backwardsclock" <<'BACK'
#!/usr/bin/env bash
state="${FAKE_CLOCK_STATE:?FAKE_CLOCK_STATE is required}"
current="$(cat "${state}" 2>/dev/null || printf '1000000')"
printf '%s' "$(( current - 5 ))" > "${state}"
printf '%s' "${current}"
BACK
chmod +x "${work}/bin/backwardsclock"

clock_case() {
    local name="$1" expect_code="$2" expect_text="$3" clock="$4"
    local dir="${work}/case" out status=0
    rm -rf "${dir}"; mkdir -p "${dir}"
    printf '0' > "${dir}/step"
    printf '%s' "${ABSENT}" > "${dir}/default"
    : > "${work}/sleeps"

    run_bounded 20 env FAKE_CONAN_REPLIES="${dir}" FAKE_SLEEP_LOG="${work}/sleeps" \
        FAKE_CLOCK_STATE="${dir}/clock" \
        UTXOZ_PUBLISH_TIMEOUT=10 UTXOZ_PUBLISH_CLOCK="${clock}" \
        UTXOZ_PUBLISH_SLEEP=fakesleep UTXOZ_REMOTE_QUERY_TIMEOUT=3 \
        "${SCRIPT}" kth utxoz/0.9.0 "${REVISION}" || status=$?
    out="$(cat "${work}/bounded.out")"

    if [[ ${status} -ne ${expect_code} || "${out}" != *"${expect_text}"* ]]; then
        printf 'FAIL  %-40s exited %d, expected %d saying %q\n' \
               "${name}" "${status}" "${expect_code}" "${expect_text}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
        return
    fi
    printf 'ok    %-40s rc=%d  %s\n' "${name}" "${status}" "${expect_text}"
}

clock_case "an unreadable clock does not pass" ${EXIT_UNKNOWN} "clock" wordclock

# A stopped clock leaves the deadline permanently in the future, so the time
# bound cannot end this run — the attempt bound has to, and it must end it as
# Unknown. Reporting absence here would be asserting that nothing arrived during
# a window whose length is precisely what is not known.
#
# `timeout` wraps it as a backstop: if the second bound were ever removed this
# case would hang instead of failing, and a hanging suite is a suite nobody runs.
clock_case "a stopped clock stops the wait" ${EXIT_UNKNOWN} "not advancing" stuckclock

# A backwards clock is worse than a stopped one — the window grows — and must
# end the same way.
clock_case "a backwards clock stops the wait" ${EXIT_UNKNOWN} "not advancing" backwardsclock

# And the clock the script reaches for when nobody injects one never does that.
# Two readings a second apart, and the later one is not smaller.
first="$(python3 "${ROOT}/ci/monotonic_seconds.py")"
sleep 1
second="$(python3 "${ROOT}/ci/monotonic_seconds.py")"
if [[ "${first}" =~ ^[0-9]+$ && "${second}" =~ ^[0-9]+$ ]] && (( second >= first )); then
    printf 'ok    %-40s %s then %s\n' "the default clock does not go back" "${first}" "${second}"
else
    printf 'FAIL  %-40s read %q then %q\n' "the default clock does not go back" "${first}" "${second}"
    failures=$((failures + 1))
fi

echo
echo "== bad usage is not an answer =="

status=0
out="$("${SCRIPT}" kth utxoz/0.9.0 2>&1)" || status=$?
if [[ ${status} -eq ${EXIT_UNKNOWN} && "${out}" == *usage* ]]; then
    printf 'ok    %-40s rc=%d  usage\n' "too few arguments" "${status}"
else
    printf 'FAIL  %-40s exited %d, expected %d with usage\n' "too few arguments" "${status}" "${EXIT_UNKNOWN}"
    failures=$((failures + 1))
fi

for bad in UTXOZ_PUBLISH_TIMEOUT UTXOZ_PUBLISH_POLL_INITIAL UTXOZ_PUBLISH_POLL_MAX \
           UTXOZ_PUBLISH_MIN_QUERY UTXOZ_REMOTE_QUERY_TIMEOUT; do
    status=0
    out="$(env "${bad}=soon" "${SCRIPT}" kth utxoz/0.9.0 "${REVISION}" 2>&1)" || status=$?
    if [[ ${status} -eq ${EXIT_UNKNOWN} && "${out}" == *"whole number of seconds"* ]]; then
        printf 'ok    %-40s rc=%d  refused\n' "${bad}=soon" "${status}"
    else
        printf 'FAIL  %-40s exited %d, expected %d\n' "${bad}=soon" "${status}" "${EXIT_UNKNOWN}"
        failures=$((failures + 1))
    fi
done

# Zero is numeric, so the check above lets it through — and for these two it is
# not a smaller setting but a disabled one. A gap of zero busy-loops against
# somebody else's server; a floor of zero restores the starved final query that
# turned an established absence into Unknown. Both are refused by value rather
# than by type.
#
# The window is pinned to a second as well. These settings are refused before
# the loop starts, so it does not change what is asserted — but if the refusal
# were ever removed, the case would fall through into a five-minute wait against
# a real remote instead of failing. A test whose running time depends on the
# guard it is testing reports a regression as a hang.
for zero in UTXOZ_PUBLISH_POLL_INITIAL UTXOZ_PUBLISH_MIN_QUERY; do
    status=0
    out="$(env "${zero}=0" UTXOZ_PUBLISH_TIMEOUT=1 \
           "${SCRIPT}" kth utxoz/0.9.0 "${REVISION}" 2>&1)" || status=$?
    if [[ ${status} -eq ${EXIT_UNKNOWN} && "${out}" == *"must be at least 1 second"* ]]; then
        printf 'ok    %-40s rc=%d  refused\n' "${zero}=0" "${status}"
    else
        printf 'FAIL  %-40s exited %d, expected %d refusing zero\n' "${zero}=0" "${status}" "${EXIT_UNKNOWN}"
        printf '%s\n' "${out}" | sed 's/^/        /'
        failures=$((failures + 1))
    fi
done

echo
if (( failures != 0 )); then
    printf '%d failed\n' "${failures}" >&2
    exit 1
fi
echo "all cases behaved as expected"
