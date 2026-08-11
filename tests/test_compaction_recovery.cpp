// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_compaction_recovery.cpp
 * @brief What a crash at each barrier of a merge leaves, and what reopening
 *        makes of it.
 *
 * A merge builds a new version file from several old ones, publishes it, and
 * then retires them. It can be cut at any point in that sequence, and every
 * point has to leave a state the next open can finish or abandon — never one it
 * has to guess about.
 *
 * The crash is a real one: a forked child calls `_exit`, so nothing unwinds,
 * nothing flushes, and no destructor runs. What that does **not** test is the
 * barriers themselves. `_exit` leaves the page cache intact, so a file whose
 * `fsync` was skipped is still there afterwards. These cases prove the sequence
 * is correct at every point it can be cut; proving the barriers reach the
 * platter needs an apparatus that can cut the power, and nothing here claims to.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <span>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>

#include "detail/durability.hpp"
#include "detail/merge_sidecar.hpp"
#include "detail/segment_open.hpp"
#include "detail/scope_exit.hpp"

namespace fs = std::filesystem;
using utxoz::detail::failpoints;
using crash_point = utxoz::detail::failpoints::crash_point;
using utxoz::detail::scope_exit;

namespace {

inline std::atomic<uint64_t> rc_counter{0};

std::string unique_path(std::string_view tag) {
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fmt::format("./test_rc_{}_{}_{}_{}", tag, getpid(), ts, rc_counter.fetch_add(1));
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
std::vector<utxoz::raw_outpoint> all_keys(Db const& db) {
    std::vector<utxoz::raw_outpoint> keys;
    // Required, not discarded: a traversal that stopped early would otherwise
    // be reported as the database holding a different set of keys, which is a
    // much more confusing thing to read than "the scan failed".
    REQUIRE(db.for_each_key([&](utxoz::raw_outpoint const& k) { keys.push_back(k); }));
    std::ranges::sort(keys);
    return keys;
}

/// A latched instance answers every authoritative operation the same way.
///
/// Unwrapping before reading the error matters: `.error()` on a result that
/// holds a value reads the wrong arm of the union, and it would do so exactly
/// when the test is failing — turning a clear failure into an unclear one.
template <typename R>
void require_latched(R const& r) {
    REQUIRE_FALSE(r);
    CHECK(r.error() == utxoz::error_code::recovery_required);
}

size_t count_matching(std::string const& path, std::string_view fragment) {
    size_t n = 0;
    for (auto const& entry : fs::directory_iterator(path)) {
        if (entry.path().filename().string().find(fragment) != std::string::npos) ++n;
    }
    return n;
}

/// The identity a merge of these files would take next.
///
/// The digits are validated before they are parsed: the directory also holds
/// names in the reserved namespace, and handing one of those to stoull would
/// throw rather than be skipped.
size_t next_identity(std::string const& path, std::string_view prefix) {
    size_t highest = 0;
    bool any = false;
    for (auto const& entry : fs::directory_iterator(path)) {
        auto const name = entry.path().filename().string();
        if (name.rfind(prefix, 0) != 0 || ! name.ends_with(".dat")) continue;

        auto const digits = name.substr(prefix.size(), name.size() - prefix.size() - 4);
        if (digits.empty() || digits.find_first_not_of("0123456789") != std::string::npos) continue;

        highest = std::max(highest, size_t(std::stoull(digits)));
        any = true;
    }
    return any ? highest + 1 : 0;
}

/// Canonical data files of container 0 — the name exactly, not a substring of
/// it, so a `.dat.building` sitting beside them is not miscounted as one.
size_t count_data_files(std::string const& path) {
    size_t n = 0;
    for (auto const& entry : fs::directory_iterator(path)) {
        auto const name = entry.path().filename().string();
        if (name.rfind("cont_0_v", 0) == 0 && name.ends_with(".dat")) ++n;
    }
    return n;
}

/// Files named exactly `<prefix><NNNNN>.dat`, not merely containing the prefix.
size_t count_matching_exact(std::string const& path, std::string const& prefix) {
    size_t n = 0;
    for (auto const& entry : fs::directory_iterator(path)) {
        auto const name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0 && name.ends_with(".dat")) ++n;
    }
    return n;
}

size_t count_meta_files(std::string const& path) {
    size_t n = 0;
    for (auto const& entry : fs::directory_iterator(path)) {
        auto const name = entry.path().filename().string();
        if (name.rfind("meta_0_v", 0) == 0 && name.ends_with(".dat")) ++n;
    }
    return n;
}

/// Every name the store reserves for work in progress. After a successful
/// recovery none of them may be left: a leftover is either work nobody will
/// finish or evidence nobody will act on.
size_t count_reserved(std::string const& path) {
    return count_matching(path, ".building") + count_matching(path, ".merge");
}

/// Builds a database with several versions in container 0 and room to merge
/// them, then closes it. Returns the keys it holds.
std::vector<utxoz::raw_outpoint> build_mergeable(std::string const& path, size_t want_files) {
    auto opened = utxoz::full_db::open_for_testing(path, true);
    REQUIRE(opened);
    auto db = std::move(*opened);

    uint64_t next = 0;
    while (count_data_files(path) < want_files) {
        REQUIRE(db.insert(make_key(next++), make_value(8, 1), 100).value());
        REQUIRE(next < 2'000'000);
    }

    // Free half of it, so the survivors fit into fewer files than they occupy.
    auto const keys = all_keys(db);
    for (size_t i = 0; i < keys.size(); i += 2) {
        (void)db.erase(keys[i], 400);
    }
    (void)db.process_pending_deletions();

    auto const remaining = all_keys(db);
    db.close();
    return remaining;
}

} // anonymous namespace

#ifndef _WIN32

/**
 * Runs a compaction in a forked child that dies at `point`, then returns to the
 * parent with the database exactly as the crash left it.
 */
namespace {

template <typename Db>
void die_as(std::string const& path, crash_point point) {
    pid_t const child = ::fork();
    REQUIRE(child >= 0);

    if (child == 0) {
        // No Catch2 assertions past this line: this process is going to die on
        // purpose, and a failed REQUIRE here would be reported as the child's.
        auto opened = Db::open_for_testing(path);
        if ( ! opened) ::_exit(2);
        auto db = std::move(*opened);

        failpoints::crash_at.store(point, std::memory_order_relaxed);
        (void)db.compact_all();

        // Reached only if the point was never hit, which is a broken test.
        ::_exit(3);
    }

    int status = 0;
    REQUIRE(::waitpid(child, &status, 0) == child);
    INFO("child exit status " << status);
    REQUIRE(WIFEXITED(status));
    // 99 is the deliberate death; 3 means compaction finished without reaching
    // the point, which would make the case vacuous.
    REQUIRE(WEXITSTATUS(status) == 99);
}

void die_in_full_mode(std::string const& path, crash_point point) {
    die_as<utxoz::full_db>(path, point);
}

/// A reference-mode database with three versions and room to merge them.
std::vector<utxoz::raw_outpoint> build_mergeable_reference(std::string const& path,
                                                         size_t want_files) {
    auto opened = utxoz::reference_db::open_for_testing(path, true);
    REQUIRE(opened);
    auto db = std::move(*opened);

    uint64_t next = 0;
    while (count_matching_exact(path, "compact_v") < want_files) {
        REQUIRE(db.insert(make_key(next), next % 1000, uint32_t(next), 100).value());
        ++next;
        REQUIRE(next < 3'000'000);
    }

    std::vector<utxoz::raw_outpoint> keys;
    REQUIRE(db.for_each_key([&](utxoz::raw_outpoint const& k) { keys.push_back(k); }));
    for (size_t i = 0; i < keys.size(); i += 2) {
        REQUIRE(db.erase(keys[i], 400));
    }
    (void)db.process_pending_deletions();

    std::vector<utxoz::raw_outpoint> remaining;
    REQUIRE(db.for_each_key([&](utxoz::raw_outpoint const& k) { remaining.push_back(k); }));
    std::ranges::sort(remaining);
    db.close();
    return remaining;
}

/// Everything that has to be true of a database after recovery, whatever it
/// recovered from.
void require_sound_after_recovery(std::string const& path,
                                  std::vector<utxoz::raw_outpoint> const& expected) {
    auto opened = utxoz::full_db::open_for_testing(path);
    REQUIRE(opened);
    auto db = std::move(*opened);

    // Every key exactly once. Duplicates are what the whole protocol exists to
    // prevent, so a merged and an unmerged outcome are both acceptable — a
    // half-applied one is not.
    auto const keys = all_keys(db);
    REQUIRE(std::adjacent_find(keys.begin(), keys.end()) == keys.end());
    REQUIRE(keys == expected);

    // Nothing left in the reserved namespace.
    INFO("reserved names left: " << count_reserved(path));
    REQUIRE(count_reserved(path) == 0);

    // The database serves: reads, writes, deletes and another compaction.
    auto const fresh = make_key(9'000'000);
    auto const inserted = db.insert(fresh, make_value(8, 9), 500);
    REQUIRE(inserted);
    REQUIRE(*inserted);
    REQUIRE(db.find(fresh, 500));
    REQUIRE(db.erase(fresh, 600).value() == 1);
    (void)db.process_pending_deletions();
    REQUIRE(db.compact_all());

    db.close();

    // A second recovery finds nothing to do and changes nothing.
    auto const before = count_data_files(path);
    auto again = utxoz::full_db::open_for_testing(path);
    REQUIRE(again);
    auto db2 = std::move(*again);
    REQUIRE(count_data_files(path) == before);
    REQUIRE(count_reserved(path) == 0);
    db2.close();
}

} // anonymous namespace

