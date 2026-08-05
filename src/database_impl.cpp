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
#include <fstream>
#include <numeric>
#include <ranges>
#include <optional>
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
        std::integral_constant<size_t, 3>,
        std::integral_constant<size_t, 4>
    >;

    switch (index) {
        case 0: return variant_t{std::integral_constant<size_t, 0>{}};
        case 1: return variant_t{std::integral_constant<size_t, 1>{}};
        case 2: return variant_t{std::integral_constant<size_t, 2>{}};
        case 3: return variant_t{std::integral_constant<size_t, 3>{}};
        case 4: return variant_t{std::integral_constant<size_t, 4>{}};
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
        if (size <= container_capacities[i]) return i;
    }
    return container_count;
}

// =============================================================================
// database_impl - Optimal buckets finder
// =============================================================================

template<size_t Index>
size_t database_impl::find_optimal_buckets(std::string const& file_path,
                                           size_t file_size,
                                           size_t initial_buckets) {
    log::debug("Finding optimal buckets for container {} (file size: {})...", Index, file_size);

    size_t left = 1;
    size_t right = initial_buckets;
    size_t best_buckets = left;

    while (left <= right) {
        size_t mid = left + (right - left) / 2;

        std::string temp_file = fmt::format("{}/temp_{}_{}.dat", file_path, file_size, mid);
        try {
            bip::managed_mapped_file segment(bip::open_or_create, temp_file.c_str(), file_size);

            // Just test if construction succeeds - we don't need the pointer
            (void)segment.find_or_construct<utxo_map<container_sizes[Index]>>("temp_map")(
                mid,
                outpoint_hash{},
                outpoint_equal{},
                segment.get_allocator<std::pair<raw_outpoint const, utxo_value<container_sizes[Index]>>>()
            );

            // Success - try more buckets
            best_buckets = mid;
            left = mid + 1;
            log::trace("  {} buckets OK, trying more...", mid);
        } catch (boost::interprocess::bad_alloc const&) {
            // Too many - try fewer
            right = mid - 1;
        }

        fs::remove(temp_file);
    }

    log::debug("Optimal buckets for container {}: {}", Index, best_buckets);
    return best_buckets;
}

// =============================================================================
// database_impl - File management
// =============================================================================

template<size_t Index>
void database_impl::open_or_create_container(size_t version) {
    auto file_name = fmt::format(data_file_format, db_path_.string(), Index, version);

    segments_[Index] = std::make_unique<bip::managed_mapped_file>(
        bip::open_or_create, file_name.c_str(), active_file_sizes_[Index]);

    auto* segment = segments_[Index].get();
    containers_[Index] = segment->find_or_construct<utxo_map<container_sizes[Index]>>("db_map")(
        min_buckets_ok_[Index],
        outpoint_hash{},
        outpoint_equal{},
        segment->get_allocator<typename utxo_map<container_sizes[Index]>::value_type>()
    );

    current_versions_[Index] = version;
}

template<size_t Index>
void database_impl::close_container() {
    if (segments_[Index]) {
        save_metadata_to_disk(Index, current_versions_[Index]);
        segments_[Index]->flush();
        segments_[Index].reset();
        containers_[Index] = nullptr;
    }
}

template<size_t Index>
void database_impl::new_version() {
    close_container<Index>();

    // The file first, the catalogue after. Publishing the identity before the
    // file exists leaves a version in the catalogue that nothing on disk backs
    // if the open throws — a phantom every later traversal would try to visit.
    // This is not durable creation, which belongs with the barrier work; it only
    // keeps the in-memory catalogue from describing something that is not there.
    auto const next = catalogs_[Index].next_version();
    open_or_create_container<Index>(next);   // sets current_versions_ once it maps

    catalogs_[Index].add(next);
    catalogs_[Index].metadata(next) = file_metadata{};
    log::debug("Container {} rotated to version {}", Index, current_versions_[Index]);
}

template<size_t Index>
std::unique_ptr<bip::managed_mapped_file> database_impl::open_container_file(size_t version) {
    auto file_name = fmt::format(data_file_format, db_path_.string(), Index, version);
    return std::make_unique<bip::managed_mapped_file>(bip::open_only, file_name.c_str());
}

// =============================================================================
// database_impl - Safety checks
// =============================================================================

template<size_t Index>
bool database_impl::can_insert_safely() const {
    auto const& map = container<Index>();

    // Check load factor
    if (map.bucket_count() > 0) {
        float next_load = float(map.size() + 1) / float(map.bucket_count());
        if (next_load >= map.max_load_factor() * 0.95f) {
            return false;
        }
    }

    // Check available memory
    if (segments_[Index]) {
        try {
            size_t free_memory = segments_[Index]->get_free_memory();
            size_t entry_size = sizeof(typename utxo_map<container_sizes[Index]>::value_type);
            size_t buffer_size = entry_size * 10; // Safety buffer

            return free_memory > buffer_size;
        } catch (...) {
            return false;
        }
    }

    return true;
}

template<size_t Index>
bool database_impl::can_insert_safely_in_map(utxo_map<container_sizes[Index]> const& map,
                                              bip::managed_mapped_file const& segment) const {
    if (map.bucket_count() > 0) {
        float next_load = float(map.size() + 1) / float(map.bucket_count());
        if (next_load >= map.max_load_factor() * 0.95f) {
            return false;
        }
    }

    try {
        size_t free_memory = segment.get_free_memory();
        size_t entry_size = sizeof(typename utxo_map<container_sizes[Index]>::value_type);
        size_t buffer_size = entry_size * 100; // Larger buffer for compaction

        return free_memory > buffer_size;
    } catch (...) {
        return false;
    }
}

template<size_t Index>
bool database_impl::should_rotate() const {
    auto const& map = container<Index>();
    if (map.bucket_count() == 0) return false;
    return !can_insert_safely<Index>();
}

template<size_t Index>
float database_impl::next_load_factor() const {
    auto const& map = container<Index>();
    if (map.bucket_count() == 0) return 0.0f;
    return float(map.size() + 1) / float(map.bucket_count());
}

// =============================================================================
// database_impl - Metadata management
// =============================================================================

void database_impl::update_metadata_on_insert(size_t index, size_t version,
                                               raw_outpoint const& key, uint32_t height) {
    catalogs_[index].metadata(version).update_on_insert(key, height);
}

void database_impl::update_metadata_on_delete(size_t index, size_t version) {
    if (auto* meta = catalogs_[index].find_metadata(version)) {
        meta->update_on_delete();
    }
}

void database_impl::save_metadata_to_disk(size_t index, size_t version) {
    if (index >= catalogs_.size()) return;
    auto const* meta_ptr = catalogs_[index].find_metadata(version);
    if ( ! meta_ptr) return;

    auto const& meta = *meta_ptr;
    auto metadata_file = fmt::format("{}/meta_{}_v{:05}.dat", db_path_.string(), index, version);

    std::ofstream ofs(metadata_file, std::ios::binary);
    if ( ! ofs) {
        log::warn("Failed to save metadata: {}", metadata_file);
        return;
    }

    uint64_t entry_count = meta.entry_count;
    ofs.write(reinterpret_cast<char const*>(&meta.min_block_height), sizeof(meta.min_block_height));
    ofs.write(reinterpret_cast<char const*>(&meta.max_block_height), sizeof(meta.max_block_height));
    ofs.write(reinterpret_cast<char const*>(meta.min_key.data()), meta.min_key.size());
    ofs.write(reinterpret_cast<char const*>(meta.max_key.data()), meta.max_key.size());
    ofs.write(reinterpret_cast<char const*>(&entry_count), sizeof(entry_count));
}

void database_impl::load_metadata_from_disk(size_t index, size_t version) {
    auto metadata_file = fmt::format("{}/meta_{}_v{:05}.dat", db_path_.string(), index, version);

    std::ifstream ifs(metadata_file, std::ios::binary);
    if ( ! ifs) return;

    auto& meta = catalogs_[index].metadata(version);
    meta.container_index = index;
    meta.version = version;

    uint64_t entry_count = 0;
    ifs.read(reinterpret_cast<char*>(&meta.min_block_height), sizeof(meta.min_block_height));
    ifs.read(reinterpret_cast<char*>(&meta.max_block_height), sizeof(meta.max_block_height));
    ifs.read(reinterpret_cast<char*>(meta.min_key.data()), meta.min_key.size());
    ifs.read(reinterpret_cast<char*>(meta.max_key.data()), meta.max_key.size());
    ifs.read(reinterpret_cast<char*>(&entry_count), sizeof(entry_count));
    meta.entry_count = static_cast<size_t>(entry_count);
}

// =============================================================================
// database_impl - Public interface: configure, close, size
// =============================================================================

result<> database_impl::configure(std::string_view path, bool remove_existing, storage_mode mode) {
    active_file_sizes_ = file_sizes;
    return configure_internal(path, remove_existing, mode);
}

result<> database_impl::configure_for_testing(std::string_view path, bool remove_existing, storage_mode mode) {
    active_file_sizes_ = test_file_sizes;
    return configure_internal(path, remove_existing, mode);
}

