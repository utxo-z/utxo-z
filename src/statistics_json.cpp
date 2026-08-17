// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file statistics_json.cpp
 * @brief The read-path telemetry, machine-readable.
 *
 * Its own translation unit because `statistics.cpp` is compiled only when
 * statistics are enabled, and this has to exist either way: a build without
 * statistics still answers the question, and the answer is a document full of
 * zeros with `statistics_level: "off"` at the top. A missing symbol would be a
 * worse answer than an honest zero.
 *
 * Written by hand with fmt, as `census.cpp` does, so that the library keeps no
 * JSON dependency of its own.
 */

#include <utxoz/statistics.hpp>

#include <string>

#include <fmt/format.h>

namespace utxoz {

namespace {

/// The histogram bucket labels, in the order `lookup_stats::bucket_of` assigns
/// them. Spelled out rather than computed so that a reader of the output does
/// not have to reconstruct the bucketing from the code.
constexpr char const* bucket_labels[] = {"1", "2", "3", "4", "5-8", "9+"};
static_assert(std::size(bucket_labels) == 6,
              "the labels and lookup_stats::bucket_count have parted company");

std::string histogram_json(std::array<size_t, 6> const& buckets) {
    std::string out = "{";
    for (size_t i = 0; i < buckets.size(); ++i) {
        out += fmt::format(R"("{}": {}{})", bucket_labels[i], buckets[i],
                           i + 1 < buckets.size() ? ", " : "");
    }
    out += "}";
    return out;
}

} // namespace

std::string to_json(lookup_telemetry const& t) {
    std::string out = "{\n";
    out += fmt::format("  \"schema_version\": {},\n", lookup_telemetry::schema_version);
    // The level, not a boolean. "statistics are on" was never the same claim as
    // "these counters were collected": at `basic` the first is true and the
    // second is false, and a reader given only the first would take a page of
    // zeros for a database that answered nothing.
    out += fmt::format("  \"statistics_level\": \"{}\",\n", t.statistics_level);
    out += fmt::format("  \"storage_mode\": \"{}\",\n",
                       t.mode == storage_mode::reference ? "reference" : "full");

    // The three that have no class, and say so.
    out += fmt::format("  \"unattributed\": {{\"lookups_received\": {}, \"deferred\": {}, "
                       "\"absent\": {}, \"note\": \"a lookup arrives with a key and no size, "
                       "and an absent one was refused by every class\"}},\n",
                       t.lookups_received, t.deferred, t.absent);

    out += "  \"classes\": [\n";
    for (size_t i = 0; i < t.classes.size(); ++i) {
        auto const& c = t.classes[i];
        out += "    {";
        if (c.container_class == reference_class) {
            out += R"("container_class": "reference", )";
        } else {
            out += fmt::format(R"("container_class": {}, )", c.container_class);
        }
        out += fmt::format(R"("active_maps_probed": {}, "answered_from_active": {}, )",
                           c.active_maps_probed, c.answered_from_active);
        out += fmt::format(R"("resolved_historical": {}, "generations_probed": {}, )",
                           c.resolved_historical, c.generations_probed);
        out += fmt::format(R"("files_opened": {}, "cache_hits": {}, )",
                           c.files_opened, c.cache_hits);
        out += fmt::format(R"("avg_probe_ordinal": {:.4f}, "avg_version_distance": {:.4f}, )",
                           c.avg_probe_ordinal, c.avg_version_distance);
        out += fmt::format(R"("probe_ordinal_histogram": {}, )",
                           histogram_json(c.probe_ordinal_histogram));
        out += fmt::format(R"("version_distance_histogram": {}}})",
                           histogram_json(c.version_distance_histogram));
        out += (i + 1 < t.classes.size()) ? ",\n" : "\n";
    }
    out += "  ],\n";

    // Summed from the classes above at the moment of writing, not carried
    // alongside them. Two running totals of one fact eventually disagree, and
    // then nobody can say which is the defect.
    out += fmt::format("  \"derived_totals\": {{\"answered_from_active\": {}, "
                       "\"resolved_historical\": {}, \"generations_probed\": {}, "
                       "\"files_opened\": {}, \"cache_hits\": {}}}\n",
                       t.answered_from_active(), t.resolved_historical(),
                       t.generations_probed(), t.files_opened(), t.cache_hits());
    out += "}\n";
    return out;
}

} // namespace utxoz
