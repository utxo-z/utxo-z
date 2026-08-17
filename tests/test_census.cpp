// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_census.cpp
 * @brief That the census counts what is there, and refuses rather than
 *        under-reports.
 *
 * Two kinds of case. The first are arithmetic: the parts have to add up to the
 * whole, in every direction the report offers — buckets to entries, generations
 * to classes, classes to totals. They are dull and they are the ones that catch
 * a walk that skipped something.
 *
 * The second are refusals. A census that cannot read a generation must fail, not
 * return the rest: a report short by one file looks exactly like a database with
 * one file fewer, and the difference between those two is the whole reason
 * somebody is running it.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <boost/json.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/census.hpp>
#include <utxoz/database.hpp>
#include <utxoz/types.hpp>

#include "detail/durability.hpp"
#include "detail/census_arithmetic.hpp"
#include "detail/database_lock.hpp"
#include "detail/utxo_value.hpp"
#include "detail/file_cache.hpp"

namespace fs = std::filesystem;
using namespace utxoz;
using utxoz::detail::failpoints;

namespace {

struct temp_db {
    temp_db() {
        static std::atomic<uint64_t> counter{0};
        auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        dir = fmt::format("./test_census_{}_{}_{}", getpid(), ts, counter.fetch_add(1));
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

/// A payload that lands in class `index`: one byte more than the class below can
/// hold, so it cannot be absorbed by a smaller one.
size_t payload_for(size_t index) {
    return index == 0 ? 8 : container_capacities[index - 1] + 1;
}

/// A database with entries in every class, three payload sizes per class, and a
/// rotation in class 0 so that there is a historical generation to walk.
uint64_t populate(fs::path const& dir, size_t per_class = 12) {
    auto db = std::move(*full_db::open_for_testing(dir, true));
    uint64_t n = 0;
    for (size_t klass = 0; klass < container_sizes.size(); ++klass) {
        for (size_t i = 0; i < per_class; ++i) {
            std::vector<uint8_t> const value(payload_for(klass) + (i % 3), 0x5A);
            REQUIRE(db.insert(key_of(++n), value, 800000).has_value());
        }
    }
    failpoints::force_rotations.store(1, std::memory_order_relaxed);
    for (size_t i = 0; i < 4; ++i) {
        std::vector<uint8_t> const value(8, 0x5A);
        REQUIRE(db.insert(key_of(++n), value, 800100).has_value());
    }
    db.close();
    return n;
}

} // namespace

// =============================================================================
// The parts add up to the whole
// =============================================================================

TEST_CASE("the census counts every class and every generation", "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const inserted = populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto report = db.census();
    REQUIRE(report.has_value());

    // Exactly five classes in full mode, named by index and not merely counted:
    // a walk that visited one class twice would also report five.
    REQUIRE(report->classes.size() == container_sizes.size());
    for (size_t i = 0; i < report->classes.size(); ++i) {
        CHECK(report->classes[i].container_class == i);
        CHECK(report->classes[i].container_size == container_sizes[i]);
        CHECK(report->classes[i].payload_capacity == container_capacities[i]);
    }

    CHECK(report->entries == inserted);
    CHECK(report->entries_examined == inserted);
    CHECK(report->scope == "physical_stored");
    CHECK(report->storage_mode == "full");

    // Class 0 rotated, so it has two generations and one of them is active.
    auto const& first = report->classes[0];
    CHECK(first.generations == 2);
    CHECK(first.generations_detail.size() == 2);
    CHECK(std::ranges::count_if(first.generations_detail,
                                [](auto const& g) { return g.active; }) == 1);

    // Every generation the report describes is one it examined, and the
    // generations of every class come to the files it walked.
    uint64_t generations = 0;
    for (auto const& c : report->classes) generations += c.generations;
    CHECK(generations == report->files_examined);

    db.close();
}

TEST_CASE("the histogram accounts for every entry of its class", "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto report = db.census();
    REQUIRE(report.has_value());

    uint64_t histogram_entries = 0;
    uint64_t histogram_payload = 0;
    for (auto const& c : report->classes) {
        REQUIRE(c.payload_histogram_status == metric_status::measured);
        uint64_t class_entries = 0;
        uint64_t class_payload = 0;
        uint32_t previous = 0;
        bool first = true;
        for (auto const& b : c.payload_histogram) {
            INFO("class " << c.container_class << " bucket " << b.payload_size);
            // Ascending and without repeats, because a consumer will treat this
            // as a distribution and two buckets for one size would be two sizes.
            CHECK((first || b.payload_size > previous));
            first = false;
            previous = b.payload_size;
            // No bucket beyond what the class can hold.
            CHECK(b.payload_size <= c.payload_capacity);
            CHECK(b.entries > 0);
            class_entries += b.entries;
            class_payload += uint64_t(b.payload_size) * b.entries;
        }
        INFO("class " << c.container_class);
        CHECK(class_entries == c.entries);
        // And the histogram reproduces the payload total, which is the same sum
        // taken a different way.
        CHECK(class_payload == c.entry_payload_bytes);
        histogram_entries += class_entries;
        histogram_payload += class_payload;
    }
    CHECK(histogram_entries == report->entries_examined);
    CHECK(histogram_payload == report->entry_payload_bytes);
    db.close();
}

TEST_CASE("the classes sum to the totals, field by field", "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto report = db.census();
    REQUIRE(report.has_value());

    uint64_t entries = 0, payload = 0, unused = 0, padding = 0;
    uint64_t segment = 0, freed = 0, files = 0;
    uint64_t occupied = 0, empty = 0, metadata = 0, residual = 0, physical = 0;
    for (auto const& c : report->classes) {
        entries += c.entries;
        payload += c.entry_payload_bytes;
        REQUIRE(c.unused_payload_capacity.status == metric_status::measured);
        unused += c.unused_payload_capacity.bytes;
        padding += c.object_padding_bytes;
        segment += c.segment_size_bytes;
        freed += c.segment_free_bytes;
        files += c.logical_file_bytes;
        occupied += c.occupied_slot_bytes;
        empty += c.empty_slot_bytes;
        metadata += c.estimated_group_metadata_bytes;
        REQUIRE(c.unattributed_allocated_bytes.status == metric_status::measured);
        residual += c.unattributed_allocated_bytes.bytes;
        REQUIRE(c.physical_allocated_bytes.status == metric_status::measured);
        physical += c.physical_allocated_bytes.bytes;
    }

    CHECK(entries == report->entries);
    CHECK(payload == report->entry_payload_bytes);
    CHECK(unused == report->unused_payload_capacity.bytes);
    CHECK(padding == report->object_padding_bytes);
    CHECK(segment == report->segment_size_bytes);
    CHECK(freed == report->segment_free_bytes);
    CHECK(files == report->logical_file_bytes);
    CHECK(occupied == report->occupied_slot_bytes);
    CHECK(empty == report->empty_slot_bytes);
    CHECK(metadata == report->estimated_group_metadata_bytes);
    CHECK(residual == report->unattributed_allocated_bytes.bytes);
    CHECK(physical == report->physical_allocated_bytes.bytes);
    db.close();
}

TEST_CASE("the byte decomposition of a segment closes exactly", "[census]") {
    // The identity the report is built on, checked per generation rather than in
    // aggregate: a residual defined as a subtraction always closes in total, and
    // checking it there would be checking arithmetic rather than the model.
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto report = db.census();
    REQUIRE(report.has_value());

    size_t checked = 0;
    for (auto const& c : report->classes) {
        for (auto const& g : c.generations_detail) {
            INFO("class " << c.container_class << " generation " << g.generation);
            REQUIRE_FALSE(g.model_inconsistent);
            REQUIRE(g.unattributed_allocated_bytes.status == metric_status::measured);
            CHECK(g.occupied_slot_bytes + g.empty_slot_bytes
                      + g.estimated_group_metadata_bytes
                      + g.unattributed_allocated_bytes.bytes
                      + g.segment_free_bytes
                  == g.segment_size_bytes);
            // The occupied and empty slots are the whole table and nothing else.
            CHECK(g.occupied_slot_bytes + g.empty_slot_bytes
                  == g.bucket_count * c.pair_size);
            CHECK(g.entries <= g.bucket_count);
            ++checked;
        }
    }
    CHECK(checked == report->files_examined);
    db.close();
}

TEST_CASE("payload and unused capacity are the two halves of the slot", "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto report = db.census();
    REQUIRE(report.has_value());

    for (auto const& c : report->classes) {
        INFO("class " << c.container_class);
        REQUIRE(c.unused_payload_capacity.status == metric_status::measured);
        // Every entry occupies the class's capacity whether it uses it or not.
        CHECK(c.entry_payload_bytes + c.unused_payload_capacity.bytes
              == c.entries * c.payload_capacity);
        // And the object is the capacity plus the two fields plus whatever
        // `sizeof` rounded up — zero in this geometry, measured rather than
        // assumed.
        CHECK(c.value_size >= c.payload_capacity);
        CHECK(c.pair_size == c.value_size + sizeof(raw_outpoint));
    }
    db.close();
}

// =============================================================================
// A delete removes an entry from the count, because it removes it from the file
// =============================================================================

TEST_CASE("a deleted entry is not stored, and the census does not count it", "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const inserted = populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto before = db.census();
    REQUIRE(before.has_value());
    REQUIRE(before->entries == inserted);

    std::vector<deferred_deletion_entry> batch;
    for (uint64_t i = 1; i <= 5; ++i) batch.emplace_back(key_of(i), 800200);
    auto const progress = db.apply_deletes(batch);
    REQUIRE(progress.erased.size() == 5);

    auto after = db.census();
    REQUIRE(after.has_value());
    CHECK(after->entries == inserted - 5);
    CHECK(after->entries_examined == inserted - 5);
    // The files are still there and still the same size: what changed is what is
    // in them, which is the distinction the whole report is built on.
    CHECK(after->files_examined == before->files_examined);
    CHECK(after->segment_size_bytes == before->segment_size_bytes);
    CHECK(after->entry_payload_bytes < before->entry_payload_bytes);
    db.close();
}

// =============================================================================
// Reference mode is one class, and it is not a size class
// =============================================================================

TEST_CASE("reference mode reports exactly one class, and counts its history", "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*reference_db::open_for_testing(t.dir, true));
        for (uint32_t i = 1; i <= 20; ++i) {
            REQUIRE(db.insert(key_of(i), i, i * 2, 800000).has_value());
        }
        failpoints::force_rotations.store(1, std::memory_order_relaxed);
        for (uint32_t i = 21; i <= 24; ++i) {
            REQUIRE(db.insert(key_of(i), i, i * 2, 800100).has_value());
        }
        db.close();
    }