result<> database_impl::configure_internal(std::string_view path, bool remove_existing, storage_mode mode) {
    db_path_ = path;

    // Every filesystem question here is asked so that "I could not tell" comes
    // back as an error. Asked the throwing way, an unreadable directory raises
    // out of a result-typed open(); asked the swallowing way, it answers "no"
    // and the database gets recreated over data that is still there.
    auto const exists = path_exists(db_path_);
    if ( ! exists) return std::unexpected(exists.error());

    if (remove_existing && *exists) {
        std::error_code ec;
        fs::remove_all(db_path_, ec);
        if (ec) return std::unexpected(error_code::catalog_unreadable);
    }

    std::error_code ec;
    fs::create_directories(db_path_, ec);
    if (ec) return std::unexpected(error_code::catalog_unreadable);

    // Check config persistence (detect mode mismatch on reopen)
    auto config_path = db_path_ / "utxoz_config.dat";
    auto const config_exists = path_exists(config_path);
    if ( ! config_exists) return std::unexpected(config_exists.error());

    if (*config_exists && !remove_existing) {
        if (auto r = load_config_from_disk(); !r) {
            return std::unexpected(r.error());
        }
        if (mode_ != mode) {
            return std::unexpected(error_code::storage_mode_mismatch);
        }
    } else {
        // No config file — check for pre-existing data files from the other mode
        if (!remove_existing) {
            auto const other_mode_file = mode == storage_mode::compact
                ? fmt::format(data_file_format, db_path_.string(), 0, 0)
                : fmt::format(compact_data_file_format, db_path_.string(), 0);

            auto const other_exists = path_exists(other_mode_file);
            if ( ! other_exists) return std::unexpected(other_exists.error());
            if (*other_exists) {
                return std::unexpected(error_code::storage_mode_mismatch);
            }
        }
        mode_ = mode;
    }

    // Initialize file cache
    file_cache_ = std::make_unique<file_cache>(std::string(path));

    entries_count_ = 0;

    if (mode_ == storage_mode::compact) {
        // Compact mode: single container
        compact_active_file_size_ = (active_file_sizes_[0] == file_sizes[0])
            ? compact_file_size : compact_test_file_size;

        auto path_str = db_path_.string();
        compact_min_buckets_ok_ = find_optimal_buckets_compact(path_str, compact_active_file_size_, 7864304);

        // Build the catalogue before anything is opened. A directory we cannot
        // read is not an empty directory: opening on that assumption would
        // create v0 over a database that already has versions in it.
        auto listed = enumerate_versions(db_path_, "compact_v");
        if ( ! listed) return std::unexpected(listed.error());

        compact_catalog_.clear();
        for (auto const v : *listed) compact_catalog_.add(v);

        size_t const latest_version = compact_catalog_.active();
        compact_open_or_create(latest_version);
        compact_catalog_.add(latest_version);   // a fresh database has just created it
        entries_count_ += compact_map().size();

        // Count entries in previous versions (still searchable/deletable)
        for (auto const v : compact_catalog_.below(latest_version)) {
            auto file_name = fmt::format(compact_data_file_format, db_path_.string(), v);
            try {
                auto segment = std::make_unique<bip::managed_mapped_file>(bip::open_only, file_name.c_str());
                auto* map_ptr = segment->template find<compact_map_t>("db_map").first;
                if (map_ptr) {
                    entries_count_ += map_ptr->size();
                }
            } catch (std::exception const& e) {
                log::error("configure: error counting compact entries v{}: {}", v, e.what());
            }
        }

        for (auto const v : compact_catalog_.versions()) {
            compact_load_metadata(v);
        }
    } else {
        // Full mode: 5 containers
        static_assert(container_count == 5);
        auto path_str = db_path_.string();
        min_buckets_ok_[0] = find_optimal_buckets<0>(path_str, active_file_sizes_[0], 7864304);
        min_buckets_ok_[1] = find_optimal_buckets<1>(path_str, active_file_sizes_[1], 7864304);
        min_buckets_ok_[2] = find_optimal_buckets<2>(path_str, active_file_sizes_[2], 7864304);
        min_buckets_ok_[3] = find_optimal_buckets<3>(path_str, active_file_sizes_[3], 7864304);
        min_buckets_ok_[4] = find_optimal_buckets<4>(path_str, active_file_sizes_[4], 7864304);

        // As above: every container's catalogue is read before any of them is
        // opened, and a failure to read one aborts the open rather than being
        // taken for an empty container.
        result<> catalog_error;
        for_each_index<container_count>([&](auto I) {
            if ( ! catalog_error.has_value()) return;
            auto listed = enumerate_versions(db_path_, fmt::format("cont_{}_v", I.value));
            if ( ! listed) {
                catalog_error = std::unexpected(listed.error());
                return;
            }
            catalogs_[I].clear();
            for (auto const v : *listed) catalogs_[I].add(v);
        });
        if ( ! catalog_error.has_value()) return catalog_error;

        for_each_index<container_count>([&](auto I) {
            size_t const latest_version = catalogs_[I].active();
            open_or_create_container<I>(latest_version);
            catalogs_[I].add(latest_version);   // a fresh database has just created it

            // Count existing entries in active container
            entries_count_ += container<I>().size();

            // Count entries in previous versions (still searchable/deletable)
            for (auto const v : catalogs_[I].below(latest_version)) {
                auto file_name = fmt::format(data_file_format, db_path_.string(), I.value, v);
                try {
                    auto segment = std::make_unique<bip::managed_mapped_file>(bip::open_only, file_name.c_str());
                    auto* map_ptr = segment->template find<utxo_map<container_sizes[I]>>("db_map").first;
                    if (map_ptr) {
                        entries_count_ += map_ptr->size();
                    }
                } catch (std::exception const& e) {
                    log::error("configure: error counting entries in container {} v{}: {}", I.value, v, e.what());
                }
            }

            for (auto const v : catalogs_[I].versions()) {
                load_metadata_from_disk(I, v);
            }
        });
    }

    save_config_to_disk();
    return {};
}

void database_impl::close() {
    if (mode_ == storage_mode::compact) {
        compact_close_container();
    } else {
        for_each_index<container_count>([&](auto I) {
            close_container<I>();
        });
    }
}

size_t database_impl::size() const {
    return entries_count_;
}

// =============================================================================
// database_impl - Insert
// =============================================================================

result<bool> database_impl::insert(raw_outpoint const& key, output_data_span value, uint32_t height) {
    if (mode_ == storage_mode::compact) {
        return compact_insert(key, value, height);
    }

    size_t const index = get_index_from_size(value.size());
    if (index >= container_count) {
        log::error("insert: value too large ({} bytes) for any container (max capacity {}). height={}, outpoint={}",
            value.size(), container_capacities[container_count - 1], height, outpoint_to_string(key));
        return std::unexpected(error_code::value_too_large);
    }

    return std::visit([&](auto ic) -> result<bool> {
        return insert_in_index<ic>(key, value, height);
    }, make_index_variant(index));
}

template<size_t Index>
bool database_impl::insert_in_index(raw_outpoint const& key, output_data_span value, uint32_t height) {
    // Check if rotation needed
    if (!can_insert_safely<Index>()) {
        log::debug("Rotating container {} due to safety constraints", Index);
        new_version<Index>();
    }

    // Prepare value
    utxo_value<container_sizes[Index]> val;
    val.block_height = height;
    val.set_data(value);

    size_t max_retries = 3;
    while (max_retries > 0) {
        try {
            auto& map = container<Index>();
            [[maybe_unused]] size_t bucket_count_before = map.bucket_count();

            auto [it, inserted] = map.emplace(key, val);
            if ( ! inserted) {
                log::warn("insert: duplicate key at height {}, outpoint={}, container={}",
                    height, outpoint_to_string(key), Index);
            }
            if (inserted) {
                ++entries_count_;

#ifdef UTXOZ_STATISTICS_ENABLED
                // Update statistics
                ++container_stats_[Index].total_inserts;
                ++container_stats_[Index].current_size;
                ++container_stats_[Index].value_size_distribution[value.size()];
                ++height_range_stats_.ranges[height / height_range_stats::range_size].inserts[Index];

                if (map.bucket_count() != bucket_count_before) {
                    ++container_stats_[Index].rehash_count;
                }
#endif

                update_metadata_on_insert(Index, current_versions_[Index], key, height);
            }
            return inserted;

        } catch (boost::interprocess::bad_alloc const& e) {
            log::error("Error inserting into container {}: {}", Index, e.what());
            new_version<Index>();
        }
        --max_retries;
    }

    log::error("Failed to insert after 3 retries");
    throw boost::interprocess::bad_alloc();
}

// =============================================================================
// database_impl - Find
// =============================================================================

std::optional<find_result> database_impl::find(raw_outpoint const& key, uint32_t height) const {
    if (mode_ == storage_mode::compact) {
        return compact_find(key, height);
    }

    // Try current version first
    if (auto res = find_in_latest_version(key, height); res) {
        return res;
    }

    // Defer lookup to batch processing for efficiency
    // A probe the active map could not answer. Recording it is what makes the
    // hit rate mean something: without it every recorded probe was a hit.
    probe_stats_.record_deferred();
    add_to_deferred_lookups(key, height);
    return std::nullopt;
}

std::optional<find_result> database_impl::find_in_latest_version(raw_outpoint const& key,
                                                                  uint32_t height) const {
    std::optional<find_result> result;

    for_each_index<container_count>([&](auto I) {
        if (!result) {
            auto& map = container<I>();
            if (auto it = map.find(key); it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
                probe_stats_.record_answered(height, it->second.block_height);
#endif
                auto data = it->second.get_data();
                result = find_result{bytes(data.begin(), data.end()), it->second.block_height};
            }
        }
    });

    return result;
}

// =============================================================================
// database_impl - Erase
// =============================================================================

