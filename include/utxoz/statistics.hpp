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

#include <utxoz/config.hpp>
#include <utxoz/types.hpp>

namespace utxoz {

/**
 * @brief What probes saw.
 *
 * A probe is a find(): it either finds the key in the active map or it defers.
 * This measures how often the requested key is present in an active map. It does
 * not yet measure whether the caller can use that answer; see the warning below.
 *
 * @warning It measures **presence in the active map**, which is not yet the same
 * as "the caller could use the answer". The height argument does not affect the
 * result today, so an entry created above the height being validated against is
 * still found here and counted in `answered_from_active`, even though the caller
 * discards it afterwards. When `max_creation_height` becomes a real bound, those
 * move to `deferred` and the two meanings converge — until then read this as a
 * hit rate against the active map, not as the share that avoids resolution.
 */
struct probe_summary {
    size_t probes = 0;                 ///< find() calls
    size_t answered_from_active = 0;   ///< key present in an active map
    size_t deferred = 0;               ///< queued for historical resolution
    double active_map_hit_rate = 0.0;  ///< answered_from_active / probes
    double avg_age_answered = 0.0;     ///< blocks between creation and probe, over the answered ones
};

/**
 * @brief What the historical lookup sweeps did.
 *
 * Deliberately separate from probe_summary: one logical lookup shows up in both,
 * once as a deferred probe and once as a resolution, and folding them into a
 * single denominator would make every ratio meaningless.
 *
 * Deletions are not counted here. They resolve through their own sweep and are
 * reported by deferred_stats — mixing the age of a read with the age of a delete
 * produces an average of two different things.
 */
struct resolution_summary {
    size_t resolved = 0;          ///< keys a sweep answered
    size_t unresolved = 0;        ///< keys a sweep could not settle
    size_t files_visited = 0;     ///< version files opened or reused across sweeps
    size_t cache_hits = 0;        ///< of those, served by the file cache
    double avg_depth = 0.0;       ///< versions back from the active one, over resolved
    double cache_hit_rate = 0.0;  ///< cache_hits / files_visited
};

namespace detail {

/**
 * @brief Cache-line padded atomic counters, spread across shards.
 *
 * The recording paths are concurrent and must not contend, so each thread keeps
 * to its own shard; the summary sums them. Slots are handed out round-robin and
 * reused once more threads have recorded than there are slots, so a distinct
 * slot per thread is the common case rather than a guarantee — correctness does
 * not rest on it, the counters are atomic either way.
 */
struct sharded_counters {
    static constexpr size_t shard_count = 64;
    static constexpr size_t field_count = 8;

    void add(size_t field, uint64_t amount) noexcept;
    [[nodiscard]] uint64_t sum(size_t field) const noexcept;
    void reset() noexcept;

private:
    /// Padded to the widest cache line among the platforms we target — Apple
    /// Silicon uses 128 bytes, x86-64 uses 64 — so two shards never share one.
    struct alignas(128) shard {
        std::atomic<uint64_t> fields[field_count]{};
    };

    std::array<shard, shard_count> shards_;
};

} // namespace detail

/**
 * @brief Probe counters. Safe to record from any number of threads.
 */
struct probe_stats {
    probe_stats() = default;
    probe_stats(probe_stats const&) = delete;
    probe_stats& operator=(probe_stats const&) = delete;

#ifdef UTXOZ_STATISTICS_ENABLED
    /// A probe the active map answered, with the height it was created at.
    void record_answered(uint32_t access_height, uint32_t creation_height) noexcept;
    /// A probe that had to be deferred.
    void record_deferred() noexcept;

    void reset() noexcept;
    [[nodiscard]] probe_summary get_summary() const noexcept;

private:
    enum field : size_t { f_probes, f_answered, f_age_total };
    static_assert(f_age_total < detail::sharded_counters::field_count,
                  "probe_stats has outgrown its counter slots");

    detail::sharded_counters counters_;
#else
    // Compiled out entirely: no counters, and recording is a no-op the optimiser
    // deletes at the call site. Guarding here rather than at each call site is
    // deliberate — a call added later cannot forget the guard and put a
    // fetch_add back on the concurrent path.
    void record_answered(uint32_t, uint32_t) noexcept {}
    void record_deferred() noexcept {}
    void reset() noexcept {}
    [[nodiscard]] probe_summary get_summary() const noexcept { return {}; }
#endif
};

/**
 * @brief Historical lookup resolution counters.
 */
struct resolution_stats {
    resolution_stats() = default;
    resolution_stats(resolution_stats const&) = delete;
    resolution_stats& operator=(resolution_stats const&) = delete;

#ifdef UTXOZ_STATISTICS_ENABLED
    /// A key a sweep answered, at `depth` versions back from the active one.
    void record_resolved(uint32_t depth) noexcept;
    /// Keys a sweep finished without settling.
    void record_unresolved(size_t count) noexcept;
    /// A version file a sweep worked against.
    void record_file_visited(bool cache_hit) noexcept;

    void reset() noexcept;
    [[nodiscard]] resolution_summary get_summary() const noexcept;

private:
    enum field : size_t { f_resolved, f_unresolved, f_depth_total, f_files, f_cache_hits };
    static_assert(f_cache_hits < detail::sharded_counters::field_count,
                  "resolution_stats has outgrown its counter slots");

    detail::sharded_counters counters_;
#else
    void record_resolved(uint32_t) noexcept {}
    void record_unresolved(size_t) noexcept {}
    void record_file_visited(bool) noexcept {}
    void reset() noexcept {}
    [[nodiscard]] resolution_summary get_summary() const noexcept { return {}; }
#endif
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
    probe_summary probes;            ///< What probes answered on their own
    resolution_summary resolution;   ///< What the historical lookup sweeps did
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
