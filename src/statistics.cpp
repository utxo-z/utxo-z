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

#ifdef UTXOZ_STATISTICS_ENABLED

namespace utxoz {
namespace {

/**
 * @brief The shard this thread records into.
 *
 * Assigned round-robin on first use and then cached, so recording never hashes.
 * The counter is process-wide, which is what we want: it spreads writers across
 * slots regardless of how many database instances they touch.
 */
size_t thread_shard_index() {
    static std::atomic<size_t> next_index{0};
    static thread_local size_t const index =
        next_index.fetch_add(1, std::memory_order_relaxed) % detail::sharded_counters::shard_count;
    return index;
}

} // anonymous namespace

namespace detail {

// Relaxed throughout: these counters carry no ordering relationship with
// anything else, and a summary is allowed to be a slightly stale snapshot.

void sharded_counters::add(size_t field, uint64_t amount) noexcept {
    shards_[thread_shard_index()].fields[field].fetch_add(amount, std::memory_order_relaxed);
}

uint64_t sharded_counters::sum(size_t field) const noexcept {
    uint64_t total = 0;
    for (auto const& s : shards_) {
        total += s.fields[field].load(std::memory_order_relaxed);
    }
    return total;
}

void sharded_counters::reset() noexcept {
    for (auto& s : shards_) {
        for (auto& f : s.fields) {
            f.store(0, std::memory_order_relaxed);
        }
    }
}

} // namespace detail

// =============================================================================
// probe_stats
// =============================================================================

void probe_stats::record_answered(uint32_t access_height, uint32_t creation_height) noexcept {
    counters_.add(f_probes, 1);
    counters_.add(f_answered, 1);

    uint64_t const age = access_height >= creation_height
        ? uint64_t(access_height) - creation_height
        : 0;
    counters_.add(f_age_total, age);
}

void probe_stats::record_deferred() noexcept {
    counters_.add(f_probes, 1);
}

void probe_stats::reset() noexcept {
    counters_.reset();
}

probe_summary probe_stats::get_summary() const noexcept {
    uint64_t const probes = counters_.sum(f_probes);
    uint64_t const answered = counters_.sum(f_answered);
    uint64_t const age_total = counters_.sum(f_age_total);

    probe_summary summary;
    if (probes == 0) return summary;

    summary.probes = size_t(probes);
    // Fields are read one at a time while recorders may be running, so a
    // numerator can sit an increment ahead of its denominator. Clamp, so a
    // summary taken mid-flight never reports a rate above 1 or a negative count.
    uint64_t const answered_capped = std::min(answered, probes);
    summary.answered_from_active = size_t(answered_capped);
    summary.deferred = size_t(probes - answered_capped);
    summary.active_map_hit_rate = double(answered_capped) / double(probes);

    if (answered_capped > 0) {
        summary.avg_age_answered = double(age_total) / double(answered_capped);
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

void resolution_stats::record_unresolved(size_t count) noexcept {
    if (count == 0) return;
    counters_.add(f_unresolved, uint64_t(count));
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
    uint64_t const unresolved = counters_.sum(f_unresolved);
    uint64_t const depth_total = counters_.sum(f_depth_total);
    uint64_t const files = counters_.sum(f_files);
    uint64_t const cache_hits = counters_.sum(f_cache_hits);

    resolution_summary summary;
    summary.resolved = size_t(resolved);
    summary.unresolved = size_t(unresolved);
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

} // namespace utxoz

#endif // UTXOZ_STATISTICS_ENABLED