TEST_CASE("a crash at any barrier of a merge leaves a state reopening can settle",
          "[database][compaction][recovery][crash]") {
    struct step {
        crash_point point;
        char const* name;
        bool target_published;   ///< whether the merge should survive the crash
    };

    // One per barrier, in protocol order. `target_published` is the row of the
    // state table each one lands in: before the target has its canonical name
    // the merge is abandoned, after it the merge is finished.
    std::vector<step> const steps{
        {crash_point::after_build,           "after_build",           false},
        {crash_point::after_file_sync,       "after_file_sync",       false},
        {crash_point::after_sidecar_sync,    "after_sidecar_sync",    false},
        {crash_point::after_sidecar_publish, "after_sidecar_publish", false},
        {crash_point::after_target_publish,  "after_target_publish",  true},
        {crash_point::before_source_unlink,  "before_source_unlink",  true},
        {crash_point::mid_source_unlink,     "mid_source_unlink",     true},
        {crash_point::after_sources_retired, "after_sources_retired", true},
    };

    for (auto const& s : steps) {
        auto const path = unique_path(s.name);
        fs::remove_all(path);
        INFO("crash point: " << s.name);

        // Per iteration: a failed REQUIRE throws out of the loop, and the
        // remove_all at the end of the body would never run.
        scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

        auto const expected = build_mergeable(path, 3);
        REQUIRE_FALSE(expected.empty());
        auto const files_before = count_data_files(path);
        REQUIRE(files_before == 3);

        die_in_full_mode(path, s.point);

        // What the crash left, before anything touches it.
        auto const sidecars = count_matching(path, ".merge");
        auto const files_after_crash = count_data_files(path);
        INFO("after the crash: " << files_after_crash << " data files, " << sidecars << " sidecars");

        if (s.target_published) {
            // The target has its canonical name, so it is complete, and the
            // sources it supersedes are still around in whole or in part.
            REQUIRE(files_after_crash > 0);
        } else {
            // Nothing was published: every source is exactly where it was.
            REQUIRE(files_after_crash == files_before);
        }

        // Recovery, and then the row of the table this crash belongs to. The
        // question is settled by one fact — did the target get its canonical
        // name — so the outcome is not "whatever happened" but one of two.
        {
            auto opened = utxoz::full_db::open_for_testing(path);
            REQUIRE(opened);
            auto db = std::move(*opened);

            auto const files_after_recovery = count_data_files(path);
            INFO("after recovery: " << files_after_recovery << " data files");

            if (s.target_published) {
                // Finished: the sources it superseded are gone, so there are
                // fewer files than before the merge started.
                REQUIRE(files_after_recovery < files_before);
            } else {
                // Abandoned: every source is still there and nothing was added.
                REQUIRE(files_after_recovery == files_before);
            }

            // Recovery never rebuilds a summary, so the target it just settled
            // has none — and the database works anyway, because absent metadata
            // means unknown and every consumer already handles that.
            REQUIRE(count_meta_files(path) <= files_after_recovery);

            auto const keys = all_keys(db);
            REQUIRE(keys == expected);
            db.close();
        }

        require_sound_after_recovery(path, expected);
        fs::remove_all(path);
    }
}

TEST_CASE("a partial retirement latches the instance until it is reopened",
          "[database][compaction][recovery][failpoint]") {
    auto const path = unique_path("partial");
    fs::remove_all(path);

    auto const expected = build_mergeable(path, 3);
    REQUIRE_FALSE(expected.empty());

    {
        auto opened = utxoz::full_db::open_for_testing(path);
        REQUIRE(opened);
        auto db = std::move(*opened);

        // The merge publishes its target and then cannot retire what it
        // superseded. Several canonical files now hold the same keys.
        //
        // Disarmed from a guard: a failed assertion below throws, and a
        // failpoint left armed would follow the process into every later case.
        scope_exit const disarm([] { failpoints::clear(); });
        failpoints::fail_source_unlink.store(true, std::memory_order_relaxed);
        auto const outcome = db.compact_all();
        failpoints::clear();

        REQUIRE_FALSE(outcome);
        REQUIRE(outcome.error() == utxoz::error_code::recovery_required);

        // The instance serves nothing further. Exclusion is what kept the
        // duplicated state unobservable, and exclusion ends when the call
        // returns — so the answer to everything is the same until it is
        // reopened.
        auto const key = make_key(9'100'000);
        require_latched(db.insert(key, make_value(8, 1), 700));
        require_latched(db.find(expected.front(), 700));
        require_latched(db.erase(expected.front(), 700));
        require_latched(db.process_pending_deletions());
        require_latched(db.resolve(std::vector<utxoz::lookup_request>{{expected.front(), 700}}));
        require_latched(db.compact_all());
        require_latched(db.for_each_key([](utxoz::raw_outpoint const&) {}));

        // The evidence is still on disk, which is what makes the next open able
        // to finish the job.
        CHECK(count_matching(path, ".merge") == 1);

        db.close();   // stays available: it is how the owner gets out of this
    }

    // Reopening runs recovery, and only then does the database serve again.
    require_sound_after_recovery(path, expected);
    fs::remove_all(path);
}

#endif // _WIN32

TEST_CASE("a merge refuses to publish over a file that is already there",
          "[database][compaction][recovery]") {
    auto const path = unique_path("collision");
    fs::remove_all(path);

    auto const expected = build_mergeable(path, 3);
    REQUIRE_FALSE(expected.empty());

    auto const taken = next_identity(path, "cont_0_v");

    auto opened = utxoz::full_db::open_for_testing(path);
    REQUIRE(opened);
    auto db = std::move(*opened);

    // The identity the next merge will take, occupied after the catalogue was
    // built so the instance does not know about it. Only one instance touches a
    // database at a time, which is why checking the name is free is meaningful —
    // this is what happens when that precondition does not hold.
    auto const squatter = fmt::format("{}/cont_0_v{:05}.dat", path, taken);
    {
        std::ofstream ofs(squatter, std::ios::binary);
        ofs << "not a version file";
    }

    auto const outcome = db.compact_all();
    REQUIRE_FALSE(outcome);
    CHECK(outcome.error() == utxoz::error_code::identity_collision);

    // The file that was in the way is untouched: a collision is inconsistency,
    // and the evidence of it survives rather than being published over.
    REQUIRE(fs::exists(squatter));
    REQUIRE(fs::file_size(squatter) == std::string("not a version file").size());

    db.close();
    fs::remove_all(path);
}

#ifndef _WIN32

/**
 * A scan that cannot read one of the files it must read has not scanned the
 * database. Reporting success for a partial sweep is the same class of mistake
 * as reading an unreadable directory as an empty one: the caller is told about
 * a database that is not the one on disk.
 */