    auto db = std::move(*reference_db::open_for_testing(t.dir, false));
    auto report = db.census();
    REQUIRE(report.has_value());

    REQUIRE(report->classes.size() == 1);
    CHECK(report->storage_mode == "reference");
    auto const& c = report->classes.front();
    CHECK(c.entries == 24);
    CHECK(report->entries == 24);

    // Both generations, not only the active one: an entry that rotated out is
    // still stored, and a census that reported only the active map would say a
    // database had shrunk when it had not.
    CHECK(c.generations == 2);
    CHECK(report->files_examined == 2);
    uint64_t from_generations = 0;
    for (auto const& g : c.generations_detail) from_generations += g.entries;
    CHECK(from_generations == c.entries);

    // A reference entry has no payload capacity to leave unused. That is not the
    // same statement as leaving none, and the report does not make it look like
    // one.
    CHECK(c.payload_histogram_status == metric_status::not_applicable);
    CHECK(c.payload_histogram.empty());
    CHECK(c.unused_payload_capacity.status == metric_status::not_applicable);
    CHECK_FALSE(c.unused_payload_capacity.detail.empty());
    CHECK(c.entry_payload_bytes == c.entries * c.value_size);
    db.close();
}

// =============================================================================
// Refusals
// =============================================================================

TEST_CASE("a generation that has gone missing fails the census", "[census]") {
    // Not a smaller report. The catalogue says the file is there; if it is not,
    // the entries it held are unknown, and a total that silently omits them is
    // indistinguishable from a database that never had them.
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    REQUIRE(db.census().has_value());   // the control: it works before the damage

    auto const historical = t.dir / fmt::format(detail::data_file_format, 0, 0);
    REQUIRE(fs::exists(historical));
    fs::remove(historical);

    auto broken = db.census();
    CHECK_FALSE(broken.has_value());
    db.close();
}

TEST_CASE("a generation that will not open as a segment fails the census", "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    REQUIRE(db.census().has_value());

    // Truncated to nothing the file is still there, and still catalogued, and no
    // longer a segment. The census has to say so rather than skip it.
    auto const historical = t.dir / fmt::format(detail::data_file_format, 0, 0);
    REQUIRE(fs::exists(historical));
    {
        std::ofstream out(historical, std::ios::binary | std::ios::trunc);
        REQUIRE(out.good());
    }

    auto broken = db.census();
    CHECK_FALSE(broken.has_value());
    db.close();
}

// =============================================================================
// The two presentations
// =============================================================================

TEST_CASE("the JSON parses, is versioned, and separates zero from unavailable",
          "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));

    census_options options;
    options.measure_physical_blocks = false;
    auto report = db.census(options);
    REQUIRE(report.has_value());
    auto const json = to_json(*report);

    // Parsed, not searched. A substring check passes on output that is not JSON
    // at all — an unescaped quote or a stray comma would leave every one of these
    // findable and the document unusable.
    std::error_code ec;
    auto const parsed = boost::json::parse(json, ec);
    INFO(json);
    REQUIRE_FALSE(ec);
    auto const& root = parsed.as_object();

    CHECK(root.at("report_schema_version").as_int64() == 1);
    CHECK(root.at("scope").as_string() == "physical_stored");
    CHECK(root.at("storage_mode").as_string() == "full");
    CHECK(root.at("format_identity").as_object().at("geometry_id").as_int64() == 3);

    // What was not measured is null and says why. A zero here would be a
    // measurement, and the difference matters most exactly when somebody is
    // comparing two runs on two machines.
    REQUIRE(report->physical_allocated_bytes.status == metric_status::not_applicable);
    auto const& physical = root.at("totals").as_object()
        .at("platform").as_object()
        .at("physical_allocated_bytes").as_object();
    CHECK(physical.at("status").as_string() == "not_applicable");
    CHECK(physical.at("bytes").is_null());
    CHECK_FALSE(physical.at("detail").as_string().empty());
    CHECK(root.at("walk").as_object().at("physical_measurement_method").as_string() == "none");

    // And a figure that *is* zero is a number and not null, so the two are told
    // apart by the reader and not only by the writer.
    CHECK(report->object_padding_bytes == 0);
    auto const& padding = root.at("totals").as_object()
        .at("exact").as_object().at("object_padding_bytes");
    CHECK(padding.is_int64());
    CHECK(padding.as_int64() == 0);

    db.close();
}

TEST_CASE("the same state produces the same JSON", "[census]") {
    // Everything except the duration, which is a measurement of the machine
    // rather than of the database and is excluded on purpose.
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto first = db.census();
    auto second = db.census();
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    first->duration_ms = 0;
    second->duration_ms = 0;
    CHECK(to_json(*first) == to_json(*second));
    db.close();
}

TEST_CASE("neither presentation contains a key or a payload", "[census]") {
    // The report goes into tickets and issues. Whatever else it carries, it does
    // not carry the chain.
    failpoints::scoped_reset const disarm;
    temp_db t;
    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        std::string const marker = "SENSITIVE-PAYLOAD";
        for (uint64_t i = 1; i <= 10; ++i) {
            std::vector<uint8_t> value(marker.begin(), marker.end());
            value.resize(payload_for(2), 0x77);
            raw_outpoint key{};
            std::string const key_marker = "SENSITIVE-KEY";
            std::memcpy(key.data(), key_marker.data(), key_marker.size());
            std::memcpy(key.data() + key_marker.size(), &i, sizeof(i));
            REQUIRE(db.insert(key, value, 800000).has_value());
        }
        db.close();
    }

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto report = db.census();
    REQUIRE(report.has_value());
    REQUIRE(report->entries == 10);

