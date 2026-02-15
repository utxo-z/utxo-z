# UTXO-Z

> **Alpha Software**: This is a preliminary alpha version of UTXO-Z. Its implementation is subject to change as it is being experimented with for the first time in Bitcoin Cash nodes. The API is also subject to change.

High-performance UTXO database for Bitcoin Cash.

UTXO-Z is a purpose-built storage engine designed for the specific access patterns of UTXO set management. It achieves **2.5x higher throughput** than LevelDB during blockchain synchronization and up to **45x faster lookups** for recent outputs.

For the technical paper describing the architecture and benchmarks, see [docs/utxoz.pdf](docs/utxoz.pdf).

## Features

- **Multi-container architecture**: Optimized storage for different value sizes (44B, 128B, 512B, 10KB)
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
    self.requires("utxoz/0.1.0")
```

### Building from source

```bash
# Clone
git clone https://github.com/utxo-z/utxo-z.git
cd utxo-z

# Build and install to local Conan cache
./scripts/build-create.sh 0.1.0
```

## Usage

### Basic example

```cpp
#include <utxoz/utxoz.hpp>

int main() {
    utxoz::db db;
    db.configure("./utxo_data", true);  // path, remove_existing

    // Create a key (32-byte tx hash + 4-byte output index)
    std::array<uint8_t, 32> tx_hash = { /* ... */ };
    auto key = utxoz::make_key(tx_hash, 0);  // output index 0

    // Insert UTXO
    std::vector<uint8_t> value = { /* serialized output */ };
    db.insert(key, value, block_height);

    // Find UTXO
    auto result = db.find(key, current_height);
    if (result) {
        // Use result.value()
    }

    // Erase UTXO (may be deferred)
    db.erase(key, current_height);

    // Process deferred deletions periodically
    auto [deleted, failed] = db.process_pending_deletions();
    for (auto const& entry : failed) {
        // entry.key   - the UTXO key that failed to delete
        // entry.height - the block height that requested the deletion
    }

    // Compact periodically for optimal performance
    db.compact_all();

    db.close();
}
```

### Logging

UTXO-Z supports three logging backends configured at build time:

```bash
./scripts/build-create.sh 0.1.0 custom   # Callback-based (default)
./scripts/build-create.sh 0.1.0 spdlog   # spdlog integration
./scripts/build-create.sh 0.1.0 none     # Disabled
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

### `utxoz::db`

| Method | Description |
|--------|-------------|
| `configure(path, remove_existing)` | Open database at path |
| `configure_for_testing(path, remove_existing)` | Open with smaller file sizes |
| `close()` | Close and flush all data |
| `size()` | Total UTXO count |
| `insert(key, value, height)` | Insert UTXO, returns success |
| `find(key, height)` | Find UTXO, returns optional value |
| `erase(key, height)` | Erase UTXO (may be deferred) |
| `process_pending_deletions()` | Process deferred deletes, returns (count, failed entries with height) |
| `compact_all()` | Optimize storage |
| `get_statistics()` | Get performance stats |
| `print_statistics()` | Log formatted stats |

### `utxoz::key_t`

36-byte key: 32-byte transaction hash + 4-byte output index (little-endian).

```cpp
auto key = utxoz::make_key(tx_hash, output_index);
```

## Performance

Benchmarks on consumer hardware (single thread):

| Metric | UTXO-Z | LevelDB | Improvement |
|--------|--------|---------|-------------|
| IBD inputs/sec | 290K | 117K | 2.5x |
| IBD outputs/sec | 320K | 129K | 2.5x |
| Recent lookups/sec | 33M | 740K | 45x |
| Total lookups/sec | 1.5M | 740K | 2x |

## License

MIT License. See [LICENSE](LICENSE).
