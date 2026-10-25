// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_insert_batch.cpp
 * @brief full_db::insert_batch against the same requests inserted one by one.
 *
 * The contract is that a batch stores exactly what `insert` would have stored,
 * called once per request in order, whatever the number of workers. So almost
 * every case here builds two stores from the same requests - one sequentially,
 * one as a batch - and compares everything that can be compared: the outcome of
 * each request, the full content (an order-independent digest of every entry),
 * the size, the per-class statistics, the rotations and their causes, the
 * generations of every class and every shard, and the height-range statistics.
 * Then it closes the batch store, reopens it, and compares again.
 *
 * The failure cases check what a batch reports against what is actually stored,
 * read back from a fresh instance after the latched one is closed.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <utxoz/database.hpp>
#include <utxoz/types.hpp>

#include "detail/durability.hpp"
#include "support/batch_workload.hpp"

namespace fs = std::filesystem;
using namespace utxoz;
using namespace utxoz::testing::batch;
using utxoz::detail::failpoints;
using utxoz::testing::spread_key;

// =============================================================================
// Equivalence with the sequential inserts
// =============================================================================

TEST_CASE("batch height-range observations use the configured logging backend",
          "[batch][logging]") {
    temp_db t;
    auto db = open_db(t.dir, true);
#if defined(UTXOZ_LOG_SPDLOG)
    auto const previous = spdlog::default_logger();
    auto const previous_capture = spdlog::get("batch-capture");
#endif
    auto const lines = height_range_lines(db);
#if defined(UTXOZ_LOG_CUSTOM) || defined(UTXOZ_LOG_SPDLOG)
    REQUIRE(lines.size() == 1);
    CHECK(lines.front().find("No height range statistics collected.") != std::string::npos);
#else
    CHECK(lines.empty());
#endif
#if defined(UTXOZ_LOG_SPDLOG)
    CHECK(spdlog::default_logger() == previous);
    CHECK(spdlog::get("batch-capture") == previous_capture);
#endif
    db.close();
}

TEST_CASE("a batch stores what the same inserts in order store, for every worker count",
          "[batch]") {
    failpoints::scoped_reset const disarm;
    auto const w = mixed();

    temp_db ref_dir;
    auto ref = open_db(ref_dir.dir, true);
    auto const ref_outcomes = insert_sequentially(ref, w);
    auto const ref_figures = figures_of(ref);
    REQUIRE(ref_figures.rotations[3] >= 1);   // the rotating classes did rotate
    REQUIRE(ref_figures.rotations[4] >= 3);
    ref.close();

    for (unsigned workers : {1u, 2u, 4u, 8u}) {
        DYNAMIC_SECTION("workers " << workers) {
            temp_db t;
            auto db = open_db(t.dir, true);
            auto const r = db.insert_batch(w.requests, {.workers = workers});
            CHECK_FALSE(r.error.has_value());
            CHECK(r.workers_used == std::min<size_t>(workers, slots_touched(w)));
            CHECK(r.inserted + r.duplicates == w.requests.size());
            CHECK(r.failed == 0);
            CHECK(r.not_applied == 0);
            require_same_outcomes(r.outcomes, ref_outcomes);
            CHECK(figures_of(db) == ref_figures);

            // And what was written survives a close and a reopen.
            db.close();
            auto again = open_db(t.dir, false);
            CHECK(again.size() == ref_figures.size);
            CHECK(content_of(again) == ref_figures.stored);
            again.close();
        }
    }
}

#if defined(UTXOZ_LOG_SPDLOG)
TEST_CASE("batch capture preserves an existing named logger", "[batch][logging]") {
    struct restore_registry {
        std::shared_ptr<spdlog::logger> previous = spdlog::default_logger();
        std::shared_ptr<spdlog::logger> named = spdlog::get("batch-capture");
        ~restore_registry() {
            spdlog::drop("batch-capture");
            spdlog::set_default_logger(std::move(previous));
            if (named && named != spdlog::default_logger()) spdlog::register_logger(std::move(named));
        }
    } restore;
    auto existing = std::make_shared<spdlog::logger>("batch-capture");
    spdlog::drop("batch-capture");
    SECTION("a named logger separate from the default") {
        spdlog::register_logger(existing);
    }
    SECTION("the previous default has the capture's name") {
        spdlog::set_default_logger(existing);
    }
    auto const previous = spdlog::default_logger();
    {
        scoped_log_capture capture([](std::string_view) {});
        CHECK(spdlog::get("batch-capture") != existing);
    }
    CHECK(spdlog::default_logger() == previous);
    CHECK(spdlog::get("batch-capture") == existing);
}
#endif

