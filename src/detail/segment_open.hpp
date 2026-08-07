// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file segment_open.hpp
 * @brief The one way to open a version file that already exists.
 * @internal
 *
 * Boost, handed a mapped file whose header does not read as initialised,
 * assumes another process is midway through creating it and spins waiting for
 * that to finish — `BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC`,
 * five minutes by default — before reporting corruption.
 *
 * Nothing here ever waits on another process creating a segment. One instance
 * per database is the documented contract — though a precondition rather than a
 * guarantee: nothing currently excludes a second process, which is #71. Either
 * way this store never *waits* for one, so the wait is pure delay, and a
 * truncated or foreign file at a canonical version name would stall whatever
 * met it for minutes, once per file, before saying anything.
 *
 * Every opening of an existing version file goes through here, so the check is
 * a property of the store rather than of the call sites that remembered it.
 * There is deliberately no other `open_only` in `src/`.
 */

#pragma once

#include <algorithm>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/interprocess/managed_mapped_file.hpp>
#include <fmt/format.h>

#include <utxoz/types.hpp>

namespace utxoz::detail {

namespace bip = boost::interprocess;
namespace fs = std::filesystem;

/// The smallest version file any configuration in this build creates. Derived
/// rather than picked, so it follows the sizing constants if they change.
inline constexpr size_t smallest_configured_file = std::min({
    std::ranges::min(file_sizes),
    std::ranges::min(test_file_sizes),
    compact_file_size,
    compact_test_file_size,
});

/**
 * @brief Opens a version file that is expected to exist.
 *
 * @throws std::runtime_error if the file cannot be one this build wrote. Every
 *         caller already handles a failure to open by catching, so refusing
 *         this way needs no new path — it only makes the refusal immediate.
 */
[[nodiscard]]
inline std::unique_ptr<bip::managed_mapped_file> open_existing_segment(std::string const& path) {
    std::error_code ec;
    auto const size = fs::file_size(path, ec);
    if (ec) {
        throw std::runtime_error(fmt::format("{}: cannot be sized", path));
    }
    if (size < smallest_configured_file) {
        // Too small to be a segment this build wrote. Saying so costs one stat;
        // letting Boost find out costs minutes of spinning, because a header
        // that does not read as initialised is indistinguishable to it from one
        // another process is still writing.
        throw std::runtime_error(
            fmt::format("{}: {} bytes is too small to be a version file", path, size));
    }
    return std::make_unique<bip::managed_mapped_file>(bip::open_only, path.c_str());
}

} // namespace utxoz::detail
