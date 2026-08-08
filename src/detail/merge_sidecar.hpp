// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file merge_sidecar.hpp
 * @brief The record that says a merge is in flight, and what it supersedes.
 * @internal
 *
 * Compaction builds a new version file from several old ones. Between the
 * moment the new file becomes visible and the moment the old ones are gone,
 * the same keys are in more than one canonical file. The sidecar is what makes
 * that state recoverable rather than merely brief: it names the target and the
 * sources, so a recovery that finds it knows which files are redundant and can
 * finish retiring them without reading either side.
 *
 * It is published before the target, never after. Reversed, a crash between the
 * two would leave the target and the sources all canonical with nothing
 * recording that the sources are superseded — duplicates, visible, with no
 * evidence to resolve them.
 *
 * A sidecar that cannot be fully validated is not weak evidence, it is an
 * error. Guessing what was in flight is how a recovery destroys a file it
 * should have kept.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <expected>
#include <filesystem>
#include <fstream>
#include <limits>
#include <atomic>
#include <span>
#include <vector>

#include <utxoz/types.hpp>

#include "durability.hpp"
#include "system_entropy.hpp"

namespace utxoz::detail {

namespace fs = std::filesystem;

/// Why a sidecar did not yield a plan.
enum class sidecar_read_error : uint8_t {
    absent,      ///< No file. Nothing was in flight.
    unreadable,  ///< There and could not be read.
    invalid,     ///< There and cannot be trusted: marker, version, checksum, or incoherent.
};

/**
 * @brief Ties a sidecar to the file it was written for.
 *
 * A sidecar names a target by its identity, and an identity is only a number.
 * Recovery finding a sidecar and a file at that number is not enough to
 * conclude the file is the one the sidecar meant: nothing locks the database
 * against a second process, so between the check that the identity was free and
 * the moment it was taken, someone else could have taken it. Retiring the
 * sources on that assumption would delete the only copies of their entries.
 *
 * So the sidecar carries an identifier and the target carries the same one,
 * written into it before its contents were made durable. Recovery compares the
 * two, which is one lookup and no reading of payloads. The marker stays in the
 * target for good — removing it would cost another barrier and buy nothing.
 */
struct merge_marker {
    /// The name it is stored under inside the target's segment.
    static constexpr char const* object_name = "utxoz_merge_id";

    uint64_t high = 0;
    uint64_t low = 0;

    [[nodiscard]]
    friend bool operator==(merge_marker const&, merge_marker const&) = default;

    [[nodiscard]]
    bool empty() const noexcept { return high == 0 && low == 0; }
};

/**
 * @brief A fresh identifier, distinct from every other one.
 *
 * It has to be unique, not unguessable — nothing here defends against someone
 * choosing a colliding value deliberately. But uniqueness has to be real, so
 * the bytes come from the operating system's cryptographic generator, named
 * directly (see system_entropy.hpp). `std::random_device` is not used: the
 * standard permits it to be a deterministic engine and implementations have
 * shipped that.
 *
 * There is no fallback. The clock and the process id are not alternatives —
 * two containers started together share a second and can share a pid, which is
 * exactly the situation this identifier exists to make safe. If the system will
 * not produce entropy the merge does not start, and the caller finds out before
 * anything has been written.
 *
 * Tests set `failpoints::forced_merge_id` to make a merge reproducible without
 * having to reason about real randomness.
 */
[[nodiscard]]
inline result<merge_marker> generate_merge_id() {
    if (auto const forced = failpoints::forced_merge_id.load(std::memory_order_relaxed);
        forced != 0) {
        return merge_marker{forced, ~forced};
    }

    merge_marker id;
    static_assert(sizeof(id.high) + sizeof(id.low) == 16, "the identifier is 128 bits");

    if (auto const drawn = system_entropy(&id.high, sizeof(id.high)); ! drawn) return std::unexpected(drawn.error());
    if (auto const drawn = system_entropy(&id.low, sizeof(id.low)); ! drawn) return std::unexpected(drawn.error());

    // Zero is the "no marker" answer. Drawing it is astronomically unlikely and
    // would be indistinguishable from a file that carries no marker at all, so
    // it is refused rather than patched into something else.
    if (id.empty()) return std::unexpected(error_code::entropy_unavailable);
    return id;
}

/// What a merge intends to do. Validated as a whole before it counts.
struct merge_plan {
    static constexpr std::array<char, 4> magic{'U', 'Z', 'M', 'G'};

