// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file file_metadata_io.hpp
 * @brief Reading and writing the summary that describes a version file.
 * @internal
 *
 * Metadata is derived: everything in it can be recomputed by scanning the
 * version file it describes. Its absence is therefore an ordinary state, and
 * the only thing that must never happen is a record that looks valid and is
 * not — a summary is consulted to decide a file can be skipped, so a wrong one
 * turns a key that is present into a key that is missing.
 *
 * That is why the record carries a marker, a format version and a checksum
 * rather than relying on five reads having completed. Five completed reads
 * prove the file was long enough. They do not prove it is our file, that it was
 * written by a build that agrees about the layout, or that the write finished.
 *
 * On the way out the record is published rather than written in place: a temp
 * in the same directory, synced, renamed over the final name, and the directory
 * synced where the platform allows it. A reader therefore sees the old record
 * or the new one, never half of either.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <expected>
#include <filesystem>
#include <fstream>
#include <bit>
#include <limits>
#include <span>
#include <vector>

#include <utxoz/types.hpp>

#include "durability.hpp"
#include "record_bytes.hpp"
#include "file_metadata.hpp"

namespace utxoz::detail {

namespace fs = std::filesystem;

/// Why a metadata file did not yield a record.
///
/// Every one of these means the same thing to the store — the version's
/// metadata is *unknown* — and they differ only in what they say about how it
/// got that way, which is worth logging differently.
enum class metadata_read_error : uint8_t {
    absent,        ///< No file. The ordinary state; not a problem.
    unreadable,    ///< The file is there but could not be read: I/O or permissions.
    foreign,       ///< Not this store's format, or a format version we do not know.
    malformed,     ///< Ours, and damaged: wrong length, bad checksum, incoherent fields.
};

/**
 * @brief On-disk layout of a metadata record.
 *
 * Fixed size, so a file of any other length is rejected on sight — which is
 * what catches both a truncated write and trailing bytes.
 *
 * Little-endian. Every target the store builds for is little-endian, so nothing
 * is emitted to convert; the assertion below is what makes a big-endian port
 * fail loudly instead of quietly writing a different format under the same
 * marker and version.
 */
struct metadata_record {
    static constexpr std::array<char, 4> magic{'U', 'Z', 'M', 'D'};
    static constexpr uint16_t current_format = 1;