TEST_CASE("a traversal that cannot read a version reports failure, not a short answer",
          "[database][traversal]") {
    auto const path = unique_path("unreadable");
    fs::remove_all(path);

    auto const expected = build_mergeable(path, 3);
    REQUIRE_FALSE(expected.empty());

    // Take the read permission off one version that is not the active one, so
    // the traversal reaches it and cannot open it.
    std::vector<std::string> older;
    for (auto const& entry : fs::directory_iterator(path)) {
        auto const name = entry.path().filename().string();
        if (name.rfind("cont_0_v", 0) == 0 && name.ends_with(".dat")) older.push_back(name);
    }
    std::ranges::sort(older);
    REQUIRE(older.size() >= 3);

    auto const victim = fs::path(path) / older.front();

    scope_exit const restore([&] {
        std::error_code ec;
        fs::permissions(victim, fs::perms::owner_all, ec);
        fs::remove_all(path, ec);
    });

    auto opened = utxoz::full_db::open_for_testing(path);
    REQUIRE(opened);
    auto db = std::move(*opened);

    fs::permissions(victim, fs::perms::none);

    // A process that ignores permissions — root, or a filesystem that does not
    // enforce them — would sail past every assertion below and report a pass on
    // a case that tested nothing. Say so instead.
    {
        std::ifstream probe(victim, std::ios::binary);
        if (probe.is_open()) {
            probe.close();
            db.close();
            SKIP("file permissions do not restrict this process");
        }
    }

    {
        size_t seen = 0;
        auto const scanned = db.for_each_key([&](utxoz::raw_outpoint const&) { ++seen; });
        REQUIRE_FALSE(scanned);
        CHECK(scanned.error() == utxoz::error_code::file_open_failed);

        // It stopped rather than carrying on: a short answer is exactly what
        // must not be presented as a complete one.
        INFO("keys seen before it stopped: " << seen << " of " << expected.size());
        CHECK(seen < expected.size());

        // The other traversal reaches the same file through a different public
        // wrapper, and has to answer the same way.
        auto const entries = db.for_each_entry(
            [](utxoz::raw_outpoint const&, uint32_t, std::span<uint8_t const>) {});
        REQUIRE_FALSE(entries);
        CHECK(entries.error() == utxoz::error_code::file_open_failed);
    }

    db.close();
}

TEST_CASE("a reference-mode traversal that cannot read a version reports failure",
          "[database][traversal][reference]") {
    auto const path = unique_path("unreadable_reference");
    fs::remove_all(path);

    {
        auto opened = utxoz::reference_db::open_for_testing(path, true);
        REQUIRE(opened);
        auto db = std::move(*opened);

        uint64_t next = 0;
        while (count_matching_exact(path, "compact_v") < 2) {
            REQUIRE(db.insert(make_key(next), next % 1000, uint32_t(next), 100).value());
            ++next;
            REQUIRE(next < 3'000'000);
        }
        db.close();
    }

    std::vector<std::string> files;
    for (auto const& entry : fs::directory_iterator(path)) {
        auto const name = entry.path().filename().string();
        if (name.rfind("compact_v", 0) == 0 && name.ends_with(".dat")) files.push_back(name);
    }
    std::ranges::sort(files);
    REQUIRE(files.size() >= 2);

    auto const victim = fs::path(path) / files.front();

    scope_exit const restore([&] {
        std::error_code ec;
        fs::permissions(victim, fs::perms::owner_all, ec);
        fs::remove_all(path, ec);
    });

    auto opened = utxoz::reference_db::open_for_testing(path);
    REQUIRE(opened);
    auto db = std::move(*opened);

    fs::permissions(victim, fs::perms::none);

    {
        std::ifstream probe(victim, std::ios::binary);
        if (probe.is_open()) {
            probe.close();
            db.close();
            SKIP("file permissions do not restrict this process");
        }
    }

    {
        auto const scanned = db.for_each_key([](utxoz::raw_outpoint const&) {});
        REQUIRE_FALSE(scanned);
        CHECK(scanned.error() == utxoz::error_code::file_open_failed);

        // The typed entry traversal is a third public wrapper over a third impl
        // function, and swallowed errors independently of the other two.
        auto const entries = db.for_each_entry(
            [](utxoz::raw_outpoint const&, uint32_t, uint32_t, uint32_t) {});
        REQUIRE_FALSE(entries);
        CHECK(entries.error() == utxoz::error_code::file_open_failed);
    }

    db.close();
}

/**
 * The sidecar reached its name and the barrier that was to make that durable
 * did not. Whether a crash would find it is unknowable, so the instance stops:
 * carrying on would let a second merge publish another sidecar for the same
 * container, and recovery refuses to choose between two.
 */
TEST_CASE("a barrier failing after the merge record is named latches the instance",
          "[database][compaction][recovery][failpoint]") {
    auto const path = unique_path("sidecarbarrier");
    fs::remove_all(path);

    auto const expected = build_mergeable(path, 3);
    REQUIRE_FALSE(expected.empty());

    {
        auto opened = utxoz::full_db::open_for_testing(path);
        REQUIRE(opened);
        auto db = std::move(*opened);

        // The first directory barrier a merge reaches is the one that publishes
        // the sidecar, so this fails exactly there.
        scope_exit const disarm([] { failpoints::clear(); });
        failpoints::fail_sync_directory.store(true, std::memory_order_relaxed);
        auto const outcome = db.compact_all();
        failpoints::clear();

        require_latched(outcome);

        // Not "it failed to publish": it may or may not have. The instance
        // serves nothing further either way.
        require_latched(db.insert(make_key(9'200'000), make_value(8, 1), 700));
        require_latched(db.compact_all());

        db.close();
    }

    // Reopening settles whichever state is really there. The target was never
    // published, so any sidecar found abandons its merge and every source stays.
    require_sound_after_recovery(path, expected);
    fs::remove_all(path);
}

#endif // _WIN32

#ifndef _WIN32

/**
 * The same latch, on the reference API. Its insert() and find() are separate
 * entry points from the full ones and were missing the check — a latched
 * instance that still answers a find is exactly the state the latch exists to
 * prevent, whichever mode it is in.
 */
TEST_CASE("a latched reference instance refuses its own operations too",
          "[database][compaction][recovery][failpoint][reference]") {
    auto const path = unique_path("referencelatch");
    fs::remove_all(path);

    utxoz::raw_outpoint const witness = make_key(1);

    {
        auto opened = utxoz::reference_db::open_for_testing(path, true);
        REQUIRE(opened);
        auto db = std::move(*opened);

        uint64_t next = 0;
        while (count_matching_exact(path, "compact_v") < 3) {
            REQUIRE(db.insert(make_key(next), next % 1000, uint32_t(next), 100).value());
            ++next;
            REQUIRE(next < 3'000'000);
        }

        // Room to merge into.
        for (uint64_t i = 0; i < next; i += 2) {
            REQUIRE(db.erase(make_key(i), 400));
        }
        (void)db.process_pending_deletions();
        db.close();
    }

    auto opened = utxoz::reference_db::open_for_testing(path);
    REQUIRE(opened);
    auto db = std::move(*opened);

    scope_exit const disarm([] { failpoints::clear(); });
    failpoints::fail_source_unlink.store(true, std::memory_order_relaxed);
    auto const outcome = db.compact_all();
    failpoints::clear();

    REQUIRE_FALSE(outcome);
    REQUIRE(outcome.error() == utxoz::error_code::recovery_required);

    require_latched(db.insert(make_key(9'300'000), 1, 2, 700));
    require_latched(db.find(witness, 700));
    require_latched(db.erase(witness, 700));
    require_latched(db.resolve(std::vector<utxoz::lookup_request>{{witness, 700}}));
    require_latched(db.process_pending_deletions());
    require_latched(db.compact_all());
    require_latched(db.for_each_key([](utxoz::raw_outpoint const&) {}));

    db.close();

    // Reopening runs recovery and the database serves again.
    auto again = utxoz::reference_db::open_for_testing(path);
    REQUIRE(again);
    auto db2 = std::move(*again);
    REQUIRE(count_reserved(path) == 0);
    REQUIRE(db2.insert(make_key(9'300'001), 1, 2, 800).value());
    db2.close();

    fs::remove_all(path);
}

#endif // _WIN32

#ifndef _WIN32

/**
 * The contract of an uncertain publication, stated as three separate facts
 * because each of them is a way the design could go wrong on its own.
 */