    /// Version 2 carries the merge identifier.
    ///
    /// Version 1 did not, and this build refuses it rather than reading it: a
    /// recovery that trusted a v1 record would be trusting an identity match
    /// alone, which is the thing the identifier exists to stop. Refusing means
    /// the database does not open until someone looks at it.
    ///
    /// That is a deliberate choice and it has a cost worth stating plainly. A
    /// database left mid-merge by a build older than this one will not open —
    /// it stops with error_code::recovery_failed and a log line naming the
    /// record.
    ///
    /// @warning A version 1 record must **not** simply be deleted, and nothing
    /// here deletes one. Without an identifier the store cannot tell which
    /// state that record belongs to: the target may never have been published,
    /// or it may be published with every source still there, or with some of
    /// them already retired. Only the first is abandoned safely by removing the
    /// record; in the others the record is the sole evidence of what supersedes
    /// what, and removing it leaves duplicates with nothing left to resolve
    /// them. Recovering such a database calls for inspection, and a merge may
    /// be abandoned by hand only after establishing that the canonical target
    /// does not exist and that every source named in the record is still there.
    ///
    /// The alternative — reading a v1 record and retiring sources on an
    /// identity match — can delete the only copy of a live entry, so it is not
    /// on the table. This lands in an unreleased version, so the population it
    /// can affect is developers with a test directory from an earlier build.
    static constexpr uint16_t current_format = 2;

    /// A merge never combines more files than this, so a count above it is
    /// evidence of damage rather than an unusual plan.
    static constexpr size_t max_sources = 64;

    size_t container = 0;             ///< Container index, or the reference sentinel.
    size_t target = 0;                ///< The new identity being published.
    merge_marker id;                  ///< What the target must carry to be the one meant.
    std::vector<size_t> sources;      ///< The identities it supersedes, ascending.

