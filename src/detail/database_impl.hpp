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
#include <utxoz/census.hpp>

#include <optional>

#include "detail/distinct_keys.hpp"
#include "detail/insert_transition.hpp"
#include <utxoz/statistics.hpp>
#include <utxoz/types.hpp>

#include "database_lock.hpp"
#include "file_cache.hpp"
#include "file_metadata.hpp"
#include "file_metadata_io.hpp"
#include "merge_policy.hpp"
#include "merge_sidecar.hpp"
#include "scope_exit.hpp"
#include "format_identity.hpp"
#include "segment_open.hpp"
#include "segment_stamp.hpp"
#include "store_config_io.hpp"
#include "capacity_policy.hpp"
#include "version_catalog.hpp"
#include "utxo_value.hpp"

namespace utxoz::detail {

namespace fs = std::filesystem;

/// The result a closed or latched instance returns for a batch of deletions:
/// nothing applied, nothing proven absent, everything still owed.
///
/// Deduplicated exactly the way a completed call deduplicates, keeping the first
/// occurrence of each key. Without that, a refusal is the one path where the
/// contract on deletion_progress — every distinct key in exactly one list, once
/// — does not hold, and a caller that trusts it would count one repeated
/// outpoint as several still owed.
[[nodiscard]]
deletion_progress refuse_deletions(std::span<deferred_deletion_entry const> requests,
                                   error_code why);

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
    /// Which of the two questions an open is asking. A typed pair rather than a
    /// second bool beside `remove_existing`, where `open(path, false, false)`
    /// would say nothing about which false is which.
    enum class open_intent {
        open_or_create,   ///< the historical behaviour: make one if there is none
        inspection,       ///< read what is there and create nothing at all
    };

    result<> configure(fs::path path, bool remove_existing, storage_mode mode = storage_mode::full);
    result<> open_for_inspection(fs::path path, storage_mode mode = storage_mode::full);
    result<> open_for_inspection_for_testing(fs::path path, storage_mode mode = storage_mode::full);
    result<> configure_for_testing(fs::path path, bool remove_existing, storage_mode mode = storage_mode::full);
    void close();
    size_t size() const;

    result<bool> insert(raw_outpoint const& key, output_data_span value, uint32_t height);
    std::optional<find_result> find(raw_outpoint const& key, uint32_t height) const;

    deletion_progress apply_deletes(std::span<deferred_deletion_entry const> requests);

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

    /// Visits every stored entry of every generation, in a fixed order. Defined
    /// in src/uniqueness.cpp.
    template <typename EntryFn>
    [[nodiscard]] result<> visit_stored_entries(EntryFn&& fn, uint64_t& files_visited) const;

    /// The verdict, assembled from the walk below. Bounded by the budget in
    /// `verify_options`, refuses rather than exceeding it, and creates nothing on
    /// disk. Defined in src/uniqueness.cpp.
    [[nodiscard]] result<uniqueness_report> verify_unique_outpoints(
        verify_options const& options) const;

    /// The engine. Every distinct key, and every key stored more than once.
    [[nodiscard]] result<budget_report> walk_distinct_keys(
        budget_meter& meter,
        distinct_walk_options const& options,
        distinct_key_callback const& on_distinct,
        duplicate_group_callback const& on_group) const;

    /// Walks the files. Requires the exclusive claim and no concurrent mutation;
    /// see census.hpp. Defined in src/census.cpp.
    [[nodiscard]] result<census_report> census(census_options const& options) const;

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
    result<bool> insert_in_index(raw_outpoint const& key, output_data_span value, uint32_t height);

    /// Replaces the active generation and records why. Returns rather than
    /// throwing: a rotation that cannot make its file leaves the class with no
    /// active container, which every caller has to be able to hear about.
    template<size_t Index>
    [[nodiscard]] result<> rotate_for(rotation_cause cause);

    [[nodiscard]] result<> reference_rotate_for(rotation_cause cause);

    // Find helpers
    std::optional<find_result> find_in_latest_version(raw_outpoint const& key, uint32_t height) const;

    // The active-version phase of apply_deletes(); nothing else calls it.
    size_t erase_in_latest_version(raw_outpoint const& key, uint32_t height);

    // File management
    //
    // Opening a version that must be there and creating one that must not are
    // separate calls, and neither can turn into the other. A single
    // open-or-create meant that a file whose map could not be reached came back
    // as a new empty container, which is the reinterpretation this whole barrier
    // exists to stop.
    template<size_t Index>
    [[nodiscard]] result<> open_existing_container(size_t version);

