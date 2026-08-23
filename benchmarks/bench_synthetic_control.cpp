// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file bench_synthetic_control.cpp
 * @brief A difference the harness is supposed to find, with nothing else changed.
 *
 * ## Why not just edit a benchmark
 *
 * The first synthetic control was a source edit — `simulated IBD` doing twenty
 * lookups per block instead of five — built into a second binary. That works,
 * and both mac and fedora found it. What it cannot do is prove the *absence* of
 * an effect anywhere else, because the two binaries were not otherwise the same:
 * on mac 724 809 of 1 346 592 bytes differed, across 47 181 separate regions.
 * The relink moved code everywhere. So when `find miss` came out 1.85% slower on
 * mac, "the change cannot have touched it" was not available as an argument.
 *
 * Here the two sides are the same file with the same SHA-256, and the difference
 * is a number read from the environment. Every other benchmark executes
 * literally the same bytes, so a finding in one of them is the instrument
 * talking, not the linker.
 *
 * ## Why it runs last
 *
 * Identical instructions are not the same as identical conditions. This
 * benchmark's extra work touches caches, the page cache and the allocator, and a
 * benchmark that ran *after* it would meet a machine it had helped to change —
 * which is exactly the confound the design is trying to remove. Registered last,
 * every negative control has already been measured before the difference is
 * applied.
 *
 * ## Configuration
 *
 * `UTXOZ_BENCH_SYNTHETIC_LOOKUPS`, default 5, added to fifty lookups every side
 * performs. Five against twenty is therefore 55 against 70 — a difference of
 * 27.3%, not 4x. Parsed strictly and once, before
 * any measurement exists: a value that is not a positive integer stops the
 * process rather than silently reverting to the default, because a control that
 * quietly measured the wrong thing would be worse than no control. The value is
 * captured into a `const` and the loop reads that, never the environment.
 */

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nanobench.h>

#include "bench_common.hpp"

namespace bench {

namespace {

constexpr uint32_t default_lookups = 5;

/// Work every side does, whatever the configuration says.
///
/// Without it the two sides differ by 20/5 = 4x, which is far larger than any
/// geometry change R3b will produce and so tests the harness at an effect size
/// nobody needs it to resolve. Fifty fixed lookups put the difference at
/// 70/55 = +27.3%, the same order as the source-edit control that mac and
/// fedora already measured (+51% and +27%), and close to the region where the
/// instrument's resolution actually matters.
constexpr uint32_t fixed_lookups = 50;

} // namespace

uint32_t synthetic_lookups_from_environment() {
    char const* raw = std::getenv("UTXOZ_BENCH_SYNTHETIC_LOOKUPS");
    if (raw == nullptr || *raw == '\0') return default_lookups;

    std::string_view text{raw};
    uint32_t value = 0;
    auto const [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || value == 0) {
        throw std::runtime_error(
            "UTXOZ_BENCH_SYNTHETIC_LOOKUPS must be a positive integer, got '"
            + std::string(text) + "'");
    }
    return value;
}

void register_synthetic_control(ankerl::nanobench::Bench& bench, uint32_t lookups) {
    BenchFixture f;
    auto value = make_test_value(43);

    // A small store to look into, built once and identical on both sides: what
    // the control varies is how many times it is asked, and nothing else.
    constexpr uint32_t entries = 20000;
    for (uint32_t i = 0; i < entries; ++i) {
        (void)f.db->insert(make_test_key(i, 0), value, 100 + i / 200);
    }

    // Captured by value. The loop below never reads the environment; the only
    // difference between the two sides at run time is this integer, and the only
    // difference in the loop is that the counter compares against a variable
    // rather than a literal — the same instruction on both sides.
    uint32_t const asked = fixed_lookups + lookups;
    uint32_t probe = 0;
    bench.run("synthetic control: scaled lookups", [&] {
        for (uint32_t i = 0; i < asked; ++i) {
            probe = (probe + 7919) % entries;
            ankerl::nanobench::doNotOptimizeAway(
                f.db->find(make_test_key(probe, 0), 100 + entries / 200));
        }
    });
}

} // namespace bench
