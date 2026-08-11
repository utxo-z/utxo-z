# UTXO-Z

> **Alpha Software**: This is a preliminary alpha version of UTXO-Z. Its implementation is subject to change as it is being experimented with for the first time in Bitcoin Cash nodes. The API is also subject to change.

High-performance UTXO database for Bitcoin Cash.

UTXO-Z is a purpose-built storage engine designed for the specific access patterns of UTXO set management. It achieves **2.5x higher throughput** than LevelDB during blockchain synchronization and up to **45x faster lookups** for recent outputs.

For the technical paper describing the architecture and benchmarks, see [docs/utxoz.pdf](docs/utxoz.pdf).

## Features

- **Two storage modes**: Full mode (variable-size values) and Reference mode (fixed-size file references)
- **Type-safe API**: `full_db` and `reference_db` with compile-time mode safety — no runtime dispatch
- **Multi-container architecture**: 5 size-optimized containers (48B, 94B, 128B, 256B, 10KB) in full mode
- **Memory-mapped files**: Automatic file rotation and OS-managed I/O
- **Deferred lookups and deletions**: Reads and deletes that miss the mapped version are batched — see [Deferred lookups and deletions](#deferred-lookups-and-deletions)
- **Generational storage**: Recent outputs are faster to access
- **Cache-optimized**: Open addressing hash tables for CPU cache efficiency

## Requirements

- C++23 compiler (GCC 13+, Clang 17+)
- CMake 3.20+
- Conan 2.0+

## Installation

### Using Conan (recommended)

Add to your `conanfile.py`:

```python
def requirements(self):
    self.requires("utxoz/0.3.0")
```

### Building from source

```bash
# Clone
git clone https://github.com/utxo-z/utxo-z.git
cd utxo-z

# Build and install to local Conan cache
./scripts/build-create.sh 0.3.0
```

## Usage

### Full mode (default)

Stores complete UTXO output data (scriptPubKey + amount). Use `utxoz::db` (alias for `utxoz::full_db`).

```cpp
#include <utxoz/utxoz.hpp>

int main() {
    // Open database — returns result<full_db>
    auto r = utxoz::db::open("./utxo_data", true);  // path, remove_existing
    if (!r) {
        // r.error().message contains the error description
        return 1;
    }
    auto& db = *r;

    // Create a key (32-byte tx hash + 4-byte output index)
    std::array<uint8_t, 32> tx_hash = { /* ... */ };
    auto key = utxoz::make_outpoint(tx_hash, 0);  // output index 0

    // Insert UTXO
    std::vector<uint8_t> value = { /* serialized output */ };
    auto inserted = db.insert(key, value, block_height);
    if (!inserted) { /* error: inserted.error() */ }
    if (!*inserted) { /* duplicate key */ }

    // Find UTXO — returns full_find_result {data, block_height}
    auto result = db.find(key, current_height);
    if (result) {
        auto& data = result->data;            // std::vector<uint8_t>
        auto height = result->block_height;   // uint32_t
    }

    // Erase UTXO (may be deferred)
    db.erase(key, current_height);

    // Process deferred deletions periodically
    auto [deleted, failed] = db.process_pending_deletions();

    // Compact periodically for optimal performance
    db.compact_all();

    // close() is optional — destructor handles it
}
```

### Reference mode

Stores only a fixed-size reference (file_number + offset) per UTXO. Use `utxoz::reference_db`.

```cpp
#include <utxoz/utxoz.hpp>

int main() {
    auto r = utxoz::reference_db::open("./utxo_data", true);
    if (!r) return 1;
    auto& db = *r;

    auto key = utxoz::make_outpoint(tx_hash, 0);

    // Insert with typed fields — no byte serialization needed
    db.insert(key, file_number, offset, block_height);

    // Find — returns reference_find_result {block_height, file_number, offset}
    auto result = db.find(key, current_height);
    if (result) {
        auto height = result->block_height;   // uint32_t
        auto fnum   = result->file_number;    // uint32_t
        auto off    = result->offset;         // uint32_t
    }

    // erase, process_pending_deletions, compact_all, etc. work the same
}
```

### Deferred lookups and deletions

Containers are **generational**: each one keeps writing to its latest version file and rotates to a new one when that file fills up. Only the latest version is memory-mapped.

`find()` and `erase()` work on that mapped version (`erase()` also checks the cached files). Anything left behind in a previous version is not answered there, so their immediate result is **not authoritative**:

- `find()` returning `not_resolved` means "not in the active versions, and nothing else was consulted". It is not absence, and nothing was recorded: **you keep the request**.
- `erase()` returning `0` means "not in the mapped version — queued as a pending deletion". Deletions still use an internal queue.

The definitive answer for a lookup comes from `resolve()`, which walks the cached files and every previous version for exactly the batch you hand it:

```cpp
// Phase 1: issue the reads for the block/batch, keeping what did not resolve.
std::vector<utxoz::lookup_request> pending;
for (auto const& outpoint : block_inputs) {
    if (auto r = db.find(outpoint, height)) {
        use(*r);                 // resolved inline
        continue;
    }
    // not_resolved — do NOT conclude the UTXO is missing yet. The database did
    // not remember this key; the line below is what remembers it.
    pending.emplace_back(outpoint, height);
}

// Phase 2: resolve your batch. `found` answers it; `absent` is proven absence.
auto resolved = db.resolve(pending);
if ( ! resolved) {
    // Could not read something it needed. `pending` is untouched — retry it
    // later and treat nothing as missing in the meantime.
    return;
}
auto& [found, absent] = *resolved;

// Same shape for deletions
for (auto const& outpoint : block_inputs) {
    db.erase(outpoint, height);  // 0 means "queued", not "absent"
}
auto [deleted, unresolved_deletions] = db.process_pending_deletions();
```

Rules to follow:

- **`absent` means absent.** A request reaches it only when every version below the current one was read and the key was in none of them. If something could not be read, `resolve()` returns `version_unreadable` or `catalog_unreadable` and **no lists at all** — never a partial answer. So `absent` can be turned into "invalid block"; an error cannot.
- **The batch is yours.** `resolve()` borrows the span and keeps nothing, so two components can each hold their own batch and neither can receive or consume the other's requests. Retry after an error with the same vector: nothing was consumed, so there is nothing to rebuild.
- **Duplicate keys collapse.** A batch naming one outpoint twice asks one question and gets one answer, in exactly one of the two lists.
- **Call `resolve()` before `process_pending_deletions()`.** Deferred deletions remove entries from the very files a resolution still needs to read, so the reverse order loses the values of UTXOs spent in that same batch.
- **`process_pending_deletions()` still drains a global queue.** It reports everything once — including keys queued by a different caller — so exactly one component may own *that* call. Lookups no longer work this way.
- **Do not decide synchronously inside a block.** A validator that needs the value on the spot has to be restructured into the two phases above.
- `deferred_deletions_size()` lets you assert nothing is left pending at the end of a batch. Lookups have no equivalent, because the pending set is the vector in your hand.

#### Threading

A database instance supports **one mutating operation at a time**, with no other operation of any kind in flight. Mutating means `insert()`, `erase()`, `process_pending_deletions()`, `resolve()`, `compact_all()` and `close()`. Serialising them is the caller's job.

`resolve()` is `const` and still on that list. Const means it does not change what is *stored*; it does move the LRU file cache. Caller-owned batches make two resolutions independent, not concurrent.

`find()` is the one exception, and only a partial one: **concurrent `find()` calls are permitted strictly while no insert, erase, rotation, resolution, compaction or cache operation can run.** Providing that reader/writer barrier is the caller's responsibility — this library has no lock to lean on.

What makes `find()` eligible at all is that it reads the active maps and writes nothing but sharded atomic counters. It holds no queue and registers no key — a miss is reported and forgotten. That removes the internal writer; it does **not** make the active map safe against modification. Nothing here does.

**Statistics are operations too, not free reads.** `get_statistics()` is not const — it recomputes the fragmentation counters as it goes — and `reset_search_stats()` / `reset_all_statistics()` write by definition; the const accessors read plain counters that `insert()` and `erase()` write. All of them may overlap with `find()`, which writes nothing they look at beyond its own sharded counters, but not with any mutation, and `get_statistics()` and the reset calls not with each other either. A summary taken while `find()` is recording is also not consistent across fields — numerators can sit an increment ahead of their denominators, so take it while nothing is recording if you need exact cross-field numbers.

The restriction on everything else is structural rather than incidental:

- The LRU file cache owns the memory mappings and has no synchronisation. Evicting an entry unmaps the segment, so a second thread reading a map it obtained earlier is a use-after-unmap: a crash, not a torn read. `erase()`, `process_pending_deletions()` and `resolve()` all touch that cache.
- The deferred-**deletion** queue is still **global, not per caller**. `process_pending_deletions()` drains all of it, so two callers steal each other's keys. Exactly one component may own that call. Lookups used to work this way too; they no longer do.
- The deferred-deletion queue, the entry count, the per-container statistics and the file metadata are plain members mutated without atomics.
- A rotation — triggered from inside `insert()` — unmaps the whole active segment and briefly leaves the container pointer null. A concurrent `find()` would be reading unmapped memory, which is why "no mutation in flight" is a condition of the exception above and not a nicety.

#### When the first rotation happens

`find()` reports `not_resolved` for **every** miss of the active versions — whether the key lives in an older version or does not exist at all. So your batch starts filling at the first lookup of an unknown key, with no rotation involved, and `resolve()` is what turns those into found-or-absent regardless of how many versions exist.

What the first rotation changes is the *meaning* of a `not_resolved`. While a container still has a single version, everything stored is in the mapped version, so an unresolved lookup can only ever resolve to "does not exist" — which is exactly why treating it as authoritative appears to work. Once a container rotates, it also covers "still stored, in a previous version", and code making that assumption starts failing abruptly, for many keys at once.

Container 0 (48-byte entries in a 2 GiB file, ~7.86M buckets) rotates when the hash table reaches `max_load_factor * 0.95`. Measured on production sizing:

| Workload | Live entries at first rotation of container 0 |
|----------|-----------------------------------------------|
| Inserts only | 6,550,001 |
| Inserts + spends | 6,700,001 |

For Bitcoin Cash mainnet that is around block 245,000. The other containers rotate much earlier, since their files are smaller.

### Iterating over entries

```cpp
// Iterate all keys (same for both modes)
db.for_each_key([](utxoz::raw_outpoint const& key) {
    // ...
});

// utxoz::full_db: iterate entries with byte data
// (assuming db is an open full_db instance)
db.for_each_entry([](utxoz::raw_outpoint const& key,
                       uint32_t block_height,
                       std::span<uint8_t const> data) {
    // ...
});

// utxoz::reference_db: iterate entries with typed fields
// (assuming rdb is an open reference_db instance)
rdb.for_each_entry([](utxoz::raw_outpoint const& key,
                          uint32_t height,
                          uint32_t file_number,
                          uint32_t offset) {
    // ...
});
```

### Logging

UTXO-Z supports three logging backends configured at build time:

```bash
./scripts/build-create.sh 0.3.0 custom   # Callback-based (default)
./scripts/build-create.sh 0.3.0 spdlog   # spdlog integration
./scripts/build-create.sh 0.3.0 none     # Disabled
```

#### Custom callback

```cpp
#include <utxoz/logging.hpp>

#ifdef UTXOZ_LOG_CUSTOM
utxoz::set_log_callback([](utxoz::log_level level, std::string_view msg) {
    std::cout << "[utxoz] [" << utxoz::log_level_name(level) << "] " << msg << "\n";
});
#endif
```

#### spdlog prefix

```cpp
#ifdef UTXOZ_LOG_SPDLOG
utxoz::set_log_prefix("utxoz");  // Messages will show as "[utxoz] ..."
#endif
```

## API Reference

### Class hierarchy

```text
db_base                    — shared methods (close, size, erase, statistics, ...)
  ├── full_db  (= db)      — variable-size byte values
  └── reference_db          — typed file_number + offset fields
```

### `db_base` (shared by both modes)

| Method | Description |
|--------|-------------|
| `close()` | Close and flush all data. Idempotent; also called by destructor |
| `size()` | Total UTXO count |
| `erase(key, height)` | Erase UTXO. `0` means deferred, not absent — see [Deferred lookups and deletions](#deferred-lookups-and-deletions) |
| `process_pending_deletions()` | Definitive answer for deferred deletes, returns (count, failed entries) |
| `deferred_deletions_size()` | Count of pending deferred deletions |
| `for_each_key(callback)` | Iterate over all stored keys |
| `compact_all()` | Optimize storage |
| `get_statistics()` | Get performance stats |
| `print_statistics()` | Log formatted stats |
| `get_sizing_report()` | Get container sizing analysis |
| `print_height_range_stats()` | Log per-height-range insert/delete statistics |
| `reset_all_statistics()` | Reset all counters |

### `full_db` (aliased as `db`)

| Method | Description |
|--------|-------------|
| `open(path, remove_existing)` | Static: open database, returns `result<full_db>` |
| `open_for_testing(path, remove_existing)` | Static: open with smaller file sizes |
| `insert(key, value, height)` | Insert UTXO with byte data |
| `find(key, height)` | Returns `result<full_find_result>` (`data`, `block_height`). `not_resolved` is not absence — see [Deferred lookups and deletions](#deferred-lookups-and-deletions) |
| `resolve(span<lookup_request const>)` | Definitive answer for your batch: `full_resolution` (`found`, `absent`) |
| `for_each_entry(callback)` | Callback: `(key, height, span<uint8_t const>)` |

### `reference_db`

| Method | Description |
|--------|-------------|
| `open(path, remove_existing)` | Static: open database, returns `result<reference_db>` |
| `open_for_testing(path, remove_existing)` | Static: open with smaller file sizes |
| `insert(key, file_number, offset, height)` | Insert UTXO with typed fields |
| `find(key, height)` | Returns `result<reference_find_result>` (`block_height`, `file_number`, `offset`). `not_resolved` is not absence — see [Deferred lookups and deletions](#deferred-lookups-and-deletions) |
| `resolve(span<lookup_request const>)` | Definitive answer for your batch: `reference_resolution` (`found`, `absent`) |
| `for_each_entry(callback)` | Callback: `(key, height, file_number, offset)` |

### `utxoz::raw_outpoint`

36-byte key: 32-byte transaction hash + 4-byte output index (little-endian).

```cpp
auto key = utxoz::make_outpoint(tx_hash, output_index);
auto txid = utxoz::get_txid(key);
auto index = utxoz::get_output_index(key);
```

## Performance

Benchmarks on consumer hardware (single thread):

| Metric | UTXO-Z | LevelDB | Improvement |
|--------|--------|---------|-------------|
| IBD inputs/sec | 290K | 117K | 2.5x |
| IBD outputs/sec | 320K | 129K | 2.5x |
| Recent lookups/sec | 33M | 740K | 45x |
| Total lookups/sec | 1.5M | 740K | 2x |

### Running benchmarks locally

For accurate performance measurements, run the benchmarks on your own hardware:

```bash
cd build/build/Release/benchmarks
./utxoz_benchmarks           # Quick operation benchmarks (insert, find, erase)
./utxoz_benchmarks_large     # Large-scale IBD simulation (50M entries)
```

### CI benchmark tracking

We track benchmark results on every commit in CI:

[Benchmark Dashboard](https://utxo-z.github.io/utxo-z/dev/bench/)

These results are **not** representative of real-world performance. GitHub Actions runners have limited and shared resources, which introduces high variability: you will see spikes in the charts where GHA assigned more or fewer resources to the run. The purpose of CI benchmarks is to detect performance regressions between commits, not to measure absolute throughput. For real UTXO-Z performance numbers, run the benchmarks locally on your machine.

## License

MIT License. See [LICENSE](LICENSE).
