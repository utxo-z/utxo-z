// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench_common.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <random>
#include <vector>

#include <fmt/format.h>

namespace fs = std::filesystem;

namespace bench_large {

namespace {

size_t dir_size_bytes(std::string const& path) {
    size_t total = 0;
    for (auto const& entry : fs::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {
            total += entry.file_size();
        }
    }
    return total;
}

size_t count_files_with_prefix(std::string const& path, std::string const& prefix) {
    size_t count = 0;
    for (auto const& entry : fs::directory_iterator(path)) {
        auto name = entry.path().filename().string();
        if (name.starts_with(prefix) && name.ends_with(".dat")) {
            ++count;
        }
    }
    return count;
}

struct timer {
    std::chrono::high_resolution_clock::time_point start_;
    timer() : start_(std::chrono::high_resolution_clock::now()) {}
    double elapsed_s() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(now - start_).count();
    }
    void reset() { start_ = std::chrono::high_resolution_clock::now(); }
};

} // anonymous namespace

void run_ibd_simulation() {
    fmt::println("{:=^80}", " IBD Simulation (Production Scale) ");

    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto unique_id = bench::bench_counter.fetch_add(1);
    auto path = fmt::format("./bench_large_ibd_{}_{}_{}", getpid(), ts, unique_id);

    if (fs::exists(path)) {
        fs::remove_all(path);
    }

    utxoz::db db;
    db.configure(path, true);

    // =========================================================================
    // Parameters
    // =========================================================================
    constexpr size_t total_inserts      = 50'000'000;
    constexpr double erase_ratio        = 0.60;
    constexpr size_t deferred_interval  = 500'000;
    constexpr size_t progress_interval  = 5'000'000;

    // Pre-create value buffers (avoid per-insert allocation)
    // Distribution from real BCH chain sync at block 930K:
    //   82% P2PKH (43B), 13% P2SH (41B), 4% 123B, 1% 89B
    auto value_43  = bench::make_test_value(43);
    auto value_41  = bench::make_test_value(41);
    auto value_123 = bench::make_test_value(123);
    auto value_89  = bench::make_test_value(89);

    std::mt19937 rng(42);

    // Reserve space for keys we'll erase later
    size_t erase_count = static_cast<size_t>(total_inserts * erase_ratio);
    std::vector<uint32_t> keys_to_erase;
    keys_to_erase.reserve(erase_count);

    // =========================================================================
    // Phase 1: Massive insert
    // =========================================================================
    fmt::println("\n--- Phase 1: Insert {:L} entries ---", total_inserts);
    timer t;

    for (size_t i = 0; i < total_inserts; ++i) {
        // Choose value size from chain distribution
        uint32_t r = rng() % 100;
        utxoz::output_data_span value;
        if (r < 82) value = value_43;        // P2PKH
        else if (r < 95) value = value_41;   // P2SH
        else if (r < 99) value = value_123;
        else value = value_89;

        auto key = bench::make_test_key(static_cast<uint32_t>(i), 0);
        (void)db.insert(key, value, static_cast<uint32_t>(i / 1000));

        // Remember some keys for erasing (uniformly sampled)
        if (keys_to_erase.size() < erase_count) {
            if ((i * 2654435761u) % 100 < static_cast<size_t>(erase_ratio * 100)) {
                keys_to_erase.push_back(static_cast<uint32_t>(i));
            }
        }

        if ((i + 1) % progress_interval == 0) {
            fmt::println("  {:>10L} / {:L}  ({:.1f}s, db size: {:L})",
                i + 1, total_inserts, t.elapsed_s(), db.size());
        }
    }

    double insert_s = t.elapsed_s();
    fmt::println("  Insert complete: {:.1f}s  ({:.0f} inserts/sec)",
        insert_s, total_inserts / insert_s);
    fmt::println("  DB size after inserts: {:L}", db.size());

    // =========================================================================
    // Phase 2: Erase (shuffle keys for realistic random-access pattern)
    // =========================================================================
    fmt::println("\n--- Phase 2: Erase {:L} entries ---", keys_to_erase.size());
    std::shuffle(keys_to_erase.begin(), keys_to_erase.end(), rng);

    size_t deferred_total = 0;
    t.reset();

    for (size_t i = 0; i < keys_to_erase.size(); ++i) {
        auto key = bench::make_test_key(keys_to_erase[i], 0);
        (void)db.erase(key, static_cast<uint32_t>(total_inserts / 1000 + i / 1000));

        if ((i + 1) % deferred_interval == 0) {
            auto [processed, failed] = db.process_pending_deletions();
            deferred_total += processed;
        }

        if ((i + 1) % progress_interval == 0) {
            fmt::println("  {:>10L} / {:L}  ({:.1f}s, db size: {:L})",
                i + 1, keys_to_erase.size(), t.elapsed_s(), db.size());
        }
    }

    double erase_s = t.elapsed_s();
    fmt::println("  Erase complete: {:.1f}s  ({:.0f} erases/sec)",
        erase_s, keys_to_erase.size() / erase_s);
    fmt::println("  Deferred processed during erase: {:L}", deferred_total);

    // =========================================================================
    // Phase 3: Final deferred processing
    // =========================================================================
    fmt::println("\n--- Phase 3: Process remaining deferred deletions ---");
    t.reset();
    auto [processed, failed] = db.process_pending_deletions();
    double deferred_s = t.elapsed_s();
    fmt::println("  Processed: {:L}, Failed: {:L}, Time: {:.3f}s",
        processed, failed.size(), deferred_s);

    // =========================================================================
    // Phase 4: Find throughput on the resulting database
    // =========================================================================
    fmt::println("\n--- Phase 4: Find throughput ({:L} lookups) ---", 1'000'000UL);

    std::uniform_int_distribution<uint32_t> dist(0, total_inserts - 1);
    t.reset();
    size_t found = 0;
    for (size_t i = 0; i < 1'000'000; ++i) {
        auto key = bench::make_test_key(dist(rng), 0);
        auto result = db.find(key, static_cast<uint32_t>(total_inserts / 1000));
        if (result) ++found;
    }
    double find_s = t.elapsed_s();
    fmt::println("  Find complete: {:.1f}s  ({:.0f} finds/sec, {:.1f}% hits)",
        find_s, 1'000'000 / find_s, found / 10'000.0);

    // =========================================================================
    // Summary
    // =========================================================================
    double total_s = insert_s + erase_s + deferred_s + find_s;

    fmt::println("\n{:=^80}", " IBD Results ");
    fmt::println("  Total time:        {:>10.1f}s", total_s);
    fmt::println("  Insert:            {:>12L}  ({:>10.0f} ops/sec)", total_inserts, total_inserts / insert_s);
    fmt::println("  Erase:             {:>12L}  ({:>10.0f} ops/sec)", keys_to_erase.size(), keys_to_erase.size() / erase_s);
    fmt::println("  Find (1M random):  {:>12.0f} ops/sec", 1'000'000 / find_s);
    fmt::println("  Live UTXOs:        {:>12L}", db.size());
    fmt::println("  Disk usage:        {:>10.2f} GiB", dir_size_bytes(path) / (1024.0 * 1024.0 * 1024.0));

    for (size_t i = 0; i < utxoz::container_count; ++i) {
        auto prefix = fmt::format("cont_{}_v", i);
        size_t file_count = count_files_with_prefix(path, prefix);
        if (file_count > 0) {
            fmt::println("  Container {} ({:>5}B):  {} file(s)",
                i, utxoz::container_sizes[i], file_count);
        }
    }
    fmt::println("{:=^80}\n", "");

    db.close();
    fs::remove_all(path);
}

} // namespace bench_large
