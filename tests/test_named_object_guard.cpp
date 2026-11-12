// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_named_object_guard.cpp
 * @brief Pins what find_single_named() refuses, and what a refusal costs the
 *        callers.
 *
 * A segment resolves an object by its slot alone: an absent object arrives as a
 * null, and an object of the wrong size as a pointer that looks usable. Every
 * call site used to read the result and carry on, which cost three different
 * things depending on where it happened:
 *
 * - an enumeration returned a subset and reported success;
 * - the entry count at open came out short with nothing said;
 * - a compaction dropped a source's entries from the merge and then deleted the
 *   source, which is the only one of the three that loses data permanently.
 *
 * The last is what makes this worth a test rather than a comment. The cases
 * below cover the guard itself, one enumeration of each mode, and the merge.
 *
 * The guard is a cardinality check and nothing more. It cannot see a layout that
 * reorders fields without changing the total size, and it cannot see a change to
 * the hash, which relocates every key while leaving every size alone. Those need
 * an explicit format epoch and fixtures written by earlier builds; nothing here
 * should be read as covering them.
 */

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
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
#include <catch2/generators/catch_generators.hpp>
#include <fmt/format.h>

#include "shard_paths.hpp"

#include <utxoz/database.hpp>

#include "detail/mapped_segment.hpp"
#include "detail/segment_open.hpp"
#include "detail/segment_stamp.hpp"
#include "detail/store_config_io.hpp"

namespace fs = std::filesystem;

using utxoz::detail::find_single_named;
using utxoz::detail::mapped_segment;
using utxoz::detail::segment_slot;

namespace {

std::atomic<uint64_t> guard_test_counter{0};

fs::path make_unique_path(std::string_view tag) {
    auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto const id = guard_test_counter.fetch_add(1);
    return fs::path(fmt::format("./test_guard_{}_{}_{}_{}", tag, getpid(), ts, id));
}

utxoz::raw_outpoint outpoint_of(uint64_t n) {
    // Every byte varies, as a txid's do. Writing only the low eight and leaving
    // the rest zero makes distinct keys that all share bytes 24..35 — the only
    // ones shard_of() reads — so every one of them lands in shard zero.
    return utxoz::testing::spread_key(n);
}

size_t count_files(fs::path const& dir, std::string_view prefix) {
    size_t n = 0;
    for (auto const& e : fs::directory_iterator(dir)) {
        if (e.path().filename().string().starts_with(prefix)) ++n;
    }
    return n;
}

/// A slot the header has room for and this store assigns to nothing: the last
/// one, so a slot added later takes a lower number first.
constexpr auto unused_slot = segment_slot(utxoz::detail::segment_slot_count - 1);

/// A segment big enough that open_existing_segment() accepts it, holding
/// whatever the caller constructs and nothing else.
constexpr size_t segment_bytes = 16u * 1024u * 1024u;

/// Replaces a version file with a segment that opens perfectly and simply does
/// not hold what the reader is going to ask for. Not truncation and not
/// corruption: open_existing_segment() has no complaint about it, so only the
/// guard stands between this file and a silent misread.
/// Replaces a version file with a segment that opens perfectly, holds no map,
/// and carries a stamp this database would accept — so what is missing is the
/// map and only the map. Without the stamp the file would be refused one check
/// earlier and the map-level refusal, a separate check on a separate object,
/// would never be reached.
void blank_the_map_keeping_the_stamp(fs::path const& dir, fs::path const& data_file,
                                     uint32_t kind, uint64_t version) {
    auto const config = utxoz::detail::read_config_file(dir / "utxoz_config.dat");
    REQUIRE(config.has_value());

    std::error_code ec;
    fs::remove(data_file, ec);
    auto replacement = mapped_segment::create(data_file, segment_bytes);
    REQUIRE(replacement.has_value());
    auto const placed = utxoz::detail::place_stamp(
        **replacement, data_file,
        utxoz::detail::local_identity(config->database_id, kind, version));
    REQUIRE(placed.has_value());
}

/// Rebuilds a version file as one this database would open happily: the stamp it
/// should carry, and an empty map. Used to repair a source so a second
/// compaction has something valid to work with.
void rebuild_as_valid(fs::path const& dir, fs::path const& data_file, uint32_t kind,
                      uint64_t version) {
    auto const config = utxoz::detail::read_config_file(dir / "utxoz_config.dat");
    REQUIRE(config.has_value());

    std::error_code ec;
    fs::remove(data_file, ec);
    auto created = mapped_segment::create(data_file, segment_bytes);
    REQUIRE(created.has_value());
    auto& segment = **created;
    auto const placed = utxoz::detail::place_stamp(
        segment, data_file, utxoz::detail::local_identity(config->database_id, kind, version));
    REQUIRE(placed.has_value());

    using map_t = utxoz::detail::utxo_map<utxoz::container_sizes[0]>;
    REQUIRE(segment.construct<map_t>(
        utxoz::detail::map_object_name,
        64, utxoz::detail::outpoint_hash{}, utxoz::detail::outpoint_equal{},
        segment.get_allocator<map_t::value_type>()) != nullptr);
}

/// Replaces a version file with a segment that opens and holds neither a stamp
/// nor a map: nothing this database wrote.
void blank_the_map(fs::path const& data_file) {
    std::error_code ec;
    fs::remove(data_file, ec);
    auto replacement = mapped_segment::create(data_file, segment_bytes);
    REQUIRE(replacement.has_value());
    // A slot this store never reads, so the file is not simply empty.
    REQUIRE((*replacement)->construct<uint64_t>(unused_slot, uint64_t(0)) != nullptr);
}

struct scratch_segment {
    fs::path path;
    std::unique_ptr<mapped_segment> segment;

