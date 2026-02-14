#!/usr/bin/env bash
set -e

# Start ssh-agent if not already running to avoid multiple passphrase prompts
SSH_AGENT_STARTED=false
if [ -z "$SSH_AUTH_SOCK" ]; then
    echo "Starting SSH agent..."
    eval $(ssh-agent -s)
    SSH_AGENT_STARTED=true
fi

# Add SSH key (will ask for passphrase once)
echo "Adding SSH key..."
ssh-add ~/.ssh/id_rsa 2>/dev/null || ssh-add ~/.ssh/id_ed25519 2>/dev/null || ssh-add 2>/dev/null || true

if [ -z "$1" ]; then
    echo "Usage: $0 <version> [--force]"
    echo "Example: $0 0.1.0"
    exit 1
fi

VERSION="$1"
FORCE=false

if [ "$2" == "--force" ]; then
    FORCE=true
fi

# Validate semver format
if ! [[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Version must be in semver format (e.g., 0.1.0)"
    exit 1
fi

echo "Rollback release ${VERSION}..."

# Check if tag exists and how old it is
if git tag --list "v${VERSION}" | grep -q "v${VERSION}"; then
    TAG_DATE=$(git log -1 --format=%ai "v${VERSION}" 2>/dev/null || echo "")
    if [ -n "$TAG_DATE" ]; then
        TAG_TIMESTAMP=$(date -j -f "%Y-%m-%d %H:%M:%S %z" "$TAG_DATE" "+%s" 2>/dev/null || date -d "$TAG_DATE" "+%s" 2>/dev/null)
        CURRENT_TIMESTAMP=$(date "+%s")
        AGE_HOURS=$(( (CURRENT_TIMESTAMP - TAG_TIMESTAMP) / 3600 ))

        echo "Tag v${VERSION} exists and is ${AGE_HOURS} hours old"

        if [ $AGE_HOURS -gt 24 ] && [ "$FORCE" != true ]; then
            echo "Tag is older than 24 hours. This might be an established release."
            echo "Use --force to proceed anyway"
            exit 1
        fi
    fi
fi

# Ask for confirmation unless --force is used
if [ "$FORCE" != true ]; then
    echo "This will delete:"
    echo "  - Local branch: release/${VERSION}"
    echo "  - Remote branch: release/${VERSION}"
    echo "  - Local tag: v${VERSION}"
    echo "  - Remote tag: v${VERSION}"
    echo "  - GitHub release: v${VERSION}"
    echo "  - Temporary tag: temp-v${VERSION}"
    echo "  - Pull request for: release/${VERSION}"
    echo ""
    read -p "Are you sure you want to continue? (yes/no): " -r
    if [ "$REPLY" != "yes" ]; then
        echo "Aborted."
        exit 0
    fi
fi

# Switch to master first
echo "Switching to master branch..."
git checkout master 2>/dev/null || true

# 1. Cancel running workflows for the release branch
echo "Cancelling running workflows for release/${VERSION}..."
RUNNING_WORKFLOWS=$(gh run list --branch "release/${VERSION}" --status in_progress --json databaseId -q '.[].databaseId' 2>/dev/null || echo "")
if [ -n "$RUNNING_WORKFLOWS" ]; then
    for run_id in $RUNNING_WORKFLOWS; do
        echo "  Cancelling workflow run #${run_id}"
        gh run cancel "$run_id" 2>/dev/null || true
    done
else
    echo "  No running workflows found"
fi

# 2. Close and delete PR if it exists
echo "Closing PR for release/${VERSION}..."
PR_NUMBER=$(gh pr list --head "release/${VERSION}" --json number -q '.[0].number' 2>/dev/null || echo "")
if [ -n "$PR_NUMBER" ]; then
    echo "  Closing PR #${PR_NUMBER}"
    gh pr close "$PR_NUMBER" --delete-branch 2>/dev/null || true
else
    echo "  No PR found"
fi

# 3. Delete GitHub release if it exists
if gh release view "v${VERSION}" >/dev/null 2>&1; then
    echo "Deleting GitHub release v${VERSION}"
    gh release delete "v${VERSION}" --yes 2>/dev/null || true
else
    echo "  No GitHub release found for v${VERSION}"
fi

# 4. Delete temporary GitHub release if it exists
if gh release view "temp-v${VERSION}" >/dev/null 2>&1; then
    echo "Deleting temporary GitHub release temp-v${VERSION}"
    gh release delete "temp-v${VERSION}" --yes 2>/dev/null || true
else
    echo "  No temporary GitHub release found"
fi

# 5. Delete remote tag if it exists
if git ls-remote --tags origin "v${VERSION}" | grep -q "v${VERSION}"; then
    echo "Deleting remote tag v${VERSION}"
    git push origin --delete "v${VERSION}" 2>/dev/null || true
else
    echo "  No remote tag found for v${VERSION}"
fi

# 6. Delete remote temporary tag if it exists
if git ls-remote --tags origin "temp-v${VERSION}" | grep -q "temp-v${VERSION}"; then
    echo "Deleting remote temporary tag temp-v${VERSION}"
    git push origin --delete "temp-v${VERSION}" 2>/dev/null || true
else
    echo "  No remote temporary tag found"
fi

# 7. Delete local tag if it exists
if git tag --list "v${VERSION}" | grep -q "v${VERSION}"; then
    echo "Deleting local tag v${VERSION}"
    git tag -d "v${VERSION}" 2>/dev/null || true
else
    echo "  No local tag found for v${VERSION}"
fi

# 8. Delete local temporary tag if it exists
if git tag --list "temp-v${VERSION}" | grep -q "temp-v${VERSION}"; then
    echo "Deleting local temporary tag temp-v${VERSION}"
    git tag -d "temp-v${VERSION}" 2>/dev/null || true
else
    echo "  No local temporary tag found"
fi

# 9. Delete remote release branch if it exists
if git ls-remote --heads origin "release/${VERSION}" | grep -q "release/${VERSION}"; then
    echo "Deleting remote branch release/${VERSION}"
    git push origin --delete "release/${VERSION}" 2>/dev/null || true
else
    echo "  No remote branch found for release/${VERSION}"
fi

# 10. Delete local release branch if it exists
if git branch --list "release/${VERSION}" | grep -q "release/${VERSION}"; then
    echo "Deleting local branch release/${VERSION}"
    git branch -D "release/${VERSION}" 2>/dev/null || true
else
    echo "  No local branch found for release/${VERSION}"
fi

# 11. Check for release notes commit and offer to revert it
echo "Checking for release notes commit..."
RELEASE_NOTES_COMMIT=$(git log --all --grep="docs: update release notes for v${VERSION}" --format="%H" -n 1 2>/dev/null || echo "")
if [ -n "$RELEASE_NOTES_COMMIT" ]; then
    echo "Found release notes commit: ${RELEASE_NOTES_COMMIT}"
    if [ "$FORCE" == true ]; then
        REVERT_ANSWER="yes"
    else
        read -p "Do you want to revert this commit? (yes/no): " -r REVERT_ANSWER
    fi

    if [ "$REVERT_ANSWER" == "yes" ]; then
        echo "  Reverting commit ${RELEASE_NOTES_COMMIT}"
        git revert --no-edit "$RELEASE_NOTES_COMMIT" 2>/dev/null || echo "  Could not revert automatically"
    fi
else
    echo "  No release notes commit found"
fi

echo "Rollback completed for version ${VERSION}"
echo ""
echo "Summary:"
echo "  - Cancelled running workflows"
echo "  - Closed and deleted PR"
echo "  - Deleted GitHub releases (main and temp)"
echo "  - Deleted tags (local and remote)"
echo "  - Deleted branches (local and remote)"
echo ""
echo "You can now fix the issues and run the release script again."

# Clean up ssh-agent if we started it
if [ "$SSH_AGENT_STARTED" = true ]; then
    echo "Stopping SSH agent..."
    ssh-agent -k
fi
