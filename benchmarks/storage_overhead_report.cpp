// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench_common.hpp"

#include <filesystem>
#include <fmt/format.h>

namespace fs = std::filesystem;

namespace bench {

namespace {

// File naming follows: cont_{index}_v{version:05}.dat
// E.g., cont_0_v00000.dat, cont_0_v00001.dat, ...
size_t count_container_files(std::string const& path, size_t container_index) {
    auto prefix = fmt::format("cont_{}_v", container_index);
    size_t count = 0;
    for (auto const& entry : fs::directory_iterator(path)) {
        auto name = entry.path().filename().string();
        if (name.starts_with(prefix) && name.ends_with(".dat")) {
            ++count;
        }
    }
    return count;
}

size_t get_container_index(size_t data_size) {
    for (size_t i = 0; i < utxoz::container_sizes.size(); ++i) {
        if (data_size <= utxoz::container_sizes[i]) return i;
    }
    return utxoz::container_sizes.size() - 1;
}

// Insert entries until the container rotates (a second version file appears).
// Returns the number of entries that fit in the first file (before rotation).
size_t fill_until_rotation(utxoz::db& db, std::string const& path,
                           size_t data_size, size_t container_index) {
    auto value = make_test_value(data_size);
    size_t initial_files = count_container_files(path, container_index);
    for (size_t i = 0;; ++i) {
        auto key = make_test_key(static_cast<uint32_t>(i), 0);
        (void)db.insert(key, value, 100);
        if (count_container_files(path, container_index) > initial_files) {
            return i;  // entry i went to the new file, so i entries fit
        }
    }
}

} // anonymous namespace

void run_storage_overhead_report() {
    // Structural overhead report.
    //
    // Measures how much extra space the hash table infrastructure and the
    // file/segment manager add beyond the raw entry data.
    //
    // Method:
    //   For each container size, insert entries (with values at maximum
    //   capacity) until the file rotates. At that point the file is as
    //   full as the allocator allows.
    //
    // Metric:
    //   structural_overhead = file_size / (N * sizeof(pair<key, utxo_value<S>>))
    //
    //   A value of 1.0x means zero overhead (all file space is entry data).
    //   A value of 1.25x means 25% of the file is used by infrastructure
    //   (hash table control bytes, unused slots due to load factor,
    //   segment manager bookkeeping, etc.).

    fmt::println("\n{:=^80}", " Storage Overhead Report ");
    fmt::println("  Values at max capacity — isolates purely structural overhead.\n");

    struct OverheadCase {
        char const* label;
        size_t container_size;
        size_t max_data_size;  // container_size - sizeof(block_height) - sizeof(size_type)
    };

    // size_type<S> = uint8_t (1B) if S <= 255, uint16_t (2B) otherwise
    OverheadCase const cases[] = {
        {"44B container",    44,    39},    // 44 - 4 - 1
        {"128B container",  128,   123},    // 128 - 4 - 1
        {"512B container",  512,   506},    // 512 - 4 - 2
        {"10KB container", 10240, 10234},   // 10240 - 4 - 2
    };

    constexpr size_t key_size = utxoz::outpoint_size;  // 36

    for (auto const& c : cases) {
        size_t index = get_container_index(c.max_data_size);
        BenchFixture f;
        size_t entries = fill_until_rotation(f.db, f.path, c.max_data_size, index);
        f.db.close();

        size_t file_size = utxoz::test_file_sizes[0];
        size_t pair_size = key_size + c.container_size;
        size_t entry_data = entries * pair_size;
        double overhead = double(file_size) / double(entry_data);

        fmt::println("--- {} ---", c.label);
        fmt::println("  Entries at rotation:  {:>10L}", entries);
        fmt::println("  sizeof(pair):         {:>10} B  (key {} + value {})", pair_size, key_size, c.container_size);
        fmt::println("  N * sizeof(pair):     {:>10.2f} MiB", double(entry_data) / (1024.0 * 1024.0));
        fmt::println("  File size:            {:>10.2f} MiB", double(file_size) / (1024.0 * 1024.0));
        fmt::println("  Structural overhead:  {:>10.2f}x", overhead);
        fmt::println("");
    }

    fmt::println("{:=^80}\n", "");
}

} // namespace bench
