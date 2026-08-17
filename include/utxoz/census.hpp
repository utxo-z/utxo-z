// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file census.hpp
 * @brief What is actually stored, counted from the files rather than from
 *        counters that started when the process did.
 *
 * The statistics in `statistics.hpp` describe what this process has done since
 * it opened the database: inserts it performed, probes it answered. They are
 * counters, they start at zero, and a database opened a second ago reports
 * almost nothing about the years of data in it.
 *
 * A census is the other question. It walks every generation of every class,
 * reads every entry, and reports what is there. It answers what a geometry
 * decision needs — how large the payloads really are, how much of each slot they
 * use, how much of each segment is table rather than data, how many blocks the
 * filesystem actually gave the file — and it answers it about this database, not
 * about a model of one.
 *
 * ## What this scope does and does not claim
 *
 * `physical_stored` is everything currently materialised: every entry in every
 * generation, counted once per copy. It is **not** the logical state of the
 * store. The two differ when one outpoint is present in more than one place,
 * which nothing in the write path prevents: an insert consults only the active
 * map of the class its payload size selects, so the same outpoint inserted twice
 * with payloads of different sizes lives in two classes at once, and inserted
 * again after a rotation lives in two generations. Reconciling that is a
 * separate, more expensive walk, and it is deliberately not what this reports.
 *
 * So `entries` here is a count of stored entries, not of distinct outpoints.
 *
 * ## Exact, modelled, residual
 *
 * The byte figures are not all the same kind of number and are never added
 * together as though they were:
 *
 *  - **exact** — read from the entries or asked of Boost.Interprocess and the
 *    filesystem: payload sizes, segment size, free bytes, file size.
 *  - **modelled** — computed from the certified map layout: slot bytes from
 *    `bucket_count × sizeof(value_type)`, group metadata from the group size.
 *    They match the layout this build is certified against; they are not
 *    observations of what the allocator did.
 *  - **residual** — what is left of the allocated bytes once the modelled parts
 *    are taken out. It is a subtraction, so it is where every modelling error
 *    lands. It is reported as a residual and never described as the segment
 *    manager's own overhead, which would be a claim about bytes nobody counted.
 *
 * ## Concurrency and exclusivity
 *
 * `census()` is `const` and that is a statement about this object, not about
 * safety. It requires the database to be open with the exclusive directory claim
 * and no concurrent mutation: it reads the active maps in place and opens every
 * historical generation, and an insert running underneath it would be a data
 * race. Run against a stopped node, or against a copy — and if against a copy,
 * say so in whatever consumes the report, because the consistency of that
 * snapshot is a property of how it was copied, which nothing here can see.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <utxoz/types.hpp>

namespace utxoz {

/**
 * @brief Whether a figure was obtained, and if not, why not.
 *
 * A metric nobody could measure is not zero. Zero is an answer; these are the
 * ways of not having one, kept apart so that a report cannot present a missing
 * measurement as a small one.
 */
enum class metric_status : uint8_t {
    measured,        ///< the number is the answer
    not_applicable,  ///< the question does not arise here (reference has no payload sizes)
    unavailable,     ///< it applies, and this platform or this call could not answer it
};

[[nodiscard]] char const* to_string(metric_status) noexcept;

/// A byte count that may not exist. `bytes` is meaningless unless `status` is
/// `measured`, and the JSON writes it as null rather than as 0 in that case.
struct optional_bytes {
    uint64_t bytes = 0;
    metric_status status = metric_status::unavailable;
    /// How it was obtained, or why it could not be — never empty when the status
    /// is not `measured`.
    std::string detail;
};

/// How the physical allocation of a file was measured. The platforms do not
/// measure the same thing, so the method travels with the number.
enum class allocation_method : uint8_t {
    none,                   ///< not measured
    posix_st_blocks,        ///< `st_blocks × 512`: blocks the filesystem allocated
    windows_compressed_size ///< `GetCompressedFileSize`: bytes on disk for sparse/compressed files
};

[[nodiscard]] char const* to_string(allocation_method) noexcept;

/**
 * @brief One generation of one class, as it sits on disk.
 *
 * `entries` and `bucket_count` come from the map itself. The byte figures are
 * grouped by what kind of number they are; see the file comment.
 */
struct generation_census {
    uint64_t generation = 0;
    bool active = false;         ///< the generation inserts currently go to

