#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# The release notes: generated once, carried, and never regenerated.
#
# What these cover is a class of bug that only shows up in the published
# artefact, days later, when somebody reads a release that cites itself or a
# changelog file that disagrees with the release above it. A release cannot be
# run to test it — it publishes — so `gh` is replaced by one that answers what
# each case needs and records what it was asked, and `git` runs against a real
# temporary repository.
#
# The old two-pass design is the negative control. Every assertion here is
# checked against it in the last section: a test that passes on the code it was
# written to reject is not testing anything.
#
# Run from anywhere:  scripts/test_release_lib.sh

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT
readonly LIB="${ROOT}/scripts/release_lib.sh"

failures=0

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT
mkdir -p "${work}/bin"
MISSING_PATTERNS="${work}/missing-patterns"
: > "${MISSING_PATTERNS}"

ok()   { printf 'ok    %-56s %s\n' "$1" "${2:-}"; }
bad()  { printf 'FAIL  %-56s %s\n' "$1" "${2:-}"; failures=$((failures + 1)); }

# The line a pattern is on, or a loud failure.
#
# `grep -n | cut` yields an empty string when the pattern is not there, and an
# empty string in an arithmetic comparison is zero — so an assertion about
# ordering would quietly pass because neither side was found. A renamed variable
# would then turn every ordering check into a no-op with no sign of it.
# Misses are recorded in a file, not by calling `bad`. This runs inside a command
# substitution, which is a subshell: a counter incremented in here is discarded
# when the subshell exits, so the failure would vanish and the caller would
# compare zero against zero and pass. The file survives; it is checked once every
# lookup has run.
line_of() {
    local pattern="$1" text="$2" n
    n="$(grep -nF -- "${pattern}" <<<"${text}" | head -1 | cut -d: -f1)"
    if [[ -z "${n}" ]]; then
        printf '%s\n' "${pattern}" >> "${MISSING_PATTERNS}"
        printf '0'
        return 1
    fi
    printf '%s' "${n}"
}

# shellcheck source=scripts/release_lib.sh
. "${LIB}"

# ---------------------------------------------------------------------------
# A gh that answers from files and records every call.
# ---------------------------------------------------------------------------
cat > "${work}/bin/gh" <<'FAKEGH'
#!/usr/bin/env bash
set -u
state="${FAKE_GH_STATE:?FAKE_GH_STATE is required}"
printf '%s\n' "$*" >> "${state}/calls"

# `${1:-}` and not `$1`: gh gets called with a single argument in places, and
# under `set -u` an unset $2 aborts the fake rather than the code under test —
# which would look like the code failing.
sub1="${1:-}"; sub2="${2:-}"

case "${sub1} ${sub2}" in
    "repo view")
        printf '%s' "${FAKE_GH_REPO:-utxo-z/utxo-z}"
        exit 0
        ;;
esac

if [[ "${sub1}" == "api" && "${sub2}" == *"releases/generate-notes"* ]]; then
    printf '%s\n' "$*" >> "${state}/generate-notes-calls"
    if [[ -f "${state}/generate-notes-fails" ]]; then
        echo "gh: HTTP 422 Unprocessable Entity" >&2
        exit 1
    fi
    # gh writes notices to stderr — deprecations, rate-limit warnings, upgrade
    # nags — while still succeeding and printing JSON on stdout.
    if [[ -f "${state}/generate-notes-warns" ]]; then
        echo "gh: A new release of gh is available: 2.40.0 -> 2.62.0" >&2
    fi
    cat "${state}/generate-notes-reply"
    exit 0
fi

if [[ "${sub1}" == "release" && "${sub2}" == "create" ]]; then
    if [[ -f "${state}/release-create-fails" ]]; then
        echo "gh: HTTP 422 Validation Failed" >&2
        exit 7
    fi
    exit 0
fi

if [[ "${sub1}" == "pr" && "${sub2}" == "view" ]]; then
    cat "${state}/pr-body"
    exit 0
fi

exit 0
FAKEGH
chmod +x "${work}/bin/gh"
export PATH="${work}/bin:${PATH}"

new_state() {
    local dir="${work}/state"
    rm -rf "${dir}"; mkdir -p "${dir}"
    : > "${dir}/calls"
    : > "${dir}/generate-notes-calls"
    printf '%s' "${dir}"
}

echo "== the notes are generated against the commit being released =="

state="$(new_state)"
export FAKE_GH_STATE="${state}"
# Built with jq rather than printf: a literal newline inside a JSON string is
# invalid JSON, and printf turns \n in its format into one.
jq -n --arg b '## What'"'"'s Changed
* thing by @someone in #99
' '{name:"v0.9.0", body:$b}' > "${state}/generate-notes-reply"

BASE_SHA="1111111111111111111111111111111111111111"
notes="$(generate_release_notes "0.9.0" "${BASE_SHA}" "v0.8.1")"
gen_call="$(cat "${state}/generate-notes-calls")"

if [[ "${gen_call}" == *"target_commitish=${BASE_SHA}"* ]]; then
    ok "generated against the given commit" "target_commitish=${BASE_SHA:0:7}"
else
    bad "generated against the given commit" "${gen_call}"
fi
[[ "${gen_call}" == *"tag_name=v0.9.0"* ]] \
    && ok "asks for the right tag name" || bad "asks for the right tag name" "${gen_call}"
[[ "${gen_call}" == *"previous_tag_name=v0.8.1"* ]] \
    && ok "bounds the range with the previous tag" || bad "bounds the range with the previous tag" "${gen_call}"
