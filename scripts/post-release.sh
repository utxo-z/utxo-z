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

# Step 1: Create temporary release to generate notes
echo "Creating temporary release to generate notes..."
TEMP_TAG="temp-v${VERSION}"
git tag -a "${TEMP_TAG}" -m "Temporary tag for release notes generation"
git push origin "${TEMP_TAG}"

gh release create "${TEMP_TAG}" \
    --title "temp-v${VERSION}" \
    --generate-notes \
    --prerelease

# Step 2: Extract the generated notes
echo "Extracting generated release notes..."
RELEASE_NOTES=$(gh release view "${TEMP_TAG}" --json body -q '.body')

if [ -z "$RELEASE_NOTES" ]; then
    echo "Failed to extract release notes"
    exit 1
fi

# Replace temporary tag references with final tag in the release notes
echo "Fixing references in release notes..."
RELEASE_NOTES=$(echo "$RELEASE_NOTES" | sed "s/temp-v${VERSION}/v${VERSION}/g")

# Step 3: Update local release notes file
echo "Updating local release notes file..."
NOTES_FILE="doc/release-notes/release-notes.md"

# Create a backup
cp "$NOTES_FILE" "${NOTES_FILE}.backup"

# Prepare the new release notes entry
NEW_ENTRY="# version ${VERSION}

${RELEASE_NOTES}

"

# Add the new entry at the top of the file
{
    echo "$NEW_ENTRY"
    cat "$NOTES_FILE"
} > "${NOTES_FILE}.tmp" && mv "${NOTES_FILE}.tmp" "$NOTES_FILE"

echo "Updated release notes file"

# Step 4: Commit the updated release notes
git add "$NOTES_FILE"
git commit -m "docs: update release notes for v${VERSION}"
git push origin master

# Step 5: Clean up temporary release and tag
echo "Cleaning up temporary release..."
gh release delete "${TEMP_TAG}" --yes
git tag -d "${TEMP_TAG}"
git push origin --delete "${TEMP_TAG}"

# Step 6: Create the real release with auto-generated notes
echo "Creating final release v${VERSION}..."
git tag -a "v${VERSION}" -m "Release version ${VERSION}"
git push origin "v${VERSION}"

gh release create "v${VERSION}" \
    --title "v${VERSION}" \
    --generate-notes \
    --latest

echo "Release v${VERSION} created successfully!"
echo "Release notes have been updated in $NOTES_FILE"

# Remove the release branch locally and remotely
echo "Cleaning up release branch..."
git push origin --delete "release/${VERSION}"
git branch -D "release/${VERSION}" 2>/dev/null || true

# Remove backup
rm -f "${NOTES_FILE}.backup"

echo ""
echo "========================================"
echo "Release ${VERSION} completed successfully!"
echo "========================================"

