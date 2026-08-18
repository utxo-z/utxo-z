// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_uniqueness.cpp
 * @brief That the verdict is exact, bounded, and free of policy.
 *
 * A correct UTXO set holds each outpoint at most once. Nothing in the write path
 * enforces it, so every case below builds the violation deliberately and asserts
 * an exact verdict and exact counts.
 *
 * Three things are checked that are easy to get wrong and impossible to see from
 * a passing total: that a hash collision is not treated as equality, that the
 * answer does not depend on the order the generations were read in, and that a
 * report never carries an outpoint unless it was asked to.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <limits>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <boost/json.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fmt/format.h>

#include <utxoz/census.hpp>
#include <utxoz/database.hpp>
#include <utxoz/types.hpp>
#include <utxoz/uniqueness.hpp>

#include "detail/durability.hpp"
#include "detail/file_cache.hpp"
#include "detail/budget_meter.hpp"
#include "detail/distinct_keys.hpp"

namespace fs = std::filesystem;
using namespace utxoz;
using utxoz::detail::failpoints;

namespace {

struct temp_db {
    temp_db() {
        static std::atomic<uint64_t> counter{0};
        auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        dir = fmt::format("./test_unique_{}_{}_{}", getpid(), ts, counter.fetch_add(1));
        fs::remove_all(dir);
    }
    ~temp_db() { std::error_code ec; fs::remove_all(dir, ec); }
    temp_db(temp_db const&) = delete;
    temp_db& operator=(temp_db const&) = delete;
    fs::path dir;
};

raw_outpoint key_of(uint64_t n) {
    raw_outpoint k{};
    std::memcpy(k.data(), &n, sizeof(n));
    return k;
}

std::vector<uint8_t> payload_for(size_t index, uint8_t fill = 0x5A) {
    return std::vector<uint8_t>(index == 0 ? 8 : container_capacities[index - 1] + 1, fill);
}

/// A budget with room for `records` records and nothing to spare, derived from
/// what a roomy run reported rather than guessed. One definition, because several
/// copies of this drift and then the cases are testing different thresholds while
/// looking identical.
uint64_t budget_holding(uniqueness_report const& roomy, uint64_t records) {
    return (roomy.fixed_overhead + records * roomy.record_bytes) * 10 / 9 + 1024;
}

verify_options budget_of(uint64_t bytes) {
    verify_options o;
    o.memory_budget = bytes;
    return o;
}

uniqueness_report verify_of(fs::path const& dir, verify_options const& options = {}) {
    // Checked before it is dereferenced: an open that failed here would otherwise
    // be undefined behaviour reported as whatever the next assertion happened to
    // see, which is the least useful way for a fixture to break.
    auto opened = full_db::open_for_testing(dir, false);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);
    auto report = db.verify_unique_outpoints(options);
    REQUIRE(report.has_value());
    auto out = *report;
    db.close();
    return out;
}

/// How many locations a finding has in the given class.
uint64_t in_class(duplicate_finding const& f, uint32_t klass) {
    return uint64_t(std::ranges::count_if(f.locations,
        [&](auto const& l) { return l.container_class == klass; }));
}

} // namespace

// =============================================================================
// The verdict
// =============================================================================

TEST_CASE("a database with no duplicates is unique", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 40; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(i % 5), 800000).has_value());
        }
        db.close();
    }

    auto const r = verify_of(t.dir);
    CHECK(r.unique);
    CHECK(r.storage_mode == "full");
    CHECK(r.physical_entries == 40);
    CHECK(r.distinct_outpoints == 40);
    CHECK(r.duplicate_copies == 0);
    CHECK(r.keys_with_multiple_copies == 0);
    CHECK(r.max_copies_for_one_key == 1);
    CHECK(r.findings.empty());
    CHECK(r.findings_omitted == 0);
}

TEST_CASE("an empty database is unique, not an error", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        db.close();
    }

    auto const r = verify_of(t.dir);
    CHECK(r.unique);
    CHECK(r.physical_entries == 0);
    CHECK(r.distinct_outpoints == 0);
    CHECK(r.max_copies_for_one_key == 0);
    // The planning pass still ran, and there were no groups to walk.
    CHECK(r.planning_passes == 1);
    CHECK(r.data_passes == 0);
}

TEST_CASE("a key in two generations of one class is not unique", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const shared = key_of(0xAA01);
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(shared, payload_for(0), 800000).has_value());
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(2), payload_for(0), 800000).has_value());
        // The same outpoint again, now in the active generation.
        REQUIRE(db.insert(shared, payload_for(0), 800100).has_value());
        db.close();
    }

    auto const r = verify_of(t.dir);
    CHECK_FALSE(r.unique);
    CHECK(r.physical_entries == 3);
    CHECK(r.distinct_outpoints == 2);
    CHECK(r.duplicate_copies == 1);
    CHECK(r.keys_with_multiple_copies == 1);
    CHECK(r.max_copies_for_one_key == 2);

    // The dimension, from the locations rather than from a taxonomy: one class,
    // two generations, one of them the active one.
    REQUIRE(r.findings.size() == 1);
    auto const& f = r.findings.front();
    CHECK(f.multiplicity == 2);
    REQUIRE(f.locations.size() == 2);
    CHECK(f.locations_omitted == 0);
    CHECK(f.locations[0].container_class == f.locations[1].container_class);
    CHECK(f.locations[0].generation != f.locations[1].generation);
    CHECK(std::ranges::count_if(f.locations, [](auto const& l) { return l.active; }) == 1);
}

TEST_CASE("a key in two historical generations and nowhere active", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const shared = key_of(0xBB02);
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(shared, payload_for(0), 800000).has_value());
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(2), payload_for(0), 800000).has_value());
        REQUIRE(db.insert(shared, payload_for(0), 800100).has_value());
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(3), payload_for(0), 800200).has_value());
        db.close();
    }

    auto const r = verify_of(t.dir);
    CHECK_FALSE(r.unique);
    CHECK(r.distinct_outpoints == 3);
    CHECK(r.keys_with_multiple_copies == 1);
    REQUIRE(r.findings.size() == 1);
    auto const& f = r.findings.front();
    CHECK(f.multiplicity == 2);
    CHECK(std::ranges::none_of(f.locations, [](auto const& l) { return l.active; }));
}

TEST_CASE("a key in two different classes is not unique", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const shared = key_of(0xCC03);
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(shared, payload_for(0), 800000).has_value());
        REQUIRE(db.insert(shared, payload_for(3), 800000).has_value());
        db.close();
    }

    auto const r = verify_of(t.dir);
    CHECK_FALSE(r.unique);
    CHECK(r.physical_entries == 2);
    CHECK(r.distinct_outpoints == 1);
    CHECK(r.keys_with_multiple_copies == 1);

    // Compaction is per class and can never see this one, which is why the class
    // is part of a location rather than left out.
    REQUIRE(r.findings.size() == 1);
    auto const& f = r.findings.front();
    REQUIRE(f.locations.size() == 2);
    CHECK(f.locations[0].container_class != f.locations[1].container_class);
    CHECK(in_class(f, 0) == 1);
    CHECK(in_class(f, 3) == 1);
    // Both copies are in an active generation.
    CHECK(std::ranges::all_of(f.locations, [](auto const& l) { return l.active; }));
}

