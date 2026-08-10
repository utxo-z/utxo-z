#!/usr/bin/env bash
set -x

# Error handling - rollback on failure
trap 'echo "Script failed. Consider running cleanup manually."; exit 1' ERR


if [ -z "$1" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 0.1.0"
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

# ---------------------------------------------------------------------------
# The release notes, taken now.
#
# Now, and not after the merge, because the notes are a photograph of what is
# being released and this is the last moment at which nothing about the release
# exists yet. Generated later they include the release pull request itself, and
# the commit that writes the notes file — a release citing its own paperwork.
#
# Everything below this point creates something: a branch, a commit, a pull
# request, eventually a tag. So this is also the last moment at which failing
# costs nothing. If GitHub cannot generate the notes, the release stops here with
# no state to unwind, and no placeholder body gets published as though it were
# real.
# ---------------------------------------------------------------------------
RELEASE_BASE_SHA="$(git rev-parse HEAD)"
echo "Releasing commit ${RELEASE_BASE_SHA}"

# Tags come from the remote, not from whatever this clone happens to have.
# `git pull` above updates the branch; it does not necessarily bring tags that
# were created elsewhere, and a stale clone would pick a previous release older
# than the real one — producing notes covering releases that already shipped.
#
# No --force: a tag that disagrees with the remote is a problem to look at, not
# one to overwrite silently on somebody's working clone.
echo "Syncing tags from origin..."
if ! git fetch --tags origin; then
    echo "Could not fetch tags from origin. The previous release cannot be"
    echo "determined reliably. Aborting before creating anything."
    exit 1
fi

# Three outcomes, and only two of them mean "carry on". A tag list that could
# not be read is not an empty tag list: treating it as one would generate notes
# over the whole history and present every version this project ever shipped as
# the contents of this release.
if ! PREVIOUS_TAG="$(previous_stable_tag "v${VERSION}")"; then
    echo "The previous release tag could not be determined. Nothing has been created."
    exit 1
fi

if [ -n "${PREVIOUS_TAG}" ]; then
    echo "Previous release: ${PREVIOUS_TAG}"
else
    echo "No previous release tag found; notes will cover the whole history."
fi

REPO_SLUG="$(gh repo view --json nameWithOwner -q .nameWithOwner)"
if [ -z "${REPO_SLUG}" ]; then
    echo "Cannot determine the repository. Aborting before creating anything."
    exit 1
fi

echo "Generating release notes for v${VERSION} against ${RELEASE_BASE_SHA}..."
if ! RELEASE_NOTES="$(generate_release_notes "${VERSION}" "${RELEASE_BASE_SHA}" "${PREVIOUS_TAG}")"; then
    echo ""
    echo "Release notes could not be generated. Nothing has been created:"
    echo "  no branch, no commit, no pull request, no tag."
    exit 1
fi

PR_BODY_FILE="$(mktemp)"
trap 'rm -f "${PR_BODY_FILE}"' EXIT
render_pr_body "${VERSION}" "${PREVIOUS_TAG}" "${RELEASE_BASE_SHA}" "${RELEASE_NOTES}" "${REPO_SLUG}" > "${PR_BODY_FILE}"
echo "Release notes captured (${#RELEASE_NOTES} characters)."

git checkout -b "release/${VERSION}"

# No version to bump. The version is not written down anywhere in the tree any
# more: #85 moved include/utxoz/version.hpp into the build tree, because it was
# generated into the source tree and tracked, so every build rewrote a file
# under version control with whatever version that build happened to use.
#
# It comes from the ref instead. This branch is named release/${VERSION}, and
# ci/determine_version.sh derives ${VERSION} from that name; CMake then requires
# it and generates the header. Editing a file to say the version would be
# recording a second copy that nothing reads.
#
# The commit is empty because the branch still needs one: without it there is
# nothing between master and release/${VERSION}, and `gh pr create` has no pull
# request to open. It marks the release point and is what post-release.sh later
# confirms landed on master before tagging.
git commit --allow-empty -m "release: ${VERSION}"

git push origin "release/${VERSION}"

# Create PR
#
# The body carries the release notes generated above, and post-release.sh reads
# them back out of it. That makes the pull request the carrier of the release
# notes rather than a formality, which is why a pull request this run did not
# create is not something to carry on with.
echo "Creating PR for release/${VERSION}..."
if gh pr create --title "release: ${VERSION}" --body-file "${PR_BODY_FILE}" --base master --head "release/${VERSION}"; then
    echo "PR created successfully for release/${VERSION}"
else
    echo ""
    echo "Could not create the pull request for release/${VERSION}."

    existing_prs="$(gh pr list --head "release/${VERSION}" --base master --json number,url 2>/dev/null || echo '[]')"
    pr_number="$(printf '%s' "${existing_prs}" | jq -r '.[0].number // empty')"
    pr_url="$(printf '%s' "${existing_prs}" | jq -r '.[0].url // empty')"

    if [ -n "${pr_number}" ]; then
        echo "There is already a pull request for this branch: #${pr_number}"
        echo "URL: ${pr_url}"
        echo ""
        # It was not created by this run, so its body does not carry the notes
        # generated for this commit. post-release.sh reads the notes out of the
        # pull request body: carrying on would either refuse there — after CI,
        # at the point where stopping is expensive — or publish an older set of
        # notes as though they described this release.
        echo "Its body does not carry the release notes generated for"
        echo "${RELEASE_BASE_SHA}. Either update it:"
        echo "    gh pr edit ${pr_number} --body-file <file>"
        echo "or close it and run this script again."
    fi

    echo ""
    echo "Aborting: a release must not be cut with notes nobody generated for it."
    exit 1
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

# `base64 --decode` is a GNU long option; BSD/macOS base64 uses -D. Pick the
# form this platform actually accepts instead of assuming GNU coreutils.
if printf '' | base64 --decode >/dev/null 2>&1; then
    BASE64_DECODE=(base64 --decode)
else
    BASE64_DECODE=(base64 -D)
fi

MAX_WAIT_TIME=7200  # 2 hours
ELAPSED=0
while [ $ELAPSED -lt $MAX_WAIT_TIME ]; do
    run_info=$(gh run list --branch "release/${VERSION}" --workflow ci.yml --limit 10 --json status,conclusion,url,number,event,createdAt)

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

    push_run_decoded=$(echo "$push_run" | "${BASE64_DECODE[@]}")

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