    auto const json = to_json(*report);
    auto const text = to_text(*report);
    CHECK(json.find("SENSITIVE") == std::string::npos);
    CHECK(text.find("SENSITIVE") == std::string::npos);
    db.close();
}

TEST_CASE("the text report carries the same figures as the structure", "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto report = db.census();
    REQUIRE(report.has_value());
    auto const text = to_text(*report);

    CHECK(text.find(fmt::format("walked {} files and {} entries",
                                report->files_examined, report->entries_examined))
          != std::string::npos);
    CHECK(text.find("physical_stored") != std::string::npos);
    // The caveat travels with the numbers rather than living only in a header
    // file nobody reading a pasted report will open.
    CHECK(text.find("not distinct outpoints") != std::string::npos);
    db.close();
}

// =============================================================================
// The build's own identity, and what it says about itself
// =============================================================================

TEST_CASE("the report states the identity of the build that produced it", "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto report = db.census();
    REQUIRE(report.has_value());

    CHECK(report->geometry_id != 0);
    CHECK(report->map_layout_epoch != 0);
    CHECK(report->hash_epoch != 0);
    CHECK(report->platform_abi_id != 0);

#if UTXOZ_STATISTICS_LEVEL >= 1
    CHECK(report->statistics_enabled);
#else
    CHECK_FALSE(report->statistics_enabled);
#endif

    // The census reads files, so it works identically either way: this is the
    // same suite in both configurations, and the count below is the assertion
    // that nothing about it was compiled out.
    CHECK(report->entries > 0);
    CHECK(report->files_examined > 0);
    db.close();
}

