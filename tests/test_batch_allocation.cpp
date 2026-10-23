// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_batch_allocation.cpp
 * @brief insert_batch allocates nothing between storing an entry and counting it.
 *
 * A batch makes room for its bookkeeping before it writes: the height ranges,
 * each slot's value-size distribution, each slot's metadata record, and its own
 * accumulators. After that, counting a stored entry (on a worker) and folding
 * the counts in (on the calling thread) must not allocate, because an
 * allocation that failed there would leave an entry stored and not counted.
 *
 * The library marks both places with failpoints::no_allocation_region. This
 * binary replaces operator new so that, with the watch armed, any allocation
 * made inside a region fails and is counted. Its own binary because the
 * replacement is global; the main suite keeps the default allocator.
 *
 * The workloads are built to need room: thousands of height ranges the store
 * has never seen, value sizes no slot has seen, a second batch over a store
 * that already has some of both.
 */

#include <atomic>
#include <cstdlib>
#include <new>

#include <catch2/catch_test_macros.hpp>

#include "detail/durability.hpp"
#include "support/batch_workload.hpp"

using namespace utxoz;
using namespace utxoz::testing::batch;
using utxoz::detail::failpoints;
using utxoz::testing::spread_key;

namespace {

std::atomic<size_t> forbidden_allocations{0};

void* allocate(std::size_t n) {
    if (failpoints::in_no_allocation_region) {
        forbidden_allocations.fetch_add(1, std::memory_order_relaxed);
        throw std::bad_alloc();
    }
    if (void* p = std::malloc(n == 0 ? 1 : n)) return p;
    throw std::bad_alloc();
}

} // namespace

// Every unaligned form, new and delete alike, so that under a sanitizer each
// allocation is released by the same family that made it.
void* operator new(std::size_t n) { return allocate(n); }
void* operator new[](std::size_t n) { return allocate(n); }
void* operator new(std::size_t n, std::nothrow_t const&) noexcept {
    try { return allocate(n); } catch (...) { return nullptr; }
}
void* operator new[](std::size_t n, std::nothrow_t const&) noexcept {
    try { return allocate(n); } catch (...) { return nullptr; }
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::nothrow_t const&) noexcept { std::free(p); }
void operator delete[](void* p, std::nothrow_t const&) noexcept { std::free(p); }

namespace {

struct watch {
    watch() { forbidden_allocations = 0; failpoints::watch_no_allocation.store(true); }
    ~watch() { failpoints::watch_no_allocation.store(false); }
};

/// Heights in thousands of ranges, one each, and value sizes across the whole
/// of classes 0 to 2: room the statistics have to make, before the batch.
workload spread_out(uint64_t first, uint64_t count) {
    workload w;
    for (uint64_t i = 0; i != count; ++i) {
        uint64_t const n = first + i;
        size_t const cls = size_t(i % 3);
        w.add(spread_key(n), cls, n, uint32_t(n * height_range_stats::range_size + i % 7));
        // A size of its own inside the class: from just above the previous
        // class's capacity up to this one's.
        size_t const low = cls == 0 ? 1 : container_capacities[cls - 1] + 1;
        size_t const high = container_capacities[cls];
        w.values.back().resize(low + n % (high - low + 1));
    }
    w.bind();
    return w;
}

figures sequential_figures(std::vector<workload const*> const& batches) {
    temp_db t;
    auto db = open_db(t.dir, true);
    for (auto const* w : batches) insert_sequentially(db, *w);
    auto f = figures_of(db);
    db.close();
    return f;
}

} // namespace

TEST_CASE("the seam itself: an allocation inside an armed region fails and is counted",
          "[batch][allocation]") {
    failpoints::scoped_reset const disarm;
    watch const armed;
    {
        failpoints::no_allocation_region const region;
        CHECK_THROWS_AS(std::make_unique<int>(1), std::bad_alloc);
    }
    CHECK(forbidden_allocations == 1);
    CHECK_NOTHROW(std::make_unique<int>(1));   // outside a region
    CHECK(forbidden_allocations == 1);
}

TEST_CASE("disarmed, a region marks nothing", "[batch][allocation]") {
    failpoints::scoped_reset const disarm;
    forbidden_allocations = 0;
    failpoints::no_allocation_region const region;
    CHECK_NOTHROW(std::make_unique<int>(1));
    CHECK(forbidden_allocations == 0);
}

TEST_CASE("a batch counts what it stores without allocating, and folds without allocating",
          "[batch][allocation]") {
    failpoints::scoped_reset const disarm;
    auto const first = spread_out(0, 6000);
    auto const second = spread_out(3000, 6000);   // half seen, half new
    auto const mix = mixed();
    auto const want_one = sequential_figures({&first});
    auto const want_all = sequential_figures({&first, &second, &mix});

    for (unsigned workers : {1u, 2u, 4u, 8u}) {
        DYNAMIC_SECTION("workers " << workers) {
            temp_db t;
            auto db = open_db(t.dir, true);
            {
                watch const armed;
                auto const r = db.insert_batch(first.requests, {.workers = workers});
                CHECK(forbidden_allocations == 0);
                CHECK_FALSE(r.error.has_value());
            }
            CHECK(figures_of(db) == want_one);
            {
                watch const armed;
                auto const r2 = db.insert_batch(second.requests, {.workers = workers});
                auto const r3 = db.insert_batch(mix.requests, {.workers = workers});
                CHECK(forbidden_allocations == 0);
                CHECK_FALSE(r2.error.has_value());
                CHECK_FALSE(r3.error.has_value());
            }
            CHECK(figures_of(db) == want_all);
            db.close();
        }
    }
}
