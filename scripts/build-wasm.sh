#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

VERSION="0.0.0-dev"
BUILD_DIR="${PROJECT_DIR}/build-wasm"

echo "Building utxoz for WebAssembly (Emscripten)"

cd "${PROJECT_DIR}"

# CI and local builds use the same compiler version and SDK requirement.
# Recipe remote setup is documented in doc/build-wasm.md. Conan supplies the
# SDK: an unrelated em++ on PATH must not select the toolchain.
PROFILE="${PROJECT_DIR}/ci/conan-ems6"
conan profile show -pr:h "${PROFILE}" -pr:b default

# Install dependencies
conan install . -of "${BUILD_DIR}" --version="${VERSION}" --build=missing -pr:h "${PROFILE}" -pr:b default \
    -c "tools.cmake.cmaketoolchain:user_presets="

GENERATORS="${BUILD_DIR}/build/Release/generators"
# shellcheck disable=SC1091
source "${GENERATORS}/conanbuild.sh"

# Configure
cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}/build/Release" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="${GENERATORS}/conan_toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build "${BUILD_DIR}/build/Release" --parallel

echo "WASM build successful!"
