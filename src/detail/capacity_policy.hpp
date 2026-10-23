// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file capacity_policy.hpp
 * @brief How large a new segment is and how many buckets it starts with.
 * @internal
 *
 * One place decides both, because they are one decision. A file size without the
 * capacity it was chosen for is a number nobody can check, and a capacity without
 * the file size it needs is a rehash waiting for the entries to arrive.
 *
 * @par Why this is a table and not a search
 * It used to be a search: `find_optimal_buckets` bisected over bucket counts,
 * creating and destroying a segment of the full file size at each probe and
 * reading `bad_alloc` as the answer. The result was not a decision but whatever
 * survived the probes, and it was capped by a constant — 7864304 — that turned out
 * to be what actually limited container 0, not the memory.
 *
 * The values below were measured instead, by `tools/sizing.cpp`, on Linux x86_64,
 * macOS arm64 and Windows x64. The three agree on everything that scales with the
 * map; they differ by a fixed 48 and 56 bytes of `segment_manager`, which the
 * margin and the rounding absorb entirely. See doc/capacity-policy.md.
 *
 * @par Two profiles, and they are not the same question
 * Production fixes the capacity and derives the file size from it. Testing fixes
 * the file size at 10 MiB and derives the capacity that fits. Applying the
 * production margin to the test profile would drop container 0 from 122879 buckets
 * to 61439 — its floor there leaves 0.27% of slack — and every compatibility
 * fixture would change for a reason that has nothing to do with the format.
 */

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <utxoz/types.hpp>

#include "utxo_value.hpp"

namespace utxoz::detail {

/// One container's decision: what a new segment gets, and what it may hold.
struct capacity_entry {
    size_t file_size = 0;        ///< bytes a new segment is created with
    size_t capacity = 0;         ///< what the map constructor is asked for
    size_t bucket_count = 0;     ///< what Boost gives back for that capacity
    bool certified = false;      ///< measured and sized to it, or carried over

