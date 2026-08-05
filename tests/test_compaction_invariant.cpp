// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_compaction_invariant.cpp
 * @brief A stored key is unique across the database; compaction reports a
 *        violation instead of quietly resolving it.
 *
 * Compaction is the one routine operation that holds two version files open at
 * once, so it is where the store can notice two entries sharing a key without
 * paying for a search. It used to ignore the result of the target insert and
 * erase the source entry regardless, which silently dropped one of the two
 * copies — an arbitrary repair, decided by which file happened to be the
 * target, that also left the entry counter describing a database that no longer
 * existed.
 *
 * A duplicate is local inconsistency. It is reported, never resolved: choosing
 * a copy would hide a corrupt database behind a plausible answer.
 */

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <numeric>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>

namespace {

inline std::atomic<uint64_t> ci_counter{0};

std::string make_unique_path(std::string_view tag) {
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fmt::format("./test_ci_{}_{}_{}_{}", tag, getpid(), ts, ci_counter.fetch_add(1));
}

utxoz::raw_outpoint make_key(uint64_t n) {
    utxoz::raw_outpoint key{};
    std::memcpy(key.data(), &n, sizeof(n));
    return key;
}

std::vector<uint8_t> make_value(size_t size, uint8_t seed) {
    std::vector<uint8_t> v(size);
    std::iota(v.begin(), v.end(), seed);
    return v;
}

template<typename Db>
size_t count_occurrences(Db const& db, utxoz::raw_outpoint const& needle) {
    size_t n = 0;
    db.for_each_key([&](utxoz::raw_outpoint const& k) {
        if (k == needle) ++n;
    });
    return n;
}

} // anonymous namespace

/**
 * Builds the violation the way the store itself can produce it: insert only
 * checks the active map, so once a container has rotated, inserting a key that
 * still lives in an older version puts a second physical entry in the database.
 */
TEST_CASE("compaction reports a duplicate key instead of dropping one",
          "[database][compaction][invariant]") {
    auto const path = make_unique_path("full");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        auto const dup = make_key(0xD00D);

        // First copy, in what will become a rotated-out version.
        REQUIRE(db.insert(dup, make_value(33, 0x11), 100).value());

        // Fill until container 0 rotates, so the first copy is no longer in the
        // active map and insert cannot see it. Keys are recorded per chunk and a
        // chunk that straddles the rotation is dropped, so everything kept is
        // known to live in version 0 alongside the first copy.
        uint64_t filler = 1'000'000;
        std::vector<utxoz::raw_outpoint> v0_keys;
        auto const rotations = [&] { return db.get_statistics().rotations_per_container[0]; };

        while (rotations() == 0) {
            size_t const before = rotations();
            std::vector<utxoz::raw_outpoint> chunk;
            for (size_t i = 0; i < 1'000; ++i) {
                auto const k = make_key(filler++);
                REQUIRE(db.insert(k, make_value(33, 1), 200).has_value());
                chunk.push_back(k);
            }
            if (before == rotations()) {
                v0_keys.insert(v0_keys.end(), chunk.begin(), chunk.end());
            }
        }
        REQUIRE(rotations() >= 1);
        REQUIRE_FALSE(v0_keys.empty());

        // Second copy, into the new active map. insert() only checks there, so
        // it succeeds and the database now holds two entries for one key.
        REQUIRE(db.insert(dup, make_value(33, 0x22), 300).value());
        REQUIRE(count_occurrences(db, dup) == 2);

        // Version 0 rotated because it was full, and compaction only moves
        // entries into a target that has room — with none, it would move nothing
        // and never compare a key against the target at all. Free most of
        // version 0, leaving the duplicate itself in place.
        for (auto const& k : v0_keys) {
            (void)db.erase(k, 300);
        }
        {
            auto const [deleted, failed] = db.process_pending_deletions();
            CHECK(failed.empty());
            CHECK(deleted > 0);
        }
        REQUIRE(count_occurrences(db, dup) == 2);

        auto const size_before = db.size();

        // Compaction sees both and must refuse.
        auto const compacted = db.compact_all();
        REQUIRE_FALSE(compacted.has_value());
        CHECK(compacted.error() == utxoz::error_code::duplicate_key);

        // Neither copy was removed, and the counter still describes what is on
        // disk — reporting must not double as a repair.
        CHECK(count_occurrences(db, dup) == 2);
        CHECK(db.size() == size_before);

        // And the database is still usable: compaction closes the active
        // container before it starts, so the failure path has to reopen one.
        // Probe with a fresh key — the entries compaction had already relocated
        // before it hit the duplicate are no longer in the active map, and it
        // does not roll those moves back.
        auto const fresh = make_key(filler + 1);
        REQUIRE(db.insert(fresh, make_value(33, 3), 400).has_value());
        CHECK(db.find(fresh, 400).has_value());

        db.close();
    }

    std::filesystem::remove_all(path);
}

