// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file insert_transition.hpp
 * @brief When a generation has to be replaced, and what a failed insert left
 *        behind.
 *
 * ## Why the operating threshold was not enough
 *
 * The store rotates a generation when it holds `max_entries_for(bucket_count)`
 * entries — a fixed fraction of the capacity, chosen to sit below the point at
 * which Boost would grow the map. `capacity_policy.hpp` even asserts it:
 *
 *     static_assert(max_entries_for(959) < max_size_without_rehash(959), ...)
 *
 * That assertion is true of a *new* map and stops being true of a used one.
 * Boost's own threshold is not fixed: `foa/core.hpp` decrements it on erase —
 *
 *     size_ctrl.ml -= group_type::maybe_caused_overflow(pc);
 *
 * with the comment that this keeps the average probe length from drifting
 * upward over repeated insert/erase cycles. So on a workload that erases as
 * much as it inserts, the growth point walks down toward the operating
 * threshold and eventually below it. A real initial block download crossed that
 * line twice on the 44-byte class: 398.8 million inserts against 355.0 million
 * erases, with the two thresholds 688 128 entries apart to begin with.
 *
 * Under the line, the store believes it has room, Boost decides to grow, and the
 * segment — sized so that the map fills it — cannot hold a second bucket array.
 * The allocator throws.
 *
 * ## What this file provides
 *
 * The live threshold, so the decision is made against the map that exists rather
 * than the one it was when it was created; and, for the exceptional path that
 * remains, a way to say what state the map is in afterwards without trusting
 * that it is the state the container documents.
 */

#pragma once

#include <cstdint>

#include <utxoz/statistics.hpp>

#include "detail/capacity_policy.hpp"

namespace utxoz::detail {

/**
 * @brief The size at which the next insert of a key that is **not present**
 *        would make Boost grow the map.
 *
 * Derived from the source rather than from the documentation, because the
 * off-by-one decides whether the guard fires one insert early or one insert too
 * late. `foa/table.hpp::emplace_impl`:
 *
 *     if (BOOST_LIKELY(this->size_ctrl.size < this->size_ctrl.ml)) {
 *       unchecked_emplace_at(...)             // no growth
 *     } else {
 *       unchecked_emplace_with_rehash(...)    // grows
 *     }
 *
 * So growth happens at `size >= max_load()`, and a map holding one entry fewer
 * is still safe. The same shape as the operating threshold, which is why the two
 * combine by taking the smaller.
 *
 * `live_max_load` is `boost::unordered_flat_map::max_load()`, which is public
 * and `noexcept`.
 */
[[nodiscard]] constexpr uint64_t effective_insert_limit(uint64_t bucket_count,
                                                        uint64_t live_max_load) noexcept {
    uint64_t const operating = max_entries_for(bucket_count);
    return live_max_load < operating ? live_max_load : operating;
}

static_assert(effective_insert_limit(15728639, 13762559) == 13074431,
              "a fresh production map is governed by the operating threshold, which is "
              "the lower of the two while nothing has been erased");
static_assert(effective_insert_limit(15728639, 12000000) == 12000000,
              "and a drifted one by Boost's own, which is what the fixed threshold "
              "could not see");
static_assert(effective_insert_limit(959, 839) == 797, "the fixtures' smallest class");

/// What the map looked like at one moment. Three `noexcept` accessors, cheap
/// enough to read on the two paths that need them: the insert that is about to
/// rotate, and the one that has just caught an exception.
struct map_snapshot {
    uint64_t size = 0;
    uint64_t bucket_count = 0;
    uint64_t live_max_load = 0;

    [[nodiscard]] friend constexpr bool operator==(map_snapshot const&,
                                                   map_snapshot const&) = default;
};

/**
 * @brief What a `bad_alloc` left behind.
 *
 * Boost gives the strong guarantee for everything but the hash function — and
 * `outpoint_hash::operator()` is `noexcept`, so the carve-out cannot apply here.
 * `unchecked_emplace_with_rehash` allocates the new arrays *before* its `try`
 * block, deletes them and rethrows if the element cannot be constructed, and
 * increments the size only after the rehash has completed. The container should
 * therefore be exactly as it was.
 *
 * `map_mutated` is the case where it is not. That is not a full segment and not
 * a rotation the store can make good: it is the container contradicting its own
 * documented guarantee, and nothing on disk records what half-happened.
 */
enum class post_exception_state : uint8_t {
    map_intact,
    map_mutated,
};

/**
 * @brief Which of the two it was.
 *
 * A free function on two snapshots and a boolean, so the decision can be tested
 * directly. Reaching it through a real allocator failure would mean corrupting
 * Boost's internals to make the container lie, which is a fragile way to test a
 * comparison.
 *
 * `key_present_after` is asked only here. An insert does not look the key up
 * before emplacing — `emplace` is the lookup — so this is the one place the
 * question is worth a second probe.
 */
[[nodiscard]] constexpr post_exception_state classify_post_exception(
        map_snapshot const& before, map_snapshot const& after,
        bool key_present_after) noexcept {
    if (before != after) return post_exception_state::map_mutated;
    if (key_present_after) return post_exception_state::map_mutated;
    return post_exception_state::map_intact;
}

// All three figures, including the growth point. An earlier version left it out
// on the grounds that Boost is allowed to lower it — which it is, on erase. But
// an insert runs under the exclusive write claim, so nothing may erase while this
// one is in flight, and a `max_load` that moved across a throwing `emplace` is a
// map doing something no path in this store can account for. Excluding it would
// have made exactly one kind of contradiction invisible.
static_assert(classify_post_exception({10, 100, 87}, {10, 100, 86}, false)
                  == post_exception_state::map_mutated,
              "the growth point moved with no erase to explain it");
static_assert(classify_post_exception({10, 100, 87}, {11, 100, 87}, false)
                  == post_exception_state::map_mutated, "a size that moved");
static_assert(classify_post_exception({10, 100, 87}, {10, 200, 87}, false)
                  == post_exception_state::map_mutated, "a map that grew after all");
static_assert(classify_post_exception({10, 100, 87}, {10, 100, 87}, true)
                  == post_exception_state::map_mutated,
              "the key the insert threw on is in the map: it went in and reported failure");

/// Why a rotation was asked for. Only the two that complete one; a rotation that
/// fails is counted by its own field and has no cause to attribute.
enum class rotation_cause : uint8_t { preventive, capacity_exception };

[[nodiscard]] constexpr char const* to_string(rotation_cause c) noexcept {
    return c == rotation_cause::preventive ? "preventive" : "capacity_exception";
}

/// Reads the three figures a decision needs. Every accessor is `noexcept` and
/// O(1); `max_load()` is public on `boost::unordered_flat_map`.
template <typename Map>
[[nodiscard]] map_snapshot snapshot_of(Map const& map) noexcept {
    return map_snapshot{map.size(), map.bucket_count(), map.max_load()};
}

} // namespace utxoz::detail