TEST_CASE("an uncertain merge record stops before the target and does not undo itself",
          "[database][compaction][recovery][failpoint]") {
    for (bool reference_mode : {false, true}) {
        auto const path = unique_path(reference_mode ? "uncertain_reference" : "uncertain_full");
        fs::remove_all(path);
        INFO(std::string(reference_mode ? "reference mode" : "full mode"));

        std::string const prefix = reference_mode ? "compact_v" : "cont_0_v";

        if (reference_mode) {
            auto opened = utxoz::reference_db::open_for_testing(path, true);
            REQUIRE(opened);
            auto db = std::move(*opened);
            uint64_t next = 0;
            while (count_matching_exact(path, "compact_v") < 3) {
                REQUIRE(db.insert(make_key(next), next % 1000, uint32_t(next), 100).value());
                ++next;
                REQUIRE(next < 3'000'000);
            }
            for (uint64_t i = 0; i < next; i += 2) REQUIRE(db.erase(make_key(i), 400));
            (void)db.process_pending_deletions();
            db.close();
        } else {
            REQUIRE_FALSE(build_mergeable(path, 3).empty());
        }

        auto const files_before = count_matching_exact(path, prefix);
        auto const target = next_identity(path, prefix);
        auto const target_path = fmt::format("{}/{}{:05}.dat", path, prefix, target);
        REQUIRE_FALSE(fs::exists(target_path));

        auto run = [&]() -> utxoz::result<> {
            scope_exit const disarm([] { failpoints::clear(); });
            failpoints::fail_sync_directory.store(true, std::memory_order_relaxed);
            utxoz::result<> outcome;
            if (reference_mode) {
                auto opened = utxoz::reference_db::open_for_testing(path);
                REQUIRE(opened);
                auto db = std::move(*opened);
                outcome = db.compact_all();
                // Latched immediately: nothing else is served.
                require_latched(db.compact_all());
                db.close();
            } else {
                auto opened = utxoz::full_db::open_for_testing(path);
                REQUIRE(opened);
                auto db = std::move(*opened);
                outcome = db.compact_all();
                require_latched(db.compact_all());
                db.close();
            }
            failpoints::clear();
            return outcome;
        };

        auto const outcome = run();
        REQUIRE_FALSE(outcome);
        require_latched(outcome);

        // 1. The target was never published. The merge stopped at the record,
        //    which is the whole point of publishing the record first.
        CHECK_FALSE(fs::exists(target_path));
        CHECK(count_matching_exact(path, prefix) == files_before);

        // 2. Whatever the tidy-up managed, reopening settles it: sources intact,
        //    nothing reserved left behind, every key still reachable.
        auto reopened_files = size_t{0};
        if (reference_mode) {
            auto again = utxoz::reference_db::open_for_testing(path);
            REQUIRE(again);
            auto db = std::move(*again);
            reopened_files = count_matching_exact(path, prefix);
            db.close();
        } else {
            auto again = utxoz::full_db::open_for_testing(path);
            REQUIRE(again);
            auto db = std::move(*again);
            reopened_files = count_matching_exact(path, prefix);
            db.close();
        }
        CHECK(reopened_files == files_before);
        CHECK(count_reserved(path) == 0);

        fs::remove_all(path);
    }
}

/**
 * The two states an uncertain publication can leave, fabricated directly so
 * both are covered rather than whichever one the failed barrier happened to
 * produce.
 */
TEST_CASE("reopening settles both outcomes of an uncertain merge record",
          "[database][compaction][recovery]") {
    SECTION("the record survived: the merge is abandoned and the sources stay") {
        auto const path = unique_path("survived");
        fs::remove_all(path);

        auto const expected = build_mergeable(path, 3);
        REQUIRE_FALSE(expected.empty());
        auto const files_before = count_data_files(path);

        auto const target = next_identity(path, "cont_0_v");

        // A sidecar naming a target that was never published, plus the build it
        // was for — exactly what a crash after the record was named leaves.
        utxoz::detail::merge_plan plan;
        plan.container = 0;
        plan.target = target;
        plan.id = utxoz::detail::generate_merge_id().value();
        plan.sources = {0, 1};
        REQUIRE(utxoz::detail::write_merge_sidecar(
            fmt::format("{}/cont_0_v{:05}.merge", path, target), plan));

        {
            std::ofstream ofs(fmt::format("{}/cont_0_v{:05}.dat.building", path, target),
                              std::ios::binary);
            ofs << "half a merge";
        }
        REQUIRE(count_reserved(path) == 2);

        auto opened = utxoz::full_db::open_for_testing(path);
        REQUIRE(opened);
        auto db = std::move(*opened);

        // Abandoned: nothing was published, so nothing is retired.
        CHECK(count_data_files(path) == files_before);
        CHECK(count_reserved(path) == 0);
        CHECK(all_keys(db) == expected);

        db.close();
        fs::remove_all(path);
    }

    SECTION("the record did not survive: the build is discarded as an orphan") {
        auto const path = unique_path("vanished");
        fs::remove_all(path);

        auto const expected = build_mergeable(path, 3);
        REQUIRE_FALSE(expected.empty());
        auto const files_before = count_data_files(path);

        auto const target = next_identity(path, "cont_0_v");
        {
            std::ofstream ofs(fmt::format("{}/cont_0_v{:05}.dat.building", path, target),
                              std::ios::binary);
            ofs << "half a merge";
        }
        REQUIRE(count_reserved(path) == 1);

        auto opened = utxoz::full_db::open_for_testing(path);
        REQUIRE(opened);
        auto db = std::move(*opened);

        // A build nothing points at is work nobody will finish.
        CHECK(count_data_files(path) == files_before);
        CHECK(count_reserved(path) == 0);
        CHECK(all_keys(db) == expected);

        db.close();
        fs::remove_all(path);
    }
}

#endif // _WIN32

#ifndef _WIN32

// =============================================================================
// The merge identifier
// =============================================================================
//
// A sidecar names its target by identity, and an identity is only a number.
// Nothing locks the database, so a file at that number is not necessarily the
// file the sidecar meant. Retiring the sources on that assumption deletes the
// only copies of their entries. The identifier is what turns "a file is there"
// into "this is the file", and every way of failing to establish that must stop
// the recovery with everything intact.

namespace {

/// Path of the squatter a test wants created between the record and the target.
inline std::string squatter_path;
inline std::string squatter_body = "not the file this merge built";

void create_squatter() {
    std::ofstream ofs(squatter_path, std::ios::binary);
    ofs << squatter_body;
}

/// Leaves a real database mid-merge: the record is durable, the target has its
/// canonical name and its marker, and the sources are still there.
void leave_published_merge(std::string const& path) {
    die_in_full_mode(path, crash_point::before_source_unlink);
}

std::string sole_sidecar(std::string const& path) {
    for (auto const& entry : fs::directory_iterator(path)) {
        auto const name = entry.path().filename().string();
        if (name.ends_with(".merge")) return entry.path().string();
    }
    return {};
}

std::string target_of(std::string const& sidecar) {
    auto const plan = utxoz::detail::read_merge_sidecar(sidecar);
    REQUIRE(plan);
    return fmt::format("{}/cont_{}_v{:05}.dat",
                       fs::path(sidecar).parent_path().string(), plan->container, plan->target);
}

} // anonymous namespace

