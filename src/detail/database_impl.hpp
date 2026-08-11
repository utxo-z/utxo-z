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
#include <mutex>
#include <set>
#include <utility>
#include <variant>

#include <boost/unordered/unordered_flat_set.hpp>

#include <utxoz/aliases.hpp>
#include <utxoz/database.hpp>
#include <utxoz/statistics.hpp>
#include <utxoz/types.hpp>

#include "database_lock.hpp"
#include "file_cache.hpp"
#include "file_metadata.hpp"
#include "file_metadata_io.hpp"
#include "merge_policy.hpp"
#include "merge_sidecar.hpp"
#include "scope_exit.hpp"
#include "segment_open.hpp"
#include "version_catalog.hpp"
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
    result<> configure(fs::path path, bool remove_existing, storage_mode mode = storage_mode::full);
    result<> configure_for_testing(fs::path path, bool remove_existing, storage_mode mode = storage_mode::full);
    void close();
    size_t size() const;

    result<bool> insert(raw_outpoint const& key, output_data_span value, uint32_t height);
    std::optional<find_result> find(raw_outpoint const& key, uint32_t height) const;
    size_t erase(raw_outpoint const& key, uint32_t height);

    std::pair<uint32_t, std::vector<deferred_deletion_entry>> process_pending_deletions();
    size_t deferred_deletions_size() const;

    result<> compact_all();

    /// Puts everything written so far on stable storage. See db_base::sync().
    result<> sync();
    result<> for_each_key_impl(void(*cb)(void*, raw_outpoint const&), void* ctx) const;
    result<> for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, std::span<uint8_t const>), void* ctx) const;

    // Typed full-mode methods (no runtime dispatch)
    std::optional<full_find_result> full_find(raw_outpoint const& key, uint32_t height) const;
    [[nodiscard]]
    result<full_resolution> full_resolve(std::span<lookup_request const> requests) const;

    // Typed reference-mode methods (no serialization)
    result<bool> reference_insert_typed(raw_outpoint const& key, uint32_t height, uint32_t file_number, uint32_t offset);
    std::optional<reference_find_result> reference_find_typed(raw_outpoint const& key, uint32_t height) const;
    [[nodiscard]]
    result<reference_resolution> reference_resolve(std::span<lookup_request const> requests) const;
    result<> reference_for_each_entry_typed(void(*cb)(void*, raw_outpoint const&, uint32_t, uint32_t, uint32_t), void* ctx) const;

    database_statistics get_statistics();
    void print_statistics();
    sizing_report get_sizing_report() const;
    void print_sizing_report() const;
    void print_height_range_stats() const;
    void reset_all_statistics();

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
    std::optional<find_result> find_in_latest_version(raw_outpoint const& key, uint32_t height) const;

    // Erase helpers
    size_t erase_in_latest_version(raw_outpoint const& key, uint32_t height);
    size_t erase_from_cached_files_only(raw_outpoint const& key, uint32_t height, size_t& search_depth);
    void add_to_deferred_deletions(raw_outpoint const& key, uint32_t height);
    size_t process_deferred_deletions_in_file(size_t container_index, size_t version, bool is_cached);

    // Deferred lookup helpers

    // File management
    template<size_t Index>
    void open_or_create_container(size_t version);

    template<size_t Index>
    void close_container();

    template<size_t Index>
    void new_version();


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
    result<> compact_container();

    // -- Crash-atomic compaction (see merge_sidecar.hpp) ----------------------

    /// Paths in the store's reserved namespace. Only `.dat` is canonical; the
    /// rest are invisible to discovery, which is what lets a merge be built
    /// without ever being half-visible.
    fs::path data_path(size_t index, size_t version) const;
    fs::path building_path(size_t index, size_t version) const;
    fs::path sidecar_path(size_t index, size_t version) const;
    fs::path metadata_path(size_t index, size_t version) const;

    /// Mandatory phase of open(): finishes or abandons whatever a previous
    /// process left in flight, before any container is opened and therefore
    /// before an intermediate state could be observed.
    /// The directory barrier, tagged with which of a merge's four crossings it
    /// is, so a test can reach any one of them.
    [[nodiscard]]
    result<> directory_barrier(failpoints::dir_barrier stage) const;

    /// Reads the marker a merge wrote into its target. Absent, duplicated or
    /// unreadable are all distinct from "does not match", and none of them let
    /// a source be retired.
    [[nodiscard]]
    result<merge_marker> read_target_marker(size_t index, size_t version) const;

    result<> recover_pending_merges();
    result<> recover_one(merge_plan const& plan, fs::path const& sidecar);

    /// Builds one new version file holding everything in `sources`, publishes
    /// it, and retires them. One implementation; the policy names the six
    /// things that differ between the storage modes.
    template<typename Policy>
    result<> merge_versions(Policy policy, std::vector<size_t> const& sources);

    /// The merging itself, bracketed by compact_container() so that reopening
    /// the active container is part of the typed result rather than something a
    /// destructor does and cannot report.
    template<typename Policy>
    result<> merge_groups(Policy policy);

    template<size_t Index>
    result<> reopen_active_container();
    result<> reopen_active_reference_container();

    /// True once a merge published its target and could not retire everything
    /// it superseded. Latches: the instance serves nothing further until it is
    /// closed and reopened, which runs recovery.
    bool cleanup_pending_ = false;

    /// The exclusive claim on the database directory, held for the life of this
    /// instance and released by its destructor. Nothing releases it by hand.
    database_lock lock_;

    template <size_t I> friend struct full_merge_policy;
    friend struct reference_merge_policy;

    /**
     * @brief Version files this instance has written to and not yet made durable.
     *
     * Kept apart from the file cache on purpose. The cache is an LRU that holds
     * one mapping by default, so a sweep that deletes from three generations
     * evicts the first two before it finishes — and unmapping is not a barrier.
     * A sync that walked the cache would flush whatever happened to still be
     * resident and report the database durable, with the other two generations'
     * deletions nowhere but in pages the kernel had not been asked to write.
     *
     * So the obligation is recorded by identity and outlives the mapping. It is
     * discharged by a successful sync(), or by compaction retiring the file —
     * a version that no longer exists cannot owe anything. A partial sync
     * clears nothing: an obligation half met is an obligation.
     */
    std::set<std::pair<size_t, size_t>> dirty_versions_;

    /// Records that a historical version file was written to.
    void note_dirty(size_t container_index, size_t version) {
        dirty_versions_.emplace(container_index, version);
    }

