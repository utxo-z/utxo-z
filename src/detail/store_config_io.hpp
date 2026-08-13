// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file store_config_io.hpp
 * @brief The database's own account of what it is, read before anything else.
 * @internal
 *
 * `utxoz_config.dat` is the authority. It is read and checked against this build
 * before a single segment is mapped, so a database written under a geometry, a
 * map layout, a hash or a platform this build does not share is refused while
 * every file is still untouched.
 *
 * @par Format 2
 * Fixed length, field by field, little-endian, checksum last — the same
 * encoding the metadata records use, and for the same reason: five completed
 * reads prove the file was long enough, not that it is ours or that the write
 * finished.
 *
 * ```
 *  0   4  magic "UTXO"
 *  4   4  config_format_version
 *  8   1  storage_mode
 *  9   3  reserved, zero
 * 12   4  geometry_id
 * 16   4  map_layout_epoch
 * 20   4  hash_epoch
 * 24   4  platform_abi_id
 * 28  16  database_id
 * 44   4  boost_version    (diagnostic only — never compared)
 * 48   4  checksum
 * ```
 *
 * @par Format 1
 * Four bytes of magic, a version, and a mode byte. It records nothing about the
 * layout it was written under, which is the whole problem: there is no way to
 * establish after the fact what a format-1 database's files mean. It is
 * recognised only so that it can be refused by name.
 *
 * @par Why BOOST_VERSION is stored and not compared
 * Several Boost releases can share one `map_layout_epoch`, and a version bump on
 * its own says nothing about compatibility. Comparing it would invalidate every
 * database on an upgrade for no reason. It is here so that an operator reading a
 * refusal can see which Boost wrote the file.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <expected>
#include <filesystem>
#include <fstream>
#include <span>
#include <vector>

#include <boost/version.hpp>

#include <utxoz/types.hpp>

#include "format_identity.hpp"
#include "log.hpp"
#include "path_display.hpp"
#include "record_bytes.hpp"

namespace utxoz::detail {

namespace fs = std::filesystem;

struct store_config {
    static constexpr std::array<char, 4> magic{'U', 'T', 'X', 'O'};
    static constexpr uint32_t legacy_format = 1;
    static constexpr uint32_t current_format = 2;
    static constexpr size_t encoded_size = 52;

