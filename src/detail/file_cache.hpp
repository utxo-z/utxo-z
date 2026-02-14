// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file file_cache.hpp
 * @brief LRU file cache for database files
 * @internal
 */

#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>
#include <fmt/format.h>

#include "mapped_segment.hpp"
#include "utxo_value.hpp"

namespace utxoz::detail {

inline constexpr std::string_view data_file_format = "{}/cont_{}_v{:05}.dat";

/**
 * @brief LRU cache for memory-mapped database files
 *
 * Provides efficient access to historical database files with automatic
 * eviction of least-recently-used entries.
 *
 * With mmap_flat_map, opening a file is just mmap + attach (O(1)).
 * No deserialization needed. Closing is save scalars + munmap.
 */
struct file_cache {
    template<size_t Index>
    using ret_t = std::pair<utxo_map<container_sizes[Index]>&, bool>;

    using file_key_t = std::pair<size_t, size_t>; // (container_index, version)

    explicit file_cache(std::string const& path, size_t max_size = 1)
        : base_path_(path), max_cached_files_(max_size) {}

    template<size_t Index>
    ret_t<Index> get_or_open_file(size_t container_index, size_t version) {
        file_key_t file_key{container_index, version};

        ++gets_;
        auto const now = std::chrono::steady_clock::now();

        // Track access frequency
        ++access_frequency_[file_key];

        // Check cache
        if (auto it = cache_.find(file_key); it != cache_.end()) {
            it->second.last_used = now;
            ++it->second.access_count;
            ++hits_;
            return {*static_cast<utxo_map<container_sizes[Index]>*>(it->second.map_ptr), true};
        }

        // Evict if needed
        if (cache_.size() >= max_cached_files_) {
            evict_lru();
        }

        // Open file and attach to the table already in the mmap buffer (O(1))
        auto file_path = make_file_path(container_index, version);
        auto segment = std::make_unique<mapped_segment>(file_path, open_only);
        auto& hdr = segment->header();

        using map_t = utxo_map<container_sizes[Index]>;
        auto* map = new map_t(map_t::attach(
            segment->table_buffer(),
            hdr.groups_size_index,
            hdr.groups_size_mask,
            hdr.entry_count,
            hdr.max_load,
            outpoint_hash{},
            outpoint_equal{}));

        // Capture raw pointers before moving into cached_file
        auto* seg_ptr = segment.get();

        cached_file cf;
        cf.segment = std::move(segment);
        cf.map_ptr = map;
        // On eviction: save scalar state to header, delete map wrapper, flush segment
        cf.on_close = [map, seg_ptr]() {
            auto& h = seg_ptr->header();
            h.entry_count = map->size();
            h.max_load = map->get_max_load();
            delete map;
        };
        cf.last_used = now;
        cf.access_count = 1;
        cf.is_pinned = false;
        cache_[file_key] = std::move(cf);

        return {*map, false};
    }

    float get_hit_rate() const {
        return gets_ > 0 ? float(hits_) / float(gets_) : 0.0f;
    }

    void set_cache_size(size_t new_size) {
        max_cached_files_ = new_size;
    }

    void clear() {
        cache_.clear();
    }

    std::vector<std::pair<size_t, size_t>> get_cached_files() const {
        std::vector<std::pair<size_t, size_t>> files;
        files.reserve(cache_.size());
        for (auto const& [file_key, cf] : cache_) {
            files.push_back(file_key);
        }
        return files;
    }

    bool is_cached(size_t container_index, size_t version) const {
        return cache_.contains(file_key_t{container_index, version});
    }

    std::optional<std::pair<size_t, size_t>> get_most_recent_cached_file() const {
        if (cache_.empty()) return std::nullopt;

        auto most_recent = std::ranges::max_element(cache_,
            [](auto const& a, auto const& b) {
                return a.second.last_used < b.second.last_used;
            });

        return most_recent->first;
    }

private:
    struct cached_file {
        std::unique_ptr<mapped_segment> segment;  // destroyed last (declared first)
        void* map_ptr = nullptr;
        std::function<void()> on_close;            // save state + delete map
        std::chrono::steady_clock::time_point last_used;
        size_t access_count = 0;
        bool is_pinned = false;

        ~cached_file() {
            // Body runs before member destruction, so segment is still alive
            if (on_close) on_close();
        }

        cached_file() = default;
        cached_file(cached_file&&) = default;
        cached_file& operator=(cached_file&& other) {
            if (this != &other) {
                // Save state for current map before replacing
                if (on_close) on_close();
                segment = std::move(other.segment);
                map_ptr = other.map_ptr;
                on_close = std::move(other.on_close);
                last_used = other.last_used;
                access_count = other.access_count;
                is_pinned = other.is_pinned;
                other.map_ptr = nullptr;
                other.on_close = nullptr;
            }
            return *this;
        }
        cached_file(cached_file const&) = delete;
        cached_file& operator=(cached_file const&) = delete;
    };

    std::string make_file_path(size_t container_index, size_t version) const {
        return fmt::format(data_file_format, base_path_, container_index, version);
    }

    void evict_lru() {
        auto lru = std::ranges::min_element(cache_,
            [](auto const& a, auto const& b) {
                if (a.second.is_pinned != b.second.is_pinned)
                    return !a.second.is_pinned;
                return a.second.last_used < b.second.last_used;
            });

        if (lru != cache_.end() && !lru->second.is_pinned) {
            cache_.erase(lru);
            ++evictions_;
        }
    }

    boost::unordered_flat_map<file_key_t, cached_file> cache_;
    boost::unordered_flat_map<file_key_t, size_t> access_frequency_;
    std::string base_path_;
    size_t max_cached_files_;
    size_t gets_ = 0;
    size_t hits_ = 0;
    size_t evictions_ = 0;
};

} // namespace utxoz::detail
