#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

BUILD_TYPE="${1:-Release}"

echo "Rebuilding utxoz (${BUILD_TYPE})"

cd "${PROJECT_DIR}"

# Clean previous build
echo "Cleaning previous build..."
rm -rf build
rm -rf CMakeUserPresets.json

# Install Conan dependencies
echo "Installing Conan dependencies..."
conan install . -of build --build=missing -s build_type=${BUILD_TYPE}

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