TEST_CASE("a key repeated across classes and across generations at once", "[unique]") {
    // Both dimensions in one key. The verdict does not change and neither does
    // the count — a key stored four times is one key — but the locations have to
    // describe both, because that is what tells somebody where to look.
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const shared = key_of(0xDD04);
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(shared, payload_for(0), 800000).has_value());   // class 0, gen 0
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(2), payload_for(0), 800000).has_value());
        REQUIRE(db.insert(shared, payload_for(0), 800100).has_value());   // class 0, gen 1 (active)
        REQUIRE(db.insert(shared, payload_for(3), 800200).has_value());   // class 3, active
        db.close();
    }

    auto const r = verify_of(t.dir);
    CHECK_FALSE(r.unique);
    CHECK(r.physical_entries == 4);
    CHECK(r.distinct_outpoints == 2);
    CHECK(r.duplicate_copies == 2);
    CHECK(r.keys_with_multiple_copies == 1);
    CHECK(r.max_copies_for_one_key == 3);

    REQUIRE(r.findings.size() == 1);
    auto const& f = r.findings.front();
    CHECK(f.multiplicity == 3);
    REQUIRE(f.locations.size() == 3);
    CHECK(in_class(f, 0) == 2);          // two generations of one class
    CHECK(in_class(f, 3) == 1);          // and another class
    CHECK(std::ranges::count_if(f.locations, [](auto const& l) { return l.active; }) == 2);
    CHECK(std::ranges::count_if(f.locations, [](auto const& l) { return ! l.active; }) == 1);
}

TEST_CASE("five copies of one key", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const shared = key_of(0xEE05);
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (int i = 0; i < 5; ++i) {
            REQUIRE(db.insert(shared, payload_for(0), 800000 + uint32_t(i)).has_value());
            failpoints::force_rotations.store(1, std::memory_order_relaxed);
            REQUIRE(db.insert(key_of(100 + uint64_t(i)), payload_for(0), 800000).has_value());
        }
        db.close();
    }

    auto const r = verify_of(t.dir);
    CHECK_FALSE(r.unique);
    CHECK(r.distinct_outpoints == 6);
    CHECK(r.keys_with_multiple_copies == 1);
    CHECK(r.max_copies_for_one_key == 5);
    CHECK(r.duplicate_copies == 4);
    REQUIRE(r.findings.size() == 1);
    CHECK(r.findings.front().multiplicity == 5);
    CHECK(r.findings.front().locations.size() == 5);
}

// =============================================================================
// A collision is not an equality
// =============================================================================

TEST_CASE("two keys that share a partition prefix are two keys", "[unique]") {
    // The failure this guards against is the cheap one: grouping by hash and then
    // treating a group as a key. Found by search rather than asserted from a
    // constant, because the hash is ours and a constant would go stale the moment
    // it changed — the search would then simply find a different pair.
    failpoints::scoped_reset const disarm;

    constexpr size_t shared_bits = 32;
    auto const prefix = [](raw_outpoint const& k) {
        return detail::partition_hash(k) >> (64 - shared_bits);
    };

    raw_outpoint first{}, second{};
    bool found = false;
    // A map rather than a scan. The birthday bound puts the first collision at
    // around 2^16 draws, and a linear search over what has been seen makes that
    // a couple of billion comparisons — slow in Release and much worse under the
    // sanitizers, for a case whose subject is not the search.
    std::unordered_map<uint64_t, uint64_t> seen;      // prefix -> source number
    seen.reserve(1u << 17);
    for (uint64_t n = 1; n < (1u << 19) && ! found; ++n) {
        auto const key = key_of(n);
        auto const [it, fresh] = seen.emplace(prefix(key), n);
        if ( ! fresh) {
            first = key_of(it->second);
            second = key;
            found = true;
        }
    }
    REQUIRE(found);
    REQUIRE(prefix(first) == prefix(second));
    // Same partition, different keys — which is the whole point.
    REQUIRE(first != second);

    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(first, payload_for(0), 800000).has_value());
        REQUIRE(db.insert(second, payload_for(0), 800000).has_value());
        db.close();
    }

    auto const r = verify_of(t.dir);
    CHECK(r.unique);
    CHECK(r.physical_entries == 2);
    CHECK(r.distinct_outpoints == 2);
    CHECK(r.findings.empty());
}

// =============================================================================
// The sample is bounded, and it withholds by default
// =============================================================================

TEST_CASE("without the opt-in a report carries no outpoint at all", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const shared = key_of(0x0BADF00Dull);
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(shared, payload_for(0), 800000).has_value());
        REQUIRE(db.insert(shared, payload_for(3), 800000).has_value());
        db.close();
    }

    auto const r = verify_of(t.dir);          // the default
    CHECK_FALSE(r.unique);
    CHECK_FALSE(r.outpoints_included);
    REQUIRE(r.findings.size() == 1);
    CHECK_FALSE(r.findings.front().outpoint.has_value());
    // Where it counts: the rendered documents. A struct field nobody serialises
    // is not what leaks; the JSON that gets pasted into an issue is.
    auto const json = to_json(r);
    CHECK(json.find("\"outpoint\"") == std::string::npos);
    CHECK(json.find("\"outpoints_included\": false") != std::string::npos);
    auto const text = to_text(r);
    // The hex of the key, in either case, in either document.
    CHECK(text.find("0df0ad0b") == std::string::npos);
    CHECK(json.find("0df0ad0b") == std::string::npos);
}

TEST_CASE("both renderings carry the per-pass cost", "[unique]") {
    // `duration_per_pass_ms` is published in the report, and a document that
    // calls itself machine-readable and then leaves a field out is not one. The
    // per-pass figure is also the one that matters here: a small budget re-reads
    // the whole database once per partition group, and where the time went is
    // what tells a reader whether to raise it.
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 300; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        db.close();
    }

    auto const roomy = verify_of(t.dir);
    uint64_t const cramped_budget = budget_holding(roomy, 40);
    auto const r = verify_of(t.dir, budget_of(cramped_budget));

    // One planning pass and one per partition group.
    REQUIRE(r.data_passes > 1);
    REQUIRE(r.duration_per_pass_ms.size() == r.planning_passes + r.data_passes);

    auto const json = to_json(r);
    CHECK(json.find("\"duration_per_pass_ms\"") != std::string::npos);
    std::error_code ec;
    auto const parsed = boost::json::parse(json, ec);
    REQUIRE_FALSE(ec);
    auto const& passes = parsed.at("walk").at("duration_per_pass_ms").as_array();
    CHECK(passes.size() == r.duration_per_pass_ms.size());

    CHECK(to_text(r).find("per pass:") != std::string::npos);
}

TEST_CASE("the opt-in puts the real outpoint in, and only then", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const shared = key_of(0x0BADF00Dull);
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(shared, payload_for(0), 800000).has_value());
        REQUIRE(db.insert(shared, payload_for(3), 800000).has_value());
        db.close();
    }

    verify_options options;
    options.include_outpoints = true;
    auto const r = verify_of(t.dir, options);
    CHECK(r.outpoints_included);
    REQUIRE(r.findings.size() == 1);
    REQUIRE(r.findings.front().outpoint.has_value());
    // The key itself, not a hash of it and not a prefix: a finding exists so that
    // somebody can go and look at the entries, and a substitute cannot be fed
    // back to find().
    CHECK(*r.findings.front().outpoint == shared);

    auto const json = to_json(r);
    CHECK(json.find("\"outpoints_included\": true") != std::string::npos);
    CHECK(json.find("0df0ad0b") != std::string::npos);
}