size_t database_impl::erase(raw_outpoint const& key, uint32_t height) {
    if (mode_ == storage_mode::compact) {
        return compact_erase(key, height);
    }

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

#endif
                map.erase(it);

#ifdef UTXOZ_STATISTICS_ENABLED
                --container_stats_[I].current_size;
                ++container_stats_[I].total_deletes;
                ++height_range_stats_.ranges[height / height_range_stats::range_size].deletes[I];
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

#endif
                        map.erase(it);

#ifdef UTXOZ_STATISTICS_ENABLED
                        --container_stats_[Index].current_size;
                        ++container_stats_[Index].total_deletes;
                        ++height_range_stats_.ranges[height / height_range_stats::range_size].deletes[Index];
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
            case 4: process_file(std::integral_constant<size_t, 4>{}); break;
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
        if (mode_ == storage_mode::compact) {
            std::set<size_t> processed_versions_compact;
            for (auto const& [ci, version] : cached_files) {
                if (ci == compact_sentinel_index) {
                    processed_versions_compact.insert(version);
                }
            }

            // Nearest generation first, over the versions that exist. Walking
            // a range down from the active one would visit every number ever
            // rotated through, and those never come back.
            for (auto const v : compact_catalog_.below(compact_current_version_)) {
                if (deferred_deletions_.empty()) break;
                if (processed_versions_compact.contains(v)) continue;

                successful_deletions += process_deferred_deletions_in_file(compact_sentinel_index, v, false);
            }
        } else {
            std::array<std::set<size_t>, container_count> processed_versions;
            for (auto const& [container_index, version] : cached_files) {
                processed_versions[container_index].insert(version);
            }

            for_each_index<container_count>([&](auto I) {
                if (deferred_deletions_.empty()) return;

                for (auto const v : catalogs_[I.value].below(current_versions_[I.value])) {
                    if (deferred_deletions_.empty()) break;
                    if (processed_versions[I.value].contains(v)) continue;

                    successful_deletions += process_deferred_deletions_in_file(I.value, v, false);
                }
            });
        }
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

    entries_count_ -= successful_deletions;

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
                    --container_stats_[Index].deferred_deletes;
                    --container_stats_[Index].current_size;
                    ++container_stats_[Index].total_deletes;
                    ++height_range_stats_.ranges[it->height / height_range_stats::range_size].deletes[Index];
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

    if (container_index == compact_sentinel_index) {
        // Compact mode deferred deletions
        try {
            auto [map, cache_hit] = file_cache_->get_or_open_compact_file(version);

            auto it = deferred_deletions_.begin();
            while (it != deferred_deletions_.end()) {
                auto erased_count = map.erase(it->key);
                if (erased_count > 0) {
                    if (auto* meta = compact_catalog_.find_metadata(version)) {
                        meta->update_on_delete();
                    }
#ifdef UTXOZ_STATISTICS_ENABLED
                    auto depth = static_cast<uint32_t>(compact_current_version_ - version);
                    ++deferred_stats_.deletions_by_depth[depth];
                    --container_stats_[0].deferred_deletes;
                    --container_stats_[0].current_size;
                    ++container_stats_[0].total_deletes;
                    ++height_range_stats_.ranges[it->height / height_range_stats::range_size].deletes[0];
#endif
                    it = deferred_deletions_.erase(it);
                    ++successful_deletions;
                } else {
                    ++it;
                }
            }
            return successful_deletions;
        } catch (std::exception const& e) {
            log::error("Error processing compact file v{}: {}", version, e.what());
            return 0;
        }
    }

    switch (container_index) {
        case 0: return process_with_container(std::integral_constant<size_t, 0>{});
        case 1: return process_with_container(std::integral_constant<size_t, 1>{});
        case 2: return process_with_container(std::integral_constant<size_t, 2>{});
        case 3: return process_with_container(std::integral_constant<size_t, 3>{});
        case 4: return process_with_container(std::integral_constant<size_t, 4>{});
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
        if (mode_ == storage_mode::compact) {
            std::set<size_t> processed_versions_compact;
            for (auto const& [ci, version] : cached_files) {
                if (ci == compact_sentinel_index) {
                    processed_versions_compact.insert(version);
                }
            }

            for (auto const v : compact_catalog_.below(compact_current_version_)) {
                if (deferred_lookups_.empty()) break;
                if (processed_versions_compact.contains(v)) continue;

                process_deferred_lookups_in_file(compact_sentinel_index, v, false, successful_lookups);
            }
        } else {
            std::array<std::set<size_t>, container_count> processed_versions;
            for (auto const& [container_index, version] : cached_files) {
                processed_versions[container_index].insert(version);
            }

            for_each_index<container_count>([&](auto I) {
                if (deferred_lookups_.empty()) return;

                for (auto const v : catalogs_[I.value].below(current_versions_[I.value])) {
                    if (deferred_lookups_.empty()) break;
                    if (processed_versions[I.value].contains(v)) continue;

                    process_deferred_lookups_in_file(I.value, v, false, successful_lookups);
                }
            });
        }
    }

    // Collect failed lookups (includes key and block height)
    std::vector<deferred_lookup_entry> failed_lookups;
    failed_lookups.reserve(deferred_lookups_.size());
    resolution_stats_.record_unresolved(deferred_lookups_.size());
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

            resolution_stats_.record_file_visited(cache_hit);


            deferred_lookups_.erase_if([&](auto const& entry) {
                auto map_it = map.find(entry.key);
                if (map_it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
                    auto depth = static_cast<uint32_t>(current_versions_[Index] - version);

                    ++deferred_stats_.lookups_by_depth[depth];
                    resolution_stats_.record_resolved(depth);
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

    if (container_index == compact_sentinel_index) {
        try {
            auto [map, cache_hit] = file_cache_->get_or_open_compact_file(version);

            resolution_stats_.record_file_visited(cache_hit);


            deferred_lookups_.erase_if([&](auto const& entry) {
                auto map_it = map.find(entry.key);
                if (map_it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
                    auto depth = static_cast<uint32_t>(compact_current_version_ - version);
                    ++deferred_stats_.lookups_by_depth[depth];
                    resolution_stats_.record_resolved(depth);
#endif
                    bytes data(sizeof(uint32_t) * 2);
                    std::memcpy(data.data(), &map_it->second.file_number, sizeof(uint32_t));
                    std::memcpy(data.data() + sizeof(uint32_t), &map_it->second.offset, sizeof(uint32_t));
                    successful_lookups.emplace(entry.key, std::move(data));
                    return true;
                }
                return false;
            });
        } catch (std::exception const& e) {
            log::error("Error processing compact lookups v{}: {}", version, e.what());
        }
        return;
    }

    switch (container_index) {
        case 0: process_with_container(std::integral_constant<size_t, 0>{}); break;
        case 1: process_with_container(std::integral_constant<size_t, 1>{}); break;
        case 2: process_with_container(std::integral_constant<size_t, 2>{}); break;
        case 3: process_with_container(std::integral_constant<size_t, 3>{}); break;
        case 4: process_with_container(std::integral_constant<size_t, 4>{}); break;
    }
}

// =============================================================================
// database_impl - Compaction
// =============================================================================

template<size_t Index>
result<> database_impl::compact_container() {
    log::debug("Starting compaction for container {}...", Index);

    size_t files_deleted = 0;
    size_t entries_moved = 0;

    close_container<Index>();

    // compact_container() begins by closing the active container, so every exit
    // has to put one back. Leaving containers_[Index] null would turn the next
    // operation into a null dereference.
    auto reopen_active = [&] {
        auto const active = catalogs_[Index].active();
        open_or_create_container<Index>(active);
        catalogs_[Index].add(active);   // only once the file is there: see new_version()
    };

    // Metadata describes a file layout, and compaction changes that layout as it
    // goes: it empties files, removes them and renumbers what is left. Every
    // exit past that point has to rebuild it, the failing one included, or the
    // persisted metadata goes on describing versions that no longer exist.
    auto rebuild_metadata = [&] {
        catalogs_[Index].clear_metadata();

        for (auto const v : catalogs_[Index].versions()) {
            auto& meta = catalogs_[Index].metadata(v);
            meta.container_index = Index;
            meta.version = v;

            auto file_name = fmt::format(data_file_format, db_path_.string(), Index, v);
            if (!fs::exists(file_name)) continue;

            try {
                auto segment = std::make_unique<bip::managed_mapped_file>(bip::open_only, file_name.c_str());
                auto* map_ptr = segment->template find<utxo_map<container_sizes[Index]>>("db_map").first;
                if (!map_ptr) continue;

                for (auto const& [key, val] : *map_ptr) {
                    meta.update_on_insert(key, val.block_height);
                }
            } catch (std::exception const& e) {
                log::error("compact_container: error scanning container {} v{} for metadata: {}", Index, v, e.what());
            }

            save_metadata_to_disk(Index, v);
        }
    };

    // Work over the versions the catalogue holds, by position, rather than over
    // a dense range. Files keep the numbers they have.
    auto versions = catalogs_[Index].versions();
    if (versions.size() <= 1) {
        log::trace("Container {} has {} files, no compaction needed", Index, versions.size());
        reopen_active();
        return {};
    }

    std::unique_ptr<bip::managed_mapped_file> target_segment;
    std::unique_ptr<bip::managed_mapped_file> source_segment;

    // From here the layout is being changed, so every way out — the early
    // returns below, and a throw from any of the file operations — has to
    // release the mappings, put the metadata back in step with what is on disk,
    // and leave an active container mapped.
    scope_exit const restore([&] {
        target_segment.reset();
        source_segment.reset();
        rebuild_metadata();
        reopen_active();
    });

    size_t source_pos = 1;
    size_t target_idx = versions[0];
    size_t source_idx = versions[source_pos];

    target_segment = open_container_file<Index>(target_idx);
    source_segment = open_container_file<Index>(source_idx);

    auto* target_map = target_segment->template find<utxo_map<container_sizes[Index]>>("db_map").first;
    auto* source_map = source_segment->template find<utxo_map<container_sizes[Index]>>("db_map").first;

    while (source_pos < versions.size()) {
        auto it = source_map->begin();

        while (it != source_map->end()) {
            if (!can_insert_safely_in_map<Index>(*target_map, *target_segment)) {
                log::trace("Target file {} is full, rotating...", target_idx);

                target_segment.reset();
                target_idx = source_idx;
                target_segment = std::move(source_segment);
                target_map = source_map;

                ++source_pos;
                if (source_pos >= versions.size()) break;
                source_idx = versions[source_pos];

                source_segment = open_container_file<Index>(source_idx);
                source_map = source_segment->template find<utxo_map<container_sizes[Index]>>("db_map").first;
                break;
            }

            auto key = it->first;
            auto value = it->second;

            try {
                auto const [pos, inserted] = target_map->emplace(key, value);
                if ( ! inserted) {
                    // The target already held this key before we touched it —
                    // either from the start, or moved there from a third file
                    // earlier in this same run. Both mean two version files held
                    // the key at once, which a published state must never do.
                    //
                    // Report, do not repair: the source copy stays where it is,
                    // the target's is untouched, and the entry counter is left
                    // alone so it still describes what is on disk. Choosing one
                    // would erase the evidence of a corrupt database.
                    log::error("compaction: duplicate key in container {} — present in both v{} and v{}: {}",
                               Index, source_idx, target_idx, outpoint_to_string(key));
                    return std::unexpected(error_code::duplicate_key);
                }
                it = source_map->erase(it);
                ++entries_moved;
            } catch (boost::interprocess::bad_alloc const&) {
                target_segment.reset();
                target_idx = source_idx;
                target_segment = std::move(source_segment);
                target_map = source_map;

                ++source_pos;
                if (source_pos >= versions.size()) break;
                source_idx = versions[source_pos];

                source_segment = open_container_file<Index>(source_idx);
                source_map = source_segment->template find<utxo_map<container_sizes[Index]>>("db_map").first;
                it = source_map->begin();
            }
        }

        if (source_map && source_map->empty()) {
            source_segment.reset();

            // The catalogue follows what actually happened on disk, not what we
            // meant to happen. Ordering the removals data-then-metadata is what
            // makes each outcome describable: the data file is what the
            // catalogue is a catalogue of.
            //
            // TODO(#59): the removals are still merely attempted, not ordered
            // against a durability barrier. Making them survive a crash belongs
            // with that work; reporting when they fail belongs here.
            std::error_code ec;
            fs::remove(fmt::format(data_file_format, db_path_.string(), Index, source_idx), ec);
            if (ec) {
                // The file is still there, and it is empty. Leaving it in the
                // catalogue is the truthful state: a later traversal will visit
                // an empty file, which costs a little and hides nothing.
                log::error("compaction: could not remove container {} v{}: {}",
                           Index, source_idx, ec.message());
                return std::unexpected(error_code::removal_failed);
            }

            // The data is gone, so the version is gone whatever happens next.
            // Nothing is renumbered to close the hole: renaming every version
            // above it was a multi-step rewrite of the catalogue with no
            // atomicity, and an interrupted one used to hide every version past
            // the gap. Version numbers are identities, so a hole costs nothing.
            catalogs_[Index].remove(source_idx);
            versions.erase(versions.begin() + std::ptrdiff_t(source_pos));
            ++files_deleted;

            fs::remove(fmt::format("{}/meta_{}_v{:05}.dat", db_path_.string(), Index, source_idx), ec);
            if (ec) {
                // The data file went and its metadata did not. That record now
                // describes nothing, and identities are only unique for the life
                // of the process — after a restart this number can be issued
                // again, and a stale record would then be read as describing the
                // new file. Metadata is used to skip files during a search, so
                // believing it would turn a present key into a miss.
                log::error("compaction: removed container {} v{} but not its metadata: {}",
                           Index, source_idx, ec.message());
                return std::unexpected(error_code::removal_failed);
            }

            if (source_pos < versions.size()) {
                source_idx = versions[source_pos];
                source_segment = open_container_file<Index>(source_idx);
                source_map = source_segment->template find<utxo_map<container_sizes[Index]>>("db_map").first;
            }
        }
    }

    log::debug("Compaction complete for container {}: {} files deleted, {} entries moved",
              Index, files_deleted, entries_moved);
    return {};
}

void database_impl::for_each_key_impl(void(*cb)(void*, raw_outpoint const&), void* ctx) const {
    if (mode_ == storage_mode::compact) {
        compact_for_each_key(cb, ctx);
        return;
    }

    for_each_index<container_count>([&](auto I) {
        // Current version (active container)
        auto const& map = container<I>();
        for (auto const& [key, _] : map) {
            cb(ctx, key);
        }

        // Previous versions
        for (auto const v : catalogs_[I].below(current_versions_[I])) {
            auto file_name = fmt::format(data_file_format, db_path_.string(), I.value, v);

            try {
                auto segment = std::make_unique<bip::managed_mapped_file>(bip::open_only, file_name.c_str());
                auto* map_ptr = segment->template find<utxo_map<container_sizes[I]>>("db_map").first;
                if (!map_ptr) continue;

                for (auto const& [key, _] : *map_ptr) {
                    cb(ctx, key);
                }
            } catch (std::exception const& e) {
                log::error("for_each_key: error reading container {} v{}: {}", I.value, v, e.what());
            }
        }
    });
}

void database_impl::for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, std::span<uint8_t const>), void* ctx) const {
    if (mode_ == storage_mode::compact) {
        compact_for_each_entry(cb, ctx);
        return;
    }

    for_each_index<container_count>([&](auto I) {
        // Current version (active container)
        auto const& map = container<I>();
        for (auto const& [key, val] : map) {
            cb(ctx, key, val.block_height, val.get_data());
        }

        // Previous versions
        for (auto const v : catalogs_[I].below(current_versions_[I])) {
            auto file_name = fmt::format(data_file_format, db_path_.string(), I.value, v);

            try {
                auto segment = std::make_unique<bip::managed_mapped_file>(bip::open_only, file_name.c_str());
                auto* map_ptr = segment->template find<utxo_map<container_sizes[I]>>("db_map").first;
                if (!map_ptr) continue;

                for (auto const& [key, val] : *map_ptr) {
                    cb(ctx, key, val.block_height, val.get_data());
                }
            } catch (std::exception const& e) {
                log::error("for_each_entry: error reading container {} v{}: {}", I.value, v, e.what());
            }
        }
    });
}

result<> database_impl::compact_all() {
    log::info("Starting full database compaction...");

    // Compaction moves entries between files and renames/removes versions, so
    // every cached (container_index, version) mapping becomes stale.
    if (file_cache_) file_cache_->clear();

    result<> outcome;

    if (mode_ == storage_mode::compact) {
        outcome = compact_compact_container();
    } else {
        for_each_index<container_count>([&](auto I) {
            // Stop at the first failure. Carrying on would mutate more of the
            // database after a condition the owner is going to treat as fatal,
            // and destroy more of the evidence of how it got that way.
            if ( ! outcome) return;
            outcome = compact_container<I>();
        });
    }

    if (file_cache_) file_cache_->clear();

    if ( ! outcome) {
        log::error("Full database compaction aborted: the database is locally inconsistent");
        return outcome;
    }

    log::info("Full database compaction complete");
    return {};
}

// =============================================================================
// database_impl - Statistics
// =============================================================================

void database_impl::update_fragmentation_stats() {
#ifdef UTXOZ_STATISTICS_ENABLED
    for_each_index<container_count>([&](auto I) {
        if (segments_[I]) {
            try {
                size_t total_size = active_file_sizes_[I];
                size_t free_memory = segments_[I]->get_free_memory();
                size_t used_memory = total_size - free_memory;

                fragmentation_stats_.fill_ratios[I] = double(used_memory) / double(total_size);

                auto& map = container<I>();
                size_t ideal_size = map.size() * sizeof(typename utxo_map<container_sizes[I]>::value_type);
                fragmentation_stats_.wasted_space[I] =
                    used_memory > ideal_size ? used_memory - ideal_size : 0;
            } catch (...) {
                fragmentation_stats_.fill_ratios[I] = 0.0;
                fragmentation_stats_.wasted_space[I] = 0;
            }
        }
    });
#endif
}

size_t database_impl::estimate_memory_usage(size_t index) const {
    size_t total = 0;

    if (segments_[index]) {
        total += active_file_sizes_[index];
    }

    for (auto const v : catalogs_[index].below(current_versions_[index])) {
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
    stats.mode = mode_;
    stats.total_entries = entries_count_;
    stats.cache_hit_rate = get_cache_hit_rate();
    stats.cached_files_count = file_cache_ ? file_cache_->get_cached_files().size() : 0;
    stats.cached_files_info = get_cached_file_info();
    stats.probes = probe_stats_.get_summary();
    stats.resolution = resolution_stats_.get_summary();

    stats.total_inserts = 0;
    stats.total_deletes = 0;

    if (mode_ == storage_mode::compact) {
        stats.containers[0] = container_stats_[0];
        stats.total_inserts = container_stats_[0].total_inserts;
        stats.total_deletes = container_stats_[0].total_deletes;
        stats.rotations_per_container[0] = compact_current_version_;
        stats.memory_usage_per_container[0] = compact_active_file_size_;
    } else {
        for (size_t i = 0; i < container_count; ++i) {
            stats.containers[i] = container_stats_[i];
            stats.total_inserts += container_stats_[i].total_inserts;
            stats.total_deletes += container_stats_[i].total_deletes;
            stats.rotations_per_container[i] = current_versions_[i];
            stats.memory_usage_per_container[i] = estimate_memory_usage(i);
        }
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
    log::info("Storage mode: {}", stats.mode == storage_mode::compact ? "compact" : "full");
    log::info("Total entries: {}", stats.total_entries);
    log::info("Total inserts: {}", stats.total_inserts);
    log::info("Total deletes: {}", stats.total_deletes);

    log::info("--- Container Statistics ---");
    if (stats.mode == storage_mode::compact) {
        log::info("Compact container ({} bytes per entry):", sizeof(compact_value));
        log::info("  Current entries: {}", stats.containers[0].current_size);
        log::info("  Total inserts: {}", stats.containers[0].total_inserts);
        log::info("  Total deletes: {}", stats.containers[0].total_deletes);
        log::info("  File rotations: {}", stats.rotations_per_container[0]);
        log::info("  Est. memory: {:.2f} MB", stats.memory_usage_per_container[0] / (1024.0*1024.0));
    } else {
        for (size_t i = 0; i < container_count; ++i) {
            log::info("Container {} (size <= {} bytes):", i, container_sizes[i]);
            log::info("  Current entries: {}", stats.containers[i].current_size);
            log::info("  Total inserts: {}", stats.containers[i].total_inserts);
            log::info("  Total deletes: {}", stats.containers[i].total_deletes);
            log::info("  File rotations: {}", stats.rotations_per_container[i]);
            log::info("  Est. memory: {:.2f} MB", stats.memory_usage_per_container[i] / (1024.0*1024.0));
        }
    }

    log::info("--- Cache Statistics ---");
    log::info("Cache hit rate: {:.2f}%", stats.cache_hit_rate * 100);
    log::info("Cached files: {}", stats.cached_files_count);

    log::info("--- Probes ---");
    log::info("Probes: {}", stats.probes.probes);
    log::info("Answered from the active map: {} ({:.2f}%)",
        stats.probes.answered_from_active, stats.probes.active_map_hit_rate * 100);
    log::info("Deferred to historical resolution: {}", stats.probes.deferred);
    log::info("Avg age of answered probes: {:.1f} blocks", stats.probes.avg_age_answered);

    log::info("--- Historical resolution ---");
    log::info("Resolved: {}   unresolved: {}", stats.resolution.resolved, stats.resolution.unresolved);
    log::info("Avg depth: {:.2f} versions", stats.resolution.avg_depth);
    log::info("Files visited: {}  cache hit rate: {:.2f}%",
        stats.resolution.files_visited, stats.resolution.cache_hit_rate * 100);

    log::info("================================");
}

sizing_report database_impl::get_sizing_report() const {
    sizing_report report{};

    if (mode_ == storage_mode::compact) {
        auto& info = report.containers[0];
        info.container_size = sizeof(compact_value);
        info.file_size_setting = compact_active_file_size_;
        info.file_count = compact_catalog_.size();
        info.current_entries = container_stats_[0].current_size;
        info.historical_inserts = container_stats_[0].total_inserts;
        info.historical_deletes = container_stats_[0].total_deletes;
        info.total_wasted_bytes = 0;

        for (auto const& [value_size, count] : container_stats_[0].value_size_distribution) {
            if (sizeof(compact_value) > value_size) {
                info.total_wasted_bytes += (sizeof(compact_value) - value_size) * count;
            }
            report.global_value_size_histogram[value_size] += count;
        }

        info.avg_waste_per_entry = info.historical_inserts > 0
            ? double(info.total_wasted_bytes) / double(info.historical_inserts)
            : 0.0;
    } else {
        for (size_t i = 0; i < container_count; ++i) {
            auto& info = report.containers[i];
            info.container_size = container_sizes[i];
            info.file_size_setting = active_file_sizes_[i];
            info.file_count = catalogs_[i].size();
            info.current_entries = container_stats_[i].current_size;
            info.historical_inserts = container_stats_[i].total_inserts;
            info.historical_deletes = container_stats_[i].total_deletes;
            info.total_wasted_bytes = 0;

            for (auto const& [value_size, count] : container_stats_[i].value_size_distribution) {
                if (container_sizes[i] > value_size) {
                    info.total_wasted_bytes += (container_sizes[i] - value_size) * count;
                }
                report.global_value_size_histogram[value_size] += count;
            }

            info.avg_waste_per_entry = info.historical_inserts > 0
                ? double(info.total_wasted_bytes) / double(info.historical_inserts)
                : 0.0;
        }
    }

    return report;
}

void database_impl::print_sizing_report() const {
    auto report = get_sizing_report();

    log::info("=== UTXO-Z Sizing Report ===");
    log::info("");

    for (size_t i = 0; i < container_count; ++i) {
        auto const& c = report.containers[i];
        double file_size_gib = double(c.file_size_setting) / (1024.0 * 1024.0 * 1024.0);
        double file_size_mib = double(c.file_size_setting) / (1024.0 * 1024.0);

        if (file_size_gib >= 1.0) {
            log::info("--- Container {} (max {} bytes, file size: {:.2f} GiB) ---",
                      i, c.container_size, file_size_gib);
        } else {
            log::info("--- Container {} (max {} bytes, file size: {:.2f} MiB) ---",
                      i, c.container_size, file_size_mib);
        }

        log::info("  Files: {}", c.file_count);
        log::info("  Current entries: {:L}", c.current_entries);
        log::info("  Historical inserts: {:L}", c.historical_inserts);
        log::info("  Historical deletes: {:L}", c.historical_deletes);
        log::info("  Wasted bytes: {:L} ({:.2f} bytes/entry avg)",
                  c.total_wasted_bytes, c.avg_waste_per_entry);
        log::info("");
    }

    // Build sorted histogram (by count descending)
    std::vector<std::pair<size_t, size_t>> sorted_histogram(
        report.global_value_size_histogram.begin(),
        report.global_value_size_histogram.end());

    std::ranges::sort(sorted_histogram, [](auto const& a, auto const& b) {
        return a.second > b.second;
    });

    // Compute total for percentage
    size_t total_count = 0;
    for (auto const& [sz, cnt] : sorted_histogram) {
        total_count += cnt;
    }

    log::info("--- Global Value Size Histogram ({} distinct sizes) ---", sorted_histogram.size());
    for (auto const& [value_size, count] : sorted_histogram) {
        double pct = total_count > 0 ? double(count) / double(total_count) * 100.0 : 0.0;
        log::info("  {} bytes: {:L} ({:.1f}%)", value_size, count, pct);
    }

    log::info("");
    log::info("=== End Sizing Report ===");
}

void database_impl::print_height_range_stats() const {
    auto const& stats = height_range_stats_;
    if (stats.ranges.empty()) {
        log::info("No height range statistics collected.");
        return;
    }

    std::vector<uint32_t> sorted_keys;
    sorted_keys.reserve(stats.ranges.size());
    for (auto const& [key, _] : stats.ranges) {
        sorted_keys.push_back(key);
    }
    std::ranges::sort(sorted_keys);

    log::info("=== UTXO-Z Height Range Statistics (per {:L} blocks) ===", height_range_stats::range_size);
    log::info("");

    // Header
    std::string header = fmt::format("  {:>14}", "Range");
    for (size_t i = 0; i < container_count; ++i) {
        header += fmt::format(" | C{}({:>5})", i, container_sizes[i]);
    }
    header += fmt::format(" | {:>12}", "Total");
    log::info("{}", header);

    // Inserts
    log::info("--- Inserts ---");
    for (uint32_t key : sorted_keys) {
        auto const& data = stats.ranges.at(key);
        uint32_t start = key * height_range_stats::range_size;
        uint32_t end = start + height_range_stats::range_size - 1;

        std::string row = fmt::format("  {:>6}-{:<6}", start, end);
        size_t total = 0;
        for (size_t i = 0; i < container_count; ++i) {
            row += fmt::format(" | {:>9L}", data.inserts[i]);
            total += data.inserts[i];
        }
        row += fmt::format(" | {:>12L}", total);
        log::info("{}", row);
    }

    log::info("");

    // Deletes
    log::info("--- Deletes ---");
    for (uint32_t key : sorted_keys) {
        auto const& data = stats.ranges.at(key);
        uint32_t start = key * height_range_stats::range_size;
        uint32_t end = start + height_range_stats::range_size - 1;

        std::string row = fmt::format("  {:>6}-{:<6}", start, end);
        size_t total = 0;
        for (size_t i = 0; i < container_count; ++i) {
            row += fmt::format(" | {:>9L}", data.deletes[i]);
            total += data.deletes[i];
        }
        row += fmt::format(" | {:>12L}", total);
        log::info("{}", row);
    }

    log::info("");
    log::info("=== End Height Range Statistics ===");
}

void database_impl::reset_all_statistics() {
    for (auto& cs : container_stats_) {
        cs = container_stats{};
    }
    height_range_stats_ = height_range_stats{};
    deferred_stats_ = deferred_stats{};
    not_found_stats_ = not_found_stats{};
    lifetime_stats_ = utxo_lifetime_stats{};
    fragmentation_stats_ = fragmentation_stats{};
    reset_search_stats();
}

void database_impl::reset_search_stats() {
    probe_stats_.reset();
    resolution_stats_.reset();
}

float database_impl::get_cache_hit_rate() const {
    return file_cache_ ? file_cache_->get_hit_rate() : 0.0f;
}

std::vector<std::pair<size_t, size_t>> database_impl::get_cached_file_info() const {
    return file_cache_ ? file_cache_->get_cached_files() : std::vector<std::pair<size_t, size_t>>{};
}

// =============================================================================
// database_impl - Compact mode implementation
// =============================================================================

compact_map_t& database_impl::compact_map() {
    return *static_cast<compact_map_t*>(compact_container_);
}

compact_map_t const& database_impl::compact_map() const {
    return *static_cast<compact_map_t const*>(compact_container_);
}

size_t database_impl::find_optimal_buckets_compact(std::string const& file_path,
                                                    size_t file_size,
                                                    size_t initial_buckets) {
    log::debug("Finding optimal buckets for compact container (file size: {})...", file_size);

    size_t left = 1;
    size_t right = initial_buckets;
    size_t best_buckets = left;

    while (left <= right) {
        size_t mid = left + (right - left) / 2;

        std::string temp_file = fmt::format("{}/temp_compact_{}_{}.dat", file_path, file_size, mid);
        try {
            bip::managed_mapped_file segment(bip::open_or_create, temp_file.c_str(), file_size);

            (void)segment.find_or_construct<compact_map_t>("temp_map")(
                mid,
                outpoint_hash{},
                outpoint_equal{},
                segment.get_allocator<std::pair<raw_outpoint const, compact_value>>()
            );

            best_buckets = mid;
            left = mid + 1;
            log::trace("  {} buckets OK, trying more...", mid);
        } catch (boost::interprocess::bad_alloc const&) {
            right = mid - 1;
        }

        fs::remove(temp_file);
    }

    log::debug("Optimal buckets for compact container: {}", best_buckets);
    return best_buckets;
}

void database_impl::compact_open_or_create(size_t version) {
    auto file_name = fmt::format(compact_data_file_format, db_path_.string(), version);

    compact_segment_ = std::make_unique<bip::managed_mapped_file>(
        bip::open_or_create, file_name.c_str(), compact_active_file_size_);

    compact_container_ = compact_segment_->find_or_construct<compact_map_t>("db_map")(
        compact_min_buckets_ok_,
        outpoint_hash{},
        outpoint_equal{},
        compact_segment_->get_allocator<typename compact_map_t::value_type>()
    );

    compact_current_version_ = version;
}

void database_impl::compact_close_container() {
    if (compact_segment_) {
        compact_save_metadata(compact_current_version_);
        compact_segment_->flush();
        compact_segment_.reset();
        compact_container_ = nullptr;
    }
}

void database_impl::compact_new_version() {
    compact_close_container();

    // The file first, the catalogue after: see new_version().
    auto const next = compact_catalog_.next_version();
    compact_open_or_create(next);   // sets compact_current_version_ once it maps

    compact_catalog_.add(next);
    compact_catalog_.metadata(next) = file_metadata{};
    log::debug("Compact container rotated to version {}", compact_current_version_);
}

bool database_impl::compact_can_insert_safely() const {
    auto const& map = compact_map();

    if (map.bucket_count() > 0) {
        float next_load = float(map.size() + 1) / float(map.bucket_count());
        if (next_load >= map.max_load_factor() * 0.95f) {
            return false;
        }
    }

    if (compact_segment_) {
        try {
            size_t free_memory = compact_segment_->get_free_memory();
            size_t entry_size = sizeof(typename compact_map_t::value_type);
            size_t buffer_size = entry_size * 10;
            return free_memory > buffer_size;
        } catch (...) {
            return false;
        }
    }

    return true;
}

result<bool> database_impl::compact_insert(raw_outpoint const& key, output_data_span value, uint32_t height) {
    if (value.size() != sizeof(uint32_t) * 2) {
        return std::unexpected(error_code::value_too_large);
    }

    uint32_t file_number;
    uint32_t offset;
    std::memcpy(&file_number, value.data(), sizeof(uint32_t));
    std::memcpy(&offset, value.data() + sizeof(uint32_t), sizeof(uint32_t));

    return compact_insert_typed(key, height, file_number, offset);
}

std::optional<find_result> database_impl::compact_find(raw_outpoint const& key, uint32_t height) const {
    if (auto res = compact_find_in_latest(key, height); res) {
        return res;
    }

    // A probe the active map could not answer. Recording it is what makes the
    // hit rate mean something: without it every recorded probe was a hit.
    probe_stats_.record_deferred();
    add_to_deferred_lookups(key, height);
    return std::nullopt;
}

std::optional<find_result> database_impl::compact_find_in_latest(raw_outpoint const& key, uint32_t height) const {
    auto const& map = compact_map();
    if (auto it = map.find(key); it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
        probe_stats_.record_answered(height, it->second.height);
#endif
        bytes data(sizeof(uint32_t) * 2);
        std::memcpy(data.data(), &it->second.file_number, sizeof(uint32_t));
        std::memcpy(data.data() + sizeof(uint32_t), &it->second.offset, sizeof(uint32_t));
        return find_result{std::move(data), it->second.height};
    }
    return std::nullopt;
}

size_t database_impl::compact_erase(raw_outpoint const& key, uint32_t height) {
    // Try current version first
    if (auto res = compact_erase_in_latest(key, height); res > 0) {
        entries_count_ -= res;
        return res;
    }

    // Try cached files
    size_t search_depth = 1;
    auto cached_files = file_cache_->get_cached_files();
    for (auto const& [ci, version] : cached_files) {
        if (ci != compact_sentinel_index) continue;
        ++search_depth;

        if (file_cache_->is_cached(compact_sentinel_index, version)) {
            try {
                auto [map, cache_hit] = file_cache_->get_or_open_compact_file(version);

                if (auto it = map.find(key); it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
                    uint32_t age = height - it->second.height;
                    ++lifetime_stats_.age_distribution[age];
                    lifetime_stats_.max_age = std::max(lifetime_stats_.max_age, age);
                    ++lifetime_stats_.total_spent;
                    lifetime_stats_.average_age =
                        (lifetime_stats_.average_age * (lifetime_stats_.total_spent - 1) + age)
                        / lifetime_stats_.total_spent;

                    --container_stats_[0].current_size;
                    ++container_stats_[0].total_deletes;
                    ++height_range_stats_.ranges[height / height_range_stats::range_size].deletes[0];
#endif
                    map.erase(it);

                    if (auto* meta = compact_catalog_.find_metadata(version)) {
                        meta->update_on_delete();
                    }

                    --entries_count_;
                    return 1;
                }
            } catch (std::exception const& e) {
                log::error("Error accessing cached compact file v{}: {}", version, e.what());
            }
        }
    }

#ifdef UTXOZ_STATISTICS_ENABLED
    ++not_found_stats_.total_not_found;
    not_found_stats_.total_search_depth += search_depth;
    not_found_stats_.max_search_depth = std::max(not_found_stats_.max_search_depth, search_depth);
    ++not_found_stats_.depth_distribution[search_depth];
#endif

    add_to_deferred_deletions(key, height);
    return 0;
}

size_t database_impl::compact_erase_in_latest(raw_outpoint const& key, uint32_t height) {
    auto& map = compact_map();
    if (auto it = map.find(key); it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
        uint32_t age = height - it->second.height;
        ++lifetime_stats_.age_distribution[age];
        lifetime_stats_.max_age = std::max(lifetime_stats_.max_age, age);
        ++lifetime_stats_.total_spent;
        lifetime_stats_.average_age =
            (lifetime_stats_.average_age * (lifetime_stats_.total_spent - 1) + age)
            / lifetime_stats_.total_spent;

        --container_stats_[0].current_size;
        ++container_stats_[0].total_deletes;
        ++height_range_stats_.ranges[height / height_range_stats::range_size].deletes[0];
#endif
        map.erase(it);
        return 1;
    }
    return 0;
}

void database_impl::compact_for_each_key(void(*cb)(void*, raw_outpoint const&), void* ctx) const {
    // Current version
    auto const& map = compact_map();
    for (auto const& [key, _] : map) {
        cb(ctx, key);
    }

    // Previous versions
    for (auto const v : compact_catalog_.below(compact_current_version_)) {
        auto file_name = fmt::format(compact_data_file_format, db_path_.string(), v);

        try {
            auto segment = std::make_unique<bip::managed_mapped_file>(bip::open_only, file_name.c_str());
            auto* map_ptr = segment->find<compact_map_t>("db_map").first;
            if (!map_ptr) continue;

            for (auto const& [key, _] : *map_ptr) {
                cb(ctx, key);
            }
        } catch (std::exception const& e) {
            log::error("compact_for_each_key: error reading compact v{}: {}", v, e.what());
        }
    }
}

void database_impl::compact_for_each_entry(void(*cb)(void*, raw_outpoint const&, uint32_t, std::span<uint8_t const>), void* ctx) const {
    auto emit = [&](raw_outpoint const& key, compact_value const& val) {
        std::array<uint8_t, sizeof(uint32_t) * 2> buf;
        std::memcpy(buf.data(), &val.file_number, sizeof(uint32_t));
        std::memcpy(buf.data() + sizeof(uint32_t), &val.offset, sizeof(uint32_t));
        cb(ctx, key, val.height, {buf.data(), buf.size()});
    };

    // Current version
    auto const& map = compact_map();
    for (auto const& [key, val] : map) {
        emit(key, val);
    }

    // Previous versions
    for (auto const v : compact_catalog_.below(compact_current_version_)) {
        auto file_name = fmt::format(compact_data_file_format, db_path_.string(), v);

        try {
            auto segment = std::make_unique<bip::managed_mapped_file>(bip::open_only, file_name.c_str());
            auto* map_ptr = segment->find<compact_map_t>("db_map").first;
            if (!map_ptr) continue;

            for (auto const& [key, val] : *map_ptr) {
                emit(key, val);
            }
        } catch (std::exception const& e) {
            log::error("compact_for_each_entry: error reading compact v{}: {}", v, e.what());
        }
    }
}

result<> database_impl::compact_compact_container() {
    log::debug("Starting compaction for compact container...");

    size_t files_deleted = 0;
    size_t entries_moved = 0;

    compact_close_container();

    // Every exit has to leave an active container behind; see compact_container().
    auto reopen_active = [&] {
        auto const active = compact_catalog_.active();
        compact_open_or_create(active);
        compact_catalog_.add(active);   // only once the file is there
    };

    // And every exit past the first move has to rebuild the metadata, for the
    // same reason as in full mode: it describes a layout this function changes.
    auto rebuild_metadata = [&] {
        compact_catalog_.clear_metadata();

        for (auto const v : compact_catalog_.versions()) {
            auto& meta = compact_catalog_.metadata(v);
            meta.container_index = compact_sentinel_index;
            meta.version = v;

            auto file_name = fmt::format(compact_data_file_format, db_path_.string(), v);
            if (!fs::exists(file_name)) continue;

            try {
                auto segment = std::make_unique<bip::managed_mapped_file>(bip::open_only, file_name.c_str());
                auto* map_ptr = segment->find<compact_map_t>("db_map").first;
                if (!map_ptr) continue;

                for (auto const& [key, val] : *map_ptr) {
                    meta.update_on_insert(key, val.height);
                }
            } catch (std::exception const& e) {
                log::error("compact_compact_container: error scanning compact v{} for metadata: {}", v, e.what());
            }

            compact_save_metadata(v);
        }
    };

    auto versions = compact_catalog_.versions();
    if (versions.size() <= 1) {
        log::trace("Compact container has {} files, no compaction needed", versions.size());
        reopen_active();
        return {};
    }

    auto open_compact_file = [&](size_t version) {
        auto file_name = fmt::format(compact_data_file_format, db_path_.string(), version);
        return std::make_unique<bip::managed_mapped_file>(bip::open_only, file_name.c_str());
    };

    std::unique_ptr<bip::managed_mapped_file> target_segment;
    std::unique_ptr<bip::managed_mapped_file> source_segment;

    // Every exit past this point, throws included: see compact_container().
    scope_exit const restore([&] {
        target_segment.reset();
        source_segment.reset();
        rebuild_metadata();
        reopen_active();
    });

    size_t source_pos = 1;
    size_t target_idx = versions[0];
    size_t source_idx = versions[source_pos];

    target_segment = open_compact_file(target_idx);
    source_segment = open_compact_file(source_idx);

    auto* target_map = target_segment->find<compact_map_t>("db_map").first;
    auto* source_map = source_segment->find<compact_map_t>("db_map").first;

    auto can_insert_in_map = [](compact_map_t const& map,
                                bip::managed_mapped_file const& segment) -> bool {
        if (map.bucket_count() > 0) {
            float next_load = float(map.size() + 1) / float(map.bucket_count());
            if (next_load >= map.max_load_factor() * 0.95f) {
                return false;
            }
        }
        try {
            size_t free_memory = segment.get_free_memory();
            size_t entry_size = sizeof(typename compact_map_t::value_type);
            return free_memory > entry_size * 100;
        } catch (...) {
            return false;
        }
    };

    while (source_pos < versions.size()) {
        auto it = source_map->begin();

        while (it != source_map->end()) {
            if (!can_insert_in_map(*target_map, *target_segment)) {
                target_segment.reset();
                target_idx = source_idx;
                target_segment = std::move(source_segment);
                target_map = source_map;

                ++source_pos;
                if (source_pos >= versions.size()) break;
                source_idx = versions[source_pos];

                source_segment = open_compact_file(source_idx);
                source_map = source_segment->find<compact_map_t>("db_map").first;
                break;
            }

            auto key = it->first;
            auto value = it->second;

            try {
                auto const [pos, inserted] = target_map->emplace(key, value);
                if ( ! inserted) {
                    // See compact_container(): a collision here proves two
                    // version files held the same key. Report, do not repair.
                    log::error("compaction: duplicate key in compact container — present in both v{} and v{}: {}",
                               source_idx, target_idx, outpoint_to_string(key));
                    return std::unexpected(error_code::duplicate_key);
                }
                it = source_map->erase(it);
                ++entries_moved;
            } catch (boost::interprocess::bad_alloc const&) {
                target_segment.reset();
                target_idx = source_idx;
                target_segment = std::move(source_segment);
                target_map = source_map;

                ++source_pos;
                if (source_pos >= versions.size()) break;
                source_idx = versions[source_pos];

                source_segment = open_compact_file(source_idx);
                source_map = source_segment->find<compact_map_t>("db_map").first;
                it = source_map->begin();
            }
        }

        if (source_map && source_map->empty()) {
            source_segment.reset();

            // Data first, then metadata, and the catalogue follows each result:
            // see compact_container() for why each outcome is handled the way
            // it is. TODO(#59) applies here too.
            std::error_code ec;
            fs::remove(fmt::format(compact_data_file_format, db_path_.string(), source_idx), ec);
            if (ec) {
                log::error("compaction: could not remove compact v{}: {}", source_idx, ec.message());
                return std::unexpected(error_code::removal_failed);
            }

            compact_catalog_.remove(source_idx);
            versions.erase(versions.begin() + std::ptrdiff_t(source_pos));
            ++files_deleted;

            fs::remove(fmt::format("{}/meta_compact_v{:05}.dat", db_path_.string(), source_idx), ec);
            if (ec) {
                log::error("compaction: removed compact v{} but not its metadata: {}",
                           source_idx, ec.message());
                return std::unexpected(error_code::removal_failed);
            }

            if (source_pos < versions.size()) {
                source_idx = versions[source_pos];
                source_segment = open_compact_file(source_idx);
                source_map = source_segment->find<compact_map_t>("db_map").first;
            }
        }
    }

    log::debug("Compact compaction complete: {} files deleted, {} entries moved",
              files_deleted, entries_moved);
    return {};
}

// =============================================================================
// database_impl - Compact metadata helpers
// =============================================================================

void database_impl::compact_save_metadata(size_t version) {
    auto const* meta_ptr = compact_catalog_.find_metadata(version);
    if ( ! meta_ptr) return;

    auto const& meta = *meta_ptr;
    auto metadata_file = fmt::format("{}/meta_compact_v{:05}.dat", db_path_.string(), version);

    std::ofstream ofs(metadata_file, std::ios::binary);
    if (!ofs) {
        log::warn("Failed to save compact metadata: {}", metadata_file);
        return;
    }

    uint64_t entry_count = meta.entry_count;
    ofs.write(reinterpret_cast<char const*>(&meta.min_block_height), sizeof(meta.min_block_height));
    ofs.write(reinterpret_cast<char const*>(&meta.max_block_height), sizeof(meta.max_block_height));
    ofs.write(reinterpret_cast<char const*>(meta.min_key.data()), meta.min_key.size());
    ofs.write(reinterpret_cast<char const*>(meta.max_key.data()), meta.max_key.size());
    ofs.write(reinterpret_cast<char const*>(&entry_count), sizeof(entry_count));
}

void database_impl::compact_load_metadata(size_t version) {
    auto metadata_file = fmt::format("{}/meta_compact_v{:05}.dat", db_path_.string(), version);

    std::ifstream ifs(metadata_file, std::ios::binary);
    if (!ifs) return;

    auto& meta = compact_catalog_.metadata(version);
    meta.container_index = compact_sentinel_index;
    meta.version = version;

    uint64_t entry_count = 0;
    ifs.read(reinterpret_cast<char*>(&meta.min_block_height), sizeof(meta.min_block_height));
    ifs.read(reinterpret_cast<char*>(&meta.max_block_height), sizeof(meta.max_block_height));
    ifs.read(reinterpret_cast<char*>(meta.min_key.data()), meta.min_key.size());
    ifs.read(reinterpret_cast<char*>(meta.max_key.data()), meta.max_key.size());
    ifs.read(reinterpret_cast<char*>(&entry_count), sizeof(entry_count));
    meta.entry_count = static_cast<size_t>(entry_count);
}

// =============================================================================
// database_impl - Config persistence
// =============================================================================

void database_impl::save_config_to_disk() {
    auto config_path = db_path_ / "utxoz_config.dat";
    std::ofstream ofs(config_path, std::ios::binary);
    if (!ofs) {
        log::warn("Failed to save config: {}", config_path.string());
        return;
    }

    // Magic: "UTXO"
    char magic[4] = {'U', 'T', 'X', 'O'};
    ofs.write(magic, 4);

    // Version
    uint32_t version = 1;
    ofs.write(reinterpret_cast<char const*>(&version), sizeof(version));

    // Mode
    uint8_t mode_byte = static_cast<uint8_t>(mode_);
    ofs.write(reinterpret_cast<char const*>(&mode_byte), sizeof(mode_byte));
}

result<> database_impl::load_config_from_disk() {
    auto config_path = db_path_ / "utxoz_config.dat";
    std::ifstream ifs(config_path, std::ios::binary);
    if (!ifs) {
        return std::unexpected(error_code::config_file_corrupt);
    }

    char magic[4];
    ifs.read(magic, 4);
    if (!ifs || magic[0] != 'U' || magic[1] != 'T' || magic[2] != 'X' || magic[3] != 'O') {
        return std::unexpected(error_code::config_file_corrupt);
    }

    uint32_t version;
    ifs.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (!ifs) {
        return std::unexpected(error_code::config_file_corrupt);
    }
    if (version != 1) {
        return std::unexpected(error_code::config_file_corrupt);
    }

    uint8_t mode_byte;
    ifs.read(reinterpret_cast<char*>(&mode_byte), sizeof(mode_byte));
    if (!ifs) {
        return std::unexpected(error_code::config_file_corrupt);
    }
    if (mode_byte != static_cast<uint8_t>(storage_mode::full) &&
        mode_byte != static_cast<uint8_t>(storage_mode::compact)) {
        return std::unexpected(error_code::config_file_corrupt);
    }
    mode_ = static_cast<storage_mode>(mode_byte);
    return {};
}

// =============================================================================
// database_impl - Typed full-mode methods
// =============================================================================

std::optional<full_find_result> database_impl::full_find(raw_outpoint const& key, uint32_t height) const {
    // Try current version first
    std::optional<full_find_result> result;

    for_each_index<container_count>([&](auto I) {
        if (!result) {
            auto& map = container<I>();
            if (auto it = map.find(key); it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
                probe_stats_.record_answered(height, it->second.block_height);
#endif
                auto data = it->second.get_data();
                result = full_find_result{bytes(data.begin(), data.end()), it->second.block_height};
            }
        }
    });

    if (result) return result;

    // Defer lookup to batch processing for efficiency
    // A probe the active map could not answer. Recording it is what makes the
    // hit rate mean something: without it every recorded probe was a hit.
    probe_stats_.record_deferred();
    add_to_deferred_lookups(key, height);
    return std::nullopt;
}

std::pair<flat_map<raw_outpoint, full_find_result>, std::vector<deferred_lookup_entry>>
database_impl::full_process_pending_lookups() {
    if (deferred_lookups_.empty()) return {};

    flat_map<raw_outpoint, full_find_result> successful_lookups;

#ifdef UTXOZ_STATISTICS_ENABLED
    auto const start_time = std::chrono::steady_clock::now();
    ++deferred_stats_.processing_runs;
#endif

    size_t initial_size = deferred_lookups_.size();
    log::debug("Processing {} deferred full lookups...", initial_size);

    auto process_full_file = [&]<size_t Index>(std::integral_constant<size_t, Index>, size_t version) {
        try {
            auto [map, cache_hit] = file_cache_->get_or_open_file<Index>(Index, version);

            resolution_stats_.record_file_visited(cache_hit);


            deferred_lookups_.erase_if([&](auto const& entry) {
                auto map_it = map.find(entry.key);
                if (map_it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
                    auto depth = static_cast<uint32_t>(current_versions_[Index] - version);
                    ++deferred_stats_.lookups_by_depth[depth];
                    resolution_stats_.record_resolved(depth);
#endif
                    auto data = map_it->second.get_data();
                    successful_lookups.emplace(entry.key,
                        full_find_result{bytes(data.begin(), data.end()), map_it->second.block_height});
                    return true;
                }
                return false;
            });
        } catch (std::exception const& e) {
            log::error("Error processing full lookups (container {}, v{}): {}", Index, version, e.what());
        }
    };

    // Phase 1: cached files first
    auto cached_files = file_cache_->get_cached_files();
    if (!cached_files.empty()) {
        std::ranges::sort(cached_files, [](auto const& a, auto const& b) {
            if (a.first != b.first) return a.first < b.first;
            return a.second > b.second;
        });

        for (auto const& [container_index, version] : cached_files) {
            if (deferred_lookups_.empty()) break;
            if (container_index == compact_sentinel_index) continue;
            switch (container_index) {
                case 0: process_full_file(std::integral_constant<size_t, 0>{}, version); break;
                case 1: process_full_file(std::integral_constant<size_t, 1>{}, version); break;
                case 2: process_full_file(std::integral_constant<size_t, 2>{}, version); break;
                case 3: process_full_file(std::integral_constant<size_t, 3>{}, version); break;
                case 4: process_full_file(std::integral_constant<size_t, 4>{}, version); break;
            }
        }
    }

    // Phase 2: remaining files
    if (!deferred_lookups_.empty()) {
        std::array<std::set<size_t>, container_count> processed_versions;
        for (auto const& [container_index, version] : cached_files) {
            if (container_index < container_count) {
                processed_versions[container_index].insert(version);
            }
        }

        for_each_index<container_count>([&](auto I) {
            if (deferred_lookups_.empty()) return;

            for (auto const v : catalogs_[I.value].below(current_versions_[I.value])) {
                if (deferred_lookups_.empty()) break;
                if (processed_versions[I.value].contains(v)) continue;

                process_full_file(I, v);
            }
        });
    }

    // Collect failed lookups
    std::vector<deferred_lookup_entry> failed_lookups;
    failed_lookups.reserve(deferred_lookups_.size());
    resolution_stats_.record_unresolved(deferred_lookups_.size());
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

    log::debug("Deferred full lookups complete: {} found, {} not found",
               successful_lookups.size(), failed_lookups.size());

    return {std::move(successful_lookups), std::move(failed_lookups)};
}

// =============================================================================
// database_impl - Typed compact-mode methods
// =============================================================================

result<bool> database_impl::compact_insert_typed(raw_outpoint const& key, uint32_t height,
                                                 uint32_t file_number, uint32_t offset) {
    if (!compact_can_insert_safely()) {
        log::debug("Rotating compact container due to safety constraints");
        compact_new_version();
    }

    compact_value val;
    val.height = height;
    val.file_number = file_number;
    val.offset = offset;

    size_t max_retries = 3;
    while (max_retries > 0) {
        try {
            auto& map = compact_map();
            [[maybe_unused]] size_t bucket_count_before = map.bucket_count();

            auto [it, inserted] = map.emplace(key, val);
            if (!inserted) {
                log::warn("compact_insert_typed: duplicate key at height {}, outpoint={}",
                    height, outpoint_to_string(key));
            }
            if (inserted) {
                ++entries_count_;

#ifdef UTXOZ_STATISTICS_ENABLED
                ++container_stats_[0].total_inserts;
                ++container_stats_[0].current_size;
                ++container_stats_[0].value_size_distribution[sizeof(uint32_t) * 2];
                ++height_range_stats_.ranges[height / height_range_stats::range_size].inserts[0];

                if (map.bucket_count() != bucket_count_before) {
                    ++container_stats_[0].rehash_count;
                }
#endif

                compact_catalog_.metadata(compact_current_version_).update_on_insert(key, height);
            }
            return inserted;

        } catch (boost::interprocess::bad_alloc const& e) {
            log::error("Error inserting into compact container: {}", e.what());
            compact_new_version();
        }
        --max_retries;
    }

    log::error("Failed to insert into compact container after 3 retries");
    throw boost::interprocess::bad_alloc();
}

std::optional<compact_find_result> database_impl::compact_find_typed(raw_outpoint const& key, uint32_t height) const {
    auto const& map = compact_map();
    if (auto it = map.find(key); it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
        probe_stats_.record_answered(height, it->second.height);
#endif
        return compact_find_result{it->second.height, it->second.file_number, it->second.offset};
    }

    // A probe the active map could not answer. Recording it is what makes the
    // hit rate mean something: without it every recorded probe was a hit.
    probe_stats_.record_deferred();
    add_to_deferred_lookups(key, height);
    return std::nullopt;
}

std::pair<flat_map<raw_outpoint, compact_find_result>, std::vector<deferred_lookup_entry>>
database_impl::compact_process_pending_lookups() {
    if (deferred_lookups_.empty()) return {};

    flat_map<raw_outpoint, compact_find_result> successful_lookups;

#ifdef UTXOZ_STATISTICS_ENABLED
    auto const start_time = std::chrono::steady_clock::now();
    ++deferred_stats_.processing_runs;
#endif

    size_t initial_size = deferred_lookups_.size();
    log::debug("Processing {} deferred compact lookups...", initial_size);

    auto process_compact_file = [&](size_t version, bool /*is_cached*/) {
        try {
            auto [map, cache_hit] = file_cache_->get_or_open_compact_file(version);

            resolution_stats_.record_file_visited(cache_hit);


            deferred_lookups_.erase_if([&](auto const& entry) {
                auto map_it = map.find(entry.key);
                if (map_it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
                    auto depth = static_cast<uint32_t>(compact_current_version_ - version);
                    ++deferred_stats_.lookups_by_depth[depth];
                    resolution_stats_.record_resolved(depth);
#endif
                    successful_lookups.emplace(entry.key,
                        compact_find_result{map_it->second.height, map_it->second.file_number, map_it->second.offset});
                    return true;
                }
                return false;
            });
        } catch (std::exception const& e) {
            log::error("Error processing compact lookups v{}: {}", version, e.what());
        }
    };

    // Phase 1: cached files first
    auto cached_files = file_cache_->get_cached_files();
    if (!cached_files.empty()) {
        std::ranges::sort(cached_files, [](auto const& a, auto const& b) {
            if (a.first != b.first) return a.first < b.first;
            return a.second > b.second;
        });

        for (auto const& [ci, version] : cached_files) {
            if (deferred_lookups_.empty()) break;
            if (ci == compact_sentinel_index) {
                process_compact_file(version, true);
            }
        }
    }

    // Phase 2: remaining files
    if (!deferred_lookups_.empty()) {
        std::set<size_t> processed_versions;
        for (auto const& [ci, version] : cached_files) {
            if (ci == compact_sentinel_index) {
                processed_versions.insert(version);
            }
        }

        for (auto const v : compact_catalog_.below(compact_current_version_)) {
            if (deferred_lookups_.empty()) break;
            if (processed_versions.contains(v)) continue;

            process_compact_file(v, false);
        }
    }

    // Collect failed lookups
    std::vector<deferred_lookup_entry> failed_lookups;
    failed_lookups.reserve(deferred_lookups_.size());
    resolution_stats_.record_unresolved(deferred_lookups_.size());
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

    log::debug("Deferred compact lookup complete: {} successful, {} failed",
              successful_lookups.size(), failed_lookups.size());

    return {std::move(successful_lookups), std::move(failed_lookups)};
}

void database_impl::compact_for_each_entry_typed(
    void(*cb)(void*, raw_outpoint const&, uint32_t, uint32_t, uint32_t), void* ctx) const {

    // Current version
    auto const& map = compact_map();
    for (auto const& [key, val] : map) {
        cb(ctx, key, val.height, val.file_number, val.offset);
    }

    // Previous versions
    for (auto const v : compact_catalog_.below(compact_current_version_)) {
        auto file_name = fmt::format(compact_data_file_format, db_path_.string(), v);

        try {
            auto segment = std::make_unique<bip::managed_mapped_file>(bip::open_only, file_name.c_str());
            auto* map_ptr = segment->find<compact_map_t>("db_map").first;
            if (!map_ptr) continue;

            for (auto const& [key, val] : *map_ptr) {
                cb(ctx, key, val.height, val.file_number, val.offset);
            }
        } catch (std::exception const& e) {
            log::error("compact_for_each_entry_typed: error reading compact v{}: {}", v, e.what());
        }
    }
}

// =============================================================================
// Explicit template instantiations
// =============================================================================

template void database_impl::open_or_create_container<0>(size_t);
template void database_impl::open_or_create_container<1>(size_t);
template void database_impl::open_or_create_container<2>(size_t);
template void database_impl::open_or_create_container<3>(size_t);
template void database_impl::open_or_create_container<4>(size_t);

template void database_impl::close_container<0>();
template void database_impl::close_container<1>();
template void database_impl::close_container<2>();
template void database_impl::close_container<3>();
template void database_impl::close_container<4>();

template void database_impl::new_version<0>();
template void database_impl::new_version<1>();
template void database_impl::new_version<2>();
template void database_impl::new_version<3>();
template void database_impl::new_version<4>();

template result<> database_impl::compact_container<0>();
template result<> database_impl::compact_container<1>();
template result<> database_impl::compact_container<2>();
template result<> database_impl::compact_container<3>();
template result<> database_impl::compact_container<4>();

template bool database_impl::insert_in_index<0>(raw_outpoint const&, output_data_span, uint32_t);
template bool database_impl::insert_in_index<1>(raw_outpoint const&, output_data_span, uint32_t);
template bool database_impl::insert_in_index<2>(raw_outpoint const&, output_data_span, uint32_t);
template bool database_impl::insert_in_index<3>(raw_outpoint const&, output_data_span, uint32_t);
template bool database_impl::insert_in_index<4>(raw_outpoint const&, output_data_span, uint32_t);

} // namespace utxoz::detail
