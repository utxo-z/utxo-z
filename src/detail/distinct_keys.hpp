// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file distinct_keys.hpp
 * @brief Finding every copy of every outpoint, exactly, inside a stated budget.
 *
 * The engine underneath `verify_unique_outpoints()`. It answers one question —
 * which keys are stored more than once, and where — and does it without a set of
 * every key and without a temporary file.
 *
 * ## Exact, and bounded, and without a temporary file
 *
 * The obvious approach is a set of every key, which is a set nobody sized: two
 * hundred million keys is gigabytes, and "it is an offline tool" is not a bound.
 * The next approach is to write partitions to temporary files, which is bounded
 * but brings a directory to configure, space to estimate, and residue to clean up
 * after every way of failing.
 *
 * The generations are already memory-mapped and can be read again, so neither is
 * necessary. The data is walked once per partition group, keeping only the keys
 * whose partition hash falls in that group:
 *
 *   1. a planning pass counts entries per prefix — counters only, no records;
 *   2. prefixes are grouped so that each group's records fit the budget, using the
 *      counts just observed rather than an estimate;
 *   3. each group is walked, collected, sorted by key and grouped.
 *
 * The prefix width is **fixed at twelve bits — 4096 prefixes — and there is no
 * subdivision**. A single prefix whose entries do not fit the budget is refused,
 * conservatively: the supported answer is a larger budget, and the refusal says
 * how large. Narrower prefixes would separate distinct keys that happen to share
 * one, and might well help — but they would not help the case that cannot be
 * helped, since every copy of one key hashes alike and can never be split, and
 * building the recursion before a real database shows the skew is a problem is
 * building for a problem nobody has seen.
 *
 * Nothing is written. There is no temporary directory, no cleanup path, and no
 * way to leave residue behind, because nothing is ever created.
 *
 * **The cost is re-reading.** Three groups mean roughly three passes over every
 * segment, plus the planning pass. With a warm page cache that is memory
 * bandwidth; with a cold one it is that many times the database's worth of I/O.
 * The report says how many passes it did rather than leaving it to be inferred.
 *
 * ## The partition hash is ours
 *
 * Stable, defined here, and deliberately not Boost's: the map's hash is part of
 * the persisted layout and is pinned by `hash_epoch`, so borrowing it would make
 * this walk change with a decision that has nothing to do with it.
 *
 * All copies of one key hash the same and therefore land in the same group. That
 * is what makes the count exact — and it is also the one thing subdivision cannot
 * fix: a single key with more copies than fit the budget is refused, not split.
 *
 * **A hash collision is never equality.** The hash chooses the group; membership
 * of a group is decided by comparing all thirty-six bytes.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <span>
#include <vector>

#include <utxoz/types.hpp>
#include <utxoz/uniqueness.hpp>

#include "detail/budget_meter.hpp"

namespace utxoz::detail {

/// One stored copy of one key: where it is, and nothing about what is in it.
/// Comparing contents needs a second look at the entry and is not this walk's
/// question.
///
/// The generation leads so that the key's thirty-six bytes are not followed by
/// padding. `sizeof` is asserted where the budget arithmetic uses it, because
/// that arithmetic is built on it.
///
/// This is also what a duplicate group is handed to the caller as — a view over
/// the records already in memory. An earlier version copied each group into a
/// second vector, which was a second allocation proportional to the data and
/// outside the budget the report promised: sixteen bytes against fifty-six is
/// twenty-nine per cent in the worst case, against ten per cent of headroom. A
/// view has no worst case, and `duplicate_group` is shaped so that a copy would
/// have to be metered.
struct stored_copy {
    uint64_t generation = 0;
    raw_outpoint key{};
    uint32_t container_class = 0;
    bool active = false;            ///< the generation inserts currently go to
};

/// What a run was allowed and what it actually took. Reported so that "it fit"
/// is a figure rather than an assurance.
struct budget_report {
    uint64_t memory_budget = 0;      ///< what the caller allowed
    uint64_t record_bytes = 0;       ///< sizeof of one collected record
    uint64_t record_capacity = 0;    ///< elements the vector actually reserved
    uint64_t fixed_overhead = 0;     ///< counters, result structures, everything not records
    uint64_t headroom = 0;           ///< deliberately unused
    uint64_t estimated_peak = 0;     ///< records × capacity + overhead + headroom

