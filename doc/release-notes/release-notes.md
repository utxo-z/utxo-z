# version 0.4.0

## What's Changed
* fix: Rebuild metadata files after compaction by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/32
* feat: 5-container layout, height range stats, key/entry iteration by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/30
* docs: update README for 0.3.0 by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/33
* release: 0.4.0 by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/34


**Full Changelog**: https://github.com/utxo-z/utxo-z/compare/v0.3.0...v0.4.0


# version 0.3.0

## What's Changed
* feat: Add large-scale benchmarks with production file sizes by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/23
* test: Add edge case and regression tests for storage correctness by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/24
* fix: Implement metadata persistence (save/load to disk) by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/25
* feat: Add sizing report for container/file size optimization by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/26
* fix: Route values by data capacity, not container size by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/27
* fix: Adjust sizing report test expectations after routing fix by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/28
* fix: Update entries_count_ and container stats on deferred deletions by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/29
* release: 0.3.0 by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/31


**Full Changelog**: https://github.com/utxo-z/utxo-z/compare/v0.2.0...v0.3.0


# version 0.2.0

## What's Changed
* docs: Add alpha disclaimer to README by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/16
* fix: benchmark gh-pages switch failure by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/18
* ci: Add sanitizer support (ASAN, UBSAN, TSAN) by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/17
* fix: Disable boost cobalt module and add sanitizer build script by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/19
* ci: Add WebAssembly (Emscripten) build by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/20
* release: 0.2.0 by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/22


**Full Changelog**: https://github.com/utxo-z/utxo-z/compare/v0.1.0...v0.2.0


# version 0.1.0

## What's Changed
* docs: Update README with library usage documentation by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/1
* fix: Include block height in failed deletion reports by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/2
* feat: Add deferred lookups and improve API by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/3
* refactor: Rename key_t to raw_outpoint and value_span_t to output_data_span by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/4
* feat: Add build-time option to disable statistics collection by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/5
* refactor: Extract size literals to separate file by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/6
* fix: Use spdlog as header-only library by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/7
* perf: Replace byte-by-byte hash with constant-time hash_outpoint by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/8
* chore: Dynamic versioning via --version parameter by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/9
* feat: Add nanobench benchmark infrastructure with CI tracking by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/10
* test: Add storage correctness tests by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/11
* bench: Add storage overhead report and close/reopen benchmarks by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/12
* release: 0.1.0 by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/15


**Full Changelog**: https://github.com/utxo-z/utxo-z/commits/v0.1.0