TEST_CASE("duplicates in one slot keep the first, in order; one key in two classes is two entries",
          "[batch]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto db = open_db(t.dir, true);
    auto const k = spread_key(42);
    auto const a = payload(0, 1), b = payload(0, 2), c = payload(3, 3);
    std::vector<insert_request> const reqs{
        {k, a, 100}, {k, b, 200}, {k, c, 300}, {k, a, 400}};
    auto const r = db.insert_batch(reqs, {.workers = 4});
    REQUIRE(r.outcomes.size() == 4);
    CHECK(r.outcomes[0].status == insert_status::inserted);
    CHECK(r.outcomes[1].status == insert_status::duplicate);
    CHECK(r.outcomes[2].status == insert_status::inserted);   // class 3: another slot
    CHECK(r.outcomes[3].status == insert_status::duplicate);
    auto const found = db.find(k, 500);
    REQUIRE(found.has_value());
    CHECK(found->block_height == 100);
    CHECK(db.size() == 2);
    db.close();
}

TEST_CASE("a request too large for any class fails alone; the rest is applied",
          "[batch]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto db = open_db(t.dir, true);
    std::vector<uint8_t> const huge(container_capacities[container_count - 1] + 1, 1);
    auto const ok = payload(0, 7);
    std::vector<insert_request> const reqs{{spread_key(1), ok, 1}, {spread_key(2), huge, 1},
                                           {spread_key(3), ok, 1}};
    auto const r = db.insert_batch(reqs, {.workers = 2});
    CHECK(r.outcomes[0].status == insert_status::inserted);
    CHECK(r.outcomes[1].status == insert_status::failed);
    CHECK(r.outcomes[1].error == error_code::value_too_large);
    CHECK(r.outcomes[2].status == insert_status::inserted);
    CHECK(r.error == error_code::value_too_large);
    CHECK(db.size() == 2);
    db.close();
}

TEST_CASE("an empty batch does nothing and reports nothing wrong", "[batch]") {
    temp_db t;
    auto db = open_db(t.dir, true);
    auto const r = db.insert_batch({}, {.workers = 8});
    CHECK(r.outcomes.empty());
    CHECK_FALSE(r.error.has_value());
    CHECK(r.workers_used == 0);
    db.close();
}

// =============================================================================
// The executor
// =============================================================================

TEST_CASE("the caller's executor runs the tasks; one that starts none or throws still "
          "gets a complete batch", "[batch]") {
    failpoints::scoped_reset const disarm;
    auto const w = mixed();
    temp_db ref_dir;
    auto ref = open_db(ref_dir.dir, true);
    insert_sequentially(ref, w);
    auto const want = content_of(ref);
    ref.close();

    SECTION("an executor with its own threads") {
        temp_db t;
        auto db = open_db(t.dir, true);
        std::atomic<size_t> calls{0};
        insert_batch_options o{.workers = 4};
        o.run_tasks = [&](size_t n, std::function<void(size_t)> const& task) {
            ++calls;
            std::vector<std::thread> ts;
            for (size_t i = 0; i != n; ++i) ts.emplace_back(task, i);
            for (auto& th : ts) th.join();
        };
        auto const r = db.insert_batch(w.requests, o);
        CHECK(calls == 1);
        CHECK(r.tasks_run_inline == 0);
        CHECK_FALSE(r.error.has_value());
        CHECK(content_of(db) == want);
        db.close();
    }
    SECTION("an executor that starts nothing") {
        temp_db t;
        auto db = open_db(t.dir, true);
        insert_batch_options o{.workers = 4};
        o.run_tasks = [](size_t, std::function<void(size_t)> const&) {};
        auto const r = db.insert_batch(w.requests, o);
        CHECK(r.tasks_run_inline == r.workers_used);
        CHECK_FALSE(r.error.has_value());
        CHECK(content_of(db) == want);
        db.close();
    }
    SECTION("an executor that throws after one task finished on its own thread") {
        temp_db t;
        auto db = open_db(t.dir, true);
        insert_batch_options o{.workers = 4};
        o.run_tasks = [&](size_t, std::function<void(size_t)> const& task) {
            std::thread(task, 0).join();
            throw std::runtime_error("executor gave up");
        };
        auto const r = db.insert_batch(w.requests, o);
        // Task 0 is not run again; the batch ran the rest itself.
        CHECK(r.tasks_run_inline == r.workers_used - 1);
        CHECK_FALSE(r.error.has_value());
        CHECK(content_of(db) == want);
        db.close();
    }
}