    [[nodiscard]]
    bool supersedes(size_t version) const {
        return std::ranges::find(sources, version) != sources.end();
    }
};

namespace sidecar_detail {

[[nodiscard]]
inline uint32_t checksum(std::span<uint8_t const> bytes) noexcept {
    uint32_t hash = 2166136261u;
    for (auto const b : bytes) {
        hash ^= b;
        hash *= 16777619u;
    }
    return hash;
}

template <typename T>
void put(std::vector<uint8_t>& out, T const& value) {
    auto const* src = reinterpret_cast<uint8_t const*>(&value);
    out.insert(out.end(), src, src + sizeof(T));
}

template <typename T>
void get(uint8_t const*& cursor, T& value) {
    std::memcpy(&value, cursor, sizeof(T));
    cursor += sizeof(T);
}

/// magic + format + reserved + container + target + merge id + count, then the
/// sources, then the checksum. The identifier is inside the checksummed region:
/// a marker that could be altered without detection would be no marker at all.
[[nodiscard]]
inline constexpr size_t header_size() {
    return 4 + 2 + 2 + 8 + 8 + 16 + 8;
}

[[nodiscard]]
inline constexpr size_t encoded_size_for(size_t source_count) {
    return header_size() + source_count * 8 + 4;
}

} // namespace sidecar_detail

static_assert(std::endian::native == std::endian::little,
              "the merge sidecar is little-endian; a big-endian port must convert "
              "and bump merge_plan::current_format");

[[nodiscard]]
inline std::vector<uint8_t> encode_merge_plan(merge_plan const& plan) {
    using namespace sidecar_detail;

    std::vector<uint8_t> out;
    out.reserve(encoded_size_for(plan.sources.size()));

    out.insert(out.end(), merge_plan::magic.begin(), merge_plan::magic.end());
    put(out, merge_plan::current_format);
    put(out, uint16_t{0});   // reserved
    put(out, uint64_t(plan.container));
    put(out, uint64_t(plan.target));
    put(out, plan.id.high);
    put(out, plan.id.low);
    put(out, uint64_t(plan.sources.size()));
    for (auto const s : plan.sources) put(out, uint64_t(s));
    put(out, checksum(std::span<uint8_t const>(out)));

    return out;
}

/**
 * @brief Parses and fully validates a plan.
 *
 * Coherence is part of validation, not a separate courtesy: a plan that lists
 * its own target among its sources would have recovery unlink the file it just
 * published.
 */
[[nodiscard]]
inline std::expected<merge_plan, sidecar_read_error>
decode_merge_plan(std::span<uint8_t const> bytes) {
    using namespace sidecar_detail;

    if (bytes.size() < header_size() + 4) return std::unexpected(sidecar_read_error::invalid);
    if ( ! std::equal(merge_plan::magic.begin(), merge_plan::magic.end(),
                      reinterpret_cast<char const*>(bytes.data()))) {
        return std::unexpected(sidecar_read_error::invalid);
    }

    auto const* cursor = bytes.data() + merge_plan::magic.size();

    uint16_t format = 0;
    uint16_t reserved = 0;
    get(cursor, format);
    get(cursor, reserved);
    if (format != merge_plan::current_format) return std::unexpected(sidecar_read_error::invalid);
    if (reserved != 0) return std::unexpected(sidecar_read_error::invalid);

    uint64_t container = 0;
    uint64_t target = 0;
    merge_marker id;
    uint64_t count = 0;
    get(cursor, container);
    get(cursor, target);
    get(cursor, id.high);
    get(cursor, id.low);
    get(cursor, count);

    if (count == 0 || count > merge_plan::max_sources) {
        return std::unexpected(sidecar_read_error::invalid);
    }
    if (bytes.size() != encoded_size_for(size_t(count))) {
        return std::unexpected(sidecar_read_error::invalid);
    }

    // The record stores identities as 64-bit values and size_t is not always
    // that wide — on wasm32 it is half of it. A value that does not fit must be
    // refused *before* the conversion: truncating it produces a different
    // number that may well be a plausible identity, and every check below would
    // then be validating something the record never said.
    auto fits = [](uint64_t value) {
        if constexpr (sizeof(size_t) < sizeof(uint64_t)) {
            return value <= uint64_t(std::numeric_limits<size_t>::max());
        } else {
            return true;
        }
    };

    if ( ! fits(container) || ! fits(target)) {
        return std::unexpected(sidecar_read_error::invalid);
    }

    merge_plan plan;
    plan.container = size_t(container);
    plan.target = size_t(target);
    plan.id = id;
    plan.sources.reserve(size_t(count));
    for (uint64_t i = 0; i < count; ++i) {
        uint64_t source = 0;
        get(cursor, source);
        if ( ! fits(source)) return std::unexpected(sidecar_read_error::invalid);
        plan.sources.push_back(size_t(source));
    }

    uint32_t stored = 0;
    get(cursor, stored);
    if (checksum(bytes.subspan(0, bytes.size() - sizeof(uint32_t))) != stored) {
        return std::unexpected(sidecar_read_error::invalid);
    }

    // Coherent, not merely intact.
    if (plan.id.empty()) return std::unexpected(sidecar_read_error::invalid);
    if (plan.supersedes(plan.target)) return std::unexpected(sidecar_read_error::invalid);
    if (std::ranges::adjacent_find(plan.sources) != plan.sources.end()) {
        return std::unexpected(sidecar_read_error::invalid);   // duplicate source
    }
    if ( ! std::ranges::is_sorted(plan.sources)) {
        return std::unexpected(sidecar_read_error::invalid);
    }

    return plan;
}

[[nodiscard]]
inline std::expected<merge_plan, sidecar_read_error> read_merge_sidecar(fs::path const& path) {
    std::error_code ec;
    auto const status = fs::status(path, ec);
    if (status.type() == fs::file_type::not_found) {
        return std::unexpected(sidecar_read_error::absent);
    }
    if (ec || ! fs::is_regular_file(status)) {
        return std::unexpected(sidecar_read_error::unreadable);
    }

    auto const size = fs::file_size(path, ec);
    if (ec) return std::unexpected(sidecar_read_error::unreadable);

    // Bounded by the largest plan that could ever be legitimate.
    if (size > sidecar_detail::encoded_size_for(merge_plan::max_sources)) {
        return std::unexpected(sidecar_read_error::invalid);
    }

    std::ifstream ifs(path, std::ios::binary);
    if ( ! ifs) return std::unexpected(sidecar_read_error::unreadable);

    std::vector<uint8_t> buffer(size_t(size), 0);
    if (size > 0) {
        ifs.read(reinterpret_cast<char*>(buffer.data()), std::streamsize(size));
        if (ifs.gcount() != std::streamsize(size)) {
            return std::unexpected(sidecar_read_error::unreadable);
        }
    }

    return decode_merge_plan(buffer);
}

/// How far publishing a sidecar got.
///
/// The two failures on either side of the rename are not the same failure, and
/// a plain `result<>` cannot tell them apart. Before the rename, nothing was
/// named and the merge can be abandoned with no trace. After it, the name is
/// there — and if the barrier that was meant to make it durable failed, whether
/// it survives a crash is unknown. That is not "it did not publish"; it is
/// "nobody can say", and the two call for opposite responses.
struct sidecar_publication {
    /// The sidecar reached its final name and is visible now.
    bool named = false;
    /// And the directory barrier confirmed it. When false with `named` true,
    /// the state is uncertain and the caller must stop rather than continue.
    bool durable = false;
};

/**
 * @brief Publishes a sidecar durably: temp, sync, atomic replace, sync directory.
 *
 * Unlike a metadata record, this one is a promise. It is the only thing that
 * will tell a later open that the sources are redundant, so it is written with
 * the barriers.
 *
 * @return an error if it failed before the rename, in which case nothing was
 *         named; otherwise how far it got.
 */
[[nodiscard]]
inline result<sidecar_publication> write_merge_sidecar(fs::path const& path, merge_plan const& plan) {
    auto const temp_path = fs::path(path).concat(".tmp");
    auto const encoded = encode_merge_plan(plan);

    auto discard_temp = [&] {
        std::error_code cleanup;
        fs::remove(temp_path, cleanup);
    };

    {
        std::ofstream ofs(temp_path, std::ios::binary | std::ios::trunc);
        if ( ! ofs) return std::unexpected(error_code::metadata_write_failed);

        ofs.write(reinterpret_cast<char const*>(encoded.data()), std::streamsize(encoded.size()));
        if ( ! ofs) { ofs.close(); discard_temp(); return std::unexpected(error_code::metadata_write_failed); }

        ofs.close();
        if (ofs.fail()) { discard_temp(); return std::unexpected(error_code::metadata_write_failed); }
    }

    if (auto const synced = sync_file(temp_path);
        ! synced && synced.error() != error_code::sync_unsupported) {
        discard_temp();
        return std::unexpected(synced.error());
    }

    failpoints::maybe_crash(failpoints::crash_point::after_sidecar_sync);

    if (auto const replaced = replace_file_atomically(temp_path, path); ! replaced) {
        discard_temp();
        return std::unexpected(replaced.error());
    }

    // Named. Nothing below may report this as a failure to publish.
    if (auto const synced = sync_directory(path.parent_path());
        ! synced && synced.error() != error_code::sync_unsupported) {
        return sidecar_publication{.named = true, .durable = false};
    }

    return sidecar_publication{.named = true, .durable = true};
}

} // namespace utxoz::detail
