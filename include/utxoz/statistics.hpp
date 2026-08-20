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
#include <string>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>

#include <utxoz/config.hpp>
#include <utxoz/types.hpp>

namespace utxoz {

/**
 * @brief What probes saw.
 *
 * A probe is a find(): it either finds the key in the active map or it does not,
 * in which case the caller keeps the request and resolves it later.
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
    size_t deferred = 0;               ///< left for the caller to resolve() later
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
 * Deletions are not counted here, and lookups are not counted in deferred_stats.
 * The two paths share nothing: a resolution writes only these counters, a
 * deletion only its own. Mixing the age of a read with the age of a delete
 * produces an average of two different things, and a `successfully_processed`
 * that moves for both tells an operator nothing about either.
 */
struct resolution_summary {
    size_t resolved = 0;          ///< keys a resolution answered
    /// keys a resolution proved absent
    ///
    /// Published only by a resolution that covered every version it needed to,
    /// which is the only state in which absence is a fact. An incomplete attempt
    /// publishes nothing at all — not this counter and not any other — so this
    /// never stands for "could not settle".
    size_t absent = 0;
    size_t files_visited = 0;     ///< version files opened or reused across sweeps
    size_t cache_hits = 0;        ///< of those, served by the file cache
    double avg_depth = 0.0;       ///< versions back from the active one, over resolved
    double cache_hit_rate = 0.0;  ///< cache_hits / files_visited
};

/// One class's share of the read path. Every field is a count of one specific
/// thing; see lookup_stats for why they are not interchangeable.
struct class_lookup_summary {
    /// Which class this is. `reference_class` in reference mode, where there is
    /// one class and it is not a size class.
    size_t container_class = 0;

    /// Times `find()` asked this class's active map. **Derived, not counted.**
    ///
    /// The search visits the classes in order and stops at the first that
    /// answers, so class `i` is asked exactly when the answer came from `i` or
    /// later — or from nowhere:
    ///
    ///     active_maps_probed[i] = (answered by class i or after) + deferred
    ///
    /// Counting it directly, one increment per class per lookup, measured +160%
    /// on a lookup that misses everywhere. Counting only where the search
    /// stopped, one increment per lookup, was still an atomic on the hot path
    /// for a number this arithmetic already had.
    size_t active_maps_probed = 0;
    size_t answered_from_active = 0;    ///< times it answered
    size_t resolved_historical = 0;     ///< keys a sweep answered from this class's history

    size_t generations_probed = 0;      ///< key-against-file probes charged here
    size_t files_opened = 0;            ///< files a sweep worked against, per file
    size_t cache_hits = 0;              ///< of those, served by the file cache

    /// Over the keys answered from history: how many file probes each took, and
    /// how far back the answering generation was. Different questions; see
    /// lookup_stats.
    double avg_probe_ordinal = 0.0;
    double avg_version_distance = 0.0;

    /// Buckets 1, 2, 3, 4, 5-8, 9+.
    std::array<size_t, 6> probe_ordinal_histogram{};
    std::array<size_t, 6> version_distance_histogram{};
};

/// The class index reference mode reports under. Not a size class, and not zero,
/// so that a reader cannot mistake it for container 0.
inline constexpr size_t reference_class = static_cast<size_t>(-1);

/**
 * @brief The read path, per class, plus the two figures that have no class.
 *
 * `lookups_received`, `deferred` and `absent` are global because they cannot be
 * attributed: a lookup arrives with a key and no size, and one that is absent was
 * refused by every class. Everything else that could be derived from the
 * per-class counters is derived from them and not kept separately — a second
 * running total is a second thing to be wrong.
 */
struct lookup_telemetry {
    /// Bumped when a field changes meaning or leaves.
    static constexpr uint32_t schema_version = 1;

    /// What this build carries: "off", "basic" or "lookup". One value rather than
    /// a pair of booleans, because "statistics are on" was never the same claim
    /// as "these counters were collected": at `basic` the first is true and the
    /// second is false, and a reader given only the first would take a page of
    /// zeros for a database that answered nothing.
    std::string statistics_level = "off";
    storage_mode mode = storage_mode::full;

    size_t lookups_received = 0;   ///< find() calls
    size_t deferred = 0;           ///< of those, the ones no active map answered
    size_t absent = 0;             ///< keys a *completed* sweep proved are nowhere