TEST_CASE("the sample honours max_findings and max_locations_per_finding", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    constexpr uint64_t duplicated_keys = 6;
    constexpr uint64_t copies_each = 4;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t k = 0; k < duplicated_keys; ++k) {
            for (uint64_t c = 0; c < copies_each; ++c) {
                REQUIRE(db.insert(key_of(0x7000 + k), payload_for(0),
                                  800000 + uint32_t(c)).has_value());
                failpoints::force_rotations.store(1, std::memory_order_relaxed);
                REQUIRE(db.insert(key_of(0x9000 + k * 10 + c), payload_for(0),
                                  800000).has_value());
            }
        }
        db.close();
    }

    verify_options options;
    options.max_findings = 2;
    options.max_locations_per_finding = 3;
    auto const r = verify_of(t.dir, options);

    CHECK_FALSE(r.unique);
    // The counts are of the whole database and are not capped by the sample.
    CHECK(r.keys_with_multiple_copies == duplicated_keys);
    CHECK(r.max_copies_for_one_key == copies_each);

    // The sample is.
    CHECK(r.findings.size() == 2);
    CHECK(r.findings_omitted == duplicated_keys - 2);
    for (auto const& f : r.findings) {
        CHECK(f.multiplicity == copies_each);       // the true figure, uncapped
        CHECK(f.locations.size() == 3);             // the sample of it
        CHECK(f.locations_omitted == copies_each - 3);
    }

    // And a truncated sample never reads as a complete one.
    auto const json = to_json(r);
    CHECK(json.find(fmt::format("\"omitted\": {}", duplicated_keys - 2)) != std::string::npos);
}

TEST_CASE("the verdict comes from the counts, not from the sample", "[unique]") {
    // With no sample at all the database is still not unique. The verdict is a
    // fact about the database and the findings are a fact about the report, and
    // an operator who asks for no findings — because a report that names nothing
    // is the one they are allowed to paste — must still be told the answer.
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(key_of(1), payload_for(0), 800000).has_value());
        REQUIRE(db.insert(key_of(1), payload_for(3), 800000).has_value());
        db.close();
    }

    verify_options silent;
    silent.max_findings = 0;
    auto const r = verify_of(t.dir, silent);
    CHECK_FALSE(r.unique);
    CHECK(r.findings.empty());
    CHECK(r.keys_with_multiple_copies == 1);
    CHECK(r.findings_omitted == 1);
    CHECK(r.duplicate_copies == 1);
    CHECK(to_json(r).find("\"unique\": false") != std::string::npos);
}

// =============================================================================
// The budget is a limit, not a wish
// =============================================================================

TEST_CASE("a small budget forces several passes and changes nothing it reports",
          "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 300; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        // One duplicate, so the interesting counters are not all zero.
        REQUIRE(db.insert(key_of(7), payload_for(2), 800000).has_value());
        db.close();
    }

    auto const roomy = verify_of(t.dir);

    // Derived from what the walk itself reports rather than guessed: a budget
    // holding about forty records at a time. Guessing would make this case break
    // the day the fixed overhead changed, for a reason that has nothing to do
    // with what it is testing.
    uint64_t const cramped_budget = budget_holding(roomy, 40);
    auto const cramped = verify_of(t.dir, budget_of(cramped_budget));

    CHECK(cramped.partitions > roomy.partitions);
    CHECK(cramped.data_passes > 1);

    // And every figure is the same. The partitioning is a way of fitting the work
    // in memory, not a thing the answer depends on.
    CHECK(cramped.unique == roomy.unique);
    CHECK(cramped.physical_entries == roomy.physical_entries);
    CHECK(cramped.distinct_outpoints == roomy.distinct_outpoints);
    CHECK(cramped.duplicate_copies == roomy.duplicate_copies);
    CHECK(cramped.keys_with_multiple_copies == roomy.keys_with_multiple_copies);
    CHECK(cramped.max_copies_for_one_key == roomy.max_copies_for_one_key);
    CHECK(cramped.findings.size() == roomy.findings.size());

    // The re-reading is real and is reported rather than hidden: more passes
    // means the data was walked more times.
    CHECK(cramped.entries_examined_total > roomy.entries_examined_total);
    CHECK(cramped.generations_visited > roomy.generations_visited);
}

TEST_CASE("the peak the walk reports is inside the budget it was given", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 200; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        db.close();
    }

    uint64_t const budget = 8u * 1024 * 1024;
    auto const r = verify_of(t.dir, budget_of(budget));
    CHECK(r.memory_budget == budget);
    CHECK(r.record_bytes == sizeof(detail::stored_copy));
    CHECK(r.record_bytes > 0);
    // What the vector actually reserved, not what it was asked for.
    CHECK(r.record_capacity > 0);
    CHECK(r.estimated_peak > 0);
    CHECK(r.estimated_peak <= budget);
    CHECK(r.fixed_overhead > 0);
    CHECK(r.headroom == budget / 10);
    // Records, overhead and headroom account for the peak.
    CHECK(r.estimated_peak == r.record_capacity * r.record_bytes + r.fixed_overhead + r.headroom);
}

TEST_CASE("a budget that cannot hold the work is refused, not approximated",
          "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 20; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));

    // Smaller than the fixed overhead: there is no room for a single record.
    auto const refused = db.verify_unique_outpoints(budget_of(1024));
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() == error_code::insufficient_space);

    // And the same database with room works, which is what makes the refusal
    // about the budget rather than about the data.
    CHECK(db.verify_unique_outpoints().has_value());
    db.close();
}

TEST_CASE("a pass limit is refused before the work, not during it", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 300; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto const roomy = *db.verify_unique_outpoints();
    uint64_t const cramped_budget = budget_holding(roomy, 40);

    auto options = budget_of(cramped_budget);
    options.max_data_passes = 1;
    auto const refused = db.verify_unique_outpoints(options);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() == error_code::insufficient_space);

    options.max_data_passes = 64;
    CHECK(db.verify_unique_outpoints(options).has_value());
    db.close();
}

TEST_CASE("a prefix too large for the budget is refused, not subdivided", "[unique]") {
    // The prefix width is fixed at twelve bits and there is no subdivision. A
    // prefix whose entries do not fit is refused conservatively — the supported
    // answer is a larger budget, and the refusal is what says so.
    //
    // Many copies of one key are the way to build such a prefix on purpose: they
    // all hash alike, so they are all in one prefix however wide it is.
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const repeated = key_of(0x5150);
    constexpr int copies = 40;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (int i = 0; i < copies; ++i) {
            REQUIRE(db.insert(repeated, payload_for(0), 800000 + uint32_t(i)).has_value());
            failpoints::force_rotations.store(1, std::memory_order_relaxed);
            REQUIRE(db.insert(key_of(2000 + uint64_t(i)), payload_for(0), 800000).has_value());
        }
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto const roomy = *db.verify_unique_outpoints();
    REQUIRE(roomy.max_copies_for_one_key == copies);

    // Room for a quarter of that one prefix. No number of passes can make it fit,
    // because the prefix cannot be divided.
    uint64_t const too_small = budget_holding(roomy, copies / 4);
    auto const refused = db.verify_unique_outpoints(budget_of(too_small));
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() == error_code::insufficient_space);

    // And the refusal is about the budget: the same database, given room, answers.
    CHECK(db.verify_unique_outpoints().has_value());
    db.close();
}

TEST_CASE("a partition that is mostly copies of one key stays inside the budget",
          "[unique]") {
    // The shape that broke the guarantee before: the walk used to copy each
    // duplicate group into a second vector, so a group as large as the partition
    // doubled the storage — sixteen bytes on top of fifty-six, twenty-nine per
    // cent, against ten per cent of headroom. The group is a view now, and this
    // is the case that would have caught it.
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const repeated = key_of(0xABCDEF);
    constexpr int copies = 60;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (int i = 0; i < copies; ++i) {
            REQUIRE(db.insert(repeated, payload_for(0), 800000 + uint32_t(i)).has_value());
            failpoints::force_rotations.store(1, std::memory_order_relaxed);
            REQUIRE(db.insert(key_of(1000 + uint64_t(i)), payload_for(0), 800000).has_value());
        }
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto const roomy = *db.verify_unique_outpoints();
    REQUIRE(roomy.max_copies_for_one_key == copies);

    // A budget with room for a little more than the whole group, so the group and
    // the records it lives in are the largest thing in memory at once.
    uint64_t const cramped_budget =
        budget_holding(roomy, copies + 20);
    auto const cramped = db.verify_unique_outpoints(budget_of(cramped_budget));
    REQUIRE(cramped.has_value());

    // The same answer, and the whole of the peak accounted for: records, the
    // fixed overhead and the headroom, with nothing else allocated beside them.
    CHECK(cramped->distinct_outpoints == roomy.distinct_outpoints);
    CHECK(cramped->max_copies_for_one_key == copies);
    CHECK(cramped->estimated_peak <= cramped_budget);
    CHECK(cramped->estimated_peak == cramped->record_capacity * cramped->record_bytes
                                      + cramped->fixed_overhead + cramped->headroom);
    db.close();
}

