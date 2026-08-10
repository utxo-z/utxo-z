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
    containers_[Index] = segment->find_or_construct<utxo_map<container_sizes[Index]>>(map_object_name)(
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
    // The version being retired received every insert since it was opened, and
    // closing it flushes asynchronously — which schedules writeback and
    // promises nothing. Once it stops being active it is no longer covered by
    // the active-container barriers, so the obligation is recorded here.
    //
    // Recorded at the rotation rather than at each insert on purpose: the
    // active version is covered as an active container for as long as it is
    // one, so the only moment coverage could be lost is the moment it stops
    // being active. Marking per insert would put a set lookup on the hot path
    // to record something already known.
    note_dirty(Index, current_versions_[Index]);

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

namespace {

/// Says how a version came to have no metadata. Absent is the ordinary state
/// and says nothing; the rest are worth a line, because they mean a file is
/// there and cannot be believed.
void report_metadata_read_error(metadata_read_error err, std::string_view where, size_t version) {
    switch (err) {
        case metadata_read_error::absent:
            break;
        case metadata_read_error::unreadable:
            log::warn("Metadata for {} v{} could not be read; treating it as unknown", where, version);
            break;
        case metadata_read_error::foreign:
            log::info("Metadata for {} v{} is not in a format this build reads; treating it as unknown",
                      where, version);
            break;
        case metadata_read_error::malformed:
            log::warn("Metadata for {} v{} is damaged; treating it as unknown. The version file "
                      "itself is unaffected and remains fully searchable.", where, version);
            break;
    }
}

} // anonymous namespace

void database_impl::save_metadata_to_disk(size_t index, size_t version) noexcept {
    if (index >= catalogs_.size()) return;
    auto const* meta_ptr = catalogs_[index].find_metadata(version);
    if ( ! meta_ptr) return;

    // Reachable from close(), which runs from destructors and from the scope
    // guard that compaction unwinds through, so nothing may escape — including
    // from the reporting itself, and including whatever a formatting or
    // allocation failure would raise.
    try {
        auto const path = fmt::format("{}/meta_{}_v{:05}.dat", db_path_.string(), index, version);
        if (auto const written = write_metadata_file(path, *meta_ptr); ! written) {
            // Derived data: failing to persist it costs a rescan later and
            // nothing else. What must not happen — a half-written record read
            // back as a valid one — is prevented by publishing through an
            // atomic replace, not by this branch.
            log::warn("Could not publish metadata for container {} v{}", index, version);
        }
    } catch (...) {
    }
}

void database_impl::load_metadata_from_disk(size_t index, size_t version) {
    auto const path = fmt::format("{}/meta_{}_v{:05}.dat", db_path_.string(), index, version);

    auto record = read_metadata_file(path);
    if ( ! record) {
        // Nothing is created for this version: absent and damaged both leave the
        // metadata *unknown*, which every consumer must already handle, and
        // which is the only safe reading of a record we cannot trust. The
        // version file itself is untouched and fully searchable either way.
        report_metadata_read_error(record.error(), fmt::format("container {}", index), version);
        return;
    }

    auto& meta = catalogs_[index].metadata(version);
    meta = *record;
    meta.container_index = index;
    meta.version = version;
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
    std::error_code ec;
    fs::create_directories(db_path_, ec);
    if (ec) return std::unexpected(error_code::catalog_unreadable);

    // Any claim this instance already held goes first. Pointed at the same
    // directory it would otherwise refuse itself — the lock belongs to an open
    // file description, and a second descriptor on the same file conflicts even
    // within one process — and pointed at a different one it would keep the old
    // database claimed for nothing.
    lock_.release();

    // The claim comes before everything: before recovery, which unlinks files
    // and which two processes must never run over one directory at once, and
    // before the mode check, so a database that is both in use and of the wrong
    // mode reports the one that stops you either way.
    //
    // On the way out it is released by the destructor: a configure() that fails
    // below leaves open() destroying this instance, and the claim goes with it.
    // That is a property of how open() uses the impl, not of an arbitrary
    // return from here — which is why the release above stands on its own, for
    // an instance that were ever configured twice.
    auto claimed = database_lock::acquire(db_path_);
    if ( ! claimed) return std::unexpected(claimed.error());
    lock_ = std::move(*claimed);
    lock_.record_holder();

    if (remove_existing) {
        // The children, not the directory. Removing the directory would unlink
        // the file this instance holds its claim on: the lock would survive on
        // an inode with no name, while a second process created a fresh lock
        // file, locked a different inode, and both believed they were alone.
        // Checked where each step happens, not at the top of the next
        // iteration. A range-for advances with the throwing operator, and a
        // failure to construct leaves the iterator at the end — so the body
        // never runs, the check inside it never fires, and a directory that
        // could not be read looks like a directory that was already empty.
        // That is the same mistake enumerate_versions() exists to avoid.
        fs::directory_iterator it(db_path_, ec);
        if (ec) return std::unexpected(error_code::catalog_unreadable);

        auto const end = fs::directory_iterator{};
        while (it != end) {
            if (it->path().filename() != database_lock::file_name) {
                fs::remove_all(it->path(), ec);
                if (ec) return std::unexpected(error_code::catalog_unreadable);
            }

            // After the increment, not at the top of the next iteration: a
            // failing increment can leave the iterator at the end, and a check
            // placed there would simply never run.
            it.increment(ec);
            if (ec) return std::unexpected(error_code::catalog_unreadable);
        }
    }

    // Check config persistence (detect mode mismatch on reopen)
    auto config_path = db_path_ / "utxoz_config.dat";
    auto const config_exists = path_exists(config_path);
    if ( ! config_exists) return std::unexpected(config_exists.error());

    // Written once, when the database is created, and never again: the content
    // does not change, and rewriting it on every open would put a barrier — and
    // a way to fail — on a path that has nothing to say.
    bool must_write_config = false;

    if (*config_exists && !remove_existing) {
        if (auto r = load_config_from_disk(); !r) {
            return std::unexpected(r.error());
        }
        if (mode_ != mode) {
            return std::unexpected(error_code::storage_mode_mismatch);
        }
    } else {
        must_write_config = true;
        // No config file — check for pre-existing data files from the other mode
        if (!remove_existing) {
            auto const other_mode_file = mode == storage_mode::reference
                ? fmt::format(data_file_format, db_path_.string(), 0, 0)
                : fmt::format(reference_data_file_format, db_path_.string(), 0);

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

    // Before any container is opened, so an intermediate state left by a
    // previous process is never observable. With nothing in flight this costs
    // one directory listing per container, which open() already does.
    if (auto const recovered = recover_pending_merges(); ! recovered) {
        return std::unexpected(recovered.error());
    }

    if (mode_ == storage_mode::reference) {
        // Reference mode: single container
        reference_active_file_size_ = (active_file_sizes_[0] == file_sizes[0])
            ? reference_file_size : reference_test_file_size;

        auto path_str = db_path_.string();
        reference_min_buckets_ok_ = find_optimal_buckets_reference(path_str, reference_active_file_size_, 7864304);

        // Build the catalogue before anything is opened. A directory we cannot
        // read is not an empty directory: opening on that assumption would
        // create v0 over a database that already has versions in it.
        auto listed = enumerate_versions(db_path_, "compact_v");
        if ( ! listed) return std::unexpected(listed.error());

        reference_catalog_.clear();
        for (auto const v : *listed) reference_catalog_.add(v);

        size_t const latest_version = reference_catalog_.active();
        reference_open_or_create(latest_version);
        reference_catalog_.add(latest_version);   // a fresh database has just created it
        entries_count_ += reference_map().size();

        // Count entries in previous versions (still searchable/deletable)
        for (auto const v : reference_catalog_.below(latest_version)) {
            auto file_name = fmt::format(reference_data_file_format, db_path_.string(), v);
            try {
                auto segment = open_existing_segment(file_name);
                auto* map_ptr = segment->template find<reference_map_t>(map_object_name).first;
                if (map_ptr) {
                    entries_count_ += map_ptr->size();
                }
            } catch (std::exception const& e) {
                log::error("configure: error counting reference entries v{}: {}", v, e.what());
            }
        }

        for (auto const v : reference_catalog_.versions()) {
            reference_load_metadata(v);
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
                    auto segment = open_existing_segment(file_name);
                    auto* map_ptr = segment->template find<utxo_map<container_sizes[I]>>(map_object_name).first;
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

    if (must_write_config) return save_config_to_disk();
    return {};
}

void database_impl::close() {
    if (mode_ == storage_mode::reference) {
        reference_close_container();
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
    if (mode_ == storage_mode::reference) {
        return reference_insert(key, value, height);
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
    if (mode_ == storage_mode::reference) {
        return reference_find(key, height);
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
    if (mode_ == storage_mode::reference) {
        return reference_erase(key, height);
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

                        // A historical file written to outside the sweep. The
                        // obligation is the same, and so is the reason: the
                        // mapping can be evicted before the next sync.
                        note_dirty(container_index, version);
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
        if (mode_ == storage_mode::reference) {
            std::set<size_t> processed_versions_reference;
            for (auto const& [ci, version] : cached_files) {
                if (ci == reference_sentinel_index) {
                    processed_versions_reference.insert(version);
                }
            }

            // Nearest generation first, over the versions that exist. Walking
            // a range down from the active one would visit every number ever
            // rotated through, and those never come back.
            for (auto const v : reference_catalog_.below(reference_current_version_)) {
                if (deferred_deletions_.empty()) break;
                if (processed_versions_reference.contains(v)) continue;

                successful_deletions += process_deferred_deletions_in_file(reference_sentinel_index, v, false);
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
                    // Written to. The obligation outlives the mapping, which
                    // the LRU may evict before this sweep is over.
                    note_dirty(container_index, version);
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

    if (container_index == reference_sentinel_index) {
        // Reference mode deferred deletions
        try {
            auto [map, cache_hit] = file_cache_->get_or_open_reference_file(version);

            auto it = deferred_deletions_.begin();
            while (it != deferred_deletions_.end()) {
                auto erased_count = map.erase(it->key);
                if (erased_count > 0) {
                    note_dirty(reference_sentinel_index, version);
                    if (auto* meta = reference_catalog_.find_metadata(version)) {
                        meta->update_on_delete();
                    }
#ifdef UTXOZ_STATISTICS_ENABLED
                    auto depth = static_cast<uint32_t>(reference_current_version_ - version);
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
            log::error("Error processing reference file v{}: {}", version, e.what());
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
        if (mode_ == storage_mode::reference) {
            std::set<size_t> processed_versions_reference;
            for (auto const& [ci, version] : cached_files) {
                if (ci == reference_sentinel_index) {
                    processed_versions_reference.insert(version);
                }
            }

            for (auto const v : reference_catalog_.below(reference_current_version_)) {
                if (deferred_lookups_.empty()) break;
                if (processed_versions_reference.contains(v)) continue;

                process_deferred_lookups_in_file(reference_sentinel_index, v, false, successful_lookups);
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

    if (container_index == reference_sentinel_index) {
        try {
            auto [map, cache_hit] = file_cache_->get_or_open_reference_file(version);

            resolution_stats_.record_file_visited(cache_hit);


            deferred_lookups_.erase_if([&](auto const& entry) {
                auto map_it = map.find(entry.key);
                if (map_it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
                    auto depth = static_cast<uint32_t>(reference_current_version_ - version);
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
            log::error("Error processing reference lookups v{}: {}", version, e.what());
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

// =============================================================================
// database_impl - Crash-atomic compaction
// =============================================================================
//
// A merge builds a new version file from several old ones and then retires
// them. The canonical name is granted last and only to a file whose contents
// are already durable, so the existence of that name is itself the proof that
// the file is complete: recovery never has to verify a merge, re-read a source,
// or compare payloads.
//
// The sidecar is published before the target, never after. Reversed, a crash
// between the two would leave the target and its sources all canonical with
// nothing recording that the sources are superseded.

std::string database_impl::data_path(size_t index, size_t version) const {
    if (index == reference_sentinel_index) {
        return fmt::format(reference_data_file_format, db_path_.string(), version);
    }
    return fmt::format(data_file_format, db_path_.string(), index, version);
}

std::string database_impl::building_path(size_t index, size_t version) const {
    return data_path(index, version) + ".building";
}

std::string database_impl::sidecar_path(size_t index, size_t version) const {
    if (index == reference_sentinel_index) {
        return fmt::format("{}/compact_v{:05}.merge", db_path_.string(), version);
    }
    return fmt::format("{}/cont_{}_v{:05}.merge", db_path_.string(), index, version);
}

std::string database_impl::metadata_path(size_t index, size_t version) const {
    if (index == reference_sentinel_index) {
        return fmt::format("{}/meta_compact_v{:05}.dat", db_path_.string(), version);
    }
    return fmt::format("{}/meta_{}_v{:05}.dat", db_path_.string(), index, version);
}

result<> database_impl::directory_barrier(failpoints::dir_barrier stage) const {
    if (failpoints::fail_directory_barrier_at.load(std::memory_order_relaxed) == stage) {
        return std::unexpected(error_code::sync_failed);
    }
    return sync_directory(db_path_);
}

/**
 * Reads the marker a merge wrote into its target before making it durable.
 *
 * Every way this can fail to produce exactly one marker is a reason not to
 * retire anything: a file with no marker was not written by a merge, one with a
 * different marker belongs to a different merge, and one that cannot be opened
 * has told us nothing. The identity alone proves none of that — nothing locks
 * the database, so a file can be at that number for reasons this plan knows
 * nothing about.
 */
result<merge_marker> database_impl::read_target_marker(size_t index, size_t version) const {
    auto const path = data_path(index, version);
    try {
        auto const segment = open_existing_segment(path);
        auto const found = segment->find<merge_marker>(merge_marker::object_name);
        if (found.first == nullptr) {
            log::error("recovery: {} carries no merge marker", path);
            return std::unexpected(error_code::recovery_failed);
        }
        if (found.second != 1) {
            log::error("recovery: {} carries {} merge markers", path, found.second);
            return std::unexpected(error_code::recovery_failed);
        }
        return *found.first;
    } catch (std::exception const& e) {
        log::error("recovery: {} could not be opened to read its merge marker: {}", path, e.what());
        return std::unexpected(error_code::recovery_failed);
    }
}

namespace {

/// Removes a path, treating "it was not there" as success. Recovery has to be
/// idempotent: a crash part way through it must leave a state it can finish.
[[nodiscard]]
result<> remove_if_present(std::string const& path) {
    return remove_file(path);
}

} // anonymous namespace

/**
 * Finishes or abandons one interrupted merge.
 *
 * Which of the two depends on a single question — did the target get its
 * canonical name — and the answer is on disk. Everything here is idempotent.
 */
result<> database_impl::recover_one(merge_plan const& plan, std::string const& sidecar) {
    auto const target_path = data_path(plan.container, plan.target);

    auto const target_exists = path_exists(target_path);
    if ( ! target_exists) return std::unexpected(target_exists.error());

    if ( ! *target_exists) {
        // The merge never published. The sources were never touched, so the
        // whole thing is undone by removing what it left behind.
        log::info("Recovery: abandoning an unpublished merge into {} v{}",
                  plan.container, plan.target);

        if (auto const r = remove_if_present(building_path(plan.container, plan.target)); ! r) return r;
        if (auto const r = remove_if_present(sidecar); ! r) return r;

        if (auto const synced = sync_directory(db_path_);
            ! synced && synced.error() != error_code::sync_unsupported) {
            return std::unexpected(synced.error());
        }
        return {};
    }

    // The target is published, so its contents are complete — that is what the
    // ordering bought. But "a file exists at that identity" is not "this is the
    // file the plan meant", so the marker is checked before anything is
    // retired. A mismatch, a missing marker or a file that will not open all
    // stop here with every source, the target, the sidecar and the metadata
    // exactly where they are.
    auto const marker = read_target_marker(plan.container, plan.target);
    if ( ! marker) return std::unexpected(marker.error());

    if (*marker != plan.id) {
        log::error("recovery: the file at {} v{} was written by a different merge; "
                   "nothing will be retired", plan.container, plan.target);
        return std::unexpected(error_code::recovery_failed);
    }

    log::info("Recovery: finishing a published merge into {} v{}, retiring {} source(s)",
              plan.container, plan.target, plan.sources.size());

    for (auto const source : plan.sources) {
        if (source == plan.target) {
            // decode_merge_plan rejects this, so reaching it means the evidence
            // and the code disagree. Stop rather than unlink the target.
            return std::unexpected(error_code::recovery_failed);
        }
        if (auto const r = remove_if_present(data_path(plan.container, source)); ! r) return r;
        if (auto const r = remove_if_present(metadata_path(plan.container, source)); ! r) return r;
    }

    if (auto const synced = sync_directory(db_path_);
        ! synced && synced.error() != error_code::sync_unsupported) {
        return std::unexpected(synced.error());
    }

    // Only now: while the sidecar is there the state is recoverable, and it
    // stops being needed the moment there is nothing left to retire.
    if (auto const r = remove_if_present(sidecar); ! r) return r;
    if (auto const r = remove_if_present(building_path(plan.container, plan.target)); ! r) return r;

    if (auto const synced = sync_directory(db_path_);
        ! synced && synced.error() != error_code::sync_unsupported) {
        return std::unexpected(synced.error());
    }
    return {};
}

/**
 * Mandatory phase of open(). Acts only on evidence the store itself wrote, and
 * removes only files whose names are unambiguously in its reserved namespace —
 * anything it does not recognise is left alone.
 */
result<> database_impl::recover_pending_merges() {
    struct scope { size_t index; std::string prefix; };
    std::vector<scope> scopes;
    if (mode_ == storage_mode::reference) {
        scopes.push_back({reference_sentinel_index, "compact_v"});
    } else {
        for (size_t i = 0; i < container_count; ++i) {
            scopes.push_back({i, fmt::format("cont_{}_v", i)});
        }
    }

    for (auto const& sc : scopes) {
        auto listed = enumerate_versions(db_path_, sc.prefix, ".merge");
        if ( ! listed) return std::unexpected(listed.error());

        if (listed->size() > 1) {
            // One merge is in flight at a time, by construction. Several
            // sidecars mean something happened that this code cannot explain,
            // and choosing between them is exactly the guess that must not be
            // made.
            log::error("Recovery: {} merge records for container {}; refusing to guess",
                       listed->size(), sc.index);
            return std::unexpected(error_code::recovery_failed);
        }

        for (auto const target : *listed) {
            auto const path = sidecar_path(sc.index, target);
            auto const plan = read_merge_sidecar(path);
            if ( ! plan) {
                if (plan.error() == sidecar_read_error::absent) continue;
                log::error("Recovery: the merge record for container {} v{} cannot be read by "
                           "this build. Do not delete it: without it, nothing records which "
                           "files supersede which. Establish first whether the target exists.",
                           sc.index, target);
                return std::unexpected(error_code::recovery_failed);
            }
            if (plan->container != sc.index || plan->target != target) {
                log::error("Recovery: the merge record at {} describes a different operation", path);
                return std::unexpected(error_code::recovery_failed);
            }
            if (auto const r = recover_one(*plan, path); ! r) return r;
        }

        // A build with no sidecar never reached publication, and a half-written
        // sidecar never became evidence. Both are garbage, and only names that
        // parse as ours in the reserved namespace are touched.
        for (auto const& [suffix, describe] : std::initializer_list<std::pair<char const*, char const*>>{
                 {".dat.building", "an unfinished build"},
                 {".merge.tmp", "an unfinished merge record"}}) {
            auto stray = enumerate_versions(db_path_, sc.prefix, suffix);
            if ( ! stray) return std::unexpected(stray.error());
            for (auto const version : *stray) {
                log::info("Recovery: discarding {} of container {} v{}", describe, sc.index, version);
                if (auto const r = remove_if_present(
                        fmt::format("{}/{}{:05}{}", db_path_.string(), sc.prefix, version, suffix));
                    ! r) {
                    return r;
                }
            }
        }
    }

    return {};
}

// The six things that differ between the storage modes, and nothing else does.
template <size_t Index>
size_t full_merge_policy<Index>::index() const { return Index; }
template <size_t Index>
size_t full_merge_policy<Index>::file_size() const { return db.active_file_sizes_[Index]; }
template <size_t Index>
size_t full_merge_policy<Index>::min_buckets() const { return db.min_buckets_ok_[Index]; }
template <size_t Index>
version_catalog& full_merge_policy<Index>::catalogue() const { return db.catalogs_[Index]; }
template <size_t Index>
void full_merge_policy<Index>::save_metadata(size_t version) const {
    db.save_metadata_to_disk(Index, version);
}
template <size_t Index>
std::string full_merge_policy<Index>::describe(size_t version) const {
    return fmt::format("container {} v{}", Index, version);
}

size_t reference_merge_policy::index() const { return reference_sentinel_index; }
size_t reference_merge_policy::file_size() const { return db.reference_active_file_size_; }
size_t reference_merge_policy::min_buckets() const { return db.reference_min_buckets_ok_; }
version_catalog& reference_merge_policy::catalogue() const { return db.reference_catalog_; }
void reference_merge_policy::save_metadata(size_t version) const {
    db.reference_save_metadata(version);
}
std::string reference_merge_policy::describe(size_t version) const {
    return fmt::format("reference v{}", version);
}

template<typename Policy>
result<> database_impl::merge_versions(Policy policy, std::vector<size_t> const& sources) {
    auto const idx = policy.index();

    if (sources.size() < 2) return {};   // nothing to gain from rewriting one file

    // A fresh identity, never used before. It must not name anything that
    // exists: publishing over a file would destroy it, and a collision means
    // the catalogue and the directory disagree about what is there.
    size_t const target = policy.catalogue().next_version();

    auto const target_exists = path_exists(data_path(idx, target));
    if ( ! target_exists) return std::unexpected(target_exists.error());
    if (*target_exists) {
        log::error("compaction: the identity of {} is already taken", policy.describe(target));
        return std::unexpected(error_code::identity_collision);
    }

    // A metadata record for an identity with no data file is the second state
    // removal_failed describes. It must not survive to describe the new file.
    if (auto const r = remove_if_present(metadata_path(idx, target)); ! r) return r;

    // Preventive only: a real ENOSPC during the write stays authoritative. The
    // peak is one more file at the size this container is configured for, and
    // containers are merged one at a time so it does not accumulate.
    std::error_code space_ec;
    auto const space = fs::space(db_path_, space_ec);
    if ( ! space_ec && space.available < policy.file_size()) {
        log::error("compaction: {} needs {} bytes for a new file and {} are available",
                   policy.describe(target), policy.file_size(), space.available);
        return std::unexpected(error_code::insufficient_space);
    }

    auto const building = building_path(idx, target);
    auto const sidecar = sidecar_path(idx, target);

    // Generated once, written into the target and into the record that names
    // it. Recovery compares the two before retiring anything. Drawn before
    // anything is written, so a system that will not produce entropy stops the
    // merge rather than leaving it half identified.
    auto const drawn = generate_merge_id();
    if ( ! drawn) return std::unexpected(drawn.error());
    auto const merge_id = *drawn;

    // Until the target is published nothing canonical has changed, so every
    // failure below simply discards what was being built.
    auto abandon = [&]() -> void {
        std::error_code ec;
        fs::remove(building, ec);
    };

    size_t entries_moved = 0;
    try {
        std::error_code ec;
        fs::remove(building, ec);   // a leftover from a previous attempt

        auto segment = std::make_unique<bip::managed_mapped_file>(
            bip::create_only, building.c_str(), policy.file_size());
        auto* target_map = Policy::construct_map(*segment, policy.min_buckets());

        // Before the barriers, so the marker is as durable as the entries.
        segment->template construct<merge_marker>(merge_marker::object_name)(merge_id);

        for (auto const source : sources) {
            auto source_segment = open_existing_segment(data_path(idx, source));
            auto* source_map = Policy::find_map(*source_segment);
            if ( ! source_map) continue;

            for (auto const& [key, value] : *source_map) {
                try {
                    auto const [pos, inserted] = target_map->emplace(key, value);
                    if ( ! inserted) {
                        // Two sources held the same key. A published state holds
                        // at most one entry per key, so this is the database
                        // being locally inconsistent, and it is reported rather
                        // than resolved: choosing a copy would hide it. Nothing
                        // canonical has changed at this point.
                        log::error("compaction: duplicate key across the sources of {}: {}",
                                   policy.describe(target), outpoint_to_string(key));
                        source_segment.reset();
                        segment.reset();
                        abandon();
                        return std::unexpected(error_code::duplicate_key);
                    }
                    ++entries_moved;
                } catch (boost::interprocess::bad_alloc const&) {
                    // The group was planned to fit and did not. Leave every
                    // source exactly as it is and let the caller try a smaller
                    // group; sources are only ever read here.
                    log::debug("compaction: {} filled early, {} entries in",
                               policy.describe(target), entries_moved);
                    source_segment.reset();
                    segment.reset();
                    abandon();
                    return std::unexpected(error_code::insufficient_space);
                }
            }
        }

        // Mapped pages first, then the file itself: the page barrier covers the
        // pages, not the inode, and neither covers the name.
        if (auto const synced = sync_mapped_region(segment->get_address(), segment->get_size());
            ! synced && synced.error() != error_code::sync_unsupported) {
            segment.reset();
            abandon();
            return std::unexpected(synced.error());
        }
    } catch (std::exception const& e) {
        // Creating or mapping a file is where the filesystem says no — no room,
        // no permission, no file. Boost reports that by throwing, and this is a
        // result-typed API, so it stops here. Nothing canonical has changed:
        // the sources are only ever read.
        log::error("compaction: could not build {}: {}", policy.describe(target), e.what());
        abandon();
        return std::unexpected(error_code::file_open_failed);
    }

    failpoints::maybe_crash(failpoints::crash_point::after_build);

    if (auto const synced = sync_file(building);
        ! synced && synced.error() != error_code::sync_unsupported) {
        abandon();
        return std::unexpected(synced.error());
    }

    failpoints::maybe_crash(failpoints::crash_point::after_file_sync);

    // The sidecar goes before the target. It is the only thing that will tell a
    // later open that the sources are redundant.
    merge_plan plan;
    plan.container = idx;
    plan.target = target;
    plan.id = merge_id;
    plan.sources = sources;

    auto const written = write_merge_sidecar(sidecar, plan);
    if ( ! written) {
        // Failed before the rename: nothing was named, so the merge leaves no
        // trace and the instance is still sound.
        abandon();
        return std::unexpected(written.error());
    }
    if ( ! written->durable) {
        // Named, but the barrier that was to make the name durable failed. Two
        // futures are now possible and neither can be ruled out: a crash may
        // find this sidecar or may not. Removing it does not restore the
        // guarantee — the removal rests on the same barrier that just failed —
        // so the honest move is to stop. The instance latches, and reopening
        // resolves whichever state is really on disk: a sidecar with no target
        // abandons the merge, and no sidecar means there was never one.
        //
        // Continuing instead would let a second merge publish another sidecar
        // for this container, and a crash would then find two — which recovery
        // refuses to choose between, and rightly.
        log::error("compaction: the merge record for {} was published without a durable "
                   "barrier; the instance will not continue", policy.describe(target));
        // Tidy-up, not an undo, and deliberately unchecked: this removal rests
        // on the same barrier that just failed, so it may not persist either.
        // That is exactly why the latch below is unconditional rather than
        // contingent on it, and why the merge is not reported as "did not
        // publish". Recovery settles whichever state is really there.
        abandon();
        std::error_code ec;
        fs::remove(sidecar, ec);

        cleanup_pending_ = true;
        return std::unexpected(error_code::recovery_required);
    }

    failpoints::maybe_crash(failpoints::crash_point::after_sidecar_publish);
    failpoints::run_before_target_publish();

    // Publishing, not replacing. The identity was checked to be free above, but
    // that check is only an early answer: one instance per database is a
    // documented precondition that nothing currently enforces (#71). So this
    // refuses a taken name rather than replacing it, and the marker check below
    // is what actually establishes that the file is ours.
    if (auto const published = publish_new_file(building, data_path(idx, target)); ! published) {
        // The record is already durable. Calling this off means removing it, and
        // the removal is only real once the directory says so — a best-effort
        // delete would leave a record that may come back after a crash, naming a
        // target that was never published.
        auto const removed = failpoints::fail_sidecar_removal.load(std::memory_order_relaxed)
            ? result<>(std::unexpected(error_code::removal_failed))
            : remove_if_present(sidecar);
        auto const confirmed = removed ? directory_barrier(failpoints::dir_barrier::after_sidecar)
                                       : result<>{};

        if (removed && (confirmed || confirmed.error() == error_code::sync_unsupported)) {
            abandon();
            return std::unexpected(published.error());
        }

        log::error("compaction: {} could not be published and its merge record could not be "
                   "durably withdrawn; the instance will not continue", policy.describe(target));
        cleanup_pending_ = true;
        return std::unexpected(error_code::recovery_required);
    }
    failpoints::maybe_crash(failpoints::crash_point::after_target_publish);

    // The name is what makes the target findable. Retiring the sources while it
    // is not durable is how a power cut leaves the sources gone and the target
    // with them: contents durable, name not, sources unlinked. So where this
    // barrier exists, its failure stops the merge with every source untouched
    // and the record still in place.
    if (auto const synced = directory_barrier(failpoints::dir_barrier::after_target); ! synced) {
        if (synced.error() != error_code::sync_unsupported) {
            log::error("compaction: {} was published but its name could not be made durable; "
                       "no source will be retired", policy.describe(target));
            cleanup_pending_ = true;
            return std::unexpected(error_code::recovery_required);
        }
        // No such barrier here; see sync_support for what that costs.
    }

    // Still the file this plan meant. Nothing holds a lock between the check
    // that the identity was free and now (#71), so this is verified rather than
    // assumed — and before a single source is retired.
    {
        // Against the record's copy, not the local variable: the retirement
        // then rests on exactly the evidence recovery would use, rather than on
        // the assumption that the file just published is still ours.
        auto const marker = read_target_marker(idx, target);
        if ( ! marker || *marker != plan.id) {
            log::error("compaction: the file now at {} is not the one this merge built; "
                       "no source will be retired", policy.describe(target));
            cleanup_pending_ = true;
            return std::unexpected(error_code::recovery_failed);
        }
    }

    failpoints::maybe_crash(failpoints::crash_point::before_source_unlink);

    // Published. From here the sources are redundant and the catalogue says so.
    policy.catalogue().add(target);
    for (auto const source : sources) {
        policy.catalogue().remove(source);
        // The obligation goes with the file. Its entries are in the target,
        // which was made durable before it was published, so there is nothing
        // left to flush and nothing left to flush it to.
        dirty_versions_.erase({idx, source});
    }

    // Retire the sources. Every failure is recorded and the rest are still
    // attempted, but the operation does not report success while any of them
    // survives: until then several canonical files hold the same keys, and the
    // exclusion that keeps that unobservable ends when this call returns.
    auto retire = [](std::string const& path) -> result<> {
        if (failpoints::fail_source_unlink.load(std::memory_order_relaxed)) {
            return std::unexpected(error_code::removal_failed);
        }
        return remove_if_present(path);
    };

    bool all_retired = true;
    size_t retired = 0;
    for (auto const source : sources) {
        if (retired == 1) failpoints::maybe_crash(failpoints::crash_point::mid_source_unlink);
        ++retired;
        if (auto const r = retire(data_path(idx, source)); ! r) {
            log::error("compaction: could not retire {}", policy.describe(source));
            all_retired = false;
        }
        if (auto const r = retire(metadata_path(idx, source)); ! r) {
            log::error("compaction: could not retire the metadata of {}", policy.describe(source));
            all_retired = false;
        }
    }
    if (auto const synced = directory_barrier(failpoints::dir_barrier::after_source_retire);
        ! synced && synced.error() != error_code::sync_unsupported) {
        all_retired = false;
    }

    if ( ! all_retired) {
        // The sidecar stays: it is what makes the next open able to finish.
        cleanup_pending_ = true;
        return std::unexpected(error_code::recovery_required);
    }

    failpoints::maybe_crash(failpoints::crash_point::after_sources_retired);

    if (auto const r = remove_if_present(sidecar); ! r) {
        cleanup_pending_ = true;
        return std::unexpected(error_code::recovery_required);
    }
    if (auto const synced = directory_barrier(failpoints::dir_barrier::after_sidecar_removal);
        ! synced && synced.error() != error_code::sync_unsupported) {
        // The record is gone and so are the sources; a record that came back
        // would find its target published and nothing left to retire, which
        // recovery completes as a no-op.
        log::warn("compaction: the merge record of {} was removed without a barrier",
                  policy.describe(target));
    }

    // Metadata last, and only now: it describes a file that exists, and it is
    // rebuilt rather than carried over from anything.
    {
        auto& meta = policy.catalogue().metadata(target);
        meta = file_metadata{};
        meta.container_index = idx;
        meta.version = target;
        try {
            auto segment = open_existing_segment(data_path(idx, target));
            if (auto* map_ptr = Policy::find_map(*segment)) {
                for (auto const& [key, val] : *map_ptr) {
                    meta.update_on_insert(key, Policy::height_of(val));
                }
            }
        } catch (std::exception const& e) {
            log::warn("compaction: could not summarise {}: {}", policy.describe(target), e.what());
        }
        policy.save_metadata(target);
    }

    log::debug("Merged {} files into {}: {} entries",
               sources.size(), policy.describe(target), entries_moved);
    return {};
}

/**
 * Puts the active container back after a compaction.
 *
 * Its result is part of the operation's, never swallowed. Compaction closes the
 * active container before it starts, so a failure to reopen leaves
 * containers_[Index] null and the next operation dereferencing it — which is
 * why this both reports and latches: the instance has nothing to serve from
 * until it is closed and reopened.
 */
template<size_t Index>
result<> database_impl::reopen_active_container() {
    auto const active = catalogs_[Index].active();
    try {
        if (failpoints::fail_container_open.load(std::memory_order_relaxed)) {
            throw std::runtime_error("failpoint");
        }
        open_or_create_container<Index>(active);
        catalogs_[Index].add(active);
        return {};
    } catch (std::exception const& e) {
        log::error("compaction: container {} could not be reopened at v{}: {}", Index, active, e.what());
        cleanup_pending_ = true;
        return std::unexpected(error_code::file_open_failed);
    }
}

template<size_t Index>
result<> database_impl::compact_container() {
    log::debug("Starting compaction for container {}...", Index);

    close_container<Index>();

    // Compaction closes the active container before it starts, so every exit has
    // to put one back — including the exit nobody writes, taken by a throw. A
    // scope guard would do that, but a guard runs in a destructor and cannot
    // report, so a reopen that failed would leave the operation returning
    // success with nothing mapped. The merging is bracketed instead, and the
    // reopen's result is part of what comes back.
    auto outcome = [&]() -> result<> {
        try {
            return merge_groups(full_merge_policy<Index>{*this});
        } catch (std::exception const& e) {
            log::error("compaction: container {} failed: {}", Index, e.what());
            return std::unexpected(error_code::file_open_failed);
        }
    }();

    auto const reopened = reopen_active_container<Index>();

    // A failure inside the merge is the more informative one, so it wins; but a
    // failed reopen is never silent, and it has already latched the instance.
    if ( ! outcome) return outcome;
    return reopened;
}

template<typename Policy>
result<> database_impl::merge_groups(Policy policy) {
    auto const versions = policy.catalogue().versions();
    if (versions.size() <= 1) {
        log::trace("{} has {} files, no compaction needed",
                   policy.describe(policy.catalogue().active()), versions.size());
        return {};
    }

    // Groups are whole files. A source is never partially consumed, because a
    // partially consumed one would have to survive holding entries the new file
    // also holds — which is the duplicate this whole design exists to avoid.
    size_t first = 0;
    while (first < versions.size()) {
        size_t count = versions.size() - first;
        result<> outcome;

        // Try the largest group that is left and shrink until one fits. Sources
        // are only read, so a group that does not fit costs the build and
        // nothing else.
        while (count >= 2) {
            std::vector<size_t> const group(versions.begin() + std::ptrdiff_t(first),
                                            versions.begin() + std::ptrdiff_t(first + count));
            outcome = merge_versions(policy, group);
            if (outcome || outcome.error() != error_code::insufficient_space) break;
            --count;
        }

        if (count < 2) {
            // Nothing more can be combined starting here.
            ++first;
            continue;
        }
        if ( ! outcome) return outcome;

        first += count;
    }

    return {};
}

result<> database_impl::for_each_key_impl(void(*cb)(void*, raw_outpoint const&), void* ctx) const {
    if (mode_ == storage_mode::reference) {
        return reference_for_each_key(cb, ctx);
    }

    // A version that cannot be opened is not an empty version. Logging it and
    // carrying on reports a complete scan of an incomplete database, which is
    // the same class of mistake as reading an unreadable directory as empty.
    result<> outcome;
    for_each_index<container_count>([&](auto I) {
        if ( ! outcome.has_value()) return;
        // Current version (active container)
        auto const& map = container<I>();
        for (auto const& [key, _] : map) {
            cb(ctx, key);
        }

        // Previous versions
        for (auto const v : catalogs_[I].below(current_versions_[I])) {
            auto file_name = fmt::format(data_file_format, db_path_.string(), I.value, v);

            try {
                auto segment = open_existing_segment(file_name);
                auto* map_ptr = segment->template find<utxo_map<container_sizes[I]>>(map_object_name).first;
                if (!map_ptr) continue;

                for (auto const& [key, _] : *map_ptr) {
                    cb(ctx, key);
                }
            } catch (std::exception const& e) {
                log::error("for_each_key: error reading container {} v{}: {}", I.value, v, e.what());
                outcome = std::unexpected(error_code::file_open_failed);
                return;
            }
        }
    });

    return outcome;
}

result<> database_impl::for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, std::span<uint8_t const>), void* ctx) const {
    if (mode_ == storage_mode::reference) {
        return reference_for_each_entry(cb, ctx);
    }

    // See for_each_key_impl(): a partial scan is never reported as a whole one.
    result<> outcome;
    for_each_index<container_count>([&](auto I) {
        if ( ! outcome.has_value()) return;
        // Current version (active container)
        auto const& map = container<I>();
        for (auto const& [key, val] : map) {
            cb(ctx, key, val.block_height, val.get_data());
        }

        // Previous versions
        for (auto const v : catalogs_[I].below(current_versions_[I])) {
            auto file_name = fmt::format(data_file_format, db_path_.string(), I.value, v);

            try {
                auto segment = open_existing_segment(file_name);
                auto* map_ptr = segment->template find<utxo_map<container_sizes[I]>>(map_object_name).first;
                if (!map_ptr) continue;

                for (auto const& [key, val] : *map_ptr) {
                    cb(ctx, key, val.block_height, val.get_data());
                }
            } catch (std::exception const& e) {
                log::error("for_each_entry: error reading container {} v{}: {}", I.value, v, e.what());
                outcome = std::unexpected(error_code::file_open_failed);
                return;
            }
        }
    });

    return outcome;
}

/**
 * Puts everything written so far on stable storage.
 *
 * Three things carry writes and all three are covered. The active container of
 * each size class, obviously. The mappings the file cache holds, less
 * obviously: historical resolution erases entries in older generations through
 * it, so a sync that covered only the active containers would report a database
 * durable while a batch's deletions to older files were still nowhere but
 * memory. And the directory, because a file that exists only in an unflushed
 * directory entry is a file that does not exist.
 *
 * Each mapping is flushed before the file that backs it: the page barrier
 * covers the dirty pages, the file barrier covers the inode, and neither covers
 * the other.
 *
 * Derived metadata is deliberately **not** part of the promise. Losing a
 * metadata record costs a rescan and nothing else — an absent or damaged one
 * degrades to "unknown", which every consumer already handles — so paying a
 * barrier per record would buy nothing a caller could use. What sync() promises
 * is that the entries are there.
 */
result<> database_impl::sync() {
    if (auto const ready = refuse_if_recovery_pending(); ! ready) {
        return std::unexpected(ready.error());
    }

    if constexpr (platform_sync_support() == sync_support::none) {
        // Emscripten. There is no stable storage to reach, and returning
        // success would be a promise nothing here can keep.
        return std::unexpected(error_code::sync_unsupported);
    }

    // Absorbed where a platform simply has no such barrier; propagated when one
    // exists and failed. A caller that needs to know what this platform can
    // promise asks platform_sync_support() rather than inferring it from a
    // success here.
    auto barrier = [](result<> outcome) -> result<> {
        if ( ! outcome && outcome.error() == error_code::sync_unsupported) return {};
        return outcome;
    };

    if (mode_ == storage_mode::reference) {
        if (reference_segment_) {
            if (auto const r = barrier(sync_mapped_region(reference_segment_->get_address(),
                                                          reference_segment_->get_size()));
                ! r) {
                return r;
            }
            if (auto const r = barrier(sync_file(data_path(reference_sentinel_index,
                                                           reference_current_version_)));
                ! r) {
                return r;
            }
        }
    } else {
        result<> outcome;
        for_each_index<container_count>([&](auto I) {
            if ( ! outcome.has_value()) return;
            if ( ! segments_[I]) return;

            outcome = barrier(sync_mapped_region(segments_[I]->get_address(),
                                                 segments_[I]->get_size()));
            if ( ! outcome.has_value()) return;

            outcome = barrier(sync_file(data_path(I, current_versions_[I])));
        });
        if ( ! outcome.has_value()) return outcome;
    }

    // The pages of whatever mappings are still resident. Only the resident ones
    // can be flushed this way — the rest were unmapped, and unmapping is not a
    // barrier, which is why the file barriers below are driven by the dirty
    // register and not by what the cache happens to be holding.
    if (file_cache_) {
        if (auto const r = barrier(file_cache_->sync_mappings()); ! r) return r;
    }

    // Every version this instance wrote to and has not yet made durable,
    // whether or not its mapping survived. A sweep that deletes from three
    // generations evicts the first two before it ends; walking the cache would
    // flush the third and call the database durable.
    for (auto const& [container_index, version] : dirty_versions_) {
        if (auto const r = barrier(sync_file(data_path(container_index, version))); ! r) {
            // Nothing is discharged. An obligation half met is an obligation,
            // and the next sync has to attempt all of them again.
            return r;
        }
    }

    // A rotation creates a file, and a file nothing has flushed the directory
    // for is a file that may not be there after a power cut.
    if (auto const r = barrier(sync_directory(db_path_)); ! r) return r;

    // Only now, with every barrier this call owed having returned.
    dirty_versions_.clear();
    return {};
}

result<> database_impl::compact_all() {
    log::info("Starting full database compaction...");

    // Compaction moves entries between files and renames/removes versions, so
    // every cached (container_index, version) mapping becomes stale.
    if (file_cache_) file_cache_->clear();

    result<> outcome;

    if (mode_ == storage_mode::reference) {
        outcome = compact_reference_container();
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

    if (mode_ == storage_mode::reference) {
        stats.containers[0] = container_stats_[0];
        stats.total_inserts = container_stats_[0].total_inserts;
        stats.total_deletes = container_stats_[0].total_deletes;
        stats.rotations_per_container[0] = reference_current_version_;
        stats.memory_usage_per_container[0] = reference_active_file_size_;
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
    log::info("Storage mode: {}", stats.mode == storage_mode::reference ? "reference" : "full");
    log::info("Total entries: {}", stats.total_entries);
    log::info("Total inserts: {}", stats.total_inserts);
    log::info("Total deletes: {}", stats.total_deletes);

    log::info("--- Container Statistics ---");
    if (stats.mode == storage_mode::reference) {
        log::info("Reference container ({} bytes per entry):", sizeof(reference_value));
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

    if (mode_ == storage_mode::reference) {
        auto& info = report.containers[0];
        info.container_size = sizeof(reference_value);
        info.file_size_setting = reference_active_file_size_;
        info.file_count = reference_catalog_.size();
        info.current_entries = container_stats_[0].current_size;
        info.historical_inserts = container_stats_[0].total_inserts;
        info.historical_deletes = container_stats_[0].total_deletes;
        info.total_wasted_bytes = 0;

        for (auto const& [value_size, count] : container_stats_[0].value_size_distribution) {
            if (sizeof(reference_value) > value_size) {
                info.total_wasted_bytes += (sizeof(reference_value) - value_size) * count;
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
// database_impl - Reference mode implementation
// =============================================================================

reference_map_t& database_impl::reference_map() {
    return *static_cast<reference_map_t*>(reference_container_);
}

reference_map_t const& database_impl::reference_map() const {
    return *static_cast<reference_map_t const*>(reference_container_);
}

size_t database_impl::find_optimal_buckets_reference(std::string const& file_path,
                                                    size_t file_size,
                                                    size_t initial_buckets) {
    log::debug("Finding optimal buckets for reference container (file size: {})...", file_size);

    size_t left = 1;
    size_t right = initial_buckets;
    size_t best_buckets = left;

    while (left <= right) {
        size_t mid = left + (right - left) / 2;

        std::string temp_file = fmt::format("{}/temp_reference_{}_{}.dat", file_path, file_size, mid);
        try {
            bip::managed_mapped_file segment(bip::open_or_create, temp_file.c_str(), file_size);

            (void)segment.find_or_construct<reference_map_t>("temp_map")(
                mid,
                outpoint_hash{},
                outpoint_equal{},
                segment.get_allocator<std::pair<raw_outpoint const, reference_value>>()
            );

            best_buckets = mid;
            left = mid + 1;
            log::trace("  {} buckets OK, trying more...", mid);
        } catch (boost::interprocess::bad_alloc const&) {
            right = mid - 1;
        }

        fs::remove(temp_file);
    }

    log::debug("Optimal buckets for reference container: {}", best_buckets);
    return best_buckets;
}

void database_impl::reference_open_or_create(size_t version) {
    auto file_name = fmt::format(reference_data_file_format, db_path_.string(), version);

    reference_segment_ = std::make_unique<bip::managed_mapped_file>(
        bip::open_or_create, file_name.c_str(), reference_active_file_size_);

    reference_container_ = reference_segment_->find_or_construct<reference_map_t>(map_object_name)(
        reference_min_buckets_ok_,
        outpoint_hash{},
        outpoint_equal{},
        reference_segment_->get_allocator<typename reference_map_t::value_type>()
    );

    reference_current_version_ = version;
}

void database_impl::reference_close_container() {
    if (reference_segment_) {
        reference_save_metadata(reference_current_version_);
        reference_segment_->flush();
        reference_segment_.reset();
        reference_container_ = nullptr;
    }
}

void database_impl::reference_new_version() {
    // See new_version(): the retiring version stops being covered as an active
    // container the moment it stops being one, and closing it only schedules
    // writeback.
    note_dirty(reference_sentinel_index, reference_current_version_);

    reference_close_container();

    // The file first, the catalogue after: see new_version().
    auto const next = reference_catalog_.next_version();
    reference_open_or_create(next);   // sets reference_current_version_ once it maps

    reference_catalog_.add(next);
    reference_catalog_.metadata(next) = file_metadata{};
    log::debug("Reference container rotated to version {}", reference_current_version_);
}

bool database_impl::reference_can_insert_safely() const {
    auto const& map = reference_map();

    if (map.bucket_count() > 0) {
        float next_load = float(map.size() + 1) / float(map.bucket_count());
        if (next_load >= map.max_load_factor() * 0.95f) {
            return false;
        }
    }

    if (reference_segment_) {
        try {
            size_t free_memory = reference_segment_->get_free_memory();
            size_t entry_size = sizeof(typename reference_map_t::value_type);
            size_t buffer_size = entry_size * 10;
            return free_memory > buffer_size;
        } catch (...) {
            return false;
        }
    }

    return true;
}

result<bool> database_impl::reference_insert(raw_outpoint const& key, output_data_span value, uint32_t height) {
    if (value.size() != sizeof(uint32_t) * 2) {
        return std::unexpected(error_code::value_too_large);
    }

    uint32_t file_number;
    uint32_t offset;
    std::memcpy(&file_number, value.data(), sizeof(uint32_t));
    std::memcpy(&offset, value.data() + sizeof(uint32_t), sizeof(uint32_t));

    return reference_insert_typed(key, height, file_number, offset);
}

std::optional<find_result> database_impl::reference_find(raw_outpoint const& key, uint32_t height) const {
    if (auto res = reference_find_in_latest(key, height); res) {
        return res;
    }

    // A probe the active map could not answer. Recording it is what makes the
    // hit rate mean something: without it every recorded probe was a hit.
    probe_stats_.record_deferred();
    add_to_deferred_lookups(key, height);
    return std::nullopt;
}

std::optional<find_result> database_impl::reference_find_in_latest(raw_outpoint const& key, uint32_t height) const {
    auto const& map = reference_map();
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

size_t database_impl::reference_erase(raw_outpoint const& key, uint32_t height) {
    // Try current version first
    if (auto res = reference_erase_in_latest(key, height); res > 0) {
        entries_count_ -= res;
        return res;
    }

    // Try cached files
    size_t search_depth = 1;
    auto cached_files = file_cache_->get_cached_files();
    for (auto const& [ci, version] : cached_files) {
        if (ci != reference_sentinel_index) continue;
        ++search_depth;

        if (file_cache_->is_cached(reference_sentinel_index, version)) {
            try {
                auto [map, cache_hit] = file_cache_->get_or_open_reference_file(version);

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

                    note_dirty(reference_sentinel_index, version);   // see the full-mode path
                    if (auto* meta = reference_catalog_.find_metadata(version)) {
                        meta->update_on_delete();
                    }

                    --entries_count_;
                    return 1;
                }
            } catch (std::exception const& e) {
                log::error("Error accessing cached reference file v{}: {}", version, e.what());
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

size_t database_impl::reference_erase_in_latest(raw_outpoint const& key, uint32_t height) {
    auto& map = reference_map();
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

result<> database_impl::reference_for_each_key(void(*cb)(void*, raw_outpoint const&), void* ctx) const {
    // Current version
    auto const& map = reference_map();
    for (auto const& [key, _] : map) {
        cb(ctx, key);
    }

    // Previous versions
    for (auto const v : reference_catalog_.below(reference_current_version_)) {
        auto file_name = fmt::format(reference_data_file_format, db_path_.string(), v);

        try {
            auto segment = open_existing_segment(file_name);
            auto* map_ptr = segment->find<reference_map_t>(map_object_name).first;
            if (!map_ptr) continue;

            for (auto const& [key, _] : *map_ptr) {
                cb(ctx, key);
            }
        } catch (std::exception const& e) {
            log::error("reference_for_each_key: error reading reference v{}: {}", v, e.what());
            return std::unexpected(error_code::file_open_failed);
        }
    }

    return {};
}

result<> database_impl::reference_for_each_entry(void(*cb)(void*, raw_outpoint const&, uint32_t, std::span<uint8_t const>), void* ctx) const {
    auto emit = [&](raw_outpoint const& key, reference_value const& val) {
        std::array<uint8_t, sizeof(uint32_t) * 2> buf;
        std::memcpy(buf.data(), &val.file_number, sizeof(uint32_t));
        std::memcpy(buf.data() + sizeof(uint32_t), &val.offset, sizeof(uint32_t));
        cb(ctx, key, val.height, {buf.data(), buf.size()});
    };

    // Current version
    auto const& map = reference_map();
    for (auto const& [key, val] : map) {
        emit(key, val);
    }

    // Previous versions
    for (auto const v : reference_catalog_.below(reference_current_version_)) {
        auto file_name = fmt::format(reference_data_file_format, db_path_.string(), v);

        try {
            auto segment = open_existing_segment(file_name);
            auto* map_ptr = segment->find<reference_map_t>(map_object_name).first;
            if (!map_ptr) continue;

            for (auto const& [key, val] : *map_ptr) {
                emit(key, val);
            }
        } catch (std::exception const& e) {
            log::error("reference_for_each_entry: error reading reference v{}: {}", v, e.what());
            return std::unexpected(error_code::file_open_failed);
        }
    }

    return {};
}

result<> database_impl::reopen_active_reference_container() {
    auto const active = reference_catalog_.active();
    try {
        if (failpoints::fail_container_open.load(std::memory_order_relaxed)) {
            throw std::runtime_error("failpoint");
        }
        reference_open_or_create(active);
        reference_catalog_.add(active);
        return {};
    } catch (std::exception const& e) {
        log::error("compaction: the reference container could not be reopened at v{}: {}",
                   active, e.what());
        cleanup_pending_ = true;
        return std::unexpected(error_code::file_open_failed);
    }
}

result<> database_impl::compact_reference_container() {
    log::debug("Starting compaction for reference container...");

    reference_close_container();

    // See compact_container(): the reopen reports rather than happening in a
    // destructor that cannot.
    auto outcome = [&]() -> result<> {
        try {
            return merge_groups(reference_merge_policy{*this});
        } catch (std::exception const& e) {
            log::error("compaction: the reference container failed: {}", e.what());
            return std::unexpected(error_code::file_open_failed);
        }
    }();

    auto const reopened = reopen_active_reference_container();
    if ( ! outcome) return outcome;
    return reopened;
}


// =============================================================================
// database_impl - Reference metadata helpers
// =============================================================================

void database_impl::reference_save_metadata(size_t version) noexcept {
    auto const* meta_ptr = reference_catalog_.find_metadata(version);
    if ( ! meta_ptr) return;

    // Same boundary as save_metadata_to_disk().
    try {
        auto const path = fmt::format("{}/meta_compact_v{:05}.dat", db_path_.string(), version);
        if (auto const written = write_metadata_file(path, *meta_ptr); ! written) {
            log::warn("Could not publish reference metadata for v{}", version);
        }
    } catch (...) {
    }
}

void database_impl::reference_load_metadata(size_t version) {
    auto const path = fmt::format("{}/meta_compact_v{:05}.dat", db_path_.string(), version);

    auto record = read_metadata_file(path);
    if ( ! record) {
        report_metadata_read_error(record.error(), "the reference container", version);
        return;
    }

    auto& meta = reference_catalog_.metadata(version);
    meta = *record;
    meta.container_index = reference_sentinel_index;
    meta.version = version;
}

// =============================================================================
// database_impl - Config persistence
// =============================================================================

result<> database_impl::save_config_to_disk() {
    auto const config_path = db_path_ / "utxoz_config.dat";
    auto const temp_path = fs::path(config_path).concat(".tmp");

    // Published, not written in place. This file says whether a database is
    // full or reference, and a reader that finds half of it reads the wrong
    // answer about the whole store — the same reasoning as the metadata
    // records, and a stronger case, because this one is authoritative.
    {
        std::ofstream ofs(temp_path, std::ios::binary | std::ios::trunc);
        if ( ! ofs) {
            log::error("Failed to write config: {}", temp_path.string());
            return std::unexpected(error_code::config_file_corrupt);
        }

        char const magic[4] = {'U', 'T', 'X', 'O'};
        ofs.write(magic, 4);

        uint32_t const version = 1;
        ofs.write(reinterpret_cast<char const*>(&version), sizeof(version));

        uint8_t const mode_byte = static_cast<uint8_t>(mode_);
        ofs.write(reinterpret_cast<char const*>(&mode_byte), sizeof(mode_byte));

        ofs.close();
        if (ofs.fail()) {
            std::error_code cleanup;
            fs::remove(temp_path, cleanup);
            log::error("Failed to write config: {}", temp_path.string());
            return std::unexpected(error_code::config_file_corrupt);
        }
    }

    auto discard_temp = [&] {
        std::error_code cleanup;
        fs::remove(temp_path, cleanup);
    };

    // Contents before the name that publishes them. Reported rather than
    // warned about: sync() leaves this file out of its promise on the grounds
    // that it was made durable here, so if that did not happen, open is the
    // only place anyone finds out.
    if (auto const synced = sync_file(temp_path);
        ! synced && synced.error() != error_code::sync_unsupported) {
        discard_temp();
        log::error("Could not make the config file durable: {}", temp_path.string());
        return std::unexpected(synced.error());
    }

    if (auto const replaced = replace_file_atomically(temp_path, config_path); ! replaced) {
        discard_temp();
        log::error("Could not publish the config file: {}", config_path.string());
        return std::unexpected(replaced.error());
    }

    if (auto const synced = sync_directory(db_path_);
        ! synced && synced.error() != error_code::sync_unsupported) {
        // Published. The name may not survive a crash, which is a weaker
        // failure than a torn file but still one the caller has to hear.
        log::error("Could not make the config file's directory entry durable");
        return std::unexpected(synced.error());
    }

    return {};
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
        mode_byte != static_cast<uint8_t>(storage_mode::reference)) {
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

result<std::pair<flat_map<raw_outpoint, full_find_result>, std::vector<deferred_lookup_entry>>>
database_impl::full_process_pending_lookups() {
    if (deferred_lookups_.empty()) return {};

    flat_map<raw_outpoint, full_find_result> successful_lookups;

#ifdef UTXOZ_STATISTICS_ENABLED
    auto const start_time = std::chrono::steady_clock::now();
    ++deferred_stats_.processing_runs;
#endif

    size_t initial_size = deferred_lookups_.size();
    log::debug("Processing {} deferred full lookups...", initial_size);

    // A sweep either covers everything it needed to, or it says so.
    //
    // Every version below the current one can hold a pending key, so one that
    // cannot be read makes absence unprovable for *every* key still unresolved
    // — not only for the ones that happened to live in it. The old code logged
    // the failure and carried on, and those keys came back in the second list
    // alongside genuinely missing ones. A caller reading that list as "these
    // outpoints do not exist" turns a local storage fault into a rejected
    // block.
    //
    // The consumed entries are kept so the failure path can put them back. A
    // call that fails must consume nothing: the caller retries, and a retry
    // that had silently eaten the resolved keys would report them as neither
    // resolved nor pending.
    bool sweep_complete = true;
    // Which failure it was. A file that will not open and a catalogue that
    // cannot be listed are both fail-closed, and they send an operator to
    // different places, so the cause is carried rather than flattened.
    error_code sweep_error = error_code::version_unreadable;
    std::vector<deferred_lookup_entry> consumed;

    auto process_full_file = [&]<size_t Index>(std::integral_constant<size_t, Index>, size_t version) {
        try {
            if (failpoints::fail_lookup_open_version.load(std::memory_order_relaxed)
                    == static_cast<uint64_t>(version)) {
                throw std::runtime_error("failpoint: version file refused to open");
            }
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
                    consumed.push_back(entry);
                    return true;
                }
                return false;
            });
        } catch (std::exception const& e) {
            // Not recoverable by carrying on: this file might hold any of the
            // keys still pending, so nothing that remains can be called absent.
            log::error("Could not read full container {} v{}: {}. The sweep is incomplete.",
                       Index, version, e.what());
            sweep_complete = false;
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
            if (container_index == reference_sentinel_index) continue;
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

            // Enumerating the versions can fail too, and not knowing which files
            // exist is the same problem as not being able to read one.
            try {
            for (auto const v : catalogs_[I.value].below(current_versions_[I.value])) {
                if (deferred_lookups_.empty()) break;
                if (processed_versions[I.value].contains(v)) continue;

                process_full_file(I, v);
            }
            } catch (std::exception const& e) {
                // Not knowing which files exist is its own failure, and there is
                // already a code that says exactly that. Reporting it as
                // version_unreadable would send somebody looking at a file when
                // the problem is the catalogue.
                log::error("Could not enumerate versions of container {}: {}. The sweep is incomplete.",
                           I.value, e.what());
                sweep_complete = false;
                sweep_error = error_code::catalog_unreadable;
            }
        });
    }

    if ( ! sweep_complete) {
        // Put back everything this call took, so it consumed nothing and the
        // caller can retry once the storage fault is dealt with.
        for (auto const& entry : consumed) {
            deferred_lookups_.emplace(entry.key, entry.height);
        }
        log::error("Deferred full lookups incomplete: {} keys remain unresolved and none of them "
                   "can be reported as absent", deferred_lookups_.size());
        return std::unexpected(sweep_error);
    }

    // Every version was read, so what is left was looked for everywhere it
    // could have been. Only now is absence a fact.
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

    return std::pair{std::move(successful_lookups), std::move(failed_lookups)};
}

// =============================================================================
// database_impl - Typed reference-mode methods
// =============================================================================

result<bool> database_impl::reference_insert_typed(raw_outpoint const& key, uint32_t height,
                                                 uint32_t file_number, uint32_t offset) {
    if (!reference_can_insert_safely()) {
        log::debug("Rotating reference container due to safety constraints");
        reference_new_version();
    }

    reference_value val;
    val.height = height;
    val.file_number = file_number;
    val.offset = offset;

    size_t max_retries = 3;
    while (max_retries > 0) {
        try {
            auto& map = reference_map();
            [[maybe_unused]] size_t bucket_count_before = map.bucket_count();

            auto [it, inserted] = map.emplace(key, val);
            if (!inserted) {
                log::warn("reference_insert_typed: duplicate key at height {}, outpoint={}",
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

                reference_catalog_.metadata(reference_current_version_).update_on_insert(key, height);
            }
            return inserted;

        } catch (boost::interprocess::bad_alloc const& e) {
            log::error("Error inserting into reference container: {}", e.what());
            reference_new_version();
        }
        --max_retries;
    }

    log::error("Failed to insert into reference container after 3 retries");
    throw boost::interprocess::bad_alloc();
}

std::optional<reference_find_result> database_impl::reference_find_typed(raw_outpoint const& key, uint32_t height) const {
    auto const& map = reference_map();
    if (auto it = map.find(key); it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
        probe_stats_.record_answered(height, it->second.height);
#endif
        return reference_find_result{it->second.height, it->second.file_number, it->second.offset};
    }

    // A probe the active map could not answer. Recording it is what makes the
    // hit rate mean something: without it every recorded probe was a hit.
    probe_stats_.record_deferred();
    add_to_deferred_lookups(key, height);
    return std::nullopt;
}

result<std::pair<flat_map<raw_outpoint, reference_find_result>, std::vector<deferred_lookup_entry>>>
database_impl::reference_process_pending_lookups() {
    if (deferred_lookups_.empty()) return {};

    flat_map<raw_outpoint, reference_find_result> successful_lookups;

#ifdef UTXOZ_STATISTICS_ENABLED
    auto const start_time = std::chrono::steady_clock::now();
    ++deferred_stats_.processing_runs;
#endif

    size_t initial_size = deferred_lookups_.size();
    log::debug("Processing {} deferred reference lookups...", initial_size);

    // A sweep either covers everything it needed to, or it says so.
    //
    // Every version below the current one can hold a pending key, so one that
    // cannot be read makes absence unprovable for *every* key still unresolved
    // — not only for the ones that happened to live in it. The old code logged
    // the failure and carried on, and those keys came back in the second list
    // alongside genuinely missing ones. A caller reading that list as "these
    // outpoints do not exist" turns a local storage fault into a rejected
    // block.
    //
    // The consumed entries are kept so the failure path can put them back. A
    // call that fails must consume nothing: the caller retries, and a retry
    // that had silently eaten the resolved keys would report them as neither
    // resolved nor pending.
    bool sweep_complete = true;
    // Which failure it was. A file that will not open and a catalogue that
    // cannot be listed are both fail-closed, and they send an operator to
    // different places, so the cause is carried rather than flattened.
    error_code sweep_error = error_code::version_unreadable;
    std::vector<deferred_lookup_entry> consumed;

    auto process_compact_file = [&](size_t version, bool /*is_cached*/) {
        try {
            if (failpoints::fail_lookup_open_version.load(std::memory_order_relaxed)
                    == static_cast<uint64_t>(version)) {
                throw std::runtime_error("failpoint: version file refused to open");
            }
            auto [map, cache_hit] = file_cache_->get_or_open_reference_file(version);

            resolution_stats_.record_file_visited(cache_hit);


            deferred_lookups_.erase_if([&](auto const& entry) {
                auto map_it = map.find(entry.key);
                if (map_it != map.end()) {
#ifdef UTXOZ_STATISTICS_ENABLED
                    auto depth = static_cast<uint32_t>(reference_current_version_ - version);
                    ++deferred_stats_.lookups_by_depth[depth];
                    resolution_stats_.record_resolved(depth);
#endif
                    successful_lookups.emplace(entry.key,
                        reference_find_result{map_it->second.height, map_it->second.file_number, map_it->second.offset});
                    consumed.push_back(entry);
                    return true;
                }
                return false;
            });
        } catch (std::exception const& e) {
            log::error("Could not read reference v{}: {}. The sweep is incomplete.", version, e.what());
            sweep_complete = false;
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
            if (ci == reference_sentinel_index) {
                process_compact_file(version, true);
            }
        }
    }

    // Phase 2: remaining files
    if (!deferred_lookups_.empty()) {
        std::set<size_t> processed_versions;
        for (auto const& [ci, version] : cached_files) {
            if (ci == reference_sentinel_index) {
                processed_versions.insert(version);
            }
        }

        try {
            for (auto const v : reference_catalog_.below(reference_current_version_)) {
                if (deferred_lookups_.empty()) break;
                if (processed_versions.contains(v)) continue;

                process_compact_file(v, false);
            }
        } catch (std::exception const& e) {
            log::error("Could not enumerate reference versions: {}. The sweep is incomplete.", e.what());
            sweep_complete = false;
            sweep_error = error_code::catalog_unreadable;
        }
    }

    if ( ! sweep_complete) {
        for (auto const& entry : consumed) {
            deferred_lookups_.emplace(entry.key, entry.height);
        }
        log::error("Deferred reference lookups incomplete: {} keys remain unresolved and none of "
                   "them can be reported as absent", deferred_lookups_.size());
        return std::unexpected(sweep_error);
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

    log::debug("Deferred reference lookup complete: {} successful, {} failed",
              successful_lookups.size(), failed_lookups.size());

    return std::pair{std::move(successful_lookups), std::move(failed_lookups)};
}

result<> database_impl::reference_for_each_entry_typed(
    void(*cb)(void*, raw_outpoint const&, uint32_t, uint32_t, uint32_t), void* ctx) const {

    // Current version
    auto const& map = reference_map();
    for (auto const& [key, val] : map) {
        cb(ctx, key, val.height, val.file_number, val.offset);
    }

    // Previous versions
    for (auto const v : reference_catalog_.below(reference_current_version_)) {
        auto file_name = fmt::format(reference_data_file_format, db_path_.string(), v);

        try {
            auto segment = open_existing_segment(file_name);
            auto* map_ptr = segment->find<reference_map_t>(map_object_name).first;
            if (!map_ptr) continue;

            for (auto const& [key, val] : *map_ptr) {
                cb(ctx, key, val.height, val.file_number, val.offset);
            }
        } catch (std::exception const& e) {
            log::error("reference_for_each_entry_typed: error reading reference v{}: {}", v, e.what());
            return std::unexpected(error_code::file_open_failed);
        }
    }

    return {};
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
