// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_named_object_guard.cpp
 * @brief Pins what find_single_named() refuses, and what a refusal costs the
 *        callers.
 *
 * Boost.Interprocess resolves a named object by name alone: an absent object and
 * an object of the wrong size both arrive as something that looks usable. Every
 * call site used to read the resulting null and carry on, which cost three
 * different things depending on where it happened:
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

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <boost/interprocess/managed_mapped_file.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>

#include "detail/segment_open.hpp"

namespace bip = boost::interprocess;
namespace fs = std::filesystem;

using utxoz::detail::find_single_named;

namespace {

std::atomic<uint64_t> guard_test_counter{0};

fs::path make_unique_path(std::string_view tag) {
    auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto const id = guard_test_counter.fetch_add(1);
    return fs::path(fmt::format("./test_guard_{}_{}_{}_{}", tag, getpid(), ts, id));
}

utxoz::raw_outpoint outpoint_of(uint64_t n) {
    utxoz::raw_outpoint key{};
    std::memcpy(key.data(), &n, sizeof(n));
    return key;
}

size_t count_files(fs::path const& dir, std::string_view prefix) {
    size_t n = 0;
    for (auto const& e : fs::directory_iterator(dir)) {
        if (e.path().filename().string().starts_with(prefix)) ++n;
    }
    return n;
}

/// A segment big enough that open_existing_segment() accepts it, holding
/// whatever the caller constructs and nothing else.
constexpr size_t segment_bytes = 16u * 1024u * 1024u;

/// Replaces a version file with a segment that opens perfectly and simply does
/// not hold what the reader is going to ask for. Not truncation and not
/// corruption: open_existing_segment() has no complaint about it, so only the
/// guard stands between this file and a silent misread.
void blank_the_map(fs::path const& data_file) {
    std::error_code ec;
    fs::remove(data_file, ec);
    bip::managed_mapped_file replacement(bip::create_only, data_file.c_str(), segment_bytes);
    replacement.construct<uint64_t>("not_the_map")(0);
}

struct scratch_segment {
    fs::path path;
    std::optional<bip::managed_mapped_file> segment;

    explicit scratch_segment(fs::path p) : path(std::move(p)) {
        std::error_code ec;
        fs::remove(path, ec);
        segment.emplace(bip::create_only, path.c_str(), segment_bytes);
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

    bip::managed_mapped_file& operator*() { return *segment; }
};

} // namespace

// =============================================================================
// The guard itself
// =============================================================================

TEST_CASE("find_single_named refuses an object that is not there", "[guard]") {
    scratch_segment s(make_unique_path("absent").string() + ".dat");

    // Something else entirely, so the segment is not simply empty: the guard has
    // to answer about the name it was asked for, not about the file.
    (*s).construct<uint64_t>("something_else")(7);

    auto const found = find_single_named<uint64_t>(*s, "db_map", s.path);
    REQUIRE_FALSE(found.has_value());
    CHECK(found.error() == utxoz::error_code::version_unreadable);
}

TEST_CASE("find_single_named accepts exactly one instance", "[guard]") {
    scratch_segment s(make_unique_path("one").string() + ".dat");

    auto* written = (*s).construct<uint64_t>("db_map")(0x5A5A5A5Au);

    auto const found = find_single_named<uint64_t>(*s, "db_map", s.path);
    REQUIRE(found.has_value());
    REQUIRE(*found == written);
    CHECK(**found == 0x5A5A5A5Au);
}

TEST_CASE("find_single_named refuses more than one instance", "[guard]") {
    scratch_segment s(make_unique_path("many").string() + ".dat");

    // Three instances under the name. This is what a size change looks like from
    // the inside: the stored byte count no longer divides into one object.
    (*s).construct<uint64_t>("db_map")[3](0);

    auto const found = find_single_named<uint64_t>(*s, "db_map", s.path);
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

    explicit two_versions(std::string_view tag, uint32_t count = 8) : dir(make_unique_path(tag)) {
        {
            auto db = utxoz::full_db::open_for_testing(dir, true);
            REQUIRE(db.has_value());
            std::vector<uint8_t> const value(20, 0xAB);
            for (uint32_t i = 0; i < count; ++i) {
                REQUIRE(db->insert(outpoint_of(i), value, 700000 + i).has_value());
            }
        }
        std::error_code ec;
        fs::copy_file(dir / "cont_0_v00000.dat", dir / "cont_0_v00001.dat", ec);
        REQUIRE_FALSE(ec);
    }

    [[nodiscard]] fs::path historical() const { return dir / "cont_0_v00000.dat"; }

    ~two_versions() {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
};

/// The same shape in reference mode.
struct two_reference_versions {
    fs::path dir;

    explicit two_reference_versions(std::string_view tag, uint32_t count = 8)
        : dir(make_unique_path(tag)) {
        {
            auto db = utxoz::reference_db::open_for_testing(dir, true);
            REQUIRE(db.has_value());
            for (uint32_t i = 0; i < count; ++i) {
                REQUIRE(db->insert(outpoint_of(i), 1, i * 100, 700000 + i).has_value());
            }
        }
        std::error_code ec;
        fs::copy_file(dir / "compact_v00000.dat", dir / "compact_v00001.dat", ec);
        REQUIRE_FALSE(ec);
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
    blank_the_map(f.historical());

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::version_unreadable);
}

TEST_CASE("reference: open refuses a catalogued version whose map cannot be reached", "[guard]") {
    two_reference_versions f("open_ref");
    blank_the_map(f.historical());

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

TEST_CASE("an intact history opens and counts everything", "[guard]") {
    // The control for the three above. Without it they would equally be pinning
    // "any database with a historical version fails to open".
    two_versions f("open_intact");

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE(db.has_value());
    CHECK(db->size() == 16);   // both versions hold the same eight
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
    blank_the_map(f.historical());

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
    blank_the_map(f.historical());

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
    CHECK(seen == 16);

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
        while (count_files(dir, "cont_0_v") < 3) {
            REQUIRE(db.insert(outpoint_of(next++), value, 100).has_value());
            REQUIRE(next < 2'000'000);   // terminate on rotation, not on patience
        }
        db.close();
    }

    auto db = utxoz::full_db::open_for_testing(dir, false);
    REQUIRE(db.has_value());

    // As with the traversals: damaged after the open, since the open refuses
    // this state on its own now.
    auto const source = dir / "cont_0_v00000.dat";
    blank_the_map(source);

    auto const compacted = db->compact_all();

    // Skipping the source would have published a target without its entries and
    // then unlinked it, which is why both halves are asserted: the refusal, and
    // the file still being there to refuse again.
    REQUIRE_FALSE(compacted.has_value());
    CHECK(compacted.error() == utxoz::error_code::version_unreadable);
    CHECK(fs::exists(source));

    db->close();
    fs::remove_all(dir, ec);
}