TEST_CASE("what the ceiling covers is a list, not a claim", "[unique]") {
    // The composition of the fixed overhead, pinned. The first version of this
    // budget was an estimate because things that were "small" were left out of
    // it; this is what stops that happening again quietly. If a term is added,
    // this case has to be told about it.
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 20; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        db.close();
    }

    // Every count, including ones that are not a power of two and not one less
    // than one. 17 is the case that matters: a vector grown to hold 17 elements
    // ends up with capacity 32 on every implementation this builds against, and
    // briefly holds a block of 16 and a block of 32 at once. Nothing here grows,
    // so the storage is exactly what was asked for — and this is what says so.
    auto const counts = GENERATE(uint64_t(1), uint64_t(16), uint64_t(17), uint64_t(33));

    verify_options options;
    options.max_findings = counts;
    auto const r = verify_of(t.dir, options);

    // What the sample actually occupies, flat and reserved once, plus what the
    // public report will occupy when it is built from it.
    uint64_t const collected =
        options.max_findings * sizeof(detail::sampled_finding)
        + options.max_findings * options.max_locations_per_finding
              * sizeof(duplicate_location);
    // Everything the meter holds before the first record: the sort's allowance —
    // the one figure left, because there is no allocator to route a sort's own
    // stack through — the planning counters, the group list, and the sample. The
    // counters and the group list are metered now rather than covered by a
    // hand-computed allowance, so their exact sizes appear here.
    uint64_t const counters = detail::planning_prefixes * sizeof(uint64_t);
    uint64_t const group_list =
        detail::planning_prefixes * sizeof(detail::partition_group);
    CHECK(r.fixed_overhead == detail::sort_allowance + counters + group_list + collected);

    // The returned report is outside the ceiling, reported rather than metered,
    // and it is the shape that is bounded: at most max_findings findings of at
    // most max_locations_per_finding locations each.
    CHECK(r.report_bytes_estimate ==
          options.max_findings * sizeof(duplicate_finding)
          + options.max_findings * options.max_locations_per_finding
                * sizeof(duplicate_location)
          + r.duration_per_pass_ms.size() * sizeof(uint64_t));
    // And it does not grow with the database, which is the whole reason it can be
    // left outside: the same figure whatever is stored.
    CHECK(verify_of(t.dir, options).report_bytes_estimate == r.report_bytes_estimate);

    // And the headroom is held back rather than described: the ceiling the walk
    // could reach is the budget without it.
    CHECK(r.headroom == r.memory_budget / 10);
    CHECK(r.estimated_peak <= r.memory_budget);
    CHECK(r.estimated_peak == r.record_capacity * r.record_bytes
                              + r.fixed_overhead + r.headroom);
}

TEST_CASE("a sample just past a growth boundary is still inside the budget", "[unique]") {
    // The concrete case: 17 findings, each with locations, actually collected
    // rather than merely accounted for. If the sample grew the way a plain vector
    // does, the storage held would exceed what the report claims — so the report
    // is checked against the ceiling with the sample full.
    failpoints::scoped_reset const disarm;
    temp_db t;
    constexpr uint64_t duplicated = 20;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t k = 0; k < duplicated; ++k) {
            REQUIRE(db.insert(key_of(0x8000 + k), payload_for(0), 800000).has_value());
            REQUIRE(db.insert(key_of(0x8000 + k), payload_for(3), 800000).has_value());
        }
        db.close();
    }

    verify_options options;
    options.max_findings = 17;
    options.max_locations_per_finding = 2;
    auto const r = verify_of(t.dir, options);

    CHECK_FALSE(r.unique);
    CHECK(r.keys_with_multiple_copies == duplicated);
    // The sample is full, which is the state the accounting has to survive.
    REQUIRE(r.findings.size() == 17);
    CHECK(r.findings_omitted == duplicated - 17);
    for (auto const& f : r.findings) CHECK(f.locations.size() == 2);

    CHECK(r.estimated_peak <= r.memory_budget);
    CHECK(r.estimated_peak == r.record_capacity * r.record_bytes
                              + r.fixed_overhead + r.headroom);
}

TEST_CASE("the meter refuses before anything is allocated", "[unique]") {
    // The property the whole budget rests on: a request past the ceiling is
    // turned down, and the system is never asked. A meter that noticed the
    // overrun afterwards would be the arithmetic this replaced.
    using utxoz::detail::budget_meter;

    budget_meter meter(1000);
    CHECK(meter.ceiling() == 1000);
    CHECK(meter.acquire(600));
    CHECK(meter.outstanding() == 600);
    CHECK(meter.peak() == 600);

    // One byte past what is left is refused, and takes nothing.
    CHECK_FALSE(meter.acquire(401));
    CHECK(meter.outstanding() == 600);
    CHECK(meter.peak() == 600);
    CHECK(meter.refusals() == 1);

    // Exactly what is left fits.
    CHECK(meter.acquire(400));
    CHECK(meter.outstanding() == 1000);
    CHECK(meter.peak() == 1000);

    // Giving back lowers what is outstanding and not the peak: the peak is what
    // was held at once, which is the figure the report is about.
    meter.release(1000);
    CHECK(meter.outstanding() == 0);
    CHECK(meter.peak() == 1000);

    // And a request no ceiling could satisfy is a refusal, not a wrap.
    budget_meter small(8);
    CHECK_FALSE(small.acquire(std::numeric_limits<uint64_t>::max()));
    CHECK(small.outstanding() == 0);
}

TEST_CASE("a count that cannot be indexed is refused, at every width", "[unique]") {
    // The guard exists for 32-bit targets, and 32-bit is exactly where no test
    // runs: the wasm job builds the library and does not run ctest, and this suite
    // could not run there anyway — it needs a real filesystem and memory-mapped
    // files. A case written as `if constexpr (sizeof(size_t) < 8)` would compile
    // there and execute nowhere, which is not coverage.
    //
    // So the width is a parameter and this drives it from here, on a platform that
    // actually executes. Production calls the wrappers that take no width at all.
    using utxoz::detail::count_fits_addressable;
    using utxoz::detail::sample_fits_addressable;

    constexpr uint64_t wasm32 = 0xFFFFFFFFull;
    constexpr uint64_t sixteen = 0xFFFFull;
    constexpr uint64_t sixty_four = std::numeric_limits<uint64_t>::max();

    // The record count of a partition group narrows through the same cast, and
    // this is the primitive that guards it.
    CHECK(count_fits_addressable(wasm32, wasm32));
    CHECK_FALSE(count_fits_addressable(wasm32 + 1, wasm32));
    CHECK(count_fits_addressable(sixteen, sixteen));
    CHECK_FALSE(count_fits_addressable(sixteen + 1, sixteen));
    CHECK(count_fits_addressable(sixty_four, sixty_four));

    // The sample's two counts. Exactly the maximum fits; one past it does not, on
    // either of them.
    CHECK(sample_fits_addressable(wasm32, wasm32, wasm32));
    CHECK_FALSE(sample_fits_addressable(wasm32 + 1, 1, wasm32));
    CHECK_FALSE(sample_fits_addressable(1, wasm32 + 1, wasm32));

    // The same shape at a width where the numbers are easy to read by eye.
    CHECK(sample_fits_addressable(sixteen, sixteen, sixteen));
    CHECK_FALSE(sample_fits_addressable(sixteen + 1, sixteen, sixteen));
    CHECK_FALSE(sample_fits_addressable(sixteen, sixteen + 1, sixteen));

    // And at 64 bits nothing representable can be refused, which is why a 64-bit
    // build sees these guards do nothing.
    CHECK(sample_fits_addressable(sixty_four, sixty_four, sixty_four));

    // The wiring: the defaults are addressable everywhere, through the wrappers
    // production actually calls.
    verify_options const defaults;
    CHECK(utxoz::detail::sample_fits_platform(
        defaults.max_findings,
        defaults.max_findings * defaults.max_locations_per_finding));
    CHECK(utxoz::detail::count_fits_platform(1000000));
}