// =============================================================================
// The gate: what a call through a copy of the task can and cannot do
// =============================================================================
//
// The executor contract is synchronous: run_tasks may not return before every
// call it made to `task` has returned. These cases break that contract in the
// ways a copy of the task makes survivable, and check that the batch neither
// reads a result while a task is still running nor lets a late call touch
// anything. Under ASan or TSan a gate that lived in the batch's frame, or a fold
// that did not wait, is a report here and not a pass.

TEST_CASE("a call through a copy of the task after the batch returned runs nothing",
          "[batch][executor]") {
    failpoints::scoped_reset const disarm;
    auto const w = mixed();
    temp_db ref_dir;
    auto ref = open_db(ref_dir.dir, true);
    insert_sequentially(ref, w);
    auto const want = figures_of(ref);
    ref.close();

    temp_db t;
    auto db = open_db(t.dir, true);
    std::function<void(size_t)> kept;
    size_t count = 0;
    insert_batch_options o{.workers = 4};
    o.run_tasks = [&](size_t n, std::function<void(size_t)> const& task) {
        kept = task;   // a copy, and nothing run
        count = n;
    };
    auto const r = db.insert_batch(w.requests, o);
    REQUIRE(kept);
    CHECK(r.tasks_run_inline == r.workers_used);
    CHECK_FALSE(r.error.has_value());
    CHECK(figures_of(db) == want);

    // Late, from another thread, every index and one past the end; then again
    // after the store is closed. None of it may run or touch anything.
    std::thread late([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        for (size_t i = 0; i <= count; ++i) kept(i);
    });
    late.join();
    CHECK(figures_of(db) == want);
    db.close();
    kept(0);
    kept = nullptr;
}

TEST_CASE("a task still running through a copy when the executor returns is waited for",
          "[batch][executor]") {
#if !defined(UTXOZ_LOG_CUSTOM) && !defined(UTXOZ_LOG_SPDLOG)
    SKIP("this control requires an observable logging backend to suspend the worker");
#else
    failpoints::scoped_reset const disarm;
    // Only task 0 may log. The log is serialised, so a fallback task on the
    // calling thread that logged would queue behind the blocked line below and
    // wait for task 0 by that road instead - and a batch that did not wait
    // would pass. Class 3 rotates (and logs) and is the heaviest slot, so it is
    // task 0's; the other classes neither rotate nor see a duplicate.
    workload w;
    uint64_t n = 0;
    for (uint64_t i = 0; i != 26000; ++i, ++n) w.add(spread_key(n), 3, n, uint32_t(700000 + i / 1000));
    for (size_t cls : {0u, 1u, 2u})
        for (uint64_t i = 0; i != 3000; ++i, ++n) w.add(spread_key(n), cls, n, uint32_t(700000 + i / 1000));
    w.bind();
    temp_db ref_dir;
    auto ref = open_db(ref_dir.dir, true);
    insert_sequentially(ref, w);
    auto const want = figures_of(ref);
    ref.close();

    // The started thread blocks inside task 0's first log line, so when the
    // executor returns, task 0 is provably mid-flight. It is released a while
    // later, from a thread of its own: the batch must still be waiting for it.
    std::mutex m;
    std::condition_variable cv;
    bool entered = false, released = false;
    static thread_local bool in_started_thread = false;
    scoped_log_capture capture([&](std::string_view) {
        if ( ! in_started_thread) return;
        std::unique_lock lock(m);
        if (entered) return;
        entered = true;
        cv.notify_all();
        cv.wait(lock, [&] { return released; });
    });

    temp_db t;
    auto db = open_db(t.dir, true);
    std::thread started, releaser;
    insert_batch_options o{.workers = 4};
    o.run_tasks = [&](size_t, std::function<void(size_t)> const& task) {
        started = std::thread([copy = task] { in_started_thread = true; copy(0); });
        std::unique_lock lock(m);
        cv.wait(lock, [&] { return entered; });
        releaser = std::thread([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::scoped_lock const l(m);
            released = true;
            cv.notify_all();
        });
        // Returns, against the contract, with task 0 running.
    };
    auto const r = db.insert_batch(w.requests, o);
    {
        // The batch could only have returned after task 0 finished.
        std::scoped_lock const l(m);
        CHECK(released);
    }
    started.join();
    releaser.join();
    CHECK(r.tasks_run_inline == r.workers_used - 1);
    CHECK_FALSE(r.error.has_value());
    CHECK(r.inserted + r.duplicates == w.requests.size());
    capture.reset();
    CHECK(figures_of(db) == want);
    db.close();
#endif
}

