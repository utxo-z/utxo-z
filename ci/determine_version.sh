#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Derives the build version from a git ref, and refuses anything that is not a
# version.
#
# Part of this string comes from a branch name, and on a pull request the branch
# name is written by whoever opened it. `release/<anything>` puts that
# `<anything>` here verbatim. Downstream it is spliced into `conan create
# --version=…`, so an unchecked value is not an argument — it is shell source in
# five jobs at once.
#
# The check lives here, once, before the value is published as a step output.
# Every consumer then reads a string that has already been proven to be a
# version, and no consumer can forget to look. Repeating a guard per job is the
# arrangement where one of them is eventually missed.
#
# Reads (environment, never the command line):
#   GITHUB_REF_INPUT  the ref, or a bare branch name
#   RUN_NUMBER        the workflow run number
#   LAST_TAG          optional; computed from git when unset
#
# Writes the version to stdout. Exits non-zero, printing nothing to stdout, when
# the ref does not yield one.

set -eu

readonly REF_INPUT="${GITHUB_REF_INPUT:?GITHUB_REF_INPUT is required}"
readonly RUN="${RUN_NUMBER:?RUN_NUMBER is required}"

# The shapes this script is allowed to produce: X.Y.Z, optionally with a
# -prerelease or +build suffix — which is what the X.Y.Z-commit.<run>
# development versions are. Deliberately no wider: a space, a semicolon, a
# newline, `$(`, a backquote or a leading `v` is not a version, and a pattern
# loose enough to admit them stops being a check.
readonly VERSION_PATTERN='^[0-9]+\.[0-9]+\.[0-9]+([-+][0-9A-Za-z.-]+)?$'

if [[ -n "${LAST_TAG:-}" ]]; then
    last_tag="${LAST_TAG}"
else
    # One command whose status can be checked, rather than `git tag | head`.
    # A pipeline reports the exit status of its last stage, so `head` succeeding
    # would hide git failing — not a repository, an unreadable object store, a
    # checkout that never happened — and leave last_tag empty. That empty then
    # looks exactly like "this repository has no tags yet" and quietly becomes
    # 0.0.0, so a failure to read the tags would publish 0.0.0-commit.<run> as
    # though it were a considered answer.
    #
    # The two cases are not the same and are no longer treated the same:
    # a successful query that finds nothing falls back to 0.0.0; a failed query
    # stops here.
    if ! last_tag="$(git for-each-ref --count=1 --sort=-creatordate \
                         --format='%(refname:short)' refs/tags)"; then
        printf 'cannot read the repository tags; refusing to guess a version\n' >&2
        exit 1
    fi
fi
last_tag="${last_tag#v}"
if [[ -z "${last_tag}" ]]; then
    # A real answer: a repository with no tags yet starts at zero.
    last_tag="0.0.0"
fi

ref="${REF_INPUT}"
# A bare branch name is accepted; github.head_ref arrives without the prefix.
if [[ "${ref}" != refs/* ]]; then
    ref="refs/heads/${ref}"
fi

# The last path component, which for release/1.2.3 and hotfix/1.2.4 is the
# version.
version="${ref##*/}"

case "${ref}" in
    refs/heads/release/*|refs/heads/hotfix/*)
        : # the trailing component is the version
        ;;
    *)
        # Everything else — main, master, dev, feature branches, and tags —
        # takes a development version derived from the newest tag rather than
        # from the ref.
        #
        # Tags landing here is surprising, and it is deliberate that this change
        # does not alter it: a tag push publishes <last-tag>-commit.<run>, not
        # the tag's own version, and that is how every release so far was cut.
        # Changing it would change what a release publishes, which is not what
        # this script is for. It is worth its own issue.
        version="${last_tag}-commit.${RUN}"
        ;;
esac

if [[ ! "${version}" =~ ${VERSION_PATTERN} ]]; then
    printf 'refusing a version derived from the ref\n' >&2
    printf '  ref:     %q\n' "${REF_INPUT}" >&2
    printf '  derived: %q\n' "${version}" >&2
    printf 'expected %s\n' "${VERSION_PATTERN}" >&2
    exit 1
fi

printf '%s\n' "${version}"
