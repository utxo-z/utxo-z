// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file statistics.cpp
 * @brief Statistics implementation
 */

#include <utxoz/statistics.hpp>

#include <algorithm>
#include <atomic>
#include <thread>

#if UTXOZ_STATISTICS_LEVEL >= 1

namespace utxoz {
namespace detail {

/**
 * @brief The shard this thread records into.
 *
 * Assigned round-robin on first use and then cached, so recording never hashes.
 * The counter is process-wide, which is what we want: it spreads writers across
 * slots regardless of how many database instances they touch.
 *
 * Out of line and shared by every width, so that the counter that assigns slots
 * is one counter rather than one per instantiation.
 */
size_t thread_shard_index() noexcept {
    static std::atomic<size_t> next_index{0};
    static thread_local size_t const index =
        next_index.fetch_add(1, std::memory_order_relaxed) % counter_shard_count;
    return index;
}

} // namespace detail

namespace detail {

// Relaxed throughout: these counters carry no ordering relationship with
// anything else, and a summary is allowed to be a slightly stale snapshot.

} // namespace detail

// =============================================================================
// probe_stats
// =============================================================================

void probe_stats::record_answered(uint32_t access_height, uint32_t creation_height) noexcept {
    counters_.add(f_answered, 1);

    uint64_t const age = access_height >= creation_height
        ? uint64_t(access_height) - creation_height
        : 0;
    counters_.add(f_age_total, age);
}

void probe_stats::record_deferred() noexcept {
    counters_.add(f_deferred, 1);
}

void probe_stats::reset() noexcept {
    counters_.reset();
}

probe_summary probe_stats::get_summary() const noexcept {
    uint64_t const answered = counters_.sum(f_answered);
    uint64_t const deferred = counters_.sum(f_deferred);
    uint64_t const age_total = counters_.sum(f_age_total);

    probe_summary summary;
    // Derived, not counted: every probe is answered or deferred. The clamping the
    // old form needed is gone with it — there is no denominator to outrun when
    // the denominator is the sum of the two numerators.
    uint64_t const probes = answered + deferred;
    if (probes == 0) return summary;

    summary.probes = size_t(probes);
    summary.answered_from_active = size_t(answered);
    summary.deferred = size_t(deferred);
    summary.active_map_hit_rate = double(answered) / double(probes);

    if (answered > 0) {
        summary.avg_age_answered = double(age_total) / double(answered);
    }

    return summary;
}

// =============================================================================
// resolution_stats
// =============================================================================

void resolution_stats::record_resolved(uint32_t depth) noexcept {
    counters_.add(f_resolved, 1);
    counters_.add(f_depth_total, depth);
}

void resolution_stats::record_resolved_batch(uint64_t count, uint64_t depth_total) noexcept {
    counters_.add(f_resolved, count);
    counters_.add(f_depth_total, depth_total);
}

void resolution_stats::record_absent(size_t count) noexcept {
    if (count == 0) return;
    counters_.add(f_absent, uint64_t(count));
}

void resolution_stats::record_file_visited(bool cache_hit) noexcept {
    counters_.add(f_files, 1);
    if (cache_hit) counters_.add(f_cache_hits, 1);
}

void resolution_stats::reset() noexcept {
    counters_.reset();
}

resolution_summary resolution_stats::get_summary() const noexcept {
    uint64_t const resolved = counters_.sum(f_resolved);
    uint64_t const absent = counters_.sum(f_absent);
    uint64_t const depth_total = counters_.sum(f_depth_total);
    uint64_t const files = counters_.sum(f_files);
    uint64_t const cache_hits = counters_.sum(f_cache_hits);

    resolution_summary summary;
    summary.resolved = size_t(resolved);
    summary.absent = size_t(absent);
    summary.files_visited = size_t(files);
    summary.cache_hits = size_t(std::min(cache_hits, files));

    if (resolved > 0) {
        summary.avg_depth = double(depth_total) / double(resolved);
    }
    if (files > 0) {
        summary.cache_hit_rate = double(summary.cache_hits) / double(files);
    }

    return summary;
}


#if UTXOZ_STATISTICS_LEVEL >= 2

// =============================================================================
// lookup_stats
// =============================================================================
//
// Every method here takes a count rather than being called once per event. The
// per-key work in a sweep is accumulated in locals on the caller's stack and
// handed over once, so the inner loop of a resolution touches no atomic at all —
// the same shape the existing tally already had, for the same reason.

void lookup_stats::record_answered_from_active(uint64_t times) noexcept {
    counters_.add(f_answered_from_active, times);
}

void lookup_stats::record_resolved_batch(
        uint64_t count, uint64_t probe_ordinal_total, uint64_t version_distance_total,
        std::array<uint64_t, bucket_count> const& ordinals,
        std::array<uint64_t, bucket_count> const& distances) noexcept {
    counters_.add(f_resolved_historical, count);
    counters_.add(f_probe_ordinal_total, probe_ordinal_total);
    counters_.add(f_version_distance_total, version_distance_total);
    for (size_t b = 0; b < bucket_count; ++b) {
        if (ordinals[b] != 0) counters_.add(f_ordinal_bucket + b, ordinals[b]);
        if (distances[b] != 0) counters_.add(f_distance_bucket + b, distances[b]);
    }
}

void lookup_stats::record_generations_probed(uint64_t probes) noexcept {
    counters_.add(f_generations_probed, probes);
}

void lookup_stats::record_file_opened(uint64_t files, uint64_t cache_hits) noexcept {
    counters_.add(f_files_opened, files);
    counters_.add(f_cache_hits, cache_hits);
}

void lookup_stats::reset() noexcept {
    counters_.reset();
}

class_lookup_summary lookup_stats::get_summary() const noexcept {
    class_lookup_summary out;
    // active_maps_probed is filled in by the caller, which is the only place that
    // can see the classes after this one.
    out.answered_from_active = counters_.sum(f_answered_from_active);
    out.resolved_historical = counters_.sum(f_resolved_historical);
    out.generations_probed = counters_.sum(f_generations_probed);
    out.files_opened = counters_.sum(f_files_opened);
    out.cache_hits = counters_.sum(f_cache_hits);

    // The shards are read one field at a time and a writer may be between two of
    // them, so a hit count can outrun the files it was counted against. Clamped
    // for the same reason probe_summary clamps: a rate above one is a reader
    // artefact and reporting it as a measurement would be worse than the skew.
    if (out.cache_hits > out.files_opened) out.cache_hits = out.files_opened;

    if (out.resolved_historical > 0) {
        auto const answered = double(out.resolved_historical);
        out.avg_probe_ordinal = double(counters_.sum(f_probe_ordinal_total)) / answered;
        out.avg_version_distance = double(counters_.sum(f_version_distance_total)) / answered;
    }
    for (size_t i = 0; i < bucket_count; ++i) {
        out.probe_ordinal_histogram[i] = counters_.sum(f_ordinal_bucket + i);
        out.version_distance_histogram[i] = counters_.sum(f_distance_bucket + i);
    }
    return out;
}

#endif // UTXOZ_STATISTICS_LEVEL >= 2

} // namespace utxoz

#endif // UTXOZ_STATISTICS_LEVEL >= 1
