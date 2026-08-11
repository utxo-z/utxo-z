# version 0.10.0

## What's Changed
* fix: carry the database path to the filesystem in its native form by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/115
* fix: report the release gate apart from the checks below it by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/114
* feat!: the caller owns its lookups, and hands them to resolve() by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/118
* fix: serialise resolutions, so resolve() can be called from two threads by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/123
* feat!: the caller owns its deletions, and gets back what was applied by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/124


**Full Changelog**: https://github.com/utxo-z/utxo-z/compare/v0.9.1...ebe9abe083a4053e439da2f19df58d71ba3bca84

# version 0.9.1

## What's Changed
* fix: resolve public dependencies while verifying a published package by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/108
* fix: generate the release notes once, before the release exists by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/107
* fix: tell a proven absence from a lookup that could not be made by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/110


**Full Changelog**: https://github.com/utxo-z/utxo-z/compare/v0.9.0...d45e55c8d4fb5458dc5900d305c40141a40def4b

# version 0.9.0

## What's Changed
* docs: write down the threading invariants the library already relies on by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/55
* feat!: shard search statistics into atomic counters, making find() concurrent by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/56
* fix!: compaction reports a duplicate key instead of silently dropping one by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/58
* feat!: split the search counters into probes and historical resolution by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/60
* fix!: catalogue version files by identity, fail closed when it cannot be read by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/61
* test: restore directory permissions from a guard in the catalogue test by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/67
* fix!: publish metadata atomically and refuse a record that cannot be trusted by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/68
* feat!: make compaction crash-atomic by building a new file and swapping it in by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/70
* fix: refuse an unusable version file at once instead of waiting five minutes by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/76
* feat!: claim a database exclusively while an instance holds it by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/77
* test: run the crash matrix against compact mode as well by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/78
* feat: add sync(), which puts what has been written on stable storage by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/80
* refactor: write the merge protocol once instead of once per storage mode by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/81
* refactor!: name the storage mode reference, not compact by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/82
* fix: generate config.hpp and version.hpp into the build tree by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/85
* ci: pin and verify build toolchains by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/86
* fix: refuse a build version that is not a version by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/90
* test: keep the reference statistics test honest with counters compiled out by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/91
* fix: run every step in bash, and check the version the build actually used by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/97
* fix: pin line endings, so the recipe hashes the same on every platform by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/95
* fix: publish the version a tag names by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/96
* feat: exercise the publish path from pull requests, and check what it published by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/98
* fix: stop the release scripts editing a file that no longer exists by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/101
* fix: link bcrypt so consumers of the Windows package can link too by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/103
* fix: wait for the remote to publish before deciding it did not by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/105
* release: 0.9.0 by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/106


**Full Changelog**: https://github.com/utxo-z/utxo-z/compare/v0.8.1...v0.9.0


# version 0.8.1

## What's Changed
* docs: document the deferred lookup/deletion contract, add rotation tests by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/53
* release: 0.8.1 by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/54


**Full Changelog**: https://github.com/utxo-z/utxo-z/compare/v0.8.0...v0.8.1


# version 0.8.0

## What's Changed
* chore(deps): bump third-party libs to latest CCI by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/51
* release: 0.8.0 by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/52


**Full Changelog**: https://github.com/utxo-z/utxo-z/compare/v0.7.0...v0.8.0


# version 0.7.0

## What's Changed
* ci: add Conan package publishing on release branches by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/49
* release: 0.7.0 by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/50


**Full Changelog**: https://github.com/utxo-z/utxo-z/compare/v0.6.0...v0.7.0


# version 0.6.0

## What's Changed
* fix: suppress nodiscard warnings and fix release script by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/45
* refactor: replace default ctor + configure() with named constructors by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/46
* release: 0.6.0 by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/48


**Full Changelog**: https://github.com/utxo-z/utxo-z/compare/v0.5.0...v0.6.0


# version 0.5.0

## What's Changed
* ci: use statistical comparison for benchmark alerts by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/36
* fix: count all versions in entries_count_ on reopen and reset impl_ on close by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/37
* ci: update actions/checkout v4 to v5 (Node.js 24) by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/40
* feat: add compact storage mode with type-safe split API by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/35
* refactor: replace exceptions with std::expected in public API by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/42
* release: 0.5.0 by @fpelliccioni in https://github.com/utxo-z/utxo-z/pull/44


**Full Changelog**: https://github.com/utxo-z/utxo-z/compare/v0.4.0...v0.5.0


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


