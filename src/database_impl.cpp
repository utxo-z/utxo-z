// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file database_impl.cpp
 * @brief Database implementation - migrated from interprocess_multiple_v12.hpp
 */

#include "detail/capacity_policy.hpp"
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
#include "detail/path_display.hpp"
#include "detail/system_entropy.hpp"

namespace utxoz::detail {

namespace {

/// The working set for a batch: indices into the caller's requests, one per
/// distinct key.
///
/// Indices rather than copies. The requests belong to the caller and outlive the
/// call, so there is nothing here to own; copying them would buy a second copy of
/// data already sitting in memory, once per key, on the path every block takes.
///
/// Distinct keys, because a batch naming one outpoint twice is asking one
/// question. Without this the duplicate is probed again in every file and comes
/// back twice in the result, which reads as two missing inputs where there is
/// one. The first occurrence is the one kept, and the index type is size_t
/// because that is what it indexes — a narrower one would silently truncate a
/// batch larger than it can count.
///
/// Generic over the request type: a lookup and a deletion carry different
/// payloads but the same key, and both need the same deduplication. The closed
/// and recovery paths use it too, so that a refusal partitions the batch exactly
/// the way a completed call does.
template <typename Request>
std::vector<size_t> working_set_of(std::span<Request const> requests) {
    std::vector<size_t> pending;
    pending.reserve(requests.size());

    boost::unordered_flat_set<raw_outpoint> seen;
    seen.reserve(requests.size());

    for (size_t i = 0; i < requests.size(); ++i) {
        if (seen.insert(requests[i].key).second) pending.push_back(i);
    }
    return pending;
}

} // namespace

deletion_progress refuse_deletions(std::span<deferred_deletion_entry const> requests,
                                   error_code why) {
    deletion_progress refused;
    refused.error = why;
    if (requests.empty()) return refused;

    // The same working set the applying path builds, so a refusal partitions the
    // batch the same way a completed call does.
    auto const pending = working_set_of<deferred_deletion_entry>(requests);
    refused.unresolved.reserve(pending.size());
    for (auto const idx : pending) {
        refused.unresolved.push_back(requests[idx]);
    }
    return refused;
}


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

/// The capacity a new map is built with: the policy, unless a test has asked for
/// something else. One place, so the seam cannot apply to some maps and not
/// others and leave a compaction target disagreeing with its sources.
size_t database_impl::capacity_for(size_t index) const {
    if (auto const forced = failpoints::forced_capacity.load(std::memory_order_relaxed);
        forced != 0
        && index == failpoints::forced_capacity_index.load(std::memory_order_relaxed)) {
        return forced;
    }
    return capacity_[index].capacity;
}

size_t database_impl::capacity_for_reference() const {
    if (auto const forced = failpoints::forced_capacity.load(std::memory_order_relaxed);
        forced != 0
        && failpoints::forced_capacity_index.load(std::memory_order_relaxed)
               == reference_container_kind) {
        return forced;
    }
    return reference_capacity_.capacity;
}

size_t database_impl::get_index_from_size(size_t size) const {
    for (size_t i = 0; i < container_count; ++i) {
        if (size <= container_capacities[i]) return i;
    }
    return container_count;
}

// =============================================================================
// database_impl - File management
// =============================================================================

template<size_t Index>
result<> database_impl::open_existing_container(size_t version) {
    auto const file_name = db_path_ / fmt::format(data_file_format, Index, version);

    auto opened = open_existing_segment(file_name);
    if ( ! opened) return std::unexpected(opened.error());

    // The stamp before the map, always. find<utxo_map> on a file written under a
    // different layout does not fail, it reinterprets, and there is no later
    // point at which that becomes visible.
    if (auto const stamped = validate_stamp(**opened, file_name,
                                            expected_identity(uint32_t(Index), version));
        ! stamped) {
        return std::unexpected(stamped.error());
    }

    auto const found = find_single_named<utxo_map<container_sizes[Index]>>(
        **opened, map_object_name, file_name);
    if ( ! found) return std::unexpected(found.error());

    segments_[Index] = std::move(*opened);
    containers_[Index] = *found;
    rehash_watch_[Index].reset((*found)->bucket_count());
    current_versions_[Index] = version;
    return {};
}

template<size_t Index>
result<> database_impl::create_container(size_t version) {
    auto const file_name = db_path_ / fmt::format(data_file_format, Index, version);

    // create_only, so a name already taken is reported rather than adopted. A
    // version this call believes is new and is not means the catalogue and the
    // directory disagree, and building into whatever is there would be the
    // reinterpretation the stamp exists to prevent.
    std::unique_ptr<bip::managed_mapped_file> segment;
    try {
        segment = std::make_unique<bip::managed_mapped_file>(
            bip::create_only, file_name.c_str(), capacity_[Index].file_size);
    } catch (std::exception const& e) {
        log::error("container {} v{} could not be created: {}", Index, version, e.what());
        return std::unexpected(error_code::identity_collision);
    }

    // From here the file exists and this call is what made it — create_only
    // guarantees that, which is what makes removing it safe. Until the segment
    // is published, any failure or exception takes the file with it: a rotation
    // that failed part-way would otherwise leave the name occupied with nothing
    // usable behind it, and the retry computes the same version number and finds
    // it taken. The container would then have no active version and no way back.
    bool published = false;
    scope_exit const rollback([&] {
        if (published) return;
        segment.reset();   // unmapped before it is unlinked
        std::error_code ec;
        fs::remove(file_name, ec);
        if (ec) {
            log::error("could not withdraw the half-built {}", path_display(file_name));
        }
    });

    if (failpoints::fail_after_segment_create.load(std::memory_order_relaxed)) {
        return std::unexpected(error_code::file_open_failed);
    }

    if (auto const stamped = place_stamp(*segment, file_name,
                                         expected_identity(uint32_t(Index), version));
        ! stamped) {
        return std::unexpected(stamped.error());
    }

    if (failpoints::fail_after_segment_stamp.load(std::memory_order_relaxed)) {
        return std::unexpected(error_code::file_open_failed);
    }

    auto* map = segment->construct<utxo_map<container_sizes[Index]>>(map_object_name, std::nothrow)(
        capacity_for(Index),
        outpoint_hash{},
        outpoint_equal{},
        segment->get_allocator<typename utxo_map<container_sizes[Index]>::value_type>()
    );
    if (map == nullptr) {
        log::error("container {} v{} already holds a map", Index, version);
        return std::unexpected(error_code::identity_collision);
    }

    segments_[Index] = std::move(segment);
    containers_[Index] = map;
    rehash_watch_[Index].reset(map->bucket_count());
    current_versions_[Index] = version;
    published = true;
    return {};
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
    // A rotation always makes a file that did not exist, so this is the create
    // path and never the open one. Failure throws, as it already did when Boost
    // refused the mapping: insert() is the only caller and has no way to report
    // a rotation that could not happen.
    if (auto const created = create_container<Index>(next); ! created) {
        throw std::runtime_error(fmt::format("container {} could not rotate to v{}", Index, next));
    }

    catalogs_[Index].add(next);
    catalogs_[Index].metadata(next) = file_metadata{};
    log::debug("Container {} rotated to version {}", Index, current_versions_[Index]);
}

// =============================================================================
// database_impl - Safety checks
// =============================================================================

template<size_t Index>
bool database_impl::can_insert_safely(uint64_t* free_bytes) const {
    // The generator's seam: consumed one rotation at a time, so a fixture can be
    // given more than one generation without the hundred thousand entries a real
    // one needs. Answering here means the ordinary rotation path runs — nothing
    // downstream can tell this apart from a container that filled up.
    if (failpoints::consume_forced_rotation()) return false;

    auto const& map = container<Index>();

    // From the bucket count of the map that is open, never from the policy: a
    // generation written under an older one has its own, and it is the one that
    // decides when *that* file is full. The policy decides what a new segment
    // gets; the file decides how it is operated.
    //
    // And from the map's *live* growth point, not only from the policy's fixed
    // one. Boost lowers its own threshold on erase to keep probe lengths from
    // drifting, so on an insert/erase workload the point at which it would grow
    // walks down and can pass below the operating threshold — after which this
    // guard would be answering about a map that no longer exists. See
    // effective_insert_limit for the arithmetic and for where the off-by-one
    // comes from.
    //
    // Integers, and inclusive: the limit is the most entries this map may hold,
    // so a map already holding that many is full and this insert rotates.
    if (map.bucket_count() > 0
            && map.size() >= effective_insert_limit(map.bucket_count(), map.max_load())) {
        return false;
    }

    // Check available memory
    if (segments_[Index]) {
        try {
            size_t free_memory = segments_[Index]->get_free_memory();
            if (free_bytes) *free_bytes = free_memory;
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
    if (map.bucket_count() > 0 && map.size() >= max_entries_for(map.bucket_count())) {
        return false;
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
        auto const path = db_path_ / fmt::format("meta_{}_v{:05}.dat", index, version);
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
    auto const path = db_path_ / fmt::format("meta_{}_v{:05}.dat", index, version);

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

result<> database_impl::open_for_inspection(fs::path path, storage_mode mode) {
    capacity_ = production_capacity;
    reference_capacity_ = production_reference;
    return configure_internal(std::move(path), false, mode, open_intent::inspection);
}

result<> database_impl::open_for_inspection_for_testing(fs::path path, storage_mode mode) {
    capacity_ = testing_capacity;
    reference_capacity_ = testing_reference;
    return configure_internal(std::move(path), false, mode, open_intent::inspection);
}

result<> database_impl::configure(fs::path path, bool remove_existing, storage_mode mode) {
    // The profile is chosen here and nowhere else. Production fixes the capacity
    // and takes the file size that was measured for it; testing fixes the file
    // size at ten megabytes and takes the capacity that fits. They answer
    // different questions and share no formula.
    capacity_ = production_capacity;
    reference_capacity_ = production_reference;
    return configure_internal(std::move(path), remove_existing, mode, open_intent::open_or_create);
}

result<> database_impl::configure_for_testing(fs::path path, bool remove_existing, storage_mode mode) {
    capacity_ = testing_capacity;
    reference_capacity_ = testing_reference;
    return configure_internal(std::move(path), remove_existing, mode, open_intent::open_or_create);
}

result<> database_impl::configure_internal(fs::path path, bool remove_existing,
                                           storage_mode mode, open_intent intent) {
    db_path_ = std::move(path);
    inspection_only_ = (intent == open_intent::inspection);

    // Every filesystem question here is asked so that "I could not tell" comes
    // back as an error. Asked the throwing way, an unreadable directory raises
    // out of a result-typed open(); asked the swallowing way, it answers "no"
    // and the database gets recreated over data that is still there.
    std::error_code ec;
    if (intent == open_intent::inspection) {
        // Not created. A caller who asked to open a database that already exists
        // is not asking for a directory to be made for them, and making one is a
        // side effect on the way to an error.
        //
        // status() rather than is_directory(): absence is an answer to this
        // question and not a failure to answer it, and status() is the one that
        // reports it that way. is_directory() sets an error for a path that is
        // not there, which would come back as "the directory could not be read"
        // — a different problem, sending a person somewhere else.
        auto const what = fs::status(db_path_, ec);
        if (ec && what.type() != fs::file_type::not_found) {
            return std::unexpected(error_code::catalog_unreadable);
        }
        if ( ! fs::is_directory(what)) return std::unexpected(error_code::database_not_found);
        ec.clear();
    } else {
        fs::create_directories(db_path_, ec);
        if (ec) return std::unexpected(error_code::catalog_unreadable);
    }

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

    // The seam that pins the ordering below. See durability.hpp: it makes the
    // window between "is there a database here" and "open it" the whole of the
    // run, and the point is that there is no such window, because both happen
    // under the claim taken above.
    if (failpoints::delete_config_after_claim.load(std::memory_order_relaxed)) {
        std::error_code ignored;
        fs::remove(config_path, ignored);
    }

    auto const config_exists = path_exists(config_path);
    if ( ! config_exists) return std::unexpected(config_exists.error());

    // Asked here, under the claim, and not by the caller beforehand. A caller
    // outside the library can only ask before opening, and between their answer
    // and the open the file can go; then open-or-create would create a database
    // over the answer they were given. Inside the claim there is no such window.
    //
    // Nothing has been created at this point: the claim's own file exists, and a
    // lock file is not a database and is never read as one.
    if (intent == open_intent::inspection && ! *config_exists) {
        log::error("{}: no database here, and open_existing does not create one",
                   path_display(db_path_));
        return std::unexpected(error_code::database_not_found);
    }

    // Read and held against this build before a single segment is mapped. A
    // database written under a geometry, a map layout, a hash or a platform this
    // build does not share is refused here, with every file still untouched —
    // which is the difference between a refusal and a repair.
    if (*config_exists && !remove_existing) {
        auto config = read_config_file(config_path);
        if ( ! config) return std::unexpected(config.error());

        if (auto const usable = check_config_compatible(*config, config_path); ! usable) {
            return std::unexpected(usable.error());
        }
        if (config->mode != mode) {
            return std::unexpected(error_code::storage_mode_mismatch);
        }
        mode_ = config->mode;
        database_id_ = config->database_id;
    } else {
        // No config file — check for pre-existing data files from the other mode
        if (!remove_existing) {
            auto const other_mode_file = mode == storage_mode::reference
                ? db_path_ / fmt::format(data_file_format, 0, 0)
                : db_path_ / fmt::format(reference_data_file_format, 0);

            auto const other_exists = path_exists(other_mode_file);
            if ( ! other_exists) return std::unexpected(other_exists.error());
            if (*other_exists) {
                return std::unexpected(error_code::storage_mode_mismatch);
            }
        }
        mode_ = mode;

        // The identity is made once, here, and every segment this database ever
        // creates carries it. Sixteen bytes from the system generator, not a
        // timestamp or a pid: those collide, and two databases that share an
        // identity are exactly what the check on the way in cannot then catch.
        if (failpoints::force_database_id.load(std::memory_order_relaxed)) {
            // Only the fixture generator sets this, and only so that two writers
            // can be compared byte for byte.
            database_id_ = failpoints::forced_database_id;
        } else if (auto const seeded = system_entropy(database_id_.data(), database_id_.size());
                   ! seeded) {
            return std::unexpected(seeded.error());
        }

        // Written before anything is created, not after. The other order leaves
        // a crash between the two with segments on disk and no config, and the
        // next open would take that for a fresh database, mint a second identity
        // and then refuse its own files.
        if (auto const written = save_config_to_disk(); ! written) {
            return std::unexpected(written.error());
        }
    }

    // Initialize file cache
    //
    // db_path_, not the parameter: the parameter is moved from on the way in,
    // and fs::path converts implicitly to its native string type, so reading it
    // here still compiles and hands over an empty base path. Every historical
    // version file would then be looked for in the working directory.
    file_cache_ = std::make_unique<file_cache>(db_path_, database_id_);

    entries_count_ = 0;

    // Before any container is opened, so an intermediate state left by a
    // previous process is never observable. With nothing in flight this costs
    // one directory listing per container, which open() already does.
    if (auto const recovered = recover_pending_merges(); ! recovered) {
        return std::unexpected(recovered.error());
    }

    if (mode_ == storage_mode::reference) {
        // Reference mode: single container
        reference_active_file_size_ = reference_capacity_.file_size;

        // Build the catalogue before anything is opened. A directory we cannot
        // read is not an empty directory: opening on that assumption would
        // create v0 over a database that already has versions in it.
        auto listed = enumerate_versions(db_path_, "compact_v");
        if ( ! listed) return std::unexpected(listed.error());

        reference_catalog_.clear();
        for (auto const v : *listed) reference_catalog_.add(v);

        // A config with no generations behind it is not a database to open: it is
        // the shape a database has for the instant between its config being
        // written and its first segment existing, or what is left when the data
        // has been removed and the config has not. Opening it would take the
        // creation branch below and make segments, which is the one thing
        // open_existing() promises not to do.
        if (intent == open_intent::inspection && reference_catalog_.empty()) {
            log::error("{}: a config with no generations is not a database to open, "
                       "and open_existing does not create the missing ones",
                       path_display(db_path_));
            return std::unexpected(error_code::database_not_found);
        }

        // Whether this is a creation is the catalogue's answer, not the map's.
        // "db_map was not there" must never become "so I made one". An inspection
        // never reaches the creation branch: an empty catalogue was refused above.
        bool const fresh = reference_catalog_.empty();
        size_t const latest_version = reference_catalog_.active();
        if (auto const ready = fresh ? reference_create(latest_version)
                                     : reference_open_existing(latest_version);
            ! ready) {
            return std::unexpected(ready.error());
        }
        reference_catalog_.add(latest_version);   // a fresh database has just created it
        entries_count_ += reference_map().size();

        // Count entries in previous versions (still searchable/deletable)
        for (auto const v : reference_catalog_.below(latest_version)) {
            auto file_name = db_path_ / fmt::format(reference_data_file_format, v);
            // The catalogue says this version is here, so being unable to read
            // it is not a smaller database — it is one this instance cannot
            // describe. Carrying on would publish a size() short by whatever the
            // file held, and hand back something that looks healthy until an
            // operation happens to reach that generation. The count is also a
            // running total from here on: insert() adds to it and apply_deletes()
            // subtracts, so a wrong starting point stays wrong for the life of
            // the instance.
            auto opened = open_existing_segment(file_name);
            if ( ! opened) return std::unexpected(opened.error());
            // The stamp before the map, here too: this is the first thing that
            // reads a historical version, so it is the first place a file that
            // is not what the catalogue thinks would be believed.
            if (auto const stamped = validate_stamp(
                    **opened, file_name, expected_identity(reference_container_kind, v));
                ! stamped) {
                return std::unexpected(stamped.error());
            }
            auto const found = find_single_named<reference_map_t>(**opened, map_object_name, file_name);
            if ( ! found) return std::unexpected(found.error());
            entries_count_ += (*found)->size();
        }

        for (auto const v : reference_catalog_.versions()) {
            reference_load_metadata(v);
        }
    } else {
        // Full mode: 5 containers
        static_assert(container_count == 5);

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

        // See the reference branch. Every class of a database that was created
        // has at least version zero, so all five catalogues empty means the
        // config is there and the database behind it is not — and the loop below
        // would create the five segments rather than say so.
        //
        // All five, not any one: a class whose generations were all drained by
        // compaction is legitimately empty and gets version zero back on the next
        // open, which is ordinary and not this.
        if (intent == open_intent::inspection) {
            bool any = false;
            for (auto const& catalog : catalogs_) any = any || ! catalog.empty();
            if ( ! any) {
                log::error("{}: a config with no generations is not a database to "
                           "open, and open_existing does not create the missing ones",
                           path_display(db_path_));
                return std::unexpected(error_code::database_not_found);
            }
        }

        // As with the catalogue above, the first container that cannot be
        // described stops the open; the ones after it are not even mapped.
        result<> count_error;
        for_each_index<container_count>([&](auto I) {
            if ( ! count_error.has_value()) return;

            // See the reference branch: the catalogue decides, not the map.
            bool const fresh = catalogs_[I].empty();
            size_t const latest_version = catalogs_[I].active();

            // A class with no generations at all. Compaction can drain one
            // completely, so this is an ordinary state of a working database and
            // not damage — and an open-or-create gives it version zero back,
            // which is right for a store about to receive inserts.
            //
            // An inspection must not. Creating a ten-megabyte file on the way to
            // measuring a database means measuring something this call made, and
            // the report would carry it. The class stays absent, is censused as
            // zero generations and zero entries, and every operation that would
            // need the container it does not have is refused; see
            // refuse_if_inspection_only().
            if (fresh && intent == open_intent::inspection) return;

            if (auto const ready = fresh ? create_container<I>(latest_version)
                                         : open_existing_container<I>(latest_version);
                ! ready) {
                count_error = std::unexpected(ready.error());
                return;
            }
            catalogs_[I].add(latest_version);   // a fresh database has just created it

            // Count existing entries in active container
            entries_count_ += container<I>().size();

            // Count entries in previous versions (still searchable/deletable)
            for (auto const v : catalogs_[I].below(latest_version)) {
                auto file_name = db_path_ / fmt::format(data_file_format, I.value, v);
                // See the reference branch: a catalogued version this instance
                // cannot read is not a smaller database.
                auto opened = open_existing_segment(file_name);
                if ( ! opened) {
                    count_error = std::unexpected(opened.error());
                    return;
                }
                // See the reference branch: the stamp is checked before the map.
                if (auto const stamped = validate_stamp(
                        **opened, file_name, expected_identity(uint32_t(I.value), v));
                    ! stamped) {
                    count_error = std::unexpected(stamped.error());
                    return;
                }
                auto const found = find_single_named<utxo_map<container_sizes[I]>>(
                    **opened, map_object_name, file_name);
                if ( ! found) {
                    count_error = std::unexpected(found.error());
                    return;
                }
                entries_count_ += (*found)->size();
            }

            for (auto const v : catalogs_[I].versions()) {
                load_metadata_from_disk(I, v);
            }
        });
        if ( ! count_error.has_value()) return count_error;
    }

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
result<> database_impl::rotate_for(rotation_cause cause) {
    try {
        new_version<Index>();
    } catch (std::exception const& e) {
        // Counted, and then refused for good. `new_version` closes the previous
        // container before it makes the next one, so a failure here leaves
        // `containers_[Index]` null — and `container<Index>()` dereferences it.
        // The data on disk is untouched; this instance simply has nowhere to put
        // the next entry of this class, and finding that out by dereferencing
        // nothing is not an option.
        ++rotation_causes_[Index].failed;
        integrity_latched_ = error_code::file_open_failed;
        diagnose([&] {
            log::error("container {} could not rotate ({}): {}. No generation was made "
                       "and this instance will not serve this class again; the stored "
                       "data is intact and a new instance can open it",
                       Index, to_string(cause), e.what());
        });
        return std::unexpected(error_code::file_open_failed);
    }
    if (cause == rotation_cause::preventive) ++rotation_causes_[Index].preventive;
    else                                     ++rotation_causes_[Index].capacity_exception;
    return {};
}

template<size_t Index>
result<bool> database_impl::insert_in_index(raw_outpoint const& key, output_data_span value, uint32_t height) {
    // The guard already reads the segment's free bytes, and that is the figure
    // the diagnostic below wants. Taking it from there rather than asking again
    // keeps the common path to one read of the segment header — a different
    // mapped page from the one the map lives in, so a second read of it is a
    // second chance to miss.
    //
    // Seeded with the sentinel, not with zero: the guard writes nothing when it
    // refuses on the size check or when a seam forces the rotation, and "no
    // segment measured" must not be logged as "no bytes left".
    uint64_t free_before = free_bytes_unavailable;
    if ( ! can_insert_safely<Index>(&free_before)) {
        // The guard says the next insert would make the map grow. Only an insert
        // that actually inserts can: `emplace` looks the key up first and returns
        // without growing when it finds it.
        //
        // So a key that is already here must not rotate. Rotating would seal the
        // generation holding it and put a second copy in the empty one — two
        // copies of one outpoint across generations, made by the guard that
        // exists to keep the file healthy.
        //
        // One probe, and only on the path that has already decided to rotate:
        // once per generation, not once per insert. It closes the active map's
        // half of the question and no more. A copy in an older generation is a
        // different contract and this does not look for it.
        if (auto const& current = container<Index>(); current.find(key) != current.end()) {
            diagnose([&] {
                log::warn("insert: duplicate key at the rotation threshold, so not "
                          "rotating. container={} generation={} height={} outpoint={}",
                          Index, current_versions_[Index], height, outpoint_to_string(key));
            });
            return false;
        }
        diagnose([&] {
            log::debug("container {} reached its insert limit; rotating. generation={} "
                       "size={} buckets={} live_max_load={} effective_limit={}",
                       Index, current_versions_[Index], container<Index>().size(),
                       container<Index>().bucket_count(), container<Index>().max_load(),
                       effective_insert_limit(container<Index>().bucket_count(),
                                              container<Index>().max_load()));
        });
        if (auto const rotated = rotate_for<Index>(rotation_cause::preventive); ! rotated) {
            return std::unexpected(rotated.error());
        }
        // The figure the guard measured describes the generation that was just
        // sealed. Every attempt below has to report the segment it actually ran
        // on, or the log compares one file's free bytes with another's.
        free_before = free_bytes_best_effort(segments_[Index].get());
    }

    // Prepare value. Value-initialised: set_data() defines everything from the
    // payload onwards, and this defines what comes before it, so no byte of what
    // reaches the file is left holding whatever this stack frame last held.
    utxo_value<container_sizes[Index]> val{};
    val.block_height = height;
    val.set_data(value);

    // One rotation and one retry, where there used to be three attempts. On a
    // segment that cannot hold a single entry the old loop made three files
    // before giving up — up to four gigabytes of empty generations for the
    // 44-byte class. A second failure on a generation created moments ago is not
    // a full file; it is a configuration or a filesystem, and making more of them
    // does not help.
    for (unsigned attempt = 1; attempt <= 2; ++attempt) {
        auto& map = container<Index>();
        auto const before = snapshot_of(map);
        auto const segment_size = segments_[Index] ? segments_[Index]->get_size() : 0;

        try {
            // The seam that makes the recovery below testable exactly; see
            // failpoints::fail_insert_emplace. One relaxed load on the hot path,
            // and the first comparison ends it.
            if (failpoints::consume_insert_failure()) {
                if (failpoints::fail_insert_after_mutating.load(std::memory_order_relaxed)) {
                    map.emplace(key, val);
                }
                throw bip::bad_alloc();
            }
            auto [it, inserted] = map.emplace(key, val);
            if ( ! inserted) {
                diagnose([&] {
                    log::warn("insert: duplicate key at height {}, outpoint={}, "
                              "container={}", height, outpoint_to_string(key), Index);
                });
            }
            if (inserted) {
                ++entries_count_;

                // The invariant, checked against the count the generation was
                // opened with rather than against the previous insert: a growth
                // from any path breaks it, not only one this insert witnessed.
                //
                // The insert has already happened, so this cannot become a
                // retryable error — a caller that retried would write the entry
                // twice. It is reported and counted, and the insert is still a
                // success, because the entry is there.
                bool const rehashed = note_rehash_if_grown(
                    Index, rehash_watch_[Index], map.bucket_count());

#if UTXOZ_STATISTICS_LEVEL >= 1
                // Update statistics
                ++container_stats_[Index].total_inserts;
                ++container_stats_[Index].current_size;
                ++container_stats_[Index].value_size_distribution[value.size()];
                ++height_range_stats_.ranges[height / height_range_stats::range_size].inserts[Index];
                if (rehashed) ++container_stats_[Index].rehash_count;
#endif

                update_metadata_on_insert(Index, current_versions_[Index], key, height);

                if (attempt > 1) {
                    // The entry is in and the bookkeeping is done; what is left is
                    // the account of it, and an account that cannot be written must
                    // not turn a stored entry into an exception.
                    diagnose([&] {
                        log::info("insert retry after capacity rotation succeeded: "
                                  "container={} new_generation={} attempt={}/2 height={} "
                                  "outpoint={} size={} buckets={} live_max_load={}",
                                  Index, current_versions_[Index], attempt, height,
                                  outpoint_to_string(key), map.size(), map.bucket_count(),
                                  map.max_load());
                    });
                }
            }
            return inserted;

        } catch (bip::bad_alloc const& e) {
            // Read after the throw, from the same map: under the guarantee
            // `emplace` documents these are the figures it had before. All three
            // accessors are `noexcept`, and the probe is best-effort, so reaching
            // the classification cannot itself fail.
            auto& after_map = container<Index>();
            auto const after = snapshot_of(after_map);
            bool const key_present_after = after_map.find(key) != after_map.end();
            auto const free_after = free_bytes_best_effort(segments_[Index].get());
            auto const state = classify_post_exception(before, after, key_present_after);

            // Everything the next reader of this log would otherwise have to
            // reconstruct — built on demand, and only from inside `diagnose`.
            // Assembling it allocates, and until it was moved in here a failure
            // to allocate the *message* could leave the failure it describes
            // uncounted, unlatched and unhandled.
            auto const shared = [&] {
                return fmt::format(
                    "container={} generation={} attempt={}/2 outpoint={} height={} "
                    "payload_size={} size_before={} size_after={} buckets_before={} "
                    "buckets_after={} live_max_load_before={} live_max_load_after={} "
                    "rotation_threshold={} no_rehash_limit={} segment_bytes={} "
                    "free_before={} free_after={} key_present_after={} exception={}",
                    Index, current_versions_[Index], attempt, outpoint_to_string(key),
                    height, value.size(), before.size, after.size, before.bucket_count,
                    after.bucket_count, before.live_max_load, after.live_max_load,
                    max_entries_for(before.bucket_count),
                    max_size_without_rehash(before.bucket_count),
                    segment_size, free_bytes_display(free_before),
                    free_bytes_display(free_after),
                    key_present_after ? "true" : "false", e.what());
            };

            if (state == post_exception_state::map_mutated) {
                // The container did something it documents it will not do. There
                // is no sidecar, no merge record and nothing to rebuild the map
                // from, so there is no recovery to instruct and this instance
                // stops here.
                //
                // Counted and latched before a word is written about it.
                ++rotation_causes_[Index].unexpected_post_exception;
                integrity_latched_ = error_code::entry_corrupt;
                diagnose([&] {
                    log::critical("insert: the active map changed after a failed "
                                  "allocation, which contradicts the container's "
                                  "guarantee. Not rotating and not retrying. Stop this "
                                  "process and verify or rebuild the database before "
                                  "opening it again; this latch does not survive a "
                                  "restart. {}", shared());
                });
                return std::unexpected(error_code::entry_corrupt);
            }

            if (attempt == 2) {
                // A generation made moments ago could not take one entry. Nothing
                // is corrupt and nothing is worth rotating again.
                diagnose([&] {
                    log::error("insert: a newly created generation could not take a "
                               "single entry. This is capacity or configuration, not "
                               "corruption. {}", shared());
                });
                return std::unexpected(error_code::insufficient_space);
            }

            // The map is exactly as it was, so the entry never went in and the
            // rotation is safe. `emplace` finds the key before it grows, so a
            // throw already proves the key was absent — the duplicate question
            // the preventive path has to ask does not arise here.
            //
            // Read before the rotation rather than reconstructed after it. A
            // version number is an identity and the sequence has holes in it
            // wherever compaction has drained one, so `new - 1` names a
            // generation only for as long as the numbering happens to be dense.
            // See version_catalog.hpp.
            auto const old_generation = current_versions_[Index];
            diagnose([&] {
                log::info("insert: allocation failed; map unchanged; rotating and "
                          "retrying. cause=allocator_refused_growth {}", shared());
            });
            if (auto const rotated = rotate_for<Index>(rotation_cause::capacity_exception);
                    ! rotated) {
                return std::unexpected(rotated.error());
            }
            // The retry runs on the new generation, so its diagnostic must too.
            free_before = free_bytes_best_effort(segments_[Index].get());
            diagnose([&] {
                log::info("insert: rotated after a failed allocation: container={} "
                          "old_generation={} new_generation={} height={} outpoint={}",
                          Index, old_generation, current_versions_[Index],
                          height, outpoint_to_string(key));
            });
        }
    }

    // Unreachable: the loop returns on success, on the second failure and on
    // every classified refusal above.
    diagnose([&] {
        log::error("insert: exhausted the single retry without a decision, container={}",
                   Index);
    });
    return std::unexpected(error_code::insufficient_space);
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

    // A probe the active map could not answer. Recording it is what makes the
    // hit rate mean something: without it every recorded probe was a hit.
    //
    // The counter is all that happens. The key is not kept: whoever asked keeps
    // it and hands it to resolve() (#116).
    probe_stats_.record_deferred();
    return std::nullopt;
}

std::optional<find_result> database_impl::find_in_latest_version(raw_outpoint const& key,
                                                                  uint32_t height) const {
    std::optional<find_result> result;

    for_each_index<container_count>([&](auto I) {
        if (!result) {
            auto& map = container<I>();
            if (auto it = map.find(key); it != map.end()) {
#if UTXOZ_STATISTICS_LEVEL >= 1
                probe_stats_.record_answered(height, it->second.block_height);
#endif
#if UTXOZ_STATISTICS_LEVEL >= 2
                lookup_stats_[I.value].record_answered_from_active();
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


size_t database_impl::erase_in_latest_version(raw_outpoint const& key, uint32_t height) {
    size_t result = 0;

    for_each_index<container_count>([&](auto I) {
        if (result == 0) {
            auto& map = container<I>();
            if (auto it = map.find(key); it != map.end()) {
#if UTXOZ_STATISTICS_LEVEL >= 1
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

#if UTXOZ_STATISTICS_LEVEL >= 1
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


// =============================================================================
// database_impl - Deferred deletions
// =============================================================================


deletion_progress database_impl::apply_deletes(std::span<deferred_deletion_entry const> requests) {
    deletion_progress progress;
    if (requests.empty()) return progress;

    // Indices into the caller's batch, deduplicated by key and shrinking as
    // deletions are applied. Nothing is taken: the requests stay in the caller's
    // span and are still the caller's when this returns (#119).
    auto pending = working_set_of<deferred_deletion_entry>(requests);

    // Reserved before anything is erased. progress.erased is written while the
    // maps are being changed, and a vector growing at that moment is an
    // allocation that can fail — which would make "we ran out of memory" the
    // reason a deletion that really happened went unreported. The other two are
    // filled after all mutation, and are reserved here for the same reason
    // rather than a different one.
    progress.erased.reserve(pending.size());
    progress.absent.reserve(pending.size());
    progress.unresolved.reserve(pending.size());

    // Everything that is still owed at the end, whatever the reason. Filled once,
    // at the end, from whatever survived — so a key can never be in two lists and
    // can never be missing from all three.
    auto const classify_remainder = [&](std::vector<deferred_deletion_entry>& into) {
        into.reserve(into.size() + pending.size());
        for (auto const idx : pending) into.push_back(requests[idx]);
    };

#if UTXOZ_STATISTICS_LEVEL >= 1
    auto const start_time = std::chrono::steady_clock::now();
#endif
    log::debug("Applying {} deletions ({} distinct)...", requests.size(), pending.size());

    // Unlike a resolution, this cannot be transactional: every erase below writes
    // to a mapped file immediately. So a fault partway through leaves earlier
    // deletions applied, and the contract is to enumerate them rather than to
    // pretend they did not happen. What must never happen is calling the
    // remainder absent — a version that would not open could hold any of them.
    bool complete = true;
    error_code failure = error_code::version_unreadable;

    // Phase 0: the active versions. This is what the old single-key erase() did
    // inline before queueing, and it is the common case: an output spent soon
    // after it was created has not been rotated away yet.
    {
        size_t keep = 0;
        for (size_t i = 0; i < pending.size(); ++i) {
            auto const idx = pending[i];
            auto const& request = requests[idx];
            size_t const applied = (mode_ == storage_mode::reference)
                ? reference_erase_in_latest(request.key, request.height)
                : erase_in_latest_version(request.key, request.height);
            if (applied > 0) {
                entries_count_ -= applied;
                progress.erased.push_back(request);
                continue;
            }
            pending[keep++] = idx;
        }
        pending.resize(keep);
    }

    // Applied so far by the historical walk, for the seam below. Counted across
    // files rather than per file, so a case can aim the throw at a file that has
    // already applied something.
    //
    // The active-version phase above is deliberately not counted. It has no throw
    // point — the seam lives inside the per-file loop — so including it would
    // only shift the numbering by however many keys happened to still be in the
    // active version, which is not something a case can know.
    uint64_t applied_in_walk = 0;

    // One file's worth of the walk, over whatever map it is handed.
    //
    // Written so that `pending` and the map never disagree — not even for the
    // span of one statement, and not even if something throws between them.
    //
    // The shape this replaces compacted `pending` only after the loop. An
    // exception raised after a key was gone from the map therefore left its index
    // still in `pending`, so the same key came back both applied and still owed;
    // and if the throw happened before the key was recorded, it was deleted and
    // reported nowhere. Neither is a state a caller can act on, and "deleted but
    // reported as owed" is the one that loses data on the retry.
    //
    // So: the erase is the first thing, recording it is the next thing and cannot
    // allocate — progress.erased is reserved before any mutation — and the
    // compaction of `pending` is finished by a scope guard that runs on the
    // exception path too.
    //
    // The bookkeeping is a parameter rather than a branch inside. Which catalogue
    // a deletion belongs to is a property of the file being walked, and passing it
    // in is what keeps that decision next to the code that opened the file.
    auto const step_over_file = [&](auto& map, auto const& record_deletion) {
        size_t keep = 0;
        size_t i = 0;
        bool current_applied = false;

        scope_exit const settle([&] {
            // Indices below `i` are decided: the applied ones dropped, the rest
            // already moved down into [0, keep). Index `i` is decided only if its
            // own erase went through. Everything above is untouched and copied
            // down as it stands.
            size_t const tail = current_applied ? i + 1 : i;
            for (size_t j = tail; j < pending.size(); ++j) pending[keep++] = pending[j];
            pending.resize(keep);
        });

        for (; i < pending.size(); ++i) {
            current_applied = false;
            auto const idx = pending[i];

            if (map.erase(requests[idx].key) == 0) {
                pending[keep++] = idx;
                continue;
            }

            // The map has changed. Record it before anything that can throw.
            progress.erased.push_back(requests[idx]);
            --entries_count_;
            current_applied = true;

            if (failpoints::fail_delete_after_applied.load(std::memory_order_relaxed)
                    == ++applied_in_walk) {
                throw std::runtime_error("failpoint: threw after applying a deletion");
            }

            record_deletion(requests[idx].height);
        }
    };

    // The reference walk is its own function rather than the full one with a
    // sentinel threaded through it.
    //
    // It used to share erase_in_file<Index>, reached through a switch whose
    // default mapped the sentinel onto Index 0. The guard inside was
    // `if constexpr (Index == SIZE_MAX)`, which that dispatch can never satisfy,
    // so a historical reference deletion erased the right entry, marked the right
    // file dirty — note_dirty takes the runtime index — and then updated the
    // metadata of *full container 0*. The reference catalogue never heard about
    // the deletion, and container 0's said an entry left a file it was never in.
    //
    // Two functions cost less than one that has to be told which of two shapes it
    // is, and neither can be instantiated for a sentinel that is not a container
    // index.
    auto const erase_in_reference_file = [&](size_t version) {
        try {
            if (failpoints::fail_historical_open_version.load(std::memory_order_relaxed)
                    == static_cast<uint64_t>(version)) {
                throw std::runtime_error("failpoint: version file refused to open");
            }

            auto [map, cache_hit] = file_cache_->get_or_open_reference_file(version);
            (void) cache_hit;
            step_over_file(map, [&]([[maybe_unused]] uint32_t height) {
                note_dirty(reference_sentinel_index, version);
                if (auto* meta = reference_catalog_.find_metadata(version)) meta->update_on_delete();
                failpoints::reference_metadata_deletes.fetch_add(1, std::memory_order_relaxed);
#if UTXOZ_STATISTICS_LEVEL >= 1
                // The same counters the queue-draining path kept. Reference mode
                // reports through container 0's slot, as it does everywhere else.
                auto const depth =
                    static_cast<uint32_t>(reference_current_version_ - version);
                ++deferred_stats_.deletions_by_depth[depth];
                --container_stats_[0].current_size;
                ++container_stats_[0].total_deletes;
                ++height_range_stats_.ranges[height / height_range_stats::range_size].deletes[0];
#endif
            });
        } catch (std::exception const& e) {
            log::error("Could not apply deletions in reference v{}: {}. The batch is incomplete.",
                       version, e.what());
            complete = false;
        }
    };

    auto const erase_in_full_file = [&]<size_t Index>(std::integral_constant<size_t, Index>,
                                                     size_t version) {
        try {
            if (failpoints::fail_historical_open_version.load(std::memory_order_relaxed)
                    == static_cast<uint64_t>(version)) {
                throw std::runtime_error("failpoint: version file refused to open");
            }

            auto [map, cache_hit] = file_cache_->get_or_open_file<Index>(Index, version);
            (void) cache_hit;
            step_over_file(map, [&]([[maybe_unused]] uint32_t height) {
                note_dirty(Index, version);
                update_metadata_on_delete(Index, version);
                failpoints::full_metadata_deletes.fetch_add(1, std::memory_order_relaxed);
#if UTXOZ_STATISTICS_LEVEL >= 1
                // Restored with the rest of the historical path. Dropping these
                // made a deletion that reached an older file invisible to every
                // per-container number while the active-version phase kept
                // recording its own — so the two halves of the same call
                // disagreed, and by container.
                //
                // deferred_deletes is deliberately not among them: it counted how
                // much was sitting in the queue, and there is no queue to sit in.
                auto const depth = static_cast<uint32_t>(current_versions_[Index] - version);
                ++deferred_stats_.deletions_by_depth[depth];
                --container_stats_[Index].current_size;
                ++container_stats_[Index].total_deletes;
                ++height_range_stats_.ranges[height / height_range_stats::range_size].deletes[Index];
#endif
            });
        } catch (std::exception const& e) {
            // This file could hold any of the keys still pending, so none of them
            // can be called absent. What was already erased stays erased and stays
            // reported — that is the difference from a resolution, and the guard
            // in step_over_file is what makes it true even here.
            log::error("Could not apply deletions in ({}, v{}): {}. The batch is incomplete.",
                       Index, version, e.what());
            complete = false;
        }
    };

    auto const walk = [&](size_t container_index, size_t version) {
        if (container_index == reference_sentinel_index) {
            erase_in_reference_file(version);
            return;
        }
        switch (container_index) {
            case 0: erase_in_full_file(std::integral_constant<size_t, 0>{}, version); break;
            case 1: erase_in_full_file(std::integral_constant<size_t, 1>{}, version); break;
            case 2: erase_in_full_file(std::integral_constant<size_t, 2>{}, version); break;
            case 3: erase_in_full_file(std::integral_constant<size_t, 3>{}, version); break;
            case 4: erase_in_full_file(std::integral_constant<size_t, 4>{}, version); break;
            default:
                // Not a container this build has. Silently walking it as index 0
                // is how the sentinel came to update the wrong catalogue, so an
                // index nobody recognises stops the batch instead.
                log::error("Deletion batch asked for container {}, which does not exist. "
                           "The batch is incomplete.", container_index);
                complete = false;
                break;
        }
    };

    // Phase 1: cached files first — already mapped, so they cost nothing to visit.
    auto cached_files = file_cache_->get_cached_files();
    if ( ! cached_files.empty()) {
        std::ranges::sort(cached_files, [](auto const& a, auto const& b) {
            if (a.first != b.first) return a.first < b.first;
            return a.second > b.second;
        });
        for (auto const& [container_index, version] : cached_files) {
            if (pending.empty()) break;
            if (mode_ == storage_mode::reference) {
                if (container_index == reference_sentinel_index) walk(container_index, version);
            } else if (container_index != reference_sentinel_index) {
                walk(container_index, version);
            }
        }
    }

    // Phase 2: every remaining version below the current one.
    if ( ! pending.empty()) {
        if (mode_ == storage_mode::reference) {
            std::set<size_t> seen;
            for (auto const& [ci, version] : cached_files) {
                if (ci == reference_sentinel_index) seen.insert(version);
            }
            try {
                if (failpoints::fail_historical_catalog.load(std::memory_order_relaxed)) {
                    throw std::runtime_error("failpoint: catalogue refused to be listed");
                }
                for (auto const v : reference_catalog_.below(reference_current_version_)) {
                    if (pending.empty()) break;
                    if (seen.contains(v)) continue;
                    walk(reference_sentinel_index, v);
                }
            } catch (std::exception const& e) {
                log::error("Could not enumerate reference versions: {}. The batch is incomplete.", e.what());
                complete = false;
                failure = error_code::catalog_unreadable;
            }
        } else {
            std::array<std::set<size_t>, container_count> seen;
            for (auto const& [ci, version] : cached_files) {
                if (ci < container_count) seen[ci].insert(version);
            }
            for_each_index<container_count>([&](auto I) {
                if (pending.empty()) return;
                try {
                    if (failpoints::fail_historical_catalog.load(std::memory_order_relaxed)) {
                        throw std::runtime_error("failpoint: catalogue refused to be listed");
                    }
                    for (auto const v : catalogs_[I.value].below(current_versions_[I.value])) {
                        if (pending.empty()) break;
                        if (seen[I.value].contains(v)) continue;
                        walk(I.value, v);
                    }
                } catch (std::exception const& e) {
                    log::error("Could not enumerate versions of container {}: {}. The batch is incomplete.",
                               I.value, e.what());
                    complete = false;
                    failure = error_code::catalog_unreadable;
                }
            });
        }
    }

    if ( ! complete) {
        // Owed, not absent. Every one of these could be in the file that would
        // not open, and reporting them as absent is the mistake this design
        // exists to make impossible.
        classify_remainder(progress.unresolved);
        progress.error = failure;
        log::error("Deletion batch incomplete: {} applied, {} still owed and none of them absent",
                   progress.erased.size(), progress.unresolved.size());
    } else {
        // Every version that could hold them was read, so what is left was looked
        // for everywhere it could have been. Only now is absence a fact.
        classify_remainder(progress.absent);
    }

#if UTXOZ_STATISTICS_LEVEL >= 1
    // Applied deletions are counted whether or not the batch finished: they
    // happened, and the entry count already reflects them. `processing_runs` is
    // not, because it means a run that completed — an incomplete batch that
    // counted one would make the averages describe work that was never done.
    deferred_stats_.successfully_processed += progress.erased.size();
    deferred_stats_.failed_to_delete += progress.unresolved.size();
    if (complete) ++deferred_stats_.processing_runs;

    auto const end_time = std::chrono::steady_clock::now();
    deferred_stats_.total_processing_time +=
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
#endif

    log::debug("Deletion batch: {} erased, {} absent, {} unresolved",
               progress.erased.size(), progress.absent.size(), progress.unresolved.size());
    return progress;
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

fs::path database_impl::data_path(size_t index, size_t version) const {
    if (index == reference_sentinel_index) {
        return db_path_ / fmt::format(reference_data_file_format, version);
    }
    return db_path_ / fmt::format(data_file_format, index, version);
}

fs::path database_impl::building_path(size_t index, size_t version) const {
    fs::path p = data_path(index, version);
    p += ".building";
    return p;
}

fs::path database_impl::sidecar_path(size_t index, size_t version) const {
    if (index == reference_sentinel_index) {
        return db_path_ / fmt::format("compact_v{:05}.merge", version);
    }
    return db_path_ / fmt::format("cont_{}_v{:05}.merge", index, version);
}

fs::path database_impl::metadata_path(size_t index, size_t version) const {
    if (index == reference_sentinel_index) {
        return db_path_ / fmt::format("meta_compact_v{:05}.dat", version);
    }
    return db_path_ / fmt::format("meta_{}_v{:05}.dat", index, version);
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
    // This function is where the absent/duplicated check was first written by
    // hand. find_single_named() is that check, extracted; every reason it can
    // refuse means the same thing here, so they collapse into one error.
    auto const opened = open_existing_segment(path);
    if ( ! opened) return std::unexpected(error_code::recovery_failed);

    auto const found = find_single_named<merge_marker>(**opened, merge_marker::object_name, path);
    if ( ! found) return std::unexpected(error_code::recovery_failed);
    return **found;
}

namespace {

/// Removes a path, treating "it was not there" as success. Recovery has to be
/// idempotent: a crash part way through it must leave a state it can finish.
[[nodiscard]]
result<> remove_if_present(fs::path const& path) {
    return remove_file(path);
}

} // anonymous namespace

/**
 * Finishes or abandons one interrupted merge.
 *
 * Which of the two depends on a single question — did the target get its
 * canonical name — and the answer is on disk. Everything here is idempotent.
 */
result<> database_impl::recover_one(merge_plan const& plan, fs::path const& sidecar) {
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
                log::error("Recovery: the merge record at {} describes a different operation", path_display(path));
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
                        db_path_ / fmt::format("{}{:05}{}", sc.prefix, version, suffix));
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
size_t full_merge_policy<Index>::file_size() const { return db.capacity_[Index].file_size; }
template <size_t Index>
size_t full_merge_policy<Index>::min_buckets() const { return db.capacity_for(Index); }
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
size_t reference_merge_policy::min_buckets() const { return db.capacity_for_reference(); }
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

        // Every exit from here to the end of this block discards what was being
        // built, and there are seven of them. They used to say so one by one,
        // which is exactly how one came to be missing: a source whose map could
        // not be reached returned without unmapping or removing the target, and
        // the .building file survived a failed compaction. Said once, it cannot
        // be left out of a branch added later.
        //
        // Only up to here. Past this block the target is synced, recorded and
        // renamed, and those steps abandon it deliberately and in an order that
        // the crash cases pin; a guard reaching over them would be changing a
        // sequence rather than removing a repetition.
        bool built = false;
        scope_exit const discard_target([&] {
            if (built) return;
            segment.reset();   // unmapped before it is unlinked
            abandon();
        });
        // The kind this container is written as. idx is SIZE_MAX for reference
        // mode, which is not the same number on every platform, so it never
        // reaches the file.
        auto const kind = idx == reference_sentinel_index ? reference_container_kind
                                                          : uint32_t(idx);

        // A merge target is a file this call just created, so it is stamped
        // before it holds anything — and stamped as the version it will be
        // published as, not as the one it is being built under.
        if (auto const stamped = place_stamp(*segment, building,
                                             local_identity(database_id_, kind,
                                                            uint64_t(target)));
            ! stamped) {
            return std::unexpected(stamped.error());
        }

        auto* target_map = Policy::construct_map(*segment, policy.min_buckets());

        // The invariant applies here too, and this is where it was missing: a
        // merge that filled the target past its threshold would grow the map it
        // had just built, and reference has room in its file for exactly that.
        // Recorded now so the whole construction can be checked against it.
        size_t const target_buckets = target_map->bucket_count();
        // The growth point, not the operating threshold. A sealed target is built
        // once and never inserted into, so it does not need the five per cent of
        // reserve a live container keeps — it needs only to not grow. Using the
        // live threshold here would refuse merges that fit perfectly well.
        size_t const target_limit = max_size_without_rehash(target_buckets);

        // Before the barriers, so the marker is as durable as the entries.
        segment->template construct<merge_marker>(merge_marker::object_name)(merge_id);

        for (auto const source : sources) {
            auto const source_path = data_path(idx, source);

            // Refused, not skipped. Every source is unlinked once the target is
            // published, so a source that was passed over would have its entries
            // dropped from the merge and the only copy of them deleted straight
            // after. Nothing is published unless every source was read.
            auto opened_source = open_existing_segment(source_path);
            if ( ! opened_source) {
                log::error("compaction: {} could not be read; nothing is published",
                           policy.describe(source));
                return std::unexpected(opened_source.error());
            }
            auto source_segment = std::move(*opened_source);

            if (auto const stamped = validate_stamp(
                    *source_segment, source_path,
                    local_identity(database_id_, kind, uint64_t(source)));
                ! stamped) {
                log::error("compaction: {} is not this database's to merge; nothing is "
                           "published", policy.describe(source));
                return std::unexpected(stamped.error());
            }

            auto const source_map = Policy::find_map(*source_segment, source_path);
            if ( ! source_map) {
                log::error("compaction: {} holds no usable map; nothing is published",
                           policy.describe(source));
                return std::unexpected(source_map.error());
            }

            for (auto const& [key, value] : **source_map) {
                try {
                    // Below the limit, `emplace` is the only lookup: it finds
                    // the key or inserts it, and a duplicate comes back as
                    // `!inserted`. At the limit the order matters and the lookup
                    // is worth paying for — a key present in two sources means the
                    // database is locally inconsistent, which sends the caller
                    // somewhere different from "this group is too large", and a
                    // duplicate costs no capacity. So it is asked first, and only
                    // there.
                    if (target_map->size() >= target_limit) {
                        if (target_map->find(key) != target_map->end()) {
                            log::error("compaction: duplicate key across the sources of "
                                       "{}: {}", policy.describe(target),
                                       outpoint_to_string(key));
                            return std::unexpected(error_code::duplicate_key);
                        }
                        log::debug("compaction: {} holds {} of the {} entries it can take "
                                   "without growing; the caller can retry with fewer "
                                   "sources", policy.describe(target), target_map->size(),
                                   target_limit);
                        return std::unexpected(error_code::insufficient_space);
                    }

                    auto const [pos, inserted] = target_map->emplace(key, value);
                    if ( ! inserted) {
                        // Two sources held the same key. A published state holds
                        // at most one entry per key, so this is the database
                        // being locally inconsistent, and it is reported rather
                        // than resolved: choosing a copy would hide it. Nothing
                        // canonical has changed at this point.
                        log::error("compaction: duplicate key across the sources of {}: {}",
                                   policy.describe(target), outpoint_to_string(key));
                        return std::unexpected(error_code::duplicate_key);
                    }
                    ++entries_moved;

                    // Whatever the guard above believed, the map must not have
                    // grown. Checked per entry rather than at the end: a merge
                    // that grew and then carried on would keep writing into a
                    // file that is no longer the one it planned.
                    if (target_map->bucket_count() != target_buckets) {
                        rehash_watch target_watch;
                        target_watch.reset(target_buckets);
                        note_rehash_if_grown(kind, target_watch, target_map->bucket_count());
                        log::error("compaction: {} grew from {} buckets to {}; nothing is "
                                   "published", policy.describe(target), target_buckets,
                                   target_map->bucket_count());
                        return std::unexpected(error_code::insufficient_space);
                    }
                } catch (boost::interprocess::bad_alloc const&) {
                    // The group was planned to fit and did not. Leave every
                    // source exactly as it is and let the caller try a smaller
                    // group; sources are only ever read here.
                    log::debug("compaction: {} filled early, {} entries in",
                               policy.describe(target), entries_moved);
                    return std::unexpected(error_code::insufficient_space);
                }
            }
        }

        // Mapped pages first, then the file itself: the page barrier covers the
        // pages, not the inode, and neither covers the name.
        if (auto const synced = sync_mapped_region(segment->get_address(), segment->get_size());
            ! synced && synced.error() != error_code::sync_unsupported) {
            return std::unexpected(synced.error());
        }

        // Built. From here the target is the durability sequence's to abandon,
        // and it does so explicitly at each of its own steps.
        built = true;
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
    auto retire = [](fs::path const& path) -> result<> {
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
        // Derived data: failing to rebuild it costs a rescan later and nothing
        // else, so unlike the sources above this one only warns.
        auto const target_path = data_path(idx, target);
        auto const summary_kind = idx == reference_sentinel_index ? reference_container_kind
                                                                  : uint32_t(idx);
        if (auto segment = open_existing_segment(target_path); segment) {
            if (auto const stamped = validate_stamp(
                    **segment, target_path,
                    local_identity(database_id_, summary_kind, uint64_t(target)));
                ! stamped) {
                log::warn("compaction: could not summarise {}", policy.describe(target));
            } else if (auto const map_ptr = Policy::find_map(**segment, target_path); map_ptr) {
                for (auto const& [key, val] : **map_ptr) {
                    meta.update_on_insert(key, Policy::height_of(val));
                }
            } else {
                log::warn("compaction: could not summarise {}", policy.describe(target));
            }
        } else {
            log::warn("compaction: could not summarise {}", policy.describe(target));
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
        // Compaction closed a file that is still there, so this reopens an
        // existing version; it never creates one.
        if (auto const opened = open_existing_container<Index>(active); ! opened) {
            cleanup_pending_ = true;
            return std::unexpected(opened.error());
        }
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
            auto file_name = db_path_ / fmt::format(data_file_format, I.value, v);

            auto opened = open_existing_segment(file_name);
            if ( ! opened) {
                outcome = std::unexpected(opened.error());
                return;
            }
            auto const found = find_single_named<utxo_map<container_sizes[I]>>(
                **opened, map_object_name, file_name);
            if ( ! found) {
                outcome = std::unexpected(found.error());
                return;
            }

            // The callback is the caller's code and may raise; nothing else in
            // here can any more.
            try {
                for (auto const& [key, _] : **found) {
                    cb(ctx, key);
                }
            } catch (std::exception const& e) {
                log::error("for_each_key: the callback raised over container {} v{}: {}", I.value, v, e.what());
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
            auto file_name = db_path_ / fmt::format(data_file_format, I.value, v);

            auto opened = open_existing_segment(file_name);
            if ( ! opened) {
                outcome = std::unexpected(opened.error());
                return;
            }
            auto const found = find_single_named<utxo_map<container_sizes[I]>>(
                **opened, map_object_name, file_name);
            if ( ! found) {
                outcome = std::unexpected(found.error());
                return;
            }

            // The callback is the caller's code and may raise; nothing else in
            // here can any more.
            try {
                for (auto const& [key, val] : **found) {
                    cb(ctx, key, val.block_height, val.get_data());
                }
            } catch (std::exception const& e) {
                log::error("for_each_entry: the callback raised over container {} v{}: {}", I.value, v, e.what());
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
#if UTXOZ_STATISTICS_LEVEL >= 1
    for_each_index<container_count>([&](auto I) {
        if (segments_[I]) {
            try {
                // From the segment that is open, not from the policy. These
                // describe a file; the policy describes what a new one is created
                // with, and the two are only the same number by coincidence — a
                // coincidence that ends the next time the policy moves. Asking the
                // segment also means `total - free` cannot underflow.
                size_t const total_size = segments_[I]->get_size();
                size_t const free_memory = segments_[I]->get_free_memory();
                size_t const used_memory = total_size >= free_memory
                                         ? total_size - free_memory : 0;

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
        // The open segment's own size, for the same reason: this is an
        // observation, and an observation that reads a setting is not one.
        total += segments_[index]->get_size();
    }

    for (auto const v : catalogs_[index].below(current_versions_[index])) {
        auto file_name = db_path_ / fmt::format(data_file_format, index, v);
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

    // The read path, per class. The three figures that have no class come from
    // the counters that already hold them — a lookup arrives with a key and no
    // size, and one that is absent was refused by every class — and everything
    // else is summed from the classes rather than kept a second time.
    stats.lookups.mode = mode_;
#if UTXOZ_STATISTICS_LEVEL >= 2
    stats.lookups.statistics_level = "lookup";
#elif UTXOZ_STATISTICS_LEVEL >= 1
    stats.lookups.statistics_level = "basic";
#else
    stats.lookups.statistics_level = "off";
#endif
    stats.lookups.lookups_received = stats.probes.probes;
    stats.lookups.deferred = stats.probes.deferred;
    stats.lookups.absent = stats.resolution.absent;
#if UTXOZ_STATISTICS_LEVEL >= 2
    if (mode_ == storage_mode::reference) {
        // One class, and it is not container 0. Four empty ones would be four
        // classes that do not exist.
        auto summary = lookup_stats_[0].get_summary();
        summary.container_class = reference_class;
        // One class, so it is asked by every lookup that arrives.
        summary.active_maps_probed = stats.lookups.lookups_received;
        stats.lookups.classes.push_back(summary);
    } else {
        for (size_t index = 0; index < container_count; ++index) {
            auto summary = lookup_stats_[index].get_summary();
            summary.container_class = index;
            stats.lookups.classes.push_back(summary);
        }
        // How often each class was asked, derived here because this is the only
        // place that can see the classes after it and the global deferred count
        // at the same time. A class is asked when the answer came from it or from
        // one after it, or from nowhere at all. Walking backwards makes it one
        // pass; the deferred lookups asked every class, so they are in all five.
        size_t reached = stats.lookups.deferred;
        for (size_t index = container_count; index-- > 0; ) {
            reached += stats.lookups.classes[index].answered_from_active;
            stats.lookups.classes[index].active_maps_probed = reached;
        }
    }
#endif

    stats.total_inserts = 0;
    stats.total_deletes = 0;

    if (mode_ == storage_mode::reference) {
        stats.containers[0] = container_stats_[0];
        stats.total_inserts = container_stats_[0].total_inserts;
        stats.total_deletes = container_stats_[0].total_deletes;
        stats.rotations_per_container[0] = reference_current_version_;
        stats.rotations_by_cause[0] = reference_rotation_causes_;
        // The open segment, not the setting: this reports usage.
        stats.memory_usage_per_container[0] =
            reference_segment_ ? reference_segment_->get_size() : reference_active_file_size_;
    } else {
        for (size_t i = 0; i < container_count; ++i) {
            stats.containers[i] = container_stats_[i];
            stats.total_inserts += container_stats_[i].total_inserts;
            stats.total_deletes += container_stats_[i].total_deletes;
            stats.rotations_per_container[i] = current_versions_[i];
            stats.rotations_by_cause[i] = rotation_causes_[i];
            stats.memory_usage_per_container[i] = estimate_memory_usage(i);
        }
    }

    stats.deferred = deferred_stats_;
    stats.not_found = not_found_stats_;
    stats.lifetime = lifetime_stats_;
    stats.fragmentation = fragmentation_stats_;

    return stats;
}

namespace {

/// Printed under the rotation count it explains. Silent when nothing unusual has
/// happened, which is every ordinary run: a line of zeros in a report read by a
/// person is noise, and the figure that matters is the one that is not zero.
void log_rotation_causes(rotation_causes const& causes) {
    log::info("    preventive: {}", causes.preventive);
    if (causes.capacity_exception != 0) {
        log::info("    after a failed allocation: {}", causes.capacity_exception);
    }
    if (causes.failed != 0) {
        log::info("    could not rotate: {} (not counted above)", causes.failed);
    }
    if (causes.unexpected_post_exception != 0) {
        log::info("    map changed after a failed allocation: {} (no rotation)",
                  causes.unexpected_post_exception);
    }
}

} // namespace

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
        log_rotation_causes(stats.rotations_by_cause[0]);
        log::info("  Est. memory: {:.2f} MB", stats.memory_usage_per_container[0] / (1024.0*1024.0));
    } else {
        for (size_t i = 0; i < container_count; ++i) {
            log::info("Container {} (size <= {} bytes):", i, container_sizes[i]);
            log::info("  Current entries: {}", stats.containers[i].current_size);
            log::info("  Total inserts: {}", stats.containers[i].total_inserts);
            log::info("  Total deletes: {}", stats.containers[i].total_deletes);
            log::info("  File rotations: {}", stats.rotations_per_container[i]);
            log_rotation_causes(stats.rotations_by_cause[i]);
            log::info("  Est. memory: {:.2f} MB", stats.memory_usage_per_container[i] / (1024.0*1024.0));
        }
    }

    log::info("--- Cache Statistics ---");
    log::info("Cache hit rate: {:.2f}%", stats.cache_hit_rate * 100);
    log::info("Cached files: {}", stats.cached_files_count);

    // The read path, per class. Printed before the probe summary because it is
    // the finer answer to the same question, and because the two numbers a
    // reader will want to compare — how often a class was asked and how often it
    // answered — are next to each other here.
    log::info("--- Lookups, by class ---");
    log::info("  statistics level: {}", stats.lookups.statistics_level);
    if (stats.lookups.classes.empty()) {
        log::info("  this build carries no per-class lookup telemetry, so there is");
        log::info("  nothing below — which is not the same as a database that");
        log::info("  answered nothing. Build with -DUTXOZ_STATISTICS_LEVEL=lookup.");
    }
    log::info("  {:>9} {:>12} {:>12} {:>12} {:>12} {:>10} {:>8} {:>8}",
              "class", "maps probed", "answered", "from history", "gen probes",
              "files", "cache", "avg ord");
    for (auto const& c : stats.lookups.classes) {
        auto const name = c.container_class == reference_class
            ? std::string("reference") : fmt::format("{}", c.container_class);
        log::info("  {:>9} {:>12} {:>12} {:>12} {:>12} {:>10} {:>8} {:>8.2f}",
                  name, c.active_maps_probed, c.answered_from_active,
                  c.resolved_historical, c.generations_probed, c.files_opened,
                  c.cache_hits, c.avg_probe_ordinal);
    }
    log::info("  received {}, deferred {}, absent {} — these three have no class:",
              stats.lookups.lookups_received, stats.lookups.deferred, stats.lookups.absent);
    log::info("  a lookup arrives with a key and no size, and an absent one was");
    log::info("  refused by every class");
    log::info("  probe ordinal is how many generation files a key was searched in;");
    log::info("  version distance is how far back the answering generation was.");
    log::info("  They are different numbers: compaction leaves gaps in the");
    log::info("  numbering and the cache is searched before the catalogue.");
    for (auto const& c : stats.lookups.classes) {
        if (c.resolved_historical == 0) continue;
        auto const name = c.container_class == reference_class
            ? std::string("reference") : fmt::format("{}", c.container_class);
        log::info("  class {} ordinal 1/2/3/4/5-8/9+: {}/{}/{}/{}/{}/{}  distance: {}/{}/{}/{}/{}/{}",
                  name,
                  c.probe_ordinal_histogram[0], c.probe_ordinal_histogram[1],
                  c.probe_ordinal_histogram[2], c.probe_ordinal_histogram[3],
                  c.probe_ordinal_histogram[4], c.probe_ordinal_histogram[5],
                  c.version_distance_histogram[0], c.version_distance_histogram[1],
                  c.version_distance_histogram[2], c.version_distance_histogram[3],
                  c.version_distance_histogram[4], c.version_distance_histogram[5]);
    }
    log::info("");

    log::info("--- Probes ---");
    log::info("Probes: {}", stats.probes.probes);
    log::info("Answered from the active map: {} ({:.2f}%)",
        stats.probes.answered_from_active, stats.probes.active_map_hit_rate * 100);
    log::info("Deferred to historical resolution: {}", stats.probes.deferred);
    log::info("Avg age of answered probes: {:.1f} blocks", stats.probes.avg_age_answered);

    log::info("--- Historical resolution ---");
    log::info("Resolved: {}   absent: {}", stats.resolution.resolved, stats.resolution.absent);
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
            // The setting, deliberately: this field is named for what a new
            // segment is created with. What the open one measures is reported by
            // the fragmentation figures beside it.
            info.file_size_setting = capacity_[i].file_size;
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

    // What this is, said before the numbers rather than left to be inferred.
    //
    // Every figure below except the file size is a counter that started when
    // this process opened the database: the inserts it performed, the entries it
    // is holding, the sizes it was handed. A database opened a moment ago and
    // full of years of data reports almost nothing here, and that is not a
    // defect — it is what a counter is. Reading the files is census(), which says
    // so and charges for it.
    //
    // The file size is not a measurement either: it is the setting a new segment
    // is created with, not what any file occupies.
    log::info("=== UTXO-Z Sizing Report ===");
    log::info("Session counters since this process opened the database, not the");
    log::info("contents of the files. For what is stored, run census().");
    log::info("");

    // Reference mode fills one container and leaves the rest at zero. Printing
    // five would be printing four containers that do not exist, in a report whose
    // whole subject is how much space things take.
    size_t const printed = (mode_ == storage_mode::reference) ? 1 : container_count;
    if (mode_ == storage_mode::reference) {
        log::info("Reference mode: one class of fixed {}-byte records.",
                  sizeof(reference_value));
        log::info("");
    }

    for (size_t i = 0; i < printed; ++i) {
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

        log::info("  Files (catalogued now): {}", c.file_count);
        log::info("  Entries inserted by this process and not yet deleted by it: {:L}",
                  c.current_entries);
        log::info("  Inserts by this process: {:L}", c.historical_inserts);
        log::info("  Deletes by this process: {:L}", c.historical_deletes);
        log::info("  Payload capacity left unused by those inserts: {:L} ({:.2f} bytes/entry avg)",
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

    log::info("--- Sizes of the values this process inserted ({} distinct sizes) ---",
              sorted_histogram.size());
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
    for (auto& per_class : lookup_stats_) per_class.reset();
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


result<> database_impl::reference_open_existing(size_t version) {
    auto const file_name = db_path_ / fmt::format(reference_data_file_format, version);

    auto opened = open_existing_segment(file_name);
    if ( ! opened) return std::unexpected(opened.error());

    if (auto const stamped = validate_stamp(**opened, file_name,
                                            expected_identity(reference_container_kind, version));
        ! stamped) {
        return std::unexpected(stamped.error());
    }

    auto const found = find_single_named<reference_map_t>(**opened, map_object_name, file_name);
    if ( ! found) return std::unexpected(found.error());

    reference_segment_ = std::move(*opened);
    reference_container_ = *found;
    reference_rehash_watch_.reset((*found)->bucket_count());
    reference_current_version_ = version;
    return {};
}

result<> database_impl::reference_create(size_t version) {
    auto const file_name = db_path_ / fmt::format(reference_data_file_format, version);

    std::unique_ptr<bip::managed_mapped_file> segment;
    try {
        segment = std::make_unique<bip::managed_mapped_file>(
            bip::create_only, file_name.c_str(), reference_active_file_size_);
    } catch (std::exception const& e) {
        log::error("reference v{} could not be created: {}", version, e.what());
        return std::unexpected(error_code::identity_collision);
    }

    // From here the file exists and this call is what made it — create_only
    // guarantees that, which is what makes removing it safe. Until the segment
    // is published, any failure or exception takes the file with it: a rotation
    // that failed part-way would otherwise leave the name occupied with nothing
    // usable behind it, and the retry computes the same version number and finds
    // it taken. The container would then have no active version and no way back.
    bool published = false;
    scope_exit const rollback([&] {
        if (published) return;
        segment.reset();   // unmapped before it is unlinked
        std::error_code ec;
        fs::remove(file_name, ec);
        if (ec) {
            log::error("could not withdraw the half-built {}", path_display(file_name));
        }
    });

    if (failpoints::fail_after_segment_create.load(std::memory_order_relaxed)) {
        return std::unexpected(error_code::file_open_failed);
    }

    if (auto const stamped = place_stamp(*segment, file_name,
                                         expected_identity(reference_container_kind, version));
        ! stamped) {
        return std::unexpected(stamped.error());
    }

    if (failpoints::fail_after_segment_stamp.load(std::memory_order_relaxed)) {
        return std::unexpected(error_code::file_open_failed);
    }

    auto* map = segment->construct<reference_map_t>(map_object_name, std::nothrow)(
        capacity_for_reference(),
        outpoint_hash{},
        outpoint_equal{},
        segment->get_allocator<typename reference_map_t::value_type>()
    );
    if (map == nullptr) {
        log::error("reference v{} already holds a map", version);
        return std::unexpected(error_code::identity_collision);
    }

    reference_segment_ = std::move(segment);
    reference_container_ = map;
    reference_rehash_watch_.reset(map->bucket_count());
    reference_current_version_ = version;
    published = true;
    return {};
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
    // See new_version(): a rotation only ever creates.
    if (auto const created = reference_create(next); ! created) {
        throw std::runtime_error(fmt::format("the reference container could not rotate to v{}",
                                             next));
    }

    reference_catalog_.add(next);
    reference_catalog_.metadata(next) = file_metadata{};
    log::debug("Reference container rotated to version {}", reference_current_version_);
}

bool database_impl::reference_can_insert_safely(uint64_t* free_bytes) const {
    // The generator's seam: consumed one rotation at a time, so a fixture can be
    // given more than one generation without the hundred thousand entries a real
    // one needs. Answering here means the ordinary rotation path runs — nothing
    // downstream can tell this apart from a container that filled up.
    if (failpoints::consume_forced_rotation()) return false;

    auto const& map = reference_map();

    // As in full mode: the open map's own bucket count and its own live growth
    // point, in integers, inclusive. Reference is already past 2^24 buckets in
    // production, where a float can no longer represent one exactly.
    if (map.bucket_count() > 0
            && map.size() >= effective_insert_limit(map.bucket_count(), map.max_load())) {
        return false;
    }

    if (reference_segment_) {
        try {
            size_t free_memory = reference_segment_->get_free_memory();
            if (free_bytes) *free_bytes = free_memory;
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
    //
    // The counter is all that happens. The key is not kept: whoever asked keeps
    // it and hands it to resolve() (#116).
    probe_stats_.record_deferred();
    return std::nullopt;
}

std::optional<find_result> database_impl::reference_find_in_latest(raw_outpoint const& key, uint32_t height) const {
    auto const& map = reference_map();
    if (auto it = map.find(key); it != map.end()) {
#if UTXOZ_STATISTICS_LEVEL >= 1
        probe_stats_.record_answered(height, it->second.height);
#endif
#if UTXOZ_STATISTICS_LEVEL >= 2
        lookup_stats_[0].record_answered_from_active();
#endif
        bytes data(sizeof(uint32_t) * 2);
        std::memcpy(data.data(), &it->second.file_number, sizeof(uint32_t));
        std::memcpy(data.data() + sizeof(uint32_t), &it->second.offset, sizeof(uint32_t));
        return find_result{std::move(data), it->second.height};
    }
    return std::nullopt;
}


size_t database_impl::reference_erase_in_latest(raw_outpoint const& key, uint32_t height) {
    auto& map = reference_map();
    if (auto it = map.find(key); it != map.end()) {
#if UTXOZ_STATISTICS_LEVEL >= 1
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
        auto file_name = db_path_ / fmt::format(reference_data_file_format, v);

        auto opened = open_existing_segment(file_name);
        if ( ! opened) return std::unexpected(opened.error());
        auto const found = find_single_named<reference_map_t>(**opened, map_object_name, file_name);
        if ( ! found) return std::unexpected(found.error());

        // The callback is the caller's code and may raise; nothing else in here
        // can any more.
        try {
            for (auto const& [key, _] : **found) {
                cb(ctx, key);
            }
        } catch (std::exception const& e) {
            log::error("reference_for_each_key: the callback raised over reference v{}: {}", v, e.what());
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
        auto file_name = db_path_ / fmt::format(reference_data_file_format, v);

        auto opened = open_existing_segment(file_name);
        if ( ! opened) return std::unexpected(opened.error());
        auto const found = find_single_named<reference_map_t>(**opened, map_object_name, file_name);
        if ( ! found) return std::unexpected(found.error());

        // The callback is the caller's code and may raise; nothing else in here
        // can any more.
        try {
            for (auto const& [key, val] : **found) {
                emit(key, val);
            }
        } catch (std::exception const& e) {
            log::error("reference_for_each_entry: the callback raised over reference v{}: {}", v, e.what());
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
        if (auto const opened = reference_open_existing(active); ! opened) {
            cleanup_pending_ = true;
            return std::unexpected(opened.error());
        }
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
        auto const path = db_path_ / fmt::format("meta_compact_v{:05}.dat", version);
        if (auto const written = write_metadata_file(path, *meta_ptr); ! written) {
            log::warn("Could not publish reference metadata for v{}", version);
        }
    } catch (...) {
    }
}

void database_impl::reference_load_metadata(size_t version) {
    auto const path = db_path_ / fmt::format("meta_compact_v{:05}.dat", version);

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
            log::error("Failed to write config: {}", path_display(temp_path));
            return std::unexpected(error_code::config_file_corrupt);
        }

        auto const encoded = encode_config(local_config(mode_, database_id_));
        ofs.write(reinterpret_cast<char const*>(encoded.data()),
                  static_cast<std::streamsize>(encoded.size()));

        ofs.close();
        if (ofs.fail()) {
            std::error_code cleanup;
            fs::remove(temp_path, cleanup);
            log::error("Failed to write config: {}", path_display(temp_path));
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
        log::error("Could not make the config file durable: {}", path_display(temp_path));
        return std::unexpected(synced.error());
    }

    if (auto const replaced = replace_file_atomically(temp_path, config_path); ! replaced) {
        discard_temp();
        log::error("Could not publish the config file: {}", path_display(config_path));
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
#if UTXOZ_STATISTICS_LEVEL >= 1
                probe_stats_.record_answered(height, it->second.block_height);
#endif
#if UTXOZ_STATISTICS_LEVEL >= 2
                lookup_stats_[I.value].record_answered_from_active();
#endif
                auto data = it->second.get_data();
                result = full_find_result{bytes(data.begin(), data.end()), it->second.block_height};
            }
        }
    });

    if (result) return result;

    // A probe the active map could not answer. Recording it is what makes the
    // hit rate mean something: without it every recorded probe was a hit.
    //
    // The counter is all that happens. The key is not kept: whoever asked keeps
    // it and hands it to resolve() (#116).
    probe_stats_.record_deferred();
    return std::nullopt;
}


result<full_resolution> database_impl::full_resolve(std::span<lookup_request const> requests) const {
    // Taken before anything is read and held to the return, because what has
    // to be protected is not just the cache's bookkeeping but the lifetime of
    // every map reference this call obtains from it: a concurrent eviction
    // unmaps the segment being read (#120).
    std::scoped_lock const resolution_lock(resolve_mutex_);

    if (requests.empty()) return full_resolution{};

    full_resolution resolved;

    // Indices into the caller's batch, shrinking as keys are found so each
    // further file is searched for fewer of them. Nothing is taken: the requests
    // stay in the caller's span and are still the caller's when this returns.
    // That is what makes two batches unable to mix — there is no shared
    // container for them to mix in (#116).
    auto pending = working_set_of<lookup_request>(requests);
    log::debug("Resolving {} full lookups ({} distinct)...", requests.size(), pending.size());

    // A resolution either covers everything it needed to, or it says so.
    //
    // Every version below the current one can hold a requested key, so one that
    // cannot be read makes absence unprovable for *every* key still unresolved
    // — not only for the ones that happened to live in it. Reporting those keys
    // as absent turns a local storage fault into a missing input, and rejects a
    // block that may be perfectly valid.
    //
    // There is no rollback to do on that path any more. The old sweep drained a
    // queue and had to put back what it had consumed; this one consumed nothing,
    // so a failed call simply returns and the caller retries the same span.
#if UTXOZ_STATISTICS_LEVEL >= 1
    // Accumulated here and published only if the resolution completes.
    //
    // A failed resolution is retried, and the retry does all of this work again.
    // Publishing as it goes would count the abandoned attempt as well as the one
    // that finished: every file visited twice, every lookup resolved twice, and a
    // run that never produced a result. Operators read these to decide whether
    // the deferred path is behaving, so an attempt that produced nothing must not
    // appear in them at all.
    //
    // Everything published below lands in resolution_stats_ and nowhere else.
    // deferred_stats belongs to deletions; a lookup writing into it made
    // successfully_processed and failed_to_delete move for two unrelated
    // reasons, so neither number described anything.
#if UTXOZ_STATISTICS_LEVEL >= 2
    // Fixed size and on the stack. The vector this replaces grew with the number
    // of keys resolved, which is an allocation on the read path proportional to
    // the work — and the histogram is what the report wants anyway.
    //
    // Inside the guard with the array that uses it: at `basic` nothing here is
    // reachable, so declaring it there would be a type nobody instantiates.
    struct class_tally {
        uint64_t files_opened = 0;
        uint64_t cache_hits = 0;
        uint64_t generations_probed = 0;   ///< key-against-file probes, not files
        uint64_t resolved = 0;
        uint64_t probe_ordinal_total = 0;
        uint64_t version_distance_total = 0;
        std::array<uint64_t, lookup_stats::bucket_count> ordinal_buckets{};
        std::array<uint64_t, lookup_stats::bucket_count> distance_buckets{};

        void answered(uint64_t probe_ordinal, uint64_t version_distance) {
            ++resolved;
            probe_ordinal_total += probe_ordinal;
            version_distance_total += version_distance;
            ++ordinal_buckets[lookup_stats::bucket_of(probe_ordinal)];
            ++distance_buckets[lookup_stats::bucket_of(version_distance)];
        }
    };
#endif

    struct {
        uint64_t cache_hits = 0;
        uint64_t cache_misses = 0;
        /// What the older resolution counters need, accumulated rather than
        /// collected in a list. This is the part that stays at `basic`: the
        /// `std::vector<uint32_t>` it replaces grew on the read path once per
        /// resolved key, and removing it was worth 47% of the sweep on its own.
        uint64_t resolved = 0;
        uint64_t version_distance_total = 0;
#if UTXOZ_STATISTICS_LEVEL >= 2
        /// How many generation files this resolution has searched so far, across
        /// every class. A key answered by the nth file cost n file probes, which
        /// is what `probe_ordinal` means: the cost, not the version number.
        uint64_t files_probed = 0;
        std::array<class_tally, container_count> per_class{};
#endif
    } tally;
#endif

    bool complete = true;
    // Which failure it was. A file that will not open and a catalogue that cannot
    // be listed are both fail-closed, and they send an operator to different
    // places, so the cause is carried rather than flattened.
    error_code failure = error_code::version_unreadable;

    auto probe_full_file = [&]<size_t Index>(std::integral_constant<size_t, Index>, size_t version) {
        try {
            if (failpoints::fail_historical_open_version.load(std::memory_order_relaxed)
                    == static_cast<uint64_t>(version)) {
                throw std::runtime_error("failpoint: version file refused to open");
            }
            auto [map, cache_hit] = file_cache_->get_or_open_file<Index>(Index, version);

#if UTXOZ_STATISTICS_LEVEL >= 1
            cache_hit ? ++tally.cache_hits : ++tally.cache_misses;
#if UTXOZ_STATISTICS_LEVEL >= 2
            ++tally.files_probed;
            auto& mine = tally.per_class[Index];
            ++mine.files_opened;
            if (cache_hit) ++mine.cache_hits;
            // Every key still pending is about to be looked for in this file, so
            // this is the per-key figure: a thousand keys across three files is
            // three thousand probes, and a key nobody finds was probed by all of
            // them. Added once per file rather than once per key, which is the
            // same number without an atomic in the inner loop.
            mine.generations_probed += pending.size();
#endif
#else
            (void) cache_hit;
#endif

            // Compact in place: what is not found is kept, in order, and the
            // found ones fall off the end. One pass, no allocation, and the
            // caller's span is never written to.
            size_t keep = 0;
            for (size_t i = 0; i < pending.size(); ++i) {
                auto const idx = pending[i];
                auto map_it = map.find(requests[idx].key);
                if (map_it == map.end()) {
                    pending[keep++] = idx;
                    continue;
                }
#if UTXOZ_STATISTICS_LEVEL >= 1
                ++tally.resolved;
                tally.version_distance_total +=
                    static_cast<uint64_t>(current_versions_[Index] - version);
#endif
#if UTXOZ_STATISTICS_LEVEL >= 2
                // Two different numbers, which used to be one. `files_probed` is
                // how many files this key was searched in — its cost. The version
                // distance is how far back the answering generation sits, which
                // is not the same: compaction leaves gaps in the numbering, and
                // the cache is searched before the catalogue, so neither the
                // order nor the arithmetic of the versions describes the search.
                tally.per_class[Index].answered(
                    tally.files_probed,
                    static_cast<uint64_t>(current_versions_[Index] - version));
#endif
                auto data = map_it->second.get_data();
                resolved.found.emplace(requests[idx].key,
                    full_find_result{bytes(data.begin(), data.end()), map_it->second.block_height});
            }
            pending.resize(keep);
        } catch (std::exception const& e) {
            // Not recoverable by carrying on: this file might hold any of the
            // keys still pending, so nothing that remains can be called absent.
            log::error("Could not read full container {} v{}: {}. The resolution is incomplete.",
                       Index, version, e.what());
            complete = false;
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
            if (pending.empty()) break;
            if (container_index == reference_sentinel_index) continue;
            switch (container_index) {
                case 0: probe_full_file(std::integral_constant<size_t, 0>{}, version); break;
                case 1: probe_full_file(std::integral_constant<size_t, 1>{}, version); break;
                case 2: probe_full_file(std::integral_constant<size_t, 2>{}, version); break;
                case 3: probe_full_file(std::integral_constant<size_t, 3>{}, version); break;
                case 4: probe_full_file(std::integral_constant<size_t, 4>{}, version); break;
            }
        }
    }

    // Phase 2: remaining files
    if (!pending.empty()) {
        std::array<std::set<size_t>, container_count> processed_versions;
        for (auto const& [container_index, version] : cached_files) {
            if (container_index < container_count) {
                processed_versions[container_index].insert(version);
            }
        }

        for_each_index<container_count>([&](auto I) {
            if (pending.empty()) return;

            // Enumerating the versions can fail too, and not knowing which files
            // exist is the same problem as not being able to read one.
            try {
            if (failpoints::fail_historical_catalog.load(std::memory_order_relaxed)) {
                throw std::runtime_error("failpoint: catalogue refused to be listed");
            }
            for (auto const v : catalogs_[I.value].below(current_versions_[I.value])) {
                if (pending.empty()) break;
                if (processed_versions[I.value].contains(v)) continue;

                probe_full_file(I, v);
            }
            } catch (std::exception const& e) {
                // Not knowing which files exist is its own failure, and there is
                // already a code that says exactly that. Reporting it as
                // version_unreadable would send somebody looking at a file when
                // the problem is the catalogue.
                log::error("Could not enumerate versions of container {}: {}. The resolution is incomplete.",
                           I.value, e.what());
                complete = false;
                failure = error_code::catalog_unreadable;
            }
        });
    }

    if ( ! complete) {
        // Nothing to put back. The batch was borrowed, not taken, so the caller
        // still holds every request and calls again once the fault is dealt with.
        log::error("Full resolution incomplete: {} of {} requests remain unresolved and none of "
                   "them can be reported as absent", pending.size(), requests.size());
        return std::unexpected(failure);
    }

    // Every version was read, so what is left was looked for everywhere it could
    // have been. Only now is absence a fact.
#if UTXOZ_STATISTICS_LEVEL >= 1
    // The resolution completed, so what it did is now a fact and can be published.
    for (uint64_t i = 0; i < tally.cache_hits; ++i) resolution_stats_.record_file_visited(true);
    for (uint64_t i = 0; i < tally.cache_misses; ++i) resolution_stats_.record_file_visited(false);
    // The counter that existed before, with exactly the meaning it had: a version
    // distance summed over the keys this sweep answered. Handed over as a total
    // rather than replayed key by key from a list the read path no longer keeps —
    // which is the part of this work that stays at `basic`.
    if (tally.resolved > 0) {
        resolution_stats_.record_resolved_batch(tally.resolved, tally.version_distance_total);
    }
#if UTXOZ_STATISTICS_LEVEL >= 2
    for (size_t index = 0; index < container_count; ++index) {
        auto const& mine = tally.per_class[index];
        // A class this sweep never touched has nothing to publish. Skipping it
        // is not only tidiness: every one of these is an atomic increment, and a
        // sweep that visited one class was otherwise paying for five.
        if (mine.files_opened == 0 && mine.resolved == 0) continue;

        auto& published = lookup_stats_[index];
        published.record_file_opened(mine.files_opened, mine.cache_hits);
        published.record_generations_probed(mine.generations_probed);
        if (mine.resolved > 0) {
            published.record_resolved_batch(mine.resolved, mine.probe_ordinal_total,
                                            mine.version_distance_total,
                                            mine.ordinal_buckets, mine.distance_buckets);
        }
    }
#endif
#endif

    resolved.absent.reserve(pending.size());
    for (auto const idx : pending) {
        resolved.absent.push_back(requests[idx]);
    }
    // Absence, not "unsettled". Reached only here, on the completed path, where
    // every version that could have held these was read.
    resolution_stats_.record_absent(pending.size());

    log::debug("Full resolution complete: {} found, {} absent",
               resolved.found.size(), resolved.absent.size());

    return resolved;
}

// =============================================================================
// database_impl - Typed reference-mode methods
// =============================================================================

result<> database_impl::reference_rotate_for(rotation_cause cause) {
    try {
        reference_new_version();
    } catch (std::exception const& e) {
        // As in full mode, and for the same reason: the previous container is
        // already closed when this fails.
        ++reference_rotation_causes_.failed;
        integrity_latched_ = error_code::file_open_failed;
        diagnose([&] {
            log::error("the reference container could not rotate ({}): {}. No generation "
                       "was made and this instance will not serve it again; the stored "
                       "data is intact and a new instance can open it",
                       to_string(cause), e.what());
        });
        return std::unexpected(error_code::file_open_failed);
    }
    if (cause == rotation_cause::preventive) ++reference_rotation_causes_.preventive;
    else                                     ++reference_rotation_causes_.capacity_exception;
    return {};
}

result<bool> database_impl::reference_insert_typed(raw_outpoint const& key, uint32_t height,
                                                 uint32_t file_number, uint32_t offset) {
    // As in full mode: the figure comes from the guard, which already read it,
    // and the sentinel distinguishes "not measured" from "nothing left".
    uint64_t free_before = free_bytes_unavailable;
    if ( ! reference_can_insert_safely(&free_before)) {
        // The same hazard as full mode: rotating on a key that is already in the
        // active map would seal the generation holding it and insert a second
        // copy into the empty one. One probe, only on the path that has already
        // decided to rotate.
        if (auto const& current = reference_map(); current.find(key) != current.end()) {
            diagnose([&] {
                log::warn("reference_insert_typed: duplicate key at the rotation "
                          "threshold, so not rotating. generation={} height={} "
                          "outpoint={}",
                          reference_current_version_, height, outpoint_to_string(key));
            });
            return false;
        }
        diagnose([&] {
            log::debug("the reference container reached its insert limit; rotating. "
                       "generation={} size={} buckets={} live_max_load={} "
                       "effective_limit={}",
                       reference_current_version_, reference_map().size(),
                       reference_map().bucket_count(), reference_map().max_load(),
                       effective_insert_limit(reference_map().bucket_count(),
                                              reference_map().max_load()));
        });
        if (auto const rotated = reference_rotate_for(rotation_cause::preventive); ! rotated) {
            return std::unexpected(rotated.error());
        }
        // See insert_in_index: the guard measured the generation just sealed.
        free_before = free_bytes_best_effort(reference_segment_.get());
    }

    reference_value val{};
    val.height = height;
    val.file_number = file_number;
    val.offset = offset;

    // One rotation and one retry; see insert_in_index for why it is not three.
    for (unsigned attempt = 1; attempt <= 2; ++attempt) {
        auto& map = reference_map();
        auto const before = snapshot_of(map);
        auto const segment_size = reference_segment_ ? reference_segment_->get_size() : 0;

        try {
            // The seam that makes the recovery below testable exactly; see
            // failpoints::fail_insert_emplace. One relaxed load on the hot path,
            // and the first comparison ends it.
            if (failpoints::consume_insert_failure()) {
                if (failpoints::fail_insert_after_mutating.load(std::memory_order_relaxed)) {
                    map.emplace(key, val);
                }
                throw bip::bad_alloc();
            }
            auto [it, inserted] = map.emplace(key, val);
            if (!inserted) {
                diagnose([&] {
                    log::warn("reference_insert_typed: duplicate key at height {}, "
                              "outpoint={}", height, outpoint_to_string(key));
                });
            }
            if (inserted) {
                ++entries_count_;

                // As in full mode, and for the same reason.
                bool const rehashed = note_rehash_if_grown(
                    reference_container_kind, reference_rehash_watch_, map.bucket_count());

#if UTXOZ_STATISTICS_LEVEL >= 1
                ++container_stats_[0].total_inserts;
                ++container_stats_[0].current_size;
                ++container_stats_[0].value_size_distribution[sizeof(uint32_t) * 2];
                ++height_range_stats_.ranges[height / height_range_stats::range_size].inserts[0];
                if (rehashed) ++container_stats_[0].rehash_count;
#endif

                reference_catalog_.metadata(reference_current_version_).update_on_insert(key, height);

                if (attempt > 1) {
                    // See insert_in_index: the entry is stored, so the account of
                    // it must not be able to undo the report of it.
                    diagnose([&] {
                        log::info("reference insert retry after capacity rotation "
                                  "succeeded: new_generation={} attempt={}/2 height={} "
                                  "outpoint={} size={} buckets={} live_max_load={}",
                                  reference_current_version_, attempt, height,
                                  outpoint_to_string(key), map.size(), map.bucket_count(),
                                  map.max_load());
                    });
                }
            }
            return inserted;

        } catch (bip::bad_alloc const& e) {
            auto& after_map = reference_map();
            auto const after = snapshot_of(after_map);
            bool const key_present_after = after_map.find(key) != after_map.end();
            auto const free_after = free_bytes_best_effort(reference_segment_.get());
            auto const state = classify_post_exception(before, after, key_present_after);

            // Assembled only from inside `diagnose`; see insert_in_index for
            // what a throw from here used to cost.
            auto const shared = [&] {
                return fmt::format(
                    "container=reference generation={} attempt={}/2 outpoint={} height={} "
                    "payload_size={} size_before={} size_after={} buckets_before={} "
                    "buckets_after={} live_max_load_before={} live_max_load_after={} "
                    "rotation_threshold={} no_rehash_limit={} segment_bytes={} "
                    "free_before={} free_after={} key_present_after={} exception={}",
                    reference_current_version_, attempt, outpoint_to_string(key), height,
                    sizeof(uint32_t) * 2, before.size, after.size, before.bucket_count,
                    after.bucket_count, before.live_max_load, after.live_max_load,
                    max_entries_for(before.bucket_count),
                    max_size_without_rehash(before.bucket_count),
                    segment_size, free_bytes_display(free_before),
                    free_bytes_display(free_after),
                    key_present_after ? "true" : "false", e.what());
            };

            if (state == post_exception_state::map_mutated) {
                ++reference_rotation_causes_.unexpected_post_exception;
                integrity_latched_ = error_code::entry_corrupt;
                diagnose([&] {
                    log::critical("reference insert: the active map changed after a "
                                  "failed allocation, which contradicts the container's "
                                  "guarantee. Not rotating and not retrying. Stop this "
                                  "process and verify or rebuild the database before "
                                  "opening it again; this latch does not survive a "
                                  "restart. {}", shared());
                });
                return std::unexpected(error_code::entry_corrupt);
            }

            if (attempt == 2) {
                diagnose([&] {
                    log::error("reference insert: a newly created generation could not "
                               "take a single entry. This is capacity or configuration, "
                               "not corruption. {}", shared());
                });
                return std::unexpected(error_code::insufficient_space);
            }

            // Captured, not reconstructed; see insert_in_index.
            auto const old_generation = reference_current_version_;
            diagnose([&] {
                log::info("reference insert: allocation failed; map unchanged; rotating "
                          "and retrying. cause=allocator_refused_growth {}", shared());
            });
            if (auto const rotated = reference_rotate_for(rotation_cause::capacity_exception);
                    ! rotated) {
                return std::unexpected(rotated.error());
            }
            free_before = free_bytes_best_effort(reference_segment_.get());
            diagnose([&] {
                log::info("reference insert: rotated after a failed allocation: "
                          "old_generation={} new_generation={} height={} outpoint={}",
                          old_generation, reference_current_version_,
                          height, outpoint_to_string(key));
            });
        }
    }

    diagnose([] {
        log::error("reference insert: exhausted the single retry without a decision");
    });
    return std::unexpected(error_code::insufficient_space);
}

std::optional<reference_find_result> database_impl::reference_find_typed(raw_outpoint const& key, uint32_t height) const {
    auto const& map = reference_map();
    if (auto it = map.find(key); it != map.end()) {
#if UTXOZ_STATISTICS_LEVEL >= 1
        probe_stats_.record_answered(height, it->second.height);
#endif
#if UTXOZ_STATISTICS_LEVEL >= 2
        lookup_stats_[0].record_answered_from_active();
#endif
        return reference_find_result{it->second.height, it->second.file_number, it->second.offset};
    }

    // A probe the active map could not answer. Recording it is what makes the
    // hit rate mean something: without it every recorded probe was a hit.
    //
    // The counter is all that happens. The key is not kept: whoever asked keeps
    // it and hands it to resolve() (#116).
    probe_stats_.record_deferred();
    return std::nullopt;
}

result<reference_resolution> database_impl::reference_resolve(std::span<lookup_request const> requests) const {
    // Taken before anything is read and held to the return, because what has
    // to be protected is not just the cache's bookkeeping but the lifetime of
    // every map reference this call obtains from it: a concurrent eviction
    // unmaps the segment being read (#120).
    std::scoped_lock const resolution_lock(resolve_mutex_);

    if (requests.empty()) return reference_resolution{};

    reference_resolution resolved;

    // The same contract as full_resolve(), case for case: indices into the
    // caller's batch, nothing taken, nothing kept.
    auto pending = working_set_of<lookup_request>(requests);
    log::debug("Resolving {} reference lookups ({} distinct)...", requests.size(), pending.size());

    // A resolution either covers everything it needed to, or it says so. Every
    // version below the current one can hold a requested key, so one that cannot
    // be read makes absence unprovable for every key still unresolved.
#if UTXOZ_STATISTICS_LEVEL >= 1
    // Accumulated here and published only if the resolution completes; a retried
    // attempt would otherwise be counted twice.
    //
    // Everything published below lands in resolution_stats_ and nowhere else.
    // deferred_stats belongs to deletions; a lookup writing into it made
    // successfully_processed and failed_to_delete move for two unrelated
    // reasons, so neither number described anything.
    // The same shape as the full-mode tally, with one class: reference has one,
    // and reporting five with four of them empty would be inventing classes.
    struct {
        uint64_t cache_hits = 0;
        uint64_t cache_misses = 0;
        uint64_t resolved = 0;
        uint64_t version_distance_total = 0;
#if UTXOZ_STATISTICS_LEVEL >= 2
        /// One class, so the count of files probed *is* the count opened and
        /// `cache_hits` is the whole of this class's cache hits. Full mode keeps
        /// them apart because there `files_probed` spans every class and the
        /// per-class figure does not.
        uint64_t files_probed = 0;
        uint64_t generations_probed = 0;
        uint64_t probe_ordinal_total = 0;
        std::array<uint64_t, lookup_stats::bucket_count> ordinal_buckets{};
        std::array<uint64_t, lookup_stats::bucket_count> distance_buckets{};
#endif
    } tally;
#endif

    bool complete = true;
    error_code failure = error_code::version_unreadable;

    auto probe_reference_file = [&](size_t version) {
        try {
            if (failpoints::fail_historical_open_version.load(std::memory_order_relaxed)
                    == static_cast<uint64_t>(version)) {
                throw std::runtime_error("failpoint: version file refused to open");
            }
            auto [map, cache_hit] = file_cache_->get_or_open_reference_file(version);

#if UTXOZ_STATISTICS_LEVEL >= 1
            cache_hit ? ++tally.cache_hits : ++tally.cache_misses;
#if UTXOZ_STATISTICS_LEVEL >= 2
            ++tally.files_probed;
            // Per key, as in full mode: every key still pending is about to be
            // looked for here, and a key nobody finds was probed by every file.
            tally.generations_probed += pending.size();
#endif
#else
            (void) cache_hit;
#endif

            size_t keep = 0;
            for (size_t i = 0; i < pending.size(); ++i) {
                auto const idx = pending[i];
                auto map_it = map.find(requests[idx].key);
                if (map_it == map.end()) {
                    pending[keep++] = idx;
                    continue;
                }
#if UTXOZ_STATISTICS_LEVEL >= 1
                ++tally.resolved;
                tally.version_distance_total +=
                    static_cast<uint64_t>(reference_current_version_ - version);
#endif
#if UTXOZ_STATISTICS_LEVEL >= 2
                {
                    // Cost and age, kept apart. See the full-mode path.
                    uint64_t const ordinal = tally.files_probed;
                    uint64_t const distance = reference_current_version_ - version;
                    tally.probe_ordinal_total += ordinal;
                    ++tally.ordinal_buckets[lookup_stats::bucket_of(ordinal)];
                    ++tally.distance_buckets[lookup_stats::bucket_of(distance)];
                }
#endif
                resolved.found.emplace(requests[idx].key,
                    reference_find_result{map_it->second.height, map_it->second.file_number,
                                          map_it->second.offset});
            }
            pending.resize(keep);
        } catch (std::exception const& e) {
            log::error("Could not read reference v{}: {}. The resolution is incomplete.", version, e.what());
            complete = false;
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
            if (pending.empty()) break;
            if (ci == reference_sentinel_index) {
                probe_reference_file(version);
            }
        }
    }

    // Phase 2: remaining files
    if (!pending.empty()) {
        std::set<size_t> processed_versions;
        for (auto const& [ci, version] : cached_files) {
            if (ci == reference_sentinel_index) {
                processed_versions.insert(version);
            }
        }

        try {
            if (failpoints::fail_historical_catalog.load(std::memory_order_relaxed)) {
                throw std::runtime_error("failpoint: catalogue refused to be listed");
            }
            for (auto const v : reference_catalog_.below(reference_current_version_)) {
                if (pending.empty()) break;
                if (processed_versions.contains(v)) continue;

                probe_reference_file(v);
            }
        } catch (std::exception const& e) {
            log::error("Could not enumerate reference versions: {}. The resolution is incomplete.", e.what());
            complete = false;
            failure = error_code::catalog_unreadable;
        }
    }

    if ( ! complete) {
        // Nothing to put back: the batch was borrowed, not taken.
        log::error("Reference resolution incomplete: {} of {} requests remain unresolved and none "
                   "of them can be reported as absent", pending.size(), requests.size());
        return std::unexpected(failure);
    }

#if UTXOZ_STATISTICS_LEVEL >= 1
    // The resolution completed, so what it did is now a fact and can be published.
    for (uint64_t i = 0; i < tally.cache_hits; ++i) resolution_stats_.record_file_visited(true);
    for (uint64_t i = 0; i < tally.cache_misses; ++i) resolution_stats_.record_file_visited(false);
    // One class. `lookup_stats_[0]` is where it lives; the report labels it
    // `reference_class` so nobody reads it as container 0.
    // Unchanged in meaning, and kept at `basic`: a version distance summed over
    // the keys this sweep answered.
    if (tally.resolved > 0) {
        resolution_stats_.record_resolved_batch(tally.resolved, tally.version_distance_total);
    }
#if UTXOZ_STATISTICS_LEVEL >= 2
    if (tally.files_probed > 0 || tally.resolved > 0) {
        auto& published = lookup_stats_[0];
        published.record_file_opened(tally.files_probed, tally.cache_hits);
        published.record_generations_probed(tally.generations_probed);
        if (tally.resolved > 0) {
            published.record_resolved_batch(tally.resolved, tally.probe_ordinal_total,
                                            tally.version_distance_total,
                                            tally.ordinal_buckets, tally.distance_buckets);
        }
    }
#endif
#endif

    resolved.absent.reserve(pending.size());
    for (auto const idx : pending) {
        resolved.absent.push_back(requests[idx]);
    }
    // Absence, not "unsettled". Reached only here, on the completed path, where
    // every version that could have held these was read.
    resolution_stats_.record_absent(pending.size());

    log::debug("Reference resolution complete: {} found, {} absent",
               resolved.found.size(), resolved.absent.size());

    return resolved;
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
        auto file_name = db_path_ / fmt::format(reference_data_file_format, v);

        auto opened = open_existing_segment(file_name);
        if ( ! opened) return std::unexpected(opened.error());
        auto const found = find_single_named<reference_map_t>(**opened, map_object_name, file_name);
        if ( ! found) return std::unexpected(found.error());

        // The callback is the caller's code and may raise; nothing else in here
        // can any more.
        try {
            for (auto const& [key, val] : **found) {
                cb(ctx, key, val.height, val.file_number, val.offset);
            }
        } catch (std::exception const& e) {
            log::error("reference_for_each_entry_typed: the callback raised over reference v{}: {}", v, e.what());
            return std::unexpected(error_code::file_open_failed);
        }
    }

    return {};
}

// =============================================================================
// Explicit template instantiations
// =============================================================================

template result<> database_impl::open_existing_container<0>(size_t);
template result<> database_impl::open_existing_container<1>(size_t);
template result<> database_impl::open_existing_container<2>(size_t);
template result<> database_impl::open_existing_container<3>(size_t);
template result<> database_impl::open_existing_container<4>(size_t);
template result<> database_impl::create_container<0>(size_t);
template result<> database_impl::create_container<1>(size_t);
template result<> database_impl::create_container<2>(size_t);
template result<> database_impl::create_container<3>(size_t);
template result<> database_impl::create_container<4>(size_t);

// The const accessor, because src/census.cpp reads the active maps from another
// translation unit. The non-const one is used only here and needs no instance.
template utxo_map<container_sizes[0]> const& database_impl::container<0>() const;
template utxo_map<container_sizes[1]> const& database_impl::container<1>() const;
template utxo_map<container_sizes[2]> const& database_impl::container<2>() const;
template utxo_map<container_sizes[3]> const& database_impl::container<3>() const;
template utxo_map<container_sizes[4]> const& database_impl::container<4>() const;

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

template result<bool> database_impl::insert_in_index<0>(raw_outpoint const&, output_data_span, uint32_t);
template result<> database_impl::rotate_for<0>(rotation_cause);
template result<bool> database_impl::insert_in_index<1>(raw_outpoint const&, output_data_span, uint32_t);
template result<> database_impl::rotate_for<1>(rotation_cause);
template result<bool> database_impl::insert_in_index<2>(raw_outpoint const&, output_data_span, uint32_t);
template result<> database_impl::rotate_for<2>(rotation_cause);
template result<bool> database_impl::insert_in_index<3>(raw_outpoint const&, output_data_span, uint32_t);
template result<> database_impl::rotate_for<3>(rotation_cause);
template result<bool> database_impl::insert_in_index<4>(raw_outpoint const&, output_data_span, uint32_t);
template result<> database_impl::rotate_for<4>(rotation_cause);

} // namespace utxoz::detail