TEST_CASE("physical block measurement is reported with the method that produced it",
          "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));

    auto measured = db.census();
    REQUIRE(measured.has_value());
    // Asked for, so the method is named and is not "none". The platforms do not
    // measure the same thing, which is exactly why the name travels with it.
    CHECK(measured->physical_measurement != allocation_method::none);
    if (measured->physical_allocated_bytes.status == metric_status::measured) {
        CHECK_FALSE(measured->physical_allocated_bytes.detail.empty());
    }

    census_options options;
    options.measure_physical_blocks = false;
    auto skipped = db.census(options);
    REQUIRE(skipped.has_value());
    CHECK(skipped->physical_measurement == allocation_method::none);
    CHECK(skipped->physical_allocated_bytes.status != metric_status::measured);
    CHECK_FALSE(skipped->physical_allocated_bytes.detail.empty());
    // Everything else is unchanged by not asking: the option withholds one
    // figure, it does not change the walk.
    CHECK(skipped->entries == measured->entries);
    CHECK(skipped->files_examined == measured->files_examined);
    db.close();
}

TEST_CASE("asking for no per-generation detail keeps every total", "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));
    auto full = db.census();
    REQUIRE(full.has_value());

    census_options options;
    options.per_generation_detail = false;
    auto brief = db.census(options);
    REQUIRE(brief.has_value());

    CHECK(brief->entries == full->entries);
    CHECK(brief->files_examined == full->files_examined);
    CHECK(brief->segment_size_bytes == full->segment_size_bytes);
    for (auto const& c : brief->classes) {
        CHECK(c.generations_detail.empty());
        // The count of generations is a count, not the length of the list that
        // was suppressed.
        CHECK(c.generations > 0);
    }
    db.close();
}