TEST_CASE("compact-mode compaction reports a duplicate key too",
          "[database][compact][compaction][invariant]") {
    auto const path = make_unique_path("compact");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::compact_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        auto const dup = make_key(0xBEEF);
        REQUIRE(db.insert(dup, /*file_number=*/1, /*offset=*/10, /*height=*/100).value());

        uint64_t filler = 2'000'000;
        std::vector<utxoz::raw_outpoint> v0_keys;
        auto const rotations = [&] { return db.get_statistics().rotations_per_container[0]; };

        while (rotations() == 0) {
            size_t const before = rotations();
            std::vector<utxoz::raw_outpoint> chunk;
            for (size_t i = 0; i < 1'000; ++i) {
                auto const k = make_key(filler++);
                REQUIRE(db.insert(k, 2, 20, 200).has_value());
                chunk.push_back(k);
            }
            if (before == rotations()) {
                v0_keys.insert(v0_keys.end(), chunk.begin(), chunk.end());
            }
        }
        REQUIRE(rotations() >= 1);
        REQUIRE_FALSE(v0_keys.empty());

        REQUIRE(db.insert(dup, /*file_number=*/3, /*offset=*/30, /*height=*/300).value());
        REQUIRE(count_occurrences(db, dup) == 2);

        // Make room in version 0, as in the full-mode case.
        for (auto const& k : v0_keys) {
            (void)db.erase(k, 300);
        }
        {
            auto const [deleted, failed] = db.process_pending_deletions();
            CHECK(failed.empty());
            CHECK(deleted > 0);
        }
        REQUIRE(count_occurrences(db, dup) == 2);

        auto const size_before = db.size();

        auto const compacted = db.compact_all();
        REQUIRE_FALSE(compacted.has_value());
        CHECK(compacted.error() == utxoz::error_code::duplicate_key);

        CHECK(count_occurrences(db, dup) == 2);
        CHECK(db.size() == size_before);

        // Fresh key, for the same reason as in full mode.
        auto const fresh = make_key(filler + 1);
        REQUIRE(db.insert(fresh, 4, 40, 400).has_value());
        CHECK(db.find(fresh, 400).has_value());

        db.close();
    }

    std::filesystem::remove_all(path);
}

TEST_CASE("compaction succeeds when the invariant holds",
          "[database][compaction][invariant]") {
    auto const path = make_unique_path("clean");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        // Several versions, every key distinct.
        uint64_t next = 0;
        std::vector<utxoz::raw_outpoint> keys;
        for (size_t batch = 0; batch < 400; ++batch) {
            for (size_t i = 0; i < 5'000; ++i) {
                auto const k = make_key(next++);
                REQUIRE(db.insert(k, make_value(33, 7), 100).has_value());
                if (keys.size() < 100 && i % 500 == 0) keys.push_back(k);
            }
            if (db.get_statistics().rotations_per_container[0] >= 2) break;
        }
        REQUIRE(db.get_statistics().rotations_per_container[0] >= 2);

        auto const size_before = db.size();

        REQUIRE(db.compact_all().has_value());

        CHECK(db.size() == size_before);
        for (auto const& k : keys) {
            INFO("key must survive compaction");
            CHECK(count_occurrences(db, k) == 1);
        }

        db.close();
    }

    std::filesystem::remove_all(path);
}

/// Files on disk for a container, and the metadata files that describe them.
namespace {

size_t count_files(std::string const& path, std::string const& prefix) {
    size_t n = 0;
    for (auto const& entry : std::filesystem::directory_iterator(path)) {
        if (entry.path().filename().string().rfind(prefix, 0) == 0) ++n;
    }
    return n;
}

} // anonymous namespace

/**
 * Compaction rewrites the file layout as it goes — it empties files, removes
 * them and renumbers what is left — and the metadata describes that layout. The
 * failing path has to leave the two agreeing, exactly like the successful one:
 * a metadata file for a version that no longer exists is a description of a
 * database that is not there.
 *
 * The duplicate is placed deep enough that at least one earlier version is
 * drained, removed and renumbered before the collision is reached.
 */
