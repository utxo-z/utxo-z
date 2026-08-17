// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_open_existing.cpp
 * @brief Opening a database that is already there, and creating nothing when it
 *        is not.
 *
 * `open()` makes a database where there is none. For a store that is right: a
 * node starting for the first time should not have to create one by hand. For
 * anything that only wants to look — a census, a diagnostic, a script — it is
 * exactly wrong, and the failure is silent: a mistyped path leaves a new empty
 * database behind and is then reported as holding nothing.
 *
 * A caller cannot fix this from outside. Checking first and opening second has a
 * window between the two, and open-or-create fills that window by creating. The
 * property has to be decided where the claim is held, which is inside, and that
 * is what these cases pin — including the window itself, made deterministic
 * rather than raced for.
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <limits>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/census.hpp>
#include <utxoz/database.hpp>
#include <utxoz/types.hpp>

#include "detail/database_lock.hpp"
#include "detail/durability.hpp"

namespace fs = std::filesystem;
using namespace utxoz;
using utxoz::detail::failpoints;

namespace {

struct temp_dir {
    temp_dir() {
        static std::atomic<uint64_t> counter{0};
        auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        dir = fmt::format("./test_open_existing_{}_{}_{}", getpid(), ts, counter.fetch_add(1));
        fs::remove_all(dir);
    }
    ~temp_dir() { std::error_code ec; fs::remove_all(dir, ec); }
    temp_dir(temp_dir const&) = delete;
    temp_dir& operator=(temp_dir const&) = delete;
    fs::path dir;
};

raw_outpoint key_of(uint64_t n) {
    raw_outpoint k{};
    std::memcpy(k.data(), &n, sizeof(n));
    return k;
}

/// Everything in a directory except the claim's own file, which is permanent by
/// design and is not a database.
std::vector<std::string> contents_besides_the_lock(fs::path const& dir) {
    std::vector<std::string> names;
    std::error_code ec;
    if ( ! fs::exists(dir, ec)) return names;
    for (auto const& entry : fs::directory_iterator(dir, ec)) {
        auto const name = entry.path().filename().string();
        if (name == detail::database_lock::file_name) continue;
        names.push_back(name);
    }
    return names;
}

/// Every file in the directory and a digest of its bytes, so that "unchanged" is
/// a statement about content and not only about names. The claim's own file is
/// excluded: it records who holds the directory, and an open that refuses still
/// took it.
std::map<std::string, size_t> contents_of(fs::path const& dir) {
    std::map<std::string, size_t> out;
    for (auto const& entry : fs::directory_iterator(dir)) {
        auto const name = entry.path().filename().string();
        if (name == detail::database_lock::file_name) continue;
        if ( ! fs::is_regular_file(entry)) continue;
        std::ifstream in(entry.path(), std::ios::binary);
        REQUIRE(in.good());
        std::string const bytes((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
        out.emplace(name, std::hash<std::string>{}(bytes));
    }
    return out;
}

void make_a_database(fs::path const& dir, size_t entries = 20) {
    auto opened = full_db::open_for_testing(dir, true);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);
    std::vector<uint8_t> const value(8, 0x21);
    for (uint64_t i = 1; i <= entries; ++i) {
        REQUIRE(db.insert(key_of(i), value, 800000).has_value());
    }
    db.close();
}

} // namespace

TEST_CASE("open_for_inspection refuses an empty directory and creates nothing", "[open]") {
    failpoints::scoped_reset const disarm;
    temp_dir t;
    fs::create_directories(t.dir);

    auto refused = full_db::open_for_inspection_for_testing(t.dir);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() == error_code::database_not_found);

    // No config, no segments, no metadata — nothing that would make this look
    // like a database to the next person or the next program.
    auto const left = contents_besides_the_lock(t.dir);
    for (auto const& name : left) INFO("left behind: " << name);
    CHECK(left.empty());

    // And asking again gives the same answer, which is what says the first
    // attempt did not quietly make one.
    auto again = full_db::open_for_inspection_for_testing(t.dir);
    CHECK_FALSE(again.has_value());
    CHECK(again.error() == error_code::database_not_found);
}

