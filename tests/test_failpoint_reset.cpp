// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_failpoint_reset.cpp
 * @brief That disarming the seams disarms all of them.
 *
 * The failpoints are process-wide, so one left armed does not fail where it was
 * set — it follows the next test in. `clear()` is the only thing standing between
 * a test that arms a seam and every test that runs after it, and a seam added
 * without being added there is invisible until something far away starts failing
 * for no reason anyone can see.
 *
 * Nothing here tests what the seams do. It tests that they are all reachable from
 * the one function that turns them off.
 */

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "detail/durability.hpp"

using utxoz::detail::failpoints;

/// Every seam `clear()` resets, the value it is armed with, and the value it has
/// to be back at afterwards.
///
/// One list, used three times — to arm, to prove the arming took, and to check
/// the reset — so those three can never drift apart. The hand-picked subset this
/// case used to carry named six of the thirty-four, which is how
/// `fail_after_segment_create` sat outside `clear()` altogether: a case armed it,
/// every case that ran afterwards failed to create a container, and nothing
/// pointed at the cause.
///
/// A new seam belongs here in the same commit that adds it. That is still a
/// manual step, but it is one line in one place, and a seam missing from
/// `clear()` now fails here rather than somewhere unrelated.
#define UTXOZ_FAILPOINT_SEAMS(X)                                                   \
    X(fail_sync_file,                 true,                            false)      \
    X(fail_sync_mapped_region,        true,                            false)      \
    X(sync_mapped_region_calls,       11u,                             0u)         \
    X(sync_file_calls,                12u,                             0u)         \
    X(fail_sync_file_at,              13u,                             0u)         \
    X(fail_sync_directory,            true,                            false)      \
    X(fail_replace,                   true,                            false)      \
    X(fail_unlink,                    true,                            false)      \
    X(fail_source_unlink,             true,                            false)      \
    X(fail_historical_open_version,   5u,          failpoints::no_version)         \
    X(fail_historical_catalog,        true,                            false)      \
    X(fail_delete_after_applied,      7u,     failpoints::no_applied_count)        \
    X(reference_metadata_deletes,     21u,                             0u)         \
    X(full_metadata_deletes,          22u,                             0u)         \
    X(crash_at,     failpoints::crash_point::after_build,                          \
                    failpoints::crash_point::none)                                 \
    X(fail_directory_barrier_at, failpoints::dir_barrier::after_target,            \
                                 failpoints::dir_barrier::none)                    \
    X(fail_container_open,            true,                            false)      \
    X(fail_after_segment_create,      true,                            false)      \
    X(fail_after_segment_stamp,       true,                            false)      \
    X(segments_mapped,                31u,                             0u)         \
    X(fail_sidecar_removal,           true,                            false)      \
    X(fail_insert_emplace,            2u,                              0u)         \
    X(fail_insert_after_mutating,     true,                            false)      \
    X(fail_free_memory_probe,         true,                            false)      \
    X(fail_diagnostic_format,         true,                            false)      \
    X(forced_merge_id,                77u,                             0u)         \
    X(force_rotations,                3u,                              0u)         \
    X(forced_capacity,                959u,                            0u)         \
    X(forced_capacity_index,          2u,                              0u)         \
    X(force_database_id,              true,                            false)      \
    X(delete_config_after_claim,      true,                            false)      \
    X(fail_segment_open_after,        41u,                             0u)

namespace {
/// Something for `before_target_publish` to hold that is not null.
void a_hook() {}

/// The index of the first byte that is not `expected`, or -1 when every byte is.
///
/// An `all_of` over the array answers the same question and reports it as
/// `false`, which tells whoever reads the failure nothing about which byte went
/// wrong. This makes the expansion print the index.
std::ptrdiff_t first_byte_not(std::array<uint8_t, 16> const& bytes, uint8_t expected) {
    auto const at = std::ranges::find_if(bytes, [expected](uint8_t b) { return b != expected; });
    return at == bytes.end() ? -1 : at - bytes.begin();
}
} // namespace

TEST_CASE("clear() reaches every seam, including the newest", "[failpoints]") {
    // Armed first so that a failure below cannot leave them set for the rest of
    // the suite — which is the very hazard being tested.
    failpoints::scoped_reset const disarm;

#define UTXOZ_ARM(name, armed, cleared) \
    failpoints::name.store(armed, std::memory_order_relaxed);
    UTXOZ_FAILPOINT_SEAMS(UTXOZ_ARM)
#undef UTXOZ_ARM
    failpoints::forced_database_id.fill(0xAB);
    failpoints::before_target_publish.store(&a_hook, std::memory_order_relaxed);

    // That the arming took. Without this the case would pass just as well
    // against a `clear()` that reset nothing, because nothing was ever set.
#define UTXOZ_ARMED(name, armed, cleared)                                  \
    {                                                                      \
        INFO("arming " #name);                                             \
        REQUIRE(failpoints::name.load(std::memory_order_relaxed) == armed);\
    }
    UTXOZ_FAILPOINT_SEAMS(UTXOZ_ARMED)
#undef UTXOZ_ARMED
    // Every byte, not the two ends: a seam checked at its edges is a seam whose
    // middle nobody is watching, and the rest of this case is built on refusing
    // exactly that. Verified by leaving byte 7 dirty in `clear()`, which the
    // edge-only form passed.
    REQUIRE(first_byte_not(failpoints::forced_database_id, 0xAB) == -1);
    REQUIRE(failpoints::before_target_publish.load(std::memory_order_relaxed) != nullptr);

    failpoints::clear();

#define UTXOZ_CLEARED(name, armed, cleared)                                  \
    {                                                                        \
        INFO(#name " survived clear()");                                     \
        CHECK(failpoints::name.load(std::memory_order_relaxed) == cleared);  \
    }
    UTXOZ_FAILPOINT_SEAMS(UTXOZ_CLEARED)
#undef UTXOZ_CLEARED
    CHECK(first_byte_not(failpoints::forced_database_id, 0) == -1);
    CHECK(failpoints::before_target_publish.load(std::memory_order_relaxed) == nullptr);
}

TEST_CASE("the guard disarms on the way out, including the way out an exception takes",
          "[failpoints]") {
    // The reason the guard exists rather than a call at the end of a scope: a
    // failing REQUIRE leaves by throwing, so the call would be skipped.
    struct escape {};
    try {
        failpoints::scoped_reset const disarm;
        failpoints::force_rotations.store(5, std::memory_order_relaxed);
        failpoints::force_database_id.store(true, std::memory_order_relaxed);
        throw escape{};
    } catch (escape const&) {
    }

    CHECK(failpoints::force_rotations.load(std::memory_order_relaxed) == 0);
    CHECK_FALSE(failpoints::force_database_id.load(std::memory_order_relaxed));
}
