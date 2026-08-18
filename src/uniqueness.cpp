// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file uniqueness.cpp
 * @brief The walk that finds every copy of every outpoint, and the verdict.
 *
 * See detail/distinct_keys.hpp for why there are no temporary files and why the
 * partition hash is defined here rather than borrowed from the map, and
 * uniqueness.hpp for why a duplicate is a verdict rather than an error.
 */

#include <utxoz/uniqueness.hpp>

#include "detail/distinct_keys.hpp"

#include "detail/capacity_policy.hpp"
#include "detail/census_arithmetic.hpp"
#include "detail/database_impl.hpp"
#include "detail/log.hpp"
#include "detail/path_display.hpp"
#include "detail/segment_open.hpp"
#include "detail/segment_stamp.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <cstring>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace utxoz::detail {

namespace {

static_assert(sizeof(stored_copy) == 56,
              "the budget arithmetic reports sizeof(stored_copy); if it changes, the "
              "figure in the report changes with it and nothing here is wrong — but "
              "this assertion is how anybody finds out");

/// The prefix of a hash at a given width, taken from the top bits — where the
/// finaliser in partition_hash() put the dependence on every input byte.
constexpr uint64_t prefix_of(uint64_t hash, size_t bits) noexcept {
    return bits == 0 ? 0 : hash >> (64 - bits);
}

/// What the walk calls reference mode's single class.
///
/// Zero, matching what the physical census reports for reference — not the
/// `reference_class` sentinel the lookup telemetry uses. The two disagree, which
/// is a wart from two features naming the same thing differently; changing
/// either is a change to published output and belongs in its own decision. What
/// matters here is that this agrees with what a census of the same directory
/// calls the same class.
constexpr uint32_t reference_walk_class = 0;

} // namespace

/// Visits every stored entry of every generation, in a fixed order: class by
/// class, generation by generation, ascending. The order does not affect the
/// result — the records are sorted before they are grouped — but a fixed order
/// makes two runs read the same files in the same sequence, which is what makes
/// the pass counts comparable.
template <typename EntryFn>
result<> database_impl::visit_stored_entries(EntryFn&& fn, uint64_t& files_visited) const {
    if (mode_ == storage_mode::reference) {
        for (auto const version : reference_catalog_.versions()) {
            bool const active = (version == reference_current_version_);
            auto const path = data_path(reference_sentinel_index, version);
            ++files_visited;
            if (active && reference_container_ != nullptr && reference_segment_) {
                for (auto const& entry : reference_map()) {
                    fn(entry.first, reference_walk_class, version, active);
                }
                continue;
            }
            auto opened = open_existing_segment(path);
            if ( ! opened) return std::unexpected(opened.error());
            if (auto stamped = validate_stamp(**opened, path,
                    expected_identity(reference_container_kind, version)); ! stamped) {
                return std::unexpected(stamped.error());
            }
            auto found = find_single_named<reference_map_t>(**opened, map_object_name, path);
            if ( ! found) return std::unexpected(found.error());
            for (auto const& entry : **found) {
                fn(entry.first, reference_walk_class, version, active);
            }
        }
        return {};
    }

    std::optional<error_code> failure;
    auto const walk_class = [&]<size_t Index>(std::integral_constant<size_t, Index>) {
        if (failure) return;
        constexpr size_t Size = container_sizes[Index];
        for (auto const version : catalogs_[Index].versions()) {
            bool const active = (version == current_versions_[Index]);
            auto const path = data_path(Index, version);
            ++files_visited;
            if (active && segments_[Index]) {
                for (auto const& entry : container<Index>()) {
                    fn(entry.first, uint32_t(Index), version, active);
                }
                continue;
            }
            auto opened = open_existing_segment(path);
            if ( ! opened) { failure = opened.error(); return; }
            if (auto stamped = validate_stamp(**opened, path,
                    expected_identity(uint32_t(Index), version)); ! stamped) {
                failure = stamped.error(); return;
            }
            auto found = find_single_named<utxo_map<Size>>(**opened, map_object_name, path);
            if ( ! found) { failure = found.error(); return; }
            for (auto const& entry : **found) {
                fn(entry.first, uint32_t(Index), version, active);
            }
        }
    };
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        (walk_class(std::integral_constant<size_t, Is>{}), ...);
    }(std::make_index_sequence<container_count>{});

    if (failure) return std::unexpected(*failure);
    return {};
}

