// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file census_arithmetic.hpp
 * @brief Arithmetic on figures that came out of a file, and the refusals.
 *
 * Separate from census.cpp for one reason: these checks fire only on inputs that
 * cannot be produced by a working store — a map claiming more entries than
 * buckets, a count too large to multiply. Forging such a file to reach them is
 * fragile and version-dependent, so they are reached directly instead. A check
 * nobody can observe is a check somebody deletes.
 *
 * The failure mode being guarded against is specific: unchecked arithmetic on
 * file data does not produce an obviously wrong number, it produces a plausibly
 * *small* one. A census exists to be believed, so it fails instead.
 */

#pragma once

#include <cstdint>
#include <limits>

#include <utxoz/types.hpp>

#include "detail/log.hpp"

namespace utxoz::detail {

/// Multiplication that refuses rather than wraps.
[[nodiscard]]
inline constexpr bool checked_mul(uint64_t a, uint64_t b, uint64_t& out) noexcept {
    if (a != 0 && b > std::numeric_limits<uint64_t>::max() / a) return false;
    out = a * b;
    return true;
}

/// Addition that refuses rather than wraps.
[[nodiscard]]
inline constexpr bool checked_add(uint64_t& accumulator, uint64_t value) noexcept {
    if (value > std::numeric_limits<uint64_t>::max() - accumulator) return false;
    accumulator += value;
    return true;
}

/// Accumulates a run of additions and remembers whether any could not be done.
/// One flag for a whole fold reads better than a check per field, and fails in
/// exactly the same cases.
struct summing {
    bool ok = true;
    constexpr void operator()(uint64_t& accumulator, uint64_t value) noexcept {
        if ( ! checked_add(accumulator, value)) ok = false;
    }
};

/// Boost's flat map keeps one 16-byte group descriptor per 15 slots. A model of
/// the certified layout, not a measurement — the same model `tools/sizing.cpp`
/// uses, so the instrument and the census cannot describe one table two ways.
///
/// Checked like every other figure derived from a file. A bucket count large
/// enough to overflow this cannot survive the multiplications the caller does
/// first, so today the check cannot fire — which is an argument about the order
/// of two functions, and the reader should not have to reconstruct it to trust
/// the number.
[[nodiscard]]
inline constexpr bool group_metadata_model(uint64_t bucket_count, uint64_t& out) noexcept {
    uint64_t groups = bucket_count / 15;
    return checked_add(groups, 1) && checked_mul(groups, 16, out);
}

// Evaluated at compile time, and not for the value: `constexpr` on a function
// whose body calls a non-constexpr one is accepted by GCC and Clang and rejected
// by MSVC, which is a build that fails on one platform and one platform only,
// after everything else has gone green. Asking for the answer here makes it a
// failure everywhere or nowhere.
static_assert([] {
    uint64_t bytes = 0;
    return group_metadata_model(122879, bytes) && bytes == 131072;
}(), "the group metadata model must be usable at compile time");
static_assert([] {
    uint64_t bytes = 0;
    return ! group_metadata_model(std::numeric_limits<uint64_t>::max(), bytes);
}(), "and it must refuse, at compile time, what it refuses at run time");

/**
 * @brief What a generation's own counts have to satisfy before anything is
 *        computed from them.
 *
 * A map cannot hold more entries than it has buckets. Nothing a working store
 * does produces that, which is exactly why it is worth saying: if it is ever
 * seen, the numbers about to be multiplied are not describing anything, and the
 * report built from them would be a confident description of nonsense.
 *
 * The class and the generation are logged. No key and no payload — a diagnostic
 * that ends up in an issue does not carry the chain.
 */
[[nodiscard]]
inline result<> validate_generation_counts(uint64_t container_class, uint64_t generation,
                                           uint64_t entries, uint64_t bucket_count) {
    if (entries > bucket_count) {
        log::error("census: class {} generation {} reports {} entries in {} buckets, "
                   "which cannot be", container_class, generation, entries, bucket_count);
        return std::unexpected(error_code::entry_corrupt);
    }
    return {};
}

/// The payload length an entry records, against what its class can hold.
///
/// The config and the stamp certify identity and layout; neither certifies that
/// an entry is internally consistent. Taking the minimum here would turn an
/// impossible length into a plausible one — the entry would be reported as full
/// and nothing would be said.
[[nodiscard]]
inline result<> validate_payload_length(uint64_t container_class, uint64_t generation,
                                        uint64_t recorded, uint64_t capacity) {
    if (recorded > capacity) {
        log::error("census: class {} generation {} holds an entry whose recorded "
                   "payload length is {}, and the class holds {}",
                   container_class, generation, recorded, capacity);
        return std::unexpected(error_code::entry_corrupt);
    }
    return {};
}

} // namespace utxoz::detail
