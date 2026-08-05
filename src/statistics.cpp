// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file statistics.cpp
 * @brief Statistics implementation
 */

#include <utxoz/statistics.hpp>

#include <algorithm>
#include <numeric>
#include <thread>

namespace utxoz {
namespace {

/**
 * @brief The shard this thread records into.
 *
 * Assigned round-robin on first use and then cached, so recording never hashes.
 * The counter is process-wide, which is what we want: it spreads writers across
 * slots regardless of how many database instances they touch. Slots are reused
 * once more threads have recorded than there are slots, so a distinct slot per
 * thread is the common case, not a guarantee — the counters are atomic either
 * way.
 */
size_t thread_shard_index() {
    static std::atomic<size_t> next_index{0};
    static thread_local size_t const index =
        next_index.fetch_add(1, std::memory_order_relaxed) % search_stats::shard_count;
    return index;
}

} // anonymous namespace

void search_stats::add_record(uint32_t access_height, uint32_t insertion_height,
                              uint32_t depth, bool cache_hit, bool found,
                              [[maybe_unused]] char operation) {
    // Relaxed throughout: these counters carry no ordering relationship with
    // anything else, and a summary is allowed to be a slightly stale snapshot.
    auto& s = shards_[thread_shard_index()];

    s.operations.fetch_add(1, std::memory_order_relaxed);
    s.total_depth.fetch_add(depth, std::memory_order_relaxed);

    if (found) {
        s.found.fetch_add(1, std::memory_order_relaxed);

        uint64_t const age = access_height >= insertion_height
            ? uint64_t(access_height) - insertion_height
            : 0;
        s.total_age.fetch_add(age, std::memory_order_relaxed);

        if (depth == 0) {
            s.current_version_hits.fetch_add(1, std::memory_order_relaxed);
        }
    }

    if (depth > 0) {
        s.cache_accesses.fetch_add(1, std::memory_order_relaxed);
        if (cache_hit) {
            s.cache_hits.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

void search_stats::reset() {
    for (auto& s : shards_) {
        s.operations.store(0, std::memory_order_relaxed);
        s.found.store(0, std::memory_order_relaxed);
        s.current_version_hits.store(0, std::memory_order_relaxed);
        s.total_depth.store(0, std::memory_order_relaxed);
        s.total_age.store(0, std::memory_order_relaxed);
        s.cache_accesses.store(0, std::memory_order_relaxed);
        s.cache_hits.store(0, std::memory_order_relaxed);
    }
}

search_summary search_stats::get_summary() const {
    uint64_t operations = 0;
    uint64_t found = 0;
    uint64_t current_version_hits = 0;
    uint64_t total_depth = 0;
    uint64_t total_age = 0;
    uint64_t cache_accesses = 0;
    uint64_t cache_hits = 0;

    for (auto const& s : shards_) {
        operations += s.operations.load(std::memory_order_relaxed);
        found += s.found.load(std::memory_order_relaxed);
        current_version_hits += s.current_version_hits.load(std::memory_order_relaxed);
        total_depth += s.total_depth.load(std::memory_order_relaxed);
        total_age += s.total_age.load(std::memory_order_relaxed);
        cache_accesses += s.cache_accesses.load(std::memory_order_relaxed);
        cache_hits += s.cache_hits.load(std::memory_order_relaxed);
    }

    search_summary summary;
    if (operations == 0) {
        return summary;
    }

    summary.total_operations = size_t(operations);
    summary.found_operations = size_t(found);
    summary.current_version_hits = size_t(current_version_hits);
    summary.cache_hits = size_t(cache_hits);

    // The counters are read one at a time while recorders may be running, so a
    // numerator can sit an increment ahead of its denominator. Clamp, so a
    // summary taken mid-flight never reports a rate above 1.
    summary.hit_rate = double(std::min(found, operations)) / double(operations);
    summary.avg_depth = double(total_depth) / double(operations);

    if (found > 0) {
        summary.avg_utxo_age = double(total_age) / double(found);
    }

    if (cache_accesses > 0) {
        summary.cache_hit_rate = double(std::min(cache_hits, cache_accesses)) / double(cache_accesses);
    }

    return summary;
}

} // namespace utxoz