public:
    /// Refuses every operation once a merge has published its target and could
    /// not retire everything it superseded. Until this instance is closed and
    /// reopened, several canonical files hold the same keys, and serving from
    /// that state would hand out duplicates.
    [[nodiscard]]
    result<> refuse_if_recovery_pending() const {
        if (cleanup_pending_) return std::unexpected(error_code::recovery_required);
        return {};
    }

private:

    // Optimal buckets finder
    template<size_t Index>
    size_t find_optimal_buckets(fs::path const& file_path, size_t file_size, size_t initial_buckets);

    // Utilities
    size_t get_index_from_size(size_t size) const;

    size_t estimate_memory_usage(size_t index) const;

    // Internal configuration
    result<> configure_internal(fs::path path, bool remove_existing, storage_mode mode);

    // Metadata management
    void update_metadata_on_insert(size_t index, size_t version, raw_outpoint const& key, uint32_t height);
    void update_metadata_on_delete(size_t index, size_t version);
    void save_metadata_to_disk(size_t index, size_t version) noexcept;
    void load_metadata_from_disk(size_t index, size_t version);

    // Statistics
    void update_fragmentation_stats();

    // Reference mode operations
    result<bool> reference_insert(raw_outpoint const& key, output_data_span value, uint32_t height);
    std::optional<find_result> reference_find(raw_outpoint const& key, uint32_t height) const;
    std::optional<find_result> reference_find_in_latest(raw_outpoint const& key, uint32_t height) const;
    size_t reference_erase(raw_outpoint const& key, uint32_t height);
    size_t reference_erase_in_latest(raw_outpoint const& key, uint32_t height);

    void reference_open_or_create(size_t version);
    void reference_close_container();
    void reference_new_version();
    bool reference_can_insert_safely() const;
    result<> compact_reference_container();
    result<> reference_for_each_key(void(*cb)(void*, raw_outpoint const&), void* ctx) const;
    result<> reference_for_each_entry(void(*cb)(void*, raw_outpoint const&, uint32_t, std::span<uint8_t const>), void* ctx) const;

    size_t find_optimal_buckets_reference(fs::path const& file_path, size_t file_size, size_t initial_buckets);

    reference_map_t& reference_map();
    reference_map_t const& reference_map() const;

    // Config persistence
    [[nodiscard]]
    result<> save_config_to_disk();
    result<> load_config_from_disk();

    // Reference metadata helpers
    void reference_save_metadata(size_t version) noexcept;
    void reference_load_metadata(size_t version);

    // Member variables
    fs::path db_path_;
    storage_mode mode_ = storage_mode::full;

    // Full mode storage
    std::array<size_t, container_count> active_file_sizes_ = file_sizes; // Can be changed for testing
    std::array<std::unique_ptr<bip::managed_mapped_file>, container_count> segments_;
    std::array<void*, container_count> containers_{};
    std::array<size_t, container_count> current_versions_{};
    std::array<size_t, container_count> min_buckets_ok_{};

    // Reference mode storage
    std::unique_ptr<bip::managed_mapped_file> reference_segment_;
    void* reference_container_ = nullptr;
    size_t reference_current_version_ = 0;
    size_t reference_min_buckets_ok_ = 0;
    size_t reference_active_file_size_ = 0;
    version_catalog reference_catalog_;

    size_t entries_count_ = 0;

    // The versions each container has, and the metadata describing them.
    // Sparse: version numbers are identities, never positions.
    std::array<version_catalog, container_count> catalogs_;
    std::unique_ptr<file_cache> file_cache_;

    // Serialises resolutions against each other.
    //
    // Held for the whole of full_resolve() / reference_resolve(), not merely
    // around the cache's bookkeeping. The bookkeeping race is the smaller half
    // of the problem: file_cache hands out a *reference* into a
    // managed_mapped_file that it owns and destroys on eviction, so a second
    // resolution evicting a segment the first one is still reading unmaps that
    // memory underneath it. That is a SIGSEGV, not a torn read, and no amount of
    // locking the contents would prevent it — only keeping the two resolutions
    // from overlapping at all does. Measured before this existed: 90
    // ThreadSanitizer races in file_cache::get_or_open_file and exit 139 (#120).
    //
    // resolve-vs-resolve only. insert(), erase(), process_pending_deletions()
    // and compact_all() touch the same cache and remain the caller's to
    // serialise, exactly as db_base documents.
    //
    // mutable because resolve() is const: it does not change what is stored.
    mutable std::mutex resolve_mutex_;

    // Statistics (mutable to allow const find and resolve operations)
    mutable probe_stats probe_stats_;
    mutable resolution_stats resolution_stats_;
    boost::unordered_flat_set<deferred_deletion_entry> deferred_deletions_;
    std::array<container_stats, container_count> container_stats_;
    height_range_stats height_range_stats_;
    deferred_stats deferred_stats_;
    not_found_stats not_found_stats_;
    utxo_lifetime_stats lifetime_stats_;
    fragmentation_stats fragmentation_stats_;
};

} // namespace utxoz::detail