bool fits_in_budget(uint64_t capacity, uint64_t record_bytes, uint64_t fixed_overhead,
                    uint64_t headroom, uint64_t budget, uint64_t& peak) noexcept {
    // On overflow the peak is set to the maximum rather than left as it was. The
    // caller prints it as "this needs a budget of at least N", and a partial sum
    // there would name a figure that is not merely wrong but *small* — an
    // operator would raise the budget to it and be refused again.
    uint64_t reserved = 0;
    if ( ! checked_mul(capacity, record_bytes, reserved)) {
        peak = std::numeric_limits<uint64_t>::max();
        return false;
    }
    peak = reserved;
    if ( ! checked_add(peak, fixed_overhead) || ! checked_add(peak, headroom)) {
        peak = std::numeric_limits<uint64_t>::max();
        return false;
    }
    return peak <= budget;
}

result<budget_report> database_impl::walk_distinct_keys(
        budget_meter& meter,
        distinct_walk_options const& options,
        distinct_key_callback const& on_distinct,
        duplicate_group_callback const& on_group) const {
    auto const started = std::chrono::steady_clock::now();
    budget_report report;
    report.record_bytes = sizeof(stored_copy);
    report.memory_budget = options.memory_budget;
    // A tenth, held back rather than subtracted in a report nobody could act on:
    // the meter's ceiling is the budget without it, so the walk cannot reach it.
    report.headroom = headroom_of(options.memory_budget);

    // The meter arrives already holding whatever the caller took from it — the
    // sample, in the only caller there is. Sharing one meter is what makes the
    // ceiling cover the whole operation instead of the engine alone.
    //
    // The sort's allowance is the one thing still taken as a figure: introsort
    // recurses O(log n) deep and allocates no buffer proportional to the input,
    // but "no allocation at all" is not a portable promise and there is no
    // allocator to route it through. Everything else this walk holds is metered.
    if ( ! meter.acquire(sort_allowance)) {
        log::error("verify: a budget of {} bytes does not cover the {} the sort may need, "
                   "on top of the {} the caller already holds",
                   options.memory_budget, sort_allowance, meter.outstanding());
        return std::unexpected(error_code::insufficient_space);
    }

    // ---------------------------------------------------------------------
    // The planning pass: counters only, no records.
    // ---------------------------------------------------------------------
    // Metered, like everything else. It used to be a plain vector covered by a
    // hand-computed allowance, which is a number somebody has to keep right; the
    // allocator keeps it right for free.
    counted_vector<uint64_t> counts{counted_allocator<uint64_t>(meter)};
    uint64_t physical_entries = 0;
    try {
        counts.assign(planning_prefixes, 0);
    } catch (budget_exceeded const&) {
        log::error("verify: a budget of {} bytes does not hold the {} of planning counters",
                   options.memory_budget, planning_prefixes * sizeof(uint64_t));
        return std::unexpected(error_code::insufficient_space);
    }
    {
        auto const pass_started = std::chrono::steady_clock::now();
        uint64_t files = 0;
        auto const counted = visit_stored_entries(
            [&](raw_outpoint const& key, uint32_t, uint64_t, bool) {
                ++counts[prefix_of(partition_hash(key), planning_prefix_bits)];
                ++physical_entries;
            }, files);
        if ( ! counted) return std::unexpected(counted.error());
        report.planning_passes = 1;
        report.generations_visited += files;
        report.entries_examined_total += physical_entries;
        report.duration_per_pass_ms.push_back(uint64_t(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - pass_started).count()));
    }

    // ---------------------------------------------------------------------
    // Grouping, from the counts just taken rather than from an estimate.
    // ---------------------------------------------------------------------
    //
    // Reserved to its exact worst case — one group per prefix — so the vector
    // never grows and no reallocation ever holds two blocks at once. That
    // transient is what a fixed allowance could not describe: the worst case came
    // to 196 608 bytes against an allowance of 196 608, which held by luck and by
    // one allocator's growth factor. Reserving the maximum costs 96 KiB always,
    // which is half of what the allowance set aside and is now a figure the meter
    // knows rather than one somebody has to keep right.
    counted_vector<partition_group> groups{counted_allocator<partition_group>(meter)};
    try {
        groups.reserve(planning_prefixes);
    } catch (budget_exceeded const&) {
        log::error("verify: a budget of {} bytes does not hold the {} a group list needs",
                   options.memory_budget, planning_prefixes * sizeof(partition_group));
        return std::unexpected(error_code::insufficient_space);
    }

    // Everything held that is not records, measured rather than summed. Taken
    // here, with the counters and the group list already allocated, so that the
    // figure is what the meter holds rather than a sum of terms somebody has to
    // keep in step.
    report.fixed_overhead = meter.outstanding();

    uint64_t const records_budget = meter.ceiling() - meter.outstanding();
    uint64_t const records_per_pass = records_budget / report.record_bytes;
    if (records_per_pass == 0) {
        log::error("verify: a budget of {} bytes holds no records at all",
                   options.memory_budget);
        return std::unexpected(error_code::insufficient_space);
    }

    {
        partition_group current{0, 0, 0};
        bool open = false;
        for (uint64_t prefix = 0; prefix < planning_prefixes; ++prefix) {
            uint64_t const here = counts[size_t(prefix)];
            if (here == 0 && ! open) continue;
            if (open && current.records + here > records_per_pass) {
                groups.push_back(current);
                open = false;
            }
            if ( ! open) { current = partition_group{prefix, prefix, 0}; open = true; }
            current.last = prefix;
            current.records += here;
        }
        if (open) groups.push_back(current);
    }
    report.partitions = groups.size();

    if (groups.size() > options.max_data_passes) {
        log::error("verify: {} entries at this budget need {} passes over every segment, "
                   "and the limit is {}. Raise the budget or the limit; this refuses "
                   "before doing the work rather than during it",
                   physical_entries, groups.size(), options.max_data_passes);
        return std::unexpected(error_code::insufficient_space);
    }

    // Every group has to fit, and this is where that is settled: **before the
    // first collecting pass**, so a database that cannot be walked at this
    // budget costs the planning pass and nothing more. The meter would refuse
    // the reservation in any case; what this adds is when, and a message that
    // names the budget it would take.
    for (auto const& group : groups) {
        uint64_t needed_peak = 0;
        if (fits_in_budget(group.records, report.record_bytes, report.fixed_overhead,
                           report.headroom, options.memory_budget, needed_peak)) {
            continue;
        }
        // A group of one prefix cannot be divided: every copy of one key hashes
        // alike, and the prefix width is fixed. Saying which case it is matters,
        // because only one of them has an answer other than "a larger budget".
        bool const indivisible = (group.first == group.last);
        log::error("verify: {} entries {}{} and only {} records fit the budget of {} "
                   "bytes. This needs a budget of at least {}",
                   group.records,
                   indivisible ? "share one of the " : "fall in one partition group",
                   indivisible ? fmt::format("{} partition prefixes, which is not "
                                             "subdivided", planning_prefixes)
                               : std::string{},
                   records_per_pass, options.memory_budget, needed_peak);
        return std::unexpected(error_code::insufficient_space);
    }

    // ---------------------------------------------------------------------
    // One pass per group: collect, sort, group.
    // ---------------------------------------------------------------------
    try {
        for (auto const& group : groups) {
            auto const pass_started = std::chrono::steady_clock::now();

            // Constructed fresh for each group, and not cleared and reused: a
            // vector that grew would hold the old block and the new one at once
            // while it moved them, and both are inside the ceiling. Destroying
            // the previous one first gives its bytes back before the next are
            // asked for.
            counted_vector<stored_copy> records{counted_allocator<stored_copy>(meter)};
            // The same narrowing the sample is guarded against, on the count that
            // is actually proportional to the database. `reserve` takes a
            // `size_t`, and on a 32-bit target a group larger than that would be
            // truncated on the way in and reserved at the wrong size. Checked
            // against what this vector can hold as well, because exceeding that
            // is a `length_error` rather than a budget problem.
            if ( ! count_fits_platform(group.records)
                    || group.records > uint64_t(records.max_size())) {
                log::error("verify: a partition group of {} records cannot be held by a "
                           "vector on this platform", group.records);
                return std::unexpected(error_code::insufficient_space);
            }
            records.reserve(size_t(group.records));
            report.record_capacity = std::max<uint64_t>(report.record_capacity,
                                                        records.capacity());

            uint64_t files = 0;
            uint64_t seen = 0;
            auto const collected_ok = visit_stored_entries(
                [&](raw_outpoint const& key, uint32_t klass, uint64_t generation, bool active) {
                    ++seen;
                    uint64_t const prefix = prefix_of(partition_hash(key), planning_prefix_bits);
                    if (prefix < group.first || prefix > group.last) return;
                    records.push_back(stored_copy{generation, key, klass, active});
                }, files);
            if ( ! collected_ok) return std::unexpected(collected_ok.error());

            // The planning pass counted this group; the collecting pass just
            // walked the same files under the same claim, so it has to have found
            // the same number.
            //
            // Nothing can produce a disagreement while the exclusivity contract
            // holds, so no test reaches this and a mutation removing it survives
            // — the same category as the arithmetic guards in
            // census_arithmetic.hpp, and stated for the same reason. It is here
            // because the alternative to refusing is a verdict computed from a
            // collection that does not match what was planned, which would arrive
            // as a wrong count rather than as an error.
            if (records.size() != group.records) {
                log::error("verify: the planning pass counted {} entries in partitions "
                           "{}..{} and the collecting pass found {}",
                           group.records, group.first, group.last, records.size());
                return std::unexpected(error_code::entry_corrupt);
            }

            ++report.data_passes;
            report.generations_visited += files;
            report.entries_examined_total += seen;

            // Sorted by the whole key, then by a fixed order within a key, so that
            // the copies handed to the callback are in the same order every run
            // whatever order the generations were read in.
            std::ranges::sort(records, [](stored_copy const& a, stored_copy const& b) {
                if (auto const c = std::memcmp(a.key.data(), b.key.data(), a.key.size()); c != 0) {
                    return c < 0;
                }
                if (a.container_class != b.container_class) {
                    return a.container_class < b.container_class;
                }
                return a.generation < b.generation;
            });

            for (size_t i = 0; i < records.size(); ) {
                size_t j = i + 1;
                // The hash chose the group; equality is thirty-six bytes. Two keys
                // that collide are two keys.
                while (j < records.size()
                       && std::memcmp(records[j].key.data(), records[i].key.data(),
                                      records[i].key.size()) == 0) {
                    ++j;
                }
                uint64_t const multiplicity = j - i;
                on_distinct(multiplicity);
                if (multiplicity > 1 && on_group) {
                    // A view over storage the meter already holds. Copying the
                    // group would be an allocation proportional to the data, and
                    // `duplicate_group` can only be built from metered storage
                    // so that such a copy could not escape the ceiling.
                    on_group(duplicate_group(records, i, size_t(multiplicity)));
                }
                i = j;
            }

            report.duration_per_pass_ms.push_back(uint64_t(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - pass_started).count()));
        }
    } catch (budget_exceeded const&) {
        // The system was never asked for the memory, so nothing ran out. The
        // pre-pass check above should have caught this; reaching here means the
        // walk tried to hold more than it planned to, and the honest answer is
        // the same refusal rather than a report computed from part of the data.
        log::error("verify: the walk asked for more than the {} bytes it is allowed "
                   "(the meter refused after {} bytes at peak)",
                   options.memory_budget, meter.peak());
        return std::unexpected(error_code::insufficient_space);
    } catch (std::length_error const&) {
        // A reservation beyond what a vector can address at all. Guarded before
        // every reserve, so this is the backstop rather than the path — and it is
        // a refusal like the others, not an exception leaving the library.
        log::error("verify: a reservation exceeded what a vector can hold");
        return std::unexpected(error_code::insufficient_space);
    }

    // The records of every pass have been destroyed by now. If the meter's books
    // do not balance, the peak it reports describes nothing and neither does the
    // ceiling it claims to have held.
    if ( ! meter.balanced()) {
        log::error("verify: the budget meter's accounting did not balance during the walk");
        return std::unexpected(error_code::insufficient_space);
    }

    // Observed, not predicted: the most the meter ever had outstanding, plus the
    // headroom it was never allowed to hand out.
    report.estimated_peak = meter.peak() + report.headroom;
    report.duration_ms = uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started).count());
    return report;
}