TEST_CASE("a sample larger than a vector can hold is refused, not thrown", "[unique]") {
    // Fitting `size_t` is not fitting a vector, and how far apart those two are is
    // an implementation's choice:
    //
    //   libstdc++   max_size() == PTRDIFF_MAX / sizeof(T)
    //   libc++      max_size() == SIZE_MAX   / sizeof(T)
    //
    // On libstdc++ that leaves a window of counts — about 1.28e17 of them for a
    // `sampled_finding` — that narrow cleanly into `size_t`, pass the overflow
    // check, and make `reserve` answer with `length_error`. On libc++ the two
    // coincide and the overflow check gets there first, so the window is empty.
    //
    // The contract is the same either way and that is what this asserts: a count
    // a vector will not hold comes back as `insufficient_space`, never as an
    // exception crossing the library. Which guard refuses it is not the property.
    //
    // Nothing large is reserved: every path here refuses before allocating.
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(key_of(1), payload_for(0), 800000).has_value());
        db.close();
    }

    // Asked of exactly the vector types the walk uses — the allocator is part of
    // the type and can change the answer, so probing `std::vector<T>` would be
    // probing something else.
    detail::budget_meter probe_meter(1024);
    detail::counted_vector<detail::sampled_finding> const findings_probe{
        detail::counted_allocator<detail::sampled_finding>(probe_meter)};
    detail::counted_vector<duplicate_location> const locations_probe{
        detail::counted_allocator<duplicate_location>(probe_meter)};
    auto const findings_max = uint64_t(findings_probe.max_size());
    auto const locations_max = uint64_t(locations_probe.max_size());
    REQUIRE(findings_max > 0);
    REQUIRE(locations_max > 0);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));

    verify_options too_many_findings;
    too_many_findings.max_findings = findings_max + 1;
    too_many_findings.max_locations_per_finding = 1;
    auto const refused = db.verify_unique_outpoints(too_many_findings);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() == error_code::insufficient_space);

    // And on the locations count, which is a separate vector with its own limit.
    verify_options too_many_locations;
    too_many_locations.max_findings = 2;
    too_many_locations.max_locations_per_finding = locations_max / 2 + 1;
    auto const also_refused = db.verify_unique_outpoints(too_many_locations);
    REQUIRE_FALSE(also_refused.has_value());
    CHECK(also_refused.error() == error_code::insufficient_space);

    // The defaults are nowhere near either limit.
    CHECK(db.verify_unique_outpoints().has_value());
    db.close();
}

TEST_CASE("a meter whose books do not balance authorises nothing", "[unique]") {
    // Releasing more than was acquired means what is actually held is no longer
    // known. Clamping to zero — which this used to do — hides that and then hands
    // out memory against a figure that has stopped describing anything, which is
    // the overrun the meter exists to prevent.
    using utxoz::detail::budget_meter;

    budget_meter meter(1000);
    REQUIRE(meter.acquire(100));
    CHECK(meter.balanced());

    meter.release(400);          // more than the 100 outstanding
    CHECK_FALSE(meter.balanced());

    // And from here it refuses, including requests that would obviously fit.
    CHECK_FALSE(meter.acquire(1));
    CHECK_FALSE(meter.acquire(0));
    CHECK(meter.refusals() == 2);
    // It does not pretend to know what is held, either.
    CHECK(meter.outstanding() == 0);

    // A balanced sequence is unaffected, which is what makes the check about the
    // mismatch and not about releasing at all.
    budget_meter fine(1000);
    REQUIRE(fine.acquire(600));
    fine.release(600);
    CHECK(fine.balanced());
    CHECK(fine.acquire(1000));
}

TEST_CASE("an unbalanced meter yields no verdict, even with nothing left to refuse",
          "[unique]") {
    // The gap the earlier fix left: `release()` marks the meter unbalanced and
    // later acquisitions fail, but a mismatch in the *last* deallocation — the
    // sample's own destructor — has no acquisition after it to be refused. So the
    // books are checked once the metered containers are gone, and that is what
    // this asserts: a meter that lost track produces no report at all.
    using utxoz::detail::budget_meter;
    using utxoz::detail::counted_allocator;
    using utxoz::detail::counted_vector;

    budget_meter meter(1000);
    {
        counted_vector<uint8_t> v{counted_allocator<uint8_t>(meter)};
        v.reserve(100);
        // Somebody else gives back bytes this meter never handed out. The vector's
        // own destructor, below, then releases correctly — and the mismatch has
        // already happened, with nothing acquiring afterwards.
        meter.release(500);
    }
    CHECK_FALSE(meter.balanced());
    CHECK(meter.acquire(1) == false);
}

TEST_CASE("a container past the ceiling never reaches the allocator", "[unique]") {
    // What makes the budget structural rather than descriptive: the vector the
    // walk holds its records in cannot exceed the ceiling, because asking for
    // the memory is what checks.
    using utxoz::detail::budget_meter;
    using utxoz::detail::budget_exceeded;
    using utxoz::detail::counted_allocator;
    using utxoz::detail::counted_vector;

    budget_meter meter(10 * sizeof(detail::stored_copy));
    counted_vector<detail::stored_copy> records{counted_allocator<detail::stored_copy>(meter)};

    records.reserve(10);
    CHECK(meter.outstanding() >= 10 * sizeof(detail::stored_copy));
    auto const held = meter.outstanding();

    // Eleven does not fit. It throws, nothing is taken, and — the part that
    // matters on a real database — no allocation of that size was attempted.
    counted_vector<detail::stored_copy> greedy{counted_allocator<detail::stored_copy>(meter)};
    CHECK_THROWS_AS(greedy.reserve(11), budget_exceeded);
    CHECK(meter.outstanding() == held);
    CHECK(meter.refusals() == 1);

    // Destroying the first gives its bytes back, and then the same request fits.
    records = counted_vector<detail::stored_copy>{counted_allocator<detail::stored_copy>(meter)};
    CHECK(meter.outstanding() == 0);
    CHECK_NOTHROW(greedy.reserve(10));
}

TEST_CASE("generations visited is not segment opens", "[unique]") {
    // The metric counts generations walked, and the active generation of a class
    // is already mapped — it is walked without opening anything. Naming it after
    // file opens would overstate the I/O by exactly the active generations, which
    // in a database with few historical ones is most of the figure.
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 30; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(900), payload_for(0), 800000).has_value());
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto const before = failpoints::segments_mapped.load(std::memory_order_relaxed);
    auto const r = db.verify_unique_outpoints();
    REQUIRE(r.has_value());
    auto const opened = failpoints::segments_mapped.load(std::memory_order_relaxed) - before;

    // Per pass: the active generation of every class, plus the one historical
    // generation the rotation left behind. Only that historical one is opened; the
    // five active ones are already mapped.
    auto const passes = r->planning_passes + r->data_passes;
    REQUIRE(passes == 2);
    CHECK(r->generations_visited == passes * (container_count + 1));
    CHECK(opened == passes);
    // Which is the whole point of the name: the visits are six times the opens
    // here, and calling them opens would overstate the I/O by that much.
    CHECK(r->generations_visited > opened);
    db.close();
}