    explicit scratch_segment(fs::path p) : path(std::move(p)) {
        std::error_code ec;
        fs::remove(path, ec);
        auto created = mapped_segment::create(path, segment_bytes);
        REQUIRE(created.has_value());
        segment = std::move(*created);
    }

    ~scratch_segment() {
        // Unmapped before it is unlinked. A destructor body runs before the
        // members are destroyed, so leaving this to `segment` would have removed
        // a file that was still mapped — which Windows refuses, quietly, because
        // the removal is the ignoring overload.
        segment.reset();
        std::error_code ec;
        fs::remove(path, ec);
    }

    mapped_segment& operator*() { return *segment; }
};

} // namespace

// =============================================================================
// The guard itself
// =============================================================================

TEST_CASE("find_single_named refuses an object that is not there", "[guard]") {
    scratch_segment s(make_unique_path("absent").string() + ".dat");

    // Something else entirely, so the segment is not simply empty: the guard has
    // to answer about the slot it was asked for, not about the file.
    REQUIRE((*s).construct<uint64_t>(unused_slot, uint64_t(7)) != nullptr);

    auto const found = find_single_named<uint64_t>(*s, segment_slot::map, s.path);
    REQUIRE_FALSE(found.has_value());
    CHECK(found.error() == utxoz::error_code::version_unreadable);
}

TEST_CASE("find_single_named accepts exactly one instance", "[guard]") {
    scratch_segment s(make_unique_path("one").string() + ".dat");

    auto* written = (*s).construct<uint64_t>(segment_slot::map, uint64_t(0x5A5A5A5Au));
    REQUIRE(written != nullptr);

    auto const found = find_single_named<uint64_t>(*s, segment_slot::map, s.path);
    REQUIRE(found.has_value());
    REQUIRE(*found == written);
    CHECK(**found == 0x5A5A5A5Au);
}

TEST_CASE("find_single_named refuses more than one instance", "[guard]") {
    scratch_segment s(make_unique_path("many").string() + ".dat");

    // Three instances in the slot. This is what a size change looks like from
    // the inside: the stored byte count no longer divides into one object.
    REQUIRE((*s).construct<std::array<uint64_t, 3>>(segment_slot::map) != nullptr);

    auto const found = find_single_named<uint64_t>(*s, segment_slot::map, s.path);
    REQUIRE_FALSE(found.has_value());
    CHECK(found.error() == utxoz::error_code::version_unreadable);
}

// =============================================================================
// What opening does with a version it cannot describe
// =============================================================================

namespace {

/// A database with two versions of one container, both holding the same entries.
/// The caller damages whichever it wants and decides when.
struct two_versions {
    fs::path dir;
    size_t entries = 0;