[[ "${notes}" == *"thing by @someone in #99"* ]] \
    && ok "returns the body GitHub produced" || bad "returns the body GitHub produced" "${notes}"

# A first release has no previous tag, and the field must be absent rather than
# empty: GitHub rejects a blank previous_tag_name.
state="$(new_state)"; export FAKE_GH_STATE="${state}"
jq -n '{body:"first release"}' > "${state}/generate-notes-reply"
generate_release_notes "0.1.0" "${BASE_SHA}" "" >/dev/null
if grep -q 'previous_tag_name' "${state}/generate-notes-calls"; then
    bad "a first release omits previous_tag_name" "$(cat "${state}/generate-notes-calls")"
else
    ok "a first release omits previous_tag_name"
fi

# No temporary tag or prerelease is created to read a body out of. The old
# design left `temp-v*` behind whenever it failed midway.
state="$(new_state)"; export FAKE_GH_STATE="${state}"
jq -n '{body:"x"}' > "${state}/generate-notes-reply"
generate_release_notes "0.9.0" "${BASE_SHA}" "v0.8.1" >/dev/null
if grep -qE 'release create|temp-v' "${state}/calls"; then
    bad "no temporary tag or release is created" "$(cat "${state}/calls")"
else
    ok "no temporary tag or release is created"
fi

echo
echo "== generating the notes can fail, and then nothing is created =="

state="$(new_state)"; export FAKE_GH_STATE="${state}"
touch "${state}/generate-notes-fails"
if generate_release_notes "0.9.0" "${BASE_SHA}" "v0.8.1" >/dev/null 2>&1; then
    bad "a GitHub failure is refused"
else
    ok "a GitHub failure is refused" "rc!=0"
fi

state="$(new_state)"; export FAKE_GH_STATE="${state}"
jq -n '{body:""}' > "${state}/generate-notes-reply"
if generate_release_notes "0.9.0" "${BASE_SHA}" "v0.8.1" >/dev/null 2>&1; then
    bad "an empty body is refused"
else
    ok "an empty body is refused" "rc!=0"
fi

state="$(new_state)"; export FAKE_GH_STATE="${state}"
jq -n '{something:"else"}' > "${state}/generate-notes-reply"
if generate_release_notes "0.9.0" "${BASE_SHA}" "v0.8.1" >/dev/null 2>&1; then
    bad "a reply with no body field is refused"
else
    ok "a reply with no body field is refused" "rc!=0"
fi

state="$(new_state)"; export FAKE_GH_STATE="${state}"
printf 'not json at all' > "${state}/generate-notes-reply"
if generate_release_notes "0.9.0" "${BASE_SHA}" "v0.8.1" >/dev/null 2>&1; then
    bad "a reply that is not JSON is refused"
else
    ok "a reply that is not JSON is refused" "rc!=0"
fi


# gh chatting on stderr is not a failure, and must not end up spliced into the
# JSON. Folding the two together with 2>&1 is what would break this.
state="$(new_state)"; export FAKE_GH_STATE="${state}"
touch "${state}/generate-notes-warns"
jq -n '{body:"## Changes\n* something"}' > "${state}/generate-notes-reply"
if warned="$(generate_release_notes "0.9.0" "${BASE_SHA}" "v0.8.1" 2>/dev/null)"; then
    if [[ "${warned}" == *"something"* && "${warned}" != *"new release of gh"* ]]; then
        ok "a warning on stderr does not corrupt the JSON"
    else
        bad "a warning on stderr does not corrupt the JSON" "${warned}"
    fi
else
    bad "a warning on stderr does not corrupt the JSON" "refused a good reply"
fi

# The Full Changelog GitHub emits points at the tag it was asked about, which
# does not exist while the pull request is open. Checked on the real URL shape,
# not a fragment.
state="$(new_state)"; export FAKE_GH_STATE="${state}"
jq -n '{body:"## What'"'"'s Changed\n* a change in https://github.com/utxo-z/utxo-z/pull/99\n\n**Full Changelog**: https://github.com/utxo-z/utxo-z/compare/v0.8.1...v0.9.0"}' \
    > "${state}/generate-notes-reply"
normalized="$(generate_release_notes "0.9.0" "${BASE_SHA}" "v0.8.1")"

[[ "${normalized}" == *"compare/v0.8.1...${BASE_SHA}"* ]] \
    && ok "the Full Changelog points at the commit" \
    || bad "the Full Changelog points at the commit" "${normalized##*Changelog}"
[[ "${normalized}" != *"compare/v0.8.1...v0.9.0"* ]] \
    && ok "…and no longer at the tag that does not exist yet" \
    || bad "…and no longer at the tag that does not exist yet"
[[ "${normalized}" == *"pull/99"* ]] \
    && ok "the rest of the notes is untouched" \
    || bad "the rest of the notes is untouched"

# An older tag mentioned in the notes is a real link and stays as it is.
[[ "$(normalize_future_tag_links "see /compare/v0.7.0...v0.8.1 here" "0.9.0" "${BASE_SHA}")" == *"v0.7.0...v0.8.1"* ]] \
    && ok "links to tags that do exist are left alone" \
    || bad "links to tags that do exist are left alone"

