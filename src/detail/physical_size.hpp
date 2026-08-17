// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file physical_size.hpp
 * @brief How much disk a file was actually given, where the platform will say.
 *
 * A version file is created at its full configured size and then written into
 * gradually, so on a filesystem with sparse files the size it reports and the
 * space it occupies are different numbers — and for deciding a geometry the
 * second one is the one that costs money. This asks for it.
 *
 * **The platforms do not measure the same thing, and the answer says which was
 * used.** On POSIX, `st_blocks × 512` is the blocks the filesystem allocated:
 * a sparse file reports fewer than its length, which is exactly the intent. On
 * Windows, `GetCompressedFileSize` documents that it returns the compressed or
 * sparse size *only* where the volume supports it and the file is compressed or
 * sparse — otherwise it returns the same value as the logical size. So a Windows
 * number equal to the file size is not evidence that nothing is sparse; it may
 * be the API declining to distinguish. That is why the method travels with the
 * figure and why the two are never presented as the same measurement.
 *
 * `st_blocks` is in 512-byte units by POSIX definition regardless of the
 * filesystem's own block size, which is what makes the multiplication right.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include <utxoz/census.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

namespace utxoz::detail {

namespace fs = std::filesystem;

/// The blocks a file occupies, or a reason there is no such number here.
[[nodiscard]]
inline optional_bytes physical_allocation_of(fs::path const& path) {
#ifdef _WIN32
    ULARGE_INTEGER size{};
    // Cleared first, because INVALID_FILE_SIZE is also a legitimate low DWORD for
    // a file of exactly that many bytes. The documented way to tell the two apart
    // is to clear the error, call, and ask again — without the clear, a stale
    // error from anywhere earlier in the process would read as this call failing.
    ::SetLastError(NO_ERROR);
    DWORD const low = ::GetCompressedFileSizeW(path.c_str(), &size.HighPart);
    if (low == INVALID_FILE_SIZE) {
        DWORD const err = ::GetLastError();
        if (err != NO_ERROR) {
            return {0, metric_status::unavailable,
                    "GetCompressedFileSize failed with error " + std::to_string(err)};
        }
    }
    size.LowPart = low;
    return {size.QuadPart, metric_status::measured,
            "GetCompressedFileSize: the size on disk where the volume distinguishes it, "
            "and the logical size where it does not"};
#else
    struct stat st {};
    if (::stat(path.string().c_str(), &st) != 0) {
        return {0, metric_status::unavailable, "stat() failed"};
    }
    return {static_cast<uint64_t>(st.st_blocks) * 512u, metric_status::measured,
            "st_blocks x 512: blocks the filesystem allocated, so a sparse file "
            "reports less than its length"};
#endif
}

/// Which method `physical_allocation_of` uses on this build.
[[nodiscard]]
inline constexpr allocation_method physical_allocation_method() noexcept {
#ifdef _WIN32
    return allocation_method::windows_compressed_size;
#else
    return allocation_method::posix_st_blocks;
#endif
}

} // namespace utxoz::detail