    uint64_t planning_passes = 0;    ///< passes that only counted
    uint64_t data_passes = 0;        ///< passes that collected records
    uint64_t partitions = 0;         ///< groups the prefixes were gathered into
    uint64_t entries_examined_total = 0;  ///< including every re-read
    /// Generations visited, counting every re-read. Not segment opens — the active
    /// generation is already mapped.
    uint64_t generations_visited = 0;
    uint64_t duration_ms = 0;
    std::vector<uint64_t> duration_per_pass_ms;
};

/// How many prefixes the planning pass counts. Twelve bits: 32 KB of counters,
/// and fine enough that grouping them has room to balance. Named here rather
/// than inside the walk because the budget's composition is checked against it.
inline constexpr size_t planning_prefix_bits = 12;
inline constexpr size_t planning_prefixes = size_t(1) << planning_prefix_bits;
/// The part of a budget the meter is never allowed to hand out.
///
/// One definition because two sides depend on it agreeing: the ceiling the meter
/// is built with is the budget without this, and the figure the report calls
/// `headroom` is this. Written out twice, in two functions, they would eventually
/// stop matching and the report would describe a ceiling that was never enforced.
[[nodiscard]] constexpr uint64_t headroom_of(uint64_t budget) noexcept {
    return budget / 10;
}

/// What the sort itself may take. std::sort is introsort, which recurses
/// O(log n) deep and allocates no buffer proportional to the input the way
/// stable_sort would — but "no allocation at all" is not a portable promise, so
/// an allowance is taken from the meter up front rather than hoped for.
inline constexpr uint64_t sort_allowance = 1u * 1024 * 1024;

/// How to run the walk. The budget knobs of `verify_options`, and nothing about
/// reporting: what the walk finds is the caller's to describe.
struct distinct_walk_options {
    /// The ceiling this run holds itself to.
    uint64_t memory_budget = 256u * 1024 * 1024;
    /// Refused before the expensive work rather than discovered at pass forty.
    uint64_t max_data_passes = 64;
};

/// A contiguous run of planning prefixes, walked together. Part of what the budget
/// holds, which is why it is here rather than private to the walk.
struct partition_group {
    uint64_t first = 0;    ///< inclusive, at `planning_prefix_bits`
    uint64_t last = 0;     ///< inclusive
    uint64_t records = 0;  ///< what the planning pass counted for it
};

/// One sampled finding while it is being collected.
///
/// Flat, and the locations live in a second vector addressed by offset, so the
/// whole sample is two allocations this allocator can see rather than a vector of
/// vectors whose growth nobody can account for. It is converted into the public
/// `duplicate_finding` after the walk, when the records are no longer held.
struct sampled_finding {
    raw_outpoint key{};
    uint64_t multiplicity = 0;
    uint64_t locations_omitted = 0;
    uint64_t first_location = 0;
    uint64_t location_count = 0;
};

/**
 * @brief Every copy of one key, as a view over storage that is already metered.
 *
 * The only way to build one is from a `counted_vector`, and that is the point.
 * The bug this shape forecloses is a copy of the group into a container beside
 * the records — which is an allocation proportional to the data, and which the
 * budget would not have seen. Now there is nothing to hand over that was not
 * taken from the meter first.
 *
 * It is not a sandbox: code that reaches for the global allocator is outside any
 * budget this process can impose. What changed is which one the code invites.
 */
class duplicate_group {
public:
    [[nodiscard]] std::span<stored_copy const> copies() const noexcept { return copies_; }

    /// Passed because a caller may need to look the entry up again. It is not,
    /// and must not be, put in any report unless the caller was asked for it.
    [[nodiscard]] raw_outpoint const& key() const noexcept { return copies_.front().key; }

private:
    friend struct database_impl;
    duplicate_group(counted_vector<stored_copy> const& held, size_t first, size_t count) noexcept
        : copies_(held.data() + first, count) {}

