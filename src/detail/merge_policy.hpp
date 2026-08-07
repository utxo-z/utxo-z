// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file merge_policy.hpp
 * @brief Everything that differs between the two storage modes when merging.
 * @internal
 *
 * The crash-atomic merge protocol was written twice, once per mode, and the two
 * were identical step for step — twenty-eight of them, in the same order, down
 * to which failpoint sits between which barriers. Only six things actually differed: the map type, the
 * container's identity, the file size, the bucket count, which catalogue holds
 * the versions, and where the block height lives in a stored value.
 *
 * Two copies of a sequence whose correctness is entirely in its ordering is the
 * shape that drifts, and it already had: the compact twin was left on the
 * previous protocol when the full one was rewritten, and every correction since
 * had to be applied twice by hand.
 *
 * So the six differences are named here and the protocol is written once. A
 * policy is a view onto the database for one container; it holds a reference
 * and owns nothing.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <boost/interprocess/managed_mapped_file.hpp>

#include <utxoz/aliases.hpp>
#include <utxoz/types.hpp>

#include "utxo_value.hpp"
#include "version_catalog.hpp"

namespace utxoz::detail {

namespace bip = boost::interprocess;

struct database_impl;

/// One of the five size-tiered containers of a full-mode database.
template <size_t Index>
struct full_merge_policy {
    using map_type = utxo_map<container_sizes[Index]>;

    database_impl& db;

    [[nodiscard]] size_t index() const;
    [[nodiscard]] size_t file_size() const;
    [[nodiscard]] size_t min_buckets() const;
    [[nodiscard]] version_catalog& catalogue() const;
    void save_metadata(size_t version) const;

    /// The merge target is a file this call just created, so the map cannot
    /// already be there. `construct` says that and refuses a duplicate;
    /// `find_or_construct` would accept one silently, which here would mean
    /// building into a map that came from somewhere unexplained.
    [[nodiscard]]
    static map_type* construct_map(bip::managed_mapped_file& segment, size_t buckets) {
        return segment.construct<map_type>(map_object_name)(
            buckets, outpoint_hash{}, outpoint_equal{},
            segment.get_allocator<typename map_type::value_type>());
    }

    [[nodiscard]]
    static map_type* find_map(bip::managed_mapped_file& segment) {
        return segment.find<map_type>(map_object_name).first;
    }

    /// Where the creation height lives in a stored value. The one field the two
    /// value types spell differently.
    [[nodiscard]]
    static uint32_t height_of(typename map_type::mapped_type const& value) {
        return value.block_height;
    }

    /// How this container's versions are named in a log line.
    [[nodiscard]] std::string describe(size_t version) const;
};

/// The single container of a compact-mode database.
struct compact_merge_policy {
    using map_type = compact_map_t;

    database_impl& db;

    [[nodiscard]] size_t index() const;
    [[nodiscard]] size_t file_size() const;
    [[nodiscard]] size_t min_buckets() const;
    [[nodiscard]] version_catalog& catalogue() const;
    void save_metadata(size_t version) const;

    /// The merge target is a file this call just created, so the map cannot
    /// already be there. `construct` says that and refuses a duplicate;
    /// `find_or_construct` would accept one silently, which here would mean
    /// building into a map that came from somewhere unexplained.
    [[nodiscard]]
    static map_type* construct_map(bip::managed_mapped_file& segment, size_t buckets) {
        return segment.construct<map_type>(map_object_name)(
            buckets, outpoint_hash{}, outpoint_equal{},
            segment.get_allocator<typename map_type::value_type>());
    }

    [[nodiscard]]
    static map_type* find_map(bip::managed_mapped_file& segment) {
        return segment.find<map_type>(map_object_name).first;
    }

    [[nodiscard]]
    static uint32_t height_of(map_type::mapped_type const& value) {
        return value.height;
    }

    [[nodiscard]] std::string describe(size_t version) const;
};

} // namespace utxoz::detail
