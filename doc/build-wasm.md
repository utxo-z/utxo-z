# Building for WebAssembly

CI and `scripts/build-wasm.sh` share `ci/conan-ems6`: Emscripten **6.0.13**,
Conan's `emcc` compiler setting, C++23 and a native build profile. Use Conan
2.31.0 or newer (CI pins its own exact Conan version).

The SDK recipe comes from `conan-io/conan-toolchains`, not the older ConanCenter
emsdk recipe. Configure its local recipe remote once:

```bash
git clone https://github.com/conan-io/conan-toolchains.git conan-toolchains
git -C conan-toolchains checkout 11b83330cdeb20622fe4b097dccb254969639a54
conan remote add conan-toolchains "$PWD/conan-toolchains"
conan profile detect  # Only if a default native profile does not already exist
conan profile show -pr:h ./ci/conan-ems6 -pr:b default
```

Run these commands from the UTXO-Z repository root. If the remote is already
configured, use its existing checkout at the revision above; do not register it
twice. CI checks out that same recipe-source revision.

```bash
./scripts/build-wasm.sh
```

Conan installs the SDK as a build requirement; no preinstalled `em++` or named
`ems2` profile is needed. The script activates Conan's generated build environment
and uses the toolchain generated in `build-wasm`, independent of other local
CMake presets.

This builds the library for wasm32. It does not certify database operations in a
WebAssembly runtime: runtime smoke coverage and the production reference-size
limitation remain tracked separately in #64 and #135.
