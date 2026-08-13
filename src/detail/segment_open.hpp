// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file segment_open.hpp
 * @brief The one way to open a version file that already exists, and the one way
 *        to reach an object inside it.
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
 *
 * Both functions report failure as a value. Mapping a file is the one operation
 * in this path that raises, so it is caught here, once, and everything above
 * reads as `result<>` like the rest of the store.
 */

#pragma once

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

#include <boost/interprocess/managed_mapped_file.hpp>
#include <fmt/format.h>

#include <utxoz/types.hpp>

#include "log.hpp"
#include "path_display.hpp"

namespace utxoz::detail {

namespace bip = boost::interprocess;
namespace fs = std::filesystem;

// The cap on how long Boost waits for a segment somebody else appears to be
// creating. It is a compile definition on the library target, and it is baked
// into open_existing_segment() below — an inline function, so a translation unit
// that included this header without it would give the program a second,
// different definition of the same function. The symptom is not a diagnostic:
// the linker keeps one, and whichever it keeps decides whether the wait is ten
// seconds or five minutes, for everybody.
//
// That is not hypothetical. tests/test_named_object_guard.cpp began including
// this header, the test target did not carry the definition, and the macOS and
// Windows runners started waiting Boost's full 300 seconds while Linux happened
// to link the other copy and stayed green.
static_assert(BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC == 10u,
              "every target that includes this header must define "
              "BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC=10u; "
              "without it this file is a second, slower definition of open_existing_segment()");

/// The smallest version file any configuration in this build creates. Derived
/// rather than picked, so it follows the sizing constants if they change.
inline constexpr size_t smallest_configured_file = std::min({
    std::ranges::min(file_sizes),
    std::ranges::min(test_file_sizes),
    reference_file_size,
    reference_test_file_size,
});

/**
 * @brief Opens a version file that is expected to exist.
 *
 * The three ways of failing — cannot be sized, too small to be one of ours,
 * Boost will not map it — differ only to whoever reads the log afterwards, so
 * they share one error and are told apart there.
 */
[[nodiscard]]
inline result<std::unique_ptr<bip::managed_mapped_file>> open_existing_segment(fs::path const& path) {
    std::error_code ec;
    auto const size = fs::file_size(path, ec);
    if (ec) {
        log::error("{}: cannot be sized", path_display(path));
        return std::unexpected(error_code::file_open_failed);
    }
    if (size < smallest_configured_file) {
        // Too small to be a segment this build wrote. Saying so costs one stat;
        // letting Boost find out costs minutes of spinning, because a header
        // that does not read as initialised is indistinguishable to it from one
        // another process is still writing.
        log::error("{}: {} bytes is too small to be a version file", path_display(path), size);
        return std::unexpected(error_code::file_open_failed);
    }
    try {
        return std::make_unique<bip::managed_mapped_file>(bip::open_only, path.c_str());
    } catch (std::exception const& e) {
        log::error("{}: will not open as a segment: {}", path_display(path), e.what());
        return std::unexpected(error_code::file_open_failed);
    }
}

/**
 * @brief The one way to reach a named object inside a segment that already exists.
 *
 * Boost.Interprocess resolves a named object by **name alone**. Its
 * `priv_generic_find<T>` looks the name up in the index, checks nothing about the
 * type beyond a `BOOST_ASSERT` that vanishes in release builds, and hands back
 * `static_cast<T*>` of the stored address. Two things follow, and this function
 * exists for both:
 *
 * 1. A version file that does not hold the object is not an empty version file.
 *    Every call site used to read the null and carry on — `if ( ! map_ptr)
 *    continue;` — so a file whose map could not be reached dropped out of an
 *    enumeration that still reported success, and dropped out of a *compaction*
 *    that then deleted it. Refusing here is what turns both into a failure the
 *    callers already know how to report.
 *
 * 2. `find<T>()` also returns how many instances of `T` the stored bytes make up:
 *    `value_bytes / sizeof(T)`. For an object written as one instance that is 1,
 *    unless `sizeof(T)` has changed since it was written. Checking it costs
 *    nothing and was being discarded.
 *
 * @warning This is a cardinality check, not a layout check, and the difference
 * matters. Integer division means the count still reads 1 for any `sizeof(T)`
 * satisfying `value_bytes / 2 < sizeof(T) <= value_bytes` — with today's
 * `sizeof(utxo_map<48>) == 56`, anything from 29 to 56 bytes passes. A layout
 * that reorders fields without changing the total passes too, and a change to
 * the hash or to how it is mixed relocates every key while leaving every size
 * alone. None of that is visible from here. The barrier for those is an explicit
 * format epoch validated before any segment is touched, plus fixtures written by
 * earlier builds; this closes only the cases that happen to be free.
 */
template <typename T>
[[nodiscard]]
inline result<T*> find_single_named(bip::managed_mapped_file& segment,
                                    char const* name,
                                    fs::path const& path) {
    auto const found = segment.find<T>(name);

    if (found.first == nullptr) {
        log::error("{}: holds no object named '{}'", path_display(path), name);
        return std::unexpected(error_code::version_unreadable);
    }
    if (found.second != 1) {
        log::error("{}: '{}' measures {} instances, not one", path_display(path), name,
                   found.second);
        return std::unexpected(error_code::version_unreadable);
    }
    return found.first;
}

} // namespace utxoz::detail
