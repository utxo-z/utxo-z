// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_database_lock.cpp
 * @brief One instance at a time, and what that does and does not mean.
 *
 * The store is written for a single owner. #70 made compaction safe against a
 * second process destroying data, but that is one hazard, not multi-process
 * safety: two instances would still each hold their own catalogue, their own
 * deferred queues, and their own idea of which version is active.
 *
 * What is pinned here is the claim itself — that a second instance is refused,
 * that the refusal says which of the two problems it is, and above all that the
 * claim dies with the process that took it. A lock *file* would outlive a crash
 * and lock a database out until someone deleted it by hand; a lock on an open
 * descriptor is released by the kernel when the last copy of that descriptor
 * closes. The `_exit` case below is the one that tells those apart.
 */

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
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>

#include "detail/database_lock.hpp"
#include "detail/scope_exit.hpp"

namespace fs = std::filesystem;
using utxoz::detail::database_lock;
using utxoz::detail::scope_exit;

namespace {

inline std::atomic<uint64_t> dl_counter{0};

std::string unique_path(std::string_view tag) {
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fmt::format("./test_dl_{}_{}_{}_{}", tag, getpid(), ts, dl_counter.fetch_add(1));
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

} // anonymous namespace

TEST_CASE("a second instance over one database is refused", "[database][lock]") {
    auto const path = unique_path("second");
    fs::remove_all(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    auto first = utxoz::full_db::open_for_testing(path, true);
    REQUIRE(first);
    auto db = std::move(*first);

    auto const second = utxoz::full_db::open_for_testing(path);
    REQUIRE_FALSE(second);
    CHECK(second.error() == utxoz::error_code::database_in_use);

    // The one that has it is unaffected.
    REQUIRE(db.insert(make_key(1), make_value(8, 1), 100).value());
    REQUIRE(db.find(make_key(1), 100));

    db.close();

    // And once it lets go, the next one gets in.
    auto const third = utxoz::full_db::open_for_testing(path);
    REQUIRE(third);
}

TEST_CASE("the claim is not the existence of a file", "[database][lock]") {
#ifndef _WIN32
    auto const path = unique_path("processdeath");
    fs::remove_all(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    {
        auto created = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(created);
        auto db = std::move(*created);
        REQUIRE(db.insert(make_key(1), make_value(8, 1), 100).value());
        db.close();
    }

    // The child takes the claim *after* the fork and dies holding it. It has to
    // be taken there and not before: a descriptor opened by the parent would be
    // copied into the child, the parent's copy would keep the claim alive after
    // the child died, and the case would prove the opposite of what it is for.
    pid_t const child = ::fork();
    REQUIRE(child >= 0);

    if (child == 0) {
        auto held = utxoz::full_db::open_for_testing(path);
        if ( ! held) ::_exit(2);
        ::_exit(99);   // no unwinding, no destructor, no release call
    }

    int status = 0;
    REQUIRE(::waitpid(child, &status, 0) == child);
    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 99);

    // The lock file is still there — it is permanent — and the claim is not.
    REQUIRE(fs::exists(fs::path(path) / database_lock::file_name));

    auto const after = utxoz::full_db::open_for_testing(path);
    INFO("a lock file would still be refusing here");
    REQUIRE(after);
#endif
}

TEST_CASE("emptying a database does not replace the file the claim is on",
          "[database][lock]") {
    auto const path = unique_path("removeexisting");
    fs::remove_all(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    {
        auto seeded = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(seeded);
        auto db = std::move(*seeded);
        REQUIRE(db.insert(make_key(7), make_value(8, 1), 100).value());
        db.close();
    }

    // A opens with remove_existing and keeps the database open. Emptying has to
    // leave the lock file alone: removing the directory would unlink the inode
    // A holds, and B would then create a fresh lock file, claim a different
    // inode, and both would believe they were alone.
    auto opened = utxoz::full_db::open_for_testing(path, true);
    REQUIRE(opened);
    auto db = std::move(*opened);

    REQUIRE(fs::exists(fs::path(path) / database_lock::file_name));
    CHECK_FALSE(db.find(make_key(7), 100));   // it really was emptied

    auto const other = utxoz::full_db::open_for_testing(path);
    REQUIRE_FALSE(other);
    CHECK(other.error() == utxoz::error_code::database_in_use);

    db.close();
}

TEST_CASE("a lock file that is not a regular file is not a database in use",
          "[database][lock]") {
#ifndef _WIN32
    auto const path = unique_path("symlink");
    fs::remove_all(path);
    fs::create_directories(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    // A symlink where the lock file belongs. Following it is not something to
    // do, and it is emphatically not "another node is running".
    auto const elsewhere = fs::path(path) / "elsewhere";
    { std::ofstream ofs(elsewhere); ofs << "x"; }

    std::error_code ec;
    fs::create_symlink(elsewhere, fs::path(path) / database_lock::file_name, ec);
    if (ec) SKIP("this filesystem does not support symlinks");

    auto const opened = utxoz::full_db::open_for_testing(path);
    REQUIRE_FALSE(opened);
    // The distinction that matters: not database_in_use. Telling an operator
    // another node holds the database would send them looking for a process
    // that does not exist.
    CHECK(opened.error() == utxoz::error_code::database_lock_unavailable);
#endif
}

TEST_CASE("a directory that cannot be written gives unavailable, not in use",
          "[database][lock]") {
#ifndef _WIN32
    auto const path = unique_path("noperm");
    fs::remove_all(path);
    fs::create_directories(path);

    scope_exit const cleanup([&] {
        std::error_code ec;
        fs::permissions(path, fs::perms::owner_all, ec);
        fs::remove_all(path, ec);
    });

    fs::permissions(path, fs::perms::owner_read | fs::perms::owner_exec);

    std::ofstream probe(fs::path(path) / "probe.tmp");
    if (probe.is_open()) {
        probe.close();
        SKIP("file permissions do not restrict this process");
    }

    auto const opened = utxoz::full_db::open_for_testing(path);
    REQUIRE_FALSE(opened);
    CHECK(opened.error() == utxoz::error_code::database_lock_unavailable);
#endif
}

TEST_CASE("a failed open releases the claim it took", "[database][lock]") {
    auto const path = unique_path("failedopen");
    fs::remove_all(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    // A reference database, then opened as a full one. configure() takes the
    // claim before it checks the mode, so this is a failure *after* the claim —
    // exactly the shape that would leak it if release were a call at each exit
    // rather than a destructor.
    {
        auto created = utxoz::reference_db::open_for_testing(path, true);
        REQUIRE(created);
        auto db = std::move(*created);
        db.close();
    }

    auto const wrong_mode = utxoz::full_db::open_for_testing(path);
    REQUIRE_FALSE(wrong_mode);
    CHECK(wrong_mode.error() == utxoz::error_code::storage_mode_mismatch);

    // If the claim had leaked, this would be refused.
    auto const reopened = utxoz::reference_db::open_for_testing(path);
    REQUIRE(reopened);
}

TEST_CASE("close() is idempotent and the database can be opened again",
          "[database][lock]") {
    auto const path = unique_path("reopen");
    fs::remove_all(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    {
        auto opened = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(opened);
        auto db = std::move(*opened);
        REQUIRE(db.insert(make_key(3), make_value(8, 1), 100).value());
        db.close();
        db.close();   // again, and nothing comes of it
    }

    for (int i = 0; i < 3; ++i) {
        INFO("reopen " << i);
        auto opened = utxoz::full_db::open_for_testing(path);
        REQUIRE(opened);
        auto db = std::move(*opened);
        REQUIRE(db.find(make_key(3), 200));
        db.close();
    }
}

TEST_CASE("separate databases do not exclude each other", "[database][lock]") {
    auto const first_path = unique_path("apart_a");
    auto const second_path = unique_path("apart_b");
    fs::remove_all(first_path);
    fs::remove_all(second_path);
    scope_exit const cleanup([&] {
        std::error_code ec;
        fs::remove_all(first_path, ec);
        fs::remove_all(second_path, ec);
    });

    auto a = utxoz::full_db::open_for_testing(first_path, true);
    REQUIRE(a);
    auto b = utxoz::full_db::open_for_testing(second_path, true);
    REQUIRE(b);

    auto db_a = std::move(*a);
    auto db_b = std::move(*b);

    REQUIRE(db_a.insert(make_key(1), make_value(8, 1), 100).value());
    REQUIRE(db_b.insert(make_key(1), make_value(8, 2), 100).value());
}

TEST_CASE("the claim is taken before recovery runs", "[database][lock]") {
    auto const path = unique_path("beforerecovery");
    fs::remove_all(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    {
        auto created = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(created);
        auto db = std::move(*created);
        REQUIRE(db.insert(make_key(1), make_value(8, 1), 100).value());
        db.close();
    }

    // Something a recovery would have to act on. It must not be acted on by a
    // second instance while the first holds the database: recovery unlinks
    // files, and two of them doing that at once over one directory is the
    // hazard the claim exists to prevent.
    {
        std::ofstream ofs(fmt::format("{}/cont_0_v00007.dat.building", path), std::ios::binary);
        ofs << "an unfinished build";
    }

    auto holder = utxoz::full_db::open_for_testing(path);
    REQUIRE(holder);
    auto db = std::move(*holder);

    auto const second = utxoz::full_db::open_for_testing(path);
    REQUIRE_FALSE(second);
    CHECK(second.error() == utxoz::error_code::database_in_use);

    // Recovery ran: the unfinished build is gone. That says the claim did not
    // stop this instance from recovering — it does not, on its own, prove the
    // claim was taken *first*. Nothing observable from out here distinguishes
    // the two orders; the ordering is verified by reading configure(), and this
    // pins the half that is observable.
    CHECK_FALSE(fs::exists(fmt::format("{}/cont_0_v00007.dat.building", path)));

    db.close();
}

TEST_CASE("the lock file names its holder, and nothing depends on that",
          "[database][lock]") {
    auto const path = unique_path("holder");
    fs::remove_all(path);
    scope_exit const cleanup([&] { std::error_code ec; fs::remove_all(path, ec); });

    auto const before = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    {
        auto opened = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(opened);
        auto db = std::move(*opened);

        // Readable while the claim is held on POSIX only. flock leaves the
        // contents readable; LockFileEx locks them, so on Windows nothing can
        // read this until the holder lets go — which is exactly when it would
        // have been useful. Documented rather than worked around: giving
        // Windows a readable holder means locking one region and keeping the
        // text in another, which is a contract to design, not something to slip
        // into a test fix.
#ifndef _WIN32
        std::ifstream ifs(fs::path(path) / database_lock::file_name);
        REQUIRE(ifs);

        long long pid = 0;
        long long stamp = 0;
        ifs >> pid >> stamp;
        REQUIRE(ifs);

        CHECK(pid == static_cast<long long>(getpid()));

        auto const after = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        CHECK(stamp >= before);
        CHECK(stamp <= after);
#else
        (void)before;
#endif

        db.close();
    }

    // This half holds everywhere, and it is the one that matters: the text is
    // diagnostic, not evidence. Whatever is in the file, the kernel decides who
    // holds the database. If it were ever consulted for ownership, this is
    // where that would show.
    {
        std::ofstream ofs(fs::path(path) / database_lock::file_name,
                          std::ios::trunc);
        ofs << "not a pid at all\n";
    }

    auto reopened = utxoz::full_db::open_for_testing(path);
    REQUIRE(reopened);
    auto db = std::move(*reopened);

#ifndef _WIN32
    // Reopening rewrote it with this process, which is what keeps it useful to
    // whoever reads a log.
    std::ifstream ifs(fs::path(path) / database_lock::file_name);
    long long pid = 0;
    ifs >> pid;
    CHECK(pid == static_cast<long long>(getpid()));
#endif

    db.close();
}

TEST_CASE("a directory that cannot be listed is not a directory that is empty",
          "[database][lock]") {
#ifndef _WIN32
    auto const path = unique_path("unlistable");
    fs::remove_all(path);
    fs::create_directories(path);

    scope_exit const cleanup([&] {
        std::error_code ec;
        fs::permissions(path, fs::perms::owner_all, ec);
        fs::remove_all(path, ec);
    });

    {
        auto seeded = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(seeded);
        auto db = std::move(*seeded);
        REQUIRE(db.insert(make_key(11), make_value(8, 1), 100).value());
        db.close();
    }

    // Write and traverse but not read. Known names can still be opened — so the
    // claim is taken — and the directory cannot be listed, which is what
    // remove_existing needs to do next.
    //
    // What this pins is the outcome: an open that cannot enumerate reports it,
    // and the data survives. It does **not** isolate the check inside the
    // emptying loop, and it cannot: recovery enumerates the same directory a
    // few lines later and answers with the same error, so from out here the two
    // paths are indistinguishable. The loop's own check is ordering hygiene —
    // it stops a wipe being reported as done when it never ran — and is
    // verified by reading it, not by this case.
    fs::permissions(path, fs::perms::owner_write | fs::perms::owner_exec);

    std::error_code probe_ec;
    fs::directory_iterator probe(path, probe_ec);
    if ( ! probe_ec) {
        fs::permissions(path, fs::perms::owner_all);
        SKIP("file permissions do not restrict this process");
    }

    auto const opened = utxoz::full_db::open_for_testing(path, true);
    REQUIRE_FALSE(opened);
    CHECK(opened.error() == utxoz::error_code::catalog_unreadable);

    // And the data it could not enumerate is still there.
    fs::permissions(path, fs::perms::owner_all);
    auto reopened = utxoz::full_db::open_for_testing(path);
    REQUIRE(reopened);
    auto db = std::move(*reopened);
    CHECK(db.find(make_key(11), 200));
    db.close();
#endif
}
