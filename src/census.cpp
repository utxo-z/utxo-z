// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file census.cpp
 * @brief The walk, and the two presentations of what it found.
 *
 * One structure is filled by one walk, and both the JSON and the text are
 * written from it. Nothing is formatted twice: a figure that appears in the text
 * and not in the JSON would be a figure nobody could compare between runs, and a
 * second assembly of the same numbers is where the two come to disagree.
 */

#include <utxoz/census.hpp>

#include "detail/capacity_policy.hpp"
#include "detail/census_arithmetic.hpp"
#include "detail/database_impl.hpp"
#include "detail/format_identity.hpp"
#include "detail/log.hpp"
#include "detail/path_display.hpp"
#include "detail/physical_size.hpp"
#include "detail/segment_open.hpp"
#include "detail/segment_stamp.hpp"

#include <utxoz/config.hpp>

#include <algorithm>
#include <chrono>
#include <limits>
#include <optional>
#include <utility>
#include <cstdint>
#include <string>
#include <vector>

#include <fmt/format.h>

namespace utxoz {

namespace {

/// The residual, and the refusal to invent one.
///
/// `allocated − modelled` is a subtraction, so it is where every modelling error
/// ends up. When the modelled parts come to more than was allocated the model is
/// wrong, and the honest report of that is no number at all plus a flag — not a
/// clamp to zero, which would read as a tight fit.
void set_residual(uint64_t segment_size, uint64_t segment_free, uint64_t modelled,
                  optional_bytes& residual, bool& inconsistent) {
    if (segment_free > segment_size) {
        inconsistent = true;
        residual = {0, metric_status::unavailable,
                    "the segment reports more free bytes than it has"};
        return;
    }
    uint64_t const allocated = segment_size - segment_free;
    if (modelled > allocated) {
        inconsistent = true;
        residual = {0, metric_status::unavailable,
                    "the modelled slot and metadata bytes came to more than the "
                    "allocator handed out, so the model does not hold here"};
        return;
    }
    residual = {allocated - modelled, metric_status::measured,
                "residual: allocated bytes minus the modelled slot and group metadata"};
}

/// Adding two figures that may not exist. Absent plus present is absent: a sum
/// that quietly skipped the part nobody could measure would be a smaller number
/// wearing the name of a complete one.
void add_optional(optional_bytes& total, optional_bytes const& part, char const* what) {
    if (total.status == metric_status::unavailable) return;
    if (part.status == metric_status::measured
            && (total.status == metric_status::measured || total.bytes == 0)) {
        total.bytes += part.bytes;
        total.status = metric_status::measured;
        total.detail = what;
        return;
    }
    if (part.status == metric_status::not_applicable
            && total.status == metric_status::not_applicable) {
        return;
    }
    total = {0, part.status,
             part.status == metric_status::not_applicable
                 ? std::string("does not apply in this mode")
                 : std::string("at least one part could not be measured: ") + part.detail};
}

} // namespace

char const* to_string(metric_status s) noexcept {
    switch (s) {
        case metric_status::measured: return "measured";
        case metric_status::not_applicable: return "not_applicable";
        case metric_status::unavailable: return "unavailable";
    }
    return "unavailable";
}

char const* to_string(snapshot_consistency c) noexcept {
    switch (c) {
        case snapshot_consistency::live_database_exclusive: return "live_database_exclusive";
        case snapshot_consistency::not_verified: return "not_verified";
    }
    return "not_verified";
}

char const* to_string(allocation_method m) noexcept {
    switch (m) {
        case allocation_method::none: return "none";
        case allocation_method::posix_st_blocks: return "posix_st_blocks";
        case allocation_method::windows_compressed_size: return "windows_compressed_size";
    }
    return "none";
}

namespace detail {

namespace {

/// Which class has this size. A fold rather than a table, so it cannot fall out
/// of step with `container_sizes`.
consteval size_t index_of_size(size_t size) {
    for (size_t i = 0; i < container_sizes.size(); ++i) {
        if (container_sizes[i] == size) return i;
    }
    throw "no container has this size";
}

/// One generation of one full-mode class: every entry read, nothing sampled.
template <size_t Size>
result<> accumulate_full(uint64_t container_class, utxo_map<Size> const& map,
                         generation_census& gen, std::vector<uint64_t>& histogram) {
    constexpr size_t capacity = utxo_value<Size>{}.data.size();
    // The capacity the geometry publishes and the one the type actually has are
    // the same number, and this is where they would silently stop being it: every
    // unused-capacity figure below is computed from the type, while every
    // consumer reads `payload_capacity` from the report, which comes from the
    // geometry.
    static_assert(container_capacities[index_of_size(Size)] == capacity,
                  "the published payload capacity of this class and the capacity of "
                  "its stored value have parted company");
    // What `sizeof` adds beyond the named fields. Zero for every class in this
    // geometry, and measured rather than assumed so that it stops being zero
    // loudly if a class ever stops being a multiple of the alignment.
    constexpr uint64_t padding = sizeof(utxo_value<Size>)
        - (sizeof(uint32_t) + sizeof(size_type<Size>) + capacity);

    gen.entries = map.size();
    gen.bucket_count = map.bucket_count();

    if (auto ok = validate_generation_counts(container_class, gen.generation,
                                             gen.entries, gen.bucket_count); ! ok) {
        return std::unexpected(ok.error());
    }
    if ( ! checked_mul(gen.entries, padding, gen.object_padding_bytes)) {
        log::error("census: class {} generation {} has an entry count that cannot be "
                   "accounted for without overflow", container_class, gen.generation);
        return std::unexpected(error_code::entry_corrupt);
    }

    uint64_t payload = 0;
    uint64_t unused = 0;
    for (auto const& entry : map) {
        uint64_t const size = entry.second.actual_size;
        // `set_data()` clamps on the way in, so a build that wrote this file
        // could not have produced a longer one. Reading it back and taking the
        // minimum would turn that impossibility into a plausible number — the
        // census would report the entry as full and say nothing. The config and
        // the stamp certify identity and layout; neither certifies that an entry
        // is internally consistent, and this is where that shows.
        //
        // The class, the generation and the offending length are logged. The key
        // and the payload are not: a diagnostic that goes into an issue does not
        // carry the chain.
        if (auto ok = validate_payload_length(container_class, gen.generation,
                                              size, capacity); ! ok) {
            return std::unexpected(ok.error());
        }
        payload += size;              // bounded by entries x capacity, checked below
        unused += capacity - size;
        ++histogram[size];
    }

    // Both sums are bounded by entries x capacity, so one check covers them.
    uint64_t bound = 0;
    if ( ! checked_mul(gen.entries, capacity, bound)) {
        log::error("census: class {} generation {} cannot be summed without overflow",
                   container_class, gen.generation);
        return std::unexpected(error_code::entry_corrupt);
    }

    gen.entry_payload_bytes = payload;
    gen.unused_payload_capacity = {unused, metric_status::measured,
                                   "payload capacity of the class minus what each entry uses"};
    return {};
}

/// The reference map. An entry is a fixed record, so there is no capacity left
/// unused — which is why that figure is `not_applicable` here rather than zero.
result<> accumulate_reference(reference_map_t const& map, generation_census& gen) {
    gen.entries = map.size();
    gen.bucket_count = map.bucket_count();

    if (auto ok = validate_generation_counts(0, gen.generation,
                                             gen.entries, gen.bucket_count); ! ok) {
        return std::unexpected(ok.error());
    }
    // There is no recorded length to disagree with the layout: a reference entry
    // is three fixed fields. What can still be impossible is the count.
    if ( ! checked_mul(gen.entries, sizeof(reference_value), gen.entry_payload_bytes)
            || ! checked_mul(gen.entries, sizeof(reference_value) - 3 * sizeof(uint32_t),
                             gen.object_padding_bytes)) {
        log::error("census: reference generation {} cannot be summed without overflow",
                   gen.generation);
        return std::unexpected(error_code::entry_corrupt);
    }
    gen.unused_payload_capacity = {0, metric_status::not_applicable,
                                   "a reference entry is a fixed record: there is no "
                                   "payload capacity to leave unused"};
    return {};
}

/// The modelled and residual parts, once the exact ones are in.
result<> finish_generation(generation_census& gen, uint64_t pair_size) {
    // Asked again here, and not because the accumulator forgot to: this function
    // subtracts, and a subtraction that wraps produces an enormous number rather
    // than an obviously wrong one. Re-checking is one comparison and removes the
    // need for the reader to prove an ordering between two functions.
    //
    // Refused rather than saturated. Clamping to zero would turn an impossible
    // file into a plausible report — the same silent normalisation this census
    // refuses to do with payload lengths.
    if (auto ok = validate_generation_counts(0, gen.generation,
                                             gen.entries, gen.bucket_count); ! ok) {
        return std::unexpected(ok.error());
    }
    if ( ! checked_mul(gen.entries, pair_size, gen.occupied_slot_bytes)
            || ! checked_mul(gen.bucket_count - gen.entries, pair_size,
                             gen.empty_slot_bytes)) {
        log::error("census: generation {} has a bucket count that cannot be turned "
                   "into a byte figure", gen.generation);
        return std::unexpected(error_code::entry_corrupt);
    }
    if ( ! group_metadata_model(gen.bucket_count, gen.estimated_group_metadata_bytes)) {
        log::error("census: generation {} has a bucket count whose group metadata "
                   "cannot be expressed in bytes", gen.generation);
        return std::unexpected(error_code::entry_corrupt);
    }

    uint64_t modelled = gen.occupied_slot_bytes;
    summing add;
    add(modelled, gen.empty_slot_bytes);
    add(modelled, gen.estimated_group_metadata_bytes);
    if ( ! add.ok) {
        log::error("census: generation {} has modelled byte figures that cannot be "
                   "added", gen.generation);
        return std::unexpected(error_code::entry_corrupt);
    }

    set_residual(gen.segment_size_bytes, gen.segment_free_bytes, modelled,
                 gen.unattributed_allocated_bytes, gen.model_inconsistent);
    return {};
}

[[nodiscard]] bool fold_into_class(class_census& cls, generation_census const& gen) {
    summing add;
    add(cls.entries, gen.entries);
    add(cls.entry_payload_bytes, gen.entry_payload_bytes);
    add(cls.object_padding_bytes, gen.object_padding_bytes);
    add(cls.segment_size_bytes, gen.segment_size_bytes);
    add(cls.segment_free_bytes, gen.segment_free_bytes);
    add(cls.logical_file_bytes, gen.logical_file_bytes);
    add(cls.occupied_slot_bytes, gen.occupied_slot_bytes);
    add(cls.empty_slot_bytes, gen.empty_slot_bytes);
    add(cls.estimated_group_metadata_bytes, gen.estimated_group_metadata_bytes);
    if ( ! add.ok) return false;
    add_optional(cls.unused_payload_capacity, gen.unused_payload_capacity,
                 "summed over the generations of this class");
    add_optional(cls.unattributed_allocated_bytes, gen.unattributed_allocated_bytes,
                 "summed over the generations of this class");
    add_optional(cls.physical_allocated_bytes, gen.physical_allocated_bytes,
                 "summed over the generations of this class");
    return true;
}

[[nodiscard]] bool fold_into_report(census_report& report, class_census const& cls) {
    summing add;
    add(report.entries, cls.entries);
    add(report.entry_payload_bytes, cls.entry_payload_bytes);
    add(report.object_padding_bytes, cls.object_padding_bytes);
    add(report.segment_size_bytes, cls.segment_size_bytes);
    add(report.segment_free_bytes, cls.segment_free_bytes);
    add(report.logical_file_bytes, cls.logical_file_bytes);
    add(report.occupied_slot_bytes, cls.occupied_slot_bytes);
    add(report.empty_slot_bytes, cls.empty_slot_bytes);
    add(report.estimated_group_metadata_bytes, cls.estimated_group_metadata_bytes);
    if ( ! add.ok) return false;
    add_optional(report.unused_payload_capacity, cls.unused_payload_capacity,
                 "summed over the classes");
    add_optional(report.unattributed_allocated_bytes, cls.unattributed_allocated_bytes,
                 "summed over the classes");
    add_optional(report.physical_allocated_bytes, cls.physical_allocated_bytes,
                 "summed over the classes");
    return true;
}

/// Sizes that occur, ascending. The dense counter array is an implementation
/// detail: what leaves is only what was seen, so a class where every payload is
/// eight bytes reports one bucket rather than forty-four.
std::vector<payload_bucket> compact_histogram(std::vector<uint64_t> const& dense) {
    std::vector<payload_bucket> out;
    for (size_t i = 0; i < dense.size(); ++i) {
        if (dense[i] != 0) out.push_back({static_cast<uint32_t>(i), dense[i]});
    }
    return out;
}

} // namespace

result<census_report> database_impl::census(census_options const& options) const {
    auto const started = std::chrono::steady_clock::now();

    census_report report;
    report.scope = "physical_stored";
    report.storage_mode = (mode_ == storage_mode::reference) ? "reference" : "full";
    report.geometry_id = geometry_id;
    report.map_layout_epoch = map_layout_epoch;
    report.hash_epoch = hash_epoch;
    report.platform_abi_id = platform_abi_id;
#ifdef UTXOZ_STATISTICS_ENABLED
    report.statistics_enabled = true;
#else
    report.statistics_enabled = false;
#endif
    report.physical_measurement = options.measure_physical_blocks
        ? physical_allocation_method() : allocation_method::none;
    report.source.declared_external_snapshot = options.declared_external_snapshot;
    report.source.consistency = options.declared_external_snapshot
        ? snapshot_consistency::not_verified
        : snapshot_consistency::live_database_exclusive;

    // The metrics that are sums of optional parts start out as measured zeros,
    // so that a database with no generations at all reports zero rather than
    // "could not be measured".
    report.unused_payload_capacity = {0, metric_status::measured, "summed over the classes"};
    report.unattributed_allocated_bytes = {0, metric_status::measured, "summed over the classes"};
    report.physical_allocated_bytes = options.measure_physical_blocks
        ? optional_bytes{0, metric_status::measured, "summed over the classes"}
        : optional_bytes{0, metric_status::not_applicable, "not requested"};

    // Everything below shares this: the file for a generation the catalogue
    // lists, its size, its blocks, and the segment it holds. A generation that
    // will not open aborts the census. A partial census is a census that would
    // be read as complete.
    auto const file_facts = [&](fs::path const& path, generation_census& gen) -> result<> {
        std::error_code ec;
        auto const size = fs::file_size(path, ec);
        if (ec) {
            log::error("census: {} could not be sized: {}", path_display(path), ec.message());
            return std::unexpected(error_code::file_open_failed);
        }
        gen.logical_file_bytes = size;
        gen.physical_allocated_bytes = options.measure_physical_blocks
            ? physical_allocation_of(path)
            : optional_bytes{0, metric_status::not_applicable, "not requested"};
        return {};
    };

    if (mode_ == storage_mode::reference) {
        class_census cls;
        cls.container_class = 0;
        cls.container_size = 0;   // not a size class; see census.hpp
        cls.payload_capacity = 0;
        cls.value_size = sizeof(reference_value);
        cls.pair_size = sizeof(reference_map_t::value_type);
        cls.active_generation = reference_current_version_;
        cls.payload_histogram_status = metric_status::not_applicable;
        cls.unused_payload_capacity = {0, metric_status::not_applicable,
                                       "a reference entry is a fixed record"};
        cls.unattributed_allocated_bytes = {0, metric_status::measured,
                                            "summed over the generations of this class"};
        cls.physical_allocated_bytes = options.measure_physical_blocks
            ? optional_bytes{0, metric_status::measured, "summed over the generations of this class"}
            : optional_bytes{0, metric_status::not_applicable, "not requested"};

        for (auto const version : reference_catalog_.versions()) {
            generation_census gen;
            gen.generation = version;
            gen.active = (version == reference_current_version_);
            auto const path = data_path(reference_sentinel_index, version);
            if (auto ok = file_facts(path, gen); ! ok) return std::unexpected(ok.error());

            if (gen.active && reference_container_ != nullptr && reference_segment_) {
                if (auto ok = accumulate_reference(reference_map(), gen); ! ok) {
                    return std::unexpected(ok.error());
                }
                gen.segment_size_bytes = reference_segment_->get_size();
                gen.segment_free_bytes = reference_segment_->get_free_memory();
            } else {
                auto opened = open_existing_segment(path);
                if ( ! opened) return std::unexpected(opened.error());
                // The same check the ordinary open makes, for the same reason: a
                // mapped segment is not yet a segment of *this* database, and a
                // file renamed into a name that is not its own would otherwise be
                // counted as the generation it is pretending to be.
                if (auto stamped = validate_stamp(
                        **opened, path,
                        expected_identity(reference_container_kind, version)); ! stamped) {
                    return std::unexpected(stamped.error());
                }
                auto found = find_single_named<reference_map_t>(**opened, map_object_name, path);
                if ( ! found) return std::unexpected(found.error());
                if (auto ok = accumulate_reference(**found, gen); ! ok) {
                    return std::unexpected(ok.error());
                }
                gen.segment_size_bytes = (*opened)->get_size();
                gen.segment_free_bytes = (*opened)->get_free_memory();
            }

            if (auto ok = finish_generation(gen, cls.pair_size); ! ok) {
                return std::unexpected(ok.error());
            }
            ++report.files_examined;
            ++cls.generations;
            if ( ! checked_add(report.entries_examined, gen.entries)
                    || ! fold_into_class(cls, gen)) {
                return std::unexpected(error_code::entry_corrupt);
            }
            if (options.per_generation_detail) cls.generations_detail.push_back(gen);
        }

        if ( ! fold_into_report(report, cls)) {
            return std::unexpected(error_code::entry_corrupt);
        }
        report.classes.push_back(std::move(cls));
    } else {
        // The classes are distinct types, so they are visited by a fold rather
        // than by a loop. `for_each_index` would do, but it is defined in
        // database_impl.cpp and so cannot be called from here.
        std::optional<error_code> failure;
        auto const walk_class = [&]<size_t Index>(std::integral_constant<size_t, Index>) {
            if (failure) return;
            constexpr size_t Size = container_sizes[Index];

            class_census cls;
            cls.container_class = Index;
            cls.container_size = Size;
            cls.payload_capacity = container_capacities[Index];
            cls.value_size = sizeof(utxo_value<Size>);
            cls.pair_size = sizeof(typename utxo_map<Size>::value_type);
            cls.active_generation = current_versions_[Index];
            cls.unused_payload_capacity = {0, metric_status::measured,
                                           "summed over the generations of this class"};
            cls.unattributed_allocated_bytes = {0, metric_status::measured,
                                                "summed over the generations of this class"};
            cls.physical_allocated_bytes = options.measure_physical_blocks
                ? optional_bytes{0, metric_status::measured,
                                 "summed over the generations of this class"}
                : optional_bytes{0, metric_status::not_applicable, "not requested"};

            std::vector<uint64_t> histogram(cls.payload_capacity + 1, 0);

            for (auto const version : catalogs_[Index].versions()) {
                generation_census gen;
                gen.generation = version;
                gen.active = (version == current_versions_[Index]);
                auto const path = data_path(Index, version);
                if (auto ok = file_facts(path, gen); ! ok) { failure = ok.error(); return; }

                if (gen.active && segments_[Index]) {
                    auto ok = accumulate_full<Size>(Index, container<Index>(), gen, histogram);
                    if ( ! ok) { failure = ok.error(); return; }
                    gen.segment_size_bytes = segments_[Index]->get_size();
                    gen.segment_free_bytes = segments_[Index]->get_free_memory();
                } else {
                    auto opened = open_existing_segment(path);
                    if ( ! opened) { failure = opened.error(); return; }
                    // As above: mapped is not the same as ours, and a census that
                    // counted a misplaced file would report a database that does
                    // not exist.
                    if (auto stamped = validate_stamp(
                            **opened, path,
                            expected_identity(uint32_t(Index), version)); ! stamped) {
                        failure = stamped.error(); return;
                    }
                    auto found = find_single_named<utxo_map<Size>>(**opened, map_object_name, path);
                    if ( ! found) { failure = found.error(); return; }
                    auto ok = accumulate_full<Size>(Index, **found, gen, histogram);
                    if ( ! ok) { failure = ok.error(); return; }
                    gen.segment_size_bytes = (*opened)->get_size();
                    gen.segment_free_bytes = (*opened)->get_free_memory();
                }

                if (auto ok = finish_generation(gen, cls.pair_size); ! ok) {
                    failure = ok.error(); return;
                }
                ++report.files_examined;
                ++cls.generations;
                if ( ! checked_add(report.entries_examined, gen.entries)
                        || ! fold_into_class(cls, gen)) {
                    failure = error_code::entry_corrupt; return;
                }
                if (options.per_generation_detail) cls.generations_detail.push_back(gen);
            }

            cls.payload_histogram = compact_histogram(histogram);
            if ( ! fold_into_report(report, cls)) { failure = error_code::entry_corrupt; return; }
            report.classes.push_back(std::move(cls));
        };
        [&]<size_t... Is>(std::index_sequence<Is...>) {
            (walk_class(std::integral_constant<size_t, Is>{}), ...);
        }(std::make_index_sequence<container_count>{});
        if (failure) return std::unexpected(*failure);
    }

    report.duration_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count());
    return report;
}

} // namespace detail

