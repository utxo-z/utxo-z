// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file statistics.hpp
 * @brief Statistics and performance monitoring for UTXO database
 */

#pragma once

#include <chrono>
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

struct search_stats {
    void add_record(uint32_t access_height, uint32_t insertion_height,
                    uint32_t depth, bool cache_hit, bool found, char operation);
    void reset();
    [[nodiscard]]
    search_summary get_summary() const;

private:
    std::vector<search_record> records_;
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

} // namespace utxoz