result<uniqueness_report> database_impl::verify_unique_outpoints(
        verify_options const& options) const {
    uniqueness_report out;
    out.storage_mode = (mode_ == storage_mode::reference) ? "reference" : "full";
    out.outpoints_included = options.include_outpoints;

    // ---------------------------------------------------------------------
    // The sample, collected in metered storage.
    // ---------------------------------------------------------------------
    //
    // The public `duplicate_finding` cannot be metered — it is a public type with
    // the default allocator, and a vector of vectors besides. An earlier version
    // declared its worst case to the walk and then filled the public vectors
    // directly, which is not the same promise: `push_back` grows geometrically,
    // `reserve` promises at least, and during a reallocation the old block and
    // the new one are both alive. With `max_findings` at 17 a vector ends up
    // holding 32, having briefly held 16 and 32 at once, against 17 accounted
    // for. The report could then say the ceiling was honoured while the operation
    // held more — the exact property the metered allocator exists to provide.
    //
    // So the sample is collected in two flat `counted_vector`s, reserved once
    // each and never grown — those are inside the ceiling. The public shape it is
    // converted into afterwards is not: see report_bytes_estimate, which is an
    // estimate of that storage and says so.
    uint64_t locations_capacity = 0;
    uint64_t sample_bytes = 0;
    uint64_t location_bytes = 0;
    uint64_t report_bytes = 0;
    if ( ! checked_mul(options.max_findings, options.max_locations_per_finding,
                       locations_capacity)
            || ! checked_mul(options.max_findings, sizeof(sampled_finding), sample_bytes)
            || ! checked_mul(locations_capacity, sizeof(duplicate_location), location_bytes)
            || ! checked_mul(options.max_findings, sizeof(duplicate_finding), report_bytes)
            || ! checked_add(report_bytes, location_bytes)) {
        log::error("verify: a sample of {} findings with up to {} locations each is "
                   "more storage than can be counted",
                   options.max_findings, options.max_locations_per_finding);
        return std::unexpected(error_code::insufficient_space);
    }

    // Refused rather than truncated by the casts below. See
    // sample_fits_addressable for why the width is a parameter.
    if ( ! sample_fits_platform(options.max_findings, locations_capacity)) {
        log::error("verify: a sample of {} findings with up to {} locations each does not "
                   "fit this platform's addressable size",
                   options.max_findings, options.max_locations_per_finding);
        return std::unexpected(error_code::insufficient_space);
    }
    out.report_bytes_estimate = report_bytes;

    budget_meter meter(options.memory_budget - headroom_of(options.memory_budget));

    // The metered containers live in this scope and nowhere else, so that the
    // meter's books can be checked after every one of them has been destroyed.
    budget_report walk_result;
    {
        counted_vector<sampled_finding> sample{counted_allocator<sampled_finding>(meter)};
        counted_vector<duplicate_location> sample_locations{
            counted_allocator<duplicate_location>(meter)};
        // Fitting `size_t` is not the same as fitting a vector, and how far apart
        // the two are is the implementation's choice: libstdc++ caps `max_size()`
        // at `PTRDIFF_MAX / sizeof(T)` and libc++ at `SIZE_MAX / sizeof(T)`. On
        // the first that leaves a window of counts — about 1.28e17 of them here —
        // that narrow cleanly, pass the overflow check above, and make `reserve`
        // answer with `length_error`. They come from the command line, not from a
        // damaged database.
        if (options.max_findings > uint64_t(sample.max_size())
                || locations_capacity > uint64_t(sample_locations.max_size())) {
            log::error("verify: a sample of {} findings with up to {} locations each is "
                       "larger than a vector can hold on this platform ({} and {})",
                       options.max_findings, options.max_locations_per_finding,
                       sample.max_size(), sample_locations.max_size());
            return std::unexpected(error_code::insufficient_space);
        }
        try {
            // Reserved exactly once, so nothing grows and no two blocks are ever
            // alive at the same moment. The meter counts what `reserve` actually
            // allocated, not what it was asked for.
            sample.reserve(size_t(options.max_findings));
            sample_locations.reserve(size_t(locations_capacity));
        } catch (budget_exceeded const&) {
            log::error("verify: a budget of {} bytes does not hold a sample of {} findings "
                       "with up to {} locations each", options.memory_budget,
                       options.max_findings, options.max_locations_per_finding);
            return std::unexpected(error_code::insufficient_space);
        } catch (std::length_error const&) {
            // The check above should have caught this. It is here as the backstop,
            // for the same reason the records loop has one: a reservation a vector
            // will not make is a refusal this API owes its caller, not an exception
            // crossing the library boundary.
            log::error("verify: the sample's reservation exceeded what a vector can hold");
            return std::unexpected(error_code::insufficient_space);
        }

        auto const on_distinct = [&](uint64_t multiplicity) {
            ++out.distinct_outpoints;
            out.physical_entries += multiplicity;
            if (multiplicity > 1) {
                ++out.keys_with_multiple_copies;
                out.duplicate_copies += multiplicity - 1;
            }
            out.max_copies_for_one_key = std::max(out.max_copies_for_one_key, multiplicity);
        };

        auto const on_group = [&](duplicate_group const& group) {
            auto const copies = group.copies();
            // Past the cap the group is still counted — by on_distinct, which sees
            // every key — and simply not described. The report says how many it
            // left out, so a truncated sample never reads as a complete one.
            if (sample.size() >= options.max_findings) return;

            auto const kept =
                std::min<uint64_t>(copies.size(), options.max_locations_per_finding);
            sampled_finding entry;
            // The key travels with the group and is read only here. It reaches the
            // report only if the caller asked for it, checked below.
            entry.key = group.key();
            entry.multiplicity = copies.size();
            entry.locations_omitted = copies.size() - kept;
            entry.first_location = sample_locations.size();
            entry.location_count = kept;
            for (uint64_t i = 0; i < kept; ++i) {
                sample_locations.push_back(duplicate_location{copies[i].generation,
                                                              copies[i].container_class,
                                                              copies[i].active});
            }
            sample.push_back(entry);
        };

        distinct_walk_options walk_options;
        walk_options.memory_budget = options.memory_budget;
        walk_options.max_data_passes = options.max_data_passes;

        auto walked = walk_distinct_keys(meter, walk_options, on_distinct, on_group);
        // No partial report. A walk that stopped saw part of the database, and a
        // verdict computed from part of a database is not a verdict.
        if ( ! walked) return std::unexpected(walked.error());
        walk_result = std::move(*walked);

        // The records are gone now, so this is where the sample becomes the report.
        // These are public types with the global allocator and are deliberately
        // **outside** the meter; `report_bytes_estimate` is what says how large
        // they can get. See uniqueness.hpp on what memory_budget does and does not
        // cover.
        out.findings.reserve(sample.size());
        for (auto const& entry : sample) {
            duplicate_finding finding;
            finding.multiplicity = entry.multiplicity;
            finding.locations_omitted = entry.locations_omitted;
            finding.locations.reserve(size_t(entry.location_count));
            for (uint64_t i = 0; i < entry.location_count; ++i) {
                finding.locations.push_back(
                    sample_locations[size_t(entry.first_location + i)]);
            }
            if (options.include_outpoints) finding.outpoint = entry.key;
            out.findings.push_back(std::move(finding));
        }
    }

    // Checked here and not earlier, because a mismatch can happen in the last
    // deallocation of all — the sample's own destructor — and then no acquisition
    // follows it to be refused, so the fail-closed state would never be observed.
    // A meter that lost track of what was held did not honour a ceiling, and there
    // is no verdict to give on that basis.
    if ( ! meter.balanced()) {
        log::error("verify: the budget meter's accounting did not balance, so the ceiling "
                   "it reports cannot be believed and no verdict is given");
        return std::unexpected(error_code::insufficient_space);
    }

    // The verdict, and the only place it is decided. Not `findings.empty()`,
    // which is a fact about the sample.
    out.unique = (out.duplicate_copies == 0);
    out.findings_omitted = out.keys_with_multiple_copies - out.findings.size();

    out.memory_budget = walk_result.memory_budget;
    out.record_bytes = walk_result.record_bytes;
    out.record_capacity = walk_result.record_capacity;
    out.fixed_overhead = walk_result.fixed_overhead;
    out.headroom = walk_result.headroom;
    out.estimated_peak = walk_result.estimated_peak;
    out.planning_passes = walk_result.planning_passes;
    out.data_passes = walk_result.data_passes;
    out.partitions = walk_result.partitions;
    out.entries_examined_total = walk_result.entries_examined_total;
    out.generations_visited = walk_result.generations_visited;
    out.duration_ms = walk_result.duration_ms;
    out.duration_per_pass_ms = walk_result.duration_per_pass_ms;
    // The per-pass durations travel in this report and are allocated with the
    // global allocator like the rest of it, so they belong to its estimate rather
    // than to the meter. Bounded by the number of passes, which the budget bounds.
    out.report_bytes_estimate += out.duration_per_pass_ms.size() * sizeof(uint64_t);
    return out;
}

} // namespace utxoz::detail

