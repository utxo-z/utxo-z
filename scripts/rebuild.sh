#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <version> [build_type] [log_backend] [statistics]"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

VERSION="$1"
BUILD_TYPE="${2:-Release}"
LOG_BACKEND="${3:-custom}"
STATISTICS="${4:-True}"

echo "Rebuilding utxoz version: ${VERSION} (${BUILD_TYPE}, log=${LOG_BACKEND}, statistics=${STATISTICS})"

cd "${PROJECT_DIR}"

# Clean previous build
echo "Cleaning previous build..."
rm -rf build
rm -rf CMakeUserPresets.json

# Install Conan dependencies
echo "Installing Conan dependencies..."
conan install . -of build --version="${VERSION}" --build=missing -s build_type=${BUILD_TYPE} -o log=${LOG_BACKEND} -o statistics=${STATISTICS} -o with_examples=True

# Configure CMake
echo "Configuring CMake..."
cmake --preset conan-release

if [ $? -ne 0 ]; then
    echo "CMake configuration failed"
    exit 1
fi

# Build
echo "Building..."
cmake --build --preset conan-release --parallel

if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi

echo "Build successful!"

# Run tests
echo "Running tests..."
ctest --preset conan-release --output-on-failure --parallel 4