TEST_CASE("recovery finishes a merge only when the target carries the right marker",
          "[database][compaction][recovery][marker]") {
    SECTION("matching marker: the merge is finished") {
        auto const path = unique_path("marker_match");
        fs::remove_all(path);

        auto const expected = build_mergeable(path, 3);
        auto const files_before = count_data_files(path);
        leave_published_merge(path);
        REQUIRE_FALSE(sole_sidecar(path).empty());

        {
            auto opened = utxoz::full_db::open_for_testing(path);
            REQUIRE(opened);
            auto db = std::move(*opened);

            CHECK(count_data_files(path) < files_before);   // sources retired
            CHECK(count_reserved(path) == 0);
            CHECK(all_keys(db) == expected);
            db.close();
        }

        // Idempotent: a second open finds nothing to do.
        auto const settled = count_data_files(path);
        auto again = utxoz::full_db::open_for_testing(path);
        REQUIRE(again);
        auto db2 = std::move(*again);
        CHECK(count_data_files(path) == settled);
        CHECK(count_reserved(path) == 0);
        db2.close();

        fs::remove_all(path);
    }

    SECTION("no marker: fatal, and every source stays") {
        auto const path = unique_path("marker_absent");
        fs::remove_all(path);

        auto const expected = build_mergeable(path, 3);
        auto const files_before = count_data_files(path);
        leave_published_merge(path);

        auto const sidecar = sole_sidecar(path);
        REQUIRE_FALSE(sidecar.empty());
        auto const target = target_of(sidecar);

        // Replace the published target with a file carrying no marker at all —
        // which is what another process taking the identity would leave.
        fs::remove(target);
        {
            std::ofstream ofs(target, std::ios::binary);
            ofs << "no marker here";
        }

        auto const opened = utxoz::full_db::open_for_testing(path);
        REQUIRE_FALSE(opened);
        CHECK(opened.error() == utxoz::error_code::recovery_failed);

        // Nothing was retired, and the evidence is all still there.
        CHECK(count_data_files(path) == files_before + 1);   // sources plus the impostor
        CHECK(fs::exists(sidecar));
        CHECK(fs::exists(target));

        fs::remove_all(path);
    }

    SECTION("a different marker: fatal, sources and target intact") {
        auto const path = unique_path("marker_other");
        fs::remove_all(path);

        REQUIRE_FALSE(build_mergeable(path, 3).empty());
        auto const files_before = count_data_files(path);
        leave_published_merge(path);

        auto const sidecar = sole_sidecar(path);
        REQUIRE_FALSE(sidecar.empty());
        auto const plan = utxoz::detail::read_merge_sidecar(sidecar);
        REQUIRE(plan);

        // Rewrite the record with a different identifier: same target, same
        // sources, a merge this file did not come from.
        auto other = *plan;
        other.id = utxoz::detail::generate_merge_id().value();
        REQUIRE(other.id != plan->id);
        REQUIRE(utxoz::detail::write_merge_sidecar(sidecar, other));

        auto const target = target_of(sidecar);
        auto const target_size = fs::file_size(target);

        auto const opened = utxoz::full_db::open_for_testing(path);
        REQUIRE_FALSE(opened);
        CHECK(opened.error() == utxoz::error_code::recovery_failed);

        CHECK(count_data_files(path) == files_before + 1);
        CHECK(fs::exists(sidecar));
        CHECK(fs::file_size(target) == target_size);

        fs::remove_all(path);
    }

    SECTION("an unreadable target: fatal, all evidence intact") {
        auto const path = unique_path("marker_unreadable");
        fs::remove_all(path);

        REQUIRE_FALSE(build_mergeable(path, 3).empty());
        auto const files_before = count_data_files(path);
        leave_published_merge(path);

        auto const sidecar = sole_sidecar(path);
        REQUIRE_FALSE(sidecar.empty());
        auto const target = target_of(sidecar);

        scope_exit const restore([&] {
            std::error_code ec;
            fs::permissions(target, fs::perms::owner_all, ec);
            fs::remove_all(path, ec);
        });

        fs::permissions(target, fs::perms::none);

        {
            std::ifstream probe(target, std::ios::binary);
            if (probe.is_open()) {
                probe.close();
                SKIP("file permissions do not restrict this process");
            }
        }

        {
            auto const opened = utxoz::full_db::open_for_testing(path);
            REQUIRE_FALSE(opened);
            // Told nothing is not the same as told it does not match, and
            // neither of them retires a source.
            CHECK(opened.error() == utxoz::error_code::recovery_failed);

            CHECK(count_data_files(path) == files_before + 1);
            CHECK(fs::exists(sidecar));
            CHECK(fs::exists(target));
        }
    }
}

TEST_CASE("an identity taken between the record and the publication is refused",
          "[database][compaction][recovery][marker]") {
    auto const path = unique_path("late_collision");
    fs::remove_all(path);

    auto const expected = build_mergeable(path, 3);
    REQUIRE_FALSE(expected.empty());
    auto const files_before = count_data_files(path);

    auto const target = next_identity(path, "cont_0_v");
    squatter_path = fmt::format("{}/cont_0_v{:05}.dat", path, target);

    scope_exit const disarm([] {
        failpoints::clear();
        squatter_path.clear();
    });

    // The window the precheck cannot cover: the name was free when it was
    // checked and is taken by the time it is used.
    failpoints::before_target_publish.store(&create_squatter, std::memory_order_relaxed);

    auto opened = utxoz::full_db::open_for_testing(path);
    REQUIRE(opened);
    auto db = std::move(*opened);

    auto const outcome = db.compact_all();
    REQUIRE_FALSE(outcome);
    // Refused, not published over.
    CHECK(outcome.error() == utxoz::error_code::identity_collision);

    failpoints::clear();

    // The file that took the name is exactly as it was, and no source went.
    REQUIRE(fs::exists(squatter_path));
    CHECK(fs::file_size(squatter_path) == squatter_body.size());
    CHECK(count_data_files(path) == files_before + 1);

    db.close();
    fs::remove_all(path);
}

#endif // _WIN32

#ifndef _WIN32

// =============================================================================
// Withdrawing a record, and the barrier that confirms the target's name
// =============================================================================

TEST_CASE("a publication that fails withdraws its record, or stops",
          "[database][compaction][recovery][failpoint]") {
    // The record is durable by the time the target is published, so a failure
    // there is not simply reported: the record has to be taken back, and taking
    // it back is only real once the directory says so.
    struct variant {
        char const* name;
        bool break_removal;
        bool break_barrier;
        utxoz::error_code expected;
        bool latches;
    };

    std::vector<variant> const variants{
        {"withdrawn cleanly", false, false, utxoz::error_code::identity_collision, false},
        {"the removal fails", true, false, utxoz::error_code::recovery_required, true},
        {"the barrier fails", false, true, utxoz::error_code::recovery_required, true},
    };

    for (auto const& v : variants) {
        auto const path = unique_path("withdraw");
        fs::remove_all(path);
        INFO(std::string(v.name));

        scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

        auto const expected = build_mergeable(path, 3);
        REQUIRE_FALSE(expected.empty());
        auto const files_before = count_data_files(path);

        auto const target = next_identity(path, "cont_0_v");
        squatter_path = fmt::format("{}/cont_0_v{:05}.dat", path, target);

        scope_exit const disarm([] {
            failpoints::clear();
            squatter_path.clear();
        });

        // Something takes the identity after the record is durable, so the
        // publication fails and the withdrawal path runs.
        failpoints::before_target_publish.store(&create_squatter, std::memory_order_relaxed);
        if (v.break_removal) {
            failpoints::fail_sidecar_removal.store(true, std::memory_order_relaxed);
        }
        if (v.break_barrier) {
            failpoints::fail_directory_barrier_at.store(
                failpoints::dir_barrier::after_sidecar, std::memory_order_relaxed);
        }

        {
            auto opened = utxoz::full_db::open_for_testing(path);
            REQUIRE(opened);
            auto db = std::move(*opened);

            auto const outcome = db.compact_all();
            REQUIRE_FALSE(outcome);
            CHECK(outcome.error() == v.expected);

            failpoints::clear();

            if (v.latches) {
                // The withdrawal is unconfirmed, so the record may come back.
                // The instance stops rather than start another merge that would
                // leave a second record for the same container.
                require_latched(db.compact_all());

                // The two unconfirmed cases differ in what is on disk right
                // now, and in neither is that the point: when the removal
                // failed the record is plainly still there, and when only its
                // barrier failed the record is gone but may return. The latch
                // is what covers both, which is why it does not depend on
                // which one happened.
                CHECK(count_matching(path, ".merge") == (v.break_removal ? 1u : 0u));
            } else {
                // Withdrawn durably: nothing left over, and the instance is
                // still usable.
                CHECK(count_matching(path, ".merge") == 0);
                CHECK(count_reserved(path) == 0);
            }

            // No source was retired in any of the three.
            CHECK(count_data_files(path) == files_before + 1);   // sources plus the squatter
            db.close();
        }

        // Whatever was left, reopening settles it and every key is still there.
        fs::remove(squatter_path);
        auto reopened = utxoz::full_db::open_for_testing(path);
        REQUIRE(reopened);
        auto db2 = std::move(*reopened);
        CHECK(count_reserved(path) == 0);
        CHECK(all_keys(db2) == expected);
        db2.close();

        fs::remove_all(path);
    }
}

