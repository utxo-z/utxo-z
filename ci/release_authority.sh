#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Which run of this workflow is the authority for a revision: what it must
# validate, and whether it publishes.
#
# The release authority (#104):
#
#   - A push to release/<version> or hotfix/<version> in this repository is the
#     release. It runs the full validation and publishes the package, and its
#     publish job verifies that the exact recipe revision is retrievable.
#   - The release pull request for that branch does not run the full matrix a
#     second time. It is validated by reference: a gate job waits for the push
#     run of the same head commit and passes only if that run succeeded. This
#     holds only when the head is the same repository's branch — a fork can name
#     a branch release/<anything> — and only when the head already contains the
#     base, so that the tree a squash merge produces is the tree the push run
#     built. Anything else gets the full matrix.
#   - The tag is a record of what was released. It is not a trigger: nothing is
#     built or published from it.
#   - Everything else — master, ordinary pull requests, pull requests from forks,
#     workflow_dispatch — runs the full validation and publishes nothing.
#
# When a question here cannot be answered — a commit that is not in the clone,
# an ancestry that cannot be read — the answer is the full matrix. Validating
# twice costs minutes; validating nothing costs a release.
#
# Reads (environment, never the command line — on a pull request the branch
# name is written by whoever opened it):
#   EVENT_NAME    github.event_name                               (required)
#   GIT_REF       github.ref                                      (required)
#   REPOSITORY    github.repository                               (required)
#   PR_HEAD_REPO  github.event.pull_request.head.repo.full_name   (pull_request)
#   PR_HEAD_REF   github.head_ref                                 (pull_request)
#   PR_BASE_SHA   github.event.pull_request.base.sha              (pull_request)
#   PR_HEAD_SHA   github.event.pull_request.head.sha              (pull_request)
#
# Writes two lines to stdout, in the form GITHUB_OUTPUT takes:
#   validation=full|by-push
#   publish=true|false
# and the reason to stderr. Exits non-zero, printing nothing to stdout, when a
# required input is missing.
#
# Run from the repository root:  ci/release_authority.sh

set -euo pipefail

readonly EVENT="${EVENT_NAME:?EVENT_NAME is required}"
readonly REF="${GIT_REF:?GIT_REF is required}"
readonly REPO="${REPOSITORY:?REPOSITORY is required}"

# The two branch families that publish. Hotfixes publish exactly as releases do;
# they are the same thing cut from a different base.
is_release_branch() {
    [[ "$1" == release/* || "$1" == hotfix/* ]]
}

validation="full"
publish="false"
reason=""

case "${EVENT}" in
    push)
        # A push event is always this repository's own: nobody outside it can
        # push to its branches.
        case "${REF}" in
            refs/heads/release/*|refs/heads/hotfix/*)
                publish="true"
                reason="a push to ${REF#refs/heads/} is the release: full validation, then publish"
                ;;
            refs/tags/*)
                # Not a trigger any more; if one ever arrives, it publishes nothing.
                reason="a tag is a record of a release, not a release"
                ;;
            *)
                reason="a push to ${REF} is validated and publishes nothing"
                ;;
        esac
        ;;
    pull_request)
        head_ref="${PR_HEAD_REF:-}"
        head_repo="${PR_HEAD_REPO:-}"
        if ! is_release_branch "${head_ref}"; then
            reason="an ordinary pull request is validated in full"
        elif [[ "${head_repo}" != "${REPO}" ]]; then
            # A fork's branch called release/1.2.3 is just a branch. No push run
            # of this repository built it, so there is nothing to defer to.
            reason="${head_ref} comes from ${head_repo:-a repository that no longer exists}, not ${REPO}: validated in full"
        elif [[ -z "${PR_BASE_SHA:-}" || -z "${PR_HEAD_SHA:-}" ]]; then
            reason="the base or head commit is unknown: validated in full"
        elif git merge-base --is-ancestor "${PR_BASE_SHA}" "${PR_HEAD_SHA}" 2>/dev/null; then
            validation="by-push"
            reason="${head_ref} contains its base; the push run of ${PR_HEAD_SHA} is the validation"
        else
            # Not an ancestor, or the question could not be answered: either way
            # the merged tree is not proven to be the tree the push run built.
            reason="${head_ref} does not contain ${PR_BASE_SHA} (or that could not be read): validated in full"
        fi
        ;;
    *)
        reason="a ${EVENT} run is validated in full and publishes nothing"
        ;;
esac

printf 'release authority: %s\n' "${reason}" >&2
printf 'validation=%s\n' "${validation}"
printf 'publish=%s\n' "${publish}"
