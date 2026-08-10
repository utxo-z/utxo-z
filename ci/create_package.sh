#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Builds the package and proves a consumer can use it. Everything the real
# publish does before it touches the network, and nothing else.
#
# The publish job runs only on tag and release/hotfix pushes, so no pull request
# exercises it: its profile, its options and its consumer check reach production
# untested. That is what this script is for — the dry-run job and the publish
# job both call it, so the two cannot drift into separate implementations.
# Whatever a release does here, a pull request already did.
#
# It cannot upload. Not by a flag that could be passed wrongly, but because it
# never adds a remote to write to; the caller adds `kth` afterwards if it is
# publishing. A dry run with the real credentials in its environment still has
# nowhere to send anything.
#
# It uses whatever Conan cache the caller has. That is deliberate: the publish
# job uploads from that cache immediately afterwards, so creating into a private
# one would leave it with nothing to upload. A caller that wants isolation — the
# dry run does — sets CONAN_HOME itself.
#
# Reads:
#   UTXOZ_BUILD_VERSION  the version to create; already validated upstream
#
# Run from the repository root:  ci/create_package.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT
readonly VERSION="${UTXOZ_BUILD_VERSION:?UTXOZ_BUILD_VERSION is required}"

cd "${ROOT}"

fail() { echo "FAIL: $*" >&2; exit 1; }

# The same profile the rest of CI builds with, so what ships is what was tested.
readonly PROFILE="${ROOT}/ci/conan-cpp23"
[[ -f "${PROFILE}" ]] || fail "the shared C++23 profile is missing: ${PROFILE}"

# One definition, used by the create and by the resolve below. The options are
# part of the package id: asking for the package with different options asks for
# a binary nobody built, and the resolve would fail for a reason that has nothing
# to do with whether the package works.
#
# Scoped to `utxoz/*` rather than bare, and the same text as the one in
# verify_published.sh. Bare options are unambiguous only while the root is the
# recipe in this directory; the resolve below makes utxoz a requirement instead,
# and there Conan warns that it cannot tell which package in the graph
# `-o with_tests=False` is addressed to. Saying which one is meant costs nothing
# here and removes the warning where it was earned (#113).
readonly PACKAGE_OPTIONS=(
    -pr:h "${PROFILE}"
    -s build_type=Release
    -o "utxoz/*:with_tests=False"
    -o "utxoz/*:with_examples=False"
    -o "utxoz/*:with_benchmarks=False"
)

echo "creating utxoz/${VERSION}"

# `conan create` runs test_package, which is the consumer: it includes both
# generated headers, uses them, and opens a database in each storage mode.
conan create . --version="${VERSION}" --build=missing "${PACKAGE_OPTIONS[@]}"

# And again as an ordinary dependency rather than as a test package, with
# --build=never. The two are not the same check: test_package runs against a
# package this command just built, while this resolves the published metadata
# and refuses to rebuild. A package that only works when it can be rebuilt from
# the recipe is not a package.
echo
# --lockfile-partial, not --lockfile="": the repository ships a conan.lock
# pinning its dependencies, and utxoz itself is not in it. Dropping the lockfile
# entirely lets those dependencies float, which changes the package id and asks
# for a binary nobody built. Partial keeps the pins and admits the new root.
echo "resolving utxoz/${VERSION} as a dependency, without permission to build it"
conan install --requires="utxoz/${VERSION}" --build=never --lockfile-partial \
    "${PACKAGE_OPTIONS[@]}" -g CMakeDeps -g CMakeToolchain -of "$(mktemp -d)" > /dev/null

# The exact recipe revision this produced, so whoever publishes can verify that
# *this* revision reached the remote rather than something with the same name.
#
# Read from the cache rather than scraped from the create output, which prints
# several references and no promise about which is last. The query's status is
# checked and its JSON is parsed separately, because `conan list | python3`
# reports python's status: a failed query would arrive as an empty revision and
# be indistinguishable from a package that has none.
if ! listing="$(conan list "utxoz/${VERSION}#latest" --format=json 2>&1)"; then
    echo "${listing}" >&2
    fail "cannot read back the recipe revision of utxoz/${VERSION}"
fi

if ! revision="$(printf '%s' "${listing}" | python3 "${ROOT}/ci/one_revision.py" "utxoz/${VERSION}")"; then
    echo "${listing}" >&2
    fail "the package was created but its recipe revision cannot be established"
fi

echo
echo "recipe revision: ${revision}"
if [[ -n "${UTXOZ_REVISION_FILE:-}" ]]; then
    printf '%s\n' "${revision}" > "${UTXOZ_REVISION_FILE}"
fi
if [[ -n "${GITHUB_ENV:-}" ]]; then
    printf 'UTXOZ_RECIPE_REVISION=%s\n' "${revision}" >> "${GITHUB_ENV}"
fi

echo "ok: utxoz/${VERSION} builds and a consumer can use it"
