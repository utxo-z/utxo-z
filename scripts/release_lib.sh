#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# The release notes, generated once and carried from the pull request to the
# GitHub release.
#
# They used to be generated twice, and the two disagreed. post-release.sh created
# a `temp-v${VERSION}` tag, made a prerelease from it with `--generate-notes`,
# scraped the body back out, rewrote `temp-v` to `v` with sed, wrote that into
# doc/release-notes/release-notes.md, committed it — and then created the real
# release with `--generate-notes` a second time. By that second call the release
# PR had merged and the docs commit existed, so the notes GitHub produced were
# not the notes in the file. The release also cited itself: the marker PR was
# inside its own range.
#
# Here the notes are a photograph, taken once, before anything about the release
# exists. release.sh asks GitHub for them against the master commit it is about
# to release, embeds them in the pull request body between markers, and
# post-release.sh reads them back from that same body — byte for byte — for both
# the file and the release. Nothing regenerates them, so nothing can disagree.
#
# Sourced by scripts/release.sh and scripts/post-release.sh, and exercised
# directly by scripts/test_release_lib.sh.

# Deliberately not `set -e` here: this file is sourced, and turning on errexit
# would change the behaviour of whatever sourced it. Every function reports
# through its exit status instead.

# The markers, initialised so that sourcing this file again is harmless and
# sourcing it into a shell that already means something else by these names is
# not.
#
# Three cases, and they are different. Unset: define and freeze. Set to the value
# we expect: leave it, which is what a second `source` looks like. Set to
# something else: refuse — the pull request body would be written with one pair
# of markers and read back with another, and the notes would come out empty or
# wrong at the point where the release is already half-made.
#
# `readonly` cannot simply be re-run: it fails on an already-readonly name, and
# both scripts here run under `set -e`.
__release_notes_marker() {
    local name="$1" expected="$2"

    if [[ -z "${!name+x}" ]]; then
        printf -v "${name}" '%s' "${expected}"
        readonly "${name}"
        return 0
    fi

    if [[ "${!name}" == "${expected}" ]]; then
        # Right value, but possibly still writable — set by a caller, or by an
        # earlier source that ran before this guard existed. Freeze it anyway:
        # the contract is that these do not change, and it should not depend on
        # nobody happening to change them. `readonly` with no assignment is a
        # no-op on a name that is already readonly.
        readonly "${name}"
        return 0
    fi

    if true; then
        echo "${name} is already set to something other than the release-notes marker." >&2
        echo "  expected: ${expected}" >&2
        echo "  found:    ${!name}" >&2
        echo "Refusing to continue: the notes would be written with one marker and" >&2
        echo "read back with another." >&2
        return 1
    fi
}

__release_notes_marker RELEASE_NOTES_BEGIN '<!-- release-notes:start -->' || return 1
__release_notes_marker RELEASE_NOTES_END '<!-- release-notes:end -->' || return 1

# The previous stable tag, or nothing when there is none.
#
# Only exact release tags count: vMAJOR.MINOR.PATCH and nothing else. That
# excludes the `temp-v*` tags the old two-pass release left behind when it was
# interrupted, prereleases like `v0.9.0-rc1`, and malformed names like `v0.9`.
# Any of those chosen as the previous release produces notes over a range that
# never shipped — and a prerelease would quietly drop every change between it
# and the last real release.
#
# Sorted by version rather than by creation date, because tags get created out of
# order: a patch on an old branch is newer in time and older in version.
#
# Reads: git tags in the current repository.
previous_stable_tag() {
    local exclude="${1:-}" tags tag

    # Collected first, then walked. A `git tag | while ...; break; done` closes
    # the pipe on the first match, git takes SIGPIPE, and under `pipefail` in the
    # calling script the whole thing reports failure having actually succeeded —
    # so the release would abort on the one path that found what it was looking
    # for. A here-string has no pipeline to fail.
    # A git that fails is not a repository with no tags. Returning success with
    # no output would tell the caller "this is the first release", and the notes
    # would then be generated over the whole history — every version this project
    # ever shipped, presented as the contents of this one.
    if ! tags="$(git tag --list 'v*' --sort=-v:refname 2>/dev/null)"; then
        echo "Could not list tags; the previous release cannot be determined." >&2
        return 1
    fi

    # Enumerated successfully and there is nothing: a genuine first release.
    [[ -n "${tags}" ]] || return 0

    while IFS= read -r tag; do
        [[ -n "${tag}" ]] || continue
        # Exactly vMAJOR.MINOR.PATCH. Anything else is not a release this project
        # made: `temp-v0.9.0` from the old two-pass script, `v0.9.0-rc1` from a
        # prerelease, `v0.9` from a typo. Any of them chosen as the previous
        # release produces notes over a range that does not correspond to
        # anything shipped — and a prerelease in particular would silently drop
        # everything between it and the last real release.
        [[ "${tag}" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]] || continue
        [[ -n "${exclude}" && "${tag}" == "${exclude}" ]] && continue
        printf '%s\n' "${tag}"
        return 0
    done <<<"${tags}"

    return 0
}