TEST_CASE("a budget that cannot hold the work is refused before any of it", "[unique]") {
    // Not just "refused": refused after the planning pass and before the first
    // collecting pass. Segment mappings are the observable — a walk that started
    // collecting and then gave up would map more files than one that refused.
    //
    // A prefix that cannot be divided is how to build a database that no number
    // of partitions can fit: every copy of one key hashes alike. A budget merely
    // too small for the whole set is not enough, because the walk answers that
    // with more passes rather than a refusal.
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const repeated = key_of(0x600D);
    constexpr int copies = 40;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (int i = 0; i < copies; ++i) {
            REQUIRE(db.insert(repeated, payload_for(0), 800000 + uint32_t(i)).has_value());
            failpoints::force_rotations.store(1, std::memory_order_relaxed);
            REQUIRE(db.insert(key_of(3000 + uint64_t(i)), payload_for(0), 800000).has_value());
        }
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));

    // One planning pass and one collecting pass, to learn what a pass costs in
    // mappings. Derived rather than hard-coded: the number depends on how many
    // historical generations there are, which is a fixture detail.
    auto const before_ok = failpoints::segments_mapped.load(std::memory_order_relaxed);
    auto const roomy = db.verify_unique_outpoints();
    REQUIRE(roomy.has_value());
    REQUIRE(roomy->data_passes == 1);
    REQUIRE(roomy->max_copies_for_one_key == copies);
    auto const two_passes = failpoints::segments_mapped.load(std::memory_order_relaxed)
        - before_ok;
    REQUIRE(two_passes > 0);
    REQUIRE(two_passes % 2 == 0);
    auto const per_pass = two_passes / 2;

    // Room for a quarter of that one prefix. The planning pass runs, the group
    // is found not to fit, and it stops there.
    uint64_t const too_small = budget_holding(*roomy, copies / 4);
    auto const before_refusal = failpoints::segments_mapped.load(std::memory_order_relaxed);
    auto const refused = db.verify_unique_outpoints(budget_of(too_small));
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() == error_code::insufficient_space);
    auto const mapped_by_refusal =
        failpoints::segments_mapped.load(std::memory_order_relaxed) - before_refusal;
    // Exactly one pass's worth: the planning pass, and nothing after it.
    CHECK(mapped_by_refusal == per_pass);
    db.close();
}

TEST_CASE("the sample is inside the budget, not beside it", "[unique]") {
    // The findings cannot be metered by the allocator — they are public types
    // with the default one — so their worst case is taken from the ceiling
    // before the walk reads anything. A sample that does not fit is therefore a
    // budget refusal, and raising the budget is what makes it fit.
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 20; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));

    // A sample far larger than a small budget can hold.
    verify_options greedy = budget_of(4u * 1024 * 1024);
    greedy.max_findings = 100'000;
    greedy.max_locations_per_finding = 64;
    auto const refused = db.verify_unique_outpoints(greedy);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() == error_code::insufficient_space);

    // The same sample against a budget that can hold it: accepted, and the
    // fixed overhead grew by what the sample takes, which is what "inside the
    // budget" means.
    greedy.memory_budget = 512u * 1024 * 1024;
    auto const allowed = db.verify_unique_outpoints(greedy);
    REQUIRE(allowed.has_value());
    auto const plain = db.verify_unique_outpoints(budget_of(512u * 1024 * 1024));
    REQUIRE(plain.has_value());
    CHECK(allowed->fixed_overhead > plain->fixed_overhead);

    // And an overflowing product is a refusal, not a small number.
    verify_options wrapping;
    wrapping.max_findings = std::numeric_limits<uint64_t>::max();
    wrapping.max_locations_per_finding = std::numeric_limits<uint64_t>::max();
    CHECK_FALSE(db.verify_unique_outpoints(wrapping).has_value());
    db.close();
}

TEST_CASE("the budget predicate refuses at the boundary and on overflow", "[unique]") {
    // Checked directly because the call site cannot be reached by data: the
    // groups are sized so the records fit, and `reserve` allocates exactly what
    // it is asked for on every implementation this builds against. The guard is
    // still worth having — `reserve` promises *at least* — and this is what makes
    // it possible to say it works.
    constexpr uint64_t max = std::numeric_limits<uint64_t>::max();
    uint64_t peak = 0;

    CHECK(detail::fits_in_budget(10, 56, 1000, 100, 100000, peak));
    CHECK(peak == 10 * 56 + 1000 + 100);

    // Exactly at the budget fits; one byte past it does not.
    CHECK(detail::fits_in_budget(10, 10, 0, 0, 100, peak));
    CHECK(peak == 100);
    CHECK_FALSE(detail::fits_in_budget(10, 10, 1, 0, 100, peak));

    // And a multiplication that would wrap is a refusal, not a small number —
    // including the figure it leaves behind. The caller prints the peak as "this
    // needs a budget of at least N", and a partial sum there would name something
    // an operator could raise the budget to and be refused again.
    CHECK_FALSE(detail::fits_in_budget(max, 56, 0, 0, max, peak));
    CHECK(peak == max);
    peak = 7;
    CHECK_FALSE(detail::fits_in_budget(2, max / 2 + 1, 0, 0, max, peak));
    CHECK(peak == max);
    peak = 7;
    CHECK_FALSE(detail::fits_in_budget(1, max, 1, 0, max, peak));
    CHECK(peak == max);
}

// =============================================================================
// Determinism, and the failures that must not become a smaller report
// =============================================================================

TEST_CASE("the answer does not depend on the cache or on being run twice",
          "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const shared = key_of(0x1234);
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 50; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        // Before the rotation and again after it, so there really are two copies.
        // Inserted only afterwards — which is what this case used to do — the key
        // is stored once, there are no findings, and the loop below compares two
        // empty vectors and proves nothing.
        REQUIRE(db.insert(shared, payload_for(0), 800000).has_value());
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(500), payload_for(0), 800000).has_value());
        REQUIRE(db.insert(shared, payload_for(0), 800100).has_value());
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto const cold = db.verify_unique_outpoints();
    REQUIRE(cold.has_value());

    // Warm the file cache by resolving through the historical generations, which
    // is what reorders the search in #136. It must not reorder this.
    std::vector<lookup_request> const request{{key_of(1), 800200}};
    (void) db.resolve(request);

    auto const warm = db.verify_unique_outpoints();
    REQUIRE(warm.has_value());
    db.close();

    CHECK(cold->unique == warm->unique);
    CHECK(cold->physical_entries == warm->physical_entries);
    CHECK(cold->distinct_outpoints == warm->distinct_outpoints);
    CHECK(cold->duplicate_copies == warm->duplicate_copies);
    CHECK(cold->keys_with_multiple_copies == warm->keys_with_multiple_copies);
    CHECK(cold->max_copies_for_one_key == warm->max_copies_for_one_key);
    // There is something to compare, asserted before comparing it.
    CHECK_FALSE(cold->unique);
    REQUIRE(cold->findings.size() == 1);
    REQUIRE(cold->findings.size() == warm->findings.size());
    for (size_t i = 0; i < cold->findings.size(); ++i) {
        CHECK(cold->findings[i].multiplicity == warm->findings[i].multiplicity);
        REQUIRE(cold->findings[i].locations.size() == warm->findings[i].locations.size());
        for (size_t j = 0; j < cold->findings[i].locations.size(); ++j) {
            CHECK(cold->findings[i].locations[j].container_class
                  == warm->findings[i].locations[j].container_class);
            CHECK(cold->findings[i].locations[j].generation
                  == warm->findings[i].locations[j].generation);
        }
    }
}

