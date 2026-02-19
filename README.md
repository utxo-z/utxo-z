# UTXO-Z

> **Alpha Software**: This is a preliminary alpha version of UTXO-Z. Its implementation is subject to change as it is being experimented with for the first time in Bitcoin Cash nodes. The API is also subject to change.

High-performance UTXO database for Bitcoin Cash.

UTXO-Z is a purpose-built storage engine designed for the specific access patterns of UTXO set management. It achieves **2.5x higher throughput** than LevelDB during blockchain synchronization and up to **45x faster lookups** for recent outputs.

For the technical paper describing the architecture and benchmarks, see [docs/utxoz.pdf](docs/utxoz.pdf).

## Features

- **Two storage modes**: Full mode (variable-size values) and Compact mode (fixed-size file references)
- **Type-safe API**: `full_db` and `compact_db` with compile-time mode safety — no runtime dispatch
- **Multi-container architecture**: 5 size-optimized containers (48B, 94B, 128B, 256B, 10KB) in full mode
- **Memory-mapped files**: Automatic file rotation and OS-managed I/O
- **Deferred deletions**: Batched deletes for optimal write performance
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
    utxoz::db db;  // or utxoz::full_db
    db.configure("./utxo_data", true);  // path, remove_existing

    // Create a key (32-byte tx hash + 4-byte output index)
    std::array<uint8_t, 32> tx_hash = { /* ... */ };
    auto key = utxoz::make_outpoint(tx_hash, 0);  // output index 0

    // Insert UTXO
    std::vector<uint8_t> value = { /* serialized output */ };
    db.insert(key, value, block_height);

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

    db.close();
}
```

### Compact mode

Stores only a fixed-size reference (file_number + offset) per UTXO. Use `utxoz::compact_db`.

```cpp
#include <utxoz/utxoz.hpp>

int main() {
    utxoz::compact_db db;
    db.configure("./utxo_data", true);

    auto key = utxoz::make_outpoint(tx_hash, 0);

    // Insert with typed fields — no byte serialization needed
    db.insert(key, file_number, offset, block_height);

    // Find — returns compact_find_result {block_height, file_number, offset}
    auto result = db.find(key, current_height);
    if (result) {
        auto height = result->block_height;   // uint32_t
        auto fnum   = result->file_number;    // uint32_t
        auto off    = result->offset;         // uint32_t
    }

    // erase, process_pending_deletions, compact_all, etc. work the same
    db.close();
}
```

### Iterating over entries

```cpp
// Iterate all keys (same for both modes)
db.for_each_key([](utxoz::raw_outpoint const& key) {
    // ...
});

// full_db: iterate entries with byte data
full_db.for_each_entry([](utxoz::raw_outpoint const& key,
                          uint32_t block_height,
                          std::span<uint8_t const> data) {
    // ...
});

// compact_db: iterate entries with typed fields
compact_db.for_each_entry([](utxoz::raw_outpoint const& key,
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

```
db_base                    — shared methods (close, size, erase, statistics, ...)
  ├── full_db  (= db)      — variable-size byte values
  └── compact_db            — typed file_number + offset fields
```

### `db_base` (shared by both modes)

| Method | Description |
|--------|-------------|
| `close()` | Close and flush all data |
| `size()` | Total UTXO count |
| `erase(key, height)` | Erase UTXO (may be deferred) |
| `process_pending_deletions()` | Process deferred deletes, returns (count, failed entries) |
| `deferred_deletions_size()` | Count of pending deferred deletions |
| `deferred_lookups_size()` | Count of pending deferred lookups |
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
| `configure(path, remove_existing)` | Open database in full mode |
| `configure_for_testing(path, remove_existing)` | Open with smaller file sizes |
| `insert(key, value, height)` | Insert UTXO with byte data |
| `find(key, height)` | Returns `optional<full_find_result>` (`data`, `block_height`) |
| `process_pending_lookups()` | Returns `(flat_map<key, full_find_result>, failed)` |
| `for_each_entry(callback)` | Callback: `(key, height, span<uint8_t const>)` |

### `compact_db`

| Method | Description |
|--------|-------------|
| `configure(path, remove_existing)` | Open database in compact mode |
| `configure_for_testing(path, remove_existing)` | Open with smaller file sizes |
| `insert(key, file_number, offset, height)` | Insert UTXO with typed fields |
| `find(key, height)` | Returns `optional<compact_find_result>` (`block_height`, `file_number`, `offset`) |
| `process_pending_lookups()` | Returns `(flat_map<key, compact_find_result>, failed)` |
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