// =============================================================================
// An entry that contradicts its own class
// =============================================================================
//
// The config and the stamp certify identity and layout. Neither certifies that
// an entry is internally consistent, and an entry whose recorded payload length
// exceeds what its class can hold is exactly that: a file that passed every
// check and still cannot be true.
//
// Taking the minimum would have turned it into a plausible number. The census
// would have reported the entry as full and said nothing, which is the worst of
// the available behaviours: a silent normalisation of corruption, inside the one
// tool whose whole purpose is to be believed.
//
// Two classes, because `actual_size` is a `uint8_t` below 256 and a `uint16_t`
// above it, and a check written against one representation is a check that has
// been tried once.

namespace {

/// Where `actual_size` sits inside the stored value of a class, and how wide it
/// is. Derived from the type rather than from arithmetic on the class size, so
/// that a change to the members moves this with it.
template <size_t Size>
struct size_field_layout {
    static constexpr size_t offset = offsetof(detail::utxo_value<Size>, actual_size);
    static constexpr size_t width = sizeof(detail::size_type<Size>);
};

/// Find the stored value of a known payload inside a generation file and
/// overwrite its `actual_size` with a value the class cannot hold.
///
/// The file is edited as bytes rather than through the map: opening the segment
/// to write would take the allocator's lock and rewrite the header, and what has
/// to be produced here is a file that is byte-for-byte what it was except for
/// one field.
template <size_t Size>
bool corrupt_one_actual_size(fs::path const& file, size_t payload_size,
                             uint64_t corrupt_value, uint64_t& restore_offset,
                             std::vector<uint8_t>& original) {
    std::fstream f(file, std::ios::binary | std::ios::in | std::ios::out);
    if ( ! f.good()) return false;
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),
                                std::istreambuf_iterator<char>());
    f.clear();

    // The stored value begins with the height and then the length. Look for the
    // exact pair this test inserted, which is unique in the file.
    uint32_t const height = 800000;
    constexpr size_t width = size_field_layout<Size>::width;
    for (size_t i = 0; i + sizeof(height) + width <= bytes.size(); ++i) {
        if (std::memcmp(&bytes[i], &height, sizeof(height)) != 0) continue;
        uint64_t recorded = 0;
        std::memcpy(&recorded, &bytes[i + sizeof(height)], width);
        if (recorded != payload_size) continue;

        restore_offset = i + sizeof(height);
        original.assign(bytes.begin() + int64_t(restore_offset),
                        bytes.begin() + int64_t(restore_offset + width));
        f.seekp(int64_t(restore_offset));
        f.write(reinterpret_cast<char const*>(&corrupt_value), width);
        return f.good();
    }
    return false;
}

void restore_bytes(fs::path const& file, uint64_t offset, std::vector<uint8_t> const& original) {
    std::fstream f(file, std::ios::binary | std::ios::in | std::ios::out);
    REQUIRE(f.good());
    f.seekp(int64_t(offset));
    f.write(reinterpret_cast<char const*>(original.data()), int64_t(original.size()));
    REQUIRE(f.good());
}

/// One generation of one class, holding entries of one payload size, closed.
template <size_t Index>
void one_generation_of(fs::path const& dir, size_t payload_size, size_t count) {
    auto db = std::move(*full_db::open_for_testing(dir, true));
    std::vector<uint8_t> const value(payload_size, 0x3C);
    for (uint64_t i = 1; i <= count; ++i) {
        REQUIRE(db.insert(key_of(i), value, 800000).has_value());
    }
    db.close();
}

} // namespace