    /// One entry per class in full mode; exactly one, labelled reference_class,
    /// in reference mode. Never five empty ones.
    std::vector<class_lookup_summary> classes;

    /// Summed from `classes`, so they cannot disagree with it. Keeping a running
    /// global beside the per-class counters would be a second authority on one
    /// fact, and when the two disagreed nobody could say which was the defect.
    ///
    /// Defined here rather than in statistics.cpp because that translation unit
    /// is compiled only when statistics are on, and these have to exist either
    /// way: without statistics the classes are empty and every sum is zero,
    /// which is the honest answer and not a missing symbol.
    [[nodiscard]] size_t answered_from_active() const noexcept {
        return sum_of(&class_lookup_summary::answered_from_active);
    }
    [[nodiscard]] size_t resolved_historical() const noexcept {
        return sum_of(&class_lookup_summary::resolved_historical);
    }
    [[nodiscard]] size_t generations_probed() const noexcept {
        return sum_of(&class_lookup_summary::generations_probed);
    }
    [[nodiscard]] size_t files_opened() const noexcept {
        return sum_of(&class_lookup_summary::files_opened);
    }
    [[nodiscard]] size_t cache_hits() const noexcept {
        return sum_of(&class_lookup_summary::cache_hits);
    }

private:
    [[nodiscard]] size_t sum_of(size_t class_lookup_summary::*field) const noexcept {
        size_t total = 0;
        for (auto const& c : classes) total += c.*field;
        return total;
    }
};

/// Machine-readable, for comparing one run against another.
[[nodiscard]] std::string to_json(lookup_telemetry const&);

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
/// The thread's slot. Round-robin on first use and cached in a thread_local, so
/// distinct threads usually get distinct shards; correctness does not rest on it,
/// since the counters are atomic either way.
[[nodiscard]] size_t thread_shard_index() noexcept;

/// How many shards every width uses. Named outside the template because
/// `thread_shard_index()` has to know it and does not know the width.
inline constexpr size_t counter_shard_count = 64;

template <size_t Fields>
struct sharded_counters {
    static constexpr size_t shard_count = counter_shard_count;
    static constexpr size_t field_count = Fields;

    void add(size_t field, uint64_t amount) noexcept {
        shards_[thread_shard_index()].fields[field].fetch_add(amount, std::memory_order_relaxed);
    }

    [[nodiscard]] uint64_t sum(size_t field) const noexcept {
        uint64_t total = 0;
        for (auto const& s : shards_) total += s.fields[field].load(std::memory_order_relaxed);
        return total;
    }

    void reset() noexcept {
        for (auto& s : shards_) {
            for (auto& f : s.fields) f.store(0, std::memory_order_relaxed);
        }
    }

private:
    /// Padded to the widest cache line among the platforms we target — Apple
    /// Silicon uses 128 bytes, x86-64 uses 64 — so two shards never share one.
    struct alignas(128) shard {
        std::atomic<uint64_t> fields[field_count]{};
    };

    std::array<shard, shard_count> shards_;
};

/// Eight fields are 64 bytes, which fit one padded shard: 8 KiB an instance.
/// This is the width the counters that existed before the lookup telemetry use,
/// and parameterising the type is what keeps it that way. A single global width
/// wide enough for the widest user would have doubled every instance in every
/// build, including the ones that never enable the widest user.
using narrow_counters = sharded_counters<8>;

/// Twenty fields are 160 bytes, so two padded shards: 16 KiB an instance. Only
/// `lookup_stats` is this wide, and only when the lookup telemetry is compiled
/// in at all.
using wide_counters = sharded_counters<20>;

} // namespace detail

/**
 * @brief Probe counters. Safe to record from any number of threads.
 */
struct probe_stats {
    probe_stats() = default;
    probe_stats(probe_stats const&) = delete;
    probe_stats& operator=(probe_stats const&) = delete;

#if UTXOZ_STATISTICS_LEVEL >= 1
    /// A probe the active map answered, with the height it was created at.
    void record_answered(uint32_t access_height, uint32_t creation_height) noexcept;
    /// A probe that had to be deferred.
    void record_deferred() noexcept;

