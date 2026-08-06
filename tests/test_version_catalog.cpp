// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_version_catalog.cpp
 * @brief The catalogue of version files: what it accepts, and what it refuses
 *        to guess.
 *
 * Two properties matter here and neither is visible from the public API.
 *
 * A failure to read the set of version files must never be reported as an empty
 * set. That mistake is the one this whole area exists to prevent: absence and
 * unreadability look the same to a caller that only gets a list back, and a
 * database opened as if it had no versions gets a v0 created over the top of the
 * ones it does have.
 *
 * And the catalogue is sparse. Version numbers are identities that only climb,
 * so anything sized or walked by the highest one grows for the life of the
 * database while the database itself does not.
 */

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>

#include "detail/scope_exit.hpp"
#include "detail/version_catalog.hpp"

namespace fs = std::filesystem;
using utxoz::detail::version_catalog;
using utxoz::detail::enumerate_versions;
using utxoz::detail::parse_canonical_version;
using utxoz::detail::scope_exit;

namespace {

inline std::atomic<uint64_t> vc_counter{0};

std::string unique_dir(std::string_view tag) {
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fmt::format("./test_vc_{}_{}_{}_{}", tag, getpid(), ts, vc_counter.fetch_add(1));
}

void touch(fs::path const& p) {
    std::ofstream ofs(p, std::ios::binary);
    ofs.put('x');
}

} // anonymous namespace

TEST_CASE("only canonically named regular files are catalogued", "[catalog]") {
    auto const dir = unique_dir("names");
    fs::create_directories(dir);

    // The names this build writes.
    touch(fs::path(dir) / "cont_0_v00000.dat");
    touch(fs::path(dir) / "cont_0_v00007.dat");
    touch(fs::path(dir) / "cont_0_v12345.dat");   // wider than the pad, still canonical

    // Spellings that name the same number but are not what we would write. A
    // catalogue entry is later formatted back into a path, so an alias would be
    // recorded under one name and reopened under another.
    touch(fs::path(dir) / "cont_0_v7.dat");
    touch(fs::path(dir) / "cont_0_v000007.dat");
    touch(fs::path(dir) / "cont_0_v0007.dat");

    // Not version files at all.
    touch(fs::path(dir) / "cont_0_v.dat");
    touch(fs::path(dir) / "cont_0_vabc.dat");
    touch(fs::path(dir) / "cont_0_v00003.dat.bak");
    touch(fs::path(dir) / "cont_1_v00009.dat");     // another container
    touch(fs::path(dir) / "meta_0_v00000.dat");
    touch(fs::path(dir) / "utxoz_config.dat");

    // A directory whose name would otherwise parse.
    fs::create_directories(fs::path(dir) / "cont_0_v00099.dat");

    auto const listed = enumerate_versions(dir, "cont_0_v");
    REQUIRE(listed);
    REQUIRE(*listed == std::vector<size_t>{0, 7, 12345});

    fs::remove_all(dir);
}

TEST_CASE("canonical parsing round-trips", "[catalog]") {
    CHECK(parse_canonical_version("cont_0_v00000.dat", "cont_0_v") == 0);
    CHECK(parse_canonical_version("cont_0_v00042.dat", "cont_0_v") == 42);
    CHECK(parse_canonical_version("compact_v00003.dat", "compact_v") == 3);

    CHECK_FALSE(parse_canonical_version("cont_0_v42.dat", "cont_0_v").has_value());
    CHECK_FALSE(parse_canonical_version("cont_0_v000042.dat", "cont_0_v").has_value());
    CHECK_FALSE(parse_canonical_version("cont_0_v.dat", "cont_0_v").has_value());
    CHECK_FALSE(parse_canonical_version("cont_0_v0004x.dat", "cont_0_v").has_value());
    CHECK_FALSE(parse_canonical_version("cont_0_v00042.txt", "cont_0_v").has_value());
    CHECK_FALSE(parse_canonical_version("cont_0_v00042", "cont_0_v").has_value());
    // Well past what a size_t holds: rejected, not wrapped or clamped.
    CHECK_FALSE(parse_canonical_version("cont_0_v99999999999999999999999.dat", "cont_0_v").has_value());
}

