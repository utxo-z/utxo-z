// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file database_impl.hpp
 * @brief Internal database implementation
 * @internal
 */

#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <variant>

#include <boost/unordered/unordered_flat_set.hpp>
#include <boost/unordered/concurrent_flat_set.hpp>

#include <utxoz/aliases.hpp>
#include <utxoz/database.hpp>
#include <utxoz/statistics.hpp>
#include <utxoz/types.hpp>

#include "file_cache.hpp"
#include "file_metadata.hpp"
#include "utxo_value.hpp"

namespace utxoz::detail {

namespace fs = std::filesystem;

/// @internal
struct database_impl {
    database_impl() = default;
    ~database_impl();

    // Non-copyable, non-movable
    database_impl(database_impl const&) = delete;
    database_impl& operator=(database_impl const&) = delete;
    database_impl(database_impl&&) = delete;
    database_impl& operator=(database_impl&&) = delete;

    // Public interface implementation
    void configure(std::string_view path, bool remove_existing);
    void configure_for_testing(std::string_view path, bool remove_existing);
    void close();
    size_t size() const;

    bool insert(raw_outpoint const& key, output_data_span value, uint32_t height);
    bytes_opt find(raw_outpoint const& key, uint32_t height) const;
    size_t erase(raw_outpoint const& key, uint32_t height);

    std::pair<uint32_t, std::vector<deferred_deletion_entry>> process_pending_deletions();
    size_t deferred_deletions_size() const;

    std::pair<flat_map<raw_outpoint, bytes>, std::vector<deferred_lookup_entry>> process_pending_lookups();
    size_t deferred_lookups_size() const;

    void compact_all();
    void for_each_key_impl(void(*cb)(void*, raw_outpoint const&), void* ctx) const;
    void for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, std::span<uint8_t const>), void* ctx) const;

    database_statistics get_statistics();
    void print_statistics();
    sizing_report get_sizing_report() const;
    void print_sizing_report() const;
    void print_height_range_stats() const;
    void reset_all_statistics();

    search_stats const& get_search_stats() const;
    void reset_search_stats();

    float get_cache_hit_rate() const;
    std::vector<std::pair<size_t, size_t>> get_cached_file_info() const;

private:
    // Template helpers for compile-time dispatch
    template<size_t N, typename Func, size_t... Is>
    static constexpr void for_each_index_impl(Func&& f, std::index_sequence<Is...>);

    template<size_t N, typename Func>
    static constexpr void for_each_index(Func&& f);

    // Variant helper for runtime->compile-time dispatch
    auto make_index_variant(size_t index);

    // Container access
    template<size_t Index> requires (Index < container_count)
    utxo_map<container_sizes[Index]>& container();

    template<size_t Index> requires (Index < container_count)
    utxo_map<container_sizes[Index]> const& container() const;

    // Core operations implementation
    template<size_t Index>
    bool insert_in_index(raw_outpoint const& key, output_data_span value, uint32_t height);

    // Find helpers
    bytes_opt find_in_latest_version(raw_outpoint const& key, uint32_t height) const;
    bytes_opt find_in_previous_versions(raw_outpoint const& key, uint32_t height);

    template<size_t Index>
    bytes_opt find_in_prev_versions(raw_outpoint const& key, uint32_t height);

    // Erase helpers
    size_t erase_in_latest_version(raw_outpoint const& key, uint32_t height);
    size_t erase_from_cached_files_only(raw_outpoint const& key, uint32_t height, size_t& search_depth);
    void add_to_deferred_deletions(raw_outpoint const& key, uint32_t height);
    size_t process_deferred_deletions_in_file(size_t container_index, size_t version, bool is_cached);

    // Deferred lookup helpers
    void add_to_deferred_lookups(raw_outpoint const& key, uint32_t height) const;
    void process_deferred_lookups_in_file(size_t container_index, size_t version, bool is_cached,
                                          flat_map<raw_outpoint, bytes>& successful_lookups);

    // File management
    template<size_t Index>
    void open_or_create_container(size_t version);

    template<size_t Index>
    void close_container();

    template<size_t Index>
    void new_version();

    template<size_t Index>
    std::unique_ptr<bip::managed_mapped_file> open_container_file(size_t version);

    // Safety checks
    template<size_t Index>
    bool can_insert_safely() const;

    template<size_t Index>
    bool can_insert_safely_in_map(utxo_map<container_sizes[Index]> const& map,
                                   bip::managed_mapped_file const& segment) const;

    template<size_t Index>
    bool should_rotate() const;

    template<size_t Index>
    float next_load_factor() const;

    // Compaction
    template<size_t Index>
    void compact_container();

    // Optimal buckets finder
    template<size_t Index>
    size_t find_optimal_buckets(std::string const& file_path, size_t file_size, size_t initial_buckets);

    // Utilities
    size_t get_index_from_size(size_t size) const;
    size_t find_latest_version_from_files(size_t index) const;
    size_t count_versions_for_container(size_t index) const;
    size_t estimate_memory_usage(size_t index) const;

    // Internal configuration
    void configure_internal(std::string_view path, bool remove_existing);

    // Metadata management
    void update_metadata_on_insert(size_t index, size_t version, raw_outpoint const& key, uint32_t height);
    void update_metadata_on_delete(size_t index, size_t version);
    void save_metadata_to_disk(size_t index, size_t version);
    void load_metadata_from_disk(size_t index, size_t version);

    // Statistics
    void update_fragmentation_stats();

    // Member variables
    fs::path db_path_;
    std::array<size_t, container_count> active_file_sizes_ = file_sizes; // Can be changed for testing
    std::array<std::unique_ptr<bip::managed_mapped_file>, container_count> segments_;
    std::array<void*, container_count> containers_{};
    std::array<size_t, container_count> current_versions_{};
    std::array<size_t, container_count> min_buckets_ok_{};

    size_t entries_count_ = 0;

    // Metadata and caching
    std::array<std::vector<file_metadata>, container_count> file_metadata_;
    std::unique_ptr<file_cache> file_cache_;

    // Statistics (mutable to allow const find operations)
    mutable search_stats search_stats_;
    boost::unordered_flat_set<deferred_deletion_entry> deferred_deletions_;
    mutable boost::concurrent_flat_set<deferred_lookup_entry> deferred_lookups_;
    std::array<container_stats, container_count> container_stats_;
    height_range_stats height_range_stats_;
    deferred_stats deferred_stats_;
    not_found_stats not_found_stats_;
    utxo_lifetime_stats lifetime_stats_;
    fragmentation_stats fragmentation_stats_;
};

} // namespace utxoz::detail
