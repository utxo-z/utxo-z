#!/usr/bin/env bash
set -x

# Error handling - rollback on failure
trap 'echo "Script failed. Consider running cleanup manually."; exit 1' ERR

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
    echo "Usage: $0 <version>"
    echo "Example: $0 0.1.0"
    exit 1
fi
VERSION="$1"

# Validate semver format
if ! [[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Version must be in semver format (e.g., 0.1.0)"
    exit 1
fi

echo "Building version: ${VERSION}"

# Verify GitHub CLI authentication
echo "Verifying GitHub CLI authentication..."
if ! gh auth status >/dev/null 2>&1; then
    echo "Not authenticated with GitHub CLI. Run: gh auth login"
    exit 1
fi

# Cleanup previous release attempts (if any)
echo "Cleaning up previous release artifacts..."

# First, ensure we're on master branch
echo "Ensuring we're on master branch..."
git checkout master 2>/dev/null || true

# Delete local release branch if it exists
if git branch --list "release/${VERSION}" | grep -q "release/${VERSION}"; then
    echo "Deleting local branch release/${VERSION}"
    git branch -D "release/${VERSION}" 2>/dev/null || true
fi

# Delete remote release branch if it exists
if git ls-remote --heads origin "release/${VERSION}" | grep -q "release/${VERSION}"; then
    echo "Deleting remote branch release/${VERSION}"
    git push origin --delete "release/${VERSION}" 2>/dev/null || true
fi

# Delete local tag if it exists
if git tag --list "v${VERSION}" | grep -q "v${VERSION}"; then
    echo "Deleting local tag v${VERSION}"
    git tag -d "v${VERSION}" 2>/dev/null || true
fi

# Delete remote tag if it exists
if git ls-remote --tags origin "v${VERSION}" | grep -q "v${VERSION}"; then
    echo "Deleting remote tag v${VERSION}"
    git push origin --delete "v${VERSION}" 2>/dev/null || true
fi

# Delete GitHub release if it exists
if gh release view "v${VERSION}" >/dev/null 2>&1; then
    echo "Deleting GitHub release v${VERSION}"
    gh release delete "v${VERSION}" --yes 2>/dev/null || true
fi

echo "Cleanup completed"

# Check for staged changes before proceeding
if git status --porcelain | grep -q "^[MADRCU]"; then
    echo "You have staged changes. Please commit or unstash them first."
    exit 1
fi

# Smart stash handling - only stash if there are changes, and track if we did
STASH_CREATED=false
if ! git diff-index --quiet HEAD --; then
    echo "Stashing local changes..."
    git stash push -m "release script temporary stash for ${VERSION}"
    STASH_CREATED=true
else
    echo "No local changes to stash"
fi

git pull origin master

# Only pop if we actually created a stash
if [ "$STASH_CREATED" = true ]; then
    echo "Restoring stashed changes..."
    git stash pop
fi

# Verify the branch doesn't exist before creating
if git show-ref --verify --quiet "refs/heads/release/${VERSION}"; then
    echo "Branch release/${VERSION} already exists locally"
    exit 1
fi

git checkout -b "release/${VERSION}"

git push origin "release/${VERSION}"

# Create PR
echo "Creating PR for release/${VERSION}..."
if ! gh pr create --title "release: ${VERSION}" --body "release: ${VERSION}" --base master --head "release/${VERSION}" 2>/dev/null; then
    echo "PR creation failed. Checking if PR already exists..."

    existing_prs=$(gh pr list --head "release/${VERSION}" --base master --json number,title,url 2>/dev/null || echo "[]")

    if [ "$existing_prs" != "[]" ] && [ -n "$existing_prs" ]; then
        pr_number=$(echo "$existing_prs" | jq -r '.[0].number // empty')
        pr_url=$(echo "$existing_prs" | jq -r '.[0].url // empty')

        if [ -n "$pr_number" ]; then
            echo "Found existing PR #${pr_number}"
            echo "URL: ${pr_url}"
        else
            echo "Failed to create PR and no existing PR found"
            exit 1
        fi
    else
        echo "Failed to create PR and no existing PR found"
        exit 1
    fi
else
    echo "PR created successfully for release/${VERSION}"
fi

echo "Waiting for the build to finish for branch: release/${VERSION}"
echo "Recording current time to filter only new workflow runs..."
# Subtract 10 seconds to account for push delay
if [[ "$OSTYPE" == "darwin"* ]]; then
    RELEASE_START_TIME=$(date -u -v-10S +"%Y-%m-%dT%H:%M:%SZ")
else
    RELEASE_START_TIME=$(date -u -d '10 seconds ago' +"%Y-%m-%dT%H:%M:%SZ")
fi
echo "Filtering workflow runs created after: ${RELEASE_START_TIME}"
sleep 5

MAX_WAIT_TIME=7200  # 2 hours
ELAPSED=0
while [ $ELAPSED -lt $MAX_WAIT_TIME ]; do
    run_info=$(gh run list --branch "release/${VERSION}" --workflow "Build and Test" --limit 10 --json status,conclusion,url,number,event,createdAt)

    if [ -z "$run_info" ] || [ "$run_info" == "[]" ]; then
        echo "No workflow runs found for branch release/${VERSION}. Waiting..."
        sleep 30
        ELAPSED=$((ELAPSED + 30))
        continue
    fi

    push_run=$(echo "$run_info" | jq -r --arg start_time "$RELEASE_START_TIME" '
        .[] |
        select(.event == "push") |
        select(.createdAt >= $start_time) |
        select(.status == "in_progress" or .status == "queued" or (.status == "completed" and .conclusion == "success")) |
        . | @base64
    ' | head -1)

    if [ -z "$push_run" ]; then
        echo "No active push-triggered workflow runs found. Checking most recent push run..."
        push_run=$(echo "$run_info" | jq -r --arg start_time "$RELEASE_START_TIME" '
            .[] |
            select(.event == "push") |
            select(.createdAt >= $start_time) |
            . | @base64
        ' | head -1)
    fi

    if [ -z "$push_run" ]; then
        echo "No push-triggered workflow runs found for branch release/${VERSION}. Waiting..."
        sleep 30
        ELAPSED=$((ELAPSED + 30))
        continue
    fi

    push_run_decoded=$(echo "$push_run" | base64 --decode)

    status=$(echo "$push_run_decoded" | jq -r '.status')
    conclusion=$(echo "$push_run_decoded" | jq -r '.conclusion')
    url=$(echo "$push_run_decoded" | jq -r '.url')
    run_number=$(echo "$push_run_decoded" | jq -r '.number')
    event=$(echo "$push_run_decoded" | jq -r '.event')
    created_at=$(echo "$push_run_decoded" | jq -r '.createdAt')

    echo "Workflow run #${run_number} (${event}): status=${status}, conclusion=${conclusion}"
    echo "Created: ${created_at}"
    echo "URL: ${url}"

    if [ "$status" == "completed" ]; then
        if [ "$conclusion" == "success" ]; then
            echo "Build completed successfully!"
            break
        elif [ "$conclusion" == "cancelled" ]; then
            echo "Most recent workflow was cancelled. Waiting for a new one..."
            sleep 30
            ELAPSED=$((ELAPSED + 30))
            continue
        else
            echo "Build completed but failed with conclusion: ${conclusion}"
            echo "Please check the workflow at: ${url}"
            echo "Continue waiting? (y/n)"
            read -r response
            if [ "$response" != "y" ] && [ "$response" != "Y" ]; then
                exit 1
            fi
            sleep 30
            ELAPSED=$((ELAPSED + 30))
            continue
        fi
    else
        echo "Build is still in progress (${status}). Waiting..."
        sleep 30
        ELAPSED=$((ELAPSED + 30))
    fi
done

if [ $ELAPSED -ge $MAX_WAIT_TIME ]; then
    echo "Timeout waiting for CI after $MAX_WAIT_TIME seconds ($(($MAX_WAIT_TIME / 60)) minutes)"
    exit 1
fi

echo ""
echo "========================================"
echo "CI build completed successfully!"
echo "========================================"
echo ""
echo "Pre-release steps completed:"
echo "  - Release branch created: release/${VERSION}"
echo "  - PR created and ready for merge"
echo "  - CI builds passed"
echo ""
echo "Next step: Run the post-release script to complete the release:"
echo "   ./scripts/post-release.sh ${VERSION}"
echo ""
echo "The post-release script will:"
echo "  - Merge the release PR"
echo "  - Generate and commit release notes"
echo "  - Create the release tag and GitHub release"
echo "  - Clean up the release branch"
echo ""

# Clean up ssh-agent if we started it
if [ "$SSH_AGENT_STARTED" = true ]; then
    echo "Stopping SSH agent..."
    ssh-agent -k
fi
