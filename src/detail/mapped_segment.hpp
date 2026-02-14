// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file mapped_segment.hpp
 * @brief Memory-mapped file segment for mmap_flat_map storage
 * @internal
 *
 * Simple wrapper around Boost's file_mapping + mapped_region.
 * The file layout is:
 *   [file_header: 64 bytes] [table buffer: groups + elements]
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace utxoz::detail {

namespace bip = boost::interprocess;

inline constexpr uint64_t file_header_magic = 0x55'54'58'4F'5A'44'42'31; // "UTXOZDB1"
inline constexpr uint32_t file_header_version = 2; // v2: mmap_flat_map layout

struct file_header {
    uint64_t magic            = file_header_magic;
    uint32_t version          = file_header_version;
    uint32_t container_index  = 0;
    uint64_t file_version     = 0;
    uint64_t entry_count      = 0;     // = size_ctrl.size
    uint64_t max_load         = 0;     // = size_ctrl.ml
    uint64_t groups_size_index = 0;    // for position_for(hash)
    uint64_t groups_size_mask = 0;     // = groups_size - 1
    uint64_t total_capacity   = 0;
};

static_assert(sizeof(file_header) == 64);

struct open_or_create_t {};
struct open_only_t {};

inline constexpr open_or_create_t open_or_create{};
inline constexpr open_only_t open_only{};

struct mapped_segment {
    mapped_segment(std::string const& path, size_t size, open_or_create_t) {
        // Create/truncate the file to the desired size
        {
            bip::file_mapping::remove(path.c_str());
            std::ofstream ofs(path, std::ios::binary);
            if ( ! ofs) {
                throw std::runtime_error("Failed to create file: " + path);
            }
            ofs.close();
            std::filesystem::resize_file(path, size);
        }

        file_mapping_ = bip::file_mapping(path.c_str(), bip::read_write);
        region_ = bip::mapped_region(file_mapping_, bip::read_write, 0, size);
        size_ = size;

        // Initialize header
        std::memset(region_.get_address(), 0, sizeof(file_header));
        auto* hdr = &header();
        hdr->magic = file_header_magic;
        hdr->version = file_header_version;
        hdr->total_capacity = size - sizeof(file_header);
    }

    mapped_segment(std::string const& path, open_only_t) {
        file_mapping_ = bip::file_mapping(path.c_str(), bip::read_write);
        region_ = bip::mapped_region(file_mapping_, bip::read_write);
        size_ = region_.get_size();

        auto const* hdr = &header();
        if (hdr->magic != file_header_magic) {
            throw std::runtime_error("Invalid file magic in: " + path);
        }
        if (hdr->version != file_header_version) {
            throw std::runtime_error("Unsupported file version in: " + path);
        }
    }

    ~mapped_segment() {
        if (region_.get_address()) {
            region_.flush();
        }
    }

    mapped_segment(mapped_segment const&) = delete;
    mapped_segment& operator=(mapped_segment const&) = delete;
    mapped_segment(mapped_segment&&) = default;
    mapped_segment& operator=(mapped_segment&&) = default;

    void* base() const { return region_.get_address(); }
    size_t size() const { return size_; }

    file_header& header() {
        return *static_cast<file_header*>(region_.get_address());
    }

    file_header const& header() const {
        return *static_cast<file_header const*>(region_.get_address());
    }

    /// Pointer to the table buffer (right after the header).
    /// This is where mmap_flat_map's groups + elements live.
    void* table_buffer() {
        return static_cast<uint8_t*>(region_.get_address()) + sizeof(file_header);
    }

    void const* table_buffer() const {
        return static_cast<uint8_t const*>(region_.get_address()) + sizeof(file_header);
    }

    size_t table_buffer_size() const {
        return size_ - sizeof(file_header);
    }

    void flush() {
        region_.flush();
    }

private:
    bip::file_mapping file_mapping_;
    bip::mapped_region region_;
    size_t size_ = 0;
};

} // namespace utxoz::detail