TEST_CASE("a generation that will not open produces no verdict", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 10; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(99), payload_for(0), 800000).has_value());
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    REQUIRE(db.verify_unique_outpoints().has_value());   // the control

    auto const historical = t.dir / fmt::format(detail::data_file_format, 0, 0);
    REQUIRE(fs::exists(historical));
    {
        std::ofstream out(historical, std::ios::binary | std::ios::trunc);
        REQUIRE(out.good());
    }

    // Not `unique == false`, and not `unique == true` either. A database part of
    // which could not be read has no verdict, and saying either would be a claim
    // about entries nobody looked at.
    auto const broken = db.verify_unique_outpoints();
    CHECK_FALSE(broken.has_value());
    db.close();
}

TEST_CASE("a failure during the collecting passes produces no report at all",
          "[unique]") {
    // The property is "never a partial report". The seam exists because a walk
    // that fails on its first pass never reaches the collecting pass, which is
    // where a walk that carried on would produce the partial answer this case
    // forbids.
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 20; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(99), payload_for(0), 800000).has_value());
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));

    // The control: the same call works, and it opens some number of segments.
    auto const before = failpoints::segments_mapped.load(std::memory_order_relaxed);
    REQUIRE(db.verify_unique_outpoints().has_value());
    REQUIRE(failpoints::segments_mapped.load(std::memory_order_relaxed) > before);

    // Armed one mapping ahead: the planning pass reopens the historical
    // generation and is allowed through, and the refusal lands on the collecting
    // pass that follows it.
    failpoints::fail_segment_open_after.store(
        failpoints::segments_mapped.load(std::memory_order_relaxed) + 1,
        std::memory_order_relaxed);

    auto const broken = db.verify_unique_outpoints();
    REQUIRE_FALSE(broken.has_value());
    // And it is *this* error: the file could not be opened. A walk that stopped
    // quietly and returned what it had would otherwise be indistinguishable from
    // one that refused.
    CHECK(broken.error() == error_code::file_open_failed);

    failpoints::fail_segment_open_after.store(0, std::memory_order_relaxed);
    CHECK(db.verify_unique_outpoints().has_value());
    db.close();
}

// =============================================================================
// Reference mode
// =============================================================================

TEST_CASE("reference mode is verified too, under one class", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const shared = key_of(0xFEED);
    {
        auto db = std::move(*reference_db::open_for_testing(t.dir, true));
        for (uint32_t i = 1; i <= 10; ++i) {
            REQUIRE(db.insert(key_of(i), i, i * 2, 800000).has_value());
        }
        REQUIRE(db.insert(shared, 77, 88, 800000).has_value());
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        REQUIRE(db.insert(key_of(500), 1, 1, 800100).has_value());
        // The same outpoint again, now in the active generation.
        REQUIRE(db.insert(shared, 99, 111, 800100).has_value());
        db.close();
    }

    auto db = std::move(*reference_db::open_for_testing(t.dir, false));
    auto const r = db.verify_unique_outpoints();
    REQUIRE(r.has_value());

    CHECK_FALSE(r->unique);
    CHECK(r->storage_mode == "reference");
    CHECK(r->physical_entries == 13);
    CHECK(r->distinct_outpoints == 12);
    CHECK(r->duplicate_copies == 1);
    CHECK(r->keys_with_multiple_copies == 1);
    CHECK(r->max_copies_for_one_key == 2);

    // One class, so both locations report it; the copies span the active
    // generation and a historical one.
    REQUIRE(r->findings.size() == 1);
    auto const& f = r->findings.front();
    REQUIRE(f.locations.size() == 2);
    CHECK(f.locations[0].container_class == 0);
    CHECK(f.locations[1].container_class == 0);
    CHECK(std::ranges::count_if(f.locations, [](auto const& l) { return l.active; }) == 1);
    db.close();
}

// =============================================================================
// The census is not involved
// =============================================================================

TEST_CASE("the census neither answers this question nor changed to ask it", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(key_of(1), payload_for(0), 800000).has_value());
        REQUIRE(db.insert(key_of(1), payload_for(3), 800000).has_value());
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto const physical = db.census();
    REQUIRE(physical.has_value());
    // Still the one scope it has always had, and still counting copies.
    CHECK(physical->scope == "physical_stored");
    CHECK(physical->entries == 2);
    CHECK(census_report::schema_version == 1);
    // Two copies to the census, one outpoint to the verification. That is the
    // whole difference between the two instruments.
    auto const verified = db.verify_unique_outpoints();
    REQUIRE(verified.has_value());
    CHECK(verified->physical_entries == physical->entries);
    CHECK(verified->distinct_outpoints == 1);
    db.close();
}

TEST_CASE("a latched database is not judged, it is refused", "[unique]") {
    // An interrupted compaction can leave a merged generation beside the sources
    // it was built from. Counting that is honest — two copies are two copies —
    // but a *verdict* on it would report the database as violating uniqueness
    // when what happened is that an operation did not finish. That is a false
    // accusation about integrity, and it is the kind that gets acted on.
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto db = std::move(*full_db::open_for_testing(t.dir, true));
    for (uint64_t i = 1; i <= 40; ++i) {
        REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
    }
    failpoints::force_rotations.store(1, std::memory_order_relaxed);
    REQUIRE(db.insert(key_of(500), payload_for(0), 800000).has_value());

    // Verified before, so the refusal below is about the latch and nothing else.
    REQUIRE(db.verify_unique_outpoints().has_value());

    failpoints::fail_source_unlink.store(true, std::memory_order_relaxed);
    auto const compacted = db.compact_all();
    failpoints::fail_source_unlink.store(false, std::memory_order_relaxed);
    REQUIRE_FALSE(compacted.has_value());
    REQUIRE(compacted.error() == error_code::recovery_required);

    auto const refused = db.verify_unique_outpoints();
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() == error_code::recovery_required);

    // And the census still answers, because a count is a description of what is
    // stored and does not become wrong when an operation is half-applied.
    CHECK(db.census().has_value());
    db.close();
}

// =============================================================================
// The tool, and the three states of its exit status
// =============================================================================

#ifdef UTXOZ_VERIFY_TOOL

namespace {

struct tool_run {
    int status = -1;
    std::string out;
};

tool_run run_verify_tool(fs::path const& dir, std::string const& args) {
    auto const log = dir.parent_path() / (dir.filename().string() + ".verify-out.txt");
    auto command = fmt::format("\"{}\" \"{}\" {} > \"{}\" 2>&1",
                               UTXOZ_VERIFY_TOOL, dir.string(), args, log.string());
#ifdef _WIN32
    // cmd.exe strips the outermost pair of quotes; one more keeps the rest.
    command = "\"" + command + "\"";
#endif
    tool_run r;
    int const raw = std::system(command.c_str());
#ifdef _WIN32
    r.status = raw;
#else
    // The verdict is in the exit code, so the wait status has to be decoded
    // rather than compared: 2 shifted left by eight is not 2.
    r.status = WIFEXITED(raw) ? WEXITSTATUS(raw) : -1;
#endif
    std::ifstream in(log, std::ios::binary);
    r.out.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    std::error_code ec;
    fs::remove(log, ec);
    return r;
}

} // namespace

