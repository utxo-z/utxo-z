// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file uniqueness.hpp
 * @brief Does this database hold at most one copy of every outpoint?
 *
 * A correct UTXO set holds each outpoint at most once, across every class and every
 * generation. Nothing in the write path enforces that today (issue #136), so
 * whether a given database satisfies it is a question that has to be asked of
 * the files rather than assumed from the code.
 *
 * ## A verdict, not an error
 *
 * A database with duplicates is not a database this can fail to read. The walk
 * finishes, every figure it reports is exact, and the answer is `unique ==
 * false`. Returning an error would conflate two different things — "the
 * verification could not be completed" and "the verification completed and the
 * answer is no" — and an operator needs to tell them apart, because only the
 * first one is about the instrument.
 *
 * There is deliberately no `error_code` for a duplicate. The tool encodes the
 * distinction in its exit status: 0 unique, 2 not unique, 1 could not tell.
 *
 * ## It does not choose a winner
 *
 * For a key stored more than once with copies that disagree there is no "the
 * value of this key": the store has no precedence policy across classes, and in
 * history which copy answers a lookup can depend on the file cache. Naming a
 * winner would be inventing that policy inside an instrument. So this reports
 * how many copies exist and where they are, and stops exactly there. Repair is
 * a rebuild from the blocks into a new directory, verified and then swapped in
 * — not a choice made here.
 *
 * ## What it costs
 *
 * Counting distinct keys exactly needs every copy of a key to be considered
 * together, and the set of all keys is a set nobody sized. So the data is walked
 * once per partition group, inside a memory budget the walk **enforces** rather
 * than describes — every allocation it makes goes through a meter, and a request
 * past the ceiling is refused before the system is asked for the memory. The
 * report says how many passes that took and how much was held at once.
 *
 * Nothing is written: no temporary directory, no spill files, no residue on any
 * failure path, because nothing is created.
 *
 * The same exclusivity contract as `census()`: the exclusive directory claim and
 * no concurrent mutation. `open_for_inspection()` satisfies it by construction.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <utxoz/aliases.hpp>
#include <utxoz/types.hpp>

namespace utxoz {

/// Where one copy of a duplicated key was found. Enough to go and look at it,
/// and nothing about what is in it — comparing contents is a second read and a
/// different question.
struct duplicate_location {
    uint64_t generation = 0;
    uint32_t container_class = 0;
    bool active = false;          ///< the generation inserts currently go to
};

/**
 * @brief One key that is stored more than once.
 *
 * A sample, not a census of the damage: `uniqueness_report::keys_with_multiple_copies`
 * is the true count, and this carries at most `verify_options::max_findings` of
 * them so that a thoroughly broken database produces a report of bounded size
 * rather than one proportional to the breakage.
 */
struct duplicate_finding {
    uint64_t multiplicity = 0;              ///< how many copies the key has, in full
    std::vector<duplicate_location> locations;
    uint64_t locations_omitted = 0;         ///< multiplicity − locations.size()

    /// Absent unless `verify_options::include_outpoints` was set.
    ///
    /// An outpoint identifies a transaction output on a public chain. It is left
    /// out by default so that a report can be pasted into an issue without a
    /// second thought, and included on request because without it a finding says
    /// that something is wrong and gives nobody a way to go and look at it.
    ///
    /// There is no substitute form. A truncated hash of the outpoint would be
    /// neither private nor useful: it is reversible against the chain and it
    /// cannot be fed back to `find()`.
    std::optional<raw_outpoint> outpoint;
};

/// What the verification is allowed to do.
struct verify_options {
    /// The ceiling the walk holds itself under, and does not exceed.
    ///
    /// Enforced over the part that is proportional to the database: every
    /// allocation the engine makes, and the sample while it is being collected,
    /// is taken against this and one that would not fit is refused instead of
    /// made.
    ///
    /// Two things are outside it, both deliberately and both stated rather than
    /// implied. The memory-mapped segments, which are the operating system's
    /// pages. And **the returned `uniqueness_report` itself**: its `findings` are
    /// public types with the global allocator, so what they occupy is the standard
    /// library's business — bounded by `max_findings` and
    /// `max_locations_per_finding`, reported as `report_bytes_estimate`, and never
    /// growing with the database. Claiming otherwise would mean claiming that
    /// `reserve(n)` allocates exactly n, which the standard does not promise.
    ///
    /// doc/uniqueness.md gives the composition and the measured figures.
    ///
    /// The default is deliberately modest: a diagnostic that takes the machine's
    /// memory is a diagnostic nobody runs on the machine that matters.
    uint64_t memory_budget = 256u * 1024 * 1024;

    /// Each partition group is one more pass over every segment. Refused before
    /// the work rather than discovered at pass forty.
    uint64_t max_data_passes = 64;

    /// Put the outpoints in the findings. Off by default; see
    /// duplicate_finding::outpoint.
    bool include_outpoints = false;

    /// How many duplicated keys the report may describe individually. Small on
    /// purpose: the findings are there to give somebody a place to start, and
    /// the counts above them are what says how big the problem is.
    uint64_t max_findings = 16;

    /// How many copies of one key the report may locate individually. A key with
    /// a million copies would otherwise be a million lines.
    ///
    /// This and `max_findings` bound the sample. It is collected in storage the
    /// walk's own allocator meters, reserved once and never grown, so a sample
    /// `memory_budget` cannot hold is refused before the walk rather than
    /// allocated beside it. The public `findings` built from that storage
    /// afterwards are **outside** the budget; see `report_bytes_estimate`.
    uint64_t max_locations_per_finding = 16;
};

/**
 * @brief What the verification found, and what it cost.
 *
 * Every figure is exact and none of them required choosing between copies.
 */
struct uniqueness_report {
    /// Bumped when a field changes meaning or leaves. New fields do not bump it.
    static constexpr uint32_t schema_version = 1;

    /// The verdict. `distinct_outpoints == physical_entries`.
    bool unique = true;

    std::string storage_mode;                 ///< "full" or "reference"

    uint64_t physical_entries = 0;            ///< copies, counted once each
    uint64_t distinct_outpoints = 0;          ///< keys, however many copies each has
    uint64_t duplicate_copies = 0;            ///< physical_entries − distinct_outpoints
    uint64_t keys_with_multiple_copies = 0;
    uint64_t max_copies_for_one_key = 0;

    /// A bounded sample. Empty when `unique`.
    std::vector<duplicate_finding> findings;
    uint64_t findings_omitted = 0;            ///< keys_with_multiple_copies − findings.size()
    bool outpoints_included = false;          ///< what the options asked for, echoed

    // What the walk was allowed and what it took.
    /// What the walk was allowed. It covers the engine and the sample while it is
    /// being collected, and **not** this object — see `report_bytes_estimate`.
    uint64_t memory_budget = 0;
    uint64_t record_bytes = 0;
    uint64_t record_capacity = 0;
    uint64_t fixed_overhead = 0;
    uint64_t headroom = 0;
    uint64_t estimated_peak = 0;
    uint64_t planning_passes = 0;
    uint64_t data_passes = 0;
    uint64_t partitions = 0;
    uint64_t entries_examined_total = 0;      ///< including every re-read
    /// Generations visited, counting every re-read. Not segment opens: the active
    /// generation of a class is already mapped and is walked without opening
    /// anything, so this is larger than the number of files touched.
    uint64_t generations_visited = 0;
    uint64_t duration_ms = 0;
    std::vector<uint64_t> duration_per_pass_ms;

    /// An estimate of what this report's own storage occupies, computed from the
    /// element counts that were requested. Outside `memory_budget` and outside the
    /// meter that enforces it.
    ///
    /// **Not an upper bound, and deliberately not called one.** `findings` are
    /// public types with the global allocator and `reserve` promises *at least*
    /// what it was asked for, so excess capacity and any transient during a
    /// reallocation are the standard library's business rather than this walk's.
    /// What is bounded is the shape: at most `max_findings` findings of at most
    /// `max_locations_per_finding` locations each, which is why the figure cannot
    /// grow with the database — and why quoting it as a ceiling would be quoting
    /// a promise nobody made.
    uint64_t report_bytes_estimate = 0;
};

/// Machine-readable. Outpoints appear as lowercase hex only when they were asked
/// for; the document says which it is, so a reader never has to infer it from an
/// absence.
[[nodiscard]] std::string to_json(uniqueness_report const&);

/// For a person. Same figures, same order.
[[nodiscard]] std::string to_text(uniqueness_report const&);

} // namespace utxoz