    /// Rotated into being, not copied. Every segment carries a stamp naming the
    /// generation it is, so a version file duplicated under another name is
    /// refused — which is the barrier working, and no way to build a fixture.
    explicit two_versions(std::string_view tag) : dir(make_unique_path(tag)) {
        auto opened = utxoz::full_db::open_for_testing(dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        std::vector<uint8_t> const value(8, 0xAB);
        uint64_t next = 0;
        while (utxoz::testing::generations_reached(dir, 0) < 2) {
            REQUIRE(db.insert(outpoint_of(next++), value, 700000).has_value());
            REQUIRE(next < 2'000'000);   // terminate on rotation, not on patience
        }
        entries = size_t(next);
        db.close();
    }

    [[nodiscard]] fs::path historical() const { return dir / utxoz::testing::class_file(0, 0); }

    ~two_versions() {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
};

/// The same shape in reference mode.
struct two_reference_versions {
    fs::path dir;
    size_t entries = 0;

    explicit two_reference_versions(std::string_view tag) : dir(make_unique_path(tag)) {
        auto opened = utxoz::reference_db::open_for_testing(dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        uint64_t next = 0;
        while (count_files(dir, "compact_v") < 2) {
            REQUIRE(db.insert(outpoint_of(next), 1, uint32_t(next), 700000).has_value());
            ++next;
            REQUIRE(next < 3'000'000);
        }
        entries = size_t(next);
        db.close();
    }

    [[nodiscard]] fs::path historical() const { return dir / "compact_v00000.dat"; }

    ~two_reference_versions() {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
};

} // namespace

TEST_CASE("full: open refuses a catalogued version whose map cannot be reached", "[guard]") {
    // The catalogue is built from the directory, so this version is known to be
    // there. Not being able to read it is not a smaller database: opening anyway
    // would publish a size() short by whatever it held, and that count is a
    // running total for the life of the instance.
    two_versions f("open_full");
    // The stamp is kept, so what is missing is the map and only the map. Without
    // that the file would be refused one check earlier and this case would be
    // pinning the stamp rather than what it says it pins.
    blank_the_map_keeping_the_stamp(f.dir, f.historical(), 0, 0);

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::version_unreadable);
}

TEST_CASE("reference: open refuses a catalogued version whose map cannot be reached", "[guard]") {
    two_reference_versions f("open_ref");
    blank_the_map_keeping_the_stamp(f.dir, f.historical(),
                                    utxoz::detail::reference_container_kind, 0);

    auto const db = utxoz::reference_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::version_unreadable);
}

TEST_CASE("open tells a version it cannot open from one it cannot read", "[guard]") {
    // Two different faults, two different errors. A file too small to be a
    // segment never gets as far as the map, so it is the open that failed;
    // blanking the map leaves a segment that opens perfectly and holds nothing
    // usable. Collapsing them would send an operator looking in the wrong place.
    two_versions f("open_kinds");
    {
        std::ofstream ofs(f.historical(), std::ios::binary | std::ios::trunc);
        ofs << "far too small to be a segment";
    }

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::file_open_failed);
}

namespace {

/// Moves the map slot of a closed version file one byte on, and nothing else.
/// `map_alignment` is the alignment of the map type that file holds.
/// The map's buckets follow it, so the object still lies inside what was
/// allocated: the header agrees with the file and the segment opens. What is
/// wrong is only that a map cannot begin at that address.
void shift_the_map_one_byte(fs::path const& data_file, size_t map_alignment) {
    using utxoz::detail::segment_header;
    auto const field = std::streamoff(offsetof(segment_header, objects)
        + size_t(segment_slot::map) * sizeof(segment_header::object)
        + offsetof(segment_header::object, offset));

    std::fstream f(data_file, std::ios::binary | std::ios::in | std::ios::out);
    REQUIRE(f);
    uint64_t offset = 0;
    f.seekg(field);
    f.read(reinterpret_cast<char*>(&offset), sizeof(offset));
    REQUIRE(f);
    REQUIRE(offset != 0);
    // Where the map really is has to be somewhere a map can be, or moving it
    // would prove nothing. Its own alignment, not the platform's widest: the
    // segment aligns each object to its type.
    REQUIRE(map_alignment > 1);
    REQUIRE(offset % map_alignment == 0);
    ++offset;
    f.seekp(field);
    f.write(reinterpret_cast<char const*>(&offset), sizeof(offset));
    f.close();
    REQUIRE_FALSE(f.fail());
}

} // namespace

TEST_CASE("full: open refuses a version whose map does not begin where a map can", "[guard]") {
    // Damage the size and bounds checks cannot see. Read as a map, the address
    // is the middle of one; the open has to refuse it as a version it cannot
    // read, before anything is asked of it.
    two_versions f("open_misaligned");
    shift_the_map_one_byte(f.historical(),
        alignof(utxoz::detail::utxo_map<utxoz::container_sizes[0]>));

    // The segment itself still opens: this is not the header being refused.
    {
        auto const segment = mapped_segment::open(f.historical());
        REQUIRE(segment.has_value());
    }

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::version_unreadable);
}

TEST_CASE("reference: open refuses a version whose map does not begin where a map can", "[guard]") {
    two_reference_versions f("open_ref_misaligned");
    shift_the_map_one_byte(f.historical(), alignof(utxoz::detail::reference_map_t));

    auto const db = utxoz::reference_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::version_unreadable);
}

TEST_CASE("an intact history opens and counts everything", "[guard]") {
    // The control for the three above. Without it they would equally be pinning
    // "any database with a historical version fails to open".
    two_versions f("open_intact");

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE(db.has_value());
    CHECK(db->size() == f.entries);   // both generations counted, none skipped
}