TEST_CASE("a directory that cannot be read is not an empty directory", "[catalog]") {
    SECTION("a path that does not exist is genuinely empty") {
        auto const listed = enumerate_versions(unique_dir("missing"), "cont_0_v");
        REQUIRE(listed);
        REQUIRE(listed->empty());
    }

    SECTION("a path that is a file is an error, not an empty list") {
        auto const dir = unique_dir("notadir");
        touch(dir);

        auto const listed = enumerate_versions(dir, "cont_0_v");
        REQUIRE_FALSE(listed);
        REQUIRE(listed.error() == utxoz::error_code::catalog_unreadable);

        fs::remove(dir);
    }

#ifndef _WIN32
    SECTION("a directory we cannot open is an error, not an empty list") {
        auto const dir = unique_dir("noperm");
        fs::create_directories(dir);
        touch(fs::path(dir) / "cont_0_v00000.dat");

        // Armed before the permissions come off. Catch2 aborts a test case by
        // throwing, so a restore written after the assertions is skipped on the
        // one run that matters — the failing one — and leaves a directory
        // nothing can read behind for every later run to trip over.
        scope_exit const restore([&] {
            std::error_code ec;
            fs::permissions(dir, fs::perms::owner_all, ec);
            fs::remove_all(dir, ec);
        });

        fs::permissions(dir, fs::perms::none);

        // root ignores the permission bits, so there is nothing to observe.
        std::error_code probe_ec;
        fs::directory_iterator probe(dir, probe_ec);
        if (probe_ec) {
            auto const listed = enumerate_versions(dir, "cont_0_v");
            REQUIRE_FALSE(listed);
            REQUIRE(listed.error() == utxoz::error_code::catalog_unreadable);
        }
    }
#endif
}

TEST_CASE("the catalogue is sparse and its numbers only climb", "[catalog]") {
    version_catalog cat;

    for (size_t v = 0; v <= 1000; ++v) cat.add(v);
    REQUIRE(cat.size() == 1001);
    REQUIRE(cat.active() == 1000);

    // Compaction drains almost everything away.
    for (size_t v = 0; v < 999; ++v) cat.remove(v);
    REQUIRE(cat.size() == 2);
    REQUIRE(cat.versions() == std::vector<size_t>{999, 1000});

    // Nothing is proportional to the highest number any more: two files, two
    // entries, and a walk that visits two versions rather than a thousand.
    REQUIRE(cat.below(1000) == std::vector<size_t>{999});
    REQUIRE(cat.below(1001).size() == 2);

    // A retired identity is not handed out again.
    cat.remove(1000);
    REQUIRE(cat.active() == 999);
    REQUIRE(cat.next_version() == 1001);
    cat.add(cat.next_version());
    REQUIRE(cat.active() == 1001);
    REQUIRE(cat.next_version() == 1002);
}

TEST_CASE("catalogue lookups distinguish unknown from empty", "[catalog]") {
    version_catalog cat;
    cat.add(3);

    // Nothing describes v3 yet. That is "unknown", and a caller must not read it
    // as "no entries" and skip the file.
    REQUIRE(cat.find_metadata(3) == nullptr);
    REQUIRE(cat.find_metadata(9) == nullptr);

    cat.metadata(3).update_on_insert(utxoz::raw_outpoint{}, 100);
    REQUIRE(cat.find_metadata(3) != nullptr);
    REQUIRE(cat.find_metadata(3)->entry_count == 1);

    // Removing a version takes its metadata with it.
    cat.remove(3);
    REQUIRE(cat.find_metadata(3) == nullptr);
    REQUIRE(cat.empty());
}

TEST_CASE("below() is the resolution order: nearest generation first", "[catalog]") {
    version_catalog cat;
    for (auto const v : {2u, 5u, 6u, 11u}) cat.add(v);

    REQUIRE(cat.below(11) == std::vector<size_t>{6, 5, 2});
    REQUIRE(cat.below(5) == std::vector<size_t>{2});
    REQUIRE(cat.below(2).empty());
    REQUIRE(cat.below(0).empty());
}

TEST_CASE("open() fails rather than assume an unreadable database is empty",
          "[database][catalog]") {
#ifndef _WIN32
    auto const path = unique_dir("openperm");
    fs::create_directories(path);

    // A real database, with versions in it.
    {
        auto opened = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(opened);
        auto db = std::move(*opened);
        utxoz::raw_outpoint k{};
        k[0] = 1;
        REQUIRE(db.insert(k, std::vector<uint8_t>{1, 2, 3}, 100).value());
        db.close();
    }

    // Armed before the permissions come off: see above.
    scope_exit const restore([&] {
        std::error_code ec;
        fs::permissions(path, fs::perms::owner_all, ec);
        fs::remove_all(path, ec);
    });

    fs::permissions(path, fs::perms::none);

    std::error_code probe_ec;
    fs::directory_iterator probe(path, probe_ec);
    if (probe_ec) {   // not running as root
        // The alternative is the one that destroys data: treat the unreadable
        // directory as empty, create v0, and write over what is there.
        auto const reopened = utxoz::full_db::open_for_testing(path);
        INFO("open returned " << (reopened ? "a database" : "an error"));
        REQUIRE_FALSE(reopened);
    }
#endif
}
