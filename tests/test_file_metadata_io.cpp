// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_file_metadata_io.cpp
 * @brief A metadata record is either whole or absent. There is no third state.
 *
 * Metadata is derived and its absence is ordinary, so nothing here is about
 * keeping records alive. It is about the one outcome that must never occur: a
 * damaged record read back as a valid one. A summary exists to let a search
 * skip a file, so a summary that is wrong turns a key that is present into a
 * key that is missing — and it does so silently, on the read path, long after
 * the write that damaged it.
 *
 * The reader used to check that the file opened and then check nothing else,
 * filling a record that had already been created in the catalogue. Every case
 * below produced a present, apparently valid record under that reader.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <utxoz/database.hpp>
#include <fmt/format.h>

#include "detail/durability.hpp"
#include "detail/file_metadata_io.hpp"
#include "detail/scope_exit.hpp"

namespace fs = std::filesystem;
using utxoz::detail::file_metadata;
using utxoz::detail::metadata_read_error;
using utxoz::detail::metadata_record;
using utxoz::detail::encode_metadata;
using utxoz::detail::read_metadata_file;
using utxoz::detail::write_metadata_file;
using utxoz::detail::scope_exit;
using utxoz::detail::failpoints;
using utxoz::detail::metadata_sync;
using utxoz::detail::platform_sync_support;
using utxoz::detail::sync_support;

namespace {

inline std::atomic<uint64_t> md_counter{0};

std::string unique_dir(std::string_view tag) {
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fmt::format("./test_md_{}_{}_{}_{}", tag, getpid(), ts, md_counter.fetch_add(1));
}

utxoz::raw_outpoint key_of(uint8_t seed) {
    utxoz::raw_outpoint k{};
    k.fill(seed);
    return k;
}

/// A record with every field set to something distinguishable.
file_metadata sample() {
    file_metadata meta;
    meta.update_on_insert(key_of(0x20), 500);
    meta.update_on_insert(key_of(0x80), 900);
    return meta;
}

void write_bytes(fs::path const& p, std::span<uint8_t const> bytes) {
    std::ofstream ofs(p, std::ios::binary | std::ios::trunc);
    ofs.write(reinterpret_cast<char const*>(bytes.data()), std::streamsize(bytes.size()));
}

} // anonymous namespace

TEST_CASE("a published record round-trips exactly", "[metadata]") {
    auto const dir = unique_dir("roundtrip");
    fs::create_directories(dir);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(dir, ec); });

    auto const path = fs::path(dir) / "meta_0_v00000.dat";
    auto const original = sample();

    REQUIRE(write_metadata_file(path, original));

    auto const read = read_metadata_file(path);
    REQUIRE(read);
    CHECK(read->min_block_height == original.min_block_height);
    CHECK(read->max_block_height == original.max_block_height);
    CHECK(read->min_key == original.min_key);
    CHECK(read->max_key == original.max_key);
    CHECK(read->entry_count == original.entry_count);

    // Published, not written in place: no temp survives a successful write.
    CHECK_FALSE(fs::exists(fs::path(dir) / "meta_0_v00000.dat.tmp"));
}

TEST_CASE("an empty record round-trips too", "[metadata]") {
    auto const dir = unique_dir("empty");
    fs::create_directories(dir);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(dir, ec); });

    auto const path = fs::path(dir) / "meta_0_v00000.dat";

    // A default record carries the sentinel range — min above max, no entries —
    // which the coherence check has to accept rather than read as impossible.
    file_metadata const fresh;
    REQUIRE(fresh.entry_count == 0);
    REQUIRE(write_metadata_file(path, fresh));

    auto const read = read_metadata_file(path);
    REQUIRE(read);
    CHECK(read->entry_count == 0);
}

TEST_CASE("no file at all is absent, which is an ordinary state", "[metadata]") {
    auto const dir = unique_dir("absent");
    fs::create_directories(dir);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(dir, ec); });

    auto const read = read_metadata_file(fs::path(dir) / "meta_0_v00000.dat");
    REQUIRE_FALSE(read);
    CHECK(read.error() == metadata_read_error::absent);
}