TEST_CASE("open_for_inspection does not create the directory either", "[open]") {
    failpoints::scoped_reset const disarm;
    temp_dir t;                       // deliberately not created
    REQUIRE_FALSE(fs::exists(t.dir));

    auto refused = full_db::open_for_inspection_for_testing(t.dir);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() == error_code::database_not_found);
    CHECK_FALSE(fs::exists(t.dir));
}

TEST_CASE("open_for_inspection opens a database that is there", "[open]") {
    failpoints::scoped_reset const disarm;
    temp_dir t;
    make_a_database(t.dir, 20);

    auto opened = full_db::open_for_inspection_for_testing(t.dir);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);
    CHECK(db.size() == 20);

    // Counting is what this door is for, and it works.
    auto const report = db.census();
    REQUIRE(report.has_value());
    CHECK(report->entries == 20);
    db.close();
}

TEST_CASE("an inspection refuses everything that is not inspecting", "[open]") {
    // Not a restriction for its own sake. A class with no generations has no
    // active container here — that is the point of the whole door — so an insert
    // or a find would be working on a map that is not there. The alternative was
    // to create it, which is what this exists not to do.
    failpoints::scoped_reset const disarm;
    temp_dir t;
    make_a_database(t.dir, 5);

    auto db = std::move(*full_db::open_for_inspection_for_testing(t.dir));

    std::vector<uint8_t> const value(8, 0x77);
    auto const inserted = db.insert(key_of(99), value, 800000);
    REQUIRE_FALSE(inserted.has_value());
    CHECK(inserted.error() == error_code::inspection_only);

    auto const found = db.find(key_of(1), 800001);
    REQUIRE_FALSE(found.has_value());
    CHECK(found.error() == error_code::inspection_only);

    auto const compacted = db.compact_all();
    REQUIRE_FALSE(compacted.has_value());
    CHECK(compacted.error() == error_code::inspection_only);

    auto const synced = db.sync();
    REQUIRE_FALSE(synced.has_value());
    CHECK(synced.error() == error_code::inspection_only);

    // Deletions answer in their own shape: nothing applied, nothing proven
    // absent, everything handed back to be resent, and the reason carried.
    std::vector<deferred_deletion_entry> batch{{key_of(1), 800002}};
    auto const progress = db.apply_deletes(batch);
    CHECK(progress.erased.empty());
    CHECK(progress.absent.empty());
    CHECK(progress.unresolved.size() == 1);
    REQUIRE(progress.error.has_value());
    CHECK(*progress.error == error_code::inspection_only);

    // And the two that are inspecting still work.
    CHECK(db.census().has_value());
    db.close();

    // None of the refusals wrote anything: opening the ordinary way finds the
    // same five entries it had.
    auto reopened = full_db::open_for_testing(t.dir, false);
    REQUIRE(reopened.has_value());
    CHECK(reopened->size() == 5);
    reopened->close();
}

TEST_CASE("open_for_inspection reports what is wrong with a database that is there",
          "[open]") {
    // "There is no database here" and "the database here is unusable" are
    // different answers and send a person to different places. A corrupt config
    // must not come back as absence.
    failpoints::scoped_reset const disarm;
    temp_dir t;
    make_a_database(t.dir, 5);

    auto const config = t.dir / "utxoz_config.dat";
    REQUIRE(fs::exists(config));
    {
        std::ofstream out(config, std::ios::binary | std::ios::trunc);
        REQUIRE(out.good());
        out << "this is not a config";
    }

    auto refused = full_db::open_for_inspection_for_testing(t.dir);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() != error_code::database_not_found);
    CHECK(refused.error() == error_code::config_file_corrupt);
}

