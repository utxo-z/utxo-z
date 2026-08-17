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
    /// specific version. Named for the walk rather than for lookups: deletions
    /// use the same seam, and the old name said otherwise.
    /// A specific version and not a blanket switch, because what has to be shown
    /// is a sweep that reads some files and then cannot read one — the case
    /// where a partial result exists and must not be handed back as absence. A
    /// blanket failure stops at the first file and never reaches it.
    static inline std::atomic<uint64_t> fail_historical_open_version{no_version};

    /// Fails a historical walk's attempt to enumerate which versions exist. A different
    /// failure from a file that will not open, reported with a different code,
    /// so the two cannot be shown to work by the same test.
    static inline std::atomic<bool> fail_historical_catalog{false};

    /// Sentinel meaning "never throw".
    ///
    /// The check below is `== ++applied_in_call`, so the first deletion tests
    /// against 1 and zero can never match — it is already unreachable as a
    /// setting. A distinct sentinel is still what the field is initialised to,
    /// rather than leaning on that.
    static constexpr uint64_t no_applied_count = ~uint64_t{0};

    /// Throws inside a deletion batch once this many deletions have been applied
    /// *by the historical walk*, between the map changing and the bookkeeping
    /// that follows it.
    ///
    /// The active-version phase is not counted: the seam is inside the per-file
    /// loop, so a deletion applied before the walk starts can never be the one
    /// that throws.
    ///
    /// A deletion writes as it walks, so the dangerous exception is not the one
    /// that stops a file from opening — fail_historical_open_version already covers
    /// that, and it fires before anything in the file has changed. It is the one
    /// raised after a key is gone from the map: dirty tracking, metadata, a
    /// vector growing. Without a seam that reaches that point, nothing shows
    /// whether an applied deletion can end up reported as still owed, or as
    /// neither applied nor owed.
    static inline std::atomic<uint64_t> fail_delete_after_applied{no_applied_count};

    /// Which catalogue a historical deletion updated, counted per call.
    ///
    /// Observable because the thing that went wrong is not: entry_count has no
    /// runtime consumer today — it is serialised and nothing reads it back
    /// during a run — and a historical version's metadata is not persisted after
    /// a deletion, so a deletion charged to the wrong catalogue produces
    /// identical answers, identical files and an identical database. The defect
    /// is real and silent, which is precisely why it needs a seam rather than a
    /// behavioural assertion.
    ///
    /// Same shape as sync_file_calls: counters the production path bumps and
    /// only a test looks at.
    static inline std::atomic<uint64_t> reference_metadata_deletes{0};
    static inline std::atomic<uint64_t> full_metadata_deletes{0};


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

    /// How many existing segments have been mapped.
    ///
    /// Incremented by `open_existing_segment`, which is the only place this store
    /// maps a segment it did not just create — so "was anything mapped?" is a
    /// question with an answer rather than an inference. A refusal that happens
    /// before any mapping leaves this untouched, and that is the observable
    /// behind the ordering the format barrier claims.
    ///
    /// A counter rather than a switch that fails: what has to be shown is that
    /// the edge was never reached, and a seam that fails when reached only shows
    /// what happens if it is.
    static inline std::atomic<uint64_t> segments_mapped{0};

    /// The two windows inside creating a version file, between the moment the
    /// file exists on disk and the moment the caller is told it does. Nothing
    /// else can reach them: the file is created, stamped and filled inside one
    /// call, and a failure part-way used to leave the name taken with nothing
    /// usable behind it — which a rotation could never then retry, because it
    /// would compute the same version number and find the name occupied.
    static inline std::atomic<bool> fail_after_segment_create{false};
    static inline std::atomic<bool> fail_after_segment_stamp{false};

    /// Makes the next `n` inserts each rotate before they store anything, by
    /// answering the safety check the way a container about to overflow does.
    ///
    /// Only the fixture generator uses it, and only to build a database with
    /// more than one generation without writing the hundred thousand entries a
    /// real rotation needs. It drives the ordinary rotation path — new_version()
    /// and everything under it — rather than fabricating a second file, because
    /// a fixture assembled by hand would attest to our ability to produce
    /// plausible bytes and not to what the writer actually writes.
    ///
    /// Internal, off by default, and not part of anything installed.
    static inline std::atomic<uint32_t> force_rotations{0};

    /// The capacity every new map is built with, overriding the policy.
    ///
    /// So that a test can put a small map inside a segment with room to spare —
    /// which is the only way to reach the compaction guard. At the ten-megabyte
    /// test profile the file is already too small for the next step, so
    /// `bad_alloc` refuses first and a guard that had been removed would look
    /// exactly like one that had not.
    ///
    /// Zero means the policy decides, which is every case but that one. Applied
    /// to one container only: the same capacity in every class would ask the
    /// 10240 class for a hundred and fifty megabytes inside a ten-megabyte file.
    static inline std::atomic<size_t> forced_capacity{0};
    static inline std::atomic<size_t> forced_capacity_index{0};

    /// The identity a database being created takes, instead of drawing one.
    ///
    /// Every database gets sixteen random bytes, so two runs of the generator
    /// produce different files even when everything else is identical — which
    /// makes "did this Boost write the same bytes?" unanswerable by comparison.
    /// Fixing the identity is what makes that question answerable at all.
    ///
    /// Internal, off by default, and used only by the fixture generator.
    static inline std::atomic<bool> force_database_id{false};
    static inline std::array<uint8_t, 16> forced_database_id{};

    /// Deletes the config file at the one moment that matters: after the
    /// directory claim has been taken and before the open decides whether a
    /// database is there.
    ///
    /// It pins an ordering; it does not simulate a crash. If "is there a database
    /// here" were asked *before* the claim — the shape a caller outside the
    /// library is forced into — a config that vanished in between would leave
    /// open-or-create making a new database over the answer already given. Armed,
    /// this makes that window the whole of the run, and open_existing still
    /// refuses because the question is asked inside the claim.
    static inline std::atomic<bool> delete_config_after_claim{false};

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
        fail_historical_open_version.store(no_version, std::memory_order_relaxed);
        fail_historical_catalog.store(false, std::memory_order_relaxed);
        fail_delete_after_applied.store(no_applied_count, std::memory_order_relaxed);
        reference_metadata_deletes.store(0, std::memory_order_relaxed);
        full_metadata_deletes.store(0, std::memory_order_relaxed);
        crash_at.store(crash_point::none, std::memory_order_relaxed);
        fail_directory_barrier_at.store(dir_barrier::none, std::memory_order_relaxed);
        fail_container_open.store(false, std::memory_order_relaxed);
        segments_mapped.store(0, std::memory_order_relaxed);
        fail_sidecar_removal.store(false, std::memory_order_relaxed);
        before_target_publish.store(nullptr, std::memory_order_relaxed);
        forced_merge_id.store(0, std::memory_order_relaxed);
        force_rotations.store(0, std::memory_order_relaxed);
        forced_capacity.store(0, std::memory_order_relaxed);
        forced_capacity_index.store(0, std::memory_order_relaxed);
        force_database_id.store(false, std::memory_order_relaxed);
        delete_config_after_claim.store(false, std::memory_order_relaxed);
        forced_database_id.fill(0);
    }

    /// Takes one forced rotation if any are pending, and says so.
    ///
    /// Both size-class and reference inserts consult this, and the decrement has
    /// to happen exactly once per rotation granted — two copies of that is two
    /// places for the counter to stop being consumed.
    static bool consume_forced_rotation() noexcept {
        auto pending = force_rotations.load(std::memory_order_relaxed);
        if (pending == 0) return false;
        force_rotations.store(pending - 1, std::memory_order_relaxed);
        return true;
    }

    /// Disarms every seam when it goes out of scope.
    ///
    /// Calling `clear()` at the end of a test is not enough. A failed REQUIRE
    /// leaves the scope by throwing, so the call is skipped and the next test
    /// runs with a seam still armed — which surfaces as a failure somewhere
    /// unrelated, or, for a seam that only makes something happen earlier, as a
    /// pass. Construct one of these before arming anything.
    struct scoped_reset {
        scoped_reset() = default;
        scoped_reset(scoped_reset const&) = delete;
        scoped_reset& operator=(scoped_reset const&) = delete;
        ~scoped_reset() { clear(); }
    };
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
