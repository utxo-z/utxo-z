// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_missing_config.cpp
 * @brief A directory with database files and no config is not a fresh directory.
 *
 * The config is written before any segment, so segments without one are a
 * database whose config has gone. Open-or-create used to look for exactly one
 * file — the other mode's version zero — and otherwise take the directory for a
 * new database: draw an identity, write a config carrying it, and only then go
 * looking at the files, which then belonged to a database the directory no
 * longer named.
 *
 * Every case here checks two things about a refusal: that it happened, and that
 * it was a refusal and not a repair. The config is still absent afterwards, and
 * every file that was there has the bytes it had.
 *
 * The fixtures are made here, in fresh directories. Most of them are a single
 * zero-byte file under a canonical name, because the refusal is decided by name
 * before anything is opened; the real databases are the testing profile's.
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "shard_paths.hpp"
#include "support/read_file.hpp"

#include <utxoz/database.hpp>
#include <utxoz/types.hpp>

#include "detail/database_lock.hpp"
#include "detail/shard.hpp"

namespace fs = std::filesystem;
using namespace utxoz;

namespace {

struct temp_dir {
    temp_dir() {
        static std::atomic<uint64_t> counter{0};
        auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        dir = fmt::format("./test_cfgmiss_{}_{}_{}", getpid(), ts, counter.fetch_add(1));
        fs::create_directories(dir);
    }
    ~temp_dir() {
        std::error_code ec;
        fs::permissions(dir, fs::perms::owner_all, fs::perm_options::add, ec);
        fs::remove_all(dir, ec);
    }
    temp_dir(temp_dir const&) = delete;
    temp_dir& operator=(temp_dir const&) = delete;
    fs::path dir;
};

fs::path config_of(fs::path const& dir) { return dir / "utxoz_config.dat"; }

raw_outpoint key_of(uint64_t n) {
    raw_outpoint k{};
    std::memcpy(k.data(), &n, sizeof(n));
    return k;
}

void touch(fs::path const& p) {
    std::ofstream ofs(p, std::ios::binary);
    REQUIRE(ofs.good());
}

/// What a directory holds, by name: a digest and size of each file's bytes, and
/// a marker for anything that is not a file. The claim's own file is left out:
/// it is permanent by design, and an open that refuses still took the claim.
using snapshot = std::map<std::string, std::string>;

snapshot snapshot_of(fs::path const& dir) {
    snapshot out;
    for (auto const& entry : fs::directory_iterator(dir)) {
        auto const name = entry.path().filename().string();
        if (name == detail::database_lock::file_name) continue;
        if ( ! entry.is_regular_file()) {
            out.emplace(name, "not a file");
            continue;
        }
        auto const bytes = testing::read_file_bytes(entry.path());
        REQUIRE(bytes.size() == entry.file_size());
        std::string_view const view(reinterpret_cast<char const*>(bytes.data()), bytes.size());
        out.emplace(name, fmt::format("{}:{}", bytes.size(), std::hash<std::string_view>{}(view)));
    }
    return out;
}

void make_full(fs::path const& dir) {
    auto opened = full_db::open_for_testing(dir, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);
    std::vector<uint8_t> const value(8, 0x33);
    for (uint64_t i = 1; i <= 40; ++i) {
        REQUIRE(db.insert(key_of(i), value, 800000).has_value());
    }
    db.close();
}

void make_reference(fs::path const& dir) {
    auto opened = reference_db::open_for_testing(dir, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);
    for (uint64_t i = 1; i <= 40; ++i) {
        REQUIRE(db.insert(key_of(i), uint32_t(i), uint32_t(i * 10), 800000).has_value());
    }
    db.close();
}

template <typename Db>
error_code refusal_of(fs::path const& dir) {
    auto opened = Db::open_for_testing(dir, false);
    REQUIRE_FALSE(opened.has_value());
    return opened.error();
}

/// Opens `dir` as `Db`, expects `expected`, and expects nothing to have changed:
/// no config written and every file exactly as it was.
template <typename Db>
void check_refused(fs::path const& dir, error_code expected) {
    auto const before = snapshot_of(dir);
    REQUIRE_FALSE(fs::exists(config_of(dir)));

    CHECK(refusal_of<Db>(dir) == expected);

    CHECK_FALSE(fs::exists(config_of(dir)));
    CHECK(snapshot_of(dir) == before);
}

} // namespace

TEST_CASE("full data whose config is gone is not created over", "[open][config]") {
    temp_dir t;
    make_full(t.dir);
    REQUIRE(fs::remove(config_of(t.dir)));

    check_refused<full_db>(t.dir, error_code::config_missing);
    check_refused<reference_db>(t.dir, error_code::storage_mode_mismatch);
}

TEST_CASE("reference data whose config is gone is not created over", "[open][config]") {
    temp_dir t;
    make_reference(t.dir);
    REQUIRE(fs::remove(config_of(t.dir)));

    check_refused<reference_db>(t.dir, error_code::config_missing);
    check_refused<full_db>(t.dir, error_code::storage_mode_mismatch);
}

TEST_CASE("full data without its generation zero is still a database", "[open][config]") {
    // Generation zero of the first slot is the one file the old check looked
    // for, from the other mode only.
    temp_dir t;
    make_full(t.dir);
    REQUIRE(fs::remove(config_of(t.dir)));
    REQUIRE(fs::remove(t.dir / testing::class_file(0, 0, 0)));

    check_refused<reference_db>(t.dir, error_code::storage_mode_mismatch);
    check_refused<full_db>(t.dir, error_code::config_missing);
}

