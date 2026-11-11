// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_persisted_segment_lock.cpp
 * @brief A lock left taken inside a version file must not cost the file.
 *
 * A managed segment keeps its segment manager's mutex inside the mapped file,
 * and Boost's named-object lookup takes it. Taking it writes to a shared
 * mapping, so the taken state is a dirty page like any other: the kernel may
 * write it back whenever it likes, and a file barrier made while a lookup is
 * inside puts it on stable storage outright. Lose power before the release
 * reaches the disk and the file comes back with a lock held by nobody.
 *
 * Nothing recovers such a lock, so nothing may wait for it. The cases below
 * build exactly that file -- a copy taken while the lock is held -- and
 * require the database to open it and answer from it.
 *
 * Not on Windows: there the mutex in the file is the name of a kernel object,
 * and its state is not in the file. Not on Emscripten, which has no second
 * process to run the reopening in.
 */

#if ! defined(_WIN32) && ! defined(__EMSCRIPTEN__)

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <optional>
#include <latch>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <boost/interprocess/managed_mapped_file.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>

#include "detail/scope_exit.hpp"

namespace bip = boost::interprocess;
namespace fs = std::filesystem;

using utxoz::detail::scope_exit;
using utxoz::lookup_request;

namespace {

std::string unique_path(std::string_view tag) {
    auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fmt::format("./test_seglock_{}_{}_{}", tag, getpid(), ts);
}

utxoz::raw_outpoint outpoint_of(uint64_t n) {
    utxoz::raw_outpoint key{};
    std::memcpy(key.data(), &n, sizeof(n));
    key[20] = 0x5A;
    return key;
}

std::vector<uint8_t> value_of(uint8_t seed) {
    std::vector<uint8_t> v(33);
    std::iota(v.begin(), v.end(), seed);
    return v;
}

/// Replaces every version file under `dir` with a copy of itself taken while
/// its segment manager's lock was held: what a barrier, or the kernel, would
/// have left on the disk had it caught a lookup inside.
///
/// `holder` takes the locks and is then parked until `release` is counted
/// down. The lock is recursive and remembers its owner, so the owner has to be
/// a thread other than the one that reopens the files: the reopening runs in a
/// child forked from the calling thread, which keeps that thread's identity.
size_t leave_every_lock_taken(fs::path const& dir, std::thread& holder, std::latch& release) {
    std::vector<fs::path> files;
    for (auto const& e : fs::directory_iterator(dir)) {
        // Version files are megabytes; the catalogue and metadata are not.
        if (e.is_regular_file() && e.file_size() >= 1024u * 1024u) files.push_back(e.path());
    }

    std::latch taken(1);
    holder = std::thread([&files, &taken, &release] {
        for (auto const& file : files) {
            auto const copy = fs::path(file.string() + ".locked");
            {
                bip::managed_mapped_file segment(bip::open_only, file.c_str());
                auto copy_while_held = [&] { fs::copy_file(file, copy); };
                segment.atomic_func(copy_while_held);
            }
            fs::rename(copy, file);
        }
        taken.count_down();
        release.wait();
    });
    taken.wait();
    return files.size();
}

/// What the child reports through its exit status: one bit per thing that
/// went wrong, so zero is the only good answer.
enum : int { not_opened = 1, newest_missed = 2, oldest_unresolved = 4 };

/// The result of running `work` in a process of its own: its exit status, or
/// nothing if it had to be killed at `limit`.
///
/// A process, because a lookup that waits for a lock nobody holds never
/// returns, and a thread in that state can be neither stopped nor collected.
/// The child is; the suite goes on with nothing left running behind it.
template <typename F>
std::optional<int> exit_status_within(std::chrono::seconds limit, F work) {
    auto const child = ::fork();
    REQUIRE(child >= 0);
    if (child == 0) {
        // No destructors, no atexit: the parent's state is not the child's to unwind.
        ::_exit(work());
    }

    auto const deadline = std::chrono::steady_clock::now() + limit;
    for (;;) {
        int status = 0;
        auto const reaped = ::waitpid(child, &status, WNOHANG);
        if (reaped == child) {
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            return std::nullopt;   // killed by something else: not an answer either
        }
        if (reaped < 0 || std::chrono::steady_clock::now() >= deadline) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    ::kill(child, SIGKILL);
    ::waitpid(child, nullptr, 0);
    return std::nullopt;
}

} // namespace

TEST_CASE("full: a lock persisted taken in a version file does not block the database",
          "[database][recovery][segment-lock]") {
    auto const path = fs::path(unique_path("full"));
    fs::remove_all(path);
    std::thread holder;
    std::latch release(1);
    scope_exit const cleanup([&] {
        release.count_down();
        if (holder.joinable()) holder.join();
        std::error_code ec;
        fs::remove_all(path, ec);
    });

    // Two generations of one class: an active one for find(), an older one for
    // a resolution to open through the cache.
    constexpr uint64_t first = 10'000;
    uint64_t next = first;
    {
        auto opened = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        while (db.get_statistics().rotations_per_container[0] < 1) {
            for (int i = 0; i < 5'000; ++i, ++next) {
                REQUIRE(db.insert(outpoint_of(next), utxoz::output_data_span{value_of(7)}, 2).has_value());
            }
            REQUIRE(next < 4'000'000);
        }
        db.close();
    }

    REQUIRE(leave_every_lock_taken(path, holder, release) >= 2);

    auto const newest = outpoint_of(next - 1);
    auto const oldest = outpoint_of(first);

    auto const status = exit_status_within(std::chrono::seconds(30), [&]() -> int {
        auto opened = utxoz::full_db::open_for_testing(path, false);
        if ( ! opened) return not_opened;
        auto const& db = *opened;
        int wrong = 0;
        if ( ! db.find(newest, 100'000).has_value()) wrong |= newest_missed;
        std::vector<lookup_request> batch{{oldest, 100'000}};
        auto const got = db.resolve(batch);
        if ( ! got.has_value() || got->found.size() != 1) wrong |= oldest_unresolved;
        return wrong;
    });

    // No status at all is the defect itself: the child never came back.
    REQUIRE(status.has_value());
    CHECK(*status == 0);
}

#endif // ! _WIN32 && ! __EMSCRIPTEN__
