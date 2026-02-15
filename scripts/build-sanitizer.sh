#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

SANITIZER="${1:-address,undefined}"
VERSION="0.0.0-dev"
BUILD_DIR="${PROJECT_DIR}/build-sanitizer"

echo "Building utxoz with sanitizer: ${SANITIZER} (Debug)"

cd "${PROJECT_DIR}"

# Install dependencies
conan install . -of "${BUILD_DIR}" --version="${VERSION}" --build=missing \
    -s build_type=Debug -s compiler.cppstd=23 \
    -o sanitizer="${SANITIZER}"

# Configure
cmake --preset conan-debug

# Build
cmake --build --preset conan-debug --parallel

# Run tests
echo "Running tests under sanitizer..."
ctest --preset conan-debug --output-on-failure --parallel 4