TEST_CASE("a record truncated at any point is refused", "[metadata]") {
    auto const dir = unique_dir("truncated");
    fs::create_directories(dir);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(dir, ec); });

    auto const path = fs::path(dir) / "meta_0_v00000.dat";
    auto const whole = encode_metadata(sample());
    REQUIRE(whole.size() == metadata_record::encoded_size);

    // Every prefix, not a sample of them: a write can stop at any byte, and the
    // interesting ones are the field boundaries, which this covers by covering
    // everything.
    for (size_t cut = 0; cut < whole.size(); ++cut) {
        INFO("truncated to " << cut << " of " << whole.size() << " bytes");
        write_bytes(path, std::span<uint8_t const>(whole.data(), cut));

        auto const read = read_metadata_file(path);
        REQUIRE_FALSE(read);
        // A zero-length file is still a file: absent means no file, and reading
        // an empty one as absent would let a truncation pass as normal.
        CHECK(read.error() == metadata_read_error::malformed);
    }
}

TEST_CASE("a record with anything appended is refused", "[metadata]") {
    auto const dir = unique_dir("trailing");
    fs::create_directories(dir);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(dir, ec); });

    auto const path = fs::path(dir) / "meta_0_v00000.dat";

    for (size_t extra : {size_t(1), size_t(7), size_t(64)}) {
        INFO("trailing bytes: " << extra);
        auto bytes = encode_metadata(sample());
        bytes.insert(bytes.end(), extra, 0x00);
        write_bytes(path, bytes);

        auto const read = read_metadata_file(path);
        REQUIRE_FALSE(read);
        CHECK(read.error() == metadata_read_error::malformed);
    }
}

TEST_CASE("a record that is not ours, or from a layout we do not know, is refused",
          "[metadata]") {
    auto const dir = unique_dir("foreign");
    fs::create_directories(dir);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(dir, ec); });

    auto const path = fs::path(dir) / "meta_0_v00000.dat";

    SECTION("wrong marker") {
        auto bytes = encode_metadata(sample());
        bytes[0] = 'X';
        write_bytes(path, bytes);

        auto const read = read_metadata_file(path);
        REQUIRE_FALSE(read);
        CHECK(read.error() == metadata_read_error::foreign);
    }

    SECTION("a format version this build does not read") {
        auto bytes = encode_metadata(sample());
        uint16_t const future = metadata_record::current_format + 1;
        std::memcpy(bytes.data() + 4, &future, sizeof(future));
        write_bytes(path, bytes);

        auto const read = read_metadata_file(path);
        REQUIRE_FALSE(read);
        CHECK(read.error() == metadata_read_error::foreign);
    }

    SECTION("a record written by the build before this one") {
        // The previous layout: min height, max height, two keys, count — 88
        // bytes, no marker and no checksum. A database written by that build is
        // old, not damaged, and has to be reported that way or every open of one
        // announces a corruption that has not happened.
        std::vector<uint8_t> old_style(88, 0x00);
        uint32_t const min_height = 100;
        uint32_t const max_height = 200;
        std::memcpy(old_style.data(), &min_height, sizeof(min_height));
        std::memcpy(old_style.data() + 4, &max_height, sizeof(max_height));
        write_bytes(path, old_style);

        REQUIRE(fs::file_size(path) != metadata_record::encoded_size);

        auto const read = read_metadata_file(path);
        REQUIRE_FALSE(read);
        CHECK(read.error() == metadata_read_error::foreign);
    }

    SECTION("something else entirely, of the right length") {
        std::vector<uint8_t> junk(metadata_record::encoded_size, 0x5A);
        write_bytes(path, junk);

        auto const read = read_metadata_file(path);
        REQUIRE_FALSE(read);
        CHECK(read.error() == metadata_read_error::foreign);
    }
}