namespace utxoz {

namespace {

/// Lowercase hex, thirty-six bytes. Only ever reached under `include_outpoints`.
std::string hex_of(raw_outpoint const& key) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(key.size() * 2);
    for (auto const byte : key) {
        out.push_back(digits[byte >> 4]);
        out.push_back(digits[byte & 0x0F]);
    }
    return out;
}

std::string findings_json(uniqueness_report const& r) {
    std::string out;
    for (size_t i = 0; i < r.findings.size(); ++i) {
        auto const& f = r.findings[i];
        out += fmt::format(R"({{"multiplicity": {}, "locations_omitted": {}, )",
                           f.multiplicity, f.locations_omitted);
        if (f.outpoint) {
            out += fmt::format(R"("outpoint": "{}", )", hex_of(*f.outpoint));
        }
        out += R"("locations": [)";
        for (size_t j = 0; j < f.locations.size(); ++j) {
            auto const& l = f.locations[j];
            out += fmt::format(R"({{"container_class": {}, "generation": {}, "active": {}}}{})",
                               l.container_class, l.generation, l.active ? "true" : "false",
                               j + 1 < f.locations.size() ? ", " : "");
        }
        out += "]}";
        out += (i + 1 < r.findings.size()) ? ",\n      " : "";
    }
    return out;
}

} // namespace

