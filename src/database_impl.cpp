// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file database_impl.cpp
 * @brief Database implementation - migrated from interprocess_multiple_v12.hpp
 */

#include "detail/database_impl.hpp"

#include <utxoz/config.hpp>
#include <utxoz/utils.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <numeric>
#include <ranges>
#include <set>

#include <fmt/format.h>

#include "detail/log.hpp"

namespace utxoz::detail {

// =============================================================================
// Template helpers for compile-time dispatch
// =============================================================================

template<size_t N, typename Func, size_t... Is>
constexpr void database_impl::for_each_index_impl(Func&& f, std::index_sequence<Is...>) {
    (f(std::integral_constant<size_t, Is>{}), ...);
}

template<size_t N, typename Func>
constexpr void database_impl::for_each_index(Func&& f) {
    for_each_index_impl<N>(std::forward<Func>(f), std::make_index_sequence<N>{});
}

auto database_impl::make_index_variant(size_t index) {
    using variant_t = std::variant<
        std::integral_constant<size_t, 0>,
        std::integral_constant<size_t, 1>,
        std::integral_constant<size_t, 2>,
        std::integral_constant<size_t, 3>
    >;

    switch (index) {
        case 0: return variant_t{std::integral_constant<size_t, 0>{}};
        case 1: return variant_t{std::integral_constant<size_t, 1>{}};
        case 2: return variant_t{std::integral_constant<size_t, 2>{}};
        case 3: return variant_t{std::integral_constant<size_t, 3>{}};
        default: throw std::out_of_range("Invalid container index");
    }
}

// =============================================================================
// database_impl - Destructor
// =============================================================================

database_impl::~database_impl() {
    close();
}

// =============================================================================
// database_impl - Container access
// =============================================================================

template<size_t Index> requires (Index < container_count)
utxo_map<container_sizes[Index]>& database_impl::container() {
    return *static_cast<utxo_map<container_sizes[Index]>*>(containers_[Index]);
}

template<size_t Index> requires (Index < container_count)
utxo_map<container_sizes[Index]> const& database_impl::container() const {
    return *static_cast<utxo_map<container_sizes[Index]> const*>(containers_[Index]);
}

// =============================================================================
// database_impl - Utilities
// =============================================================================

size_t database_impl::get_index_from_size(size_t size) const {
    for (size_t i = 0; i < container_count; ++i) {
        if (size <= container_sizes[i]) return i;
    }
    return container_count;
}

size_t database_impl::find_latest_version_from_files(size_t index) const {
    size_t version = 0;
    while (fs::exists(fmt::format(data_file_format, db_path_.string(), index, version))) {
        ++version;
    }
    return version > 0 ? version - 1 : 0;
}

size_t database_impl::count_versions_for_container(size_t index) const {
    size_t version = 0;
    while (fs::exists(fmt::format(data_file_format, db_path_.string(), index, version))) {
        ++version;
    }
    return version;
}

// =============================================================================
// database_impl - Optimal groups calculation
// =============================================================================

template<size_t Index>
void database_impl::find_optimal_groups(size_t file_size) {
    // Mathematical calculation of the largest groups_size (power-of-2)
    // whose table buffer fits in the available space after the file header.
    //
    // mmap_flat_map internal layout (from boost/unordered/detail/foa/core.hpp):
    //   group_type: 16 bytes (group15, alignas(16), N=15 slots)
    //   groups_size: gs (power of 2, minimum 2)
    //   groups_size_mask: gsm = gs - 1
    //   groups_size_index: gsi = 64 - countr_zero(gs)  (shift count for position())
    //   capacity = gs * N - 1
    //   buffer_bytes = gs * sizeof(group_type) + capacity * sizeof(value_type)

    using map_t = utxo_map<container_sizes[Index]>;
    using value_type = typename map_t::value_type;
    constexpr size_t N = 15;            // group15::N
    constexpr size_t group_sz = 16;     // sizeof(group15)
    size_t available = file_size - sizeof(file_header);

    // Find largest power-of-2 gs that fits (minimum gs = 2, per pow2_size_policy).
    size_t best_gs = 2;
    for (size_t gs = 2; gs <= available; gs *= 2) {
        size_t capacity = gs * N - 1;
        size_t buffer_bytes = gs * group_sz + capacity * sizeof(value_type);

        if (buffer_bytes <= available) {
            best_gs = gs;
        } else {
            break;
        }
    }

    // gsi = shift count used by pow2_size_policy::position(hash, gsi) = hash >> gsi
    size_t best_gsm = best_gs - 1;
    size_t best_gsi = sizeof(size_t) * CHAR_BIT - std::countr_zero(best_gs);

    groups_size_index_[Index] = best_gsi;
    groups_size_mask_[Index] = best_gsm;

    size_t capacity = best_gs * N - 1;
    log::debug("Optimal groups for container {} (file size: {}): "
               "gs={}, gsi={}, gsm={}, capacity={}",
               Index, file_size, best_gs, best_gsi, best_gsm, capacity);
}

// =============================================================================
// database_impl - File management
// =============================================================================

template<size_t Index>
void database_impl::open_or_create_container(size_t version) {
    auto file_name = fmt::format(data_file_format, db_path_.string(), Index, version);

    using map_t = utxo_map<container_sizes[Index]>;

    bool file_exists = fs::exists(file_name);

    if (file_exists) {
        // Reopen existing file — attach to the table already in the buffer
        segments_[Index] = std::make_unique<mapped_segment>(file_name, open_only);
        auto& hdr = segments_[Index]->header();

        auto* map = new map_t(map_t::attach(
            segments_[Index]->table_buffer(),
            hdr.groups_size_index,
            hdr.groups_size_mask,
            hdr.entry_count,
            hdr.max_load,
            outpoint_hash{},
            outpoint_equal{}));

        containers_[Index] = map;
        entries_count_ += hdr.entry_count;
    } else {
        // Create new file
        segments_[Index] = std::make_unique<mapped_segment>(
            file_name, active_file_sizes_[Index], open_or_create);
        auto& hdr = segments_[Index]->header();

        hdr.container_index = static_cast<uint32_t>(Index);
        hdr.file_version = version;
        hdr.groups_size_index = groups_size_index_[Index];
        hdr.groups_size_mask = groups_size_mask_[Index];

        auto* map = new map_t(map_t::create(
            segments_[Index]->table_buffer(),
            groups_size_index_[Index],
            groups_size_mask_[Index],
            outpoint_hash{},
            outpoint_equal{}));

        containers_[Index] = map;
    }

    current_versions_[Index] = version;
}

template<size_t Index>
void database_impl::close_container() {
    if (segments_[Index]) {
        if (containers_[Index]) {
            // Save scalar state to the file header
            auto& map = container<Index>();
            auto& hdr = segments_[Index]->header();
            hdr.entry_count = map.size();
            hdr.max_load = map.get_max_load();
            hdr.groups_size_index = map.get_groups_size_index();
            hdr.groups_size_mask = map.get_groups_size_mask();

            // No-op destructor — data stays in the mmap'd file
            delete static_cast<utxo_map<container_sizes[Index]>*>(containers_[Index]);
            containers_[Index] = nullptr;
        }

        save_metadata_to_disk(Index, current_versions_[Index]);
        segments_[Index]->flush();
        segments_[Index].reset();
    }
}

template<size_t Index>
void database_impl::new_version() {
    close_container<Index>();
    ++current_versions_[Index];

    // Initialize metadata for new version
    if (file_metadata_[Index].size() <= current_versions_[Index]) {
        file_metadata_[Index].resize(current_versions_[Index] + 1);
    }
    file_metadata_[Index][current_versions_[Index]] = file_metadata{};

    open_or_create_container<Index>(current_versions_[Index]);
    log::debug("Container {} rotated to version {}", Index, current_versions_[Index]);
}

template<size_t Index>
std::unique_ptr<mapped_segment> database_impl::open_container_file(size_t version) {
    auto file_name = fmt::format(data_file_format, db_path_.string(), Index, version);
    return std::make_unique<mapped_segment>(file_name, open_only);
}

// =============================================================================
// database_impl - Safety checks
// =============================================================================

template<size_t Index>
bool database_impl::can_insert_safely() const {
    auto const& map = container<Index>();

    // Only load factor check — the table has a fixed capacity in the mmap'd file.
    // When size >= max_load, the table is full.
    return map.size() + 1 <= map.get_max_load();
}

template<size_t Index>
bool database_impl::can_insert_safely_in_map(utxo_map<container_sizes[Index]> const& map) const {
    return map.size() + 1 <= map.get_max_load();
}

template<size_t Index>
bool database_impl::should_rotate() const {
    return !can_insert_safely<Index>();
}

template<size_t Index>
float database_impl::next_load_factor() const {
    auto const& map = container<Index>();
    auto cap = map.capacity();
    if (cap == 0) return 0.0f;
    return float(map.size() + 1) / float(cap);
}

// =============================================================================
// database_impl - Metadata management
// =============================================================================

void database_impl::update_metadata_on_insert(size_t index, size_t version,
                                               raw_outpoint const& key, uint32_t height) {
    if (file_metadata_[index].size() <= version) {
        file_metadata_[index].resize(version + 1);
    }
    file_metadata_[index][version].update_on_insert(key, height);
}

void database_impl::update_metadata_on_delete(size_t index, size_t version) {
    if (file_metadata_[index].size() > version) {
        file_metadata_[index][version].update_on_delete();
    }
}

void database_impl::save_metadata_to_disk(size_t index, size_t version) {
    // TODO: Implement actual saving to disk
    auto metadata_file = fmt::format("{}/meta_{}_{:05}.dat", db_path_.string(), index, version);
    (void)metadata_file;
}

void database_impl::load_metadata_from_disk(size_t index, size_t version) {
    // TODO: Implement actual loading from disk
    auto metadata_file = fmt::format("{}/meta_{}_{:05}.dat", db_path_.string(), index, version);
    (void)metadata_file;
}

// =============================================================================
// database_impl - Public interface: configure, close, size
// =============================================================================

void database_impl::configure(std::string_view path, bool remove_existing) {
    active_file_sizes_ = file_sizes;
    configure_internal(path, remove_existing);
}

void database_impl::configure_for_testing(std::string_view path, bool remove_existing) {
    active_file_sizes_ = test_file_sizes;
    configure_internal(path, remove_existing);
}

void database_impl::configure_internal(std::string_view path, bool remove_existing) {
    db_path_ = path;

    if (remove_existing && fs::exists(path)) {
        fs::remove_all(path);
    }
    fs::create_directories(path);

    // Initialize file cache
    file_cache_ = std::make_unique<file_cache>(std::string(path));

    // Calculate optimal groups for each container (pure math, no temp files)
    static_assert(container_count == 4);
    find_optimal_groups<0>(active_file_sizes_[0]);
    find_optimal_groups<1>(active_file_sizes_[1]);
    find_optimal_groups<2>(active_file_sizes_[2]);
    find_optimal_groups<3>(active_file_sizes_[3]);

    // Initialize containers
    entries_count_ = 0;
    for_each_index<container_count>([&](auto I) {
        size_t latest_version = find_latest_version_from_files(I);
        open_or_create_container<I>(latest_version);

        // Load metadata for all versions
        for (size_t v = 0; v <= latest_version; ++v) {
            load_metadata_from_disk(I, v);
        }
    });
}

void database_impl::close() {
    for_each_index<container_count>([&](auto I) {
        close_container<I>();
    });
}

size_t database_impl::size() const {
    return entries_count_;
}

// =============================================================================
// database_impl - Insert
// =============================================================================

bool database_impl::insert(raw_outpoint const& key, output_data_span value, uint32_t height) {
    size_t const index = get_index_from_size(value.size());
    if (index >= container_count) {
        log::error("insert: value too large ({} bytes) for any container (max {}). height={}, outpoint={}",
            value.size(), container_sizes[container_count - 1], height, outpoint_to_string(key));
        throw std::out_of_range("Value size too large");
    }

    return std::visit([&](auto ic) {
        return insert_in_index<ic>(key, value, height);
    }, make_index_variant(index));
}

template<size_t Index>
bool database_impl::insert_in_index(raw_outpoint const& key, output_data_span value, uint32_t height) {
    // Check if rotation needed
    if (!can_insert_safely<Index>()) {
        log::debug("Rotating container {} (table full)", Index);
        new_version<Index>();
    }

    // Prepare value
    utxo_value<container_sizes[Index]> val;
    val.block_height = height;
    val.set_data(value);

    auto& map = container<Index>();

    auto [it, inserted] = map.emplace(key, val);
    if (!inserted) {
        log::warn("insert: duplicate key at height {}, outpoint={}, container={}",
            height, outpoint_to_string(key), Index);
    }
    if (inserted) {
        ++entries_count_;

#ifdef UTXOZ_STATISTICS_ENABLED
        ++container_stats_[Index].total_inserts;
        ++container_stats_[Index].current_size;
        ++container_stats_[Index].value_size_distribution[value.size()];
#endif

        update_metadata_on_insert(Index, current_versions_[Index], key, height);
    }
    return inserted;
}

// =============================================================================
// database_impl - Find
// =============================================================================

bytes_opt database_impl::find(raw_outpoint const& key, uint32_t height) const {
    // Try current version first
    if (auto res = find_in_latest_version(key, height); res) {
        return res;
    }

    // Defer lookup to batch processing for efficiency
    add_to_deferred_lookups(key, height);
    return std::nullopt;
}

bytes_opt database_impl::find_in_latest_version(raw_outpoint const& key,
                                                                           uint32_t height) const {
    bytes_opt result;

    for_each_index<container_count>([&](auto I) {
        if (!result) {
            auto& map = container<I>();
            if (auto it = map.find(key); it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
                search_stats_.add_record(height, it->second.block_height, 0, false, true, 'f');
#endif
                auto data = it->second.get_data();
                result = bytes(data.begin(), data.end());
            }
        }
    });

    return result;
}

bytes_opt database_impl::find_in_previous_versions(raw_outpoint const& key,
                                                                              uint32_t height) {
    bytes_opt result;

    for_each_index<container_count>([&](auto I) {
        if (!result) {
            result = find_in_prev_versions<I>(key, height);
        }
    });

#ifdef UTXOZ_STATISTICS_ENABLED
    if (!result) {
        search_stats_.add_record(height, 0, 1, false, false, 'f');
    }
#endif

    return result;
}

template<size_t Index>
bytes_opt database_impl::find_in_prev_versions(raw_outpoint const& key,
                                                                          uint32_t height) {
    for (size_t v = current_versions_[Index]; v-- > 0;) {
        // Check metadata first
        if (file_metadata_[Index].size() > v && !file_metadata_[Index][v].key_in_range(key)) {
            continue;
        }

        auto [map, cache_hit] = file_cache_->get_or_open_file<Index>(Index, v);

        if (auto it = map.find(key); it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
            auto depth = static_cast<uint32_t>(current_versions_[Index] - v);
            search_stats_.add_record(height, it->second.block_height, depth, cache_hit, true, 'f');
#endif
            auto data = it->second.get_data();
            return bytes(data.begin(), data.end());
        }
    }

    return std::nullopt;
}

// =============================================================================
// database_impl - Erase
// =============================================================================

size_t database_impl::erase(raw_outpoint const& key, uint32_t height) {
    size_t search_depth = 0;

    // Try current version first
    if (auto res = erase_in_latest_version(key, height); res > 0) {
        entries_count_ -= res;
        return res;
    }
    ++search_depth;

    // Try cached files only
    if (auto res = erase_from_cached_files_only(key, height, search_depth); res > 0) {
        entries_count_ -= res;
        return res;
    }

#ifdef UTXOZ_STATISTICS_ENABLED
    // Track not found
    ++not_found_stats_.total_not_found;
    not_found_stats_.total_search_depth += search_depth;
    not_found_stats_.max_search_depth = std::max(not_found_stats_.max_search_depth, search_depth);
    ++not_found_stats_.depth_distribution[search_depth];
#endif

    // Defer deletion
    add_to_deferred_deletions(key, height);
    return 0;
}

size_t database_impl::erase_in_latest_version(raw_outpoint const& key, uint32_t height) {
    size_t result = 0;

    for_each_index<container_count>([&](auto I) {
        if (result == 0) {
            auto& map = container<I>();
            if (auto it = map.find(key); it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
                // Track UTXO lifetime
                uint32_t age = height - it->second.block_height;
                ++lifetime_stats_.age_distribution[age];
                lifetime_stats_.max_age = std::max(lifetime_stats_.max_age, age);
                ++lifetime_stats_.total_spent;

                lifetime_stats_.average_age =
                    (lifetime_stats_.average_age * (lifetime_stats_.total_spent - 1) + age)
                    / lifetime_stats_.total_spent;

                search_stats_.add_record(height, it->second.block_height, 0, false, true, 'e');
#endif
                map.erase(it);

#ifdef UTXOZ_STATISTICS_ENABLED
                --container_stats_[I].current_size;
                ++container_stats_[I].total_deletes;
#endif

                result = 1;
            }
        }
    });

    return result;
}

size_t database_impl::erase_from_cached_files_only(raw_outpoint const& key, uint32_t height,
                                                    size_t& search_depth) {
    size_t result = 0;

    auto cached_files = file_cache_->get_cached_files();

    for (auto const& [container_index, version] : cached_files) {
        ++search_depth;

        auto process_file = [&]<size_t Index>(std::integral_constant<size_t, Index>) {
            if (file_cache_->is_cached(container_index, version)) {
                try {
                    auto [map, cache_hit] = file_cache_->get_or_open_file<Index>(container_index, version);

                    if (auto it = map.find(key); it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
                        uint32_t age = height - it->second.block_height;
                        ++lifetime_stats_.age_distribution[age];
                        lifetime_stats_.max_age = std::max(lifetime_stats_.max_age, age);
                        ++lifetime_stats_.total_spent;
                        lifetime_stats_.average_age =
                            (lifetime_stats_.average_age * (lifetime_stats_.total_spent - 1) + age)
                            / lifetime_stats_.total_spent;

                        search_stats_.add_record(height, it->second.block_height,
                            static_cast<uint32_t>(current_versions_[Index] - version), cache_hit, true, 'e');
#endif
                        map.erase(it);

#ifdef UTXOZ_STATISTICS_ENABLED
                        --container_stats_[Index].current_size;
                        ++container_stats_[Index].total_deletes;
#endif

                        update_metadata_on_delete(Index, version);
                        result = 1;
                    }
                } catch (std::exception const& e) {
                    log::error("Error accessing cached file ({}, v{}): {}",
                              container_index, version, e.what());
                }
            }
        };

        switch (container_index) {
            case 0: process_file(std::integral_constant<size_t, 0>{}); break;
            case 1: process_file(std::integral_constant<size_t, 1>{}); break;
            case 2: process_file(std::integral_constant<size_t, 2>{}); break;
            case 3: process_file(std::integral_constant<size_t, 3>{}); break;
        }

        if (result > 0) break;
    }

    return result;
}

// =============================================================================
// database_impl - Deferred deletions
// =============================================================================

void database_impl::add_to_deferred_deletions(raw_outpoint const& key, uint32_t height) {
    [[maybe_unused]] auto [it, inserted] = deferred_deletions_.emplace(key, height);
#ifdef UTXOZ_STATISTICS_ENABLED
    if (inserted) {
        ++deferred_stats_.total_deferred;
        deferred_stats_.max_queue_size = std::max(deferred_stats_.max_queue_size,
                                                   deferred_deletions_.size());

        for (size_t i = 0; i < container_count; ++i) {
            ++container_stats_[i].deferred_deletes;
        }
    }
#endif
}

size_t database_impl::deferred_deletions_size() const {
    return deferred_deletions_.size();
}

std::pair<uint32_t, std::vector<deferred_deletion_entry>> database_impl::process_pending_deletions() {
    if (deferred_deletions_.empty()) return {};

#ifdef UTXOZ_STATISTICS_ENABLED
    auto const start_time = std::chrono::steady_clock::now();
    ++deferred_stats_.processing_runs;
#endif

    size_t initial_size = deferred_deletions_.size();
    log::debug("Processing {} deferred deletions...", initial_size);

    size_t successful_deletions = 0;

    // Phase 1: Process cached files first
    auto cached_files = file_cache_->get_cached_files();
    if (!cached_files.empty()) {
        std::ranges::sort(cached_files, [](auto const& a, auto const& b) {
            if (a.first != b.first) return a.first < b.first;
            return a.second > b.second;
        });

        for (auto const& [container_index, version] : cached_files) {
            if (deferred_deletions_.empty()) break;
            successful_deletions += process_deferred_deletions_in_file(container_index, version, true);
        }
    }

    // Phase 2: Process remaining files
    if (!deferred_deletions_.empty()) {
        std::array<std::set<size_t>, container_count> processed_versions;
        for (auto const& [container_index, version] : cached_files) {
            processed_versions[container_index].insert(version);
        }

        for_each_index<container_count>([&](auto I) {
            if (deferred_deletions_.empty()) return;
            if (current_versions_[I.value] == 0) return;

            for (size_t v = current_versions_[I.value] - 1; v != SIZE_MAX; --v) {
                if (deferred_deletions_.empty()) break;
                if (processed_versions[I.value].contains(v)) continue;

                auto file_name = fmt::format(data_file_format, db_path_.string(), I.value, v);
                if (!fs::exists(file_name)) continue;

                successful_deletions += process_deferred_deletions_in_file(I.value, v, false);
            }
        });
    }

    // Collect failed deletions (includes key and block height that requested it)
    std::vector<deferred_deletion_entry> failed_deletions;
    failed_deletions.reserve(deferred_deletions_.size());
    for (auto const& entry : deferred_deletions_) {
        failed_deletions.push_back(entry);
    }

    deferred_deletions_.clear();

#ifdef UTXOZ_STATISTICS_ENABLED
    auto const end_time = std::chrono::steady_clock::now();
    deferred_stats_.total_processing_time +=
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    deferred_stats_.successfully_processed += successful_deletions;
    deferred_stats_.failed_to_delete += failed_deletions.size();
#endif

    log::debug("Deferred deletion complete: {} successful, {} failed",
              successful_deletions, failed_deletions.size());

    return {static_cast<uint32_t>(successful_deletions), std::move(failed_deletions)};
}

size_t database_impl::process_deferred_deletions_in_file(size_t container_index,
                                                          size_t version,
                                                          [[maybe_unused]] bool is_cached) {
    if (deferred_deletions_.empty()) return 0;

    size_t successful_deletions = 0;

    auto process_with_container = [&]<size_t Index>(std::integral_constant<size_t, Index>) -> size_t {
        try {
            auto [map, cache_hit] = file_cache_->get_or_open_file<Index>(container_index, version);

            auto it = deferred_deletions_.begin();
            while (it != deferred_deletions_.end()) {
                auto erased_count = map.erase(it->key);
                if (erased_count > 0) {
                    update_metadata_on_delete(Index, version);
#ifdef UTXOZ_STATISTICS_ENABLED
                    auto depth = static_cast<uint32_t>(current_versions_[Index] - version);

                    ++deferred_stats_.deletions_by_depth[depth];
                    search_stats_.add_record(it->height, 0, depth, cache_hit, true, 'e');
                    --container_stats_[Index].deferred_deletes;
#endif

                    it = deferred_deletions_.erase(it);
                    ++successful_deletions;
                } else {
                    ++it;
                }
            }

            return successful_deletions;

        } catch (std::exception const& e) {
            log::error("Error processing file ({}, v{}): {}", container_index, version, e.what());
            return 0;
        }
    };

    switch (container_index) {
        case 0: return process_with_container(std::integral_constant<size_t, 0>{});
        case 1: return process_with_container(std::integral_constant<size_t, 1>{});
        case 2: return process_with_container(std::integral_constant<size_t, 2>{});
        case 3: return process_with_container(std::integral_constant<size_t, 3>{});
        default: return 0;
    }
}

// =============================================================================
// database_impl - Deferred lookups
// =============================================================================

void database_impl::add_to_deferred_lookups(raw_outpoint const& key, uint32_t height) const {
    deferred_lookups_.emplace(key, height);
}

size_t database_impl::deferred_lookups_size() const {
    return deferred_lookups_.size();
}

std::pair<flat_map<raw_outpoint, bytes>, std::vector<deferred_lookup_entry>> database_impl::process_pending_lookups() {
    if (deferred_lookups_.empty()) return {};

    flat_map<raw_outpoint, bytes> successful_lookups;

#ifdef UTXOZ_STATISTICS_ENABLED
    auto const start_time = std::chrono::steady_clock::now();
    ++deferred_stats_.processing_runs;
#endif

    size_t initial_size = deferred_lookups_.size();
    log::debug("Processing {} deferred lookups...", initial_size);

    // Phase 1: Process cached files first
    auto cached_files = file_cache_->get_cached_files();
    if (!cached_files.empty()) {
        std::ranges::sort(cached_files, [](auto const& a, auto const& b) {
            if (a.first != b.first) return a.first < b.first;
            return a.second > b.second; // Most recent version first
        });

        for (auto const& [container_index, version] : cached_files) {
            if (deferred_lookups_.empty()) break;
            process_deferred_lookups_in_file(container_index, version, true, successful_lookups);
        }
    }

    // Phase 2: Process remaining files
    if (!deferred_lookups_.empty()) {
        std::array<std::set<size_t>, container_count> processed_versions;
        for (auto const& [container_index, version] : cached_files) {
            processed_versions[container_index].insert(version);
        }

        for_each_index<container_count>([&](auto I) {
            if (deferred_lookups_.empty()) return;
            if (current_versions_[I.value] == 0) return;

            for (size_t v = current_versions_[I.value] - 1; v != SIZE_MAX; --v) {
                if (deferred_lookups_.empty()) break;
                if (processed_versions[I.value].contains(v)) continue;

                auto file_name = fmt::format(data_file_format, db_path_.string(), I.value, v);
                if (!fs::exists(file_name)) continue;

                process_deferred_lookups_in_file(I.value, v, false, successful_lookups);
            }
        });
    }

    // Collect failed lookups (includes key and block height)
    std::vector<deferred_lookup_entry> failed_lookups;
    failed_lookups.reserve(deferred_lookups_.size());
    deferred_lookups_.visit_all([&](auto const& entry) {
        failed_lookups.push_back(entry);
    });

    deferred_lookups_.clear();

#ifdef UTXOZ_STATISTICS_ENABLED
    auto const end_time = std::chrono::steady_clock::now();
    deferred_stats_.total_processing_time +=
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    deferred_stats_.successfully_processed += successful_lookups.size();
    deferred_stats_.failed_to_delete += failed_lookups.size();
#endif

    log::debug("Deferred lookup complete: {} successful, {} failed",
              successful_lookups.size(), failed_lookups.size());

    return {std::move(successful_lookups), std::move(failed_lookups)};
}

void database_impl::process_deferred_lookups_in_file(size_t container_index,
                                                      size_t version,
                                                      [[maybe_unused]] bool is_cached,
                                                      flat_map<raw_outpoint, bytes>& successful_lookups) {
    if (deferred_lookups_.empty()) return;

    auto process_with_container = [&]<size_t Index>(std::integral_constant<size_t, Index>) {
        try {
            auto [map, cache_hit] = file_cache_->get_or_open_file<Index>(container_index, version);

            deferred_lookups_.erase_if([&](auto const& entry) {
                auto map_it = map.find(entry.key);
                if (map_it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
                    auto depth = static_cast<uint32_t>(current_versions_[Index] - version);

                    ++deferred_stats_.lookups_by_depth[depth];
                    search_stats_.add_record(entry.height, map_it->second.block_height, depth, cache_hit, true, 'f');
#endif

                    auto data = map_it->second.get_data();
                    successful_lookups.emplace(entry.key, bytes(data.begin(), data.end()));

                    return true;  // Remove this entry
                }
                return false;  // Keep this entry
            });

        } catch (std::exception const& e) {
            log::error("Error processing lookups in file ({}, v{}): {}", container_index, version, e.what());
        }
    };

    switch (container_index) {
        case 0: process_with_container(std::integral_constant<size_t, 0>{}); break;
        case 1: process_with_container(std::integral_constant<size_t, 1>{}); break;
        case 2: process_with_container(std::integral_constant<size_t, 2>{}); break;
        case 3: process_with_container(std::integral_constant<size_t, 3>{}); break;
    }
}

// =============================================================================
// database_impl - Compaction
// =============================================================================

template<size_t Index>
void database_impl::compact_container() {
    log::debug("Starting compaction for container {}...", Index);

    size_t files_deleted = 0;
    size_t entries_moved = 0;

    // Save active entry count before closing — entries_count_ already includes it.
    // We'll subtract it before reopening to avoid double-counting.
    size_t old_active_count = 0;
    if (containers_[Index]) {
        old_active_count = container<Index>().size();
    }

    close_container<Index>();
    file_cache_->clear();

    size_t total_versions = count_versions_for_container(Index);
    if (total_versions <= 1) {
        log::trace("Container {} has {} files, no compaction needed", Index, total_versions);
        entries_count_ -= old_active_count;
        open_or_create_container<Index>(total_versions > 0 ? total_versions - 1 : 0);
        return;
    }

    // Helper to save map state to its segment header
    auto save_map_state = [](mapped_segment& seg, auto const& map) {
        auto& h = seg.header();
        h.entry_count = map.size();
        h.max_load = map.get_max_load();
    };

    // Helper to attach a map from a segment
    using map_t = utxo_map<container_sizes[Index]>;
    auto attach_map = [](mapped_segment& seg) {
        auto& h = seg.header();
        return std::make_unique<map_t>(map_t::attach(
            seg.table_buffer(),
            h.groups_size_index, h.groups_size_mask,
            h.entry_count, h.max_load,
            outpoint_hash{}, outpoint_equal{}));
    };

    // Open target and source segments, attach maps
    size_t target_idx = 0;
    size_t source_idx = 1;

    auto target_segment = open_container_file<Index>(target_idx);
    auto source_segment = open_container_file<Index>(source_idx);

    auto target_map = attach_map(*target_segment);
    auto source_map = attach_map(*source_segment);

    while (source_idx < total_versions) {
        auto it = source_map->begin();

        while (it != source_map->end()) {
            if (!can_insert_safely_in_map<Index>(*target_map)) {
                log::trace("Target file {} is full, rotating...", target_idx);

                // Save target state and release — access header through
                // the segment directly (not a cached reference).
                save_map_state(*target_segment, *target_map);
                target_map.reset();
                target_segment->flush();
                target_segment.reset();

                target_idx = source_idx;
                target_segment = std::move(source_segment);
                target_map = std::move(source_map);

                ++source_idx;
                if (source_idx >= total_versions) break;

                source_segment = open_container_file<Index>(source_idx);
                source_map = attach_map(*source_segment);
                break;
            }

            auto key = it->first;
            auto value = it->second;

            target_map->emplace(key, value);
            it = source_map->erase(it);
            ++entries_moved;
        }

        if (source_map && source_map->empty()) {
            // Save source state, release, and delete file
            save_map_state(*source_segment, *source_map);
            source_map.reset();
            source_segment.reset();

            auto source_path = fmt::format(data_file_format, db_path_.string(), Index, source_idx);
            fs::remove(source_path);
            ++files_deleted;

            for (size_t i = source_idx + 1; i < total_versions; ++i) {
                auto old_path = fmt::format(data_file_format, db_path_.string(), Index, i);
                auto new_path = fmt::format(data_file_format, db_path_.string(), Index, i - 1);
                fs::rename(old_path, new_path);
            }

            --total_versions;

            if (source_idx < total_versions) {
                source_segment = open_container_file<Index>(source_idx);
                source_map = attach_map(*source_segment);
            }
        }
    }

    // Save and release remaining segments
    if (target_map && target_segment) {
        save_map_state(*target_segment, *target_map);
        target_map.reset();
        target_segment->flush();
    }
    target_segment.reset();

    if (source_map && source_segment) {
        save_map_state(*source_segment, *source_map);
        source_map.reset();
        source_segment->flush();
    }
    source_segment.reset();

    current_versions_[Index] = total_versions > 0 ? total_versions - 1 : 0;
    entries_count_ -= old_active_count;
    open_or_create_container<Index>(current_versions_[Index]);

    log::debug("Compaction complete for container {}: {} files deleted, {} entries moved",
              Index, files_deleted, entries_moved);
}

void database_impl::compact_all() {
    log::info("Starting full database compaction...");

    for_each_index<container_count>([&](auto I) {
        compact_container<I>();
    });

    log::info("Full database compaction complete");
}

// =============================================================================
// database_impl - Statistics
// =============================================================================

void database_impl::update_fragmentation_stats() {
#ifdef UTXOZ_STATISTICS_ENABLED
    for_each_index<container_count>([&](auto I) {
        if (segments_[I]) {
            auto const& map = container<I>();
            size_t total_size = active_file_sizes_[I];
            size_t table_buf_size = segments_[I]->table_buffer_size();
            size_t used_entries = map.size();
            size_t capacity = map.capacity();

            fragmentation_stats_.fill_ratios[I] =
                capacity > 0 ? double(used_entries) / double(capacity) : 0.0;

            size_t ideal_size = used_entries * sizeof(typename utxo_map<container_sizes[I]>::value_type);
            fragmentation_stats_.wasted_space[I] =
                table_buf_size > ideal_size ? table_buf_size - ideal_size : 0;

            (void)total_size;
        }
    });
#endif
}

size_t database_impl::estimate_memory_usage(size_t index) const {
    size_t total = 0;

    if (segments_[index]) {
        total += active_file_sizes_[index];
    }

    for (size_t v = 0; v < current_versions_[index]; ++v) {
        auto file_name = fmt::format(data_file_format, db_path_.string(), index, v);
        if (fs::exists(file_name)) {
            total += fs::file_size(file_name);
        }
    }

    return total;
}

database_statistics database_impl::get_statistics() {
    update_fragmentation_stats();

    database_statistics stats;

    stats.total_entries = entries_count_;
    stats.cache_hit_rate = get_cache_hit_rate();
    stats.cached_files_count = file_cache_ ? file_cache_->get_cached_files().size() : 0;
    stats.cached_files_info = get_cached_file_info();
    stats.search = search_stats_.get_summary();

    stats.total_inserts = 0;
    stats.total_deletes = 0;

    for (size_t i = 0; i < container_count; ++i) {
        stats.containers[i] = container_stats_[i];
        stats.total_inserts += container_stats_[i].total_inserts;
        stats.total_deletes += container_stats_[i].total_deletes;
        stats.rotations_per_container[i] = current_versions_[i];
        stats.memory_usage_per_container[i] = estimate_memory_usage(i);
    }

    stats.deferred = deferred_stats_;
    stats.not_found = not_found_stats_;
    stats.lifetime = lifetime_stats_;
    stats.fragmentation = fragmentation_stats_;

    return stats;
}

void database_impl::print_statistics() {
    auto stats = get_statistics();

    log::info("=== UTXO Database Statistics ===");
    log::info("Total entries: {}", stats.total_entries);
    log::info("Total inserts: {}", stats.total_inserts);
    log::info("Total deletes: {}", stats.total_deletes);

    log::info("--- Container Statistics ---");
    for (size_t i = 0; i < container_count; ++i) {
        log::info("Container {} (size <= {} bytes):", i, container_sizes[i]);
        log::info("  Current entries: {}", stats.containers[i].current_size);
        log::info("  Total inserts: {}", stats.containers[i].total_inserts);
        log::info("  Total deletes: {}", stats.containers[i].total_deletes);
        log::info("  File rotations: {}", stats.rotations_per_container[i]);
        log::info("  Est. memory: {:.2f} MB", stats.memory_usage_per_container[i] / (1024.0*1024.0));
    }

    log::info("--- Cache Statistics ---");
    log::info("Cache hit rate: {:.2f}%", stats.cache_hit_rate * 100);
    log::info("Cached files: {}", stats.cached_files_count);

    log::info("--- Search Performance ---");
    log::info("Hit rate: {:.2f}%", stats.search.hit_rate * 100);
    log::info("Avg depth: {:.2f}", stats.search.avg_depth);

    log::info("================================");
}

void database_impl::reset_all_statistics() {
    for (auto& cs : container_stats_) {
        cs = container_stats{};
    }
    deferred_stats_ = deferred_stats{};
    not_found_stats_ = not_found_stats{};
    lifetime_stats_ = utxo_lifetime_stats{};
    fragmentation_stats_ = fragmentation_stats{};
    reset_search_stats();
}

search_stats const& database_impl::get_search_stats() const {
    return search_stats_;
}

void database_impl::reset_search_stats() {
    search_stats_.reset();
}

float database_impl::get_cache_hit_rate() const {
    return file_cache_ ? file_cache_->get_hit_rate() : 0.0f;
}

std::vector<std::pair<size_t, size_t>> database_impl::get_cached_file_info() const {
    return file_cache_ ? file_cache_->get_cached_files() : std::vector<std::pair<size_t, size_t>>{};
}

// =============================================================================
// Explicit template instantiations
// =============================================================================

template void database_impl::open_or_create_container<0>(size_t);
template void database_impl::open_or_create_container<1>(size_t);
template void database_impl::open_or_create_container<2>(size_t);
template void database_impl::open_or_create_container<3>(size_t);

template void database_impl::close_container<0>();
template void database_impl::close_container<1>();
template void database_impl::close_container<2>();
template void database_impl::close_container<3>();

template void database_impl::new_version<0>();
template void database_impl::new_version<1>();
template void database_impl::new_version<2>();
template void database_impl::new_version<3>();

template void database_impl::compact_container<0>();
template void database_impl::compact_container<1>();
template void database_impl::compact_container<2>();
template void database_impl::compact_container<3>();

template bool database_impl::insert_in_index<0>(raw_outpoint const&, output_data_span, uint32_t);
template bool database_impl::insert_in_index<1>(raw_outpoint const&, output_data_span, uint32_t);
template bool database_impl::insert_in_index<2>(raw_outpoint const&, output_data_span, uint32_t);
template bool database_impl::insert_in_index<3>(raw_outpoint const&, output_data_span, uint32_t);

template bytes_opt database_impl::find_in_prev_versions<0>(raw_outpoint const&, uint32_t);
template bytes_opt database_impl::find_in_prev_versions<1>(raw_outpoint const&, uint32_t);
template bytes_opt database_impl::find_in_prev_versions<2>(raw_outpoint const&, uint32_t);
template bytes_opt database_impl::find_in_prev_versions<3>(raw_outpoint const&, uint32_t);

} // namespace utxoz::detail