TEST_CASE("a target whose name is not durable retires nothing",
          "[database][compaction][recovery][failpoint]") {
    // Contents durable, name not, sources unlinked, power cut — that sequence
    // loses the entries outright. The barrier failing has to stop the merge
    // before the first source goes.
    for (bool reference_mode : {false, true}) {
        auto const path = unique_path(reference_mode ? "namebarrier_reference" : "namebarrier_full");
        fs::remove_all(path);
        INFO(std::string(reference_mode ? "reference mode" : "full mode"));

        std::string const prefix = reference_mode ? "compact_v" : "cont_0_v";

        if (reference_mode) {
            auto opened = utxoz::reference_db::open_for_testing(path, true);
            REQUIRE(opened);
            auto db = std::move(*opened);
            uint64_t next = 0;
            while (count_matching_exact(path, prefix) < 3) {
                REQUIRE(db.insert(make_key(next), next % 1000, uint32_t(next), 100).value());
                ++next;
                REQUIRE(next < 3'000'000);
            }
            for (uint64_t i = 0; i < next; i += 2) REQUIRE(db.erase(make_key(i), 400));
            (void)db.process_pending_deletions();
            db.close();
        } else {
            REQUIRE_FALSE(build_mergeable(path, 3).empty());
        }

        auto const files_before = count_matching_exact(path, prefix);

        scope_exit const disarm([] { failpoints::clear(); });
        failpoints::fail_directory_barrier_at.store(
            failpoints::dir_barrier::after_target, std::memory_order_relaxed);

        utxoz::result<> outcome;
        if (reference_mode) {
            auto opened = utxoz::reference_db::open_for_testing(path);
            REQUIRE(opened);
            auto db = std::move(*opened);
            outcome = db.compact_all();
            failpoints::clear();
            require_latched(db.compact_all());
            db.close();
        } else {
            auto opened = utxoz::full_db::open_for_testing(path);
            REQUIRE(opened);
            auto db = std::move(*opened);
            outcome = db.compact_all();
            failpoints::clear();
            require_latched(db.compact_all());
            db.close();
        }

        REQUIRE_FALSE(outcome);
        CHECK(outcome.error() == utxoz::error_code::recovery_required);

        // The target is there — it was published — and not one source went.
        CHECK(count_matching_exact(path, prefix) == files_before + 1);
        CHECK(count_matching(path, ".merge") == 1);

        // Reopening finishes it: the marker matches, so the sources are retired
        // now that the name is known to be there.
        size_t settled = 0;
        if (reference_mode) {
            auto again = utxoz::reference_db::open_for_testing(path);
            REQUIRE(again);
            auto db = std::move(*again);
            settled = count_matching_exact(path, prefix);
            db.close();
        } else {
            auto again = utxoz::full_db::open_for_testing(path);
            REQUIRE(again);
            auto db = std::move(*again);
            settled = count_matching_exact(path, prefix);
            db.close();
        }
        CHECK(settled < files_before + 1);
        CHECK(count_reserved(path) == 0);

        // And again, unchanged.
        if (reference_mode) {
            auto again = utxoz::reference_db::open_for_testing(path);
            REQUIRE(again);
            auto db = std::move(*again);
            CHECK(count_matching_exact(path, prefix) == settled);
            db.close();
        } else {
            auto again = utxoz::full_db::open_for_testing(path);
            REQUIRE(again);
            auto db = std::move(*again);
            CHECK(count_matching_exact(path, prefix) == settled);
            db.close();
        }

        fs::remove_all(path);
    }
}

#endif // _WIN32

// =============================================================================
// A record says what it says, on every word size
// =============================================================================

TEST_CASE("identities that do not fit a size_t are refused, not truncated",
          "[database][compaction][sidecar]") {
    // The record stores identities as 64-bit values; size_t is half that on
    // wasm32. Truncating one produces a different number that may be a
    // perfectly plausible identity, and every later check would then be
    // validating something the record never said.
    // Deliberately hand-built, not encode_merge_plan(). That encoder takes a
    // merge_plan, whose fields are size_t — so on a 32-bit target it would
    // truncate before encoding and destroy the very condition this checks. A
    // record is bytes on disk, and this is the only way to write bytes the
    // running program could not have produced.
    auto encode_raw_wire_plan = [](uint64_t container, uint64_t target, uint64_t source) {
        std::vector<uint8_t> out;
        auto put = [&out](auto value) {
            auto const* src = reinterpret_cast<uint8_t const*>(&value);
            out.insert(out.end(), src, src + sizeof(value));
        };

        out.insert(out.end(), utxoz::detail::merge_plan::magic.begin(),
                   utxoz::detail::merge_plan::magic.end());
        put(utxoz::detail::merge_plan::current_format);
        put(uint16_t{0});
        put(container);
        put(target);
        put(uint64_t{0x0123456789abcdefULL});   // merge id, non-zero
        put(uint64_t{0xfedcba9876543210ULL});
        put(uint64_t{1});                       // one source
        put(source);

        uint32_t hash = 2166136261u;
        for (auto const b : out) { hash ^= b; hash *= 16777619u; }
        put(hash);
        return out;
    };

    // A plan that fits everywhere, to show the encoder itself is sound.
    {
        auto const bytes = encode_raw_wire_plan(0, 7, 3);
        auto const plan = utxoz::detail::decode_merge_plan(bytes);
        REQUIRE(plan);
        CHECK(plan->target == 7);
        CHECK(plan->sources == std::vector<size_t>{3});
    }

    constexpr uint64_t too_wide = uint64_t{1} << 40;   // fits 64 bits, not 32

    if constexpr (sizeof(size_t) < sizeof(uint64_t)) {
        // Where it would narrow, every field is refused.
        CHECK_FALSE(utxoz::detail::decode_merge_plan(encode_raw_wire_plan(too_wide, 7, 3)));
        CHECK_FALSE(utxoz::detail::decode_merge_plan(encode_raw_wire_plan(0, too_wide, 3)));
        CHECK_FALSE(utxoz::detail::decode_merge_plan(encode_raw_wire_plan(0, 7, too_wide)));
    } else {
        // Where it does not narrow, the value is carried through unchanged
        // rather than rejected for being large.
        auto const plan = utxoz::detail::decode_merge_plan(encode_raw_wire_plan(0, too_wide, 3));
        REQUIRE(plan);
        CHECK(plan->target == size_t(too_wide));
    }
}

#ifndef _WIN32

/**
 * Compaction closes the active container before it starts. Putting one back was
 * a scope guard, and a guard runs in a destructor: it cannot report, and it must
 * swallow what it catches or end the process. So a reopen that failed left the
 * call returning success with nothing mapped, and the next operation
 * dereferencing a null container.
 *
 * The reopen is part of the result now, and it latches — an instance with no
 * active container has nothing to serve from.
 */
TEST_CASE("a compaction that cannot reopen its active container says so",
          "[database][compaction][failpoint]") {
    for (bool reference_mode : {false, true}) {
        auto const path = unique_path(reference_mode ? "reopenfail_reference" : "reopenfail_full");
        fs::remove_all(path);
        INFO(std::string(reference_mode ? "reference mode" : "full mode"));

        std::string const prefix = reference_mode ? "compact_v" : "cont_0_v";

        if (reference_mode) {
            auto opened = utxoz::reference_db::open_for_testing(path, true);
            REQUIRE(opened);
            auto db = std::move(*opened);
            uint64_t next = 0;
            while (count_matching_exact(path, prefix) < 3) {
                REQUIRE(db.insert(make_key(next), next % 1000, uint32_t(next), 100).value());
                ++next;
                REQUIRE(next < 3'000'000);
            }
            for (uint64_t i = 0; i < next; i += 2) REQUIRE(db.erase(make_key(i), 400));
            (void)db.process_pending_deletions();
            db.close();
        } else {
            REQUIRE_FALSE(build_mergeable(path, 3).empty());
        }

        scope_exit const disarm([] { failpoints::clear(); });

        if (reference_mode) {
            auto opened = utxoz::reference_db::open_for_testing(path);
            REQUIRE(opened);
            auto db = std::move(*opened);

            // Armed only once the database is open, so what this exercises is
            // the reopen inside compact_all() and nothing else. Arming it
            // earlier would leave the case quietly depending on open() not
            // consulting the same failpoint.
            failpoints::fail_container_open.store(true, std::memory_order_relaxed);

            auto const outcome = db.compact_all();
            failpoints::clear();

            // Reported, not swallowed.
            REQUIRE_FALSE(outcome);
            CHECK(outcome.error() == utxoz::error_code::file_open_failed);

            // And nothing is served from an instance with no active container.
            require_latched(db.compact_all());
            require_latched(db.insert(make_key(9'400'000), 1, 2, 900));
            db.close();
        } else {
            auto opened = utxoz::full_db::open_for_testing(path);
            REQUIRE(opened);
            auto db = std::move(*opened);

            failpoints::fail_container_open.store(true, std::memory_order_relaxed);

            auto const outcome = db.compact_all();
            failpoints::clear();

            REQUIRE_FALSE(outcome);
            CHECK(outcome.error() == utxoz::error_code::file_open_failed);

            require_latched(db.compact_all());
            require_latched(db.insert(make_key(9'400'000), make_value(8, 1), 900));
            db.close();
        }

        // Closing and reopening is the way out, and it works.
        if (reference_mode) {
            auto again = utxoz::reference_db::open_for_testing(path);
            REQUIRE(again);
            auto db = std::move(*again);
            CHECK(count_reserved(path) == 0);
            REQUIRE(db.insert(make_key(9'400'001), 1, 2, 950).value());
            db.close();
        } else {
            auto again = utxoz::full_db::open_for_testing(path);
            REQUIRE(again);
            auto db = std::move(*again);
            CHECK(count_reserved(path) == 0);
            REQUIRE(db.insert(make_key(9'400'001), make_value(8, 1), 950).value());
            db.close();
        }

        fs::remove_all(path);
    }
}

