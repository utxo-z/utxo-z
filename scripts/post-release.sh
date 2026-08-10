#!/usr/bin/env bash
set -e


if [ -z "$1" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 0.1.0"
    echo ""
    echo "This script performs the post-CI steps of the release process:"
    echo "  - Merges the release PR"
    echo "  - Creates release notes"
    echo "  - Creates and pushes the release tag"
    echo "  - Cleans up release branch"
    exit 1
fi

VERSION="$1"

RELEASE_LIB="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/release_lib.sh"
# shellcheck source=scripts/release_lib.sh
. "${RELEASE_LIB}"

# Validate semver format
if ! [[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Version must be in semver format (e.g., 0.1.0)"
    exit 1
fi

echo "Post-release processing for version ${VERSION}"

# Verify release branch exists
if ! git ls-remote --heads origin "release/${VERSION}" | grep -q "release/${VERSION}"; then
    echo "Release branch release/${VERSION} does not exist"
    exit 1
fi

# ---------------------------------------------------------------------------
# The notes: read back and validated before anything is merged.
#
# Before, and not after, because everything after the merge is irreversible by
# this script. A pull request whose body lost its markers — edited, or opened by
# hand instead of by release.sh — would otherwise be merged first and refused
# second, leaving master with a release commit on it, no tag, no release, and a
# release branch already half-consumed. Read first and the worst case is that
# nothing happened.
#
# They were generated once by release.sh, against the master commit being
# released, before the release branch or this pull request existed — which is
# why this pull request does not appear in them, and why the commit that writes
# the notes file below does not either.
# ---------------------------------------------------------------------------
echo "Reading the release notes from the release pull request..."
PR_BODY="$(gh pr view "release/${VERSION}" --json body -q .body)"

if ! RELEASE_NOTES="$(printf '%s' "${PR_BODY}" | extract_release_notes)"; then
    echo ""
    echo "The release notes cannot be read from the pull request for release/${VERSION}."
    echo "Nothing has been merged, tagged or published. Fix the pull request body — it"
    echo "must contain exactly one pair of release-notes markers with the notes between"
    echo "them — and run this script again."
    exit 1
fi
echo "Release notes recovered (${#RELEASE_NOTES} characters)."

# Squash merge the PR, do not delete the branch yet.
# Note: no --auto here. Auto-merge has to be enabled on the repository, and it
# returns immediately after queueing the merge — the tag below would then be
# created on a master that does not carry the version bump yet.
echo "Merging release PR..."
gh pr merge --squash "release/${VERSION}"

# Switch to master and pull latest changes
echo "Switching to master and pulling latest changes..."
git checkout master
git pull origin master

# The tag must point at a master that actually contains the release commit.
#
# This used to grep include/utxoz/version.hpp for the version, which cannot be
# true any more: #85 moved that header into the build tree, so nothing in the
# source tree carries the version. The check would have failed every release,
# and its message — "master does not carry version X yet" — would have been
# misleading rather than merely wrong.
#
# What it was really guarding is unchanged: do not tag a master that the release
# PR has not landed on. So ask about the merge itself. `gh pr merge` above
# returns before the merge is necessarily visible here, and `git pull` may have
# raced it; both are covered by requiring the merge commit to be an ancestor of
# what is checked out.
echo "Confirming the release PR landed on master..."
PR_STATE="$(gh pr view "release/${VERSION}" --json state -q .state)"
if [ "${PR_STATE}" != "MERGED" ]; then
    echo "The pull request for release/${VERSION} is ${PR_STATE}, not MERGED."
    echo "Aborting before tagging."
    exit 1
fi

MERGE_COMMIT="$(gh pr view "release/${VERSION}" --json mergeCommit -q .mergeCommit.oid)"
if [ -z "${MERGE_COMMIT}" ]; then
    echo "The pull request for release/${VERSION} reports no merge commit."
    echo "Aborting before tagging."
    exit 1
fi

if ! git merge-base --is-ancestor "${MERGE_COMMIT}" HEAD; then
    echo "Merge commit ${MERGE_COMMIT} is not an ancestor of the checked-out master."
    echo "The tag would not point at the release. Aborting before tagging."
    exit 1
fi
echo "Release commit ${MERGE_COMMIT} is on master."

# Step 1: Record the notes, if they are not recorded already.
#
# Conditional because this script is meant to be re-runnable. publish_release
# below can fail after this commit is on master — it withdraws its tag so the
# release can be attempted again — and a second run must not prepend the same
# entry twice or make a second docs commit.
echo "Updating local release notes file..."
NOTES_FILE="doc/release-notes/release-notes.md"

notes_status=0
record_release_notes "${VERSION}" "${RELEASE_NOTES}" "${NOTES_FILE}" || notes_status=$?

case ${notes_status} in
    0)
        echo "Recorded v${VERSION} in ${NOTES_FILE}"
        git add "${NOTES_FILE}"
        git commit -m "docs: update release notes for v${VERSION}"
        git push origin master
        ;;
    10)
        echo "${NOTES_FILE} already carries an entry for v${VERSION}; leaving it alone."
        ;;
    *)
        echo "Could not update ${NOTES_FILE}. Aborting before tagging."
        exit 1
        ;;
esac

echo "Creating final release v${VERSION}..."

RELEASE_NOTES_FILE="$(mktemp)"
trap 'rm -f "${RELEASE_NOTES_FILE}"' EXIT
printf '%s\n' "${RELEASE_NOTES}" > "${RELEASE_NOTES_FILE}"

# Tags, pushes and creates the release — and removes the tag again if the
# release cannot be created, so the remote never carries a v${VERSION} pointing
# at something that was never published.
publish_release "${VERSION}" "${RELEASE_NOTES_FILE}"

echo "Release v${VERSION} created successfully!"
echo "Release notes have been updated in $NOTES_FILE"

# Remove the release branch locally and remotely
echo "Cleaning up release branch..."
git push origin --delete "release/${VERSION}"
git branch -D "release/${VERSION}" 2>/dev/null || true

echo ""
echo "========================================"
echo "Release ${VERSION} completed successfully!"
echo "========================================"