TEST_CASE("a failed compaction leaves metadata describing the files that remain",
          "[database][compaction][invariant]") {
    auto const path = make_unique_path("meta");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        auto const rotations = [&] { return db.get_statistics().rotations_per_container[0]; };
        uint64_t filler = 3'000'000;

        // Version 0: emptied entirely later, so compaction removes it and
        // renumbers everything above it.
        std::vector<utxoz::raw_outpoint> v0_keys;
        // Version 1: holds the first copy of the duplicate.
        std::vector<utxoz::raw_outpoint> v1_keys;
        auto const dup = make_key(0xFEED);

        auto fill_one_version = [&](std::vector<utxoz::raw_outpoint>& keys) {
            size_t const start = rotations();
            while (rotations() == start) {
                size_t const before = rotations();
                std::vector<utxoz::raw_outpoint> chunk;
                for (size_t i = 0; i < 1'000; ++i) {
                    auto const k = make_key(filler++);
                    REQUIRE(db.insert(k, make_value(33, 1), 200).has_value());
                    chunk.push_back(k);
                }
                if (before == rotations()) keys.insert(keys.end(), chunk.begin(), chunk.end());
            }
        };

        fill_one_version(v0_keys);                                   // v0 done, active is v1
        REQUIRE(db.insert(dup, make_value(33, 0x11), 100).value());  // first copy, in v1
        fill_one_version(v1_keys);                                   // v1 done, active is v2
        REQUIRE(db.insert(dup, make_value(33, 0x22), 300).value());  // second copy, in v2
        REQUIRE(count_occurrences(db, dup) == 2);
        REQUIRE(rotations() >= 2);

        // Empty version 0 so compaction drains it away completely, and free
        // room in version 1 so the walk reaches the duplicate in version 2.
        for (auto const& k : v0_keys) (void)db.erase(k, 300);
        for (auto const& k : v1_keys) (void)db.erase(k, 300);
        {
            auto const [deleted, failed] = db.process_pending_deletions();
            CHECK(failed.empty());
        }
        REQUIRE(count_occurrences(db, dup) == 2);

        auto const files_before = count_files(path, "cont_0_v");

        auto const compacted = db.compact_all();
        REQUIRE_FALSE(compacted.has_value());
        CHECK(compacted.error() == utxoz::error_code::duplicate_key);

        // The run has to have removed and renumbered a version before reaching
        // the duplicate, or this case would be testing nothing.
        INFO("container 0 files: " << files_before << " -> " << count_files(path, "cont_0_v"));
        REQUIRE(count_files(path, "cont_0_v") < files_before);

        // Both copies still there, and the metadata files match the data files
        // that actually remain — no description of a version that was removed.
        CHECK(count_occurrences(db, dup) == 2);
        auto const data_files = count_files(path, "cont_0_v");
        auto const meta_files = count_files(path, "meta_0_v");
        INFO("data files: " << data_files << "  metadata files: " << meta_files);
        CHECK(meta_files == data_files);

        db.close();
    }

    std::filesystem::remove_all(path);
}

/**
 * compact_all() walks the containers in order. Once one of them reports the
 * database inconsistent, the rest must be left alone: continuing would mutate
 * more state after a condition the owner treats as fatal.
 */
TEST_CASE("compact_all stops at the first inconsistent container",
          "[database][compaction][invariant]") {
    auto const path = make_unique_path("stop");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        auto const rot = [&](size_t c) { return db.get_statistics().rotations_per_container[c]; };

        // Container 0: a duplicate, with room made for compaction to find it.
        auto const dup = make_key(0xC0FFEE);
        REQUIRE(db.insert(dup, make_value(33, 0x11), 100).value());

        uint64_t filler = 4'000'000;
        std::vector<utxoz::raw_outpoint> c0_keys;
        while (rot(0) == 0) {
            size_t const before = rot(0);
            std::vector<utxoz::raw_outpoint> chunk;
            for (size_t i = 0; i < 1'000; ++i) {
                auto const k = make_key(filler++);
                REQUIRE(db.insert(k, make_value(33, 1), 200).has_value());
                chunk.push_back(k);
            }
            if (before == rot(0)) c0_keys.insert(c0_keys.end(), chunk.begin(), chunk.end());
        }
        REQUIRE(db.insert(dup, make_value(33, 0x22), 300).value());
        for (auto const& k : c0_keys) (void)db.erase(k, 300);

        // Container 3: several versions with room, so it is compactable — and
        // must be left untouched because container 0 fails first.
        std::vector<utxoz::raw_outpoint> c3_keys;
        while (rot(3) < 2) {
            auto const k = make_key(filler++);
            REQUIRE(db.insert(k, make_value(200, 2), 200).has_value());
            c3_keys.push_back(k);
        }
        for (size_t i = 0; i < c3_keys.size() / 2; ++i) (void)db.erase(c3_keys[i], 300);
        {
            auto const [deleted, failed] = db.process_pending_deletions();
            CHECK(failed.empty());
        }

        auto const c3_files_before = count_files(path, "cont_3_v");
        REQUIRE(c3_files_before >= 3);

        auto const compacted = db.compact_all();
        REQUIRE_FALSE(compacted.has_value());
        CHECK(compacted.error() == utxoz::error_code::duplicate_key);

        // Container 3 was never touched: same number of files as before.
        INFO("container 3 files before: " << c3_files_before
             << "  after: " << count_files(path, "cont_3_v"));
        CHECK(count_files(path, "cont_3_v") == c3_files_before);

        db.close();
    }

    std::filesystem::remove_all(path);
}