TEST_CASE("the tool exits 0 on a unique database", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 20; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        db.close();
    }

    auto const run = run_verify_tool(t.dir, "--json");
    INFO(run.out);
    CHECK(run.status == 0);
    std::error_code ec;
    auto const parsed = boost::json::parse(run.out, ec);
    REQUIRE_FALSE(ec);
    CHECK(parsed.at("unique").as_bool());
    CHECK(parsed.at("counts").at("distinct_outpoints").as_int64() == 20);
}

TEST_CASE("the tool exits 2 when an outpoint is stored more than once", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(key_of(1), payload_for(0), 800000).has_value());
        REQUIRE(db.insert(key_of(1), payload_for(3), 800000).has_value());
        db.close();
    }

    auto const run = run_verify_tool(t.dir, "--json");
    INFO(run.out);
    // Two, not one. "The answer is no" and "there is no answer" send an operator
    // to entirely different places.
    CHECK(run.status == 2);
    std::error_code ec;
    auto const parsed = boost::json::parse(run.out, ec);
    REQUIRE_FALSE(ec);
    CHECK_FALSE(parsed.at("unique").as_bool());
    CHECK(parsed.at("counts").at("duplicate_copies").as_int64() == 1);
    // Withheld by default, in the document that actually travels.
    CHECK_FALSE(parsed.at("findings").at("outpoints_included").as_bool());
    CHECK(run.out.find("\"outpoint\"") == std::string::npos);

    // And with the opt-in, present.
    auto const with = run_verify_tool(t.dir, "--json --include-outpoints");
    CHECK(with.status == 2);
    auto const opted = boost::json::parse(with.out, ec);
    REQUIRE_FALSE(ec);
    CHECK(opted.at("findings").at("outpoints_included").as_bool());
    CHECK(with.out.find("\"outpoint\"") != std::string::npos);
}

TEST_CASE("the tool exits 1 when it cannot tell", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;

    // Nothing there: it refuses rather than creating a database and calling the
    // empty result unique.
    auto const missing = run_verify_tool(t.dir, "--json");
    INFO(missing.out);
    CHECK(missing.status == 1);
    CHECK_FALSE(fs::exists(t.dir));

    // A directory that exists and holds no database. A separate case from the
    // one above and not a duplicate of it: this one gets past the existence
    // check and fails inside the open, which is the branch that would otherwise
    // be free to report "not unique" about a database it never read.
    REQUIRE(fs::create_directories(t.dir));
    auto const empty_dir = run_verify_tool(t.dir, "--json");
    INFO(empty_dir.out);
    CHECK(empty_dir.status == 1);
    fs::remove_all(t.dir);

    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(key_of(1), payload_for(0), 800000).has_value());
        db.close();
    }

    // A budget that cannot hold the work: the verification did not happen, and
    // that is not the same as a database that passed it.
    auto const cramped = run_verify_tool(t.dir, "--json --memory-budget=1024");
    INFO(cramped.out);
    CHECK(cramped.status == 1);

    // A malformed option, likewise.
    CHECK(run_verify_tool(t.dir, "--max-findings=lots").status == 1);

    // And the same database, asked properly, answers 0.
    CHECK(run_verify_tool(t.dir, "--json").status == 0);
}

TEST_CASE("the help text says what the defaults actually are", "[unique]") {
    // The numbers in the help come from `verify_options` rather than being
    // written out beside it, and this is what says so. A help text that restates
    // a default is right until somebody changes it, and then it is a document
    // that confidently gives the wrong number.
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const help = run_verify_tool(t.dir, "--help");
    INFO(help.out);
    CHECK(help.status == 0);

    // Anchored to the phrase each one follows, not to "default N" alone: two of
    // these defaults are the same number, so a loose match would be satisfied by
    // the other option's line and would pass while the value was wrong.
    verify_options const defaults;
    CHECK(help.out.find(fmt::format("walk (default {})", defaults.memory_budget))
          != std::string::npos);
    CHECK(help.out.find(fmt::format("this (default {})", defaults.max_data_passes))
          != std::string::npos);
    CHECK(help.out.find(fmt::format("describe (default {})", defaults.max_findings))
          != std::string::npos);
    CHECK(help.out.find(fmt::format("locate (default {})",
                                    defaults.max_locations_per_finding))
          != std::string::npos);
}

TEST_CASE("an argument that looks like an option is not a directory", "[unique]") {
    // A single dash where two were meant. Taken as a path, the tool would go
    // looking for a database called "-json" and report that it does not exist —
    // a true statement about the wrong question.
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        REQUIRE(db.insert(key_of(1), payload_for(0), 800000).has_value());
        db.close();
    }

    auto const typo = run_verify_tool(t.dir, "-json");
    INFO(typo.out);
    CHECK(typo.status == 1);
    CHECK(typo.out.find("unknown option") != std::string::npos);

    auto const bare = run_verify_tool(t.dir, "-");
    INFO(bare.out);
    CHECK(bare.status == 1);
    CHECK(bare.out.find("unknown option") != std::string::npos);
}

#ifdef __linux__
TEST_CASE("a report that could not be written is not a verdict", "[unique]") {
    // The exit status is only half the answer. If the report is truncated by a
    // full pipe or a full disk, a caller reading the status alone would act on a
    // document it never received.
    //
    // Linux only: /dev/full is what makes a write fail on demand, and there is no
    // portable equivalent. The behaviour it pins is not platform-specific; the
    // way to provoke it is.
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 10; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        db.close();
    }

    // The control: the same database, the same options, writing somewhere that
    // accepts bytes.
    REQUIRE(run_verify_tool(t.dir, "--json").status == 0);

    auto const command = fmt::format("\"{}\" \"{}\" --json > /dev/full 2>/dev/null",
                                     UTXOZ_VERIFY_TOOL, t.dir.string());
    int const raw = std::system(command.c_str());
    REQUIRE(WIFEXITED(raw));
    // One, not zero: the database is unique, and the tool could not say so.
    CHECK(WEXITSTATUS(raw) == 1);
}
#endif

TEST_CASE("the tool prints for a person too", "[unique]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        for (uint64_t i = 1; i <= 10; ++i) {
            REQUIRE(db.insert(key_of(i), payload_for(0), 800000).has_value());
        }
        db.close();
    }

    auto const run = run_verify_tool(t.dir, "--text");
    INFO(run.out);
    CHECK(run.status == 0);
    // Not JSON, and it says the thing it is for.
    CHECK(run.out.find("uniqueness:") != std::string::npos);
    CHECK(run.out.find("no outpoint is stored more than once") != std::string::npos);
    CHECK(run.out.find("\"report_schema_version\"") == std::string::npos);
}

TEST_CASE("the tool opens the storage mode it was told to", "[unique]") {
    // --mode is wiring, and wiring that is never exercised is wiring that is
    // wrong. A reference database opened as full is a database the tool cannot
    // read, which is the observable that makes the flag mean something.
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*reference_db::open_for_testing(t.dir, true));
        for (uint32_t i = 1; i <= 10; ++i) {
            REQUIRE(db.insert(key_of(i), i, i * 2, 800000).has_value());
        }
        db.close();
    }

    auto const as_reference = run_verify_tool(t.dir, "--json --mode=reference");
    INFO(as_reference.out);
    CHECK(as_reference.status == 0);
    std::error_code ec;
    auto const parsed = boost::json::parse(as_reference.out, ec);
    REQUIRE_FALSE(ec);
    CHECK(parsed.at("storage_mode").as_string() == "reference");
    CHECK(parsed.at("counts").at("distinct_outpoints").as_int64() == 10);

    // The default, on the same directory: it cannot tell, and says so with 1.
    auto const as_full = run_verify_tool(t.dir, "--json");
    INFO(as_full.out);
    CHECK(as_full.status == 1);
}

#endif // UTXOZ_VERIFY_TOOL
