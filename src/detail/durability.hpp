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
#include <cstdint>
#include <cstdlib>
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
#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
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
    /// Separate from fail_sync_file: the page barrier and the file barrier
    /// cover different things and fail for different reasons, so a test that
    /// cannot drive them apart cannot show that both are wired.
    static inline std::atomic<bool> fail_sync_mapped_region{false};

    /// How many times the page barrier has been crossed. A test that needs to
    /// know a set of mappings was *visited* cannot learn it from a failure —
    /// the first one aborts the rest — so it counts instead.
    static inline std::atomic<uint64_t> sync_mapped_region_calls{0};

    /// How many times the file barrier has been crossed, for the same reason.
    static inline std::atomic<uint64_t> sync_file_calls{0};

    /// Fails the Nth file barrier and no other, counting from one. Blanket
    /// failure cannot reach past the first barrier a call performs, so it
    /// cannot show what happens when one in the middle fails — which is the
    /// case where a partial result could be recorded and must not be.
    static inline std::atomic<uint64_t> fail_sync_file_at{0};
    static inline std::atomic<bool> fail_sync_directory{false};
    static inline std::atomic<bool> fail_replace{false};
    static inline std::atomic<bool> fail_unlink{false};
    /// Fails only the removals that retire a merge's sources, which is the one
    /// place where a failure leaves several canonical files holding the same
    /// keys. A blanket unlink failure would stop the merge long before that.
    static inline std::atomic<bool> fail_source_unlink{false};

    /// Sentinel meaning "no version": versions are numbered from zero, so the
    /// switch below cannot use zero to mean both "version 0" and "off".
    /// Declared first so it is the initialiser rather than a repeat of it — two
    /// spellings of the same constant are two places to get it wrong.
    static constexpr uint64_t no_version = ~uint64_t{0};

    /// Fails a historical walk — resolution or deletion alike — as it opens one
    /// specific version.
    /// A specific version and not a blanket switch, because what has to be shown
    /// is a sweep that reads some files and then cannot read one — the case
    /// where a partial result exists and must not be handed back as absence. A
    /// blanket failure stops at the first file and never reaches it.
    static inline std::atomic<uint64_t> fail_lookup_open_version{no_version};

    /// Fails a historical walk's attempt to enumerate which versions exist. A different
    /// failure from a file that will not open, reported with a different code,
    /// so the two cannot be shown to work by the same test.
    static inline std::atomic<bool> fail_lookup_catalog{false};

    /// Sentinel meaning "never throw": zero is a real count — throw before
    /// applying anything — so it cannot also mean "off".
    static constexpr uint64_t no_applied_count = ~uint64_t{0};

    /// Throws inside a deletion batch once this many deletions have been applied
    /// within the call, between the map changing and the bookkeeping that
    /// follows it.
    ///
    /// A deletion writes as it walks, so the dangerous exception is not the one
    /// that stops a file from opening — fail_lookup_open_version already covers
    /// that, and it fires before anything in the file has changed. It is the one
    /// raised after a key is gone from the map: dirty tracking, metadata, a
    /// vector growing. Without a seam that reaches that point, nothing shows
    /// whether an applied deletion can end up reported as still owed, or as
    /// neither applied nor owed.
    static inline std::atomic<uint64_t> fail_delete_after_applied{no_applied_count};


    /// Where to stop the process dead, for the tests that check what a crash at
    /// each barrier leaves behind. Named after the step that has just finished.
    enum class crash_point : uint8_t {
        none,
        after_build,             ///< target written, nothing synced
        after_file_sync,         ///< contents durable, no sidecar yet
        after_sidecar_sync,      ///< sidecar written and synced, not yet named
        after_sidecar_publish,   ///< sidecar named; target still unpublished
        after_target_publish,    ///< target named; directory not yet synced
        before_source_unlink,    ///< everything published, nothing retired
        mid_source_unlink,       ///< some sources retired, some not
        after_sources_retired,   ///< sources gone, sidecar still there
    };

    static inline std::atomic<crash_point> crash_at{crash_point::none};

    /// Which directory barrier to fail. A merge crosses four of them and they
    /// guard different things, so a single switch that always fires on the
    /// first call can never reach the later ones — and the barrier after the
    /// target is published is the one whose failure loses data.
    enum class dir_barrier : uint8_t {
        none,
        after_sidecar,          ///< the record is named; nothing else has happened
        after_target,           ///< the target is named; no source retired yet
        after_source_retire,    ///< the sources are gone
        after_sidecar_removal,  ///< the record is gone
    };

    static inline std::atomic<dir_barrier> fail_directory_barrier_at{dir_barrier::none};

    /// Fails reopening the active container after a compaction.
    static inline std::atomic<bool> fail_container_open{false};

    /// Fails the removal of the sidecar, and the barrier that confirms it.
    static inline std::atomic<bool> fail_sidecar_removal{false};

    /// Runs after the merge record is durable and before the target takes its
    /// canonical name — the one window a test cannot reach from outside, and
    /// the one where another process could take the identity.
    static inline std::atomic<void (*)()> before_target_publish{nullptr};

    /// Makes a merge use a known identifier, so a test does not have to reason
    /// about real randomness to know what it should find. Zero means "draw one".
    static inline std::atomic<uint64_t> forced_merge_id{0};

    static void run_before_target_publish() {
        if (auto* hook = before_target_publish.load(std::memory_order_relaxed)) hook();
    }

    /// Dies the way a power cut does: no unwinding, no flushing, no destructors.
    static void maybe_crash(crash_point point) {
        if (crash_at.load(std::memory_order_relaxed) == point) {
            ::_exit(99);
        }
    }

    static void clear() noexcept {
        fail_sync_file.store(false, std::memory_order_relaxed);
        fail_sync_mapped_region.store(false, std::memory_order_relaxed);
        sync_mapped_region_calls.store(0, std::memory_order_relaxed);
        sync_file_calls.store(0, std::memory_order_relaxed);
        fail_sync_file_at.store(0, std::memory_order_relaxed);
        fail_sync_directory.store(false, std::memory_order_relaxed);
        fail_replace.store(false, std::memory_order_relaxed);
        fail_unlink.store(false, std::memory_order_relaxed);
        fail_source_unlink.store(false, std::memory_order_relaxed);
        fail_lookup_open_version.store(no_version, std::memory_order_relaxed);
        fail_lookup_catalog.store(false, std::memory_order_relaxed);
        fail_delete_after_applied.store(no_applied_count, std::memory_order_relaxed);
        crash_at.store(crash_point::none, std::memory_order_relaxed);
        fail_directory_barrier_at.store(dir_barrier::none, std::memory_order_relaxed);
        fail_container_open.store(false, std::memory_order_relaxed);
        fail_sidecar_removal.store(false, std::memory_order_relaxed);
        before_target_publish.store(nullptr, std::memory_order_relaxed);
        forced_merge_id.store(0, std::memory_order_relaxed);
    }
};

