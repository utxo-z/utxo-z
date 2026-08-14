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
# where it matters, and then proves the check is worth running by showing that a
# CRLF copy of the same files really does export a different revision.
#
# Nothing here may pass because it could not look. Every external command has
# its status inspected, and every three-way outcome — matched, did not match,
# could not read — is kept three-way. A verifier that reports success when it
# failed to run is worse than no verifier, which is the whole reason this file
# exists.
#
# Run from anywhere:  ci/check_recipe_reproducible.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT
cd "${ROOT}"

fail() { echo "FAIL: $*" >&2; exit 1; }

# What the recipe is made of: conanfile.py, which is the recipe, plus the paths
# it lists in exports_sources. Anything outside this — scripts/, ci/, .github/ —
# can have whatever line endings it likes without changing the revision, so it is
# deliberately not checked and deliberately not part of the fixture below.
readonly RECIPE_INPUTS=(conanfile.py CMakeLists.txt src include examples tests benchmarks LICENSE README.md)
readonly FIXTURE_VERSION="0.0.0-reproducible"

# A carriage return *at the end of a line*, which is what CRLF is. Matching a
# bare \r would also flag a lone carriage return inside a string literal or a
# file using old Mac line endings — neither is the thing this guards against,
# and a check that fires on the wrong thing is a check nobody trusts.
CRLF_PATTERN="$(printf '\r')$"
readonly CRLF_PATTERN

# grep's three outcomes, kept three. 0 matched, 1 did not, anything else is a
# failure to read — which is not "no match", and not "binary either".
has_crlf() {
    local file="$1" status=0
    grep -qIUE "${CRLF_PATTERN}" -- "${file}" || status=$?
    (( status <= 1 )) || fail "cannot read ${file} to check its line endings (grep exited ${status})"
    return ${status}
}

# Whether grep considers the file text. The empty pattern matches any line of a
# text file and never matches a binary one under -I, so status 1 means binary —
# but only when grep could actually read it.
is_text() {
    local file="$1" status=0
    grep -qI '' -- "${file}" || status=$?
    (( status <= 1 )) || fail "cannot read ${file} to tell whether it is text (grep exited ${status})"
    return ${status}
}

# ---------------------------------------------------------------------------
# 1. No CRLF in anything the recipe is made of.
#
# This is the check that fails on a Windows checkout without .gitattributes,
# which is the configuration that produced the second recipe revision.
# ---------------------------------------------------------------------------
recipe_files="$(git ls-files -- "${RECIPE_INPUTS[@]}")" \
    || fail "cannot list the recipe's files; refusing to report on line endings"
[[ -n "${recipe_files}" ]] || fail "git listed no recipe files, which cannot be right"

offenders=""
while IFS= read -r file; do
    [[ -f "${file}" ]] || continue
    # A file git exports verbatim cannot change the revision by its line
    # endings, because it has none: `binary` means the bytes go out as they are
    # on every platform. Asking anyway finds carriage returns inside binary data
    # and reports them as a portability fault.
    if git check-attr binary -- "${file}" | grep -q ': binary: set$'; then
        continue
    fi
    if has_crlf "${file}"; then
        offenders+="  ${file}"$'\n'
    fi
done <<< "${recipe_files}"

if [[ -n "${offenders}" ]]; then
    echo "these files have CRLF line endings, so this platform would export a" >&2
    echo "different recipe than the others:" >&2
    printf '%s' "${offenders}" >&2
    fail "the working tree is not normalised to LF"
fi
echo "ok: no CRLF in the recipe's sources"

# ---------------------------------------------------------------------------
# 2. And the check above is not vacuous: those line endings really do decide
#    the revision.
#
# The fixture converts exactly the files the recipe is made of and nothing else.
# Converting the whole tree would also change scripts/ and ci/, which are not
# part of the recipe, and the demonstration would then be "changing many files
# changes the revision" — true, and not the claim being made.
# ---------------------------------------------------------------------------
work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT
export CONAN_HOME="${work}/conan"
conan profile detect --force > /dev/null 2>&1 \
    || fail "conan profile detect failed; the exports below would be meaningless"

# The reference this fixture exports under, matched as a literal string rather
# than a regex, so the dots in the version cannot act as wildcards.
readonly REVISION_MARKER="utxoz/${FIXTURE_VERSION}#"

# Parses the whole export output rather than piping it through grep. A pipeline
# reports its last stage, so `conan export | grep` turned a failed export into
# an empty revision; and taking the first match would hide an output carrying
# several different ones. Three outcomes, three answers: exactly one revision,
# none, or something ambiguous.
revision_of() {
    local tree="$1" output status=0

    output="$(cd "${tree}" && conan export . --version="${FIXTURE_VERSION}" 2>&1)" || status=$?
    if (( status != 0 )); then
        echo "${output}" >&2
        fail "conan export failed in ${tree} (exited ${status})"
    fi

    local -a found=()
    local line rest hex
    while IFS= read -r line; do
        [[ "${line}" == *"${REVISION_MARKER}"* ]] || continue
        rest="${line#*"${REVISION_MARKER}"}"
        hex="${rest%%[!0-9a-f]*}"
        (( ${#hex} == 32 )) || continue
        # Conan names the reference on more than one line; only distinct values
        # are ambiguous.
        local seen="" candidate
        for candidate in ${found[@]+"${found[@]}"}; do
            [[ "${candidate}" == "${hex}" ]] && seen=yes && break
        done
        [[ -n "${seen}" ]] || found+=("${hex}")
    done <<< "${output}"

    case ${#found[@]} in
        1) printf '%s' "${found[0]}" ;;
        0) echo "${output}" >&2
           fail "conan export produced no recipe revision for ${tree}" ;;
        *) echo "${output}" >&2
           fail "conan export named ${#found[@]} different revisions for ${tree}: ${found[*]}" ;;
    esac
}

git archive --format=tar HEAD > "${work}/tree.tar" \
    || fail "git archive failed; there is no tree to compare"
mkdir -p "${work}/lf"
tar -x -f "${work}/tree.tar" -C "${work}/lf" || fail "cannot unpack the exported tree"
cp -r "${work}/lf" "${work}/crlf"

# Only the recipe's own files, listed by git rather than matched by extension,
# so the fixture and the property cannot drift apart.
while IFS= read -r file; do
    target="${work}/crlf/${file}"
    [[ -f "${target}" ]] || continue
    has_crlf "${target}" && continue
    # Binary files are left alone: giving them CRLF would be corrupting data
    # rather than changing line endings.
    is_text "${target}" || continue
    sed -i.bak 's/$/\r/' "${target}" || fail "cannot rewrite ${target} to CRLF"
    rm -f "${target}.bak"
done <<< "${recipe_files}"

lf_revision="$(revision_of "${work}/lf")"
crlf_revision="$(revision_of "${work}/crlf")"
readonly lf_revision crlf_revision

if [[ "${lf_revision}" == "${crlf_revision}" ]]; then
    echo "  LF:   ${lf_revision}" >&2
    echo "  CRLF: ${crlf_revision}" >&2
    fail "line endings no longer change the recipe revision — check 1 is now vacuous"
fi
echo "ok: line endings do change the recipe revision, so check 1 is worth running"
echo "      LF:   utxoz/${FIXTURE_VERSION}#${lf_revision}"
echo "      CRLF: utxoz/${FIXTURE_VERSION}#${crlf_revision}"

echo "all checks passed"
