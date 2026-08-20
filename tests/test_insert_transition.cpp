// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_insert_transition.cpp
 * @brief When a generation is replaced, and what a failed insert leaves behind.
 *
 * The store used to decide when a map was full from a fixed fraction of its
 * capacity. Boost decides from a threshold of its own that **moves**: it lowers
 * it on erase so that probe lengths do not drift upward over repeated
 * insert/erase cycles. On a workload that erases nearly as much as it inserts,
 * Boost's threshold walks below the store's, and then the store believes it has
 * room while Boost has already decided to grow — into a segment sized so that
 * the map fills it. A real initial block download reached that state twice.
 *
 * The first case here reproduces the drift itself, on a real map, without any
 * failpoint: it is the evidence for everything the rest of the file assumes. The
 * failpoint appears only where the exceptional path is exercised.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>
#include <utxoz/logging.hpp>
#include <utxoz/statistics.hpp>
#include <utxoz/types.hpp>

#include "detail/capacity_policy.hpp"
#include "detail/durability.hpp"
#include "detail/file_cache.hpp"
#include "detail/insert_transition.hpp"
#include "detail/segment_open.hpp"
#include "detail/utxo_value.hpp"

namespace fs = std::filesystem;
using namespace utxoz;
using utxoz::detail::failpoints;
using utxoz::detail::classify_post_exception;
using utxoz::detail::effective_insert_limit;
using utxoz::detail::free_bytes_best_effort;
using utxoz::detail::free_bytes_display;
using utxoz::detail::map_snapshot;
using utxoz::detail::max_entries_for;
using utxoz::detail::max_size_without_rehash;
using utxoz::detail::post_exception_state;

namespace {

struct temp_db {
    temp_db() {
        static std::atomic<uint64_t> counter{0};
        auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        dir = fmt::format("./test_transition_{}_{}_{}", getpid(), ts, counter.fetch_add(1));
        fs::remove_all(dir);
    }
    ~temp_db() { std::error_code ec; fs::remove_all(dir, ec); }
    temp_db(temp_db const&) = delete;
    temp_db& operator=(temp_db const&) = delete;
    fs::path dir;
};

/// A key whose bytes look like a txid rather than like a counter.
///
/// It matters here and not in most tests: Boost lowers its growth point only for
/// a slot whose group had *overflowed*, and a group overflows when probe
/// sequences run past it. Sequential keys hash to a sequence and spread almost
/// perfectly, so nothing ever overflows and the threshold never moves. Real
/// outpoints start with a SHA-256 txid.
raw_outpoint scattered_key(uint64_t n) {
    uint64_t h = n + 0x9e3779b97f4a7c15ULL;
    h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ULL;
    h = (h ^ (h >> 27)) * 0x94d049bb133111ebULL;
    h ^= h >> 31;
    raw_outpoint k{};
    std::memcpy(k.data(), &h, sizeof(h));
    std::memcpy(k.data() + 8, &n, sizeof(n));
    return k;
}

raw_outpoint key_of(uint64_t n) {
    raw_outpoint k{};
    std::memcpy(k.data(), &n, sizeof(n));
    return k;
}

std::vector<uint8_t> payload_for(size_t index, uint8_t fill = 0x5A) {
    return std::vector<uint8_t>(index == 0 ? 8 : container_capacities[index - 1] + 1, fill);
}

/// Opens a fresh database, or stops the case where it failed.
///
/// Returns by value so the fixtures below can initialise their member from it;
/// `REQUIRE` is here rather than in a constructor body so that a database that
/// would not open never becomes an object that half exists.
template <typename Db>
Db opened_or_fail(fs::path const& dir) {
    auto opened = Db::open_for_testing(dir, true);
    REQUIRE(opened.has_value());
    return std::move(*opened);
}

/// A database with one entry in it, which is where most of these cases start.
///
/// Thirty sections opened the same database, checked the same result, moved it
/// out of the same `expected` and stored the same first key. None of those five
/// lines said anything about the case they belonged to — what a case is *about*
/// begins at the failpoint it arms.
///
/// Derived from `temp_db` rather than holding one, so `dir` still names the
/// directory and the destruction order is the one the five lines had: the
/// database closes first, and the files are still there while it does.
struct full_with_one : temp_db {
    full_db db;
    explicit full_with_one(size_t index) : db(opened_or_fail<full_db>(dir)) {
        REQUIRE(db.insert(key_of(1), payload_for(index), 800000).has_value());
    }
};

/// The same, in reference mode, whose entry is a file number and an offset.
struct reference_with_one : temp_db {
    reference_db db;
    reference_with_one() : db(opened_or_fail<reference_db>(dir)) {
        REQUIRE(db.insert(key_of(1), 1, 1, 800000).has_value());
    }
};

/// Asserts a refusal, in the order the two questions have to be asked.
///
/// `std::expected::error()` on a result that holds a value is undefined, so the
/// absence has to be established before the code is read — otherwise a case that
/// wrongly *succeeded* reads a dead union member and can pass by accident, which
/// is the one outcome a refusal test must never have. As a helper because the
/// latch case asks the same question of nine operations in a row.
template <typename Result>
void refused_with(char const* what, Result const& outcome, error_code expected) {
    INFO(what);
    REQUIRE_FALSE(outcome.has_value());
    CHECK(outcome.error() == expected);
}

/// The identifier of the generation inserts currently go to, from the census.
///
/// Distinct from the rotation count below, which happens to hold the same number
/// today and is not the same statement.
std::optional<uint64_t> active_generation(full_db& db, size_t index) {
    auto const report = db.census();
    if ( ! report) return std::nullopt;
    for (auto const& cl : report->classes) {
        if (cl.container_class != index) continue;
        for (auto const& g : cl.generations_detail) {
            if (g.active) return g.generation;
        }
    }
    return std::nullopt;
}

/// How many generations a class has been through, which is what the statistics
/// call a rotation count.
uint64_t generations_of(full_db& db, size_t index) {
    return db.get_statistics().rotations_per_container[index];
}

} // namespace

// =============================================================================
// The root cause, on a real map
// =============================================================================

