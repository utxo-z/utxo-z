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

if ! command -v em++ >/dev/null 2>&1 || ! em++ --version | head -n 1 | grep -q ' 6\.'; then
    echo "Error: Emscripten 6.x is required on PATH."
    exit 1
fi

# Check that ems2 profile exists
if ! conan profile show -pr ems2 > /dev/null 2>&1; then
    echo "Error: Conan profile 'ems2' not found."
    echo "Create it with:"
    echo ""
    echo "  cat > ~/.conan2/profiles/ems2 <<'EOF'"
    echo "  [settings]"
    echo "  arch=wasm"
    echo "  build_type=Release"
    echo "  compiler=clang"
    echo "  compiler.cppstd=23"
    echo "  compiler.libcxx=libc++"
    echo "  compiler.version=21"
    echo "  os=Emscripten"
    echo ""
    echo "  [buildenv]"
    echo "  CC_FOR_BUILD=clang"
    echo "  CXX_FOR_BUILD=clang++"
    echo "  EOF"
    exit 1
fi

# Install dependencies
conan install . -of "${BUILD_DIR}" --version="${VERSION}" --build=missing -pr ems2

# Configure
cmake --preset conan-release

# Build
cmake --build --preset conan-release --parallel

echo "WASM build successful!"