    /// 4 + 2 + 2 + 4 + 4 + 36 + 36 + 8 + 4
    static constexpr size_t encoded_size = 100;
};

/// This record was where the encoding was first written; it now lives in
/// record_bytes.hpp, shared with the config and the segment stamp, so the three
/// cannot drift into three slightly different little-endian conventions.
namespace metadata_detail = record_bytes;

/// Serialises a record, checksum included.
[[nodiscard]]
inline std::vector<uint8_t> encode_metadata(file_metadata const& meta) {
    using namespace metadata_detail;

    std::vector<uint8_t> out;
    out.reserve(metadata_record::encoded_size);

    out.insert(out.end(), metadata_record::magic.begin(), metadata_record::magic.end());
    put(out, metadata_record::current_format);
    put(out, uint16_t{0});   // reserved, must be zero
    put(out, meta.min_block_height);
    put(out, meta.max_block_height);
    out.insert(out.end(), meta.min_key.begin(), meta.min_key.end());
    out.insert(out.end(), meta.max_key.begin(), meta.max_key.end());
    put(out, uint64_t(meta.entry_count));
    put(out, checksum(std::span<uint8_t const>(out)));

    return out;
}

/**
 * @brief Parses and fully validates a record.
 *
 * Nothing is committed to the caller unless every check passes, so a partial
 * record cannot reach the store as a partially filled one.
 */
[[nodiscard]]
inline std::expected<file_metadata, metadata_read_error>
decode_metadata(std::span<uint8_t const> bytes) {
    using namespace metadata_detail;

    // The marker decides whose file this is, and it decides before the length
    // does. A record written by a build with a different layout is *foreign*,
    // not damaged — calling it damaged would report a corrupt database every
    // time an older one is opened, which is both alarming and wrong.
    if (bytes.size() < metadata_record::magic.size()) {
        // Too short to even identify. Could be a torn write of ours.
        return std::unexpected(metadata_read_error::malformed);
    }
    if ( ! std::equal(metadata_record::magic.begin(), metadata_record::magic.end(),
                      reinterpret_cast<char const*>(bytes.data()))) {
        return std::unexpected(metadata_read_error::foreign);
    }

    // Ours, so the length is now a statement about this record: anything but
    // the exact size is a write that was cut short or a file with something
    // appended to it.
    if (bytes.size() != metadata_record::encoded_size) {
        return std::unexpected(metadata_read_error::malformed);
    }

    auto const* cursor = bytes.data() + metadata_record::magic.size();

    uint16_t format = 0;
    uint16_t reserved = 0;
    get(cursor, format);
    get(cursor, reserved);

    if (format != metadata_record::current_format) {
        // Ours, but written by a build that does not agree about the layout.
        return std::unexpected(metadata_read_error::foreign);
    }
    if (reserved != 0) {
        return std::unexpected(metadata_read_error::malformed);
    }

    file_metadata meta;
    get(cursor, meta.min_block_height);
    get(cursor, meta.max_block_height);
    std::memcpy(meta.min_key.data(), cursor, meta.min_key.size());
    cursor += meta.min_key.size();
    std::memcpy(meta.max_key.data(), cursor, meta.max_key.size());
    cursor += meta.max_key.size();

    uint64_t entry_count = 0;
    get(cursor, entry_count);

    uint32_t stored_checksum = 0;
    get(cursor, stored_checksum);

    auto const covered = bytes.subspan(0, metadata_record::encoded_size - sizeof(uint32_t));
    if (checksum(covered) != stored_checksum) {
        return std::unexpected(metadata_read_error::malformed);
    }

    // Only a real narrowing needs the check; where size_t is 64-bit the
    // comparison is always false and the compiler says so.
    if constexpr (sizeof(size_t) < sizeof(uint64_t)) {
        if (entry_count > uint64_t(std::numeric_limits<size_t>::max())) {
            return std::unexpected(metadata_read_error::malformed);
        }
    }
    meta.entry_count = size_t(entry_count);

    // Coherent, not merely intact. A record that passes its checksum can still
    // describe something impossible if it was written by a build with a bug,
    // and a summary that is consulted to skip files has to be refused then too.
    if (meta.entry_count > 0) {
        if (meta.min_block_height > meta.max_block_height) {
            return std::unexpected(metadata_read_error::malformed);
        }
        if (meta.min_key > meta.max_key) {
            return std::unexpected(metadata_read_error::malformed);
        }
    }

    return meta;
}

/// Reads a record, or says why there is none. Never returns a partial one.
[[nodiscard]]
inline std::expected<file_metadata, metadata_read_error>
read_metadata_file(fs::path const& path) {
    std::error_code ec;
    auto const status = fs::status(path, ec);
    if (status.type() == fs::file_type::not_found) {
        return std::unexpected(metadata_read_error::absent);
    }
    if (ec || ! fs::is_regular_file(status)) {
        return std::unexpected(metadata_read_error::unreadable);
    }

    auto const size = fs::file_size(path, ec);
    if (ec) return std::unexpected(metadata_read_error::unreadable);

    std::ifstream ifs(path, std::ios::binary);
    if ( ! ifs) return std::unexpected(metadata_read_error::unreadable);

    // Identify before reading. Whose file this is decides how a wrong length is
    // reported, and it is answered by four bytes — so a file of any size at all
    // is classified without being loaded.
    std::array<char, metadata_record::magic.size()> marker{};
    ifs.read(marker.data(), std::streamsize(marker.size()));
    if (ifs.gcount() != std::streamsize(marker.size())) {
        // Shorter than the marker: cannot be identified, and a torn write of
        // ours is the likeliest way to get one.
        return std::unexpected(metadata_read_error::malformed);
    }
    if ( ! std::ranges::equal(marker, metadata_record::magic)) {
        return std::unexpected(metadata_read_error::foreign);
    }

    // Ours, so the length is now a statement about this record — including a
    // file far larger than a record, which is damage and not a foreign format.
    if (size != metadata_record::encoded_size) {
        return std::unexpected(metadata_read_error::malformed);
    }

    std::vector<uint8_t> buffer(metadata_record::encoded_size, 0);
    std::ranges::copy(marker, reinterpret_cast<char*>(buffer.data()));

    auto const remaining = std::streamsize(buffer.size() - marker.size());
    ifs.read(reinterpret_cast<char*>(buffer.data()) + marker.size(), remaining);
    if (ifs.gcount() != remaining) {
        return std::unexpected(metadata_read_error::unreadable);
    }

    return decode_metadata(buffer);
}

/**
 * @brief Whether publication should also try to make the record durable.
 *
 * Metadata is derived, so the ordinary answer is no. Correctness needs only
 * that no reader sees a partial record, and that comes from writing a temp and
 * replacing atomically — not from a barrier. Whatever a crash leaves behind is
 * the old record, the new one, nothing, or something that fails validation, and
 * the last two are both *unknown*, which every consumer already handles.
 *
 * Barriers are not free. Every rotation publishes a record, so making that path
 * synchronous puts an fsync on a rotation, which sits on the insert path.
 *
 * `durable` exists for the callers that are making a promise to someone — the
 * compaction protocol — and is not used for ordinary publication.
 */
enum class metadata_sync : uint8_t { publish_only, durable };

/**
 * @brief Publishes a record: temp beside the target, then an atomic replace.
 *
 * The temp goes in the same directory so the replace is within one filesystem.
 * Every write and the close are checked — an ofstream reports a failed write by
 * setting a bit nobody looks at, which is how a short file gets produced in the
 * first place.
 *
 * Outcomes are not all the same, and the difference is where the replace sits:
 *
 * | Result | Meaning |
 * |---|---|
 * | success | the new record is published |
 * | `metadata_write_failed` | failed **before or during** the replace; the previous record, if any, is untouched |
 * | `sync_failed` | `durable` was asked for and a barrier **after** the replace failed; the new record is published and visible, and only its persistence is unconfirmed |
 *
 * The distinction matters because a replace cannot be undone: once the new
 * record has the name, reporting the whole call as a failure would say the
 * previous one survived, and it did not.
 *
 * A platform without a given barrier is **not** an error and is absorbed here —
 * this record is derived and the function promises nobody it survived a crash.
 * A caller that needs to know what the platform can do asks
 * `platform_sync_support()`; it is not inferred from this returning success.
 */
[[nodiscard]]
inline result<> write_metadata_file(fs::path const& path, file_metadata const& meta,
                                    metadata_sync policy = metadata_sync::publish_only) {
    auto const temp_path = fs::path(path).concat(".tmp");
    auto const encoded = encode_metadata(meta);

    auto discard_temp = [&] {
        std::error_code cleanup;
        fs::remove(temp_path, cleanup);
    };

    {
        std::ofstream ofs(temp_path, std::ios::binary | std::ios::trunc);
        if ( ! ofs) return std::unexpected(error_code::metadata_write_failed);

        ofs.write(reinterpret_cast<char const*>(encoded.data()), std::streamsize(encoded.size()));
        if ( ! ofs) {
            ofs.close();
            discard_temp();
            return std::unexpected(error_code::metadata_write_failed);
        }

        ofs.close();
        if (ofs.fail()) {
            discard_temp();
            return std::unexpected(error_code::metadata_write_failed);
        }
    }

    if (policy == metadata_sync::durable) {
        // Contents durable before the name that publishes them. Still before
        // the replace, so a failure here leaves the previous record intact.
        if (auto const synced = sync_file(temp_path);
            ! synced && synced.error() != error_code::sync_unsupported) {
            discard_temp();
            return std::unexpected(error_code::metadata_write_failed);
        }
    }

    if (auto const replaced = replace_file_atomically(temp_path, path); ! replaced) {
        discard_temp();
        return std::unexpected(error_code::metadata_write_failed);
    }

    // Past this line the new record is published. Nothing below can un-publish
    // it, so nothing below may be reported as the publication having failed.
    if (policy == metadata_sync::durable) {
        if (auto const synced = sync_directory(path.parent_path());
            ! synced && synced.error() != error_code::sync_unsupported) {
            return std::unexpected(error_code::sync_failed);
        }
    }

    return {};
}

} // namespace utxoz::detail