// =============================================================================
// What the traversals do with a refusal
// =============================================================================

TEST_CASE("for_each_key refuses a version whose map cannot be reached", "[guard]") {
    two_versions f("keys");

    auto db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE(db.has_value());

    // After the open, because the open now refuses this state outright. The
    // traversal is a separate path with its own way of failing, and it is the
    // one a caller reaches while holding a healthy instance.
    blank_the_map_keeping_the_stamp(f.dir, f.historical(), 0, 0);

    // Every key is in the active version, so a traversal that skipped the
    // damaged one would return them all and report success. Its being an error
    // is the property: a partial enumeration is not an enumeration.
    size_t seen = 0;
    auto const outcome = db->for_each_key([&](utxoz::raw_outpoint const&) { ++seen; });

    REQUIRE_FALSE(outcome.has_value());
    CHECK(outcome.error() == utxoz::error_code::version_unreadable);
}

TEST_CASE("for_each_entry refuses a version whose map cannot be reached", "[guard]") {
    two_versions f("entries");

    auto db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE(db.has_value());
    blank_the_map_keeping_the_stamp(f.dir, f.historical(), 0, 0);

    auto const outcome = db->for_each_entry(
        [](utxoz::raw_outpoint const&, uint32_t, std::span<uint8_t const>) {});

    REQUIRE_FALSE(outcome.has_value());
    CHECK(outcome.error() == utxoz::error_code::version_unreadable);
}

TEST_CASE("an undamaged history still traverses", "[guard]") {
    two_versions f("intact");

    auto db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE(db.has_value());

    size_t seen = 0;
    auto const outcome = db->for_each_key([&](utxoz::raw_outpoint const&) { ++seen; });
    CHECK(outcome.has_value());
    CHECK(seen == f.entries);

    db->close();
}

// =============================================================================
// The one that loses data
// =============================================================================

TEST_CASE("compaction refuses a source whose map cannot be reached", "[guard][compaction]") {
    auto const dir = make_unique_path("merge");
    std::error_code ec;
    fs::remove_all(dir, ec);

    // Three versions of container 0, so a merge has two sources to work with.
    {
        auto opened = utxoz::full_db::open_for_testing(dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        std::vector<uint8_t> const value(8, 0x11);
        uint64_t next = 0;
        while (utxoz::testing::generations_reached(dir, 0) < 3) {
            REQUIRE(db.insert(outpoint_of(next++), value, 100).has_value());
            REQUIRE(next < 2'000'000);   // terminate on rotation, not on patience
        }
        db.close();
    }

    auto db = utxoz::full_db::open_for_testing(dir, false);
    REQUIRE(db.has_value());

    // As with the traversals: damaged after the open, since the open refuses
    // this state on its own now.
    auto const source = dir / utxoz::testing::class_file(0, 0);

    auto const expected_error = GENERATE(utxoz::error_code::segment_stamp_missing,
                                         utxoz::error_code::version_unreadable);
    if (expected_error == utxoz::error_code::segment_stamp_missing) {
        // Nothing this database wrote.
        blank_the_map(source);
    } else {
        // Ours, and empty of the one thing the merge needs. This is the case
        // #127 added, still reachable now that the stamp is checked first.
        blank_the_map_keeping_the_stamp(dir, source, 0, 0);
    }

    auto const compacted = db->compact_all();

    // Skipping the source would have published a target without its entries and
    // then unlinked it, which is why both halves are asserted: the refusal, and
    // the file still being there to refuse again.
    REQUIRE_FALSE(compacted.has_value());
    CHECK(compacted.error() == expected_error);
    CHECK(fs::exists(source));

    // And nothing half-built survives the refusal. A `.building` left behind
    // would be a merge's target that was never published — invisible to
    // discovery, and in the way of whatever tries next.
    size_t building = 0;
    for (auto const& e : fs::directory_iterator(dir)) {
        if (e.path().filename().string().ends_with(".building")) ++building;
    }
    CHECK(building == 0);

    // Every source is still where it was: a refusal is not a partial merge.
    // Depth, not a total: the class reached three generations when every one of
    // its containers did. Counting files would give three times the shard count.
    CHECK(utxoz::testing::generations_reached(dir, 0) == 3);

    // And the refusal latches nothing. Repair the source into a file this
    // database accepts and run compaction again: it has to get as far as
    // building, which it cannot do if the first attempt's target is still
    // sitting at the name the second one needs.
    db->close();
    rebuild_as_valid(dir, source, 0, 0);

    auto retried = utxoz::full_db::open_for_testing(dir, false);
    REQUIRE(retried.has_value());
    auto const again = retried->compact_all();
    CHECK(again.has_value());

    retried->close();
    fs::remove_all(dir, ec);
}