/**
 * @brief Flushes a mapping's dirty pages, synchronously.
 *
 * Boost's `managed_mapped_file::flush()` takes no arguments and maps to
 * `msync(MS_ASYNC)`, which schedules writeback and returns — useful for hinting,
 * useless as a barrier. This is the barrier.
 *
 * It covers the pages and nothing else. POSIX requires it before `fsync` for a
 * mapped region's stores to be guaranteed visible; on Linux the two share the
 * page cache and it is redundant, which is not a reason to omit it elsewhere.
 */
[[nodiscard]]
inline result<> sync_mapped_region(void* address, size_t length) {
    failpoints::sync_mapped_region_calls.fetch_add(1, std::memory_order_relaxed);

    if (failpoints::fail_sync_mapped_region.load(std::memory_order_relaxed)) {
        return std::unexpected(error_code::sync_failed);
    }
    if (address == nullptr || length == 0) return {};

#if defined(__EMSCRIPTEN__)
    return std::unexpected(error_code::sync_unsupported);
#elif defined(_WIN32)
    return ::FlushViewOfFile(address, length) != 0
        ? result<>{}
        : std::unexpected(error_code::sync_failed);
#else
    return ::msync(address, length, MS_SYNC) == 0
        ? result<>{}
        : std::unexpected(error_code::sync_failed);
#endif
}

