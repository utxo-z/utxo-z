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

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <fmt/format.h>

#include "durability.hpp"
#include "path_display.hpp"
#include "segment_open.hpp"
#include "utxo_value.hpp"

namespace utxoz::detail {

inline constexpr std::string_view data_file_format = "cont_{}_v{:05}.dat";
inline constexpr std::string_view reference_data_file_format = "compact_v{:05}.dat";
inline constexpr size_t reference_sentinel_index = SIZE_MAX;

/**
 * @brief LRU cache for memory-mapped database files
 *
 * Provides efficient access to historical database files with automatic
 * eviction of least-recently-used entries.
 *
 * @warning No synchronisation, and the cache owns the mappings. Two rules
 * follow, and both are load-bearing:
 *
 * 1. Any map reference returned by the cache — get_or_open_file() or
 *    get_or_open_reference_file() alike — is valid only until the next cache
 *    operation. Evicting an entry destroys its managed_mapped_file, which
 *    unmaps the memory the map lives in, so holding a reference across a second
 *    call is a use-after-unmap. This bites single-threaded code too, as soon as
 *    anything interleaves two version files.
 * 2. Only one thread may touch the cache. Concurrently, the eviction above
 *    unmaps memory another thread is reading — a SIGSEGV/SIGBUS, not a data
 *    race, so locking the *contents* would not help.
 *
 * The default of one cached file makes rule 1 easy to trip: nearly every miss
 * evicts the previous entry.
 */
struct file_cache {
    template<size_t Index>
    using ret_t = std::pair<utxo_map<container_sizes[Index]>&, bool>;

    using file_key_t = std::pair<size_t, size_t>; // (container_index, version)

    explicit file_cache(fs::path path, size_t max_size = 1)
        : base_path_(std::move(path)), max_cached_files_(max_size) {}

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

        // Open file
        auto file_path = make_file_path(container_index, version);
        auto segment = open_existing_segment(file_path);

        auto* map = segment->find<utxo_map<container_sizes[Index]>>(map_object_name).first;
        if (!map) {
            throw std::runtime_error("Map not found in file: " + path_display(file_path));
        }

        cache_[file_key] = cached_file{
            std::move(segment),
            map,
            now,
            1,
            false
        };

        return {*map, false};
    }

    std::pair<reference_map_t&, bool> get_or_open_reference_file(size_t version) {
        file_key_t file_key{reference_sentinel_index, version};

        ++gets_;
        auto const now = std::chrono::steady_clock::now();
        ++access_frequency_[file_key];

        if (auto it = cache_.find(file_key); it != cache_.end()) {
            it->second.last_used = now;
            ++it->second.access_count;
            ++hits_;
            return {*static_cast<reference_map_t*>(it->second.map_ptr), true};
        }

        if (cache_.size() >= max_cached_files_) {
            evict_lru();
        }

        auto file_path = make_file_path(reference_sentinel_index, version);
        auto segment = open_existing_segment(file_path);

        auto* map = segment->find<reference_map_t>(map_object_name).first;
        if (!map) {
            throw std::runtime_error("Map not found in reference file: " + path_display(file_path));
        }

        cache_[file_key] = cached_file{
            std::move(segment),
            map,
            now,
            1,
            false
        };

        return {*map, false};
    }

    float get_hit_rate() const {
        return gets_ > 0 ? float(hits_) / float(gets_) : 0.0f;
    }

    void set_cache_size(size_t new_size) {
        max_cached_files_ = new_size;
    }

    /**
     * @brief Drop every cached mapping.
     *
     * Must be called whenever the on-disk file layout changes underneath the
     * cache (compaction renames and removes version files), otherwise a cached
     * (container_index, version) entry keeps pointing at the mapping of a file
     * that no longer holds that version's data.
     */
    void clear() {
        cache_.clear();
    }

    /**
     * @brief Flushes the dirty pages of every mapping the cache holds.
     *
     * Historical resolution erases entries in older version files through this
     * cache, so those mappings carry writes that no active container knows
     * about. A sync that covered only the active containers would report the
     * database durable while the deletions a batch applied to older generations
     * were still nowhere but memory.
     *
     * The pages only. Making the files themselves durable is the caller's next
     * step, and it needs their paths, which it already has.
     */
    [[nodiscard]]
    result<> sync_mappings() const {
        for (auto const& [file_key, cf] : cache_) {
            if ( ! cf.segment) continue;
            if (auto const synced = sync_mapped_region(cf.segment->get_address(),
                                                       cf.segment->get_size());
                ! synced && synced.error() != error_code::sync_unsupported) {
                return synced;
            }
        }
        return {};
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
        std::unique_ptr<bip::managed_mapped_file> segment;
        void* map_ptr;
        std::chrono::steady_clock::time_point last_used;
        size_t access_count = 0;
        bool is_pinned = false;
    };

    fs::path make_file_path(size_t container_index, size_t version) const {
        if (container_index == reference_sentinel_index) {
            return base_path_ / fmt::format(reference_data_file_format, version);
        }
        return base_path_ / fmt::format(data_file_format, container_index, version);
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
    fs::path base_path_;
    size_t max_cached_files_;
    size_t gets_ = 0;
    size_t hits_ = 0;
    size_t evictions_ = 0;
};

} // namespace utxoz::detail