    template<size_t Index>
    [[nodiscard]] result<> create_container(size_t version);

    template<size_t Index>
    void close_container();

    template<size_t Index>
    void new_version();


    // Safety checks
    /// `free_bytes`, when given, receives the segment's free space, which this
    /// already reads. The diagnostic an insert needs if it later throws is the
    /// same figure, so it is passed out rather than asked for again: the read is
    /// a subtraction over the segment header, but the header is a different
    /// mapped page from the map's buckets, and reading it twice per insert
    /// measurably cost more than reading it once.
    ///
    /// Left untouched when the answer is "no" — the size check and the rotation
    /// seam both refuse before reaching the segment — so the caller must seed it
    /// with `free_bytes_unavailable` and not with zero.
    template<size_t Index>
    bool can_insert_safely(uint64_t* free_bytes = nullptr) const;

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

    /**
     * @brief Set once, never cleared: this instance may not be used again.
     *
     * It carries the code it answers with, because two different facts reach it
     * and folding them into one would tell an operator the wrong thing:
     *
     *  - `entry_corrupt` — the active map moved under a `bad_alloc`, contradicting
     *    the guarantee `emplace` documents. Nothing on disk records it and no
     *    reopen repairs it; see refuse_if_integrity_latched();
     *  - `file_open_failed` — a rotation could not create the next generation.
     *    The data is intact, but `close_container()` has already released the
     *    previous one and set `containers_[Index]` to null, which
     *    `container<Index>()` dereferences. The class has no active map, so the
     *    instance cannot serve it and must stop rather than find that out by
     *    dereferencing nothing.
     *
     * In memory only. The code says so where it is read.
     */
    std::optional<error_code> integrity_latched_;

    /// Why generations were replaced. Present in every build; see rotation_causes.
    std::array<rotation_causes, container_count> rotation_causes_{};
    rotation_causes reference_rotation_causes_{};

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

    /**
     * @brief Refuses every operation once the active map contradicted the
     *        guarantee `emplace` makes.
     *
     * Separate from `refuse_if_recovery_pending()` and deliberately not folded
     * into it, because the two describe different situations and call for
     * different actions.
     *
     * A pending cleanup means a merge published its target and could not retire
     * everything it superseded. There is a sidecar on disk, and the next open
     * settles it: `recovery_required` is an honest instruction because a restart
     * really does recover.
     *
     * This is not that. If the map moved under a `bad_alloc` there is no sidecar,
     * no merge record and no target marker — nothing a reopen could read to know
     * anything happened, and nothing to rebuild the map from. **This latch does
     * not survive a restart**, and the next open would map the same file and use
     * it: `validate_stamp` certifies the format identity, not the coherence of
     * what the file holds. Calling that `recovery_required` would tell an
     * operator to do the one thing that erases the only evidence.
     *
     * So it answers `entry_corrupt`, which already means "these figures cannot
     * describe anything real", and the log says what to do instead: stop, and
     * verify or rebuild before opening it again. Making the quarantine survive a
     * restart is a persisted mark, with its own format and its own questions
     * about who may lift it; that is not this.
     */
    [[nodiscard]]
    result<> refuse_if_integrity_latched() const {
        if (integrity_latched_) return std::unexpected(*integrity_latched_);
        return {};
    }

    /// Both of the above, for the callers that want to say "usable at all" once.
    /// The causes stay distinct in what they return.
    [[nodiscard]]
    result<> refuse_if_unusable() const {
        if (auto const intact = refuse_if_integrity_latched(); ! intact) return intact;
        return refuse_if_recovery_pending();
    }

    /// An inspection open creates nothing, which includes the active container of
    /// a class that has no generations. Everything except census() and close()
    /// would then be working on a container that is not there, so everything
    /// except census() and close() is refused.
    ///
    /// The alternative was to create the missing container and call the result an
    /// inspection, which is what this whole door exists not to do: an instrument
    /// that writes a ten-megabyte file on its way to measuring is measuring
    /// something it made.
    [[nodiscard]] result<> refuse_if_inspection_only() const {
        if (inspection_only_) return std::unexpected(error_code::inspection_only);
        return {};
    }

private:

    // Utilities
    size_t capacity_for(size_t index) const;
    size_t capacity_for_reference() const;

