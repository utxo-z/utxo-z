#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# After the upload: prove the package is actually retrievable from the remote.
#
# A successful `conan upload` is not that proof. utxoz/0.8.1 was uploaded on
# 2026-07-31 — three publish jobs, each printing a complete upload summary with
# `(Uploaded)` — and it is not on the remote today; every recipe revision those
# jobs reported answers 404. The release was declared done on the strength of
# the log. See #92.
#
# Four things have to hold, and each fails with its own message:
#
#   1. the remote holds the reference — and "we could not ask" is its own
#      outcome, never folded into "it is not there";
#   2. the exact recipe revision just created is among the ones it holds, not
#      merely something with the same name;
#   3. it installs from the remote, pinned to that revision, with no permission
#      to build;
#   4. a consumer compiles and runs against what came back.
#
# All of it against an empty cache, so nothing can be answered by what
# `conan create` left behind locally. And the remote is read anonymously: what
# this verifies is what a consumer sees, not what an authenticated session can
# reach.
#
# --build=never is load-bearing. Without it a missing binary is papered over by
# rebuilding from the recipe, and this passes while a consumer with no compiler
# would fail.
#
# Reads:
#   UTXOZ_BUILD_VERSION    the version that was just uploaded
#   UTXOZ_RECIPE_REVISION  the revision create_package.sh reported; required
#   UTXOZ_REMOTE           remote name, default `kth-verify`
#   UTXOZ_REMOTE_URL       remote URL; added when set
#
# Run from the repository root:  ci/verify_published.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT
readonly VERSION="${UTXOZ_BUILD_VERSION:?UTXOZ_BUILD_VERSION is required}"
readonly REVISION="${UTXOZ_RECIPE_REVISION:?UTXOZ_RECIPE_REVISION is required}"
readonly REMOTE="${UTXOZ_REMOTE:-kth-verify}"
readonly REFERENCE="utxoz/${VERSION}"

cd "${ROOT}"

fail() { echo "FAIL: $*" >&2; exit 1; }

readonly PROFILE="${ROOT}/ci/conan-cpp23"
[[ -f "${PROFILE}" ]] || fail "the shared C++23 profile is missing: ${PROFILE}"

# The same options the package was created with. They are part of the package
# id, so asking with different ones asks for a binary nobody built — a failure
# about this script rather than about the release.
readonly PACKAGE_OPTIONS=(
    -pr:h "${PROFILE}"
    -s build_type=Release
    -o with_tests=False
    -o with_examples=False
    -o with_benchmarks=False
)

# A cache of its own, holding nothing. Verifying against the cache that just
# built the package would pass whether or not the upload arrived.
work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT
export CONAN_HOME="${work}/conan"
conan profile detect --force > /dev/null

if [[ -n "${UTXOZ_REMOTE_URL:-}" ]]; then
    conan remote add "${REMOTE}" "${UTXOZ_REMOTE_URL}" > /dev/null
fi

# ---------------------------------------------------------------------------
# 1 and 2. The remote holds it, and holds the revision just made.
# ---------------------------------------------------------------------------
echo "asking ${REMOTE} for ${REFERENCE}"
set +e
"${ROOT}/ci/remote_revisions.py" "${REMOTE}" "${REFERENCE}" > "${work}/revisions" 2> "${work}/why"
lookup=$?
set -e
cat "${work}/why" >&2 || true

case ${lookup} in
    0) ;;
    3) fail "the upload reported success and ${REFERENCE} is not on ${REMOTE}" ;;
    4) fail "could not establish whether ${REFERENCE} is on ${REMOTE}; that is not a pass" ;;
    *) fail "the revision lookup failed in an unexpected way (${lookup})" ;;
esac

echo "recipe revisions on ${REMOTE}:"
sed 's/^/  /' "${work}/revisions"

# grep answers three things, and they are not interchangeable: 0 the revision is
# listed, 1 it is not, anything else the list could not be read — which is not
# "the revision is missing" and must not be reported as one.
grep_status=0
grep -qxF -- "${REVISION}" "${work}/revisions" || grep_status=$?
case ${grep_status} in
    0) ;;
    1) fail "${REFERENCE} is on ${REMOTE}, but not the revision just built (${REVISION})" ;;
    *) fail "cannot read the revision list back (grep exited ${grep_status}); this is not a pass" ;;
esac
echo "ok: ${REVISION} is there"

# ---------------------------------------------------------------------------
# 3. It installs, pinned to that revision, with no permission to build.
# ---------------------------------------------------------------------------
echo
echo "installing ${REFERENCE}#${REVISION} from ${REMOTE} with --build=never"
readonly CONSUMER_BUILD="${work}/consumer"
mkdir -p "${CONSUMER_BUILD}"
# --lockfile-partial keeps the repository's pinned dependencies while admitting
# utxoz itself as a new root; dropping the lockfile lets them float and changes
# the package id, asking for a binary nobody built.
conan install --requires="${REFERENCE}#${REVISION}" -r "${REMOTE}" --build=never --lockfile-partial \
    "${PACKAGE_OPTIONS[@]}" -g CMakeDeps -g CMakeToolchain -of "${CONSUMER_BUILD}"

# ---------------------------------------------------------------------------
# 4. A consumer compiles and runs against what came back.
#
# The same consumer `conan create` uses, so this exercises both storage modes
# and both generated headers.
# ---------------------------------------------------------------------------
echo
echo "building a consumer against the downloaded package"
cmake -S "${ROOT}/test_package" -B "${CONSUMER_BUILD}/build" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE="${CONSUMER_BUILD}/conan_toolchain.cmake" > "${work}/cmake.log" 2>&1 \
    || { cat "${work}/cmake.log" >&2; fail "the consumer does not configure against the published package"; }
cmake --build "${CONSUMER_BUILD}/build" --parallel > "${work}/build.log" 2>&1 \
    || { cat "${work}/build.log" >&2; fail "the consumer does not compile against the published package"; }

# `find | head` reports head's status, so a find that failed — an unreadable
# directory, a bad predicate — would look exactly like "no executable was built".
# The results are collected first and the status is checked on its own.
candidates=()
while IFS= read -r -d '' candidate; do
    candidates+=("${candidate}")
done < <(find "${CONSUMER_BUILD}/build" -name 'test_package*' -type f -perm -u+x -print0; \
         printf '%d' $? > "${work}/find-status")
find_status="$(cat "${work}/find-status")"
(( find_status == 0 )) || fail "cannot search the consumer build tree (find exited ${find_status})"
(( ${#candidates[@]} > 0 )) || fail "the consumer built but produced no executable"
consumer="${candidates[0]}"

( cd "${work}" && "${consumer}" )

echo
echo "ok: ${REFERENCE}#${REVISION} is on ${REMOTE} and a consumer can install and run it"