TEST_CASE("an entry longer than its class fails the census, in both width classes",
          "[census]") {
    failpoints::scoped_reset const disarm;

    // Class 0 records its length in a uint8_t; class 4 in a uint16_t. A check
    // written against one of those is a check nobody has tried on the other.
    struct {
        size_t index;
        size_t payload;
        uint64_t corrupt;
        char const* what;
    } const cases[] = {
        {0, 8, 200, "class 0, uint8_t length"},
        {4, 300, 60000, "class 4, uint16_t length"},
    };

    for (auto const& c : cases) {
        INFO(c.what);
        temp_db t;
        if (c.index == 0) one_generation_of<0>(t.dir, c.payload, 6);
        else              one_generation_of<4>(t.dir, c.payload, 6);

        auto const file = t.dir / fmt::format(detail::data_file_format, c.index, 0);
        REQUIRE(fs::exists(file));

        // The control first: the census passes on the file as written. Without
        // this the failure below could be the corruption or could be anything
        // else about the fixture.
        {
            auto db = std::move(*full_db::open_for_testing(t.dir, false));
            auto ok = db.census();
            REQUIRE(ok.has_value());
            CHECK(ok->entries == 6);
            db.close();
        }

        uint64_t offset = 0;
        std::vector<uint8_t> original;
        bool patched = false;
        if (c.index == 0) {
            patched = corrupt_one_actual_size<container_sizes[0]>(
                file, c.payload, c.corrupt, offset, original);
        } else {
            patched = corrupt_one_actual_size<container_sizes[4]>(
                file, c.payload, c.corrupt, offset, original);
        }
        REQUIRE(patched);

        {
            auto db = std::move(*full_db::open_for_testing(t.dir, false));
            auto broken = db.census();
            // Refused, and refused for the right reason: an entry that cannot be
            // true, which is a different thing from a file that will not open.
            REQUIRE_FALSE(broken.has_value());
            CHECK(broken.error() == error_code::entry_corrupt);
            db.close();
        }

        // And restoring the one field makes the same census pass again, which is
        // what shows the refusal was about that field and not about the edit.
        restore_bytes(file, offset, original);
        {
            auto db = std::move(*full_db::open_for_testing(t.dir, false));
            auto ok = db.census();
            REQUIRE(ok.has_value());
            CHECK(ok->entries == 6);
            db.close();
        }
    }
}

// =============================================================================
// Where the directory came from is not what was counted
// =============================================================================

TEST_CASE("declaring a snapshot changes the provenance and not the scope", "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto db = std::move(*full_db::open_for_testing(t.dir, false));

    auto live = db.census();
    REQUIRE(live.has_value());
    CHECK(live->scope == "physical_stored");
    CHECK_FALSE(live->source.declared_external_snapshot);
    CHECK(live->source.consistency == snapshot_consistency::live_database_exclusive);

    census_options options;
    options.declared_external_snapshot = true;
    auto copied = db.census(options);
    REQUIRE(copied.has_value());

    // The scope is what was counted, and that did not change. A caveat about
    // provenance inside an enumerated field would make the field unparseable for
    // the sake of a sentence.
    CHECK(copied->scope == "physical_stored");
    CHECK(copied->scope == live->scope);
    CHECK(copied->source.declared_external_snapshot);
    CHECK(copied->source.consistency == snapshot_consistency::not_verified);

    auto const json = to_json(*copied);
    std::error_code ec;
    auto const parsed = boost::json::parse(json, ec);
    INFO(json);
    REQUIRE_FALSE(ec);
    auto const& root = parsed.as_object();
    CHECK(root.at("scope").as_string() == "physical_stored");
    auto const& source = root.at("source").as_object();
    CHECK(source.at("declared_external_snapshot").as_bool());
    CHECK(source.at("consistency").as_string() == "not_verified");
    // And it says that this is the caller's word, not an observation.
    CHECK(source.at("note").as_string().find("cannot verify") != std::string_view::npos);

    auto const text = to_text(*copied);
    CHECK(text.find("declared to be an external copy") != std::string::npos);
    CHECK(to_text(*live).find("declared to be an external copy") == std::string::npos);

    // The numbers are the same either way: a declaration about provenance does
    // not change what was counted.
    CHECK(copied->entries == live->entries);
    CHECK(copied->files_examined == live->files_examined);
    db.close();
}

// =============================================================================
// The checks that only impossible inputs reach
// =============================================================================
//
// A map holding more entries than it has buckets, and a count too large to
// multiply, cannot be produced by a working store — forging a file to reach them
// would be fragile and would break with the next Boost. So they are reached
// directly. Without these the checks are unobservable, and an unobservable check
// is one somebody deletes as dead code.

