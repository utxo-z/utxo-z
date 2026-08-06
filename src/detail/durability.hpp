// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file durability.hpp
 * @brief Getting bytes to stable storage, replacing a file atomically, and
 *        saying honestly what the platform can actually do.
 * @internal
 *
 * Three operations are needed to make a file durable and they cover different
 * things: flushing the mapped pages of a mapping, flushing the file itself
 * (its size and inode metadata, and anything written outside a mapping), and
 * flushing the directory entry that names it. A rename is not durable until the
 * directory is.
 *
 * Not every platform offers all three, and a missing one is reported rather
 * than papered over. A call that silently succeeds without doing anything is
 * how a store ends up promising durability it does not have.
 */

#pragma once

#include <atomic>
#include <filesystem>

#include <utxoz/types.hpp>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace utxoz::detail {

namespace fs = std::filesystem;

/**
 * @brief What this platform can actually promise.
 *
 * - `full`: file contents and directory entries can both be made durable.
 * - `file_only`: file contents can; directory entries have no exposed barrier,
 *   so the ordering between a rename and the data it publishes is weaker than
 *   POSIX gives and must be documented as such rather than assumed equivalent.
 * - `none`: the filesystem is virtual and nothing here means anything.
 *
 * Callers that make a durability promise consult this rather than inferring it
 * from a call that returned success.
 */
enum class sync_support { full, file_only, none };

[[nodiscard]]
inline constexpr sync_support platform_sync_support() noexcept {
#if defined(__EMSCRIPTEN__)
    return sync_support::none;
#elif defined(_WIN32)
    return sync_support::file_only;
#else
    return sync_support::full;
#endif
}

/**
 * @brief Test-only switches that make a barrier or a replace fail on demand.
 *
 * A durability protocol is mostly error paths, and an error path nothing can
 * reach is an error path nobody has checked. These are the seams that let a
 * test reach them. They live in an internal header that is never installed, and
 * cost one relaxed load on operations that already involve the disk.
 */
struct failpoints {
    static inline std::atomic<bool> fail_sync_file{false};
    static inline std::atomic<bool> fail_sync_directory{false};
    static inline std::atomic<bool> fail_replace{false};

    static void clear() noexcept {
        fail_sync_file.store(false, std::memory_order_relaxed);
        fail_sync_directory.store(false, std::memory_order_relaxed);
        fail_replace.store(false, std::memory_order_relaxed);
    }
};

/**
 * @brief Flushes a file's contents and metadata to stable storage.
 *
 * @return empty on success, `error_code::sync_unsupported` where the platform
 *         has no such operation, `error_code::sync_failed` if it failed.
 */
[[nodiscard]]
inline result<> sync_file(fs::path const& path) {
    if (failpoints::fail_sync_file.load(std::memory_order_relaxed)) {
        return std::unexpected(error_code::sync_failed);
    }

#if defined(__EMSCRIPTEN__)
    (void)path;
    // The filesystem is virtual. fsync() exists in the POSIX layer and returns
    // success without doing anything, which is worse than not having it: the
    // caller cannot tell. Say so instead.
    return std::unexpected(error_code::sync_unsupported);
#elif defined(_WIN32)
    HANDLE const h = ::CreateFileW(path.wstring().c_str(), GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return std::unexpected(error_code::sync_failed);

    bool const ok = ::FlushFileBuffers(h) != 0;
    ::CloseHandle(h);
    return ok ? result<>{} : std::unexpected(error_code::sync_failed);
#else
    int const fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) return std::unexpected(error_code::sync_failed);

    int const rc = ::fsync(fd);
    ::close(fd);
    return rc == 0 ? result<>{} : std::unexpected(error_code::sync_failed);
#endif
}

/**
 * @brief Flushes a directory's entries, so a rename or unlink in it is durable.
 *
 * Windows exposes no equivalent: a directory handle cannot be flushed in a way
 * that is specified to persist its entries. That is reported as unsupported,
 * not approximated.
 */
[[nodiscard]]
inline result<> sync_directory(fs::path const& path) {
    if (failpoints::fail_sync_directory.load(std::memory_order_relaxed)) {
        return std::unexpected(error_code::sync_failed);
    }

#if defined(__EMSCRIPTEN__) || defined(_WIN32)
    (void)path;
    return std::unexpected(error_code::sync_unsupported);
#else
    int const fd = ::open(path.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd < 0) return std::unexpected(error_code::sync_failed);

    int const rc = ::fsync(fd);
    ::close(fd);
    return rc == 0 ? result<>{} : std::unexpected(error_code::sync_failed);
#endif
}

/**
 * @brief Moves `from` onto `to`, replacing `to` if it is there, atomically.
 *
 * Atomic means a reader sees the old file or the new one and never neither —
 * which rules out removing the destination first, since that leaves a window
 * with nothing at the name.
 *
 * `std::filesystem::rename` is not enough. POSIX `rename(2)` replaces an
 * existing destination, and on POSIX that is exactly what the standard library
 * calls; on Windows it maps to a move that fails when the destination exists.
 * So the two cases are separated here: `ReplaceFileW` when there is something
 * to replace, `MoveFileExW` when there is not.
 */
[[nodiscard]]
inline result<> replace_file_atomically(fs::path const& from, fs::path const& to) {
    if (failpoints::fail_replace.load(std::memory_order_relaxed)) {
        return std::unexpected(error_code::rename_failed);
    }

#if defined(_WIN32)
    std::error_code ec;
    bool const destination_exists = fs::exists(to, ec);
    if (ec) return std::unexpected(error_code::rename_failed);

    if (destination_exists) {
        // ReplaceFileW is the atomic replace: the destination keeps its
        // identity and the swap is not observable as an absence.
        if (::ReplaceFileW(to.wstring().c_str(), from.wstring().c_str(),
                           nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr) == 0) {
            return std::unexpected(error_code::rename_failed);
        }
        return {};
    }

    // Nothing to replace: a plain move, still atomic with respect to the name.
    if (::MoveFileExW(from.wstring().c_str(), to.wstring().c_str(),
                      MOVEFILE_REPLACE_EXISTING) == 0) {
        return std::unexpected(error_code::rename_failed);
    }
    return {};
#else
    std::error_code ec;
    fs::rename(from, to, ec);
    return ec ? result<>(std::unexpected(error_code::rename_failed)) : result<>{};
#endif
}

} // namespace utxoz::detail
