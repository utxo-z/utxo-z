#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set -e
set -x

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

VERSION="${1:-0.0.1}"
LOG_BACKEND="${2:-custom}"
STATISTICS="${3:-True}"

echo "Building utxoz version: ${VERSION} (log=${LOG_BACKEND}, statistics=${STATISTICS})"

cd "${PROJECT_DIR}"

rm -rf build
rm -rf conan.lock

# Remove existing package from cache to force rebuild
conan remove "utxoz/${VERSION}" -c 2>/dev/null || true

conan lock create conanfile.py --version="${VERSION}" -o log=${LOG_BACKEND} -o statistics=${STATISTICS} -o with_tests=True -o with_examples=False --update

conan lock create conanfile.py --version="${VERSION}" -o log=${LOG_BACKEND} -o statistics=${STATISTICS} -o with_tests=True -o with_examples=False --lockfile=conan.lock --lockfile-out=build/conan.lock
conan create conanfile.py --version "${VERSION}" --lockfile=build/conan.lock --build=missing -o log=${LOG_BACKEND} -o statistics=${STATISTICS} -o with_tests=True -o with_examples=False

echo "Package utxoz/${VERSION} created successfully in local cache"