    void reset() noexcept;
    [[nodiscard]] probe_summary get_summary() const noexcept;

private:
    /// No `f_probes`. Every probe is either answered or deferred, so the total is
    /// the sum of the two and counting it as well was a third atomic increment
    /// on the hot path for a number arithmetic already had.
    enum field : size_t { f_answered, f_deferred, f_age_total };
    static_assert(f_age_total < detail::narrow_counters::field_count,
                  "probe_stats has outgrown its counter slots");

    detail::narrow_counters counters_;
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
 * @brief What one class of the read path did, counted per class.
 *
 * The three things this separates are three different questions, and the reason
 * they are separate is that the existing counters answered a mixture of them.
 *
 *  - **which class answered.** A full-mode lookup is not addressed to a class:
 *    the caller supplies a key, not a size, and `find()` probes the active maps
 *    in class order until one answers. So "the class of a lookup" only exists
 *    for a lookup that was answered, and an absent one belongs to no class at
 *    all. `active_maps_probed` is the other half of that: it says how often each
 *    class was *asked*, which falls off sharply after class 0 precisely because
 *    the search stops at the first hit.
 *
 *  - **how many files it cost** — `probe_ordinal`. A key answered by the third
 *    generation file searched cost three file probes, whatever those files are
 *    numbered. This is what "how much of the read path is used" means.
 *
 *  - **how far back it was found** — `version_distance`, the answering
 *    generation's distance from its class's active version. Compaction removes
 *    generations, so version numbers are sparse and this is *not* the ordinal;
 *    the file cache is consulted first, so the order of the search is not the
 *    order of the versions either. Two names, because they are two numbers.
 *
 * `generations_probed` is charged per key: a sweep of a thousand keys across
 * three files probed three thousand times, and an absent key was probed by every
 * file. `files_opened` and `cache_hits` are per file, which is the other
 * question — what the cache did — and they are never added to the first.
 */
struct lookup_stats {
    lookup_stats() = default;
    lookup_stats(lookup_stats const&) = delete;
    lookup_stats& operator=(lookup_stats const&) = delete;

    /// Ordinals and distances are histogrammed into fixed buckets: a bounded
    /// number of counters, no allocation, and no unbounded key space.
    static constexpr size_t bucket_count = 6;
    /// Which bucket a one-based ordinal or distance belongs in: 1, 2, 3, 4, 5-8,
    /// 9 or more.
    [[nodiscard]] static constexpr size_t bucket_of(uint64_t value) noexcept {
        if (value <= 4) return value == 0 ? 0 : value - 1;
        return value <= 8 ? 4 : 5;
    }

#if UTXOZ_STATISTICS_LEVEL >= 2
    void record_answered_from_active(uint64_t times = 1) noexcept;
    /// A key this class answered from a historical generation: how many file
    /// probes it took, and how far back the file was.
    /// A batch of keys this class answered from history, handed over as totals
    /// and histograms accumulated on the caller's stack. One publication per
    /// sweep per class, so the inner loop touches no atomic.
    void record_resolved_batch(uint64_t count, uint64_t probe_ordinal_total,
                               uint64_t version_distance_total,
                               std::array<uint64_t, bucket_count> const& ordinals,
                               std::array<uint64_t, bucket_count> const& distances) noexcept;
    void record_generations_probed(uint64_t probes) noexcept;
    void record_file_opened(uint64_t files, uint64_t cache_hits) noexcept;

    void reset() noexcept;
    [[nodiscard]] class_lookup_summary get_summary() const noexcept;

private:
    enum field : size_t {
        f_answered_from_active,
        f_resolved_historical,
        f_generations_probed,
        f_files_opened,
        f_cache_hits,
        f_version_distance_total,
        f_probe_ordinal_total,
        f_ordinal_bucket,                                  // six consecutive
        f_distance_bucket = f_ordinal_bucket + bucket_count // six more
    };
    static_assert(f_distance_bucket + bucket_count <= detail::wide_counters::field_count,
                  "lookup_stats has outgrown its counter slots");