    // --- exact -------------------------------------------------------------
    uint64_t entries = 0;                    ///< map.size()
    uint64_t bucket_count = 0;               ///< map.bucket_count()
    uint64_t entry_payload_bytes = 0;        ///< full: Σ actual_size. reference: entries × sizeof(reference_value)
    /// full: Σ (capacity − actual_size). `not_applicable` in reference mode,
    /// where an entry is a fixed record and there is no capacity to leave unused
    /// — which is a different statement from leaving none.
    optional_bytes unused_payload_capacity;
    uint64_t object_padding_bytes = 0;       ///< what `sizeof` adds to a class beyond its named fields
    uint64_t segment_size_bytes = 0;         ///< managed_mapped_file::get_size()
    uint64_t segment_free_bytes = 0;         ///< get_free_memory(): never handed out by the allocator
    uint64_t logical_file_bytes = 0;         ///< the size the filesystem reports

    // --- modelled ----------------------------------------------------------
    uint64_t occupied_slot_bytes = 0;        ///< entries × sizeof(value_type)
    uint64_t empty_slot_bytes = 0;           ///< (bucket_count − entries) × sizeof(value_type)
    uint64_t estimated_group_metadata_bytes = 0;

    // --- residual ----------------------------------------------------------
    /// allocated − modelled. Unavailable when the modelled parts exceed what was
    /// allocated, which means the model is wrong rather than that the number is
    /// negative.
    optional_bytes unattributed_allocated_bytes;

    // --- platform ----------------------------------------------------------
    optional_bytes physical_allocated_bytes;

    /// Set when the modelled components came to more than the allocated bytes.
    /// Nothing is silently clamped: the flag says the decomposition does not
    /// hold here and the residual is withheld.
    bool model_inconsistent = false;

    [[nodiscard]] double load_factor() const noexcept {
        return bucket_count == 0 ? 0.0 : double(entries) / double(bucket_count);
    }
};

/// One (payload size, count) pair. Only sizes that occur are carried.
struct payload_bucket {
    uint32_t payload_size = 0;
    uint64_t entries = 0;
};

/**
 * @brief One size class, and every generation of it.
 *
 * In reference mode there is exactly one of these and it is not a size class in
 * the same sense: reference entries are a fixed 12-byte record, so the payload
 * histogram is `not_applicable` rather than empty.
 */
struct class_census {
    uint64_t container_class = 0;
    uint64_t container_size = 0;        ///< the size class in bytes; 0 in reference mode
    uint64_t payload_capacity = 0;      ///< the bytes of payload one entry can hold
    uint64_t value_size = 0;            ///< sizeof(utxo_value<S>) / sizeof(reference_value)
    uint64_t pair_size = 0;             ///< sizeof(map::value_type): what one slot occupies

    uint64_t entries = 0;
    uint64_t generations = 0;
    uint64_t active_generation = 0;

    // Sums over the generations below, in the same three kinds.
    uint64_t entry_payload_bytes = 0;
    optional_bytes unused_payload_capacity;
    uint64_t object_padding_bytes = 0;
    uint64_t segment_size_bytes = 0;
    uint64_t segment_free_bytes = 0;
    uint64_t logical_file_bytes = 0;
    uint64_t occupied_slot_bytes = 0;
    uint64_t empty_slot_bytes = 0;
    uint64_t estimated_group_metadata_bytes = 0;
    optional_bytes unattributed_allocated_bytes;
    optional_bytes physical_allocated_bytes;