std::string to_json(uniqueness_report const& r) {
    std::string out = "{\n";
    out += fmt::format("  \"report_schema_version\": {},\n", uniqueness_report::schema_version);
    // The verdict first, and spelled as a boolean rather than left to be derived
    // from two counts further down.
    out += fmt::format("  \"unique\": {},\n", r.unique ? "true" : "false");
    out += fmt::format("  \"storage_mode\": \"{}\",\n", r.storage_mode);
    out += fmt::format("  \"counts\": {{\"physical_entries\": {}, \"distinct_outpoints\": {}, "
                       "\"duplicate_copies\": {}, \"keys_with_multiple_copies\": {}, "
                       "\"max_copies_for_one_key\": {}}},\n",
                       r.physical_entries, r.distinct_outpoints, r.duplicate_copies,
                       r.keys_with_multiple_copies, r.max_copies_for_one_key);

    // `outpoints_included` is stated rather than inferred from whether the field
    // is there: a report with no findings and one with the outpoints withheld
    // would otherwise look the same.
    out += fmt::format("  \"findings\": {{\"outpoints_included\": {}, \"reported\": {}, "
                       "\"omitted\": {}, \"report_bytes_estimate\": {},\n"
                       "    \"note\": \"a bounded sample; keys_with_multiple_copies is "
                       "the count, and report_bytes_estimate is outside memory_budget\",\n"
                       "    \"items\": [",
                       r.outpoints_included ? "true" : "false", r.findings.size(),
                       r.findings_omitted, r.report_bytes_estimate);
    if ( ! r.findings.empty()) {
        out += "\n      " + findings_json(r) + "\n    ";
    }
    out += "]},\n";

    out += fmt::format("  \"walk\": {{\"memory_budget\": {}, \"record_bytes\": {}, "
                       "\"record_capacity\": {}, \"fixed_overhead\": {}, \"headroom\": {}, "
                       "\"estimated_peak\": {}, \"planning_passes\": {}, \"data_passes\": {}, "
                       "\"partitions\": {}, \"entries_examined_total\": {}, "
                       "\"generations_visited\": {}, \"duration_ms\": {}, ",
                       r.memory_budget, r.record_bytes, r.record_capacity, r.fixed_overhead,
                       r.headroom, r.estimated_peak, r.planning_passes, r.data_passes,
                       r.partitions, r.entries_examined_total, r.generations_visited,
                       r.duration_ms);
    // Per pass, and not only in total. A small budget re-reads the whole database
    // once per partition group, so where the time went is the thing a reader
    // needs in order to decide whether to raise the budget.
    out += "\"duration_per_pass_ms\": [";
    for (size_t i = 0; i < r.duration_per_pass_ms.size(); ++i) {
        out += fmt::format("{}{}", r.duration_per_pass_ms[i],
                           i + 1 < r.duration_per_pass_ms.size() ? ", " : "");
    }
    out += "]}\n";
    out += "}\n";
    return out;
}

