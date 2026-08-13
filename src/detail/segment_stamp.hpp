// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file segment_stamp.hpp
 * @brief What every version file says about itself, before anything reads it.
 * @internal
 *
 * The config is the authority on what this database is; this is the redundancy
 * that catches the things a config cannot know about an individual file:
 *
 * - a version file copied in from a different database;
 * - two geometries mixed in one directory;
 * - a config from one database beside data from another;
 * - a historical version left behind by an older, incompatible build.
 *
 * The stamp is read and validated before `db_map` is so much as looked for. That
 * ordering is the whole point: `find<utxo_map>()` on a file written under a
 * different layout does not fail, it reinterprets, and there is no later moment
 * at which that becomes detectable.
 *
 * @par Why a byte array rather than a struct
 * The stamp lives inside the mapped file, so its own layout is part of the
 * format — and a struct of six integers is exactly the kind of thing whose
 * padding and alignment differ between the platforms this is meant to tell
 * apart. So the object stored is an array of bytes, which has one layout
 * everywhere, and the fields are encoded into it field by field, little-endian,
 * with a checksum. It also gives `find_single_named` something whose `sizeof`
 * cannot drift.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <new>
#include <span>
#include <vector>

#include <boost/interprocess/managed_mapped_file.hpp>

#include <utxoz/types.hpp>

#include "format_identity.hpp"
#include "log.hpp"
#include "path_display.hpp"
#include "record_bytes.hpp"
#include "segment_open.hpp"

namespace utxoz::detail {

namespace bip = boost::interprocess;
namespace fs = std::filesystem;

/// What a stamp says. Decoded form; the stored form is the byte array below.
struct segment_identity {
    uint32_t geometry_id = 0;
    uint32_t map_layout_epoch = 0;
    uint32_t hash_epoch = 0;
    uint32_t platform_abi_id = 0;
    database_id_t database_id{};
    uint32_t container_kind = 0;   ///< container index, or reference_container_kind
    uint64_t version = 0;          ///< the generation this file is
};

/**
 * @brief The stored object: bytes, and nothing a compiler can rearrange.
 *
 * Named separately from the map so it can be reached without knowing anything
 * about the map's layout — which is the only way a layout check can run before
 * the layout is trusted.
 */
struct segment_stamp {
    static constexpr char const* object_name = "utxoz_stamp";
    static constexpr std::array<char, 4> magic{'U', 'Z', 'S', 'T'};
    static constexpr uint16_t current_format = 1;

    /// 4 magic + 2 format + 2 reserved + 4*4 identities + 16 id + 4 kind + 8 version + 4 checksum
    static constexpr size_t encoded_size = 56;