TEST_CASE("a single flipped byte is caught by the checksum", "[metadata]") {
    auto const dir = unique_dir("checksum");
    fs::create_directories(dir);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(dir, ec); });

    auto const path = fs::path(dir) / "meta_0_v00000.dat";
    auto const whole = encode_metadata(sample());

    // Every byte after the marker and format, which have their own checks.
    for (size_t i = 8; i < whole.size(); ++i) {
        auto bytes = whole;
        bytes[i] ^= 0xFF;
        write_bytes(path, bytes);

        auto const read = read_metadata_file(path);
        INFO("flipped byte " << i);
        REQUIRE_FALSE(read);
        CHECK(read.error() == metadata_read_error::malformed);
    }
}

TEST_CASE("a record that is intact but describes something impossible is refused",
          "[metadata]") {
    auto const dir = unique_dir("incoherent");
    fs::create_directories(dir);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(dir, ec); });

    auto const path = fs::path(dir) / "meta_0_v00000.dat";

    // Built by hand and checksummed, so it is internally intact: the only thing
    // wrong with it is what it says. A checksum proves a record survived the
    // trip, not that it was true when it left.
    auto make_checksummed = [](file_metadata const& meta) {
        return encode_metadata(meta);
    };

    SECTION("heights inverted with entries present") {
        file_metadata meta;
        meta.entry_count = 5;
        meta.min_block_height = 900;
        meta.max_block_height = 500;
        meta.min_key = key_of(0x10);
        meta.max_key = key_of(0x90);
        write_bytes(path, make_checksummed(meta));

        auto const read = read_metadata_file(path);
        REQUIRE_FALSE(read);
        CHECK(read.error() == metadata_read_error::malformed);
    }

    SECTION("keys inverted with entries present") {
        file_metadata meta;
        meta.entry_count = 5;
        meta.min_block_height = 100;
        meta.max_block_height = 200;
        meta.min_key = key_of(0x90);
        meta.max_key = key_of(0x10);
        write_bytes(path, make_checksummed(meta));

        auto const read = read_metadata_file(path);
        REQUIRE_FALSE(read);
        CHECK(read.error() == metadata_read_error::malformed);
    }
}

TEST_CASE("a directory that cannot be read gives unreadable, not absent", "[metadata]") {
#ifndef _WIN32
    auto const dir = unique_dir("noperm");
    fs::create_directories(dir);

    auto const path = fs::path(dir) / "meta_0_v00000.dat";
    REQUIRE(write_metadata_file(path, sample()));

    scope_exit const cleanup([&] {
        std::error_code ec;
        fs::permissions(dir, fs::perms::owner_all, ec);
        fs::remove_all(dir, ec);
    });

    fs::permissions(dir, fs::perms::none);

    std::error_code probe_ec;
    fs::directory_iterator probe(dir, probe_ec);
    if (probe_ec) {   // not running as root
        auto const read = read_metadata_file(path);
        REQUIRE_FALSE(read);
        // Not `absent`. A record we cannot look at is not a record that is not
        // there, and the two must not collapse into one answer.
        CHECK(read.error() == metadata_read_error::unreadable);
    }
#endif
}

TEST_CASE("a failed publication leaves no file and no temp behind", "[metadata]") {
#ifndef _WIN32
    auto const dir = unique_dir("writefail");
    fs::create_directories(dir);

    scope_exit const cleanup([&] {
        std::error_code ec;
        fs::permissions(dir, fs::perms::owner_all, ec);
        fs::remove_all(dir, ec);
    });

    // Read and traverse but not write: creating the temp fails.
    fs::permissions(dir, fs::perms::owner_read | fs::perms::owner_exec);

    std::ofstream probe(fs::path(dir) / "probe.tmp");
    bool const blocked = ! probe.is_open();
    probe.close();

    if (blocked) {
        auto const path = fs::path(dir) / "meta_0_v00000.dat";
        auto const written = write_metadata_file(path, sample());
        REQUIRE_FALSE(written);
        CHECK(written.error() == utxoz::error_code::metadata_write_failed);

        fs::permissions(dir, fs::perms::owner_all);
        CHECK_FALSE(fs::exists(path));
        CHECK_FALSE(fs::exists(fs::path(dir) / "meta_0_v00000.dat.tmp"));
    }
#endif
}

