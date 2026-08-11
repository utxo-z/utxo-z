// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_deletion_ownership.cpp
 * @brief apply_deletes() answers the batch it was handed, and enumerates what it did.
 *
 * UTXO-Z used to hold the pending deletions itself: erase() wrote the key into a
 * database-wide set and process_pending_deletions() emptied all of it. Ownership
 * worked only because of who the callers happened to be, and the moment a writer
 * appeared that was not part of the existing exclusion, it would have acquired
 * the lookup defect silently (#119).
 *
 * Deletions are not lookups, though, and the difference is what most of this file
 * is about. A resolution is a read: if it cannot finish, it discards its partial
 * work and promises nothing was consumed. A deletion has already written to a
 * mapped file by the time it meets a version it cannot open. There is no promise
 * to make, so the API enumerates instead:
 *
 *   erased      what was applied — including on the failure path
 *   absent      not stored, established only after full coverage
 *   unresolved  still owed; the only category that may be resent
 *
 * The invariant every case below checks is that those three partition the
 * distinct keys of the request span: each exactly once, never twice, never
 * dropped. Getting that wrong in the direction of `absent` is the dangerous one —
 * it turns a storage fault into "this output never existed".
 */

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <numeric>
#include <ranges>
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

#include "detail/durability.hpp"

using utxoz::detail::failpoints;
using utxoz::deferred_deletion_entry;

namespace {

inline std::atomic<uint64_t> case_counter{0};

std::string make_unique_path(std::string_view tag) {
    auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fmt::format("./test_del_{}_{}_{}_{}", tag, getpid(), ts, case_counter.fetch_add(1));
}

utxoz::raw_outpoint outpoint_of(uint64_t n, uint32_t index) {
    utxoz::raw_outpoint key{};
    std::memcpy(key.data(), &n, sizeof(n));
    key[20] = 0x3D;
    std::memcpy(key.data() + 32, &index, sizeof(index));
    return key;
}

std::vector<uint8_t> value_of(size_t size, uint8_t seed) {
    std::vector<uint8_t> v(size);
    std::iota(v.begin(), v.end(), seed);
    return v;
}

struct failpoint_guard {
    failpoint_guard() { failpoints::clear(); }
    ~failpoint_guard() { failpoints::clear(); }
};

/// The invariant, checked the same way everywhere: the three lists partition the
/// distinct keys of the request span. A key in two lists, or in none, is the bug
/// this whole design exists to make impossible.
void check_partition(utxoz::deletion_progress const& p,
                     std::vector<deferred_deletion_entry> const& requests) {
    std::vector<utxoz::raw_outpoint> distinct;
    for (auto const& r : requests) {
        if (std::ranges::find(distinct, r.key) == distinct.end()) distinct.push_back(r.key);
    }

    std::vector<utxoz::raw_outpoint> classified;
    for (auto const* list : {&p.erased, &p.absent, &p.unresolved}) {
        for (auto const& e : *list) classified.push_back(e.key);
    }

    CHECK(classified.size() == distinct.size());

    auto sorted_classified = classified;
    std::ranges::sort(sorted_classified);
    // No key classified twice.
    CHECK(std::ranges::adjacent_find(sorted_classified) == sorted_classified.end());

    // And every requested key classified once.
    for (auto const& key : distinct) {
        CHECK(std::ranges::find(classified, key) != classified.end());
    }
}

/// Keys below the active version, plus keys never stored. Container 0 is rotated
/// twice so a deletion has to reach older files, and version 1 is neither the
/// first walked nor the only one — which is what lets a failpoint land after
/// some deletions have already been applied.
struct fixture {
    std::string path;
    std::vector<utxoz::raw_outpoint> historical;   // stored, below the active version
    std::vector<utxoz::raw_outpoint> never_stored;
};

fixture build_full(std::string_view tag) {
    fixture f{make_unique_path(tag), {}, {}};
    for (uint64_t i = 0; i < 8; ++i) f.historical.push_back(outpoint_of(1'000 + i, 0));
    for (uint64_t i = 0; i < 4; ++i) f.never_stored.push_back(outpoint_of(900'000'000ULL + i, 5));

    auto opened = utxoz::full_db::open_for_testing(f.path, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    for (size_t i = 0; i < f.historical.size(); ++i) {
        REQUIRE(db.insert(f.historical[i],
                          utxoz::output_data_span{value_of(33, static_cast<uint8_t>(i))}, 1).value());
    }

    uint64_t n = 10'000;
    uint32_t height = 2;
    for (int round = 0; round < 400; ++round) {
        if (db.get_statistics().rotations_per_container[0] >= 2) break;
        for (int i = 0; i < 5'000; ++i, ++n) {
            (void) db.insert(outpoint_of(n, 0), utxoz::output_data_span{value_of(33, 7)}, height);
        }
        ++height;
    }
    REQUIRE(db.get_statistics().rotations_per_container[0] >= 2);
    db.close();
    return f;
}

fixture build_reference(std::string_view tag) {
    fixture f{make_unique_path(tag), {}, {}};
    for (uint64_t i = 0; i < 8; ++i) f.historical.push_back(outpoint_of(2'000 + i, 0));
    for (uint64_t i = 0; i < 4; ++i) f.never_stored.push_back(outpoint_of(950'000'000ULL + i, 6));

    auto opened = utxoz::reference_db::open_for_testing(f.path, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    for (size_t i = 0; i < f.historical.size(); ++i) {
        REQUIRE(db.insert(f.historical[i], static_cast<uint32_t>(i), static_cast<uint32_t>(10 + i), 1).value());
    }

    uint64_t n = 10'000;
    uint32_t height = 2;
    for (int round = 0; round < 400; ++round) {
        if (db.get_statistics().rotations_per_container[0] >= 2) break;
        for (int i = 0; i < 5'000; ++i, ++n) {
            (void) db.insert(outpoint_of(n, 0), static_cast<uint32_t>(n % 1000), static_cast<uint32_t>(n), height);
        }
        ++height;
    }
    REQUIRE(db.get_statistics().rotations_per_container[0] >= 2);
    db.close();
    return f;
}

std::vector<deferred_deletion_entry> batch_of(std::vector<utxoz::raw_outpoint> const& keys, uint32_t height) {
    std::vector<deferred_deletion_entry> b;
    b.reserve(keys.size());
    for (auto const& k : keys) b.emplace_back(k, height);
    return b;
}

} // namespace

// =============================================================================
// 1. An empty batch
// =============================================================================

TEST_CASE("full: an empty batch does nothing and says so", "[deletion][full]") {
    failpoint_guard guard;
    auto const f = build_full("empty");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));
        auto const before = db.size();

        auto const progress = db.apply_deletes(std::span<deferred_deletion_entry const>{});
        CHECK(progress.erased.empty());
        CHECK(progress.absent.empty());
        CHECK(progress.unresolved.empty());
        CHECK_FALSE(progress.error.has_value());
        CHECK(db.size() == before);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

// =============================================================================
// 2 & 3. Immediate and historical deletion
// =============================================================================

TEST_CASE("full: a key in the active version is applied without touching a file",
          "[deletion][full]") {
    failpoint_guard guard;
    auto const f = build_full("immediate");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));

        // Inserted now, so it is in the active version.
        auto const fresh = outpoint_of(4'242, 1);
        REQUIRE(db.insert(fresh, utxoz::output_data_span{value_of(33, 0x5A)}, 50'000).value());
        auto const before = db.size();

        std::vector<deferred_deletion_entry> const batch{{fresh, 50'001}};
        auto const progress = db.apply_deletes(batch);

        CHECK(progress.erased.size() == 1);
        CHECK(progress.absent.empty());
        CHECK(progress.unresolved.empty());
        CHECK_FALSE(progress.error.has_value());
        check_partition(progress, batch);
        CHECK(db.size() == before - 1);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("full: keys below the active version are applied by walking the files",
          "[deletion][full]") {
    failpoint_guard guard;
    auto const f = build_full("historical");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));
        auto const before = db.size();

        auto const batch = batch_of(f.historical, 60'000);
        auto const progress = db.apply_deletes(batch);

        CHECK(progress.erased.size() == f.historical.size());
        CHECK(progress.absent.empty());
        CHECK(progress.unresolved.empty());
        CHECK_FALSE(progress.error.has_value());
        check_partition(progress, batch);
        CHECK(db.size() == before - f.historical.size());

        // And they really are gone.
        for (auto const& k : f.historical) {
            auto const again = db.apply_deletes(std::vector<deferred_deletion_entry>{{k, 60'001}});
            CHECK(again.erased.empty());
            CHECK(again.absent.size() == 1);
        }

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

// =============================================================================
// 4. Genuine absence, after a complete walk
// =============================================================================

TEST_CASE("full: a key that was never stored is absent, not unresolved",
          "[deletion][full]") {
    failpoint_guard guard;
    auto const f = build_full("absent");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));

        auto const batch = batch_of(f.never_stored, 60'000);
        auto const progress = db.apply_deletes(batch);

        CHECK(progress.erased.empty());
        CHECK(progress.absent.size() == f.never_stored.size());
        CHECK(progress.unresolved.empty());
        // No error: the walk covered everything, which is what makes absence a
        // fact rather than a guess.
        CHECK_FALSE(progress.error.has_value());
        check_partition(progress, batch);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

// =============================================================================
// 5, 7 & 8. An unreadable version: partial progress, kept and enumerated
// =============================================================================

TEST_CASE("full: an unreadable version keeps what was applied and calls nothing absent",
          "[deletion][full][negative]") {
    failpoint_guard guard;
    auto const f = build_full("unreadable");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));

        // Stored keys and never-stored keys in one batch, so a wrong
        // implementation has both kinds available to misclassify.
        std::vector<deferred_deletion_entry> batch = batch_of(f.historical, 60'000);
        for (auto const& k : f.never_stored) batch.emplace_back(k, 60'000);

        // Version 1: container 0 has versions 0 and 1 below its current, so the
        // walk applies deletions from one of them before it reaches this one.
        failpoints::fail_lookup_open_version.store(1, std::memory_order_relaxed);

        auto const progress = db.apply_deletes(batch);

        REQUIRE(progress.error.has_value());
        CHECK(*progress.error == utxoz::error_code::version_unreadable);

        // The load-bearing assertion: nothing may be called absent. Every key
        // still owed could have been in the file that would not open — including
        // the ones that genuinely do not exist, which cannot be distinguished
        // from the others while coverage is incomplete.
        CHECK(progress.absent.empty());
        CHECK_FALSE(progress.unresolved.empty());

        // And whatever was applied is reported, not discarded.
        check_partition(progress, batch);

        // The retry: only the unresolved, which is what the contract says to
        // resend. It completes, and between the two calls every key is accounted
        // for exactly once.
        failpoints::clear();
        auto const retry = db.apply_deletes(progress.unresolved);
        CHECK_FALSE(retry.error.has_value());
        CHECK(retry.unresolved.empty());
        check_partition(retry, progress.unresolved);

        // Across both calls every key is accounted for exactly once: the stored
        // ones erased, the never-stored ones absent. Nothing lost, nothing
        // deleted twice, and nothing invented.
        size_t const total_erased = progress.erased.size() + retry.erased.size();
        CHECK(total_erased == f.historical.size());
        CHECK(retry.absent.size() == f.never_stored.size());
        CHECK(total_erased + retry.absent.size() == batch.size());

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

// =============================================================================
// 6. An unlistable catalogue
// =============================================================================

TEST_CASE("full: an unlistable catalogue keeps its own cause and calls nothing absent",
          "[deletion][full][negative]") {
    failpoint_guard guard;
    auto const f = build_full("catalog");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));

        std::vector<deferred_deletion_entry> batch = batch_of(f.historical, 60'000);
        for (auto const& k : f.never_stored) batch.emplace_back(k, 60'000);

        failpoints::fail_lookup_catalog.store(true, std::memory_order_relaxed);
        auto const progress = db.apply_deletes(batch);

        REQUIRE(progress.error.has_value());
        // Not version_unreadable: not knowing which files exist sends an operator
        // somewhere else than a file that will not open.
        CHECK(*progress.error == utxoz::error_code::catalog_unreadable);
        CHECK(progress.absent.empty());
        check_partition(progress, batch);

        failpoints::clear();
        auto const retry = db.apply_deletes(progress.unresolved);
        CHECK_FALSE(retry.error.has_value());
        CHECK(retry.unresolved.empty());
        check_partition(retry, progress.unresolved);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

// =============================================================================
// 9. Duplicate requests
// =============================================================================

TEST_CASE("full: a duplicated key is one question and one answer", "[deletion][full]") {
    failpoint_guard guard;
    auto const f = build_full("duplicates");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));

        std::vector<deferred_deletion_entry> const batch{
            {f.historical[0], 60'000}, {f.historical[0], 60'001}, {f.historical[0], 60'002},
            {f.never_stored[0], 60'000}, {f.never_stored[0], 60'003},
        };

        auto const progress = db.apply_deletes(batch);

        CHECK(progress.erased.size() == 1);
        // Not two. A repeated key coming back repeatedly would read as two
        // missing outputs where the caller asked about one.
        CHECK(progress.absent.size() == 1);
        CHECK(progress.unresolved.empty());
        check_partition(progress, batch);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

// =============================================================================
// 10. Independent batches
// =============================================================================

TEST_CASE("full: independent batches return only their own results", "[deletion][full]") {
    failpoint_guard guard;
    auto const f = build_full("batches");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));

        std::vector<utxoz::raw_outpoint> const group_a(f.historical.begin(), f.historical.begin() + 4);
        std::vector<utxoz::raw_outpoint> const group_b(f.historical.begin() + 4, f.historical.end());

        auto const batch_a = batch_of(group_a, 60'000);
        auto const batch_b = batch_of(group_b, 60'000);

        auto const a = db.apply_deletes(batch_a);
        CHECK(a.erased.size() == group_a.size());
        check_partition(a, batch_a);
        for (auto const& e : a.erased) {
            CHECK(std::ranges::find(group_b, e.key) == group_b.end());
        }

        auto const b = db.apply_deletes(batch_b);
        CHECK(b.erased.size() == group_b.size());
        check_partition(b, batch_b);
        for (auto const& e : b.erased) {
            CHECK(std::ranges::find(group_a, e.key) == group_a.end());
        }

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

// =============================================================================
// 11. Statistics, complete and partial
// =============================================================================

TEST_CASE("full: an incomplete batch counts what it applied and no completed run",
          "[deletion][full][statistics]") {
    failpoint_guard guard;
    auto const f = build_full("stats");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));

        auto const before = db.get_statistics();

        // Stored and never-stored together, so the failure is guaranteed to
        // leave something owed. A batch whose every key happened to be applied
        // before the fault would retry with nothing in it, and an empty batch is
        // not a run — it does nothing and counts nothing.
        std::vector<deferred_deletion_entry> batch = batch_of(f.historical, 60'000);
        for (auto const& k : f.never_stored) batch.emplace_back(k, 60'000);

        failpoints::fail_lookup_open_version.store(1, std::memory_order_relaxed);
        auto const partial = db.apply_deletes(batch);
        REQUIRE(partial.error.has_value());

        auto const after_partial = db.get_statistics();
#ifdef UTXOZ_STATISTICS_ENABLED
        // The deletions it applied really happened, and the entry count already
        // reflects them, so they are counted. The run did not complete, so it is
        // not counted as a run — that number is what the averages divide by.
        CHECK(after_partial.deferred.successfully_processed
              == before.deferred.successfully_processed + partial.erased.size());
        CHECK(after_partial.deferred.processing_runs == before.deferred.processing_runs);
#else
        CHECK(after_partial.deferred.processing_runs == before.deferred.processing_runs);
        CHECK(after_partial.deferred.successfully_processed == before.deferred.successfully_processed);
#endif

        failpoints::clear();
        REQUIRE_FALSE(partial.unresolved.empty());
        auto const rest = db.apply_deletes(partial.unresolved);
        REQUIRE_FALSE(rest.error.has_value());

        auto const after_complete = db.get_statistics();
#ifdef UTXOZ_STATISTICS_ENABLED
        CHECK(after_complete.deferred.processing_runs == before.deferred.processing_runs + 1);
#else
        CHECK(after_complete.deferred.processing_runs == before.deferred.processing_runs);
#endif

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

/**
 * A historical deletion reaches the per-container counters, not only the batch.
 *
 * The walk over older files was rewritten to consolidate each deletion as it
 * happens, and the rewrite silently dropped every statistic the queue-draining
 * path used to record: deletions_by_depth, current_size, total_deletes and the
 * height-range histogram. The active-version phase kept recording its own, so
 * the two halves of the same call disagreed — and by container.
 *
 * Nothing caught it. The batch-level assertions all still passed, because
 * `erased` was right; it was only the numbers an operator reads that were wrong.
 * This case exists so the next rewrite of that loop cannot repeat it.
 */
TEST_CASE("full: a historical deletion records the per-container statistics",
          "[deletion][full][statistics]") {
    failpoint_guard guard;
    auto const f = build_full("hist_stats");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));

        auto const before = db.get_statistics();

        auto const batch = batch_of(f.historical, 60'000);
        auto const progress = db.apply_deletes(batch);
        REQUIRE(progress.erased.size() == f.historical.size());
        REQUIRE(progress.unresolved.empty());

        auto const after = db.get_statistics();

#ifdef UTXOZ_STATISTICS_ENABLED
        // Every applied deletion is one delete somewhere, and the totals across
        // containers have to move by exactly the number applied — not by less,
        // which is what dropping the historical half looked like.
        size_t before_deletes = 0, after_deletes = 0;
        for (size_t i = 0; i < utxoz::container_count; ++i) {
            before_deletes += before.containers[i].total_deletes;
            after_deletes += after.containers[i].total_deletes;
        }
        CHECK(after_deletes == before_deletes + progress.erased.size());

        // And the depth histogram gained the same number of entries, at depths
        // below the active version — these keys are all in older files.
        size_t before_by_depth = 0, after_by_depth = 0;
        for (auto const& [depth, count] : before.deferred.deletions_by_depth) {
            (void) depth; before_by_depth += count;
        }
        for (auto const& [depth, count] : after.deferred.deletions_by_depth) {
            (void) depth; after_by_depth += count;
        }
        CHECK(after_by_depth == before_by_depth + progress.erased.size());
#else
        CHECK(after.deferred.deletions_by_depth.size() == before.deferred.deletions_by_depth.size());
#endif

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

/**
 * A historical reference deletion is charged to the reference catalogue.
 *
 * The dispatch used to reach the reference walk through the same
 * erase_in_file<Index> the full containers use, via a switch whose `default`
 * mapped reference_sentinel_index onto Index 0. The branch inside was guarded by
 * `if constexpr (Index == SIZE_MAX)`, which that dispatch can never satisfy — so
 * a historical reference deletion erased the right entry, marked the right file
 * dirty (note_dirty takes the runtime index), and then called
 * update_metadata_on_delete(0, version): the metadata of *full container 0*.
 *
 * Nothing about the answers changed, which is the point. entry_count has no
 * runtime consumer today and a historical version's metadata is not persisted
 * after a deletion, so the wrong catalogue produces identical results, identical
 * files and an identical database. That is why this case reads the seam rather
 * than a behaviour: there is no behaviour to read, and the bookkeeping is wrong
 * the moment anything starts trusting it.
 */
TEST_CASE("reference: a historical deletion updates the reference catalogue, not container 0",
          "[deletion][reference][negative]") {
    failpoint_guard guard;
    auto const f = build_reference("ref_meta");
    {
        auto db = std::move(*utxoz::reference_db::open_for_testing(f.path, false));

        REQUIRE(failpoints::reference_metadata_deletes.load(std::memory_order_relaxed) == 0);
        REQUIRE(failpoints::full_metadata_deletes.load(std::memory_order_relaxed) == 0);

        // Historical, so it is applied by the file walk rather than by the
        // active-version phase, which is the path the dispatch is on.
        auto const batch = batch_of(f.historical, 60'000);
        auto const progress = db.apply_deletes(batch);

        REQUIRE(progress.erased.size() == f.historical.size());
        CHECK(progress.absent.empty());
        CHECK(progress.unresolved.empty());

        // Every one of them went to the reference catalogue...
        CHECK(failpoints::reference_metadata_deletes.load(std::memory_order_relaxed)
              == f.historical.size());
        // ...and none to a full container's, which this database does not even
        // have. Restoring the old dispatch makes this the non-zero one.
        CHECK(failpoints::full_metadata_deletes.load(std::memory_order_relaxed) == 0);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("full: a historical deletion updates its own container's catalogue",
          "[deletion][full]") {
    failpoint_guard guard;
    auto const f = build_full("full_meta");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));

        auto const batch = batch_of(f.historical, 60'000);
        auto const progress = db.apply_deletes(batch);
        REQUIRE(progress.erased.size() == f.historical.size());

        // The mirror of the case above: a full database charges its own
        // catalogue and never the reference one.
        CHECK(failpoints::full_metadata_deletes.load(std::memory_order_relaxed)
              == f.historical.size());
        CHECK(failpoints::reference_metadata_deletes.load(std::memory_order_relaxed) == 0);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

// =============================================================================
// A refusal partitions the batch too
// =============================================================================

/**
 * closed and recovery_required are not exempt from the contract.
 *
 * The applying path deduplicates by key; the refusal path used to copy the span
 * verbatim. So a batch naming one outpoint three times came back with three
 * entries in `unresolved` — three still owed where the caller asked about one —
 * and deletion_progress's promise that every distinct key appears exactly once
 * held everywhere except in the two states a caller is most likely to be
 * looping over the result in.
 *
 * Both states are checked with duplicates at different heights, because the
 * surviving entry has to be the first occurrence and that is only visible when
 * the heights differ.
 */
TEST_CASE("full: a closed database refuses the batch, deduplicated", "[deletion][full][negative]") {
    failpoint_guard guard;
    auto const f = build_full("closed");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));
        db.close();

        std::vector<deferred_deletion_entry> const batch{
            {f.historical[0], 100}, {f.historical[0], 200}, {f.historical[0], 300},
            {f.never_stored[0], 400}, {f.never_stored[0], 500},
        };

        auto const refused = db.apply_deletes(batch);

        CHECK(refused.erased.empty());
        CHECK(refused.absent.empty());
        REQUIRE(refused.error.has_value());
        CHECK(*refused.error == utxoz::error_code::closed);

        // One per distinct key, not one per request.
        REQUIRE(refused.unresolved.size() == 2);
        check_partition(refused, batch);

        // The first occurrence survives, which is what its height says.
        auto const by_key = [&](utxoz::raw_outpoint const& k) {
            auto const it = std::ranges::find_if(refused.unresolved,
                                                 [&](auto const& e) { return e.key == k; });
            REQUIRE(it != refused.unresolved.end());
            return it->height;
        };
        CHECK(by_key(f.historical[0]) == 100u);
        CHECK(by_key(f.never_stored[0]) == 400u);
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("full: a latched database refuses the batch, deduplicated",
          "[deletion][full][negative]") {
    failpoint_guard guard;
    auto const f = build_full("latched");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));

        // Latch the instance the way a failed cleanup does.
        failpoints::fail_source_unlink.store(true, std::memory_order_relaxed);
        auto const outcome = db.compact_all();
        failpoints::fail_source_unlink.store(false, std::memory_order_relaxed);
        REQUIRE_FALSE(outcome);
        REQUIRE(outcome.error() == utxoz::error_code::recovery_required);

        std::vector<deferred_deletion_entry> const batch{
            {f.historical[1], 100}, {f.historical[1], 200},
            {f.historical[2], 300}, {f.historical[2], 400}, {f.historical[2], 500},
        };

        auto const refused = db.apply_deletes(batch);

        CHECK(refused.erased.empty());
        CHECK(refused.absent.empty());
        REQUIRE(refused.error.has_value());
        CHECK(*refused.error == utxoz::error_code::recovery_required);

        REQUIRE(refused.unresolved.size() == 2);
        check_partition(refused, batch);

        auto const by_key = [&](utxoz::raw_outpoint const& k) {
            auto const it = std::ranges::find_if(refused.unresolved,
                                                 [&](auto const& e) { return e.key == k; });
            REQUIRE(it != refused.unresolved.end());
            return it->height;
        };
        CHECK(by_key(f.historical[1]) == 100u);
        CHECK(by_key(f.historical[2]) == 300u);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

// =============================================================================
// An exception after a deletion has already been applied
// =============================================================================

/**
 * The window between the map changing and the working set knowing about it.
 *
 * fail_lookup_open_version fires before a file is touched, so every case using
 * it leaves that window unexercised. This one throws between the erase and the
 * bookkeeping that follows it, after a chosen number of deletions have already
 * been applied within the call — which is the only way to reach the state the
 * old shape got wrong: an index still sitting in `pending` for a key that is
 * already gone from the map.
 *
 * What that produced was a key reported as applied *and* still owed. A caller
 * resending its `unresolved` would then be told the key is absent, and on the
 * connect path an absence is fatal — so a deletion that succeeded turned into a
 * rejected block one retry later.
 */
TEST_CASE("full: an exception after an applied deletion loses neither side",
          "[deletion][full][negative]") {
    failpoint_guard guard;
    auto const f = build_full("midfile");
    {
        auto db = std::move(*utxoz::full_db::open_for_testing(f.path, false));
        auto const before_size = db.size();

        auto const batch = batch_of(f.historical, 60'000);

        // Throw once three deletions have been applied, so the failure lands
        // mid-walk with real progress behind it and real work ahead.
        failpoints::fail_delete_after_applied.store(3, std::memory_order_relaxed);
        auto const progress = db.apply_deletes(batch);
        failpoints::clear();

        REQUIRE(progress.error.has_value());
        CHECK(*progress.error == utxoz::error_code::version_unreadable);

        // Real progress, and exactly the amount asked for.
        REQUIRE(progress.erased.size() == 3);
        // Incomplete coverage, so nothing may be called absent.
        CHECK(progress.absent.empty());
        REQUIRE_FALSE(progress.unresolved.empty());

        // The load-bearing pair: no key is in both lists, and none went missing.
        check_partition(progress, batch);
        for (auto const& applied : progress.erased) {
            for (auto const& owed : progress.unresolved) {
                CHECK(applied.key != owed.key);
            }
        }

        // The store agrees with the report: exactly what `erased` names is gone.
        CHECK(db.size() == before_size - progress.erased.size());

        // Resending only what is owed completes the batch...
        auto const retry = db.apply_deletes(progress.unresolved);
        CHECK_FALSE(retry.error.has_value());
        CHECK(retry.unresolved.empty());
        check_partition(retry, progress.unresolved);

        // ...and the two calls together partition the original distinct keys,
        // once each, with nothing deleted twice and nothing left behind.
        CHECK(progress.erased.size() + retry.erased.size() == f.historical.size());
        CHECK(retry.absent.empty());
        CHECK(db.size() == before_size - f.historical.size());

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("reference: an exception after an applied deletion loses neither side",
          "[deletion][reference][negative]") {
    failpoint_guard guard;
    auto const f = build_reference("ref_midfile");
    {
        auto db = std::move(*utxoz::reference_db::open_for_testing(f.path, false));
        auto const before_size = db.size();

        auto const batch = batch_of(f.historical, 60'000);

        failpoints::fail_delete_after_applied.store(2, std::memory_order_relaxed);
        auto const progress = db.apply_deletes(batch);
        failpoints::clear();

        REQUIRE(progress.error.has_value());
        REQUIRE(progress.erased.size() == 2);
        CHECK(progress.absent.empty());
        check_partition(progress, batch);
        for (auto const& applied : progress.erased) {
            for (auto const& owed : progress.unresolved) {
                CHECK(applied.key != owed.key);
            }
        }
        CHECK(db.size() == before_size - progress.erased.size());

        auto const retry = db.apply_deletes(progress.unresolved);
        CHECK_FALSE(retry.error.has_value());
        CHECK(retry.unresolved.empty());
        CHECK(progress.erased.size() + retry.erased.size() == f.historical.size());
        CHECK(db.size() == before_size - f.historical.size());

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

// =============================================================================
// reference mode — the same contract, case for case
// =============================================================================

TEST_CASE("reference: keys below the active version are applied by walking the files",
          "[deletion][reference]") {
    failpoint_guard guard;
    auto const f = build_reference("ref_historical");
    {
        auto db = std::move(*utxoz::reference_db::open_for_testing(f.path, false));
        auto const before = db.size();

        auto const batch = batch_of(f.historical, 60'000);
        auto const progress = db.apply_deletes(batch);

        CHECK(progress.erased.size() == f.historical.size());
        CHECK(progress.absent.empty());
        CHECK(progress.unresolved.empty());
        check_partition(progress, batch);
        CHECK(db.size() == before - f.historical.size());

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("reference: a key that was never stored is absent, not unresolved",
          "[deletion][reference]") {
    failpoint_guard guard;
    auto const f = build_reference("ref_absent");
    {
        auto db = std::move(*utxoz::reference_db::open_for_testing(f.path, false));

        auto const batch = batch_of(f.never_stored, 60'000);
        auto const progress = db.apply_deletes(batch);

        CHECK(progress.erased.empty());
        CHECK(progress.absent.size() == f.never_stored.size());
        CHECK(progress.unresolved.empty());
        CHECK_FALSE(progress.error.has_value());
        check_partition(progress, batch);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("reference: an unreadable version keeps what was applied and calls nothing absent",
          "[deletion][reference][negative]") {
    failpoint_guard guard;
    auto const f = build_reference("ref_unreadable");
    {
        auto db = std::move(*utxoz::reference_db::open_for_testing(f.path, false));

        std::vector<deferred_deletion_entry> batch = batch_of(f.historical, 60'000);
        for (auto const& k : f.never_stored) batch.emplace_back(k, 60'000);

        failpoints::fail_lookup_open_version.store(1, std::memory_order_relaxed);
        auto const progress = db.apply_deletes(batch);

        REQUIRE(progress.error.has_value());
        CHECK(*progress.error == utxoz::error_code::version_unreadable);
        CHECK(progress.absent.empty());
        check_partition(progress, batch);

        failpoints::clear();
        auto const retry = db.apply_deletes(progress.unresolved);
        CHECK_FALSE(retry.error.has_value());
        CHECK(retry.unresolved.empty());
        check_partition(retry, progress.unresolved);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("reference: an unlistable catalogue keeps its own cause", "[deletion][reference][negative]") {
    failpoint_guard guard;
    auto const f = build_reference("ref_catalog");
    {
        auto db = std::move(*utxoz::reference_db::open_for_testing(f.path, false));

        auto const batch = batch_of(f.historical, 60'000);
        failpoints::fail_lookup_catalog.store(true, std::memory_order_relaxed);
        auto const progress = db.apply_deletes(batch);

        REQUIRE(progress.error.has_value());
        CHECK(*progress.error == utxoz::error_code::catalog_unreadable);
        CHECK(progress.absent.empty());
        check_partition(progress, batch);

        failpoints::clear();
        auto const retry = db.apply_deletes(progress.unresolved);
        CHECK_FALSE(retry.error.has_value());
        check_partition(retry, progress.unresolved);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}

TEST_CASE("reference: a duplicated key is one question and one answer", "[deletion][reference]") {
    failpoint_guard guard;
    auto const f = build_reference("ref_duplicates");
    {
        auto db = std::move(*utxoz::reference_db::open_for_testing(f.path, false));

        std::vector<deferred_deletion_entry> const batch{
            {f.historical[0], 60'000}, {f.historical[0], 60'001},
            {f.never_stored[0], 60'000}, {f.never_stored[0], 60'002},
        };

        auto const progress = db.apply_deletes(batch);
        CHECK(progress.erased.size() == 1);
        CHECK(progress.absent.size() == 1);
        check_partition(progress, batch);

        db.close();
    }
    std::filesystem::remove_all(f.path);
}