/**
 * @brief Flushes a file's contents and metadata to stable storage.
 *
 * @return empty on success, `error_code::sync_unsupported` where the platform
 *         has no such operation, `error_code::sync_failed` if it failed.
 */
[[nodiscard]]
inline result<> sync_file(fs::path const& path) {
    auto const nth = failpoints::sync_file_calls.fetch_add(1, std::memory_order_relaxed) + 1;

    if (failpoints::fail_sync_file.load(std::memory_order_relaxed)) {
        return std::unexpected(error_code::sync_failed);
    }
    if (auto const target = failpoints::fail_sync_file_at.load(std::memory_order_relaxed);
        target != 0 && nth == target) {
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

/**
 * @brief Moves `from` to `to`, failing if `to` already exists.
 *
 * For publishing a file under an identity that has never been used. The
 * distinction from `replace_file_atomically` is the whole point.
 *
 * A merge checks that the name is free before it starts. That check would be
 * conclusive if one instance were guaranteed to be the only one touching the
 * database — but nothing enforces that today; it is a documented precondition
 * and no more, and giving it teeth is #71. So the check is treated as what it
 * is, a cheap early answer, and the publication itself refuses rather than
 * replaces. A name taken in between is then caught with the file that took it
 * intact, instead of destroyed.
 *
 * POSIX has no portable rename-if-absent (`RENAME_NOREPLACE` is Linux-only), so
 * this uses `link` — which fails with EEXIST and is atomic — followed by
 * unlinking the source. A crash between the two leaves the destination
 * published, which is the intended outcome, and the source as a stray in the
 * reserved namespace, which recovery removes.
 *
 * @warning `link` needs both names on one filesystem and a filesystem that
 * supports hard links at all. Both hold for the caller here, which puts its
 * temporary beside the target in the same directory. Where they do not — a
 * filesystem that answers `EPERM`, or a database directory that is somehow a
 * mount boundary away from itself — this returns `rename_failed` and that
 * compaction simply does not happen. The database is untouched, which is the
 * right outcome for a merge that cannot be published safely.
 */
[[nodiscard]]
inline result<> publish_new_file(fs::path const& from, fs::path const& to) {
    if (failpoints::fail_replace.load(std::memory_order_relaxed)) {
        return std::unexpected(error_code::rename_failed);
    }

#if defined(_WIN32)
    // Without MOVEFILE_REPLACE_EXISTING this fails when the destination is there.
    if (::MoveFileExW(from.wstring().c_str(), to.wstring().c_str(), 0) != 0) return {};

    // Captured once: any call in between, including one the runtime makes,
    // would overwrite it.
    DWORD const last_error = ::GetLastError();
    return std::unexpected(last_error == ERROR_ALREADY_EXISTS || last_error == ERROR_FILE_EXISTS
                           ? error_code::identity_collision
                           : error_code::rename_failed);
#else
    if (::link(from.c_str(), to.c_str()) != 0) {
        int const err = errno;
        return std::unexpected(err == EEXIST ? error_code::identity_collision
                                             : error_code::rename_failed);
    }
    if (::unlink(from.c_str()) != 0) {
        // Published, and a stray left behind. Recovery removes it; reporting a
        // failure here would say the publication did not happen.
        return {};
    }
    return {};
#endif
}

/// Removes a path, with a seam for the tests that need it to fail.
[[nodiscard]]
inline result<> remove_file(fs::path const& path) {
    if (failpoints::fail_unlink.load(std::memory_order_relaxed)) {
        return std::unexpected(error_code::removal_failed);
    }
    std::error_code ec;
    fs::remove(path, ec);
    return ec ? result<>(std::unexpected(error_code::removal_failed)) : result<>{};
}

} // namespace utxoz::detail