TEST_CASE("arithmetic on figures read from a file refuses rather than wraps",
          "[census]") {
    constexpr uint64_t max = std::numeric_limits<uint64_t>::max();
    uint64_t out = 0;

    // The ordinary cases, so that the refusals below are not the only thing
    // these functions are ever seen doing.
    CHECK(detail::checked_mul(3, 4, out));
    CHECK(out == 12);
    CHECK(detail::checked_mul(0, max, out));
    CHECK(out == 0);
    CHECK(detail::checked_mul(max, 1, out));
    CHECK(out == max);

    // And the refusal. The failure being guarded against is not a big number, it
    // is a plausibly small one: max * 2 wraps to max - 1.
    out = 0xDEAD;
    CHECK_FALSE(detail::checked_mul(max, 2, out));
    CHECK(out == 0xDEAD);            // untouched, so nothing downstream reads a wrap
    CHECK_FALSE(detail::checked_mul(max / 2 + 1, 2, out));

    uint64_t accumulator = 10;
    CHECK(detail::checked_add(accumulator, 5));
    CHECK(accumulator == 15);
    CHECK_FALSE(detail::checked_add(accumulator, max));
    CHECK(accumulator == 15);        // likewise untouched

    detail::summing add;
    uint64_t total = 0;
    add(total, 100);
    add(total, 200);
    CHECK(add.ok);
    CHECK(total == 300);
    add(total, max);
    CHECK_FALSE(add.ok);
    // One failure poisons the fold: a sum that carried on would be a smaller
    // number wearing the name of a complete one.
    add(total, 1);
    CHECK_FALSE(add.ok);
}

TEST_CASE("a generation claiming more entries than buckets is refused", "[census]") {
    // The ordinary case first, because a check that refuses everything passes a
    // refusal test.
    CHECK(detail::validate_generation_counts(0, 7, 0, 0).has_value());
    CHECK(detail::validate_generation_counts(0, 7, 100, 100).has_value());
    CHECK(detail::validate_generation_counts(3, 7, 99, 100).has_value());

    auto const impossible = detail::validate_generation_counts(2, 7, 101, 100);
    REQUIRE_FALSE(impossible.has_value());
    CHECK(impossible.error() == error_code::entry_corrupt);
}

TEST_CASE("a payload length longer than its class is refused", "[census]") {
    CHECK(detail::validate_payload_length(0, 0, 0, 43).has_value());
    CHECK(detail::validate_payload_length(0, 0, 43, 43).has_value());   // exactly full

    auto const impossible = detail::validate_payload_length(0, 0, 44, 43);
    REQUIRE_FALSE(impossible.has_value());
    CHECK(impossible.error() == error_code::entry_corrupt);
}

TEST_CASE("the group metadata model is the one the sizing instrument uses",
          "[census]") {
    // Stated as literals rather than as the formula, so that changing the formula
    // has to be a decision about both users of it and not an edit to one.
    uint64_t bytes = 0;
    CHECK(detail::group_metadata_model(0, bytes));
    CHECK(bytes == 16);
    CHECK(detail::group_metadata_model(15, bytes));
    CHECK(bytes == 32);
    CHECK(detail::group_metadata_model(959, bytes));
    CHECK(bytes == 1024);
    CHECK(detail::group_metadata_model(122879, bytes));
    CHECK(bytes == 131072);

    // And it refuses rather than wrapping. A bucket count this large cannot
    // reach here today — the caller multiplies it by the slot size first, and
    // that fails long before — but the number is derived from a file either way,
    // and "it is safe because of the order of two functions" is an argument the
    // reader should not have to reconstruct.
    constexpr uint64_t max = std::numeric_limits<uint64_t>::max();
    bytes = 0xBEEF;
    CHECK_FALSE(detail::group_metadata_model(max, bytes));
    CHECK(bytes == 0xBEEF);          // untouched, so nothing downstream reads a wrap
    CHECK_FALSE(detail::group_metadata_model(max - 1, bytes));

    // The largest count that still works, and the first that does not: the
    // boundary itself, so the refusal is not merely "very large numbers fail".
    constexpr uint64_t last_good = (max / 16 - 1) * 15 + 14;
    CHECK(detail::group_metadata_model(last_good, bytes));
    CHECK_FALSE(detail::group_metadata_model(last_good + 1, bytes));
}

// =============================================================================
// The tool, run as a process
// =============================================================================
//
// The library is tested above through its own API. This runs the binary, because
// what an operator gets is the binary: its exit status, its stdout, and whether
// what comes out of it parses.

#ifdef UTXOZ_CENSUS_TOOL