// =============================================================================
// The two presentations
// =============================================================================
//
// Both are written from the report and neither recomputes anything. A figure a
// person reads and a figure a script compares have to be the same figure, and
// the way to guarantee that is to have one source for both.

namespace {

/// Escaped for JSON. The strings here are ours, but a detail that ever carries a
/// path would carry a backslash on Windows, and a report that stopped parsing at
/// that point would be a bug found by whoever least expected it.
std::string json_string(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 2);
    out.push_back('"');
    for (char const c : text) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out += fmt::format("\\u{:04x}", static_cast<unsigned char>(c));
                } else {
                    out.push_back(c);
                }
        }
    }
    out.push_back('"');
    return out;
}

/// A figure that may not exist, written so that absent and zero cannot be
/// confused: the number is `null` unless it was measured, and the status says
/// which kind of absence it is.
std::string json_optional(optional_bytes const& v) {
    return fmt::format(R"({{"status": {}, "bytes": {}, "detail": {}}})",
                       json_string(to_string(v.status)),
                       v.status == metric_status::measured ? fmt::format("{}", v.bytes)
                                                           : std::string("null"),
                       json_string(v.detail));
}

/// The byte decomposition of one generation or one class, with each figure under
/// the kind of number it is. The grouping is the point: `exact` was read,
/// `modelled` was computed from the certified layout, `residual` is what the
/// subtraction left, and adding across the groups is the reader's decision to
/// make knowingly.
template <typename T>
std::string json_bytes(T const& x, bool include_payload_capacity = true) {
    std::string out = "\"exact\": {";
    out += fmt::format(R"("entry_payload_bytes": {}, )", x.entry_payload_bytes);
    if (include_payload_capacity) {
        out += fmt::format(R"("unused_payload_capacity": {}, )",
                           json_optional(x.unused_payload_capacity));
    }
    out += fmt::format(R"("object_padding_bytes": {}, )", x.object_padding_bytes);
    out += fmt::format(R"("segment_size_bytes": {}, )", x.segment_size_bytes);
    out += fmt::format(R"("segment_free_bytes": {}, )", x.segment_free_bytes);
    out += fmt::format(R"("logical_file_bytes": {}}}, )", x.logical_file_bytes);

    out += "\"modelled\": {";
    out += fmt::format(R"("occupied_slot_bytes": {}, )", x.occupied_slot_bytes);
    out += fmt::format(R"("empty_slot_bytes": {}, )", x.empty_slot_bytes);
    out += fmt::format(R"("estimated_group_metadata_bytes": {}}}, )",
                       x.estimated_group_metadata_bytes);

    out += fmt::format(R"("residual": {{"unattributed_allocated_bytes": {}}}, )",
                       json_optional(x.unattributed_allocated_bytes));
    out += fmt::format(R"("platform": {{"physical_allocated_bytes": {}}})",
                       json_optional(x.physical_allocated_bytes));
    return out;
}

} // namespace

