// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file database_lock.hpp
 * @brief One instance at a time over a database directory.
 * @internal
 *
 * The store is written for a single owner: one catalogue, one set of deferred
 * queues, one active container per size class. Two instances over one directory
 * would each hold their own and neither would know about the other's rotations,
 * merges or recoveries. #70 made compaction safe against a second process
 * *destroying data* — a merge cannot retire a source unless the target carries
 * its identifier — but safe against that one hazard is not the same as
 * multi-process safe.
 *
 * What this excludes is **cooperating processes using this library**. A process
 * that ignores the lock, or someone deleting files by hand, is outside it and
 * always will be; that is what an advisory lock is. It adds no thread safety
 * either: the contract of one mutating operation at a time, with nothing else
 * in flight, is unchanged. This closes the gap between instances, not the one
 * between threads.
 *
 * The claim is a lock on an open descriptor, not the existence of a file. The
 * distinction is the whole point: the kernel releases the claim when the last
 * copy of the descriptor closes, which includes a process dying however it
 * dies. A lock *file* would survive a crash and lock the database out until
 * someone deleted it by hand.
 *
 * "The last copy" is why the descriptor is opened close-on-exec and the handle
 * non-inheritable. A copy inherited across an exec would keep the claim alive
 * in a process that never asked for it, long after the one that took it exited.
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <utility>

#include <utxoz/types.hpp>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif ! defined(__EMSCRIPTEN__)
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace utxoz::detail {

namespace fs = std::filesystem;

/**
 * @brief An exclusive claim on a database directory, released by RAII.
 *
 * Move-only, and moved with the instance that owns it. Nothing releases it by
 * hand: a `configure()` that fails at any point after taking it drops it on the
 * way out, without a release call at each early return to forget.
 */
class database_lock {
public:
    /// The file the claim is taken on. Permanent: it is created once and never
    /// removed, renamed or replaced while any instance exists. Replacing it
    /// would leave the holder's lock on an inode nobody can find any more,
    /// while a second process locked the new one and both believed they were
    /// alone.
    static constexpr char const* file_name = ".utxoz.lock";

    database_lock() = default;

    database_lock(database_lock const&) = delete;
    database_lock& operator=(database_lock const&) = delete;

    database_lock(database_lock&& other) noexcept { swap(other); }

    database_lock& operator=(database_lock&& other) noexcept {
        if (this != &other) {
            release();
            swap(other);
        }
        return *this;
    }

    ~database_lock() { release(); }

    /**
     * @brief Claims `db_dir` for this instance.
     *
     * @return the claim; `error_code::database_in_use` when another instance
     *         holds it; `error_code::database_lock_unavailable` for anything
     *         else — no permission, a filesystem without locking, a lock file
     *         that is not a regular file. The two are kept apart because they
     *         mean different things to whoever reads the log: one says another
     *         node is running, the other says this machine cannot answer.
     */
    [[nodiscard]]
    static result<database_lock> acquire(fs::path const& db_dir);

    [[nodiscard]]
    bool held() const noexcept {
#if defined(_WIN32)
        return handle_ != INVALID_HANDLE_VALUE;
#elif defined(__EMSCRIPTEN__)
        return vacuous_;
#else
        return fd_ >= 0;
#endif
    }

    /**
     * @brief Writes who holds it, for a human reading a log.
     *
     * Best effort and diagnostic only. It is written after the claim is taken,
     * never consulted to decide ownership, and never read by recovery — the
     * kernel's answer is the only thing that decides whether a database is in
     * use, and this text is only ever looked at once that answer is yes. A
     * failure to write it does not weaken a claim already held.
     *
     * @warning Readable while the claim is held on POSIX only. `flock` does not
     * stop anyone reading the file; `LockFileEx` locks its whole contents, so on
     * Windows nothing can read this until the holder lets go — which is exactly
     * when it would have been useful. The text is still written there, and is
     * still readable afterwards, but Windows gets no answer to "who has this
     * database open right now" from it.
     *
     * Making it work there would mean locking one region and keeping the text
     * in another, with offsets to design and to keep compatible. That is a
     * contract, not a detail, and it is not being decided as a side effect of
     * adding the claim. The claim itself works identically on both.
     */
    void record_holder() noexcept;

    void release() noexcept;

private:
    void swap(database_lock& other) noexcept {
#if defined(_WIN32)
        std::swap(handle_, other.handle_);
#elif defined(__EMSCRIPTEN__)
        std::swap(vacuous_, other.vacuous_);
#else
        std::swap(fd_, other.fd_);
#endif
    }

#if defined(_WIN32)
    HANDLE handle_ = INVALID_HANDLE_VALUE;
#elif defined(__EMSCRIPTEN__)
    /// The filesystem is virtual and there is no second process to exclude, so
    /// the claim is vacuous. Succeeding is right: refusing would break the one
    /// build where the hazard cannot arise.
    bool vacuous_ = false;
#else
    int fd_ = -1;
#endif
};

inline result<database_lock> database_lock::acquire(fs::path const& db_dir) {
    auto const path = db_dir / file_name;

#if defined(__EMSCRIPTEN__)
    (void)path;
    database_lock lock;
    lock.vacuous_ = true;
    return lock;

#elif defined(_WIN32)
    // A null security descriptor gives a non-inheritable handle, which is what
    // keeps the claim from outliving this process through a child.
    HANDLE const h = ::CreateFileW(path.wstring().c_str(), GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                   OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return std::unexpected(error_code::database_lock_unavailable);
    }

    OVERLAPPED overlapped{};
    if (::LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0,
                     MAXDWORD, MAXDWORD, &overlapped) == 0) {
        DWORD const last_error = ::GetLastError();
        ::CloseHandle(h);
        // Only this one means "someone else has it". Everything else is a
        // machine that could not answer, and saying "another node is running"
        // would send whoever reads it looking for a process that is not there.
        return std::unexpected(last_error == ERROR_LOCK_VIOLATION
                                   ? error_code::database_in_use
                                   : error_code::database_lock_unavailable);
    }

    database_lock lock;
    lock.handle_ = h;
    return lock;

#else
    // O_NOFOLLOW: the lock file is ours and is a regular file. A symlink in its
    // place is not something to follow into.
    int const fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0644);
    if (fd < 0) {
        return std::unexpected(error_code::database_lock_unavailable);
    }

    struct stat info{};
    if (::fstat(fd, &info) != 0 || ! S_ISREG(info.st_mode)) {
        ::close(fd);
        return std::unexpected(error_code::database_lock_unavailable);
    }

    if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
        int const err = errno;
        ::close(fd);
        return std::unexpected(err == EWOULDBLOCK || err == EAGAIN
                                   ? error_code::database_in_use
                                   : error_code::database_lock_unavailable);
    }

    database_lock lock;
    lock.fd_ = fd;
    return lock;