TEST_CASE("publishing over an existing record replaces it whole", "[metadata]") {
    auto const dir = unique_dir("replace");
    fs::create_directories(dir);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(dir, ec); });

    auto const path = fs::path(dir) / "meta_0_v00000.dat";

    file_metadata first;
    first.update_on_insert(key_of(0x11), 10);
    REQUIRE(write_metadata_file(path, first));

    file_metadata second;
    second.update_on_insert(key_of(0x22), 20);
    second.update_on_insert(key_of(0x33), 30);
    REQUIRE(write_metadata_file(path, second));

    auto const read = read_metadata_file(path);
    REQUIRE(read);
    CHECK(read->entry_count == 2);
    CHECK(read->max_block_height == 30);
    CHECK(fs::file_size(path) == metadata_record::encoded_size);
}

/**
 * The store-level consequence. A damaged summary must cost a rescan and nothing
 * else — the version file it describes is untouched, and every key in it stays
 * reachable. Under the previous reader the damaged record was loaded as a
 * present one, which is the state that would later let a search skip the file.
 */
TEST_CASE("a damaged metadata file does not affect what the database can find",
          "[metadata][database]") {
    auto const path = unique_dir("dbcorrupt");
    fs::remove_all(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    std::vector<utxoz::raw_outpoint> keys;
    {
        auto opened = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(opened);
        auto db = std::move(*opened);

        for (uint64_t i = 0; i < 500; ++i) {
            utxoz::raw_outpoint k{};
            std::memcpy(k.data(), &i, sizeof(i));
            REQUIRE(db.insert(k, std::vector<uint8_t>{1, 2, 3, 4}, 100).value());
            keys.push_back(k);
        }
        db.close();
    }

    // Damage every metadata record the database wrote.
    size_t damaged = 0;
    for (auto const& entry : fs::directory_iterator(path)) {
        auto const name = entry.path().filename().string();
        if (name.rfind("meta_", 0) != 0) continue;

        auto bytes = std::vector<uint8_t>(metadata_record::encoded_size / 2, 0);
        {
            std::ifstream ifs(entry.path(), std::ios::binary);
            ifs.read(reinterpret_cast<char*>(bytes.data()), std::streamsize(bytes.size()));
        }
        write_bytes(entry.path(), bytes);   // half a record: ours, and cut short
        ++damaged;
    }
    INFO("damaged " << damaged << " metadata files");
    REQUIRE(damaged > 0);

    auto reopened = utxoz::full_db::open_for_testing(path);
    REQUIRE(reopened);
    auto db = std::move(*reopened);

    for (auto const& k : keys) {
        REQUIRE(db.find(k, 200));
    }

    // And it still takes writes, which republishes sound records.
    utxoz::raw_outpoint fresh{};
    fresh.fill(0xEE);
    REQUIRE(db.insert(fresh, std::vector<uint8_t>{9}, 300).value());
    REQUIRE(db.find(fresh, 300));

    db.close();
}

TEST_CASE("a reserved field that is not zero is refused", "[metadata]") {
    auto const dir = unique_dir("reserved");
    fs::create_directories(dir);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(dir, ec); });

    auto const path = fs::path(dir) / "meta_0_v00000.dat";

    // Set the reserved field and recompute the checksum, so the record is
    // internally intact and the only thing wrong with it is a field this format
    // requires to be zero. Without recomputing, the checksum would catch it and
    // the reserved check would never be exercised.
    auto bytes = encode_metadata(sample());
    uint16_t const not_zero = 1;
    std::memcpy(bytes.data() + 6, &not_zero, sizeof(not_zero));

    auto const covered = bytes.size() - sizeof(uint32_t);
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < covered; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    std::memcpy(bytes.data() + covered, &hash, sizeof(hash));

    write_bytes(path, bytes);

    auto const read = read_metadata_file(path);
    REQUIRE_FALSE(read);
    CHECK(read.error() == metadata_read_error::malformed);
}

TEST_CASE("a file far larger than a record, but ours, is damage and not a foreign format",
          "[metadata]") {
    auto const dir = unique_dir("huge");
    fs::create_directories(dir);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(dir, ec); });

    auto const path = fs::path(dir) / "meta_0_v00000.dat";

    std::vector<uint8_t> big(1u << 20, 0x00);
    std::ranges::copy(metadata_record::magic, reinterpret_cast<char*>(big.data()));
    write_bytes(path, big);

    auto const read = read_metadata_file(path);
    REQUIRE_FALSE(read);
    CHECK(read.error() == metadata_read_error::malformed);
}