    size_t get_index_from_size(size_t size) const;

    size_t estimate_memory_usage(size_t index) const;

    // Internal configuration
    result<> configure_internal(fs::path path, bool remove_existing, storage_mode mode,
                                open_intent intent);

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
    size_t reference_erase_in_latest(raw_outpoint const& key, uint32_t height);

    [[nodiscard]] result<> reference_open_existing(size_t version);
    [[nodiscard]] result<> reference_create(size_t version);
    void reference_close_container();
    void reference_new_version();
    bool reference_can_insert_safely(uint64_t* free_bytes = nullptr) const;
    result<> compact_reference_container();
    result<> reference_for_each_key(void(*cb)(void*, raw_outpoint const&), void* ctx) const;
    result<> reference_for_each_entry(void(*cb)(void*, raw_outpoint const&, uint32_t, std::span<uint8_t const>), void* ctx) const;

    reference_map_t& reference_map();
    reference_map_t const& reference_map() const;

    // Config persistence
    [[nodiscard]]
    result<> save_config_to_disk();

    // Reference metadata helpers
    void reference_save_metadata(size_t version) noexcept;
    void reference_load_metadata(size_t version);

    // Member variables
    fs::path db_path_;
    storage_mode mode_ = storage_mode::full;

    /// Set by open_for_inspection(). See refuse_if_inspection_only().
    bool inspection_only_ = false;

    // Full mode storage
    /// What a new segment gets: its size and the capacity its map is built with,
    /// together, because they are one decision. Set by which configure() was
    /// called and never derived from anything at run time.
    std::array<capacity_entry, container_count> capacity_ = production_capacity;
    std::array<std::unique_ptr<bip::managed_mapped_file>, container_count> segments_;
    std::array<void*, container_count> containers_{};

    /// The bucket count each open generation was created or opened with.
    ///
    /// The invariant this store rests on is that it never changes: a container
    /// that fills up gets a new generation, and compaction deals with the cost
    /// later. Comparing against this rather than against the previous insert
    /// catches a growth from any path, not only from the insert that saw it.
    std::array<rehash_watch, container_count> rehash_watch_{};
    std::array<size_t, container_count> current_versions_{};

    // Reference mode storage
    std::unique_ptr<bip::managed_mapped_file> reference_segment_;
    void* reference_container_ = nullptr;
    size_t reference_current_version_ = 0;
    capacity_entry reference_capacity_ = production_reference;
    rehash_watch reference_rehash_watch_{};
    size_t reference_active_file_size_ = 0;
    version_catalog reference_catalog_;

    size_t entries_count_ = 0;

    /// This database's identity, made once when it is created and written into
    /// the config and into every segment it owns. Read back from the config on
    /// every later open; a segment that does not carry it belongs to a different
    /// database and is refused before anything reads it.
    database_id_t database_id_{};

    /// What this build expects a segment of `kind` at `version` to say about
    /// itself.
    [[nodiscard]] segment_identity expected_identity(uint32_t kind, uint64_t version) const {
        return local_identity(database_id_, kind, version);
    }

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
    // resolve-vs-resolve only. insert(), apply_deletes()
    // and compact_all() touch the same cache and remain the caller's to
    // serialise, exactly as db_base documents.
    //
    // mutable because resolve() is const: it does not change what is stored.
    mutable std::mutex resolve_mutex_;

    // Statistics (mutable to allow const find and resolve operations)
    mutable probe_stats probe_stats_;
    /// The read path, per class. In reference mode only slot zero is used, and
    /// it is reported under `reference_class` rather than as container 0.
    ///
    /// Compiled away below `UTXOZ_STATISTICS_LEVEL >= 2`, where each element is
    /// an empty struct. At `lookup` it is five instances of 16 KiB — 80 KiB per
    /// open database, on top of the 16 KiB the two narrower counter sets take,
    /// which is 96 KiB for these three sharded counter sets. A subtotal: the
    /// members below hold maps that grow with the data. Figures in
    /// doc/statistics-levels.md.
    mutable std::array<lookup_stats, container_count> lookup_stats_;
    mutable resolution_stats resolution_stats_;
    std::array<container_stats, container_count> container_stats_;
    height_range_stats height_range_stats_;
    deferred_stats deferred_stats_;
    not_found_stats not_found_stats_;
    utxo_lifetime_stats lifetime_stats_;
    fragmentation_stats fragmentation_stats_;
};

} // namespace utxoz::detail