# A first release has a commits link instead of a compare.
[[ "$(normalize_future_tag_links "**Full Changelog**: https://github.com/utxo-z/utxo-z/commits/v0.1.0" "0.1.0" "${BASE_SHA}")" == *"commits/${BASE_SHA}"* ]] \
    && ok "a first release's commits link points at the commit" \
    || bad "a first release's commits link points at the commit"

echo
echo "== the pull request body =="

state="$(new_state)"; export FAKE_GH_STATE="${state}"
NOTES=$'## What\'s Changed\n* a change by @someone in https://github.com/utxo-z/utxo-z/pull/99\n\n**Full Changelog**: v0.8.1...v0.9.0'
body="$(render_pr_body "0.9.0" "v0.8.1" "${BASE_SHA}" "${NOTES}" "utxo-z/utxo-z")"

begin_count="$(grep -cF -- '<!-- release-notes:start -->' <<<"${body}")"
end_count="$(grep -cF -- '<!-- release-notes:end -->' <<<"${body}")"
if (( begin_count == 1 && end_count == 1 )); then
    ok "exactly one pair of markers"
else
    bad "exactly one pair of markers" "start=${begin_count} end=${end_count}"
fi

[[ "${body}" == *"v0.8.1"* ]]        && ok "names the previous tag"      || bad "names the previous tag"
[[ "${body}" == *"${BASE_SHA}"* ]]   && ok "names the exact commit"      || bad "names the exact commit"
[[ "${body}" == *"compare/v0.8.1...${BASE_SHA}"* ]] \
    && ok "compares against the SHA, not a future tag" \
    || bad "compares against the SHA, not a future tag"
[[ "${body}" != *"compare/v0.8.1...v0.9.0"* ]] \
    && ok "never links to the tag that does not exist yet" \
    || bad "never links to the tag that does not exist yet"
# The exact heading, not merely the word "empty" — which appears in the notes,
# in the checklist and in prose, so the loose match passed whether or not the
# explanation was there.
[[ "${body}" == *"## Why this commit is empty"* ]] \
    && ok "explains the empty commit under its own heading" \
    || bad "explains the empty commit under its own heading"
[[ "${body}" == *"- [ ]"* ]]         && ok "carries a checklist"         || bad "carries a checklist"

# A first release still renders, and does not link a compare against nothing.
# The notes themselves may legitimately mention a compare link, so the check is
# on the scaffolding this function renders: everything outside the marker block.
PLAIN_NOTES="## Changes
* a first change"
first="$(render_pr_body "0.1.0" "" "${BASE_SHA}" "${PLAIN_NOTES}" "utxo-z/utxo-z")"
# Everything before the marker block: parameter expansion rather than sed, so
# nothing in the marker has to be escaped for a regex.
first_scaffolding="${first%%"${RELEASE_NOTES_BEGIN}"*}"

[[ "${first_scaffolding}" == *"commits/${BASE_SHA}"* ]] \
    && ok "a first release links its commits" \
    || bad "a first release links its commits"
[[ "${first_scaffolding}" != *"/compare/"* ]] \
    && ok "a first release renders no compare link at all" \
    || bad "a first release renders no compare link at all" \
       "$(grep -o '/compare/[^)]*' <<<"${first_scaffolding}" | head -1)"

echo
echo "== the notes come back byte for byte =="

recovered="$(printf '%s' "${body}" | extract_release_notes)"
if [[ "${recovered}" == "${NOTES}" ]]; then
    ok "what went in is what comes out"
else
    bad "what went in is what comes out"
    diff <(printf '%s' "${NOTES}") <(printf '%s' "${recovered}") | head -6 | sed 's/^/        /'
fi

# The marker pull request cannot appear in its own notes, because the notes were
# taken before it existed. This is the release-cites-itself bug.
[[ "${recovered}" != *"release: 0.9.0"* ]] \
    && ok "the release pull request is not in its own notes" \
    || bad "the release pull request is not in its own notes"

echo
echo "== bodies that cannot be trusted are refused =="

reject() {
    local name="$1" candidate="$2" expect="$3"
    local out status=0
    out="$(printf '%s' "${candidate}" | extract_release_notes 2>&1)" || status=$?
    if (( status == 0 )); then
        bad "${name}" "accepted, and returned: ${out:0:60}"
        return
    fi
    if [[ "${out}" != *"${expect}"* ]]; then
        bad "${name}" "refused, but said: ${out:0:80}"
        return
    fi
    ok "${name}" "refused"
}

reject "an empty body"          ""                                   "empty body"
reject "a body of whitespace"   $'   \n\n  '                         "empty body"
reject "no markers at all"      "release: 0.9.0"                     "no release-notes markers"
reject "only the opening marker" $'intro\n<!-- release-notes:start -->\nnotes' "no release-notes markers"
reject "only the closing marker" $'intro\nnotes\n<!-- release-notes:end -->'   "no release-notes markers"
reject "duplicated markers" \
    $'<!-- release-notes:start -->\na\n<!-- release-notes:end -->\n<!-- release-notes:start -->\nb\n<!-- release-notes:end -->' \
    "Exactly one of each"
reject "inverted markers" \
    $'<!-- release-notes:end -->\nnotes\n<!-- release-notes:start -->' \
    "wrong order"
reject "markers with nothing between them" \
    $'intro\n<!-- release-notes:start -->\n<!-- release-notes:end -->\nrest' \
    "nothing between them"
# Two markers on one line: `grep -c` counts lines and would see one of each,
# so this only fails when occurrences are counted rather than lines.
reject "two opening markers on one line" \
    $'<!-- release-notes:start --> notes <!-- release-notes:start -->\nmore\n<!-- release-notes:end -->' \
    "Exactly one of each"
reject "an opening and a closing marker on one line" \
    $'intro\n<!-- release-notes:start --> the notes <!-- release-notes:end -->\nrest' \
    "wrong order"

reject "markers with only blank lines between them" \
    $'intro\n<!-- release-notes:start -->\n\n   \n<!-- release-notes:end -->' \
    "blank"

echo
echo "== the previous tag ignores the old temporary tags =="

repo="${work}/repo"
rm -rf "${repo}"; mkdir -p "${repo}"
(
    cd "${repo}" || exit 1
    git init -q .
    git config user.email t@example.com; git config user.name test
    git commit -q --allow-empty -m one
    git tag -a v0.8.0 -m v0.8.0
    git commit -q --allow-empty -m two
    git tag -a v0.8.1 -m v0.8.1
    # Left behind by an interrupted run of the old two-pass release.
    git tag -a temp-v0.9.0 -m temp
)

found="$(cd "${repo}" && previous_stable_tag "v0.9.0")"
[[ "${found}" == "v0.8.1" ]] \
    && ok "picks the newest stable tag" "${found}" \
    || bad "picks the newest stable tag" "got '${found}'"

# The leftover temp tag must not be picked. It sorts above every real tag, so a
# selection that does not exclude it returns it — and the notes would then start
# from a tag that was never a release.
[[ "${found}" != temp-* ]] \
    && ok "a leftover temp-v tag is never the previous release" \
    || bad "a leftover temp-v tag is never the previous release" "got '${found}'"


(cd "${repo}" && git tag -a v0.10.0 -m v0.10.0)
found="$(cd "${repo}" && previous_stable_tag "v0.11.0")"
[[ "${found}" == "v0.10.0" ]] \
    && ok "sorts by version, not alphabetically" "${found}" \
    || bad "sorts by version, not alphabetically" "got '${found}' (v0.9.0 > v0.10.0 alphabetically)"

found="$(cd "${repo}" && previous_stable_tag "v0.10.0")"
[[ "${found}" == "v0.8.1" ]] \
    && ok "excludes the tag being released" "${found}" \
    || bad "excludes the tag being released" "got '${found}'"

# Only vMAJOR.MINOR.PATCH counts as a previous release.
(
    cd "${repo}" || exit 1
    git tag -a v0.11.0-rc1 -m rc
    git tag -a v0.12 -m malformed
    git tag -a nightly-2026-08-10 -m nightly
)
found="$(cd "${repo}" && previous_stable_tag "v1.0.0")"
[[ "${found}" == "v0.10.0" ]] \
    && ok "prereleases and malformed tags are not previous releases" "${found}" \
    || bad "prereleases and malformed tags are not previous releases" "got '${found}'"

# Printing the right answer is not enough: the function has to succeed as well.
# `git tag | while ...; break; done` closes the pipe, git takes SIGPIPE, and a
# caller running under `set -e -o pipefail` aborts on the one path that found
# what it was looking for — while the value it captured was correct all along,
# which is why comparing values alone never catches this.
pipefail_status=0
bash -c 'set -euo pipefail; . "$1"; cd "$2"; previous_stable_tag v1.0.0 >/dev/null' \
    _ "${LIB}" "${repo}" || pipefail_status=$?
(( pipefail_status == 0 )) \
    && ok "it succeeds, and not only prints, under set -e -o pipefail" \
    || bad "it succeeds, and not only prints, under set -e -o pipefail" "exit ${pipefail_status}"

# With few tags the hazard hides: git writes everything into the pipe buffer and
# exits before the loop stops reading, so there is no SIGPIPE and the old shape
# passes. It only appears once the output is larger than the buffer — which a
# repository accumulates over years and a test has to arrange deliberately.
mkdir -p "${work}/bigbin"
cat > "${work}/bigbin/git" <<'BIGGIT'
#!/usr/bin/env bash
if [[ "${1:-}" == "tag" ]]; then
    # Newest first, as --sort=-v:refname would give, and far past any pipe buffer.
    for i in $(seq 9000 -1 1); do printf 'v9.%d.0\n' "$i"; done
    exit 0
fi
exec /usr/bin/env git "$@"
BIGGIT
chmod +x "${work}/bigbin/git"

bigpipe_status=0
PATH="${work}/bigbin:${PATH}" bash -c \
    'set -euo pipefail; . "$1"; previous_stable_tag v9.9999.0 >/dev/null' _ "${LIB}" \
    || bigpipe_status=$?
(( bigpipe_status == 0 )) \
    && ok "…and with more tags than fit in a pipe buffer" \
    || bad "…and with more tags than fit in a pipe buffer" "exit ${bigpipe_status}"

# A git that cannot answer is not a repository with no tags. Run outside any
# repository, so `git tag` genuinely fails rather than being faked.
outside="${work}/not-a-repo"
mkdir -p "${outside}"
enumerate_status=0
( cd "${outside}" && previous_stable_tag "v1.0.0" ) >/dev/null 2>&1 || enumerate_status=$?
(( enumerate_status != 0 )) \
    && ok "a tag list that cannot be read is a failure, not an empty list" "rc=${enumerate_status}" \
    || bad "a tag list that cannot be read is a failure, not an empty list" \
       "returned success, so the caller would think this is a first release"

empty_repo="${work}/empty"
mkdir -p "${empty_repo}"
(cd "${empty_repo}" && git init -q . && git config user.email t@e.com && git config user.name t && git commit -q --allow-empty -m one)
found="$(cd "${empty_repo}" && previous_stable_tag "v0.1.0")"
[[ -z "${found}" ]] \
    && ok "a repository with no tags yields nothing" \
    || bad "a repository with no tags yields nothing" "got '${found}'"

echo
echo "== a release that cannot be created leaves no tag behind =="

# A real repository with a real remote, because what is being checked is that a
# tag stops existing in both places. The tag has to be pushed before
# `gh release create` can reference it, which is the window this closes: a
# failure there would otherwise leave the remote carrying a v0.9.0 pointing at
# something that was never published.
publish_repo="${work}/publish"
publish_remote="${work}/publish-remote.git"
rm -rf "${publish_repo}" "${publish_remote}"
git init -q --bare "${publish_remote}"
git init -q "${publish_repo}"
(
    cd "${publish_repo}" || exit 1
    git config user.email t@example.com; git config user.name test
    git remote add origin "${publish_remote}"
    git commit -q --allow-empty -m one
    git push -q origin HEAD:refs/heads/master
)

state="$(new_state)"; export FAKE_GH_STATE="${state}"
touch "${state}/release-create-fails"
notes_file="${work}/notes.md"
printf '## Changes\n* one\n' > "${notes_file}"

publish_status=0
( cd "${publish_repo}" && publish_release "0.9.0" "${notes_file}" ) >/dev/null 2>&1 || publish_status=$?

if (( publish_status == 7 )); then
    ok "the original failure is what comes back" "exit ${publish_status}"
else
    bad "the original failure is what comes back" "got ${publish_status}, expected gh's 7"
fi

local_tag="$(cd "${publish_repo}" && git tag --list 'v0.9.0')"
[[ -z "${local_tag}" ]] \
    && ok "no local tag is left behind" \
    || bad "no local tag is left behind" "found ${local_tag}"

remote_tag="$(git --git-dir="${publish_remote}" tag --list 'v0.9.0')"
[[ -z "${remote_tag}" ]] \
    && ok "no remote tag is left behind" \
    || bad "no remote tag is left behind" "found ${remote_tag}"

# And the success path still tags, pushes and keeps it.
state="$(new_state)"; export FAKE_GH_STATE="${state}"
publish_status=0
( cd "${publish_repo}" && publish_release "0.9.0" "${notes_file}" ) >/dev/null 2>&1 || publish_status=$?

(( publish_status == 0 )) \
    && ok "a release that works reports success" \
    || bad "a release that works reports success" "exit ${publish_status}"
[[ -n "$(cd "${publish_repo}" && git tag --list 'v0.9.0')" ]] \
    && ok "…and keeps the tag locally" || bad "…and keeps the tag locally"
[[ -n "$(git --git-dir="${publish_remote}" tag --list 'v0.9.0')" ]] \
    && ok "…and on the remote" || bad "…and on the remote"

echo
echo "== an entry that says something else is a conflict, not idempotence =="

notes_probe="${work}/notes-probe.md"
PROBE_NOTES="## Changes
* the one change"

fresh_notes() { printf '# version 0.8.1\n\nolder notes\n' > "${notes_probe}"; }

fresh_notes
probe_status=0
record_release_notes "0.9.0" "${PROBE_NOTES}" "${notes_probe}" || probe_status=$?
(( probe_status == 0 )) \
    && ok "an absent version is written" "rc=0" \
    || bad "an absent version is written" "rc=${probe_status}"

probe_status=0
record_release_notes "0.9.0" "${PROBE_NOTES}" "${notes_probe}" || probe_status=$?
(( probe_status == 10 )) \
    && ok "the identical entry is left alone" "rc=10" \
    || bad "the identical entry is left alone" "rc=${probe_status}"

# The one that used to pass: same heading, different text. Accepting it publishes
# the release from the pull request while the file keeps saying something else.
probe_status=0
record_release_notes "0.9.0" "## Changes
* something entirely different" "${notes_probe}" >/dev/null 2>&1 || probe_status=$?
(( probe_status != 0 && probe_status != 10 )) \
    && ok "a different entry for the same version is refused" "rc=${probe_status}" \
    || bad "a different entry for the same version is refused" \
       "rc=${probe_status} — the file and the release would disagree"

# Truncated: the heading is there and the notes under it are not all there.
fresh_notes
printf '# version 0.9.0\n\n## Changes\n\n# version 0.8.1\n\nolder\n' > "${notes_probe}"
probe_status=0
record_release_notes "0.9.0" "${PROBE_NOTES}" "${notes_probe}" >/dev/null 2>&1 || probe_status=$?
(( probe_status != 0 && probe_status != 10 )) \
    && ok "a truncated entry is refused" "rc=${probe_status}" \
    || bad "a truncated entry is refused" "rc=${probe_status}"

# Two headings for one version: which is the entry cannot be guessed.
printf '# version 0.9.0\n\nfirst\n\n# version 0.9.0\n\nsecond\n' > "${notes_probe}"
probe_status=0
probe_out="$(record_release_notes "0.9.0" "${PROBE_NOTES}" "${notes_probe}" 2>&1)" || probe_status=$?
# The message is asserted, not merely the failure: without the duplicate check
# the function breaks anyway, on malformed arithmetic, and a test that accepts
# any error would pass on a version that has no such check at all.
if (( probe_status != 0 && probe_status != 10 )) && [[ "${probe_out}" == *"more than one heading"* ]]; then
    ok "a duplicated heading is refused, and named" "rc=${probe_status}"
else
    bad "a duplicated heading is refused, and named" "rc=${probe_status}: ${probe_out:0:70}"
fi

# An entry whose notes are longer than what is being published is different, not
# a prefix match.
fresh_notes
record_release_notes "0.9.0" "${PROBE_NOTES}
* and another line" "${notes_probe}" >/dev/null
probe_status=0
record_release_notes "0.9.0" "${PROBE_NOTES}" "${notes_probe}" >/dev/null 2>&1 || probe_status=$?
(( probe_status != 0 && probe_status != 10 )) \
    && ok "a longer existing entry is not treated as the same" "rc=${probe_status}" \
    || bad "a longer existing entry is not treated as the same" "rc=${probe_status}"

echo
echo "== a retry after a failed release leaves one entry and one commit =="

# The whole cycle, because the failure and the retry are only a problem
# together: publish_release withdraws its tag so the release can be attempted
# again, and the notes commit is already on master by then. A second run that
# prepends the entry again would leave the file saying the same thing twice and
# master carrying two identical docs commits.
cycle="${work}/cycle"
cycle_remote="${work}/cycle-remote.git"
rm -rf "${cycle}" "${cycle_remote}"
git init -q --bare "${cycle_remote}"
git init -q "${cycle}"
(
    cd "${cycle}" || exit 1
    git config user.email t@example.com; git config user.name test
    git remote add origin "${cycle_remote}"
    mkdir -p doc/release-notes
    printf '# version 0.8.1\n\nolder notes\n' > doc/release-notes/release-notes.md
    git add -A && git commit -q -m "notes"
    git push -q origin HEAD:refs/heads/master
)

CYCLE_NOTES="## Changes
* the one change"
cycle_file="${cycle}/doc/release-notes/release-notes.md"

# --- first attempt: the notes land, gh fails, the tag is withdrawn
state="$(new_state)"; export FAKE_GH_STATE="${state}"
touch "${state}/release-create-fails"
printf '%s\n' "${CYCLE_NOTES}" > "${work}/cycle-notes.md"

first_status=0
(
    cd "${cycle}" || exit 1
    record_release_notes "0.9.0" "${CYCLE_NOTES}" "doc/release-notes/release-notes.md"
    git add -A && git commit -q -m "docs: update release notes for v0.9.0"
    git push -q origin HEAD:refs/heads/master
    publish_release "0.9.0" "${work}/cycle-notes.md"
) >/dev/null 2>&1 || first_status=$?

(( first_status == 7 )) \
    && ok "the first attempt fails with gh's status" \
    || bad "the first attempt fails with gh's status" "got ${first_status}"
[[ -z "$(cd "${cycle}" && git tag --list 'v0.9.0')" ]] \
    && ok "…and withdraws its tag" || bad "…and withdraws its tag"

# --- retry: the notes are already recorded, so nothing is added
state="$(new_state)"; export FAKE_GH_STATE="${state}"
retry_status=0
retry_recorded=0
(
    cd "${cycle}" || exit 1
    record_release_notes "0.9.0" "${CYCLE_NOTES}" "doc/release-notes/release-notes.md" || exit $?
) >/dev/null 2>&1 || retry_recorded=$?

(( retry_recorded == 10 )) \
    && ok "the retry sees the entry is already there" "rc=10" \
    || bad "the retry sees the entry is already there" "got ${retry_recorded}, expected 10"

(
    cd "${cycle}" || exit 1
    publish_release "0.9.0" "${work}/cycle-notes.md"
) >/dev/null 2>&1 || retry_status=$?

(( retry_status == 0 )) \
    && ok "…and the release is created this time" \
    || bad "…and the release is created this time" "exit ${retry_status}"
[[ -n "$(cd "${cycle}" && git tag --list 'v0.9.0')" ]] \
    && ok "…and the tag stays" || bad "…and the tag stays"

entries="$(grep -cxF '# version 0.9.0' "${cycle_file}")"
(( entries == 1 )) \
    && ok "exactly one entry in the notes file" \
    || bad "exactly one entry in the notes file" "found ${entries}"

docs_commits="$(cd "${cycle}" && git log --oneline --grep='release notes for v0.9.0' | wc -l | tr -d '[:space:]')"
(( docs_commits == 1 )) \
    && ok "exactly one docs commit" \
    || bad "exactly one docs commit" "found ${docs_commits}"

# One entry is not enough: it has to be the same text the release was published
# from. Compared against the file handed to publish_release, not against the
# variable, because the file is what gh received.
persisted="$(cd "${cycle}" && recorded_release_notes "0.9.0" "doc/release-notes/release-notes.md")"
published="$(cat "${work}/cycle-notes.md")"
if [[ "${persisted}" == "$(printf '%s' "${published}")" ]]; then
    ok "the persisted entry is exactly what was published"
else
    bad "the persisted entry is exactly what was published"
    diff <(printf '%s' "${persisted}") <(printf '%s' "${published}") | head -6 | sed 's/^/        /'
fi

echo
echo "== the fake gh survives a single argument under set -u =="

state="$(new_state)"; export FAKE_GH_STATE="${state}"
one_arg_status=0
gh --version >/dev/null 2>&1 || one_arg_status=$?
(( one_arg_status == 0 )) \
    && ok "gh called with one argument does not abort" \
    || bad "gh called with one argument does not abort" "exit ${one_arg_status}"

echo
echo "== the library can be sourced twice =="

resource_status=0
bash -c 'set -euo pipefail; . "$1"; . "$1"' _ "${LIB}" >/dev/null 2>&1 || resource_status=$?
(( resource_status == 0 )) \
    && ok "sourcing twice under set -e does not abort" \
    || bad "sourcing twice under set -e does not abort" "exit ${resource_status}"

# Unset means define and freeze.
frozen_status=0
bash -c 'set -euo pipefail; . "$1"; RELEASE_NOTES_BEGIN=x' _ "${LIB}" >/dev/null 2>&1 || frozen_status=$?
(( frozen_status != 0 )) \
    && ok "the marker is readonly once defined" \
    || bad "the marker is readonly once defined" "it was reassignable"

# Already set to the value we mean: that is what a second source looks like, and
# it is fine.
same_status=0
bash -c 'set -euo pipefail; RELEASE_NOTES_BEGIN="<!-- release-notes:start -->"; . "$1"' \
    _ "${LIB}" >/dev/null 2>&1 || same_status=$?
(( same_status == 0 )) \
    && ok "a marker preset to the expected value is accepted" \
    || bad "a marker preset to the expected value is accepted" "exit ${same_status}"

# Preset to the right value but still writable — by a caller, or by an earlier
# source. The contract is that these do not change, and it should not rest on
# nobody changing them.
freeze_status=0
bash -c 'set -euo pipefail
         RELEASE_NOTES_BEGIN="<!-- release-notes:start -->"
         . "$1"
         RELEASE_NOTES_BEGIN=changed' _ "${LIB}" >/dev/null 2>&1 || freeze_status=$?
(( freeze_status != 0 )) \
    && ok "a preset marker is frozen too" \
    || bad "a preset marker is frozen too" "it stayed writable"

# Set to something else: the body would be written with one marker and read back
# with another, and the notes would come out empty or wrong halfway through a
# release. That has to be loud.
for preset in RELEASE_NOTES_BEGIN RELEASE_NOTES_END; do
    conflict_status=0
    conflict_out="$(bash -c "set -euo pipefail; ${preset}='something else'; . \"\$1\"" _ "${LIB}" 2>&1)" \
        || conflict_status=$?
    if (( conflict_status != 0 )) && [[ "${conflict_out}" == *"other than the release-notes marker"* ]]; then
        ok "a conflicting ${preset} is refused" "rc=${conflict_status}"
    else
        bad "a conflicting ${preset} is refused" "rc=${conflict_status}: ${conflict_out:0:60}"
    fi
done

echo
echo "== the scripts themselves =="

release_sh="$(cat "${ROOT}/scripts/release.sh")"
post_sh="$(cat "${ROOT}/scripts/post-release.sh")"

# Ordering is the substance of the fix: the notes have to be taken while nothing
# about the release exists. Asserted against the source because running it would
# create a branch, a pull request and eventually a tag.
notes_at=$(line_of 'generate_release_notes "${VERSION}"' "${release_sh}")
branch_at=$(line_of 'git checkout -b' "${release_sh}")
commit_at=$(line_of 'git commit --allow-empty' "${release_sh}")
pr_at=$(line_of 'gh pr create' "${release_sh}")

if (( notes_at < branch_at && notes_at < commit_at && notes_at < pr_at )); then
    ok "notes are generated before branch, commit and pull request"
else
    bad "notes are generated before branch, commit and pull request" \
        "notes@${notes_at} branch@${branch_at} commit@${commit_at} pr@${pr_at}"
fi

grep -q 'body-file' <<<"${release_sh}" \
    && ok "the pull request body comes from a file" \
    || bad "the pull request body comes from a file"
grep -q -- '--body "release: ' <<<"${release_sh}" \
    && bad "the one-line body is gone" \
    || ok "the one-line body is gone"

grep -q 'extract_release_notes' <<<"${post_sh}" \
    && ok "post-release reads the notes from the pull request" \
    || bad "post-release reads the notes from the pull request"
lib_src="$(cat "${LIB}")"
grep -qF -- '--notes-file' <<<"${lib_src}" \
    && ok "the release is created from those notes" \
    || bad "the release is created from those notes"
# The endpoint `releases/generate-notes` is the right thing to call and appears
# here; the flag `--generate-notes` on `gh release create` is the wrong thing and
# must not. Comments are stripped so prose about the old design does not count.
lib_code="$(grep -v '^[[:space:]]*#' <<<"${lib_src}")"
grep -qF -- '--generate-notes' <<<"${lib_code}" \
    && bad "the release is never created with --generate-notes" \
    || ok "the release is never created with --generate-notes"
grep -qF -- 'releases/generate-notes' <<<"${lib_code}" \
    && ok "the notes come from the generate-notes endpoint" \
    || bad "the notes come from the generate-notes endpoint"

# `\s` is a GNU extension; BSD grep on macOS does not know it and would match
# a literal "s". POSIX character classes work on both.
post_code="$(grep -v '^[[:space:]]*#' <<<"${post_sh}")"

for forbidden in '--generate-notes' 'temp-v' 'TEMP_TAG'; do
    if grep -qF -- "${forbidden}" <<<"${post_code}"; then
        bad "post-release no longer uses ${forbidden}"
    else
        ok "post-release no longer uses ${forbidden}"
    fi
done

# The notes file and the GitHub release are fed from the same variable. Two
# sources is how they came to disagree in the first place.
uses_for_file="$(grep -cF 'record_release_notes "${VERSION}" "${RELEASE_NOTES}"' <<<"${post_sh}")"
uses_for_release="$(grep -cF '"${RELEASE_NOTES}" > "${RELEASE_NOTES_FILE}"' <<<"${post_sh}")"
assignments="$(grep -cE '^RELEASE_NOTES=|RELEASE_NOTES="\$\(' <<<"${post_sh}")"

if (( uses_for_file == 1 && uses_for_release == 1 && assignments == 1 )); then
    ok "the file and the release come from one variable, assigned once"
else
    bad "the file and the release come from one variable, assigned once" \
        "file=${uses_for_file} release=${uses_for_release} assignments=${assignments}"
fi

# And recording it is conditional, so a retry after a failed release does not
# write the entry a second time.
grep -qF 'notes_status' <<<"${post_sh}" \
    && ok "recording the notes is conditional on them not being recorded" \
    || bad "recording the notes is conditional on them not being recorded"

# The guarantees that were already there.
grep -q 'PR_STATE.*MERGED' <<<"${post_sh}" \
    && ok "still refuses to tag an unmerged pull request" \
    || bad "still refuses to tag an unmerged pull request"
grep -q 'merge-base --is-ancestor' <<<"${post_sh}" \
    && ok "still refuses to tag when the merge is not on master" \
    || bad "still refuses to tag when the merge is not on master"

tag_at=$(line_of 'publish_release "${VERSION}"' "${post_sh}")
extract_at=$(line_of 'extract_release_notes' "${post_sh}")
merge_at=$(line_of 'gh pr merge --squash' "${post_sh}")
if (( extract_at < tag_at )); then
    ok "the notes are recovered before anything is tagged"
else
    bad "the notes are recovered before anything is tagged" "extract@${extract_at} tag@${tag_at}"
fi

branch_delete_at=$(line_of 'push origin --delete "release/' "${post_sh}")
release_at=$(line_of 'publish_release "${VERSION}"' "${post_sh}")
if (( release_at < branch_delete_at )); then
    ok "the branch is not deleted until the release exists"
else
    bad "the branch is not deleted until the release exists" "release@${release_at} delete@${branch_delete_at}"
fi

# An invalid body must be refused while refusing is still free. After the merge,
# master already carries the release commit and there is no tag and no release.
if (( extract_at < merge_at )); then
    ok "the notes are validated before the pull request is merged"
else
    bad "the notes are validated before the pull request is merged" \
        "extract@${extract_at} merge@${merge_at}"
fi

grep -qF -- 'git fetch --tags origin' <<<"${release_sh}" \
    && ok "tags are synced from origin before choosing the previous one" \
    || bad "tags are synced from origin before choosing the previous one"
grep -qF -- 'fetch --tags --force' <<<"${release_sh}" \
    && bad "the tag sync does not force" \
    || ok "the tag sync does not force"

# A pre-existing pull request carries somebody else's body, so the run must stop
# rather than continue into CI and, later, a merge whose notes nobody generated
# for this commit. The abort has to sit between creating the pull request and
# waiting for CI — a window, not a fixed number of lines, because the message
# around it will grow.
abort_at=$(line_of 'Aborting: a release must not be cut' "${release_sh}")
wait_at=$(line_of 'Waiting for the build to finish' "${release_sh}")
if (( pr_at < abort_at && abort_at < wait_at )); then
    ok "an existing pull request aborts before CI is waited on" "line ${abort_at}"
else
    bad "an existing pull request aborts before CI is waited on" \
        "pr@${pr_at} abort@${abort_at} wait@${wait_at}"
fi

grep -qE 'grep -P|\\\\s|--include=' <<<"${release_sh}${post_sh}$(cat "${LIB}")" \
    && bad "no GNU-only grep constructs in the release scripts" \
    || ok "no GNU-only grep constructs in the release scripts"

grep -qE 'conan' <<<"${post_sh}${release_sh}" \
    && bad "the release scripts do not touch Conan packages" \
    || ok "the release scripts do not touch Conan packages"

# Every ordering assertion above rests on these having been found. A renamed
# command would otherwise turn each of them into a comparison of zero with zero.
if [[ -s "${MISSING_PATTERNS}" ]]; then
    while IFS= read -r missing; do
        bad "structural pattern not found in the scripts" "${missing}"
    done < "${MISSING_PATTERNS}"
else
    ok "every structural pattern was found where it was looked for"
fi

echo
echo "== the controls fail against the previous implementation =="

# The old post-release.sh, as it was: temp tag, prerelease, scrape, sed, and
# --generate-notes again for the real release. If the assertions above pass on
# this, they are not assertions.
old_post="$(cat <<'OLD'
TEMP_TAG="temp-v${VERSION}"
git tag -a "${TEMP_TAG}" -m "Temporary tag for release notes generation"
git push origin "${TEMP_TAG}"
gh release create "${TEMP_TAG}" --title "temp-v${VERSION}" --generate-notes --prerelease
RELEASE_NOTES=$(gh release view "${TEMP_TAG}" --json body -q '.body')
RELEASE_NOTES=$(echo "$RELEASE_NOTES" | sed "s/temp-v${VERSION}/v${VERSION}/g")
gh release create "v${VERSION}" --title "v${VERSION}" --generate-notes --latest
OLD
)"

old_failures=0
grep -q 'extract_release_notes' <<<"${old_post}" || old_failures=$((old_failures + 1))
grep -q -- '--notes-file' <<<"${old_post}"       || old_failures=$((old_failures + 1))
grep -q -- '--generate-notes' <<<"${old_post}"   && old_failures=$((old_failures + 1))
grep -q 'temp-v' <<<"${old_post}"                && old_failures=$((old_failures + 1))

if (( old_failures == 4 )); then
    ok "all four script assertions reject the old implementation"
else
    bad "all four script assertions reject the old implementation" "only ${old_failures}/4 rejected"
fi

# And the old one-line pull request body carries no notes at all, so
# post-release could not have reused anything from it.
if printf '%s' "release: 0.9.0" | extract_release_notes >/dev/null 2>&1; then
    bad "the old one-line body is rejected by the extractor"
else
    ok "the old one-line body is rejected by the extractor"
fi

echo
if (( failures != 0 )); then
    printf '%d failed\n' "${failures}" >&2
    exit 1
fi
echo "all cases behaved as expected"