TEST_CASE("a failed replace leaves the previous record intact", "[metadata][failpoint]") {
    auto const dir = unique_dir("replacefail");
    fs::create_directories(dir);
    scope_exit const cleanup([&] {
        failpoints::clear();
        std::error_code ec;
        fs::remove_all(dir, ec);
    });

    auto const path = fs::path(dir) / "meta_0_v00000.dat";

    file_metadata first;
    first.update_on_insert(key_of(0x11), 10);
    REQUIRE(write_metadata_file(path, first));

    failpoints::fail_replace.store(true, std::memory_order_relaxed);

    file_metadata second;
    second.update_on_insert(key_of(0x22), 20);
    second.update_on_insert(key_of(0x33), 30);

    auto const written = write_metadata_file(path, second);
    REQUIRE_FALSE(written);
    CHECK(written.error() == utxoz::error_code::metadata_write_failed);

    failpoints::clear();

    // The claim this makes is narrow and it has to hold exactly: a failure at
    // or before the replace leaves the record that was there.
    auto const read = read_metadata_file(path);
    REQUIRE(read);
    CHECK(read->entry_count == 1);
    CHECK(read->max_block_height == 10);

    // And no temp is left lying beside it.
    CHECK_FALSE(fs::exists(fs::path(dir) / "meta_0_v00000.dat.tmp"));
}

TEST_CASE("a barrier that fails after the replace reports sync_failed, not a failed write",
          "[metadata][failpoint]") {
    if constexpr (platform_sync_support() == sync_support::none) return;

    auto const dir = unique_dir("syncfail");
    fs::create_directories(dir);
    scope_exit const cleanup([&] {
        failpoints::clear();
        std::error_code ec;
        fs::remove_all(dir, ec);
    });

    auto const path = fs::path(dir) / "meta_0_v00000.dat";

    file_metadata first;
    first.update_on_insert(key_of(0x11), 10);
    REQUIRE(write_metadata_file(path, first));

    failpoints::fail_sync_directory.store(true, std::memory_order_relaxed);

    file_metadata second;
    second.update_on_insert(key_of(0x22), 20);
    second.update_on_insert(key_of(0x33), 30);

    auto const written = write_metadata_file(path, second, metadata_sync::durable);
    REQUIRE_FALSE(written);

    // Not metadata_write_failed. The replace happened; a replace cannot be
    // undone, and reporting a failed write would say the previous record
    // survived when it did not.
    CHECK(written.error() == utxoz::error_code::sync_failed);

    failpoints::clear();

    auto const read = read_metadata_file(path);
    REQUIRE(read);
    CHECK(read->entry_count == 2);
    CHECK(read->max_block_height == 30);
}

TEST_CASE("a failed content barrier happens before the replace", "[metadata][failpoint]") {
    if constexpr (platform_sync_support() == sync_support::none) return;

    auto const dir = unique_dir("filesyncfail");
    fs::create_directories(dir);
    scope_exit const cleanup([&] {
        failpoints::clear();
        std::error_code ec;
        fs::remove_all(dir, ec);
    });

    auto const path = fs::path(dir) / "meta_0_v00000.dat";

    file_metadata first;
    first.update_on_insert(key_of(0x11), 10);
    REQUIRE(write_metadata_file(path, first));

    failpoints::fail_sync_file.store(true, std::memory_order_relaxed);

    file_metadata second;
    second.update_on_insert(key_of(0x22), 20);

    auto const written = write_metadata_file(path, second, metadata_sync::durable);
    REQUIRE_FALSE(written);
    CHECK(written.error() == utxoz::error_code::metadata_write_failed);

    failpoints::clear();

    auto const read = read_metadata_file(path);
    REQUIRE(read);
    CHECK(read->entry_count == 1);
    CHECK_FALSE(fs::exists(fs::path(dir) / "meta_0_v00000.dat.tmp"));
}