namespace {

struct tool_run {
    int status = -1;
    std::string out;
};

tool_run run_census_tool(fs::path const& dir, std::string const& args) {
    auto const log = dir.parent_path() / (dir.filename().string() + ".census-out.txt");
    auto command = fmt::format("\"{}\" \"{}\" {} > \"{}\" 2>&1",
                               UTXOZ_CENSUS_TOOL, dir.string(), args, log.string());
#ifdef _WIN32
    // cmd.exe strips the outermost pair of quotes; one more keeps the rest.
    command = "\"" + command + "\"";
#endif
    tool_run r;
    r.status = std::system(command.c_str());
    std::ifstream in(log, std::ios::binary);
    r.out.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    std::error_code ec;
    fs::remove(log, ec);
    return r;
}

} // namespace

TEST_CASE("the tool censuses a database and prints a report that parses", "[census]") {
    failpoints::scoped_reset const disarm;
    temp_db t;
    auto const inserted = populate(t.dir);

    // Closed first, and deliberately: the claim is exclusive, so the tool cannot
    // read a directory this process still holds. That is the intended behaviour
    // and the next case pins it.
    auto const json = run_census_tool(t.dir, "--json");
    INFO(json.out);
    REQUIRE(json.status == 0);
    std::error_code ec;
    auto const parsed = boost::json::parse(json.out, ec);
    REQUIRE_FALSE(ec);
    auto const& root = parsed.as_object();
    CHECK(root.at("report_schema_version").as_int64() == 1);
    CHECK(root.at("scope").as_string() == "physical_stored");
    CHECK(root.at("storage_mode").as_string() == "full");
    CHECK(root.at("totals").as_object().at("entries").as_int64() == int64_t(inserted));
    CHECK_FALSE(root.at("source").as_object().at("declared_external_snapshot").as_bool());

    auto const text = run_census_tool(t.dir, "--text");
    REQUIRE(text.status == 0);
    CHECK(text.out.find("scope physical_stored") != std::string::npos);
    CHECK(text.out.find("not distinct outpoints") != std::string::npos);

    // The declaration reaches the report, and does not touch the scope.
    auto const snapshot = run_census_tool(t.dir, "--json --snapshot");
    REQUIRE(snapshot.status == 0);
    auto const snapshot_parsed = boost::json::parse(snapshot.out, ec);
    REQUIRE_FALSE(ec);
    auto const& snapshot_root = snapshot_parsed.as_object();
    CHECK(snapshot_root.at("scope").as_string() == "physical_stored");
    auto const& snapshot_source = snapshot_root.at("source").as_object();
    CHECK(snapshot_source.at("declared_external_snapshot").as_bool());
    CHECK(snapshot_source.at("consistency").as_string() == "not_verified");
}

TEST_CASE("the tool refuses a directory a process is holding", "[census]") {
    // Not a nicety: censusing underneath a writer would read maps while they are
    // being changed. The claim is what makes that impossible, and the tool is
    // required to lose to it rather than work around it.
    failpoints::scoped_reset const disarm;
    temp_db t;
    populate(t.dir);

    auto held = full_db::open_for_testing(t.dir, false);
    REQUIRE(held.has_value());

    auto const refused = run_census_tool(t.dir, "--json");
    CHECK(refused.status != 0);
    CHECK(refused.out.find("cannot open") != std::string::npos);

    held->close();

    // And once the holder lets go, the same command works — which is what shows
    // the refusal was the claim and not something else about the directory.
    auto const allowed = run_census_tool(t.dir, "--json");
    CHECK(allowed.status == 0);
}

TEST_CASE("the tool refuses a directory that is not a database, and creates nothing",
          "[census]") {
    // `open()` creates a database where there is none, which is right for a store
    // and wrong for an instrument. A mistyped path must not leave a new empty
    // database behind and then be reported as holding nothing.
    fs::path const empty = fs::temp_directory_path()
        / fmt::format("utxoz-census-empty-{}", getpid());
    std::error_code ec;
    fs::remove_all(empty, ec);
    fs::create_directories(empty);

    auto const refused = run_census_tool(empty, "--json");
    CHECK(refused.status != 0);
    CHECK(refused.out.find("census:") != std::string::npos);
    CHECK(refused.out.find("there is no database") != std::string::npos);

    // Nothing was written except the claim's own file, which is permanent by
    // design and is not a database — the next attempt refuses just the same. No
    // config, no segments, no metadata: nothing that would make this directory
    // look like a database to the next person or the next program.
    for (auto const& entry : fs::directory_iterator(empty)) {
        INFO("left behind: " << entry.path().filename().string());
        CHECK(entry.path().filename() == detail::database_lock::file_name);
    }
    auto const again = run_census_tool(empty, "--json");
    CHECK(again.status != 0);

    fs::remove_all(empty, ec);
}

#endif // UTXOZ_CENSUS_TOOL
