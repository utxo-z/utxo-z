#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# The recipe must hash the same on every platform.
#
# Conan derives the recipe revision from the bytes of the exported sources, so
# the revision is the package's identity. A checkout that converts text to CRLF
# exports different bytes, and the same commit then publishes a different
# recipe: `release/0.8.1` published 9fbdb476… from Linux and macOS and
# cd1d5b96… from Windows, and a consumer resolving that version gets whichever
# upload the server considers latest.
#
# .gitattributes pins the working tree to LF. This checks that it took effect
# where it matters — on the files that are exported — and then proves the check
# is worth running by showing that a CRLF copy of the same tree really does
# export a different revision.
#
# Run from anywhere:  ci/check_recipe_reproducible.sh

set -eu

readonly ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT}"

fail() { echo "FAIL: $*" >&2; exit 1; }

# The paths conanfile.py lists in exports_sources. Anything outside them can
# have whatever line endings it likes without changing the recipe.
readonly EXPORTED=(CMakeLists.txt conanfile.py src include examples tests benchmarks LICENSE README.md)

# ---------------------------------------------------------------------------
# 1. No CRLF in anything that gets exported.
#
# This is the check that fails on a Windows checkout without .gitattributes,
# which is the configuration that produced the second recipe revision.
# ---------------------------------------------------------------------------
crlf="$(git ls-files -z -- "${EXPORTED[@]}" | xargs -0 grep -lIU "$(printf '\r')" 2>/dev/null || true)"
if [[ -n "${crlf}" ]]; then
    echo "these exported files have CRLF line endings, so this platform would" >&2
    echo "export a different recipe than the others:" >&2
    printf '  %s\n' ${crlf} >&2
    fail "the working tree is not normalised to LF"
fi
echo "ok: no CRLF in the exported sources"

# ---------------------------------------------------------------------------
# 2. And the check above is not vacuous: line endings really do decide the
#    revision.
#
# Exporting a CRLF copy of this same tree must produce a different revision, and
# normalising it back must restore the original. If those do not hold, the check
# above is guarding a property that no longer exists and should be revisited
# rather than trusted.
# ---------------------------------------------------------------------------
work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT
export CONAN_HOME="${work}/conan"
conan profile detect --force > /dev/null 2>&1

revision_of() {
    (cd "$1" && conan export . --version=0.0.0-reproducible 2>&1) \
        | grep -oE 'utxoz/0\.0\.0-reproducible#[a-f0-9]+' | head -n 1
}

git archive --format=tar HEAD | (mkdir -p "${work}/lf" && tar -x -C "${work}/lf")
cp -r "${work}/lf" "${work}/crlf"
find "${work}/crlf" -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.txt' \
     -o -name '*.md' -o -name '*.in' -o -name '*.py' \) -exec sed -i.bak 's/$/\r/' {} +
find "${work}/crlf" -name '*.bak' -delete

readonly LF_REVISION="$(revision_of "${work}/lf")"
readonly CRLF_REVISION="$(revision_of "${work}/crlf")"

[[ -n "${LF_REVISION}"   ]] || fail "could not export the LF tree"
[[ -n "${CRLF_REVISION}" ]] || fail "could not export the CRLF tree"

if [[ "${LF_REVISION}" == "${CRLF_REVISION}" ]]; then
    echo "  LF:   ${LF_REVISION}" >&2
    echo "  CRLF: ${CRLF_REVISION}" >&2
    fail "line endings no longer change the recipe revision — check 1 is now vacuous"
fi
echo "ok: line endings do change the recipe revision, so check 1 is worth running"
echo "      LF:   ${LF_REVISION}"
echo "      CRLF: ${CRLF_REVISION}"

echo "all checks passed"