    std::span<stored_copy const> copies_;
};

/// `on_group` is called once per distinct key that has **more than one** copy,
/// with every copy of it. `on_distinct` is called once per distinct key with its
/// multiplicity, which is what the counting needs.
///
/// Deterministic: the result does not depend on the order generations are walked
/// in, on the file cache, or on anything else that varies between runs. Copies
/// within a group are sorted before they are handed over.
using duplicate_group_callback = std::function<void(duplicate_group const&)>;
using distinct_key_callback = std::function<void(uint64_t multiplicity)>;

/**
 * @brief The partition hash: ours, stable, and not the map's.
 *
 * A 64-bit FNV-1a over all thirty-six bytes, finalised with a mixer so that the
 * high bits — which is where the prefix is taken from — depend on every byte.
 * FNV alone leaves the top bits sluggish, and a prefix taken from sluggish bits
 * is a partition scheme that concentrates rather than spreads.
 *
 * Defined here rather than borrowed from the map for one reason: the map's hash
 * is part of what `hash_epoch` pins, and this walk must not change because a
 * decision about the persisted format did.
 */
[[nodiscard]] constexpr uint64_t partition_hash(raw_outpoint const& key) noexcept {
    uint64_t h = 1469598103934665603ull;              // FNV-1a offset basis
    for (auto const byte : key) {
        h ^= static_cast<uint64_t>(byte);
        h *= 1099511628211ull;                        // FNV-1a prime
    }
    // splitmix64's finaliser: cheap, and it makes every output bit depend on
    // every input byte, which is what taking a prefix requires.
    h ^= h >> 30; h *= 0xbf58476d1ce4e5b9ull;
    h ^= h >> 27; h *= 0x94d049bb133111ebull;
    h ^= h >> 31;
    return h;
}

/**
 * @brief Can a sample of this shape be indexed where `size_t` tops out at
 *        `addressable`?
 *
 * The counts are `uint64_t` and `vector::reserve` takes a `size_t`. On a 32-bit
 * target — wasm32 is the one this builds for — a count above 2^32 narrows on the
 * way in, and `reserve` then succeeds at a size nobody asked for.
 *
 * The width is a parameter rather than `sizeof(size_t)` for one reason: the only
 * platform where the guard can fire is the one where no test runs. The wasm job
 * builds the library and does not run `ctest`, and this suite could not run there
 * anyway — it needs a real filesystem and memory-mapped files. A predicate that
 * takes the width can be driven from Linux with a 32-bit one, so the logic is
 * exercised on a platform that executes tests, and the wiring — passing the real
 * `size_t` maximum — is a single expression at the call site.
 *
 * The boundary is also pinned by `static_assert` below, which is evaluated by
 * every build including the wasm32 one. That is the part of this that the
 * 32-bit toolchain does check.
 */
[[nodiscard]] constexpr bool count_fits_addressable(uint64_t count,
                                                    uint64_t addressable) noexcept {
    return count <= addressable;
}

[[nodiscard]] constexpr bool sample_fits_addressable(uint64_t findings,
                                                     uint64_t locations,
                                                     uint64_t addressable) noexcept {
    return count_fits_addressable(findings, addressable)
        && count_fits_addressable(locations, addressable);
}

/// What production calls. These take no width, so no call site can pass the wrong
/// one — the freedom that a parameter leaves open is removed rather than reviewed.
/// The parameterised forms above stay, as the primitives the tests drive.
[[nodiscard]] constexpr bool count_fits_platform(uint64_t count) noexcept {
    return count_fits_addressable(count, uint64_t(std::numeric_limits<size_t>::max()));
}

[[nodiscard]] constexpr bool sample_fits_platform(uint64_t findings,
                                                  uint64_t locations) noexcept {
    return sample_fits_addressable(findings, locations,
                                   uint64_t(std::numeric_limits<size_t>::max()));
}

// A 32-bit platform's boundary, from both sides, and on either count.
static_assert(sample_fits_addressable(0xFFFFFFFFull, 0xFFFFFFFFull, 0xFFFFFFFFull),
              "exactly the maximum has to fit; refusing it would refuse a legal request");
static_assert( ! sample_fits_addressable(0x100000000ull, 1, 0xFFFFFFFFull),
              "one past the maximum must be refused rather than narrowed");
static_assert( ! sample_fits_addressable(1, 0x100000000ull, 0xFFFFFFFFull),
              "the locations count is narrowed by the same cast and needs the same guard");

// And what production actually calls, on the platform where it can fire. A build
// that wired production to a 64-bit width would fail here — on wasm32, which runs
// no tests but does compile this.
static_assert(count_fits_addressable(0xFFFFFFFFull, 0xFFFFFFFFull),
              "exactly the maximum has to fit here too");
static_assert( ! count_fits_addressable(0x100000000ull, 0xFFFFFFFFull),
              "a record count one past the maximum must be refused, not narrowed");

#if SIZE_MAX < UINT64_MAX
static_assert( ! sample_fits_platform(uint64_t(SIZE_MAX) + 1, 0),
              "production must refuse a findings count this platform cannot index");
static_assert( ! sample_fits_platform(0, uint64_t(SIZE_MAX) + 1),
              "production must refuse a locations count this platform cannot index");
static_assert( ! count_fits_platform(uint64_t(SIZE_MAX) + 1),
              "production must refuse a record count this platform cannot index");
#endif

/**
 * @brief Would a group of this many records fit, with everything else held?
 *
 * Asked of every group **before the first pass**, so a database that cannot be
 * walked at this budget is refused before any of the work rather than after some
 * of it. The meter would refuse the reservation anyway; what this adds is when,
 * and a message that names the budget it would take.
 *
 * Refuses on overflow as well: a capacity times a record size is a
 * multiplication on numbers derived from a database, and one that wrapped would
 * produce a small figure and a cheerful answer.
 */
[[nodiscard]] bool fits_in_budget(uint64_t capacity, uint64_t record_bytes,
                                  uint64_t fixed_overhead, uint64_t headroom,
                                  uint64_t budget, uint64_t& peak) noexcept;

// The walk itself is `database_impl::walk_distinct_keys`, declared with the rest
// of that class: it needs the catalogues and the open segments, and reaching them
// through an accessor written for this would be a second way into the same state.

} // namespace utxoz::detail