#endif // _WIN32

/**
 * A file at a canonical name that is not one of ours must be rejected at once.
 *
 * Boost, handed a mapped file whose header does not read as initialised,
 * assumes another process is midway through creating it and spins waiting —
 * `BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC`, five
 * minutes by default — before reporting corruption. Nothing here waits on
 * another process creating a segment, so that wait is pure delay: an open that
 * meets a truncated version file would stall for five minutes, and for five
 * minutes *per file* if several were damaged.
 *
 * Two things prevent it: the size check that refuses a file too small to be one
 * of ours before Boost ever sees it, and the build's cap on that timeout. They
 * are complementary — the first is instant and covers the common case, the
 * second bounds anything the first cannot recognise.
 *
 * The bound below is what distinguishes them from their absence. With the size
 * check the traversal takes milliseconds; with only the timeout cap it takes
 * about twenty seconds; with neither, five minutes. Five seconds is therefore
 * loose by three orders of magnitude against the real figure and still fails if
 * either protection is removed. It is not measuring performance — the outcome
 * is the same error either way, so elapsed time is the only observable that
 * changes.
 */
TEST_CASE("a truncated version file is refused promptly, not after a long wait",
          "[database][recovery]") {
    auto const path = unique_path("truncated");
    fs::remove_all(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    auto const expected = build_mergeable(path, 3);
    REQUIRE_FALSE(expected.empty());

    // Truncate a version that is not the active one, so a traversal reaches it.
    std::vector<std::string> older;
    for (auto const& entry : fs::directory_iterator(path)) {
        auto const name = entry.path().filename().string();
        if (name.rfind("cont_0_v", 0) == 0 && name.ends_with(".dat")) older.push_back(name);
    }
    std::ranges::sort(older);
    REQUIRE(older.size() >= 3);

    auto const victim = fs::path(path) / older.front();
    REQUIRE(fs::file_size(victim) > 1024);
    {
        std::ofstream ofs(victim, std::ios::binary | std::ios::trunc);
        ofs << "far too small to be a segment";
    }

    auto opened = utxoz::full_db::open_for_testing(path);
    REQUIRE(opened);
    auto db = std::move(*opened);

    auto const start = std::chrono::steady_clock::now();
    auto const scanned = db.for_each_key([](utxoz::raw_outpoint const&) {});
    auto const elapsed = std::chrono::steady_clock::now() - start;

    REQUIRE_FALSE(scanned);
    CHECK(scanned.error() == utxoz::error_code::file_open_failed);

    auto const millis = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    INFO("the traversal took " << millis << " ms");
    CHECK(millis < 5000);

    db.close();
}

/**
 * The same preflight, reached through the file cache.
 *
 * Historical resolution does not open version files directly — it goes through
 * the cache, which had its own `open_only` and so its own five-minute wait. The
 * property worth testing is not "database_impl checks" but "every opening of an
 * existing version goes through the same preflight", and the cache is the path
 * a node actually spends its time on: a deferred lookup or deletion sweeping
 * back through older generations.
 *
 * As above, the bound is what distinguishes the preflight from its absence; the
 * error is the same either way.
 */
TEST_CASE("a truncated version reached through the file cache is refused promptly",
          "[database][recovery][cache]") {
    SECTION("full mode, through a deferred lookup") {
        auto const path = unique_path("cachetrunc_full");
        fs::remove_all(path);
        scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

        auto const expected = build_mergeable(path, 3);
        REQUIRE_FALSE(expected.empty());

        // A key that lives in an older generation, so resolving it has to walk
        // back through the version the cache will open.
        auto const witness = expected.front();

        std::vector<std::string> older;
        for (auto const& entry : fs::directory_iterator(path)) {
            auto const name = entry.path().filename().string();
            if (name.rfind("cont_0_v", 0) == 0 && name.ends_with(".dat")) older.push_back(name);
        }
        std::ranges::sort(older);
        REQUIRE(older.size() >= 3);

        {
            std::ofstream ofs(fs::path(path) / older.front(), std::ios::binary | std::ios::trunc);
            ofs << "far too small to be a segment";
        }

        auto opened = utxoz::full_db::open_for_testing(path);
        REQUIRE(opened);
        auto db = std::move(*opened);

        // Misses the active map, which is what sends the resolution through the
        // cache.
        CHECK_FALSE(db.find(witness, 900));
        std::vector<utxoz::lookup_request> const batch{{witness, 900}};

        auto const start = std::chrono::steady_clock::now();
        auto const resolved = db.resolve(batch);
        auto const elapsed = std::chrono::steady_clock::now() - start;

        // Semantics before timing. A sweep that returned an error immediately —
        // before it ever reached the cache — would also be fast, and the
        // stopwatch alone would call that a pass. version_unreadable is only
        // produced by the open path, so getting it is itself evidence the cache
        // was reached.
        //
        // This used to require the sweep to succeed and hand the witness back in
        // the unresolved list. That is the ambiguity the second list no longer
        // carries: a truncated version is a failure to look up, not a proof of
        // absence, and a caller that read it as absence would reject a valid
        // block over a local storage fault.
        REQUIRE_FALSE(resolved);
        CHECK(resolved.error() == utxoz::error_code::version_unreadable);

        // Nothing was consumed: the batch is the caller's and comes back intact,
        // so the same vector can simply be retried.
        CHECK(batch.size() == 1);

        auto const millis = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        INFO("the sweep took " << millis << " ms");
        CHECK(millis < 5000);

        db.close();
    }

    SECTION("reference mode, through a deferred lookup") {
        auto const path = unique_path("cachetrunc_reference");
        fs::remove_all(path);
        scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

        utxoz::raw_outpoint witness{};

        {
            auto opened = utxoz::reference_db::open_for_testing(path, true);
            REQUIRE(opened);
            auto db = std::move(*opened);

            witness = make_key(0);
            uint64_t next = 0;
            while (count_matching_exact(path, "compact_v") < 3) {
                REQUIRE(db.insert(make_key(next), next % 1000, uint32_t(next), 100).value());
                ++next;
                REQUIRE(next < 3'000'000);
            }
            db.close();
        }

        std::vector<std::string> older;
        for (auto const& entry : fs::directory_iterator(path)) {
            auto const name = entry.path().filename().string();
            if (name.rfind("compact_v", 0) == 0 && name.ends_with(".dat")) older.push_back(name);
        }
        std::ranges::sort(older);
        REQUIRE(older.size() >= 3);

        {
            std::ofstream ofs(fs::path(path) / older.front(), std::ios::binary | std::ios::trunc);
            ofs << "far too small to be a segment";
        }

        auto opened = utxoz::reference_db::open_for_testing(path);
        REQUIRE(opened);
        auto db = std::move(*opened);

        CHECK_FALSE(db.find(witness, 900));
        std::vector<utxoz::lookup_request> const batch{{witness, 900}};

        auto const start = std::chrono::steady_clock::now();
        auto const resolved = db.resolve(batch);
        auto const elapsed = std::chrono::steady_clock::now() - start;

        // See above: the sweep has to have reached that request, not merely
        // returned quickly. version_unreadable comes only from the open path.
        REQUIRE_FALSE(resolved);
        CHECK(resolved.error() == utxoz::error_code::version_unreadable);
        CHECK(batch.size() == 1);

        auto const millis = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        INFO("the sweep took " << millis << " ms");
        CHECK(millis < 5000);

        db.close();
    }
}

/**
 * The other half of the protection, on its own.
 *
 * The preflight recognises a file too small to be one of ours. It cannot
 * recognise a file of exactly the right size whose header happens not to say
 * "initialised" — and that is the state Boost interprets as *another process is
 * still creating this*, which is what it waits for.
 *
 * This case builds precisely that: a version file of the configured size with
 * `InitializingSegment` written into its header word. The preflight passes it,
 * so what is left is the build's cap on Boost's timeout, and nothing else.
 *
 * Kept apart from the truncated-file case deliberately. That one pins the
 * preflight and finishes in milliseconds; this one pins the fallback and takes
 * about ten seconds. Folded together, whichever fired first would hide the
 * other, and a regression in either would still look green.
 */
TEST_CASE("a file whose header claims a creation in progress is not waited on for five minutes",
          "[database][recovery][slow]") {
    auto const path = unique_path("initializing");
    fs::remove_all(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    auto const expected = build_mergeable(path, 3);
    REQUIRE_FALSE(expected.empty());

    std::vector<std::string> older;
    for (auto const& entry : fs::directory_iterator(path)) {
        auto const name = entry.path().filename().string();
        if (name.rfind("cont_0_v", 0) == 0 && name.ends_with(".dat")) older.push_back(name);
    }
    std::ranges::sort(older);
    REQUIRE(older.size() >= 3);

    auto const victim = fs::path(path) / older.front();
    auto const original_size = fs::file_size(victim);

    // Boost's states are Uninitialized, Initializing, Initialized, Corrupted —
    // in that order, in a uint32 at offset zero. `Initializing` is the one it
    // waits on; `Corrupted` it reports at once, which would not exercise this.
    {
        std::fstream f(victim, std::ios::binary | std::ios::in | std::ios::out);
        REQUIRE(f);
        uint32_t const initializing = 1;
        f.seekp(0);
        f.write(reinterpret_cast<char const*>(&initializing), sizeof(initializing));
        REQUIRE(f);
    }

    // The size is untouched, so the preflight has nothing to object to — which
    // is the point: this reaches Boost.
    REQUIRE(fs::file_size(victim) == original_size);
    REQUIRE(original_size >= utxoz::detail::smallest_configured_file);

    auto opened = utxoz::full_db::open_for_testing(path);
    REQUIRE(opened);
    auto db = std::move(*opened);

    auto const start = std::chrono::steady_clock::now();
    auto const scanned = db.for_each_key([](utxoz::raw_outpoint const&) {});
    auto const elapsed = std::chrono::steady_clock::now() - start;

    REQUIRE_FALSE(scanned);
    CHECK(scanned.error() == utxoz::error_code::file_open_failed);

    auto const seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    INFO("the traversal took " << seconds << " seconds");

    // Generous: the cap is ten seconds, the default it replaces is three
    // hundred. Anything in between says the cap is in force; thirty leaves room
    // for a slow machine without coming close to the default.
    CHECK(seconds < 30);

    db.close();
}

#ifndef _WIN32

/**
 * The same eight barriers, in reference mode.
 *
 * The protocol is written twice — once per storage mode — and only the full
 * one was covered here. Two copies of a sequence whose correctness is entirely
 * in its ordering is exactly the shape that drifts, and a matrix that watches
 * one of them would not notice the other drifting.
 *
 * This is the net for #72: once the two are one piece of code, both matrices
 * exercise it, and a barrier moved in the shared version fails on both sides
 * rather than silently in the mode nobody watched.
 */
TEST_CASE("a crash at any barrier of a reference-mode merge leaves a state reopening can settle",
          "[database][compaction][recovery][crash][reference]") {
    struct step {
        crash_point point;
        char const* name;
        bool target_published;
    };

    std::vector<step> const steps{
        {crash_point::after_build,           "after_build",           false},
        {crash_point::after_file_sync,       "after_file_sync",       false},
        {crash_point::after_sidecar_sync,    "after_sidecar_sync",    false},
        {crash_point::after_sidecar_publish, "after_sidecar_publish", false},
        {crash_point::after_target_publish,  "after_target_publish",  true},
        {crash_point::before_source_unlink,  "before_source_unlink",  true},
        {crash_point::mid_source_unlink,     "mid_source_unlink",     true},
        {crash_point::after_sources_retired, "after_sources_retired", true},
    };

    for (auto const& s : steps) {
        auto const path = unique_path(fmt::format("referencecrash_{}", s.name));
        fs::remove_all(path);
        INFO("crash point: " << s.name);
        scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

        auto const expected = build_mergeable_reference(path, 3);
        REQUIRE_FALSE(expected.empty());
        auto const files_before = count_matching_exact(path, "compact_v");
        REQUIRE(files_before == 3);

        die_as<utxoz::reference_db>(path, s.point);

        std::vector<utxoz::raw_outpoint> expected_after;

        auto const files_after_crash = count_matching_exact(path, "compact_v");
        INFO("after the crash: " << files_after_crash << " data files");

        if (s.target_published) {
            REQUIRE(files_after_crash > 0);
        } else {
            REQUIRE(files_after_crash == files_before);
        }

        // Recovery, then the row this crash belongs to.
        {
            auto opened = utxoz::reference_db::open_for_testing(path);
            REQUIRE(opened);
            auto db = std::move(*opened);

            auto const files_after_recovery = count_matching_exact(path, "compact_v");
            INFO("after recovery: " << files_after_recovery << " data files");

            if (s.target_published) {
                REQUIRE(files_after_recovery < files_before);
            } else {
                REQUIRE(files_after_recovery == files_before);
            }
            REQUIRE(count_reserved(path) == 0);

            std::vector<utxoz::raw_outpoint> keys;
            REQUIRE(db.for_each_key([&](utxoz::raw_outpoint const& k) { keys.push_back(k); }));
            std::ranges::sort(keys);
            REQUIRE(std::adjacent_find(keys.begin(), keys.end()) == keys.end());
            REQUIRE(keys == expected);

            // It serves: reads, writes, deletes and another compaction.
            auto const fresh = make_key(9'500'000);
            REQUIRE(db.insert(fresh, 1, 2, 500).value());
            REQUIRE(db.find(fresh, 500));

            // The count, not merely the absence of an error: a successful
            // delete of nothing satisfies the latter, and a database that
            // accepts writes but silently deletes none of them would pass.
            REQUIRE(db.erase(fresh, 600).value() == 1);
            (void)db.process_pending_deletions();
            CHECK_FALSE(db.find(fresh, 700));

            REQUIRE(db.compact_all());

            // What the database holds once this iteration has finished with it.
            // The second recovery must find exactly this.
            REQUIRE(db.for_each_key([&](utxoz::raw_outpoint const& k) {
                expected_after.push_back(k);
            }));
            std::ranges::sort(expected_after);

            db.close();
        }

        // And a second recovery changes nothing.
        auto const settled = count_matching_exact(path, "compact_v");
        auto again = utxoz::reference_db::open_for_testing(path);
        REQUIRE(again);
        auto db2 = std::move(*again);
        REQUIRE(count_matching_exact(path, "compact_v") == settled);
        REQUIRE(count_reserved(path) == 0);

        // On the keys, not only on the file count. A recovery that moved or
        // dropped entries without changing how many files there are would
        // satisfy every count above and still have lost data.
        std::vector<utxoz::raw_outpoint> settled_keys;
        REQUIRE(db2.for_each_key([&](utxoz::raw_outpoint const& k) {
            settled_keys.push_back(k);
        }));
        std::ranges::sort(settled_keys);
        REQUIRE(std::adjacent_find(settled_keys.begin(), settled_keys.end())
                == settled_keys.end());
        REQUIRE(settled_keys == expected_after);

        db2.close();
    }
}

#endif // _WIN32