TEST_CASE("Boost lowers its own growth point as entries are erased", "[transition]") {
    // No failpoint and no store: this is the property the guard was written
    // against, asserted on the container itself. If Boost ever stops doing it,
    // this case says so before anything downstream starts behaving oddly.
    //
    // The map is opened out of a real segment so that it is exactly the type the
    // store operates, allocator included.
    constexpr size_t index = 0;
    constexpr size_t small = 15359;

    failpoints::scoped_reset const disarm;
    failpoints::forced_capacity.store(small, std::memory_order_relaxed);
    failpoints::forced_capacity_index.store(index, std::memory_order_relaxed);

    temp_db t;
    {
        auto opened = full_db::open_for_testing(t.dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        db.close();
    }

    auto const file = t.dir / fmt::format(detail::data_file_format, index, 0);
    REQUIRE(fs::exists(file));
    auto seg = detail::open_existing_segment(file);
    REQUIRE(seg.has_value());
    auto found = detail::find_single_named<detail::utxo_map<container_sizes[index]>>(
        **seg, detail::map_object_name, file);
    REQUIRE(found.has_value());
    auto& map = **found;

    auto const buckets = uint64_t(map.bucket_count());
    REQUIRE(buckets == small);

    auto const operating = max_entries_for(buckets);
    auto const initial_growth_point = uint64_t(map.max_load());

    // Where the store's assumption comes from, and it is true here: a new map
    // reaches the operating threshold before Boost would grow it.
    CHECK(initial_growth_point == max_size_without_rehash(buckets));
    CHECK(operating < initial_growth_point);

    // Now churn — but with the map nearly full, which is the condition that
    // matters. Boost lowers its threshold in `recover_slot`, and only by
    // `maybe_caused_overflow(pc)`: the slot has to be in a group whose probe
    // sequence ran past it. At three per cent load that never happens and the
    // threshold does not move at all. It is a full map that drifts.
    detail::utxo_value<container_sizes[index]> v{};
    auto const payload = payload_for(index);
    v.set_data(payload);

    uint64_t next = 1;
    for (; next <= operating - 64; ++next) {
        map.emplace(scattered_key(next), v);
    }
    REQUIRE(map.size() == operating - 64);
    REQUIRE(uint64_t(map.max_load()) == initial_growth_point);

    // One out, one in, holding the size where it is.
    uint64_t oldest = 1;
    uint64_t cycles = 0;
    while (uint64_t(map.max_load()) >= operating && cycles < 3'000'000) {
        map.erase(scattered_key(oldest++));
        map.emplace(scattered_key(next++), v);
        ++cycles;
    }

    INFO("churned " << cycles << " keys at a size of " << map.size()
         << "; max_load went from " << initial_growth_point << " to " << map.max_load()
         << " against an operating threshold of " << operating);

    CHECK(uint64_t(map.max_load()) < operating);
    CHECK(uint64_t(map.bucket_count()) == buckets);
}

// =============================================================================
// The limit the guard uses
// =============================================================================

TEST_CASE("the effective limit is the lower of the two thresholds", "[transition]") {
    // The off-by-one comes from foa/table.hpp: Boost grows when
    // `size >= max_load()`, so a map holding one fewer is still safe. Both
    // thresholds have that shape, which is why they combine by taking the
    // smaller rather than by any weighting.
    CHECK(effective_insert_limit(15359, max_size_without_rehash(15359))
          == max_entries_for(15359));
    CHECK(effective_insert_limit(15359, 100) == 100);
    CHECK(effective_insert_limit(15359, max_entries_for(15359))
          == max_entries_for(15359));
    // One below the operating threshold, and Boost's is what governs.
    CHECK(effective_insert_limit(15359, max_entries_for(15359) - 1)
          == max_entries_for(15359) - 1);
}

TEST_CASE("what a map looks like after a failed allocation is classified, not assumed",
          "[transition]") {
    map_snapshot const before{100, 1000, 875};

    // The guarantee held: nothing moved and the key never went in.
    CHECK(classify_post_exception(before, before, false) == post_exception_state::map_intact);

    // The growth point too. Boost lowers it on erase — but an insert runs under
    // the exclusive write claim, so nothing may erase while this one is in
    // flight, and a max_load that moved across a throwing emplace has no path in
    // this store that accounts for it.
    CHECK(classify_post_exception(before, {100, 1000, 874}, false)
          == post_exception_state::map_mutated);

    // The three that mean it did.
    CHECK(classify_post_exception(before, {101, 1000, 875}, false)
          == post_exception_state::map_mutated);
    CHECK(classify_post_exception(before, {100, 2000, 875}, false)
          == post_exception_state::map_mutated);
    CHECK(classify_post_exception(before, before, true)
          == post_exception_state::map_mutated);
}

TEST_CASE("a failed rotation is not a rotation", "[transition]") {
    rotation_causes c;
    c.preventive = 7;
    c.capacity_exception = 2;
    c.failed = 3;
    c.unexpected_post_exception = 1;
    // Only the two that produced a generation.
    CHECK(c.completed() == 9);
}

// =============================================================================
// The hazard the guard used to have
// =============================================================================

TEST_CASE("a key already in the active map does not rotate the generation it is in",
          "[transition]") {
    // The sequence this forecloses: the guard asks for a rotation, the store
    // seals the generation holding the key, and then inserts a second copy into
    // the empty one. Two copies of one outpoint, made by the guard.
    //
    // `force_rotations` puts the guard in the state where it asks, without
    // needing to fill a map first.
    constexpr size_t index = 0;
    failpoints::scoped_reset const disarm;

    temp_db t;
    auto opened = full_db::open_for_testing(t.dir, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    auto const key = key_of(0xA11CE);
    REQUIRE(db.insert(key, payload_for(index), 800000).has_value());

    auto const generations_before = generations_of(db, index);

    // The guard will now ask for a rotation on the next insert.
    failpoints::force_rotations.store(1, std::memory_order_relaxed);
    auto const again = db.insert(key, payload_for(index), 800001);

    REQUIRE(again.has_value());
    CHECK_FALSE(*again);                                    // reported as not inserted
    CHECK(generations_of(db, index) == generations_before);  // and nothing was sealed

    auto const stats = db.get_statistics();
    CHECK(stats.rotations_by_cause[index].preventive == 0);

    // The property, asked of the store rather than inferred: one copy.
    auto const unique = db.verify_unique_outpoints();
    REQUIRE(unique.has_value());
    CHECK(unique->unique);
    CHECK(unique->distinct_outpoints == 1);
    CHECK(unique->physical_entries == 1);
    db.close();
}

TEST_CASE("reference: a key already in the active map does not rotate either",
          "[transition]") {
    failpoints::scoped_reset const disarm;

    temp_db t;
    auto opened = reference_db::open_for_testing(t.dir, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    auto const key = key_of(0xB0B);
    REQUIRE(db.insert(key, 1, 2, 800000).has_value());
    auto const generations_before = db.get_statistics().rotations_per_container[0];

    failpoints::force_rotations.store(1, std::memory_order_relaxed);
    auto const again = db.insert(key, 3, 4, 800001);

    REQUIRE(again.has_value());
    CHECK_FALSE(*again);
    CHECK(db.get_statistics().rotations_per_container[0] == generations_before);

    auto const unique = db.verify_unique_outpoints();
    REQUIRE(unique.has_value());
    CHECK(unique->unique);
    CHECK(unique->physical_entries == 1);
    db.close();
}

TEST_CASE("a new key at the threshold does rotate, and is counted as preventive",
          "[transition]") {
    // The other half: the guard still does its job for a key that is not there.
    constexpr size_t index = 0;
    failpoints::scoped_reset const disarm;

    temp_db t;
    auto opened = full_db::open_for_testing(t.dir, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    REQUIRE(db.insert(key_of(1), payload_for(index), 800000).has_value());
    auto const generations_before = generations_of(db, index);

    failpoints::force_rotations.store(1, std::memory_order_relaxed);
    auto const fresh = db.insert(key_of(2), payload_for(index), 800001);

    REQUIRE(fresh.has_value());
    CHECK(*fresh);
    CHECK(generations_of(db, index) == generations_before + 1);

    auto const stats = db.get_statistics();
    CHECK(stats.rotations_by_cause[index].preventive == 1);
    CHECK(stats.rotations_by_cause[index].capacity_exception == 0);
    CHECK(stats.rotations_by_cause[index].failed == 0);
    CHECK(stats.rotations_by_cause[index].completed() == 1);

    auto const unique = db.verify_unique_outpoints();
    REQUIRE(unique.has_value());
    CHECK(unique->unique);
    CHECK(unique->distinct_outpoints == 2);
    db.close();
}

// =============================================================================
// The exceptional path that remains
// =============================================================================

namespace {

/// Data files a class has on disk. A rotation that completed leaves one more; a
/// rotation that did not must leave none.
size_t files_for(fs::path const& dir, size_t index) {
    size_t n = 0;
    for (uint64_t v = 0; v < 64; ++v) {
        if (fs::exists(dir / fmt::format(detail::data_file_format, index, v))) ++n;
    }
    return n;
}

/// Every generation's bucket count, from the census. A map that grew says so
/// here, which is how "no rehash" is asserted without reaching into the map.
std::vector<uint64_t> bucket_counts(full_db& db, size_t index) {
    std::vector<uint64_t> out;
    auto const report = db.census();
    if ( ! report) return out;
    for (auto const& cl : report->classes) {
        if (cl.container_class != index) continue;
        for (auto const& g : cl.generations_detail) out.push_back(g.bucket_count);
    }
    return out;
}

} // namespace

TEST_CASE("an allocation that fails rotates once, retries once, and stores one copy",
          "[transition]") {
    // Deterministic, and it has to be: filling a segment until the allocator
    // happens to refuse depends on a geometry, takes a hundred thousand inserts,
    // and cannot say which insert failed. The earlier version of this case did
    // that and asserted nothing about the cause, so a run that only ever rotated
    // preventively passed it.
    constexpr size_t index = 0;
    failpoints::scoped_reset const disarm;

    temp_db t;
    auto opened = full_db::open_for_testing(t.dir, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    REQUIRE(db.insert(key_of(1), payload_for(index), 800000).has_value());
    auto const buckets_before = bucket_counts(db, index);
    REQUIRE(buckets_before.size() == 1);
    auto const files_before = files_for(t.dir, index);

    auto const key = key_of(0xFA11);
    failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
    auto const stored = db.insert(key, payload_for(index), 800001);

    // The retry succeeded and the caller is told the entry went in.
    REQUIRE(stored.has_value());
    CHECK(*stored);
    CHECK(failpoints::fail_insert_emplace.load(std::memory_order_relaxed) == 0);

    auto const stats = db.get_statistics();
    CHECK(stats.rotations_by_cause[index].capacity_exception == 1);
    CHECK(stats.rotations_by_cause[index].preventive == 0);
    CHECK(stats.rotations_by_cause[index].failed == 0);
    CHECK(stats.rotations_by_cause[index].unexpected_post_exception == 0);
    CHECK(stats.rotations_by_cause[index].completed() == 1);

    // Exactly one new generation, and one new file.
    CHECK(files_for(t.dir, index) == files_before + 1);

    // No map grew: a rotation is not a rehash, and this is the difference.
    auto const buckets_after = bucket_counts(db, index);
    REQUIRE(buckets_after.size() == 2);
    for (auto const b : buckets_after) CHECK(b == buckets_before.front());

    // One physical copy of the key that the failed attempt was carrying.
    auto const unique = db.verify_unique_outpoints();
    REQUIRE(unique.has_value());
    CHECK(unique->unique);
    CHECK(unique->physical_entries == 2);
    CHECK(unique->distinct_outpoints == 2);
    auto const found = db.find(key, 800002);
    CHECK(found.has_value());
    db.close();
}

TEST_CASE("a second failure on the new generation is refused, not rotated again",
          "[transition]") {
    // A generation made moments ago that cannot take one entry is capacity or
    // configuration, not a full file. Rotating again would only make more empty
    // files — three of them, in the loop this replaced.
    constexpr size_t index = 0;
    failpoints::scoped_reset const disarm;

    temp_db t;
    auto opened = full_db::open_for_testing(t.dir, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    REQUIRE(db.insert(key_of(1), payload_for(index), 800000).has_value());
    auto const files_before = files_for(t.dir, index);

    failpoints::fail_insert_emplace.store(2, std::memory_order_relaxed);
    auto const refused = db.insert(key_of(2), payload_for(index), 800001);

    refused_with("a second refusal on a fresh generation", refused,
                 error_code::insufficient_space);

    auto const stats = db.get_statistics();
    CHECK(stats.rotations_by_cause[index].capacity_exception == 1);   // one, not two
    CHECK(stats.rotations_by_cause[index].unexpected_post_exception == 0);
    CHECK(files_for(t.dir, index) == files_before + 1);               // one file, not two

    // Nothing is corrupt: the entry that was already stored is still reachable.
    // Through resolve() rather than find(), because the rotation sealed the
    // generation holding it and find() answers about the active maps only —
    // `not_resolved` there is the ordinary answer, not a failure.
    refused_with("find over a sealed generation", db.find(key_of(1), 800002),
                 error_code::not_resolved);
    std::vector<lookup_request> const batch{{key_of(1), 800002}};
    auto const swept = db.resolve(batch);
    REQUIRE(swept.has_value());
    CHECK(swept->found.size() == 1);
    db.close();
}

TEST_CASE("a map that moved under a failed allocation latches the instance",
          "[transition]") {
    // The state the container documents cannot happen. It is reached from a seam
    // rather than from data, because reaching it from data would mean making
    // Boost break its own guarantee — and a refusal nobody can observe is a
    // refusal somebody deletes.
    constexpr size_t index = 0;
    failpoints::scoped_reset const disarm;

    temp_db t;
    auto opened = full_db::open_for_testing(t.dir, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    REQUIRE(db.insert(key_of(1), payload_for(index), 800000).has_value());
    auto const files_before = files_for(t.dir, index);

    failpoints::fail_insert_after_mutating.store(true, std::memory_order_relaxed);
    failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
    auto const contradicted = db.insert(key_of(2), payload_for(index), 800001);

    refused_with("a map that moved under a failed allocation", contradicted,
                 error_code::entry_corrupt);

    auto const stats = db.get_statistics();
    CHECK(stats.rotations_by_cause[index].unexpected_post_exception == 1);
    CHECK(stats.rotations_by_cause[index].capacity_exception == 0);
    CHECK(stats.rotations_by_cause[index].completed() == 0);
    // It did not rotate, so it made no file.
    CHECK(files_for(t.dir, index) == files_before);

    // And the latch is the instance's, not the insert's. Every operation that
    // reads or writes the store answers the same way.
    failpoints::fail_insert_after_mutating.store(false, std::memory_order_relaxed);
    auto const latched = error_code::entry_corrupt;
    refused_with("insert", db.insert(key_of(3), payload_for(index), 800002), latched);
    refused_with("find", db.find(key_of(1), 800002), latched);
    std::vector<lookup_request> const batch{{key_of(1), 800002}};
    refused_with("resolve", db.resolve(batch), latched);
    refused_with("compact_all", db.compact_all(), latched);
    refused_with("sync", db.sync(), latched);
    refused_with("census", db.census(), latched);
    refused_with("verify_unique_outpoints", db.verify_unique_outpoints(), latched);
    refused_with("for_each_key", db.for_each_key([](raw_outpoint const&) {}), latched);
    std::vector<deferred_deletion_entry> const deletes{{key_of(1), 800002}};
    auto const progress = db.apply_deletes(deletes);
    REQUIRE(progress.error.has_value());
    CHECK(*progress.error == error_code::entry_corrupt);
    db.close();
}

TEST_CASE("a rotation that cannot make its file latches and leaves nothing behind",
          "[transition]") {
    // `new_version` closes the previous container before it makes the next one,
    // so a failure here leaves the class with no active map — and
    // `container<Index>()` dereferences a null pointer. The data is intact; this
    // instance simply has nowhere to put the next entry.
    constexpr size_t index = 0;
    failpoints::scoped_reset const disarm;

    temp_db t;
    {
        auto opened = full_db::open_for_testing(t.dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        REQUIRE(db.insert(key_of(1), payload_for(index), 800000).has_value());
        auto const files_before = files_for(t.dir, index);

        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        failpoints::fail_after_segment_create.store(true, std::memory_order_relaxed);
        auto const refused = db.insert(key_of(2), payload_for(index), 800001);
        failpoints::fail_after_segment_create.store(false, std::memory_order_relaxed);

        refused_with("a rotation that could not make its file", refused,
                      error_code::file_open_failed);

        auto const stats = db.get_statistics();
        CHECK(stats.rotations_by_cause[index].failed == 1);
        CHECK(stats.rotations_by_cause[index].completed() == 0);
        CHECK(stats.rotations_by_cause[index].preventive == 0);

        // No poisoned file: the catalogue is written after the file exists, so a
        // failure leaves neither.
        CHECK(files_for(t.dir, index) == files_before);

        // The instance stops rather than dereferencing a container it released.
        refused_with("a later insert on a class with nowhere to write",
                      db.insert(key_of(3), payload_for(index), 800002),
                      error_code::file_open_failed);
        db.close();
    }

    // And a new instance opens what was already there.
    auto reopened = full_db::open_for_testing(t.dir, false);
    REQUIRE(reopened.has_value());
    auto db2 = std::move(*reopened);
    CHECK(db2.find(key_of(1), 800003).has_value());   // the generation it was in is active again
    auto const unique = db2.verify_unique_outpoints();
    REQUIRE(unique.has_value());
    CHECK(unique->unique);
    CHECK(unique->physical_entries == 1);
    db2.close();
}

TEST_CASE("reference: the same three outcomes", "[transition]") {
    failpoints::scoped_reset const disarm;

    // Recovered.
    {
        reference_with_one t;
        auto& db = t.db;

        failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
        auto const stored = db.insert(key_of(2), 2, 2, 800001);
        REQUIRE(stored.has_value());
        CHECK(*stored);
        CHECK(db.get_statistics().rotations_by_cause[0].capacity_exception == 1);
        auto const unique = db.verify_unique_outpoints();
        REQUIRE(unique.has_value());
        CHECK(unique->unique);
        CHECK(unique->physical_entries == 2);
        db.close();
    }

    // Refused on the second failure.
    {
        reference_with_one t;
        auto& db = t.db;

        failpoints::fail_insert_emplace.store(2, std::memory_order_relaxed);
        auto const refused = db.insert(key_of(2), 2, 2, 800001);
        refused_with("reference: a second refusal on a fresh generation", refused,
                      error_code::insufficient_space);
        CHECK(db.get_statistics().rotations_by_cause[0].capacity_exception == 1);
        db.close();
    }

    // Latched.
    {
        reference_with_one t;
        auto& db = t.db;

        failpoints::fail_insert_after_mutating.store(true, std::memory_order_relaxed);
        failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
        auto const contradicted = db.insert(key_of(2), 2, 2, 800001);
        failpoints::fail_insert_after_mutating.store(false, std::memory_order_relaxed);

        refused_with("reference: a map that moved under a failed allocation",
                      contradicted, error_code::entry_corrupt);
        CHECK(db.get_statistics().rotations_by_cause[0].unexpected_post_exception == 1);
        refused_with("reference: find", db.find(key_of(1), 800002),
                      error_code::entry_corrupt);
        refused_with("reference: census", db.census(), error_code::entry_corrupt);
        db.close();
    }
}

// =============================================================================
// The diagnostic must never overrule the decision
// =============================================================================

TEST_CASE("the free-memory probe reports rather than throws", "[transition]") {
    failpoints::scoped_reset const disarm;

    // No segment is a legitimate state — a class whose rotation failed released
    // its mapping — and it is not the same fact as a segment with nothing left.
    CHECK(free_bytes_best_effort<detail::bip::managed_mapped_file>(nullptr)
          == utxoz::detail::free_bytes_unavailable);

    temp_db t;
    {
        auto opened = full_db::open_for_testing(t.dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        db.close();
    }
    auto const file = t.dir / fmt::format(detail::data_file_format, 0, 0);
    auto seg = detail::open_existing_segment(file);
    REQUIRE(seg.has_value());

    // A live segment gives a real figure, so the sentinel cannot be mistaken for
    // the ordinary answer.
    auto const live = free_bytes_best_effort(seg->get());
    CHECK(live != utxoz::detail::free_bytes_unavailable);
    CHECK(live > 0);
    CHECK(free_bytes_display(live) == std::to_string(live));

    // And a probe that fails is absorbed, not propagated.
    failpoints::fail_free_memory_probe.store(true, std::memory_order_relaxed);
    CHECK(free_bytes_best_effort(seg->get()) == utxoz::detail::free_bytes_unavailable);
    CHECK(free_bytes_display(utxoz::detail::free_bytes_unavailable) == "unavailable");
}

TEST_CASE("a diagnostic that fails does not change what an insert decides",
          "[transition]") {
    // The probe is read inside the `catch` whose remaining work is to classify,
    // latch and count. Before it was made best-effort, a throw from it would have
    // unwound past all three — the instance would not have latched, no cause
    // would have been counted, and the caller would have seen an unrelated
    // exception instead of a code. Each of the three outcomes is driven here with
    // the probe armed to fail throughout.
    constexpr size_t index = 0;

    SECTION("recovered") {
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;
        auto const files_before = files_for(t.dir, index);

        failpoints::fail_free_memory_probe.store(true, std::memory_order_relaxed);
        failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
        auto const stored = db.insert(key_of(2), payload_for(index), 800001);

        REQUIRE(stored.has_value());
        CHECK(*stored);
        auto const stats = db.get_statistics();
        CHECK(stats.rotations_by_cause[index].capacity_exception == 1);
        CHECK(stats.rotations_by_cause[index].unexpected_post_exception == 0);
        CHECK(files_for(t.dir, index) == files_before + 1);

        failpoints::fail_free_memory_probe.store(false, std::memory_order_relaxed);
        auto const unique = db.verify_unique_outpoints();
        REQUIRE(unique.has_value());
        CHECK(unique->unique);
        CHECK(unique->physical_entries == 2);
        db.close();
    }

    SECTION("refused on the second failure") {
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;

        failpoints::fail_free_memory_probe.store(true, std::memory_order_relaxed);
        failpoints::fail_insert_emplace.store(2, std::memory_order_relaxed);
        auto const refused = db.insert(key_of(2), payload_for(index), 800001);

        refused_with("a blind second refusal is still insufficient_space", refused,
                     error_code::insufficient_space);
        CHECK(db.get_statistics().rotations_by_cause[index].capacity_exception == 1);
        db.close();
    }

    SECTION("latched") {
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;
        auto const files_before = files_for(t.dir, index);

        failpoints::fail_free_memory_probe.store(true, std::memory_order_relaxed);
        failpoints::fail_insert_after_mutating.store(true, std::memory_order_relaxed);
        failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
        auto const contradicted = db.insert(key_of(2), payload_for(index), 800001);
        failpoints::fail_insert_after_mutating.store(false, std::memory_order_relaxed);

        refused_with("a blind classification still latches", contradicted,
                     error_code::entry_corrupt);
        CHECK(db.get_statistics().rotations_by_cause[index].unexpected_post_exception == 1);
        CHECK(files_for(t.dir, index) == files_before);
        refused_with("and the latch is the instance's",
                     db.insert(key_of(3), payload_for(index), 800002),
                     error_code::entry_corrupt);
        db.close();
    }
}

TEST_CASE("reference: a diagnostic that fails does not change what an insert decides",
          "[transition]") {
    failpoints::scoped_reset const disarm;
    reference_with_one t;
    auto& db = t.db;

    failpoints::fail_free_memory_probe.store(true, std::memory_order_relaxed);
    failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
    auto const stored = db.insert(key_of(2), 2, 2, 800001);

    REQUIRE(stored.has_value());
    CHECK(*stored);
    CHECK(db.get_statistics().rotations_by_cause[0].capacity_exception == 1);

    failpoints::fail_free_memory_probe.store(false, std::memory_order_relaxed);
    auto const unique = db.verify_unique_outpoints();
    REQUIRE(unique.has_value());
    CHECK(unique->unique);
    CHECK(unique->physical_entries == 2);
    db.close();
}

// =============================================================================
// What the diagnostic actually says
// =============================================================================

#if defined(UTXOZ_LOG_CUSTOM)
namespace {

/// Collects the store's log lines through the public callback.
///
/// The two figures the fixes below correct — which generation was retired, and
/// which segment the free-byte counts came from — reach nobody except through
/// this line. Asserting on them is the only way to hold them to anything; the
/// alternative is a field that can quietly start naming the wrong file, which is
/// how a diagnostic stops being evidence.
struct log_capture {
    /// Restores whatever was installed rather than clearing. `get_log_callback`
    /// is already part of the public logging API, so no test-only mechanism is
    /// needed to do this properly — and clearing unconditionally would silently
    /// disarm an outer scope's callback, which is how one test starts depending
    /// on having run after another.
    log_capture() : previous(get_log_callback()) {
        set_log_callback([this](log_level, std::string_view m) { lines.emplace_back(m); });
    }
    ~log_capture() { set_log_callback(previous); }
    log_capture(log_capture const&) = delete;
    log_capture& operator=(log_capture const&) = delete;

    /// Every line carrying a marker, in order.
    [[nodiscard]] std::vector<std::string> matching(std::string_view marker) const {
        std::vector<std::string> out;
        for (auto const& l : lines) {
            if (l.find(marker) != std::string::npos) out.push_back(l);
        }
        return out;
    }

    log_callback_t previous;
    std::vector<std::string> lines;
};

/// The value of `field=` in a log line, as text. Empty when the field is absent,
/// which fails the comparison rather than passing it.
///
/// The name has to start the line or follow a space, or `generation` finds
/// itself inside `old_generation=` and reports the wrong number — with both
/// fields on the same line, and the shorter one a prefix of the longer, that is
/// not a hypothetical.
std::string field_in(std::string const& line, std::string const& field) {
    auto const needle = field + "=";
    for (size_t at = line.find(needle); at != std::string::npos;
         at = line.find(needle, at + 1)) {
        if (at != 0 && line[at - 1] != ' ') continue;
        auto const from = at + needle.size();
        auto const to = line.find(' ', from);
        return line.substr(from, to == std::string::npos ? std::string::npos : to - from);
    }
    return {};
}

} // namespace
#endif

TEST_CASE("a field name is read whole, not as a suffix of a longer one", "[transition]") {
#if ! defined(UTXOZ_LOG_CUSTOM)
    SKIP("field_in only exists alongside the log reader");
#else
    // The line the diagnostic actually emits carries both names, and the shorter
    // one is a suffix of the longer. Reading `generation` off it used to return
    // `old_generation`'s value; nothing about that reads as wrong at the call
    // site, which is why it is asserted here rather than left to a comment.
    std::string const line =
        "insert: rotated. container=0 old_generation=4 new_generation=7 height=800001";
    CHECK(field_in(line, "old_generation") == "4");
    CHECK(field_in(line, "new_generation") == "7");
    CHECK(field_in(line, "generation").empty());
    CHECK(field_in(line, "container") == "0");
    CHECK(field_in(line, "height") == "800001");   // the last field, with no trailing space
    CHECK(field_in(line, "absent").empty());
#endif
}

TEST_CASE("the failure diagnostic names the generation it retired and the segment it "
          "measured", "[transition]") {
#if ! defined(UTXOZ_LOG_CUSTOM)
    SKIP("the log is only readable through the callback backend");
#else
    constexpr size_t index = 0;

    SECTION("a rotation reports the generation that was active, not one less than the new") {
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;

        // The generation's own identifier, from the census that reports it, and
        // not from rotations_per_container — that is a count of rotations, and a
        // count and an identity only coincide while nothing has ever disturbed
        // the numbering. Reading the identity from the field that holds one is
        // what keeps this case honest if they ever diverge.
        auto const before = active_generation(db, index);
        REQUIRE(before.has_value());
        auto const rotations_before = generations_of(db, index);

        log_capture capture;
        failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(2), payload_for(index), 800001).has_value());

        auto const rotated = capture.matching("the generation was rotated and the entry");
        REQUIRE(rotated.size() == 1);
        // Read before the call, so it names a generation rather than an offset
        // from one. The two agree while the numbering is dense; the point is that
        // the line no longer depends on it being dense.
        CHECK(field_in(rotated[0], "old_generation") == std::to_string(*before));
        auto const after = active_generation(db, index);
        REQUIRE(after.has_value());
        CHECK(field_in(rotated[0], "new_generation") == std::to_string(*after));
        CHECK(*after > *before);

        // The rotation count is a separate claim, asserted separately.
        CHECK(generations_of(db, index) == rotations_before + 1);
        db.close();
    }

    SECTION("nothing is announced before it has happened") {
        // The ordering the contract claims, read off the log. Two rotations in
        // one insert — one preventive, one after a refused allocation — and
        // neither may be described in the future tense, because a rotation that
        // then fails leaves the earlier line standing as a false statement.
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;

        log_capture capture;
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(2), payload_for(index), 800001).has_value());

        // Each rotation is reported once, after the fact, naming both ends.
        auto const preventive = capture.matching("reached its insert limit and was rotated");
        REQUIRE(preventive.size() == 1);
        CHECK(field_in(preventive[0], "old_generation") == "0");
        CHECK(field_in(preventive[0], "new_generation") == "1");

        auto const recovered = capture.matching("the generation was rotated and the entry");
        REQUIRE(recovered.size() == 1);
        CHECK(field_in(recovered[0], "old_generation") == "1");
        CHECK(field_in(recovered[0], "new_generation") == "2");

        // And the failure's own figures name the generation that failed, not the
        // one made to replace it — the copy taken before the rotation.
        CHECK(field_in(recovered[0], "generation") == "1");
        db.close();
    }

    SECTION("a preventive rotation refreshes the figure the guard never took") {
        // The guard answers "rotate" from the seam without reaching the segment,
        // so it writes no free-byte count at all. Whatever the diagnostic prints
        // afterwards has to come from the generation the retry actually ran on —
        // before the refresh was added it printed the untouched initial value.
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;

        log_capture capture;
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(2), payload_for(index), 800001).has_value());

        auto const failed = capture.matching("cause=allocator_refused_growth");
        REQUIRE(failed.size() == 1);
        auto const reported = field_in(failed[0], "free_before");
        CHECK(reported != "unavailable");
        CHECK(reported != "0");
        db.close();
    }

    SECTION("every attempt reports a figure, whichever generation it ran on") {
        // A preventive rotation and then two refusals, so all three transitions
        // run inside one insert and both attempts log.
        //
        // What this can assert is that neither line falls back to the sentinel:
        // the guard writes no figure when it refuses from the seam, so without a
        // read after each rotation the first line reads `free_before=unavailable`.
        // What it cannot assert is that the second attempt re-read rather than
        // reusing the first. The map is open-addressed and its bucket array is
        // allocated when the generation is created, so entries consume no segment
        // memory afterwards and two generations of the same geometry report
        // byte-identical free space — measured, not assumed: an earlier version
        // of this case expected the emptier one to have more room and found the
        // same 32336 bytes on both. The two implementations are numerically
        // indistinguishable here; only the code says which segment was asked.
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;

        log_capture capture;
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        failpoints::fail_insert_emplace.store(2, std::memory_order_relaxed);
        refused_with("both attempts refused",
                     db.insert(key_of(2), payload_for(index), 800001),
                     error_code::insufficient_space);

        auto const attempts = capture.matching("free_before=");
        REQUIRE(attempts.size() == 2);
        for (auto const& line : attempts) {
            INFO(line);
            auto const reported = field_in(line, "free_before");
            CHECK(reported != "unavailable");
            CHECK(reported != "0");
        }
        db.close();
    }
#endif
}

#if defined(UTXOZ_LOG_CUSTOM)
namespace {

/// A log backend that refuses, for as long as it is in scope.
///
/// The callback is the one part of the diagnostic this library does not own: it
/// runs arbitrary caller code, and there is nothing it can be assumed not to do.
/// Arming it to throw exercises the whole diagnostic boundary at once — the
/// formatting behind it is inside the same guarded lambda, so a decision that
/// survives this survives an allocation failure in `fmt::format` too.
struct throwing_log {
    throwing_log() : previous(get_log_callback()) {
        set_log_callback([](log_level, std::string_view) {
            throw std::runtime_error("the log backend refused");
        });
    }
    ~throwing_log() { set_log_callback(previous); }
    throwing_log(throwing_log const&) = delete;
    throwing_log& operator=(throwing_log const&) = delete;
    log_callback_t previous;
};

} // namespace
#endif

TEST_CASE("a message that cannot be built changes nothing an insert decided",
          "[transition]") {
    // The other half of the boundary. The case below arms the emission to fail;
    // this one arms the *building* of the message, which is a plain allocation
    // and fails the way allocations fail. Both halves sit inside the same
    // guarded lambda, and the decision is settled before either runs.
    constexpr size_t index = 0;

    SECTION("the recoverable failure still rotates once and retries once") {
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;
        auto const files_before = files_for(t.dir, index);
        auto const rotations_before = generations_of(db, index);

        failpoints::fail_diagnostic_format.store(true, std::memory_order_relaxed);
        failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
        auto const stored = db.insert(key_of(2), payload_for(index), 800001);
        failpoints::fail_diagnostic_format.store(false, std::memory_order_relaxed);

        REQUIRE(stored.has_value());
        CHECK(*stored);
        auto const stats = db.get_statistics();
        CHECK(stats.rotations_by_cause[index].capacity_exception == 1);
        CHECK(stats.rotations_by_cause[index].failed == 0);
        CHECK(generations_of(db, index) == rotations_before + 1);
        CHECK(files_for(t.dir, index) == files_before + 1);
        auto const unique = db.verify_unique_outpoints();
        REQUIRE(unique.has_value());
        CHECK(unique->unique);
        CHECK(unique->physical_entries == 2);
        db.close();
    }

    SECTION("the second failure still returns insufficient_space") {
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;
        auto const files_before = files_for(t.dir, index);

        failpoints::fail_diagnostic_format.store(true, std::memory_order_relaxed);
        failpoints::fail_insert_emplace.store(2, std::memory_order_relaxed);
        auto const refused = db.insert(key_of(2), payload_for(index), 800001);
        failpoints::fail_diagnostic_format.store(false, std::memory_order_relaxed);

        refused_with("a message-less second refusal", refused,
                     error_code::insufficient_space);
        CHECK(db.get_statistics().rotations_by_cause[index].capacity_exception == 1);
        CHECK(files_for(t.dir, index) == files_before + 1);
        db.close();
    }

    SECTION("the contradicted map still counts, latches and refuses") {
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;
        auto const files_before = files_for(t.dir, index);
        auto const rotations_before = generations_of(db, index);

        failpoints::fail_diagnostic_format.store(true, std::memory_order_relaxed);
        failpoints::fail_insert_after_mutating.store(true, std::memory_order_relaxed);
        failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
        auto const contradicted = db.insert(key_of(2), payload_for(index), 800001);
        failpoints::fail_insert_after_mutating.store(false, std::memory_order_relaxed);
        failpoints::fail_diagnostic_format.store(false, std::memory_order_relaxed);

        refused_with("a message-less contradiction", contradicted,
                     error_code::entry_corrupt);
        auto const stats = db.get_statistics();
        CHECK(stats.rotations_by_cause[index].unexpected_post_exception == 1);
        CHECK(stats.rotations_by_cause[index].completed() == 0);
        CHECK(generations_of(db, index) == rotations_before);
        CHECK(files_for(t.dir, index) == files_before);
        refused_with("and the latch is set", db.insert(key_of(3), payload_for(index), 800002),
                     error_code::entry_corrupt);
        db.close();
    }

    SECTION("reference: the same three outcomes") {
        failpoints::scoped_reset const disarm;

        {
            reference_with_one t;
            auto& db = t.db;
            failpoints::fail_diagnostic_format.store(true, std::memory_order_relaxed);
            failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
            auto const stored = db.insert(key_of(2), 2, 2, 800001);
            failpoints::fail_diagnostic_format.store(false, std::memory_order_relaxed);
            REQUIRE(stored.has_value());
            CHECK(*stored);
            CHECK(db.get_statistics().rotations_by_cause[0].capacity_exception == 1);
            auto const unique = db.verify_unique_outpoints();
            REQUIRE(unique.has_value());
            CHECK(unique->unique);
            CHECK(unique->physical_entries == 2);
            db.close();
        }

        {
            reference_with_one t;
            auto& db = t.db;
            failpoints::fail_diagnostic_format.store(true, std::memory_order_relaxed);
            failpoints::fail_insert_emplace.store(2, std::memory_order_relaxed);
            auto const refused = db.insert(key_of(2), 2, 2, 800001);
            failpoints::fail_diagnostic_format.store(false, std::memory_order_relaxed);
            refused_with("reference: a message-less second refusal", refused,
                         error_code::insufficient_space);
            CHECK(db.get_statistics().rotations_by_cause[0].capacity_exception == 1);
            db.close();
        }

        {
            reference_with_one t;
            auto& db = t.db;
            failpoints::fail_diagnostic_format.store(true, std::memory_order_relaxed);
            failpoints::fail_insert_after_mutating.store(true, std::memory_order_relaxed);
            failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
            auto const contradicted = db.insert(key_of(2), 2, 2, 800001);
            failpoints::fail_insert_after_mutating.store(false, std::memory_order_relaxed);
            failpoints::fail_diagnostic_format.store(false, std::memory_order_relaxed);
            refused_with("reference: a message-less contradiction", contradicted,
                         error_code::entry_corrupt);
            CHECK(db.get_statistics().rotations_by_cause[0].unexpected_post_exception == 1);
            db.close();
        }
    }
}

TEST_CASE("a log that throws changes nothing an insert decided", "[transition]") {
#if ! defined(UTXOZ_LOG_CUSTOM)
    SKIP("a throwing backend can only be installed through the callback");
#else
    constexpr size_t index = 0;

    SECTION("the recoverable failure still rotates once and retries once") {
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;
        auto const files_before = files_for(t.dir, index);
        auto const rotations_before = generations_of(db, index);

        result<bool> stored;
        {
            throwing_log const refuse;
            failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
            stored = db.insert(key_of(2), payload_for(index), 800001);
        }

        REQUIRE(stored.has_value());
        CHECK(*stored);
        auto const stats = db.get_statistics();
        CHECK(stats.rotations_by_cause[index].capacity_exception == 1);
        CHECK(stats.rotations_by_cause[index].unexpected_post_exception == 0);
        CHECK(stats.rotations_by_cause[index].failed == 0);
        CHECK(generations_of(db, index) == rotations_before + 1);
        CHECK(files_for(t.dir, index) == files_before + 1);

        auto const unique = db.verify_unique_outpoints();
        REQUIRE(unique.has_value());
        CHECK(unique->unique);
        CHECK(unique->physical_entries == 2);
        db.close();
    }

    SECTION("the second failure still returns insufficient_space and rotates no further") {
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;
        auto const files_before = files_for(t.dir, index);

        result<bool> refused;
        {
            throwing_log const refuse;
            failpoints::fail_insert_emplace.store(2, std::memory_order_relaxed);
            refused = db.insert(key_of(2), payload_for(index), 800001);
        }

        refused_with("a blind second refusal", refused, error_code::insufficient_space);
        CHECK(db.get_statistics().rotations_by_cause[index].capacity_exception == 1);
        CHECK(files_for(t.dir, index) == files_before + 1);   // one rotation, not two

        auto const unique = db.verify_unique_outpoints();
        REQUIRE(unique.has_value());
        CHECK(unique->unique);
        CHECK(unique->physical_entries == 1);                 // the refused entry is not there
        db.close();
    }

    SECTION("the contradicted map still counts, latches and refuses") {
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;
        auto const files_before = files_for(t.dir, index);
        auto const rotations_before = generations_of(db, index);

        result<bool> contradicted;
        {
            throwing_log const refuse;
            failpoints::fail_insert_after_mutating.store(true, std::memory_order_relaxed);
            failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
            contradicted = db.insert(key_of(2), payload_for(index), 800001);
            failpoints::fail_insert_after_mutating.store(false, std::memory_order_relaxed);
        }

        refused_with("a blind contradiction", contradicted, error_code::entry_corrupt);
        auto const stats = db.get_statistics();
        CHECK(stats.rotations_by_cause[index].unexpected_post_exception == 1);
        CHECK(stats.rotations_by_cause[index].completed() == 0);
        CHECK(generations_of(db, index) == rotations_before);
        CHECK(files_for(t.dir, index) == files_before);
        refused_with("and the latch is set", db.insert(key_of(3), payload_for(index), 800002),
                     error_code::entry_corrupt);
        refused_with("for everything", db.verify_unique_outpoints(),
                     error_code::entry_corrupt);
        db.close();
    }

    SECTION("a rotation that could not be made still latches with its own code") {
        // The rotation itself logs on failure, after counting and latching. That
        // log is behind the boundary too, so the caller still gets a code.
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;

        result<bool> refused;
        {
            throwing_log const refuse;
            failpoints::fail_after_segment_create.store(true, std::memory_order_relaxed);
            failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
            refused = db.insert(key_of(2), payload_for(index), 800001);
        }

        refused_with("a blind failed rotation", refused, error_code::file_open_failed);
        CHECK(db.get_statistics().rotations_by_cause[index].failed == 1);
        db.close();
    }

    SECTION("the duplicate at the threshold still answers false rather than throwing") {
        // A `return false` gated by a log line: before the boundary, a refusing
        // backend turned "this key is already here" into an exception.
        failpoints::scoped_reset const disarm;
        full_with_one t{index};
        auto& db = t.db;
        auto const files_before = files_for(t.dir, index);

        result<bool> again;
        {
            throwing_log const refuse;
            failpoints::force_rotations.store(1, std::memory_order_relaxed);
            again = db.insert(key_of(1), payload_for(index), 800001);
        }

        REQUIRE(again.has_value());
        CHECK_FALSE(*again);
        CHECK(files_for(t.dir, index) == files_before);   // it did not rotate
        db.close();
    }

    SECTION("reference: the same three outcomes") {
        failpoints::scoped_reset const disarm;

        {
            reference_with_one t;
            auto& db = t.db;

            result<bool> stored;
            {
                throwing_log const refuse;
                failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
                stored = db.insert(key_of(2), 2, 2, 800001);
            }
            REQUIRE(stored.has_value());
            CHECK(*stored);
            CHECK(db.get_statistics().rotations_by_cause[0].capacity_exception == 1);
            auto const unique = db.verify_unique_outpoints();
            REQUIRE(unique.has_value());
            CHECK(unique->unique);
            CHECK(unique->physical_entries == 2);
            db.close();
        }

        {
            reference_with_one t;
            auto& db = t.db;

            result<bool> refused;
            {
                throwing_log const refuse;
                failpoints::fail_insert_emplace.store(2, std::memory_order_relaxed);
                refused = db.insert(key_of(2), 2, 2, 800001);
            }
            refused_with("reference: a blind second refusal", refused,
                         error_code::insufficient_space);
            CHECK(db.get_statistics().rotations_by_cause[0].capacity_exception == 1);
            db.close();
        }

        {
            reference_with_one t;
            auto& db = t.db;

            result<bool> contradicted;
            {
                throwing_log const refuse;
                failpoints::fail_insert_after_mutating.store(true, std::memory_order_relaxed);
                failpoints::fail_insert_emplace.store(1, std::memory_order_relaxed);
                contradicted = db.insert(key_of(2), 2, 2, 800001);
                failpoints::fail_insert_after_mutating.store(false, std::memory_order_relaxed);
            }
            refused_with("reference: a blind contradiction", contradicted,
                         error_code::entry_corrupt);
            CHECK(db.get_statistics().rotations_by_cause[0].unexpected_post_exception == 1);
            refused_with("reference: and the latch is set",
                         db.insert(key_of(3), 3, 3, 800002), error_code::entry_corrupt);
            db.close();
        }
    }
#endif
}

TEST_CASE("a drifted map rotates before the next insert can grow it", "[transition]") {
    // The root cause, end to end and through the public API. The case above
    // proves that Boost lowers its growth point; this one proves the store now
    // asks about it.
    //
    // It goes red the moment `can_insert_safely` stops consulting
    // `map.max_load()`: with only the fixed threshold the map is still under it,
    // no rotation happens, and Boost grows the map — which shows up as a bucket
    // count that changed.
    constexpr size_t index = 0;
    constexpr size_t small = 15359;

    failpoints::scoped_reset const disarm;
    failpoints::forced_capacity.store(small, std::memory_order_relaxed);
    failpoints::forced_capacity_index.store(index, std::memory_order_relaxed);

    temp_db t;
    {
        auto opened = full_db::open_for_testing(t.dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        db.close();
    }

    auto const operating = max_entries_for(small);

    // Drive the drift on the file the store will open next.
    {
        auto const file = t.dir / fmt::format(detail::data_file_format, index, 0);
        auto seg = detail::open_existing_segment(file);
        REQUIRE(seg.has_value());
        auto found = detail::find_single_named<detail::utxo_map<container_sizes[index]>>(
            **seg, detail::map_object_name, file);
        REQUIRE(found.has_value());
        auto& map = **found;

        detail::utxo_value<container_sizes[index]> v{};
        auto const payload = payload_for(index);
        v.set_data(payload);

        uint64_t next = 1;
        for (; next <= operating - 64; ++next) map.emplace(scattered_key(next), v);

        // Until the growth point has fallen to what the map already holds: one
        // more new key and Boost would grow it.
        uint64_t oldest = 1;
        uint64_t cycles = 0;
        while (uint64_t(map.max_load()) > map.size() && cycles < 8'000'000) {
            map.erase(scattered_key(oldest++));
            map.emplace(scattered_key(next++), v);
            ++cycles;
        }
        INFO("drifted in " << cycles << " cycles to max_load=" << map.max_load()
             << " at size=" << map.size());
        REQUIRE(uint64_t(map.max_load()) <= map.size());

        // And the state the old guard would have read: still under the fixed
        // threshold, so it would have let the insert through.
        REQUIRE(map.size() < operating);
    }

    auto opened = full_db::open_for_testing(t.dir, false);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    auto const buckets_before = bucket_counts(db, index);
    REQUIRE(buckets_before.size() == 1);
    REQUIRE(buckets_before.front() == small);

    // One new key, through the API.
    auto const stored = db.insert(scattered_key(90'000'000), payload_for(index), 800001);
    REQUIRE(stored.has_value());
    CHECK(*stored);

    // It rotated, and it rotated for the policy's reason rather than by catching
    // an allocator that refused.
    auto const stats = db.get_statistics();
    CHECK(stats.rotations_by_cause[index].preventive == 1);
    CHECK(stats.rotations_by_cause[index].capacity_exception == 0);

    // And nothing grew: every generation still has the capacity it was built with.
    auto const buckets_after = bucket_counts(db, index);
    REQUIRE(buckets_after.size() == 2);
    for (auto const b : buckets_after) CHECK(b == small);
    db.close();
}

TEST_CASE("the counters are there with statistics compiled out", "[transition]") {
    // They describe rare operational transitions, not hot-path telemetry, so
    // they are outside UTXOZ_STATISTICS_LEVEL. A build with statistics off still
    // has to be able to say why a generation was replaced.
    constexpr size_t index = 0;
    failpoints::scoped_reset const disarm;

    temp_db t;
    auto opened = full_db::open_for_testing(t.dir, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    REQUIRE(db.insert(key_of(1), payload_for(index), 800000).has_value());
    failpoints::force_rotations.store(1, std::memory_order_relaxed);
    REQUIRE(db.insert(key_of(2), payload_for(index), 800001).has_value());

    CHECK(db.get_statistics().rotations_by_cause[index].preventive == 1);
    db.close();
}