    std::array<uint8_t, encoded_size> raw{};
};

static_assert(sizeof(segment_stamp) == segment_stamp::encoded_size,
              "the stamp must be exactly its bytes: anything else means the compiler added "
              "padding to the one object whose layout has to be identical everywhere");

/// The identity this build stamps into every file it creates.
[[nodiscard]]
inline segment_identity local_identity(database_id_t const& id, uint32_t container_kind,
                                       uint64_t version) {
    return segment_identity{geometry_id,      map_layout_epoch, hash_epoch, platform_abi_id,
                            id,               container_kind,   version};
}

[[nodiscard]]
inline segment_stamp encode_stamp(segment_identity const& identity) {
    using namespace record_bytes;

    std::vector<uint8_t> out;
    out.reserve(segment_stamp::encoded_size);

    out.insert(out.end(), segment_stamp::magic.begin(), segment_stamp::magic.end());
    put(out, segment_stamp::current_format);
    put(out, uint16_t{0});   // reserved, must be zero
    put(out, identity.geometry_id);
    put(out, identity.map_layout_epoch);
    put(out, identity.hash_epoch);
    put(out, identity.platform_abi_id);
    out.insert(out.end(), identity.database_id.begin(), identity.database_id.end());
    put(out, identity.container_kind);
    put(out, identity.version);
    put(out, checksum(std::span<uint8_t const>(out)));

    segment_stamp stamp{};
    std::ranges::copy(out, stamp.raw.begin());
    return stamp;
}

/// Why a stamp did not yield an identity. Every one of these is fatal; they
/// differ in what they tell an operator to go and look at.
enum class stamp_error : uint8_t {
    corrupt,   ///< Not our marker, a format we do not know, or a bad checksum.
};

[[nodiscard]]
inline std::expected<segment_identity, stamp_error> decode_stamp(segment_stamp const& stamp) {
    using namespace record_bytes;

    if ( ! std::equal(segment_stamp::magic.begin(), segment_stamp::magic.end(),
                      stamp.raw.begin())) {
        return std::unexpected(stamp_error::corrupt);
    }

    auto const* cursor = stamp.raw.data() + segment_stamp::magic.size();

    uint16_t format = 0;
    uint16_t reserved = 0;
    get(cursor, format);
    get(cursor, reserved);
    if (format != segment_stamp::current_format || reserved != 0) {
        return std::unexpected(stamp_error::corrupt);
    }

    segment_identity identity;
    get(cursor, identity.geometry_id);
    get(cursor, identity.map_layout_epoch);
    get(cursor, identity.hash_epoch);
    get(cursor, identity.platform_abi_id);
    std::memcpy(identity.database_id.data(), cursor, identity.database_id.size());
    cursor += identity.database_id.size();
    get(cursor, identity.container_kind);
    get(cursor, identity.version);

    uint32_t stored = 0;
    get(cursor, stored);

    auto const covered = std::span<uint8_t const>(
        stamp.raw.data(), segment_stamp::encoded_size - sizeof(stored));
    if (stored != checksum(covered)) {
        return std::unexpected(stamp_error::corrupt);
    }

    return identity;
}

/**
 * @brief Reads a segment's stamp and holds it to the identity this build expects.
 *
 * Every mismatch has its own error, because they send an operator somewhere
 * different: the geometry is ours and changed, the layout is Boost's and
 * changed, the hash moved every key, the platform is not the one that wrote
 * this, or the file belongs to another database entirely.
 */
[[nodiscard]]
inline result<> validate_stamp(bip::managed_mapped_file& segment, fs::path const& path,
                               segment_identity const& expected) {
    // Asked directly rather than through find_single_named, because the two
    // answers it folds together mean different things here: no stamp at all is a
    // file this build did not write, while a stamp that does not measure one
    // instance is a file whose stamp cannot be read as a stamp.
    auto const found = segment.find<segment_stamp>(segment_stamp::object_name);
    if (found.first == nullptr) {
        log::error("{}: carries no format stamp", path_display(path));
        return std::unexpected(error_code::segment_stamp_missing);
    }
    if (found.second != 1) {
        log::error("{}: its format stamp measures {} instances, not one",
                   path_display(path), found.second);
        return std::unexpected(error_code::segment_stamp_corrupt);
    }

    auto const identity = decode_stamp(*found.first);
    if ( ! identity) {
        log::error("{}: its format stamp is damaged", path_display(path));
        return std::unexpected(error_code::segment_stamp_corrupt);
    }

    if (identity->geometry_id != expected.geometry_id) {
        log::error("{}: written under container geometry {}, this build writes {}",
                   path_display(path), identity->geometry_id, expected.geometry_id);
        return std::unexpected(error_code::geometry_mismatch);
    }
    if (identity->map_layout_epoch != expected.map_layout_epoch) {
        log::error("{}: written under map layout epoch {}, this build is certified for {}",
                   path_display(path), identity->map_layout_epoch, expected.map_layout_epoch);
        return std::unexpected(error_code::layout_mismatch);
    }
    if (identity->hash_epoch != expected.hash_epoch) {
        log::error("{}: written under hash epoch {}, this build uses {}; every key would be "
                   "in a different place", path_display(path), identity->hash_epoch,
                   expected.hash_epoch);
        return std::unexpected(error_code::hash_mismatch);
    }
    if (identity->platform_abi_id != expected.platform_abi_id) {
        log::error("{}: written on platform ABI {:#x}, this is {:#x}", path_display(path),
                   identity->platform_abi_id, expected.platform_abi_id);
        return std::unexpected(error_code::abi_mismatch);
    }
    if (identity->database_id != expected.database_id) {
        log::error("{}: belongs to a different database", path_display(path));
        return std::unexpected(error_code::database_identity_mismatch);
    }
    if (identity->container_kind != expected.container_kind
        || identity->version != expected.version) {
        // Intact, and describing a different file. Its own error: a stamp that
        // will not decode sends an operator looking for damage, and this one
        // sends them looking for a file that was moved or renamed.
        log::error("{}: stamped as container {} version {}, opened as container {} version {}",
                   path_display(path), identity->container_kind, identity->version,
                   expected.container_kind, expected.version);
        return std::unexpected(error_code::segment_misplaced);
    }

    return {};
}

/// Writes the stamp into a segment being created. `construct` and not
/// `find_or_construct`: this file is new, so a name already taken means it is not
/// the file this call thinks it is.
[[nodiscard]]
inline result<> place_stamp(bip::managed_mapped_file& segment, fs::path const& path,
                            segment_identity const& identity) {
    try {
        // nothrow, so a name already taken comes back as a null rather than as an
        // exception this would report as a failure to open. They are different
        // things: one is a file that already carries a stamp.
        auto* placed = segment.construct<segment_stamp>(segment_stamp::object_name, std::nothrow)(
            encode_stamp(identity));
        if (placed == nullptr) {
            log::error("{}: already carries a format stamp", path_display(path));
            return std::unexpected(error_code::identity_collision);
        }
    } catch (std::exception const& e) {
        log::error("{}: could not be stamped: {}", path_display(path), e.what());
        return std::unexpected(error_code::file_open_failed);
    }
    return {};
}

} // namespace utxoz::detail