    /// Compared as a whole, so a field added here is a field the geometry
    /// assertion starts checking rather than one it silently ignores.
    friend constexpr bool operator==(capacity_entry const&, capacity_entry const&) = default;
};

/// The bucket counts `unordered_flat_map` actually uses: `15·2^k − 1`, each
/// growth doubling. Named so the table below cannot drift into asking for a
/// capacity that lands on a different step than the one it claims.
inline constexpr size_t bucket_step(unsigned k) { return (size_t(15) << k) - 1; }

/// The step above `n`, which is what a growth would move to.
inline constexpr size_t next_bucket_step(size_t n) { return n * 2 + 1; }

// =============================================================================
// Production
// =============================================================================

/**
 * @brief What a new production segment is created with.
 *
 * Container 0 is the one this policy was built for. The rest keep exactly the
 * file size and the capacity they had, so nothing about them changes here — but
 * they are written down rather than discovered, which is the point.
 *
 * `certified` says which is which. It is false for a container that was carried
 * over: the size is what it always was, and the measured recommendation for its
 * capacity may be larger. Class 96 is the case to know about — its map needs
 * 499 MiB inside a 500 MiB file, so it has about a megabyte of slack where the
 * measurement recommends 524 MiB. It cannot grow, so it is not unsafe, but it has
 * no margin either. PR B decides it.
 */
inline constexpr std::array<capacity_entry, container_count> production_capacity = {{
    // Container 0: measured, and sized to the measurement.
    //
    //   floor      1 337 987 188 bytes  (Linux, the largest of the three)
    //   × 1.05     1 404 886 548
    //   rounded    1 405 091 840 = 1340 MiB, a multiple of 4 KiB, 16 KiB,
    //                              64 KiB and 2 MiB
    //
    // At 15 728 639 buckets it rotates at 13 074 431 entries, twice what it held
    // before, in a file two thirds the size. The next step, 31 457 279, neither
    // fits alone nor can be reached by growing the map that is there — so a
    // rehash cannot complete even if the guard were bypassed.
    {file_sizes[0], 15728639, 15728639, true},

    // Carried over, unchanged. Measured recommendation is 524 MiB.
    {file_sizes[1], 3932159, 3932159, false},
    // Carried over, unchanged. Measured recommendation is 650 MiB.
    {file_sizes[2], 3932159, 3932159, false},
    // Carried over, unchanged. Measured recommendation is 37 MiB.
    {file_sizes[3], 122879, 122879, false},
    // Carried over, unchanged. Measured recommendation is 10 MiB.
    {file_sizes[4], 959, 959, false},
}};

/// Reference mode, carried over unchanged.
///
/// Its map is 368 MiB inside a 4 GiB file, and a growth **does** fit there — the
/// same live defect container 0 had, measured the same way. Left alone here on
/// purpose: it is a decision of its own and gets its own change.
inline constexpr capacity_entry production_reference = {
    reference_file_size, 7864319, 7864319, false};

// =============================================================================
// Testing
// =============================================================================

/**
 * @brief What a test segment is created with.
 *
 * Ten megabytes, and the largest step that fits in it — measured, not derived
 * from the production rule. These are exactly the counts the compatibility
 * fixtures were written with, and a formula that produced anything else would
 * change them without changing anything about the format.
 */
inline constexpr std::array<capacity_entry, container_count> testing_capacity = {{
    {test_file_sizes[0], 122879, 122879, true},   // floor 10 457 204 of 10 485 760
    {test_file_sizes[1],  61439,  61439, true},   // floor  8 179 780
    {test_file_sizes[2],  61439,  61439, true},   // floor 10 145 828
    {test_file_sizes[3],  30719,  30719, true},   // floor  9 007 012
    {test_file_sizes[4],    959,    959, true},   // floor  9 869 632
}};

inline constexpr capacity_entry testing_reference = {
    reference_test_file_size, 122879, 122879, true};    // floor 6 033 560

/// The table geometry 3 is, spelled out once so the assertion below can compare
/// against the whole of it rather than against a hand-written list of the fields
/// somebody remembered. Every field that decides what a new segment looks like is
/// here; add a field to `capacity_entry` and this stops compiling.
///
/// Restated in the project's own unit literals rather than in decimal bytes, and
/// not because it reads better: `size_t` is 32 bits on wasm32, where four
/// gibibytes is not a representable size and `reference_file_size` already wraps
/// to zero. Decimal literals would make this table disagree with the constants
/// there — a narrowing error, reported as a geometry mismatch, about something
/// else entirely. Written this way the comparison follows the target's `size_t`
/// exactly as the constants do, and still fails the build on any size that is
/// genuinely changed.
///
/// So read what the assertion says on such a target, and not more: the two sides
/// agree because both are the same truncated value, which is not evidence that
/// the size is still four gibibytes. Reference mode cannot create its production
/// segment where that constant is zero, and nothing here certifies that it can.
/// Issue #135 records the truncation and the options for deciding it.
inline constexpr std::array<capacity_entry, container_count> geometry_3_capacity = {{
    {1340_mib, 15728639, 15728639, true},
    { 500_mib,  3932159,  3932159, false},
    {   1_gib,  3932159,  3932159, false},
    {  50_mib,   122879,   122879, false},
    {  10_mib,      959,      959, false},
}};
inline constexpr capacity_entry geometry_3_reference = {4_gib, 7864319, 7864319, false};

// =============================================================================
// From a per-class table to a per-slot one
// =============================================================================
//
// A sharded class splits its buckets and its file among its shards. The
// arithmetic is exact and it is not a coincidence: Boost's ladder is
// `15·2^k − 1`, and
//
//     floor((15·2^k − 1) / 2^m) = 15·2^(k−m) − 1
//
// because the subtrahend is a fraction below one. So dividing a certified bucket
// count by a power-of-two shard count lands on another rung of the same ladder,
// never between two.
//
// What it does NOT divide evenly is the total: N shards of `15·2^(k−m) − 1`
// buckets come to `15·2^k − N`, which is **N−1 buckets short** of the unsharded
// count. With shard_count == 8 that is seven buckets, 595 bytes. It is recorded
// here rather than rounded away, because a figure that looks exact and is not is
// how a geometry assertion ends up asserting the wrong thing.

/// What happens to a class's file size when the class is split.
///
/// The two profiles answer different questions, and sharding does not change
/// which question each is asking. Production fixes the capacity and takes the
/// file size that was measured for it, so a shard holding a fraction of the
/// entries takes that fraction of the file.
///
/// Testing fixes the file size at ten megabytes and takes the capacity that
/// fits. Under `keep` a shard is still ten megabytes and simply holds fewer
/// entries; under `divide` the class is still ten megabytes and its shards
/// split it. `keep` is the default because it is the reading that keeps the
/// profile's sentence true per file, but it has a cost worth naming: the
/// class's aggregate capacity then *grows* with the shard count, so a suite run
/// under it never meets the constraint a sharded production class meets.
/// `divide` is the configuration that does, and
/// UTXOZ_TESTING_DIVIDE_FILE selects it.
enum class shard_file_rule {
    divide,   ///< the shard takes its share of the file (production)
    keep,     ///< the shard takes the whole file size (testing)
};

/// The per-shard entry of a class that is split `n` ways.
[[nodiscard]]
inline constexpr capacity_entry shard_entry(capacity_entry const& whole, uint32_t n,
                                            shard_file_rule rule) {
    return capacity_entry{
        rule == shard_file_rule::divide ? whole.file_size / n : whole.file_size,
        whole.capacity / n,
        whole.bucket_count / n,
        whole.certified,
    };
}

/// Expand a per-class table into the per-slot table the containers are built
/// from. With shard_count == 1 it is the identity, entry for entry.
template <size_t N>
[[nodiscard]]
inline constexpr auto to_slot_capacity(std::array<capacity_entry, N> const& by_class,
                                       shard_file_rule rule) {
    std::array<capacity_entry, shard_count + N - 1> by_slot{};
    for (uint32_t sh = 0; sh != shard_count; ++sh) {
        by_slot[sh] = shard_entry(by_class[0], shard_count, rule);
    }
    for (size_t cls = 1; cls != N; ++cls) {
        by_slot[shard_count + cls - 1] = by_class[cls];
    }
    return by_slot;
}

/// Boost's unordered_flat_map lays its elements out in groups of fifteen slots
/// with one sixteen-byte metadata word per group. So the slots of a map cost
/// `slots × sizeof(value_type)` plus `16` per group, and nothing here needs to
/// be measured to know it.
///
/// Deliberately **derived rather than measured**. An earlier version of this
/// file carried `85067·buckets/1000 + 4096`, fitted to `utxoz_sizing` runs on
/// this machine. The fit was good — it agrees with the expression below to six
/// digits, because `sizeof(value_type)` here is 84 and `84 + 16/15 = 85.0667` —
/// but a number obtained by fitting one platform's measurements is not a bound,
/// and asserting a format invariant against one is asserting that every other
/// platform's allocator behaves like this one's.
inline constexpr uint64_t segment_group_slots = 15;
inline constexpr uint64_t segment_group_metadata_bytes = 16;

/// Room for everything in a segment that is not the map's slots: the segment
/// manager's own bookkeeping, its named-object index, the stamp, and the
/// allocator's rounding.
///
/// This part is an **allowance, not a derivation**, and the distinction matters
/// for how much the assertions below are worth. The slot cost above follows from
/// `sizeof(value_type)` and a documented group layout, so it holds wherever those
/// hold. The fixed part depends on an allocator's internals, and no expression
/// here can bound it for every platform. Eight kilobytes is roughly four times
/// what this machine's segments use, which makes the assertions a cheap sanity
/// check and not a guarantee.
///
/// The guarantee is elsewhere, and deliberately: a file too small for the map it
/// is asked to hold must fail in a defined way at run time. That is what
/// `can_insert_safely` and the capacity-exception path are for, and what the
/// insufficient-space cases test.
inline constexpr uint64_t segment_fixed_allowance = 8u * 1024u;

/// A **diagnostic** estimate, not a portable bound. See the allowance above for
/// which half of it is derived and which half is a decision.
///
/// Saturating rather than wrapping. The widest rung this ladder reaches is
/// `15·2^39`, which multiplied by the largest class's 10 240 bytes is about
/// 8.4·10^16 — comfortable in a 64-bit `size_t` and not in a 32-bit one. Since
/// this runs in constant expressions, an overflow would be a compile error on
/// some platforms and a silently small answer on others, and a silently small
/// answer here says "it fits" about a file that does not. Saturation makes the
/// wrong answer the conservative one.
[[nodiscard]] inline constexpr uint64_t segment_bytes_needed(uint64_t slots,
                                                             uint64_t value_type_bytes) {
    constexpr auto limit = std::numeric_limits<uint64_t>::max();
    if (value_type_bytes != 0 && slots > limit / value_type_bytes) return limit;
    auto const slot_bytes = slots * value_type_bytes;

    auto const groups = slots / segment_group_slots + 1;
    if (groups > limit / segment_group_metadata_bytes) return limit;
    auto const group_bytes = groups * segment_group_metadata_bytes;

    if (slot_bytes > limit - group_bytes) return limit;
    auto const total = slot_bytes + group_bytes;
    if (total > limit - segment_fixed_allowance) return limit;
    return total + segment_fixed_allowance;
}

/// The widest rung of Boost's ladder (`15·2^k − 1`) whose slots fit in `bytes`.
///
/// This is how the testing profile picks a capacity under `divide`: the file
/// size is the fixed quantity and the rung follows from it, which is the same
/// sentence the unsharded testing profile has always been. Dividing the
/// certified rung instead would be the production rule wearing the testing
/// profile's numbers, and at thirty-two shards it does not fit.
/// The widest rung considered. **A limit of the model, not of the arithmetic**:
/// `15·2^39` slots of the smallest class is already some seven hundred terabytes,
/// far past anything this format describes, so searching further would be
/// answering a question nobody asks. `15 << 40` is perfectly representable in a
/// 64-bit unsigned — the stop is a choice about scope.
inline constexpr unsigned widest_rung_exponent = 39;

/// The widest rung of Boost's ladder (`15·2^k − 1`) whose slots fit in `bytes`.
///
/// Returns `size_t`, and the conversion is guarded rather than assumed: on a
/// platform where `size_t` is narrower than 64 bits a rung can exceed it, and a
/// silent truncation here would answer with a small number that reads as "this
/// fits" about a file it does not fit in.
[[nodiscard]] inline constexpr size_t widest_rung_within(uint64_t bytes,
                                                         uint64_t value_type_bytes) {
    uint64_t best = 0;
    for (unsigned k = 1; k <= widest_rung_exponent; ++k) {
        auto const rung = (uint64_t{15} << k) - 1;
        if (rung > std::numeric_limits<size_t>::max()) break;
        if (segment_bytes_needed(rung, value_type_bytes) > bytes) break;
        best = rung;
    }
    return static_cast<size_t>(best);
}

// The two edges worth pinning: nothing fits in nothing, and the search is
// bounded by the model rather than by where the shift would misbehave.
static_assert(widest_rung_within(0, 84) == 0,
              "no rung fits in no bytes");
static_assert((uint64_t{15} << widest_rung_exponent) - 1
                  < std::numeric_limits<uint64_t>::max() / 84,
              "the widest rung considered must still be multipliable by a class's "
              "value_type without saturating, or the estimate stops being an estimate");

/// The bytes one slot of the sharded class occupies in the map.
inline constexpr size_t sharded_value_type_bytes =
    sizeof(typename utxo_map<container_sizes[0]>::value_type);

/// The two profiles as the containers actually see them: one entry per slot.
/// Computed once, at compile time, so that nothing rebuilds a table on the way
/// into a member. With shard_count == 1 each is its per-class table unchanged.
inline constexpr auto production_slot_capacity =
    to_slot_capacity(production_capacity, shard_file_rule::divide);

#if defined(UTXOZ_TESTING_DIVIDE_FILE) && UTXOZ_TESTING_DIVIDE_FILE
inline constexpr bool testing_divides_file = true;
#else
inline constexpr bool testing_divides_file = false;
#endif

#if defined(UTXOZ_TESTING_RUNG_EXACT) && UTXOZ_TESTING_RUNG_EXACT
inline constexpr bool testing_rung_exact = true;
#else
inline constexpr bool testing_rung_exact = false;
#endif

static_assert( ! (testing_divides_file && testing_rung_exact),
              "UTXOZ_TESTING_DIVIDE_FILE and UTXOZ_TESTING_RUNG_EXACT are two different "
              "testing profiles; a build gets one");

/// The rung a shard gets under UTXOZ_TESTING_RUNG_EXACT: the N=1 rung divided by
/// N. On Boost's ladder by construction, since the N=1 rung is `15·2^k − 1` and
/// N is a power of two.
inline constexpr size_t rung_exact_bucket_count =
    (testing_capacity[0].bucket_count + 1) / shard_count - 1;

/// The testing profile's per-slot table.
///
/// Under `keep` the shards each take the whole ten megabytes and the class's
/// aggregate capacity grows with the shard count. Under `divide` they split one
/// file, and the rung is re-derived from the share rather than divided out of
/// the certified one — the testing profile's sentence is "this file size, and
/// the capacity that fits", and that sentence has to be answered again once the
/// file is smaller.
[[nodiscard]] inline constexpr auto make_testing_slot_capacity() {
    auto by_slot = to_slot_capacity(
        testing_capacity,
        testing_divides_file ? shard_file_rule::divide : shard_file_rule::keep);
    if (testing_divides_file && shard_count > 1) {
        auto const share = testing_capacity[0].file_size / shard_count;
        auto const rung = widest_rung_within(share, sharded_value_type_bytes);
        for (uint32_t sh = 0; sh != shard_count; ++sh) {
            by_slot[sh] = capacity_entry{share, rung, rung, false};
        }
    }
    // Experimental: the N=1 rung divided by N, in the smallest file the derived
    // bound says holds it. The file is computed from the rung, not the other way
    // round, so no shard count loses a rung to the rounding of a fixed size.
    if (testing_rung_exact && shard_count > 1) {
        auto const rung = rung_exact_bucket_count;
        auto const file = static_cast<size_t>(
            segment_bytes_needed(rung, sharded_value_type_bytes));
        for (uint32_t sh = 0; sh != shard_count; ++sh) {
            by_slot[sh] = capacity_entry{file, rung, rung, false};
        }
    }
    return by_slot;
}

inline constexpr auto testing_slot_capacity = make_testing_slot_capacity();

// Every shard of the sharded class must have room for the slots it will be
// built with, at every shard count this build admits. Checked against the
// derived bound above, so this is a statement about the layout rather than
// about one machine's measurements.
static_assert(production_slot_capacity[0].file_size
                  >= segment_bytes_needed(production_slot_capacity[0].bucket_count,
                                          sharded_value_type_bytes),
              "a production shard's file is smaller than the slots it is built with");
static_assert(testing_slot_capacity[0].bucket_count == 0
                  || testing_slot_capacity[0].file_size
                         >= segment_bytes_needed(testing_slot_capacity[0].bucket_count,
                                                 sharded_value_type_bytes),
              "a testing shard's file is smaller than the slots it is built with");
static_assert(testing_slot_capacity[0].bucket_count != 0,
              "the testing profile found no bucket count that fits one shard's share of "
              "its file; UTXOZ_TESTING_DIVIDE_FILE cannot be used at this shard count");

// The rung-exact profile, checked in both directions against the same derived
// bound: the rung is on the ladder, the file holds it, and it is the widest rung
// that file holds - so the file is the smallest for that rung, not merely one
// that happens to fit.
static_assert( ! testing_rung_exact || shard_count == 1 || [] {
    auto const rung = rung_exact_bucket_count;
    if ((rung + 1) % 15 != 0) return false;
    auto const groups = (rung + 1) / 15;
    if ((groups & (groups - 1)) != 0) return false;
    auto const& e = testing_slot_capacity[0];
    return e.bucket_count == rung
        && e.file_size >= segment_bytes_needed(rung, sharded_value_type_bytes)
        && widest_rung_within(e.file_size, sharded_value_type_bytes) == rung;
}(), "the rung-exact testing profile must give each shard the N=1 rung divided by N, "
     "in the smallest file that holds it");
static_assert( ! testing_rung_exact || shard_count != 1
                   || testing_slot_capacity[0] == testing_capacity[0],
              "at N=1 the rung-exact profile must be the testing profile, unchanged");

static_assert(production_slot_capacity.size() == shard_count + container_count - 1,
              "the per-slot table must have exactly one entry per slot");
static_assert(shard_count != 1 || production_slot_capacity[0].capacity
                                  == production_capacity[0].capacity,
              "with one shard the per-slot table must be the per-class table, entry for entry");

// The ladder property, checked rather than asserted in prose: every shard count
// this format admits must divide the sharded class onto a real rung.
static_assert([] {
    for (uint32_t n : {1u, 2u, 4u, 8u, 16u, 32u}) {
        auto const per_shard = geometry_3_capacity[0].bucket_count / n;
        bool on_ladder = false;
        for (size_t k = 1; k != 40; ++k) {
            if (per_shard == (size_t{15} << k) - 1) { on_ladder = true; break; }
        }
        if ( ! on_ladder) return false;
    }
    return true;
}(), "dividing the sharded class by a shard count must land on Boost's bucket ladder");

// And the shortfall is exactly N-1, which is what the design says it is.
static_assert(geometry_3_capacity[0].bucket_count
              - 8 * (geometry_3_capacity[0].bucket_count / 8) == 7,
              "eight shards lose seven buckets; if this changes the design note is stale");

// =============================================================================
// When a container rotates
// =============================================================================

/**
 * @brief The load at which the store makes a new generation, as a fraction.
 *
 * `unordered_flat_map` grows when the size reaches `0.875 × bucket_count`. The
 * store rotates five per cent below that, so the growth never happens: 0.875 ×
 * 0.95 = 0.83125 = 133/160 exactly.
 *
 * Integers rather than a float. Above 2^24 a float cannot represent a bucket
 * count exactly — reference mode is already past that — and while the error is
 * far smaller than the margin, a threshold that decides whether a file rotates
 * should not be computed in a type that rounds.
 */
inline constexpr uint64_t rotation_numerator = 133;
inline constexpr uint64_t rotation_denominator = 160;

static_assert(rotation_numerator * 1000 / rotation_denominator == 831,
              "the rotation fraction is 0.875 × 0.95; if either moves, this moves");

/**
 * @brief The most entries a map may hold before Boost grows it.
 *
 * `unordered_flat_map`'s maximum load factor is 7/8, so a map with this many
 * entries keeps its bucket count and one more entry does not. Integers: `7 * n /
 * 8` is exact, and a float comparison at the boundary is a coin toss decided by
 * rounding.
 *
 * Not a universal truth: it is a certified property of the Boost version and map
 * layout this project supports, and it is certified by measurement.
 * `test_capacity_policy.cpp` inserts into real maps and finds the growth point at
 * every step this project uses, checking that the first size which rehashes is
 * exactly one past this. Those tests are the authority; this function is the
 * shorthand. A Boost whose load factor moved would fail them, which is what
 * `map_layout_epoch` and the compatibility fixtures exist to notice.
 */
inline constexpr uint64_t max_size_without_rehash(uint64_t bucket_count) {
    // Divided before multiplying, so nothing overflows however large the bucket
    // count grows; the remainder term keeps it exactly `floor(7n/8)`.
    return bucket_count / 8 * 7 + (bucket_count % 8) * 7 / 8;
}

/**
 * @brief The most entries a *live* container holds before the store rotates it.
 *
 * Five per cent below the growth point, which is operating reserve: a container
 * that is still receiving inserts should make a new generation with room to
 * spare rather than at the last possible entry.
 *
 * Inclusive: a map holding exactly this many is full and the next insert
 * rotates. Said once, in one place, because the alternative is an off-by-one
 * between "13 074 431 entries are allowed" and "rotate before inserting the
 * 13 074 432nd", which are the same sentence only if somebody checks.
 *
 * Taken from the bucket count of the map that is open, never from the table
 * above: the policy decides what a new segment is created with, and the file
 * decides how it is operated.
 *
 * A sealed compaction target does not need that reserve — it is built once and
 * not inserted into — so compaction uses `max_size_without_rehash` instead. The
 * two numbers answer different questions and the difference is the reserve.
 */
inline constexpr uint64_t max_entries_for(uint64_t bucket_count) {
    // bucket_count is at most a few billion and the numerator is 133, so the
    // product cannot overflow 64 bits by any margin worth guarding.
    return bucket_count * rotation_numerator / rotation_denominator;
}

static_assert(max_entries_for(15728639) == 13074431,
              "container 0's production threshold is part of the policy, not a "
              "consequence of it that nobody looked at");
static_assert(max_entries_for(7864319) == 6537215,
              "and a generation written under the previous policy keeps its own");
static_assert(max_entries_for(122879) == 102143, "the fixtures' container 0");

static_assert(max_size_without_rehash(15728639) == 13762559);
static_assert(max_size_without_rehash(122879) == 107519);
static_assert(max_size_without_rehash(959) == 839);
static_assert(max_entries_for(959) < max_size_without_rehash(959),
              "the operating threshold is below the growth point, or the store would "
              "rotate after Boost had already grown the map");

/// The exact decisions, spelled out. Not "at least": these are the numbers a new
/// production generation of container 0 gets, and a change to either of them is a
/// change to what the store writes.
static_assert(production_capacity[0].file_size == 1405091840,
              "container 0's segment is 1340 MiB — the measured floor plus five per "
              "cent, rounded up to a whole mebibyte");
static_assert(production_capacity[0].bucket_count == 15728639);
static_assert(production_capacity[0].bucket_count == bucket_step(20));
static_assert(next_bucket_step(production_capacity[0].bucket_count) == 31457279);
static_assert(production_capacity[0].file_size % (2u * 1024 * 1024) == 0,
              "and it is a multiple of every page and mapping granularity in use");

// =============================================================================
// Whether one ever happened
// =============================================================================

/**
 * @brief How many times a map grew underneath an insert, since this process
 *        started.
 *
 * Always compiled, not behind the statistics switch, because a rehash is not a
 * statistic. The store's whole answer to a container filling up is to rotate and
 * later compact; a map that grew instead means the rotation guard did not do its
 * job, and that has to be observable in the build an operator actually runs —
 * including the one built without statistics.
 *
 * It costs one comparison of a value already in cache, next to a hash-table
 * insert. Expected to stay at zero for the life of a process: anything else is a
 * defect, and the tests treat it as one.
 */
inline std::atomic<uint64_t> rehashes_observed{0};

/// What a generation opened with, and what has already been reported.
///
/// Two states, because they answer different questions. `at_open` is the baseline
/// the invariant is about and never moves, so every report can name where the
/// generation started rather than where the previous growth left it.
/// `last_reported` is the deduplication: without it the same growth is reported
/// again on every insert that follows, turning one defect into a number that
/// measures how much was written afterwards.
///
/// A second, different growth is a second violation and is reported again.
/// Both are reset when a generation is opened or created.
struct rehash_watch {
    size_t at_open = 0;
    size_t last_reported = 0;

    void reset(size_t buckets) {
        at_open = buckets;
        last_reported = buckets;
    }
};

/// Reports a generation whose bucket count moved, and says whether it did.
///
/// Deliberately not an error the caller can act on. It is called after the insert
/// has happened, so turning it into a failure would invite a retry, and a retry
/// would write the entry a second time. The entry is there; what is wrong is that
/// the map grew, and the honest response is to say so loudly and keep the data.
///
/// In a build with assertions it stops the program, because a store that has
/// begun rehashing under itself is not in a state anyone should keep writing to.
bool note_rehash_if_grown(uint32_t container_kind, rehash_watch& watch, size_t now);

/// The same detection without the assertion, so the counting semantics can be
/// tested in a build that has one. Separating them is the point: "did it grow"
/// and "stop the program" are different decisions, and only the first is
/// observable from a test.
bool detect_rehash(uint32_t container_kind, rehash_watch& watch, size_t now);

} // namespace utxoz::detail