#endif
}

inline void database_lock::record_holder() noexcept {
    if ( ! held()) return;

    auto const now = std::chrono::system_clock::now();
    auto const seconds = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

#if defined(__EMSCRIPTEN__)
    (void)seconds;

#elif defined(_WIN32)
    auto const text = std::to_string(::GetCurrentProcessId()) + " " + std::to_string(seconds) + "\n";
    ::SetFilePointer(handle_, 0, nullptr, FILE_BEGIN);
    DWORD written = 0;
    (void)::WriteFile(handle_, text.data(), DWORD(text.size()), &written, nullptr);
    (void)::SetEndOfFile(handle_);

#else
    auto const text = std::to_string(::getpid()) + " " + std::to_string(seconds) + "\n";

    // Write first, then trim — truncating first would leave the file empty for
    // a moment, so a reader in that window would see nothing where there is a
    // holder. And pwrite does not touch the file offset, which nothing else
    // here maintains.
    //
    // The whole text goes down before anything is trimmed. A short write
    // followed by a truncate to that length would leave a line cut in half,
    // which is worse than leaving the previous holder's line: one is stale and
    // recognisable, the other is a number with digits missing.
    size_t offset = 0;
    while (offset < text.size()) {
        auto const written = ::pwrite(fd_, text.data() + offset, text.size() - offset,
                                      static_cast<off_t>(offset));
        if (written < 0) {
            if (errno == EINTR) continue;
            return;   // diagnostic: leave whatever is there rather than a fragment
        }
        if (written == 0) return;
        offset += static_cast<size_t>(written);
    }
    (void)::ftruncate(fd_, static_cast<off_t>(offset));
#endif
}

inline void database_lock::release() noexcept {
#if defined(__EMSCRIPTEN__)
    vacuous_ = false;

#elif defined(_WIN32)
    if (handle_ != INVALID_HANDLE_VALUE) {
        // Closing the handle releases the lock; unlocking first is not needed
        // and would leave a window where the file is open and unclaimed.
        ::CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }

#else
    if (fd_ >= 0) {
        // The claim goes with the last copy of the descriptor. The file itself
        // stays: removing it would let a second process create a new one and
        // lock a different inode while this one still believed it was alone.
        ::close(fd_);
        fd_ = -1;
    }
#endif
}

} // namespace utxoz::detail