std::string to_json(census_report const& r) {
    std::string out;
    out += "{\n";
    out += fmt::format("  \"report_schema_version\": {},\n", census_report::schema_version);
    out += fmt::format("  \"scope\": {},\n", json_string(r.scope));
    out += fmt::format("  \"storage_mode\": {},\n", json_string(r.storage_mode));
    out += fmt::format("  \"format_identity\": {{\"geometry_id\": {}, \"map_layout_epoch\": {}, "
                       "\"hash_epoch\": {}, \"platform_abi_id\": {}}},\n",
                       r.geometry_id, r.map_layout_epoch, r.hash_epoch, r.platform_abi_id);
    out += fmt::format("  \"statistics_enabled\": {},\n", r.statistics_enabled ? "true" : "false");
    out += fmt::format("  \"source\": {{\"declared_external_snapshot\": {}, \"consistency\": {}, "
                       "\"note\": \"declared by the caller; this program cannot verify how a "
                       "copy was taken\"}},\n",
                       r.source.declared_external_snapshot ? "true" : "false",
                       json_string(to_string(r.source.consistency)));
    out += fmt::format("  \"walk\": {{\"duration_ms\": {}, \"files_examined\": {}, "
                       "\"entries_examined\": {}, \"physical_measurement_method\": {}}},\n",
                       r.duration_ms, r.files_examined, r.entries_examined,
                       json_string(to_string(r.physical_measurement)));

    out += fmt::format("  \"totals\": {{\"entries\": {}, {}}},\n", r.entries, json_bytes(r));

    out += "  \"classes\": [\n";
    for (size_t i = 0; i < r.classes.size(); ++i) {
        auto const& c = r.classes[i];
        out += "    {";
        out += fmt::format(R"("container_class": {}, "container_size": {}, )",
                           c.container_class, c.container_size);
        out += fmt::format(R"("payload_capacity": {}, "value_size": {}, "pair_size": {}, )",
                           c.payload_capacity, c.value_size, c.pair_size);
        out += fmt::format(R"("entries": {}, "generations": {}, "active_generation": {}, )",
                           c.entries, c.generations, c.active_generation);
        out += json_bytes(c);
        out += ",\n     \"payload_histogram\": {";
        out += fmt::format(R"("status": {}, )", json_string(to_string(c.payload_histogram_status)));
        if (c.payload_histogram_status == metric_status::measured) {
            out += "\"buckets\": [";
            for (size_t b = 0; b < c.payload_histogram.size(); ++b) {
                out += fmt::format(R"({{"payload_size": {}, "entries": {}}}{})",
                                   c.payload_histogram[b].payload_size,
                                   c.payload_histogram[b].entries,
                                   b + 1 < c.payload_histogram.size() ? ", " : "");
            }
            out += "]}";
        } else {
            out += "\"buckets\": null}";
        }

        out += ",\n     \"generations_detail\": [";
        for (size_t g = 0; g < c.generations_detail.size(); ++g) {
            auto const& d = c.generations_detail[g];
            out += fmt::format(
                "\n      {{\"generation\": {}, \"active\": {}, \"entries\": {}, "
                "\"bucket_count\": {}, \"load_factor\": {:.6f}, \"model_inconsistent\": {}, {}}}{}",
                d.generation, d.active ? "true" : "false", d.entries, d.bucket_count,
                d.load_factor(), d.model_inconsistent ? "true" : "false", json_bytes(d),
                g + 1 < c.generations_detail.size() ? "," : "");
        }
        out += "]}";
        out += (i + 1 < r.classes.size()) ? ",\n" : "\n";
    }
    out += "  ]\n}\n";
    return out;
}