# The notes GitHub would put on this release, asked for before the release
# exists.
#
# `releases/generate-notes` computes them from a range without creating anything:
# no tag, no draft, no prerelease. The old code created a real prerelease to read
# its body, which is why a failed release left `temp-v*` tags and prereleases
# behind for somebody to clean up.
#
# Prints the notes on stdout. Returns non-zero, with a reason on stderr, when
# GitHub cannot be asked, answers with something that is not a body, or answers
# with an empty one. There is no fallback: a release whose notes could not be
# generated is a release that should not be cut, and a placeholder body would be
# published as though it were the real thing.
#
#   $1  version, without the leading v
#   $2  the commit being released
#   $3  the previous tag; may be empty for a first release
generate_release_notes() {
    local version="$1" base_sha="$2" previous_tag="$3"
    local repo payload notes

    if [[ -z "${version}" || -z "${base_sha}" ]]; then
        echo "generate_release_notes: version and base commit are both required" >&2
        return 1
    fi

    if ! repo="$(gh repo view --json nameWithOwner -q .nameWithOwner 2>/dev/null)" || [[ -z "${repo}" ]]; then
        echo "Cannot determine the repository from gh; refusing to generate notes." >&2
        return 1
    fi

    local args=(
        "repos/${repo}/releases/generate-notes"
        --method POST
        -f "tag_name=v${version}"
        -f "target_commitish=${base_sha}"
    )
    # Omitted rather than passed empty: GitHub picks the previous release itself
    # when the field is absent, and rejects the request when it is present and
    # blank.
    if [[ -n "${previous_tag}" ]]; then
        args+=(-f "previous_tag_name=${previous_tag}")
    fi

    # stdout and stderr kept apart. Folding them together with 2>&1 puts any
    # warning gh writes — a deprecation, a rate-limit notice, a new-version
    # nag — into the middle of the JSON, and the parse below then fails on a
    # perfectly good reply. The diagnosis has to stay readable too, so stderr is
    # captured rather than discarded and only printed when it matters.
    local errors status=0
    errors="$(mktemp)"
    payload="$(gh api "${args[@]}" 2>"${errors}")" || status=$?
    if (( status != 0 )); then
        echo "GitHub could not generate the release notes:" >&2
        cat "${errors}" >&2
        rm -f "${errors}"
        return 1
    fi
    rm -f "${errors}"

    # The body is extracted with jq rather than by pattern, and its absence is
    # told apart from its emptiness: `.body` missing means the answer was not the
    # shape we expected, which is a different problem from a release with nothing
    # in it. Both refuse, and they refuse differently.
    if ! notes="$(printf '%s' "${payload}" | jq -e -r '.body' 2>/dev/null)"; then
        echo "The reply from GitHub has no release body in it:" >&2
        printf '%s\n' "${payload}" | head -20 >&2
        return 1
    fi

    if [[ -z "${notes//[[:space:]]/}" ]]; then
        echo "GitHub generated empty release notes for v${version} against ${base_sha}." >&2
        echo "Refusing to cut a release with no notes." >&2
        return 1
    fi

    # The notes name a tag that does not exist yet; point those links at the
    # commit instead, so nothing in the pull request is a dead link.
    normalize_future_tag_links "${notes}" "${version}" "${base_sha}"
}