    detail::wide_counters counters_;
#else
    void record_answered_from_active(uint64_t = 1) noexcept {}
    void record_resolved_batch(uint64_t, uint64_t, uint64_t,
                               std::array<uint64_t, bucket_count> const&,
                               std::array<uint64_t, bucket_count> const&) noexcept {}
    void record_generations_probed(uint64_t) noexcept {}
    void record_file_opened(uint64_t, uint64_t) noexcept {}
    void reset() noexcept {}
    [[nodiscard]] class_lookup_summary get_summary() const noexcept { return {}; }
#endif
};

/**
 * @brief Historical lookup resolution counters.
 */
struct resolution_stats {
    resolution_stats() = default;
    resolution_stats(resolution_stats const&) = delete;
    resolution_stats& operator=(resolution_stats const&) = delete;

#if UTXOZ_STATISTICS_LEVEL >= 1
    /// A key a sweep answered, at `depth` versions back from the active one.
    void record_resolved(uint32_t depth) noexcept;
    /// The same, for a batch whose depths were accumulated as a total. Identical
    /// arithmetic to calling the above once per key, without a list of depths
    /// held on the read path to replay it from.
    void record_resolved_batch(uint64_t count, uint64_t depth_total) noexcept;
    /// Keys a completed resolution proved absent.
    void record_absent(size_t count) noexcept;
    /// A version file a sweep worked against.
    void record_file_visited(bool cache_hit) noexcept;

    void reset() noexcept;
    [[nodiscard]] resolution_summary get_summary() const noexcept;

private:
    enum field : size_t { f_resolved, f_absent, f_depth_total, f_files, f_cache_hits };
    static_assert(f_cache_hits < detail::narrow_counters::field_count,
                  "resolution_stats has outgrown its counter slots");

    detail::narrow_counters counters_;
#else
    void record_resolved(uint32_t) noexcept {}
    void record_resolved_batch(uint64_t, uint64_t) noexcept {}
    void record_absent(size_t) noexcept {}
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
    size_t rehash_count = 0;         ///< Number of hash table rehashes
    boost::unordered_flat_map<size_t, size_t> value_size_distribution; ///< Value size -> count
};

/**
 * @brief Deferred deletion statistics
 *
 * Deletion counters only. Lookups are counted by resolution_stats, and nothing
 * writes both — a field that moved for a failed delete and for an outpoint that
 * is legitimately not stored described neither.
 */
struct deferred_stats {
    size_t successfully_processed = 0;        ///< Successfully processed deletions
    size_t failed_to_delete = 0;            ///< Failed deletion attempts
    size_t processing_runs = 0;              ///< Number of processing runs
    std::chrono::milliseconds total_processing_time{0}; ///< Total processing time
    boost::unordered_flat_map<size_t, size_t> deletions_by_depth; ///< Depth -> deletion count
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
 * @brief Why generations were replaced, counted for every build.
 *
 * Outside `UTXOZ_STATISTICS_LEVEL` on purpose. These are rare operational
 * transitions — a handful over an entire initial block download — and the reason
 * they exist is that the last investigation had to be reconstructed from a log
 * line that carried nothing. Making them optional would mean the next one starts
 * from the same place.
 *
 * They cost nothing when nothing happens: an increment on a path that runs once
 * per full generation.
 */
struct rotation_causes {
    /// The policy asked for the rotation and it completed.
    uint64_t preventive = 0;
    /// A `bad_alloc` asked for it, the map was intact, and it completed.
    uint64_t capacity_exception = 0;
    /// A rotation was asked for and could not be made. **Not** a completed
    /// rotation, and deliberately not added to either of the two above.
    uint64_t failed = 0;
    /// The map contradicted its guarantee after a `bad_alloc`. No rotation was
    /// attempted, which is why this is counted here rather than as a failure.
    uint64_t unexpected_post_exception = 0;

    /// Completed rotations, by cause. Excludes `failed`, which completed nothing.
    [[nodiscard]] constexpr uint64_t completed() const noexcept {
        return preventive + capacity_exception;
    }
};

/**
 * @brief Complete database statistics
 */
struct database_statistics {
    // Storage mode
    storage_mode mode = storage_mode::full;

    /// The read path, per class. See lookup_telemetry.
    lookup_telemetry lookups;

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

    /// Why those rotations happened, per container. Filled in every build,
    /// including one compiled with statistics off: these are a handful of rare
    /// operational transitions, not hot-path telemetry, and the reason they are
    /// counted is that the investigation into the first one had to be
    /// reconstructed from a log line that carried nothing.
    ///
    /// `failed` is not a rotation that happened and is not part of
    /// `rotations_per_container`; `unexpected_post_exception` rotates nothing at
    /// all. See detail/insert_transition.hpp for the transitions that move them.
    std::array<rotation_causes, container_count> rotations_by_cause{};
    
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
