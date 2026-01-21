#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

BUILD_TYPE="${1:-Release}"
LOG_BACKEND="${2:-custom}"

echo "Building utxoz (${BUILD_TYPE}, log=${LOG_BACKEND})"

cd "${PROJECT_DIR}"

# Configure if needed
if [ ! -f "build/build/${BUILD_TYPE}/CMakeCache.txt" ]; then
    echo "Configuring CMake..."

    # Install dependencies if needed
    if [ ! -d "build" ]; then
        echo "Installing Conan dependencies..."
        conan install . -of build --build=missing -s build_type=${BUILD_TYPE} -o log=${LOG_BACKEND} -o with_examples=True
    fi

    cmake --preset conan-release

    if [ $? -ne 0 ]; then
        echo "CMake configuration failed"
        exit 1
    fi
fi

# Build
cmake --build --preset conan-release --parallel

if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi

echo "Build successful!"

# Run tests
echo "Running tests..."
ctest --preset conan-release --output-on-failure --parallel 4