TEST_CASE("a later generation alone is a database", "[open][config]") {
    SECTION("reference") {
        temp_dir t;
        touch(t.dir / "compact_v00007.dat");
        check_refused<full_db>(t.dir, error_code::storage_mode_mismatch);
        check_refused<reference_db>(t.dir, error_code::config_missing);
    }
    SECTION("full") {
        temp_dir t;
        touch(t.dir / testing::class_file(0, 9));
        check_refused<reference_db>(t.dir, error_code::storage_mode_mismatch);
        check_refused<full_db>(t.dir, error_code::config_missing);
    }
}

TEST_CASE("every slot counts, each shard of the sharded class included", "[open][config]") {
    for (size_t slot = 0; slot != detail::slot_count; ++slot) {
        CAPTURE(slot);
        temp_dir t;
        touch(t.dir / detail::container_file_name(slot, 3));
        check_refused<reference_db>(t.dir, error_code::storage_mode_mismatch);
        check_refused<full_db>(t.dir, error_code::config_missing);
    }
}

TEST_CASE("metadata, merge records and unfinished builds are a database too", "[open][config]") {
    auto const last_slot = detail::slot_count - 1;
    std::vector<std::string> const full_names{
        detail::metadata_file_name(last_slot, 2),
        detail::sidecar_file_name(1, 4),
        detail::container_file_name(2, 5) + ".building",
        detail::sidecar_file_name(0, 6) + ".tmp",
    };
    for (auto const& name : full_names) {
        CAPTURE(name);
        temp_dir t;
        touch(t.dir / name);
        check_refused<full_db>(t.dir, error_code::config_missing);
        check_refused<reference_db>(t.dir, error_code::storage_mode_mismatch);
    }

    for (auto const& name : {"meta_compact_v00001.dat", "compact_v00002.merge"}) {
        CAPTURE(name);
        temp_dir t;
        touch(t.dir / name);
        check_refused<reference_db>(t.dir, error_code::config_missing);
        check_refused<full_db>(t.dir, error_code::storage_mode_mismatch);
    }
}

TEST_CASE("the sharded class under another shard count is a database too", "[open][config]") {
    // Names a build of another geometry writes. Its config would have refused it
    // by geometry; with no config the name is all there is.
    for (auto const& name : {"cont_0_v00000.dat", "cont_0s31_v00000.dat", "meta_0s16_v00001.dat"}) {
        CAPTURE(name);
        temp_dir t;
        touch(t.dir / name);
        check_refused<full_db>(t.dir, error_code::config_missing);
    }
}

TEST_CASE("a directory with only unrelated files is still a fresh directory", "[open][config]") {
    // Names the catalogue does not read as ours, and a canonical name one level
    // down, which no open ever looks at.
    auto populate = [](fs::path const& dir) {
        touch(dir / "notes.txt");
        touch(dir / "cont_0s0_v7.dat");          // not the canonical spelling
        touch(dir / "compact_v000001.dat");      // nor this
        touch(dir / "cont_9_v00000.dat");        // no such class
        fs::create_directories(dir / "backup");
        touch(dir / "backup" / "compact_v00000.dat");
    };

    SECTION("full") {
        temp_dir t;
        populate(t.dir);
        auto const before = snapshot_of(t.dir);
        auto opened = full_db::open_for_testing(t.dir, false);
        REQUIRE(opened.has_value());
        opened->close();
        CHECK(fs::exists(config_of(t.dir)));
        auto after = snapshot_of(t.dir);
        for (auto const& [name, digest] : before) CHECK(after.at(name) == digest);
    }
    SECTION("reference") {
        temp_dir t;
        populate(t.dir);
        auto const before = snapshot_of(t.dir);
        auto opened = reference_db::open_for_testing(t.dir, false);
        REQUIRE(opened.has_value());
        opened->close();
        CHECK(fs::exists(config_of(t.dir)));
        auto after = snapshot_of(t.dir);
        for (auto const& [name, digest] : before) CHECK(after.at(name) == digest);
    }
}

TEST_CASE("remove_existing still discards files with no config", "[open][config]") {
    // Destructive by request, and unchanged: the caller asked for this.
    temp_dir t;
    make_full(t.dir);
    REQUIRE(fs::remove(config_of(t.dir)));

    auto opened = full_db::open_for_testing(t.dir, true);
    REQUIRE(opened.has_value());
    CHECK(opened->size() == 0);
    opened->close();
    CHECK(fs::exists(config_of(t.dir)));
}

TEST_CASE("an inspection of files with no config still creates nothing", "[open][config]") {
    temp_dir t;
    make_full(t.dir);
    REQUIRE(fs::remove(config_of(t.dir)));
    auto const before = snapshot_of(t.dir);

    auto opened = full_db::open_for_inspection_for_testing(t.dir);
    REQUIRE_FALSE(opened.has_value());
    CHECK(opened.error() == error_code::database_not_found);
    CHECK_FALSE(fs::exists(config_of(t.dir)));
    CHECK(snapshot_of(t.dir) == before);
}

#ifndef _WIN32
TEST_CASE("a directory that cannot be listed is not an empty one", "[open][config]") {
    // Writable and searchable, so the claim can be taken and the config looked
    // for, and not readable, so nothing in it can be listed. Root reads it
    // anyway, which would make this a different test.
    if (geteuid() == 0) SKIP("running as root: permissions do not stop listing");

    temp_dir t;
    fs::permissions(t.dir, fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);

    auto opened = full_db::open_for_testing(t.dir, false);
    REQUIRE_FALSE(opened.has_value());
    CHECK(opened.error() == error_code::catalog_unreadable);
    CHECK_FALSE(fs::exists(config_of(t.dir)));

    fs::permissions(t.dir, fs::perms::owner_all, fs::perm_options::add);
}
#endif