TEST_CASE("two threads calling every task race for it and each task runs once",
          "[batch][executor]") {
    failpoints::scoped_reset const disarm;
    auto const w = mixed();
    temp_db ref_dir;
    auto ref = open_db(ref_dir.dir, true);
    insert_sequentially(ref, w);
    auto const want = figures_of(ref);
    ref.close();

    for (unsigned workers : {2u, 8u}) {
        DYNAMIC_SECTION("workers " << workers) {
            temp_db t;
            auto db = open_db(t.dir, true);
            insert_batch_options o{.workers = workers};
            o.run_tasks = [](size_t n, std::function<void(size_t)> const& task) {
                // Synchronous, as the contract asks; every task called twice,
                // from two threads, the second caller always losing or waiting.
                auto all = [&] { for (size_t i = 0; i != n + 1; ++i) task(i); };
                std::thread a(all), b(all);
                a.join();
                b.join();
            };
            auto const r = db.insert_batch(w.requests, o);
            CHECK(r.tasks_run_inline == 0);
            CHECK_FALSE(r.error.has_value());
            CHECK(figures_of(db) == want);
            db.close();
        }
    }
}

TEST_CASE("an executor that throws without joining a thread holding a copy still gets a "
          "complete batch", "[batch][executor]") {
    failpoints::scoped_reset const disarm;
    auto const w = mixed();
    temp_db ref_dir;
    auto ref = open_db(ref_dir.dir, true);
    insert_sequentially(ref, w);
    auto const want = content_of(ref);
    ref.close();

    temp_db t;
    auto db = open_db(t.dir, true);
    std::thread started;
    insert_batch_options o{.workers = 4};
    o.run_tasks = [&](size_t, std::function<void(size_t)> const& task) {
        started = std::thread(task, 0);   // std::thread copies the task
        throw std::runtime_error("executor gave up");
    };
    auto const r = db.insert_batch(w.requests, o);
    // Whoever claims task 0 first runs it, exactly once: the started thread
    // (the batch waits for it) or the calling thread.
    CHECK(r.tasks_run_inline >= r.workers_used - 1);
    CHECK(r.tasks_run_inline <= r.workers_used);
    CHECK_FALSE(r.error.has_value());
    CHECK(content_of(db) == want);
    started.join();
    db.close();
}

// =============================================================================
// Memory: what a batch allocates, it allocates before it writes
// =============================================================================

TEST_CASE("a batch that cannot allocate its bookkeeping throws before writing anything",
          "[batch][failpoint]") {
    failpoints::scoped_reset const disarm;
    auto const w = mixed();
    temp_db ref_dir;
    auto ref = open_db(ref_dir.dir, true);
    insert_sequentially(ref, w);
    auto const want = figures_of(ref);
    ref.close();

    temp_db t;
    auto db = open_db(t.dir, true);
    auto const empty = figures_of(db);
    failpoints::fail_batch_prepare.store(true);
    CHECK_THROWS_AS(db.insert_batch(w.requests, {.workers = 4}), std::bad_alloc);
    failpoints::fail_batch_prepare.store(false);
    CHECK(figures_of(db) == empty);

    // Nothing latched: the same batch, sent again, is the whole batch.
    auto const r = db.insert_batch(w.requests, {.workers = 4});
    CHECK_FALSE(r.error.has_value());
    CHECK(r.not_applied == 0);
    CHECK(figures_of(db) == want);
    db.close();
    auto again = open_db(t.dir, false);
    CHECK(content_of(again) == want.stored);
    again.close();
}

// =============================================================================
// Failures: what is reported against what is stored
// =============================================================================

namespace {

/// Reads back, on a fresh instance, whether each request's entry is stored with
/// the request's own payload and height - the only facts a report can be held to.
void check_report_against_store(fs::path const& dir, workload const& w, insert_batch_result const& r) {
    auto db = open_db(dir, false);
    std::vector<lookup_request> all;
    for (auto const& q : w.requests) all.emplace_back(q.key, 1);
    auto const resolved = db.resolve(all);
    REQUIRE(resolved.has_value());
    std::map<raw_outpoint, full_find_result> stored;
    for (auto const& q : w.requests) {
        if (auto f = db.find(q.key, 1)) stored.emplace(q.key, *f);
    }
    for (auto const& [k, v] : resolved->found) stored.emplace(k, v);

    size_t applied_but_reported_unapplied = 0, reported_inserted_but_missing = 0;
    for (size_t i = 0; i != w.requests.size(); ++i) {
        auto const& q = w.requests[i];
        auto const it = stored.find(q.key);
        bool const mine = it != stored.end() && it->second.block_height == q.height
                       && std::ranges::equal(it->second.data, q.value);
        if (r.outcomes[i].status == insert_status::inserted && ! mine) ++reported_inserted_but_missing;
        if (r.outcomes[i].status == insert_status::not_applied && mine) ++applied_but_reported_unapplied;
    }
    CHECK(reported_inserted_but_missing == 0);
    CHECK(applied_but_reported_unapplied == 0);
    db.close();
}

} // namespace

