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

# Squash merge the PR, do not delete the branch yet
echo "Merging release PR..."
gh pr merge --squash --auto "release/${VERSION}"

# Switch to master and pull latest changes
echo "Switching to master and pulling latest changes..."
git checkout master
git pull origin master

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

