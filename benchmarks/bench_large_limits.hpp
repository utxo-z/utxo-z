// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file bench_large_limits.hpp
 * @brief How much work the large benchmark does, declared in one place.
 *
 * Every path of this binary has a maximum and every maximum is here. That is
 * the point, and it is a reaction to a specific failure: an earlier attempt at
 * a small run set two environment variables that `bench_large_ops` read, while
 * `main` ran the IBD simulation first — a `constexpr` fifty million inserts
 * that had never heard of them. The run wrote 12.8 GB and printed nothing
 * before it was stopped, and the limits it was given were real but reached
 * nothing.
 *
 * So the limits travel as an argument through every entry point rather than as
 * a constant inside one of them, and the binary prints what it is about to do
 * before it does any of it.
 */

#ifndef UTXOZ_BENCH_LARGE_LIMITS_HPP_
#define UTXOZ_BENCH_LARGE_LIMITS_HPP_

#include <algorithm>
#include <cstddef>

namespace bench_large {

struct large_limits {
    size_t ibd_inserts = 0;        ///< Phase 1 of the IBD simulation
    size_t single_gen_entries = 0; ///< The one-generation ops fixture
    size_t multi_gen_entries = 0;  ///< The rotating ops fixture
    size_t delete_batch = 0;       ///< Entries handed to one apply_deletes
    size_t find_lookups = 0;       ///< Phase 4 of the IBD simulation
    bool smoke = false;            ///< Printed, and used to pick the cadences

    /// Progress and sweep cadences have to scale with the work, or a small run
    /// never reaches the phase they gate: a deferred sweep every 500 000
    /// inserts does nothing at all in a run of 40 000, and the run would report
    /// a clean pass over a phase it skipped.
    [[nodiscard]] size_t deferred_interval() const {
        return std::max<size_t>(1, std::min<size_t>(500'000, ibd_inserts / 4));
    }
    [[nodiscard]] size_t progress_interval() const {
        return std::max<size_t>(1, std::min<size_t>(5'000'000, ibd_inserts / 2));
    }
};

/// What the official run measures. Unchanged by the smoke option.
[[nodiscard]] inline large_limits production_limits() {
    return large_limits{50'000'000, 15'000'000, 25'000'000, 100'000, 1'000'000, false};
}

/// Enough to exercise every path and nothing more. `cap` bounds every fixture.
[[nodiscard]] inline large_limits smoke_limits(size_t cap) {
    return large_limits{cap, cap, cap, std::min<size_t>(cap, 1'000),
                        std::min<size_t>(cap, 10'000), true};
}

} // namespace bench_large

#endif // UTXOZ_BENCH_LARGE_LIMITS_HPP_