TEST_CASE("an integrity latch in one worker stops the batch; every report matches the store",
          "[batch][failpoint]") {
    failpoints::scoped_reset const disarm;
    auto const w = mixed();
    temp_db t;
    {
        auto db = open_db(t.dir, true);
        // The next emplace mutates the map and then throws: the one state the
        // container promises cannot happen, which latches the instance.
        failpoints::fail_insert_after_mutating.store(true);
        failpoints::fail_insert_emplace.store(1);
        auto const r = db.insert_batch(w.requests, {.workers = 4});
        failpoints::fail_insert_emplace.store(0);
        failpoints::fail_insert_after_mutating.store(false);

        REQUIRE(r.error == error_code::entry_corrupt);
        CHECK(r.failed == 1);
        CHECK(r.inserted + r.duplicates + r.failed + r.not_applied == w.requests.size());
        CHECK(std::ranges::count_if(r.outcomes, [](auto const& o) {
            return o.status == insert_status::failed && o.error == error_code::entry_corrupt;
        }) == 1);
        // A latched instance refuses the next insert, single or batched.
        CHECK(db.insert(spread_key(99999999), payload(0, 1), 1).error() == error_code::entry_corrupt);
        auto const again = db.insert_batch(w.requests, {.workers = 2});
        CHECK(again.not_applied == w.requests.size());
        CHECK(again.error == error_code::entry_corrupt);
        db.close();
        check_report_against_store(t.dir, w, r);
    }
}

TEST_CASE("a rotation that cannot make its file latches; entries before it stay, after it are "
          "not applied", "[batch][failpoint][rotation]") {
    failpoints::scoped_reset const disarm;
    // Only class 4 rotates here (797 entries per generation), so the failing
    // rotation is that slot's first; the other slots keep going until they see
    // the latch.
    workload w;
    for (uint64_t i = 0; i != 20000; ++i) w.add(spread_key(i), 0, i, uint32_t(1000 + i / 1000));
    for (uint64_t i = 0; i != 1200; ++i) w.add(spread_key(100000 + i), 4, 100000 + i, 1000);
    w.bind();
    temp_db t;
    auto db = open_db(t.dir, true);
    failpoints::fail_after_segment_create.store(true);
    auto const r = db.insert_batch(w.requests, {.workers = 4});
    failpoints::fail_after_segment_create.store(false);

    REQUIRE(r.error == error_code::file_open_failed);
    // Class 4's requests: the first 797 fill the generation, the 798th wants the
    // rotation that fails, and nothing after it in that slot is attempted.
    size_t const first4 = 20000;
    for (size_t i = first4; i != first4 + 797; ++i) {
        INFO("class 4 request " << i - first4);
        CHECK(r.outcomes[i].status == insert_status::inserted);
    }
    CHECK(r.outcomes[first4 + 797].status == insert_status::failed);
    CHECK(r.outcomes[first4 + 797].error == error_code::file_open_failed);
    for (size_t i = first4 + 798; i != w.requests.size(); ++i) {
        INFO("class 4 request " << i - first4);
        CHECK(r.outcomes[i].status == insert_status::not_applied);
    }
    db.close();
    check_report_against_store(t.dir, w, r);
}

TEST_CASE("a closed or latched store refuses the whole batch, every request not applied",
          "[batch]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto db = open_db(t.dir, true);
    auto const v = payload(0, 1);
    std::vector<insert_request> const reqs{{spread_key(1), v, 1}, {spread_key(2), v, 1}};
    db.close();
    auto const r = db.insert_batch(reqs, {.workers = 2});
    CHECK(r.not_applied == 2);
    CHECK(r.error == error_code::closed);
    CHECK(r.outcomes.size() == 2);
}

TEST_CASE("sync after a batch that rotated several slots succeeds, and the reopened store "
          "is the batch's", "[batch][durability]") {
    failpoints::scoped_reset const disarm;
    auto const w = mixed();
    temp_db t;
    content want;
    {
        auto db = open_db(t.dir, true);
        auto const r = db.insert_batch(w.requests, {.workers = 8});
        REQUIRE_FALSE(r.error.has_value());
        REQUIRE(db.sync().has_value());
        want = content_of(db);
        db.close();
    }
    auto db = open_db(t.dir, false);
    CHECK(content_of(db) == want);
    db.close();
}