    /// Sizes that actually occur, ascending. `not_applicable` in reference mode.
    metric_status payload_histogram_status = metric_status::measured;
    std::vector<payload_bucket> payload_histogram;

    std::vector<generation_census> generations_detail;
};

/**
 * @brief How sure anyone can be that this directory is a coherent moment.
 *
 * Separate from `scope`, and deliberately. `scope` says *what was counted* and is
 * an enumerated value a script can switch on; this says *where the directory came
 * from*, which is a fact about the world that no program here can check. Putting
 * a human caveat inside an enumerated field would make the field unparseable for
 * the sake of a sentence.
 */
enum class snapshot_consistency : uint8_t {
    /// Read from the database itself, under the exclusive directory claim, with
    /// the caller responsible for there being no concurrent mutation.
    live_database_exclusive,
    /// The caller said this is a copy. Whether the copy is a coherent moment of
    /// the original depends on how it was taken, and nothing here can see that.
    not_verified,
};

[[nodiscard]] char const* to_string(snapshot_consistency) noexcept;

/// Where the counted directory came from. `declared_external_snapshot` is a
/// **declaration by the caller**, never an observation: the census cannot tell a
/// copy from an original.
struct census_source {
    bool declared_external_snapshot = false;
    snapshot_consistency consistency = snapshot_consistency::live_database_exclusive;
};

/// What to measure. Kept a struct so that adding a knob does not change every
/// call site — `scope` will arrive here when the logical census does.
struct census_options {
    /// Ask the filesystem how many blocks each file was actually given. One
    /// `stat` per file; off for platforms or filesystems where it is meaningless.
    bool measure_physical_blocks = true;
    /// Include the per-generation detail. The per-class sums are always there.
    bool per_generation_detail = true;
    /// Declare that this directory is a copy taken from somewhere else. Recorded
    /// in the report as a declaration; nothing verifies it and nothing can.
    bool declared_external_snapshot = false;
};

/**
 * @brief The whole answer, versioned so that a consumer can tell what it is
 *        reading.
 */
struct census_report {
    /// Bumped when a field changes meaning or leaves. Consumers compare it.
    static constexpr uint32_t schema_version = 1;

    /// Always `physical_stored` in this version, and only ever one of the
    /// defined values: it is what a consumer switches on. Where the directory
    /// came from is `source`, which is a different question.
    std::string scope = "physical_stored";
    std::string storage_mode;          ///< "full" or "reference"
    census_source source;

    // The identity of the build that read it, so two reports can be told apart.
    uint32_t geometry_id = 0;
    uint32_t map_layout_epoch = 0;
    uint32_t hash_epoch = 0;
    uint32_t platform_abi_id = 0;
    bool statistics_enabled = false;

    uint64_t duration_ms = 0;
    uint64_t files_examined = 0;
    uint64_t entries_examined = 0;
    allocation_method physical_measurement = allocation_method::none;

    std::vector<class_census> classes;

    // Totals, summed from the classes rather than counted a second time.
    uint64_t entries = 0;
    uint64_t entry_payload_bytes = 0;
    optional_bytes unused_payload_capacity;
    uint64_t object_padding_bytes = 0;
    uint64_t segment_size_bytes = 0;
    uint64_t segment_free_bytes = 0;
    uint64_t logical_file_bytes = 0;
    uint64_t occupied_slot_bytes = 0;
    uint64_t empty_slot_bytes = 0;
    uint64_t estimated_group_metadata_bytes = 0;
    optional_bytes unattributed_allocated_bytes;
    optional_bytes physical_allocated_bytes;
};

/// Machine-readable. Deterministic for one state: the same database censused
/// twice produces the same bytes, apart from `duration_ms`, which is stated to
/// be excluded from that promise rather than quietly varying.
[[nodiscard]] std::string to_json(census_report const&);

/// The same structure for a person. Generated from the report, never assembled
/// separately, so the two cannot drift.
[[nodiscard]] std::string to_text(census_report const&);

} // namespace utxoz