TEST_CASE("the existence decision is made under the claim, not before it", "[open]") {
    // The window a caller outside the library cannot close.
    //
    // Armed, the seam removes the config immediately after the claim is taken and
    // before the open decides anything. If the decision were made before the
    // claim — the only shape available to a caller checking for themselves — it
    // would have said "yes, there is a database", and open-or-create would then
    // have created one over that answer.
    //
    // Deterministic on purpose: two threads racing would pass most of the time
    // whatever the ordering.
    failpoints::scoped_reset const disarm;
    temp_dir t;
    make_a_database(t.dir, 5);

    // The control: without the seam, this database opens.
    {
        auto opened = full_db::open_for_inspection_for_testing(t.dir);
        REQUIRE(opened.has_value());
        opened->close();
    }

    failpoints::delete_config_after_claim.store(true, std::memory_order_relaxed);
    auto refused = full_db::open_for_inspection_for_testing(t.dir);
    failpoints::delete_config_after_claim.store(false, std::memory_order_relaxed);

    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() == error_code::database_not_found);

    // And no new config was written in its place: the refusal happened instead of
    // a creation, not after one.
    CHECK_FALSE(fs::exists(t.dir / "utxoz_config.dat"));
}

TEST_CASE("open() still creates, for the callers that depend on it", "[open]") {
    // The contract that was there first. open_for_inspection() is a second question,
    // not a change to this one.
    failpoints::scoped_reset const disarm;
    temp_dir t;
    REQUIRE_FALSE(fs::exists(t.dir));

    auto created = full_db::open_for_testing(t.dir, false);
    REQUIRE(created.has_value());
    CHECK(created->size() == 0);
    created->close();

    CHECK(fs::exists(t.dir / "utxoz_config.dat"));

    // And now open_existing finds what open() made.
    auto opened = full_db::open_for_inspection_for_testing(t.dir);
    CHECK(opened.has_value());
    if (opened) opened->close();
}

TEST_CASE("reference mode has the same two doors", "[open]") {
    failpoints::scoped_reset const disarm;
    temp_dir t;
    fs::create_directories(t.dir);

    auto refused = reference_db::open_for_inspection_for_testing(t.dir);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() == error_code::database_not_found);
    CHECK(contents_besides_the_lock(t.dir).empty());

    {
        auto created = reference_db::open_for_testing(t.dir, true);
        REQUIRE(created.has_value());
        REQUIRE(created->insert(key_of(1), 7, 11, 800000).has_value());
        created->close();
    }

    auto opened = reference_db::open_for_inspection_for_testing(t.dir);
    REQUIRE(opened.has_value());
    CHECK(opened->size() == 1);
    opened->close();
}

TEST_CASE("a config with no generations is not a database to open", "[open]") {
    // The shape a database has for the instant between its config being written
    // and its first segment existing — and what is left when the data is removed
    // and the config is not.
    //
    // open() takes the creation branch here and makes the segments, which is
    // correct for a store. open_for_inspection() must not: creating five files on the
    // way to reporting a database is the side effect it exists to avoid, and the
    // report afterwards would say the database holds nothing, which is true of
    // the files it just made and not of anything that was there.
    failpoints::scoped_reset const disarm;
    temp_dir t;
    make_a_database(t.dir, 5);

    // Everything except the config and the claim.
    size_t removed = 0;
    for (auto const& entry : fs::directory_iterator(t.dir)) {
        auto const name = entry.path().filename().string();
        if (name == "utxoz_config.dat") continue;
        if (name == detail::database_lock::file_name) continue;
        std::error_code ec;
        fs::remove_all(entry.path(), ec);
        ++removed;
    }
    REQUIRE(removed > 0);
    REQUIRE(fs::exists(t.dir / "utxoz_config.dat"));

    auto refused = full_db::open_for_inspection_for_testing(t.dir);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() == error_code::database_not_found);

    // And it made none of them. Named one by one rather than counted, because a
    // count of zero is also what a walk that looked in the wrong place returns.
    for (size_t i = 0; i < container_sizes.size(); ++i) {
        INFO("container " << i);
        CHECK_FALSE(fs::exists(t.dir / fmt::format("cont_{}_v{:05}.dat", i, 0)));
    }
    CHECK(contents_besides_the_lock(t.dir) == std::vector<std::string>{"utxoz_config.dat"});

    // The control: open() does create them, which is why open_for_inspection() had to
    // be a separate door rather than a change to this one.
    {
        auto created = full_db::open_for_testing(t.dir, false);
        REQUIRE(created.has_value());
        created->close();
    }
    CHECK(fs::exists(t.dir / fmt::format("cont_{}_v{:05}.dat", 0, 0)));
}

