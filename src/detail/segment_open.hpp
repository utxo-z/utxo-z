// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file segment_open.hpp
 * @brief The one way to open a version file that already exists, and the one way
 *        to reach an object inside it.
 * @internal
 *
 * Every opening of an existing version file goes through here, so the checks
 * and the failpoints are a property of the store rather than of the call sites
 * that remembered them. There is deliberately no other opening of an existing
 * segment in `src/`.
 *
 * Both functions report failure as a value, like the rest of the store. Nothing
 * here waits: a file that is not a segment is refused at its header, at once.
 */

#pragma once

#include <algorithm>
#include <ranges>
#include <filesystem>
#include <memory>
#include <new>
#include <string>

#include "mapped_segment.hpp"
#include <fmt/format.h>

#include <utxoz/types.hpp>

#include "capacity_policy.hpp"
#include "durability.hpp"
#include "log.hpp"
#include "path_display.hpp"

namespace utxoz::detail {

namespace fs = std::filesystem;

/// The smallest version file any configuration in this build creates. Derived
/// rather than picked, so it follows the sizing constants if they change.
///
/// Taken from the **per-slot** tables, not the per-class ones. A sharded class's
/// container holds a share of its class's file, so the smallest file this build
/// writes is no longer the smallest class entry — and reading it from the class
/// table makes this guard reject files the same build had just created. That is
/// not hypothetical: it is what the divided testing profile hit at eight shards,
/// where a container is 1 310 720 bytes and the smallest class entry is ten
/// megabytes. Production happened to be safe only because its shares stay large.
inline constexpr size_t smallest_configured_file = std::min({
    std::ranges::min(production_slot_capacity | std::views::transform(
        [](capacity_entry const& e) { return e.file_size; })),
    std::ranges::min(testing_slot_capacity | std::views::transform(
        [](capacity_entry const& e) { return e.file_size; })),
    reference_file_size,
    reference_test_file_size,
});

/**
 * @brief Opens a version file that is expected to exist.
 *
 * The three ways of failing — cannot be sized, too small to be one of ours,
 * it will not map as a segment — differ only to whoever reads the log afterwards, so
 * they share one error and are told apart there.
 */
[[nodiscard]]
inline result<std::unique_ptr<mapped_segment>> open_existing_segment(fs::path const& path) {
    std::error_code ec;
    auto const size = fs::file_size(path, ec);
    if (ec) {
        log::error("{}: cannot be sized", path_display(path));
        return std::unexpected(error_code::file_open_failed);
    }
    if (size < smallest_configured_file) {
        // Too small to be a segment this build wrote: said here, by its size,
        // rather than left for the header checks to describe less exactly.
        log::error("{}: {} bytes is too small to be a version file", path_display(path), size);
        return std::unexpected(error_code::file_open_failed);
    }
    if (auto const limit = failpoints::fail_segment_open_after.load(std::memory_order_relaxed);
        limit != 0
        && failpoints::segments_mapped.load(std::memory_order_relaxed) >= limit) {
        log::error("{}: failpoint: refusing to map after {} segments",
                   path_display(path), limit);
        return std::unexpected(error_code::file_open_failed);
    }

    auto mapped = mapped_segment::open(path);
    if ( ! mapped) return std::unexpected(mapped.error());
    // Counted at the edge itself, so a test can ask whether a refusal came
    // before any segment was mapped instead of inferring it -- and counted
    // after the mapping exists rather than before it is attempted, because the
    // question is about mappings, not about tries.
    failpoints::segments_mapped.fetch_add(1, std::memory_order_relaxed);
    return mapped;
}

/**
 * @brief The one way to reach an object inside a segment that already exists.
 *
 * A slot says where an object is and how many bytes were constructed there. It
 * says nothing about what type they were, and find<T>() hands back a T* either
 * way. Two things follow, and this function exists for both:
 *
 * 1. A version file that does not hold the object is not an empty version file.
 *    Every call site used to read the null and carry on -- `if ( ! map_ptr)
 *    continue;` -- so a file whose map could not be reached dropped out of an
 *    enumeration that still reported success, and dropped out of a *compaction*
 *    that then deleted it. Refusing here is what turns both into a failure the
 *    callers already know how to report.
 *
 * 2. find<T>() also returns how many instances of `T` the stored bytes make up:
 *    `bytes / sizeof(T)`. For an object written as one instance that is 1,
 *    unless `sizeof(T)` has changed since it was written.
 *
 * @warning This is a cardinality check, not a layout check, and the difference
 * matters. Integer division means the count still reads 1 for any `sizeof(T)`
 * satisfying `bytes / 2 < sizeof(T) <= bytes`. A layout that reorders fields
 * without changing the total passes too, and a change to the hash or to how it
 * is mixed relocates every key while leaving every size alone. None of that is
 * visible from here. The barrier for those is the format stamp and the
 * identities in format_identity.hpp.
 */
template <typename T>
[[nodiscard]]
inline result<T*> find_single_named(mapped_segment& segment,
                                    segment_slot slot,
                                    fs::path const& path) {
    auto const found = segment.find<T>(slot);

    if (found.first == nullptr) {
        log::error("{}: holds no object in slot {}", path_display(path), uint32_t(slot));
        return std::unexpected(error_code::version_unreadable);
    }
    if (found.second != 1) {
        log::error("{}: slot {} measures {} instances, not one", path_display(path),
                   uint32_t(slot), found.second);
        return std::unexpected(error_code::version_unreadable);
    }
    return found.first;
}

} // namespace utxoz::detail