namespace {

std::string human(optional_bytes const& v) {
    if (v.status != metric_status::measured) return to_string(v.status);
    return fmt::format("{}", v.bytes);
}

} // namespace

std::string to_text(census_report const& r) {
    std::string out;
    out += fmt::format("census (schema {}), scope {}, mode {}\n",
                       census_report::schema_version, r.scope, r.storage_mode);
    out += fmt::format("geometry {}  map layout {}  hash {}  platform abi {}  statistics {}\n",
                       r.geometry_id, r.map_layout_epoch, r.hash_epoch, r.platform_abi_id,
                       r.statistics_enabled ? "on" : "off");
    out += fmt::format("walked {} files and {} entries in {} ms; physical allocation by {}\n\n",
                       r.files_examined, r.entries_examined, r.duration_ms,
                       to_string(r.physical_measurement));

    out += "Stored entries, not distinct outpoints: a key present in two places is\n"
           "counted in both. The logical state is a separate walk.\n";
    if (r.source.declared_external_snapshot) {
        out += "This directory was declared to be an external copy. Whether it is a\n"
               "coherent moment of the original depends on how it was taken, which\n"
               "nothing here can check.\n";
    }
    out += "\n";

    for (auto const& c : r.classes) {
        out += fmt::format("class {}  size {}  payload capacity {}  slot {} bytes\n",
                           c.container_class, c.container_size, c.payload_capacity, c.pair_size);
        out += fmt::format("  entries {}  generations {}  active generation {}\n",
                           c.entries, c.generations, c.active_generation);
        out += "  exact:\n";
        out += fmt::format("    payload in entries        {}\n", c.entry_payload_bytes);
        out += fmt::format("    payload capacity unused   {}\n", human(c.unused_payload_capacity));
        out += fmt::format("    object padding            {}\n", c.object_padding_bytes);
        out += fmt::format("    segment size              {}\n", c.segment_size_bytes);
        out += fmt::format("    segment free              {}\n", c.segment_free_bytes);
        out += fmt::format("    logical file size         {}\n", c.logical_file_bytes);
        out += "  modelled from the certified layout:\n";
        out += fmt::format("    occupied slots            {}\n", c.occupied_slot_bytes);
        out += fmt::format("    empty slots               {}\n", c.empty_slot_bytes);
        out += fmt::format("    group metadata            {}\n", c.estimated_group_metadata_bytes);
        out += "  residual:\n";
        out += fmt::format("    unattributed allocated    {}\n",
                           human(c.unattributed_allocated_bytes));
        out += "  platform:\n";
        out += fmt::format("    physically allocated      {}\n",
                           human(c.physical_allocated_bytes));

        if (c.payload_histogram_status == metric_status::measured) {
            out += fmt::format("  payload sizes present: {}\n", c.payload_histogram.size());
            for (auto const& b : c.payload_histogram) {
                out += fmt::format("    {:>6} bytes  {:>12} entries\n", b.payload_size, b.entries);
            }
        } else {
            out += fmt::format("  payload histogram: {}\n", to_string(c.payload_histogram_status));
        }

        for (auto const& g : c.generations_detail) {
            out += fmt::format("  generation {}{}  entries {}  buckets {}  load {:.4f}{}\n",
                               g.generation, g.active ? " (active)" : "", g.entries,
                               g.bucket_count, g.load_factor(),
                               g.model_inconsistent ? "  MODEL INCONSISTENT" : "");
        }
        out += "\n";
    }

    out += fmt::format("totals: entries {}  payload {}  unused capacity {}\n",
                       r.entries, r.entry_payload_bytes, human(r.unused_payload_capacity));
    out += fmt::format("        segment size {}  free {}  logical files {}\n",
                       r.segment_size_bytes, r.segment_free_bytes, r.logical_file_bytes);
    out += fmt::format("        slots occupied {}  empty {}  group metadata {}\n",
                       r.occupied_slot_bytes, r.empty_slot_bytes,
                       r.estimated_group_metadata_bytes);
    out += fmt::format("        unattributed {}  physically allocated {}\n",
                       human(r.unattributed_allocated_bytes), human(r.physical_allocated_bytes));
    return out;
}

} // namespace utxoz