TEST_CASE("reference: a config with no generations is refused too", "[open]") {
    failpoints::scoped_reset const disarm;
    temp_dir t;
    {
        auto created = reference_db::open_for_testing(t.dir, true);
        REQUIRE(created.has_value());
        REQUIRE(created->insert(key_of(1), 3, 4, 800000).has_value());
        created->close();
    }

    for (auto const& entry : fs::directory_iterator(t.dir)) {
        auto const name = entry.path().filename().string();
        if (name == "utxoz_config.dat") continue;
        if (name == detail::database_lock::file_name) continue;
        std::error_code ec;
        fs::remove_all(entry.path(), ec);
    }

    auto refused = reference_db::open_for_inspection_for_testing(t.dir);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error() == error_code::database_not_found);
    CHECK(contents_besides_the_lock(t.dir) == std::vector<std::string>{"utxoz_config.dat"});
}

TEST_CASE("a class with no generations is inspected as zero, and not created",
          "[open][census]") {
    // Compaction can drain a class completely, so a database with one class full
    // and another with nothing in it is an ordinary state and not damage.
    //
    // `open()` gives the empty class version zero back, which is right for a
    // store about to receive inserts. An inspection must not: the file it would
    // create is ten megabytes, and the census would then report a generation this
    // very call had made. The measurement would be of the instrument.
    failpoints::scoped_reset const disarm;
    temp_dir t;

    {
        auto db = std::move(*full_db::open_for_testing(t.dir, true));
        std::vector<uint8_t> const value(8, 0x11);          // class 0
        for (uint64_t i = 1; i <= 6; ++i) {
            REQUIRE(db.insert(key_of(i), value, 800000).has_value());
        }
        db.close();
    }

    // Leave class 3 with no generations at all, data and metadata alike.
    constexpr size_t emptied = 3;
    std::error_code ec;
    for (auto const& entry : fs::directory_iterator(t.dir)) {
        auto const name = entry.path().filename().string();
        if (name.rfind(fmt::format("cont_{}_v", emptied), 0) == 0
                || name.rfind(fmt::format("meta_{}_v", emptied), 0) == 0) {
            fs::remove(entry.path(), ec);
        }
    }
    REQUIRE_FALSE(fs::exists(t.dir / fmt::format("cont_{}_v{:05}.dat", emptied, 0)));

    // Every file, by name and by content, so that "nothing changed" is not the
    // same statement as "nothing was added".
    auto const before = contents_of(t.dir);
    REQUIRE_FALSE(before.empty());

    {
        auto opened = full_db::open_for_inspection_for_testing(t.dir);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        auto report = db.census();
        REQUIRE(report.has_value());
        REQUIRE(report->classes.size() == container_sizes.size());

        // The emptied class is reported, and reported as nothing.
        auto const& empty_class = report->classes[emptied];
        CHECK(empty_class.container_class == emptied);
        CHECK(empty_class.generations == 0);
        CHECK(empty_class.entries == 0);
        CHECK(empty_class.generations_detail.empty());
        CHECK(empty_class.segment_size_bytes == 0);

        // And the class that has data is unaffected by its neighbour's absence.
        CHECK(report->classes[0].generations == 1);
        CHECK(report->classes[0].entries == 6);
        CHECK(report->entries == 6);
        db.close();
    }

    // Not one byte moved, and no file appeared.
    auto const after = contents_of(t.dir);
    CHECK_FALSE(fs::exists(t.dir / fmt::format("cont_{}_v{:05}.dat", emptied, 0)));
    for (auto const& [name, digest] : after) {
        INFO("after: " << name);
        auto const was = before.find(name);
        REQUIRE(was != before.end());          // nothing new
        CHECK(was->second == digest);          // and nothing rewritten
    }
    CHECK(after.size() == before.size());      // and nothing removed

    // The control, and it is the reason this needed a separate door: the
    // ordinary open does create it.
    {
        auto created = full_db::open_for_testing(t.dir, false);
        REQUIRE(created.has_value());
        created->close();
    }
    CHECK(fs::exists(t.dir / fmt::format("cont_{}_v{:05}.dat", emptied, 0)));
}