std::string to_text(uniqueness_report const& r) {
    std::string out;
    out += fmt::format("uniqueness: {}\n",
                       r.unique ? "no outpoint is stored more than once"
                                : "SOME OUTPOINTS ARE STORED MORE THAN ONCE");
    out += fmt::format("  storage mode          {}\n", r.storage_mode);
    out += fmt::format("  stored copies         {}\n", r.physical_entries);
    out += fmt::format("  distinct outpoints    {}\n", r.distinct_outpoints);
    out += fmt::format("  extra copies          {}\n", r.duplicate_copies);
    out += fmt::format("  keys with more than one copy  {}\n", r.keys_with_multiple_copies);
    if (r.keys_with_multiple_copies != 0) {
        out += fmt::format("  most copies of one key        {}\n", r.max_copies_for_one_key);
        out += fmt::format("\nfindings ({} shown, {} not shown; outpoints {})\n",
                           r.findings.size(), r.findings_omitted,
                           r.outpoints_included ? "included" : "withheld");
        for (auto const& f : r.findings) {
            out += fmt::format("  a key with {} copies", f.multiplicity);
            if (f.outpoint) out += fmt::format(" [{}]", hex_of(*f.outpoint));
            out += "\n";
            for (auto const& l : f.locations) {
                out += fmt::format("    class {} generation {}{}\n", l.container_class,
                                   l.generation, l.active ? " (active)" : "");
            }
            if (f.locations_omitted != 0) {
                out += fmt::format("    and {} more not shown\n", f.locations_omitted);
            }
        }
    }
    out += fmt::format("\nwalk\n  {} planning + {} data passes over {} partitions\n",
                       r.planning_passes, r.data_passes, r.partitions);
    out += fmt::format("  {} entries examined, {} generation visits, {} ms\n",
                       r.entries_examined_total, r.generations_visited, r.duration_ms);
    out += fmt::format("  budget {} bytes, estimated peak {}\n",
                       r.memory_budget, r.estimated_peak);
    out += fmt::format("  the report itself is outside that budget, estimated {} bytes\n",
                       r.report_bytes_estimate);
    if ( ! r.duration_per_pass_ms.empty()) {
        // The planning pass first, then one per partition group, in the order
        // they ran.
        out += "  per pass:";
        for (auto const ms : r.duration_per_pass_ms) out += fmt::format(" {}ms", ms);
        out += "\n";
    }
    return out;
}

} // namespace utxoz