    storage_mode mode = storage_mode::full;
    uint32_t geometry_id = 0;
    uint32_t map_layout_epoch = 0;
    uint32_t hash_epoch = 0;
    uint32_t platform_abi_id = 0;
    database_id_t database_id{};
    uint32_t boost_version = 0;   ///< diagnostic
};

/// The config this build writes for a database it is creating.
[[nodiscard]]
inline store_config local_config(storage_mode mode, database_id_t const& id) {
    return store_config{mode,
                        geometry_id,
                        map_layout_epoch,
                        hash_epoch,
                        platform_abi_id,
                        id,
                        uint32_t(BOOST_VERSION)};
}

[[nodiscard]]
inline std::vector<uint8_t> encode_config(store_config const& config) {
    using namespace record_bytes;

    std::vector<uint8_t> out;
    out.reserve(store_config::encoded_size);

    out.insert(out.end(), store_config::magic.begin(), store_config::magic.end());
    put(out, store_config::current_format);
    put(out, uint8_t(config.mode));
    put(out, uint8_t{0});
    put(out, uint16_t{0});   // reserved, must be zero
    put(out, config.geometry_id);
    put(out, config.map_layout_epoch);
    put(out, config.hash_epoch);
    put(out, config.platform_abi_id);
    out.insert(out.end(), config.database_id.begin(), config.database_id.end());
    put(out, config.boost_version);
    put(out, checksum(std::span<uint8_t const>(out)));

    return out;
}

/**
 * @brief Reads the config, or says exactly why it could not.
 *
 * A format-1 file is recognised and reported as `migration_required` rather than
 * as corruption: it is a perfectly good file from an earlier build, and telling
 * an operator it is damaged would send them looking for a fault that is not
 * there.
 */
[[nodiscard]]
inline result<store_config> read_config_file(fs::path const& path) {
    using namespace record_bytes;

    // Asked about before it is opened. A directory, a device or a file of
    // arbitrary length at this name is not a config, and reading it into memory
    // first to find that out is a way to be handed something very large.
    std::error_code ec;
    auto const status = fs::status(path, ec);
    if (ec || ! fs::is_regular_file(status)) {
        return std::unexpected(error_code::config_file_corrupt);
    }
    auto const size = fs::file_size(path, ec);
    if (ec || size > store_config::encoded_size) {
        return std::unexpected(error_code::config_file_corrupt);
    }

    std::ifstream ifs(path, std::ios::binary);
    if ( ! ifs) return std::unexpected(error_code::config_file_corrupt);

    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(ifs)),
                               std::istreambuf_iterator<char>());
    if (ifs.bad()) return std::unexpected(error_code::config_file_corrupt);

    if (bytes.size() < store_config::magic.size() + sizeof(uint32_t)) {
        return std::unexpected(error_code::config_file_corrupt);
    }
    if ( ! std::equal(store_config::magic.begin(), store_config::magic.end(), bytes.begin())) {
        return std::unexpected(error_code::config_file_corrupt);
    }

    auto const* cursor = bytes.data() + store_config::magic.size();
    uint32_t format = 0;
    get(cursor, format);

    if (format == store_config::legacy_format) {
        log::error("{}: this database was created before the format recorded what it was "
                   "written under (config format 1). What its files mean cannot be "
                   "established after the fact, so it is refused rather than assumed: "
                   "rebuild the database from the chain.", path_display(path));
        return std::unexpected(error_code::migration_required);
    }
    if (format != store_config::current_format) {
        log::error("{}: config format {} is not one this build knows", path_display(path),
                   format);
        return std::unexpected(error_code::format_unsupported);
    }

    if (bytes.size() != store_config::encoded_size) {
        return std::unexpected(error_code::config_file_corrupt);
    }

    store_config config;
    uint8_t mode_byte = 0;
    uint8_t pad8 = 0;
    uint16_t pad16 = 0;
    get(cursor, mode_byte);
    get(cursor, pad8);
    get(cursor, pad16);
    if (pad8 != 0 || pad16 != 0) return std::unexpected(error_code::config_file_corrupt);
    if (mode_byte != uint8_t(storage_mode::full) && mode_byte != uint8_t(storage_mode::reference)) {
        return std::unexpected(error_code::config_file_corrupt);
    }
    config.mode = storage_mode(mode_byte);

    get(cursor, config.geometry_id);
    get(cursor, config.map_layout_epoch);
    get(cursor, config.hash_epoch);
    get(cursor, config.platform_abi_id);
    std::memcpy(config.database_id.data(), cursor, config.database_id.size());
    cursor += config.database_id.size();
    get(cursor, config.boost_version);

    uint32_t stored = 0;
    get(cursor, stored);

    auto const covered = std::span<uint8_t const>(bytes.data(),
                                                  store_config::encoded_size - sizeof(stored));
    if (stored != checksum(covered)) return std::unexpected(error_code::config_file_corrupt);

    return config;
}

/**
 * @brief Holds a config that parsed against what this build can read.
 *
 * Separate from parsing so the two failures stay apart: a file this build cannot
 * read is not the same as a database this build cannot serve.
 */
[[nodiscard]]
inline result<> check_config_compatible(store_config const& config, fs::path const& path) {
    if (config.geometry_id != geometry_id) {
        log::error("{}: written under container geometry {}, this build writes {}; the "
                   "database has to be rebuilt", path_display(path), config.geometry_id,
                   geometry_id);
        return std::unexpected(error_code::geometry_mismatch);
    }
    if (config.map_layout_epoch != map_layout_epoch) {
        log::error("{}: written under map layout epoch {} (Boost {}), this build is certified "
                   "for epoch {}", path_display(path), config.map_layout_epoch,
                   config.boost_version, map_layout_epoch);
        return std::unexpected(error_code::layout_mismatch);
    }
    if (config.hash_epoch != hash_epoch) {
        log::error("{}: written under hash epoch {}, this build uses {}; every key would be in "
                   "a different place", path_display(path), config.hash_epoch, hash_epoch);
        return std::unexpected(error_code::hash_mismatch);
    }
    if (config.platform_abi_id != platform_abi_id) {
        log::error("{}: written on platform ABI {:#x}, this is {:#x}", path_display(path),
                   config.platform_abi_id, platform_abi_id);
        return std::unexpected(error_code::abi_mismatch);
    }
    return {};
}

} // namespace utxoz::detail
