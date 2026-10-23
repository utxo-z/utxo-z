// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "detail/shard.hpp"

#include "bench_common.hpp"

#include <filesystem>
#include <fmt/format.h>

namespace fs = std::filesystem;

namespace bench {

namespace {

// The files of one class, across every container it has. The names come from
// the same function the library writes them with, so a class that is split is
// counted whole rather than reported as empty.
size_t count_container_files(std::string const& path, size_t container_index) {
    std::vector<std::string> prefixes;
    for (uint32_t sh = 0; sh != utxoz::detail::shards_in_class(container_index); ++sh) {
        prefixes.push_back(
            utxoz::detail::container_prefix(utxoz::detail::slot_of(container_index, sh)));
    }
    size_t count = 0;
    for (auto const& entry : fs::directory_iterator(path)) {
        auto name = entry.path().filename().string();
        if ( ! name.ends_with(".dat")) continue;
        for (auto const& prefix : prefixes) {
            if (name.starts_with(prefix)) { ++count; break; }
        }
    }
    return count;
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
    // Dynamically generated from container_sizes and container_capacities,
    // so it works for any container layout (4 or 5 containers).

    fmt::println("\n{:=^80}", " Storage Overhead Report ");
    fmt::println("  Values at max capacity — isolates purely structural overhead.\n");

    constexpr size_t key_size = utxoz::outpoint_size;  // 36

    for (size_t i = 0; i < utxoz::container_count; ++i) {
        size_t container_size = utxoz::container_sizes[i];
        size_t max_data = utxoz::container_capacities[i];

        BenchFixture f;
        size_t entries = fill_until_rotation(*f.db, f.path, max_data, i);
        f.db.reset();

        size_t file_size = utxoz::test_file_sizes[i];
        size_t pair_size = key_size + container_size;
        size_t entry_data = entries * pair_size;
        double overhead = double(file_size) / double(entry_data);

        fmt::println("--- {}B container ---", container_size);
        fmt::println("  Entries at rotation:  {:>10L}", entries);
        fmt::println("  sizeof(pair):         {:>10} B  (key {} + value {})", pair_size, key_size, container_size);
        fmt::println("  N * sizeof(pair):     {:>10.2f} MiB", double(entry_data) / (1024.0 * 1024.0));
        fmt::println("  File size:            {:>10.2f} MiB", double(file_size) / (1024.0 * 1024.0));
        fmt::println("  Structural overhead:  {:>10.2f}x", overhead);
        fmt::println("");
    }

    fmt::println("{:=^80}\n", "");
}

} // namespace bench