# The generated notes, with links to the tag being created pointed at the commit
# instead.
#
# GitHub ends its notes with a Full Changelog link built from the tag names it
# was given, so asking for `v0.9.0` produces
# `.../compare/v0.8.1...v0.9.0` — a link that 404s for the entire life of the
# pull request, which is exactly when somebody reads it. The header of the body
# already carries a correct compare against the SHA; this fixes the one inside
# the notes, which is the one that gets copied into the release.
#
# Only links to this release's own tag are touched. A note that legitimately
# mentions an older tag keeps it.
normalize_future_tag_links() {
    local notes="$1" version="$2" base_sha="$3" escaped
    # The version goes into a regex, and its dots are not wildcards here.
    escaped="$(printf '%s' "${version}" | sed 's/\./\\./g')"
    # `@` as the delimiter, because the alternation below needs `|` and the
    # replacement carries `/`.
    printf '%s\n' "${notes}" \
        | sed -E "s@(/compare/[^[:space:]]*)\.\.\.v${escaped}([[:space:]]|\$)@\1...${base_sha}\2@g" \
        | sed -E "s@(/commits/)v${escaped}([[:space:]]|\$)@\1${base_sha}\2@g"
}

# The body of the release pull request.
#
# It carries the notes, and it also carries the things somebody reviewing a
# release needs and cannot get from an empty commit: what is being released, from
# which commit exactly, and against which previous tag. The compare link points
# at the SHA rather than at v${VERSION}, because that tag does not exist yet —
# a link to it would 404 for the entire life of the pull request, which is
# precisely when it would be clicked.
#
#   $1 version  $2 previous tag  $3 base sha  $4 notes  $5 repo (owner/name)
render_pr_body() {
    local version="$1" previous_tag="$2" base_sha="$3" notes="$4" repo="$5"
    local compare

    if [[ -n "${previous_tag}" ]]; then
        compare="https://github.com/${repo}/compare/${previous_tag}...${base_sha}"
    else
        compare="https://github.com/${repo}/commits/${base_sha}"
    fi

    cat <<BODY
# Release ${version}

|  |  |
|---|---|
| Version | \`${version}\` |
| Previous release | ${previous_tag:-_none — this is the first_} |
| Releasing commit | [\`${base_sha}\`](https://github.com/${repo}/commit/${base_sha}) |
| Changes | [${previous_tag:-start}...${base_sha:0:7}](${compare}) |

## Why this commit is empty

Nothing in the tree records the version. \`include/utxoz/version.hpp\` is
generated into the build tree, so the version comes from the ref: this branch is
named \`release/${version}\`, \`ci/determine_version.sh\` derives the version from
that name, and CMake generates the header. Editing a file to state the version
would be a second copy that nothing reads.

The commit exists only so the branch has one — without it there is nothing
between master and \`release/${version}\` and there is no pull request to open.
It marks the release point, and \`post-release.sh\` confirms it landed on master
before tagging.

## Before merging

- [ ] CI is green on this branch
- [ ] The publish job uploaded the package **and** the post-upload check
      confirmed the exact recipe revision is retrievable from the remote
- [ ] The notes below read correctly

## After merging

Run \`./scripts/post-release.sh ${version}\`. It reuses the notes below exactly as
they are, for both \`doc/release-notes/release-notes.md\` and the GitHub release.
They are generated once, here, against \`${base_sha}\` — before this pull request
existed — which is why this pull request does not appear in them.

${RELEASE_NOTES_BEGIN}
${notes}
${RELEASE_NOTES_END}
BODY
}

# How many times a marker appears, counting occurrences and not lines.
#
# `grep -c` counts matching lines, so two markers on one line read as one and a
# body carrying `<!-- start --> notes <!-- start -->` would pass validation. `-o`
# prints each occurrence separately, and those are what get counted.
#
# grep exits 1 when it matches nothing, which under `set -e` in the sourcing
# script would abort here — and under `pipefail` the pipeline fails even though
# `wc` succeeded. Zero is a legitimate answer, so the failure is absorbed and the
# count still comes out as a number rather than as an empty string that every
# arithmetic test below would then read as zero by accident.
count_occurrences() {
    local marker="$1" body="$2" n
    n="$(printf '%s\n' "${body}" | grep -oF -- "${marker}" | wc -l || true)"
    # BSD wc pads its output with spaces; GNU does not.
    n="${n//[[:space:]]/}"
    printf '%s' "${n:-0}"
}

# The notes back out of a pull request body.
#
# Everything this rejects is a way of publishing the wrong text. A body with no
# markers is a pull request that was not created by this process, or was edited
# until the markers were lost; two pairs are ambiguous about which is the real
# one; a closing marker before an opening one cannot be read at all. None of them
# is recoverable by guessing, so each refuses and says which it was.
#
# Reads the body on stdin, prints the notes on stdout.
extract_release_notes() {
    local body begin_count end_count begin_line end_line
    body="$(cat)"

    if [[ -z "${body//[[:space:]]/}" ]]; then
        echo "The release pull request has an empty body; there are no notes to reuse." >&2
        return 1
    fi

    begin_count="$(count_occurrences "${RELEASE_NOTES_BEGIN}" "${body}")"
    end_count="$(count_occurrences "${RELEASE_NOTES_END}" "${body}")"

    if (( begin_count == 0 || end_count == 0 )); then
        echo "The release pull request body has no release-notes markers." >&2
        echo "Expected exactly one ${RELEASE_NOTES_BEGIN} and one ${RELEASE_NOTES_END}." >&2
        echo "Found ${begin_count} and ${end_count}." >&2
        return 1
    fi
    if (( begin_count > 1 || end_count > 1 )); then
        echo "The release pull request body has ${begin_count} opening and ${end_count} closing" >&2
        echo "release-notes markers. Exactly one of each is required; which pair is the" >&2
        echo "real one cannot be guessed." >&2
        return 1
    fi

    begin_line="$(printf '%s\n' "${body}" | grep -nF -- "${RELEASE_NOTES_BEGIN}" | head -1 | cut -d: -f1)"
    end_line="$(printf '%s\n' "${body}" | grep -nF -- "${RELEASE_NOTES_END}" | head -1 | cut -d: -f1)"
    if [[ -z "${begin_line}" || -z "${end_line}" ]]; then
        echo "The release-notes markers could not be located in the pull request body." >&2
        return 1
    fi

    if (( end_line <= begin_line )); then
        echo "The release-notes markers are in the wrong order in the pull request body." >&2
        return 1
    fi
    if (( end_line == begin_line + 1 )); then
        echo "The release-notes markers are there but there is nothing between them." >&2
        return 1
    fi

    local notes
    notes="$(sed -n "$((begin_line + 1)),$((end_line - 1))p" <<<"${body}")"

    if [[ -z "${notes//[[:space:]]/}" ]]; then
        echo "The release notes in the pull request body are blank." >&2
        return 1
    fi

    printf '%s\n' "${notes}"
}


# Tag, push the tag, and create the release — undoing the tag if the release
# cannot be created.
#
# The tag is pushed before the release because `gh release create` needs it to
# exist. That leaves a window: if the release then fails, the remote carries a
# `v${VERSION}` tag pointing at a release that was never published, and the next
# run of release.sh would delete it as leftover — or worse, somebody would treat
# it as a shipped version.
#
# So the tag is removed on failure, and only the tag: nothing else has been
# created yet, and the notes, the file and the merge all stay as they are. The
# compensation is allowed to fail without masking anything — the exit status
# reported is the one from `gh release create`, because that is the failure worth
# reading. If the cleanup itself fails, it says so and names the tag, so the
# manual step is one command rather than an investigation.
#
#   $1 version   $2 path to a file holding the notes
publish_release() {
    local version="$1" notes_file="$2" status=0

    git tag -a "v${version}" -m "Release version ${version}"
    git push origin "v${version}"

    gh release create "v${version}" \
        --title "v${version}" \
        --notes-file "${notes_file}" \
        --latest || status=$?

    if (( status != 0 )); then
        echo "" >&2
        echo "Creating the GitHub release for v${version} failed (exit ${status})." >&2
        echo "Removing the tag it would have pointed at, so nothing is left claiming" >&2
        echo "to be a released version." >&2

        if ! git push origin --delete "v${version}"; then
            echo "WARNING: could not delete the remote tag v${version}. Remove it with:" >&2
            echo "    git push origin --delete v${version}" >&2
        fi
        if ! git tag -d "v${version}"; then
            echo "WARNING: could not delete the local tag v${version}. Remove it with:" >&2
            echo "    git tag -d v${version}" >&2
        fi

        return "${status}"
    fi
}


# The notes already recorded for this version, or nothing.
#
# The entry runs from its heading to the next one, or to the end of the file.
# What comes back is the block with its blank framing stripped, so it can be
# compared against the notes as they were handed in.
#
# Returns 0 with the block on stdout when there is exactly one entry, 1 when
# there is none, and 2 when the file has more than one heading for this version —
# which is not something to resolve by picking one.
recorded_release_notes() {
    local version="$1" notes_file="$2" headings begin end block

    [[ -f "${notes_file}" ]] || return 1

    headings="$(grep -nxF "# version ${version}" "${notes_file}" || true)"
    [[ -n "${headings}" ]] || return 1

    if (( $(printf '%s\n' "${headings}" | wc -l) > 1 )); then
        return 2
    fi

    begin="$(printf '%s' "${headings}" | cut -d: -f1)"

    # The next heading of any version, which is where this entry stops.
    end="$(awk -v start="${begin}" 'NR > start && /^# version /{print NR; exit}' "${notes_file}")"
    if [[ -z "${end}" ]]; then
        end="$(( $(wc -l < "${notes_file}") + 1 ))"
    fi

    block="$(sed -n "$((begin + 1)),$((end - 1))p" "${notes_file}")"

    # Strip the blank lines this file's format puts around an entry, so the
    # comparison is about the notes and not about the framing.
    block="$(printf '%s\n' "${block}" | sed -e '/./,$!d' | sed -e :a -e '/^\n*$/{$d;N;};/\n$/ba')"
    printf '%s\n' "${block}"
}

# Whether this version already has an entry in the notes file.
#
# The heading is matched whole. A substring would make v0.9.0 match v0.9.01, and
# the retry described below would then skip an entry that was never written.
release_notes_recorded() {
    local version="$1" notes_file="$2"
    [[ -f "${notes_file}" ]] || return 1
    grep -qxF "# version ${version}" "${notes_file}"
}

# Prepend this version's entry to the notes file, once.
#
# `publish_release` can fail after the notes commit is already on master: the
# release notes are written and pushed, then the tag is pushed, then
# `gh release create` fails and the tag is withdrawn. Running post-release.sh
# again — which is the whole point of withdrawing the tag — would otherwise
# prepend a second copy of the same entry and make a second docs commit.
#
# So the write is conditional on the entry not being there, and the caller
# commits only when this actually changed something. A retry then lands on a
# file that already says what it should, does nothing, and goes on to create the
# release that failed the first time.
#
# Returns 0 when it wrote the entry, 10 when it was already there.
record_release_notes() {
    local version="$1" notes="$2" notes_file="$3" existing status=0 tmp

    if [[ ! -f "${notes_file}" ]]; then
        echo "${notes_file} does not exist." >&2
        return 1
    fi

    existing="$(recorded_release_notes "${version}" "${notes_file}")" || status=$?

    case ${status} in
        0)
            # Present. Identical is the retry case and does nothing; different is
            # a conflict, and the one thing that must not happen quietly — the
            # release would be published from the pull request while the file
            # kept saying something else, which is the divergence this whole
            # change exists to remove.
            if [[ "${existing}" == "$(printf '%s' "${notes}")" ]]; then
                return 10
            fi
            echo "${notes_file} already has an entry for version ${version}, and it" >&2
            echo "is not the text this release is publishing." >&2
            echo "" >&2
            echo "Refusing to continue: the release and the notes file would disagree." >&2
            echo "Reconcile ${notes_file} by hand and run this again." >&2
            return 1
            ;;
        2)
            echo "${notes_file} has more than one heading for version ${version}." >&2
            echo "Refusing to guess which one is the entry. Fix the file by hand." >&2
            return 1
            ;;
        1) ;;   # not there; fall through and write it
        *)
            echo "Could not read ${notes_file}." >&2
            return 1
            ;;
    esac

    tmp="$(mktemp)"
    {
        printf '# version %s\n\n%s\n\n' "${version}" "${notes}"
        cat "${notes_file}"
    } > "${tmp}" && mv "${tmp}" "${notes_file}"
}
