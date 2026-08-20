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

#include <atomic>

#include <catch2/catch_test_macros.hpp>

#include "detail/durability.hpp"

using utxoz::detail::failpoints;

TEST_CASE("clear() reaches every seam, including the newest", "[failpoints]") {
    // Armed first so that a failure below cannot leave them set for the rest of
    // the suite — which is the very hazard being tested.
    failpoints::scoped_reset const disarm;

    failpoints::force_rotations.store(3, std::memory_order_relaxed);
    failpoints::force_database_id.store(true, std::memory_order_relaxed);
    failpoints::forced_database_id.fill(0xAB);
    failpoints::fail_sync_file.store(true, std::memory_order_relaxed);
    failpoints::fail_container_open.store(true, std::memory_order_relaxed);
    failpoints::forced_merge_id.store(77, std::memory_order_relaxed);
    // Added after both were found missing from clear(): a case armed the first
    // one and the container creation of every case after it failed, which is
    // exactly the symptom this file exists to prevent.
    failpoints::fail_after_segment_create.store(true, std::memory_order_relaxed);
    failpoints::fail_after_segment_stamp.store(true, std::memory_order_relaxed);

    REQUIRE(failpoints::force_rotations.load(std::memory_order_relaxed) == 3);
    REQUIRE(failpoints::force_database_id.load(std::memory_order_relaxed));
    REQUIRE(failpoints::forced_database_id[0] == 0xAB);

    failpoints::clear();

    CHECK(failpoints::force_rotations.load(std::memory_order_relaxed) == 0);
    CHECK_FALSE(failpoints::force_database_id.load(std::memory_order_relaxed));
    CHECK(failpoints::forced_database_id[0] == 0);
    CHECK(failpoints::forced_database_id[15] == 0);
    CHECK_FALSE(failpoints::fail_sync_file.load(std::memory_order_relaxed));
    CHECK_FALSE(failpoints::fail_container_open.load(std::memory_order_relaxed));
    CHECK(failpoints::forced_merge_id.load(std::memory_order_relaxed) == 0);
    CHECK_FALSE(failpoints::fail_after_segment_create.load(std::memory_order_relaxed));
    CHECK_FALSE(failpoints::fail_after_segment_stamp.load(std::memory_order_relaxed));
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
