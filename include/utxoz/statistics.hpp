// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file statistics.hpp
 * @brief Statistics and performance monitoring for UTXO database
 */

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>

#include <utxoz/types.hpp>

namespace utxoz {

/**
 * @brief Search statistics summary
 */
struct search_summary {
    size_t total_operations = 0;      ///< Total search operations
    size_t found_operations = 0;      ///< Operations that found the UTXO
    size_t current_version_hits = 0;  ///< Hits in current version (depth 0)
    size_t cache_hits = 0;           ///< Cache hits for previous versions
    double avg_depth = 0.0;          ///< Average search depth
    double avg_utxo_age = 0.0;       ///< Average UTXO age in blocks
    double cache_hit_rate = 0.0;     ///< Cache hit rate (0.0-1.0)
    double hit_rate = 0.0;           ///< Overall hit rate (0.0-1.0)
};

/**
 * @brief Search counters, kept when statistics are compiled in.
 *
 * Only aggregates are retained — everything search_summary reports is a sum or
 * a ratio of sums — so recording an operation costs a handful of relaxed
 * fetch_add's and a fixed amount of memory, independent of how long the process
 * runs.
 *
 * @par Thread safety
 * add_record() is safe to call from any number of threads, and is written to
 * stay off the contention path: the counters are sharded and writers spread
 * across the shards, so concurrent recorders rarely touch the same cache line.
 * Slots are handed out round-robin and reused once more threads have recorded
 * than there are slots, so a distinct slot per thread is the common case rather
 * than a guarantee — correctness never rests on it, the counters are atomic.
 * This is what lets find() be called concurrently.
 *
 * get_summary() and reset() are safe to call while add_record() runs, but they
 * are not atomic — neither across shards nor across fields. A summary taken
 * while recording is in flight can show counters that disagree with each other,
 * because each is read separately: found can sit one increment ahead of
 * operations, cache_hits ahead of cache_accesses. The reported ratios are
 * clamped so they never exceed 1, but if you need counts that are internally
 * consistent, take the summary while nothing is recording.
 *
 * @note Statistics are ON by default. The macro is baked into the installed
 * config.hpp by this project's own build, so it is not something a consumer
 * opts into; check config.hpp in the package to see what you got. Build with
 * UTXOZ_STATISTICS_ENABLED=OFF (Conan option `statistics=False`) to compile the
 * recording out entirely.
 */
struct search_stats {
    search_stats() = default;

    // Non-copyable, non-movable: holds atomics.
    search_stats(search_stats const&) = delete;
    search_stats& operator=(search_stats const&) = delete;

    void add_record(uint32_t access_height, uint32_t insertion_height,
                    uint32_t depth, bool cache_hit, bool found, char operation);
    void reset();
    [[nodiscard]]
    search_summary get_summary() const;

    /// Number of counter shards. A power of two, and enough that collisions
    /// stay negligible for any realistic thread count.
    static constexpr size_t shard_count = 64;

private:
    /// Padded to the widest cache line among the platforms we target — Apple
    /// Silicon uses 128 bytes, x86-64 uses 64 — so two shards never share a
    /// line on either. 64 slots is 8 KiB per database, which buys nothing back
    /// by being tighter.
    struct alignas(128) shard {
        std::atomic<uint64_t> operations{0};
        std::atomic<uint64_t> found{0};
        std::atomic<uint64_t> current_version_hits{0};
        std::atomic<uint64_t> total_depth{0};
        std::atomic<uint64_t> total_age{0};
        std::atomic<uint64_t> cache_accesses{0};
        std::atomic<uint64_t> cache_hits{0};
    };

