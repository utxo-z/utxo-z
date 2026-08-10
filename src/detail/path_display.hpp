// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file path_display.hpp
 * @brief A path rendered for a human, without going through a code page.
 */

#pragma once

#include <filesystem>
#include <string>

namespace utxoz::detail {

namespace fs = std::filesystem;

/**
 * @brief A path as UTF-8 text, for messages and logs only.
 *
 * Not for reaching the filesystem. Every path this library opens travels as
 * `fs::path` and is handed to the OS in its native form; this exists because an
 * error message still has to name the file, and `path::string()` is the wrong
 * way to produce that name.
 *
 * `path::string()` converts through the active code page on Windows, which is
 * the conversion #109 is about — but in a diagnostic it fails worse than it does
 * in a filename. It *throws* `std::system_error` when a component will not fit
 * the code page, so the one line that was meant to explain the failure becomes a
 * second exception thrown from inside the handler for the first, and the
 * original error is lost. Building an error message must not be able to fail.
 *
 * `u8string()` cannot: every path the OS gave us is representable in UTF-8, and
 * the conversion is total in that direction. The result is copied into a
 * `std::string` because that is what the logging and exception types take; the
 * bytes are UTF-8 either way.
 */
[[nodiscard]]
inline std::string path_display(fs::path const& p) {
    auto const u8 = p.u8string();
    return std::string(reinterpret_cast<char const*>(u8.data()), u8.size());
}

} // namespace utxoz::detail