    std::array<shard, shard_count> shards_;
};

/**
 * @brief Per-container statistics
 */
struct container_stats {
    size_t total_inserts = 0;        ///< Total insertions
    size_t total_deletes = 0;        ///< Total deletions
    size_t current_size = 0;         ///< Current number of entries
    size_t failed_deletes = 0;       ///< Failed deletion attempts
    size_t deferred_deletes = 0;     ///< Deferred deletions
    size_t deferred_lookups = 0;     ///< Deferred lookups
    size_t rehash_count = 0;         ///< Number of hash table rehashes
    boost::unordered_flat_map<size_t, size_t> value_size_distribution; ///< Value size -> count
};

/**
 * @brief Deferred deletion statistics
 */
struct deferred_stats {
    size_t total_deferred = 0;               ///< Total deferred deletions
    size_t successfully_processed = 0;        ///< Successfully processed deletions
    size_t failed_to_delete = 0;            ///< Failed deletion attempts
    size_t max_queue_size = 0;               ///< Maximum queue size reached
    size_t processing_runs = 0;              ///< Number of processing runs
    std::chrono::milliseconds total_processing_time{0}; ///< Total processing time
    boost::unordered_flat_map<size_t, size_t> deletions_by_depth; ///< Depth -> deletion count
    boost::unordered_flat_map<size_t, size_t> lookups_by_depth;   ///< Depth -> lookup count
};

/**
 * @brief Not found operation statistics
 */
struct not_found_stats {
    size_t total_not_found = 0;      ///< Total not found operations
    size_t total_search_depth = 0;   ///< Cumulative search depth
    size_t max_search_depth = 0;     ///< Maximum search depth encountered
    boost::unordered_flat_map<size_t, size_t> depth_distribution; ///< Depth -> count
};

/**
 * @brief UTXO lifetime statistics
 */
struct utxo_lifetime_stats {
    boost::unordered_flat_map<uint32_t, size_t> age_distribution; ///< Age in blocks -> count
    uint32_t max_age = 0;            ///< Maximum UTXO age observed
    double average_age = 0.0;        ///< Average UTXO age
    size_t total_spent = 0;          ///< Total UTXOs spent
};

/**
 * @brief Storage fragmentation statistics
 */
struct fragmentation_stats {
    std::array<double, container_count> fill_ratios{};  ///< Fill ratio per container
    std::array<size_t, container_count> wasted_space{}; ///< Wasted space per container
};

/**
 * @brief Complete database statistics
 */
struct database_statistics {
    // Storage mode
    storage_mode mode = storage_mode::full;

    // Global statistics
    size_t total_entries = 0;        ///< Total entries across all containers
    size_t total_inserts = 0;        ///< Total insertions performed
    size_t total_deletes = 0;        ///< Total deletions performed
    
    // Per-container statistics
    std::array<container_stats, container_count> containers;
    
    // Cache statistics
    float cache_hit_rate = 0.0f;     ///< File cache hit rate
    size_t cached_files_count = 0;   ///< Number of currently cached files
    std::vector<std::pair<size_t, size_t>> cached_files_info; ///< (container, version) pairs
    
    // Detailed statistics
    deferred_stats deferred;         ///< Deferred deletion statistics
    not_found_stats not_found;       ///< Not found operation statistics
    search_summary search;           ///< Search performance summary
    utxo_lifetime_stats lifetime;    ///< UTXO lifetime statistics
    fragmentation_stats fragmentation; ///< Storage fragmentation statistics
    
    // File rotation statistics
    std::array<size_t, container_count> rotations_per_container{};
    
    // Memory usage estimates
    std::array<size_t, container_count> memory_usage_per_container{};
};

/**
 * @brief Per-height-range statistics for tracking UTXO distribution over time
 *
 * Tracks inserts and deletes per container per height range (default 10,000 blocks).
 * Call db::print_height_range_stats() after a full chain sync to see how the
 * value size distribution evolves across the blockchain.
 */
struct height_range_stats {
    static constexpr uint32_t range_size = 10000;

    struct range_data {
        std::array<size_t, container_count> inserts{};
        std::array<size_t, container_count> deletes{};
    };

    boost::unordered_flat_map<uint32_t, range_data> ranges; ///< key = height / range_size
};

/**
 * @brief Sizing analysis report for optimizing container and file sizes
 *
 * Generated from existing statistics data (no additional hot-path tracking).
 * Call db::print_sizing_report() after a full chain sync to get the data
 * needed for sizing decisions.
 */
struct sizing_report {
    struct container_info {
        size_t container_size;        ///< container_sizes[i] (e.g. 44, 128, 512, 10240)
        size_t file_size_setting;     ///< Configured file size for this container
        size_t file_count;            ///< Number of files (current_versions + 1)
        size_t current_entries;       ///< Live entries now
        size_t historical_inserts;    ///< Total inserts ever
        size_t historical_deletes;    ///< Total deletes ever
        size_t total_wasted_bytes;    ///< Sum of (container_size - value_size) for all inserts
        double avg_waste_per_entry;   ///< total_wasted_bytes / historical_inserts (or 0)
    };
    std::array<container_info, container_count> containers;
    boost::unordered_flat_map<size_t, size_t> global_value_size_histogram; ///< value_size -> count
};

} // namespace utxoz
