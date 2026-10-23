// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file calib_historical.cpp
 * @brief One end-to-end historical lookup, measured where the history is real.
 *
 * The accepted A/A harness (calib_stationary.cpp) measures a single-generation
 * fixture of ninety thousand entries. That store fits in cache, and every lookup
 * it makes is answered by the active container. It calibrated the instrument at
 * that point and said so; it is deliberately left untouched here, hash and all.
 *
 * This one measures the operation that point could not reach: a lookup whose
 * answer is **not** in the active container, and which therefore has to go
 * through the two-step contract the library actually publishes —
 *
 *     find(key) -> not_resolved        the active versions cannot answer
 *     resolve(batch)  -> found/absent  the older versions can, and do
 *
 * Three things follow from that, and they are the reasons this file exists.
 *
 * 1. **An absence here is proven, not assumed.** `find` returning `not_resolved`
 *    means "not answerable from the active versions" — in a single-generation
 *    fixture that is what a miss looks like, which is why a miss measured there
 *    is a measurement of the wrong thing. A miss is only established when
 *    `resolve` puts the key in `absent`, having read every version that could
 *    hold it. This harness refuses to call anything else a miss.
 *
 * 2. **The query working set is the whole history.** `resolve` walks the versions
 *    below the active one, and the file cache holds exactly one file (its default
 *    `max_size` of 1, which nothing overrides). So a batch touches every
 *    generation, and the bytes under the query are the bytes of the store, not of
 *    one container. That is what gets this past L3 — millions of probes over
 *    ninety thousand keys never will, however long they run.
 *
 * 3. **`resolve` is a batch call, so the batch size is part of the result.** Its
 *    per-key cost amortises the per-generation file opens over the batch. A
 *    figure quoted without the batch size is not reproducible, so it is declared,
 *    recorded, and checked by the runner.
 *
 * Two modes, because the fixture is expensive and must not differ between the
 * sides of a pair:
 *
 *   --build-fixture DIR   build it once, census it, write DIR/shape.json, exit
 *   --fixture DIR         measure against that image, read-only
 *
 * Both sides of a pair, and both arms of the control, then run against *the same
 * image* rather than against two images built the same way.
 *
 * The rules of the accepted harness carry over unchanged: validation sits outside
 * the timing and a failure is an exit code; setup and warmup are not in the
 * elapsed time; the operating point is part of the result.
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <utxoz/census.hpp>
#include <utxoz/database.hpp>
#include <utxoz/statistics.hpp>
#include <utxoz/types.hpp>

namespace fs = std::filesystem;

namespace {

constexpr int exit_ok = 0;
constexpr int exit_usage = 2;
constexpr int exit_operation_failed = 3;
constexpr int exit_setup_failed = 4;

/// The same key derivation as the accepted harness, so a fixture built by either
/// holds the same keys and the two results are about the same store.
utxoz::raw_outpoint key_of(uint64_t n) {
    utxoz::raw_outpoint k{};
    uint64_t x = n;
    for (size_t i = 0; i + 8 <= k.size(); i += 8) {
        uint64_t z = (x += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        z ^= z >> 31;
        for (size_t b = 0; b != 8; ++b) k[i + b] = uint8_t(z >> (8 * b));
    }
    for (size_t b = 0; b != 8; ++b) k[b] = uint8_t(n >> (8 * b));
    return k;
}

/// Keys stored by the fixture occupy [0, population). Miss keys start well past
/// that, so a miss is a miss by construction rather than by luck.
constexpr uint64_t miss_key_base = 1ull << 40;

/// The payload stored for entry `n`, encoding `n` so that a verification pass can
/// tell an answer carrying another key's value from a correct one.
std::vector<uint8_t> value_for(uint64_t n, size_t size) {
    std::vector<uint8_t> v(size);
    for (size_t i = 0; i != size; ++i) v[i] = uint8_t(i * 7 + 1);
    for (size_t b = 0; b != 8 && b < size; ++b) v[b] = uint8_t(n >> (8 * b));
    return v;
}

/// The height stored with entry `n`: a thousand entries to a block.
///
/// A constant height cannot catch an answer that carried the wrong entry's
/// height, because every entry's height is the same. This one varies across the
/// store, so the verification pass checks a second distinguishable field — and it
/// doubles as the label that says which part of the history an entry came from.
constexpr uint32_t height_of(uint64_t n) { return uint32_t(1000 + n / 1000); }

/// The height a lookup is made at: above every stored height, fixed, declared.
constexpr uint32_t query_height = 9'000'000;

/// What one find() answered, as four cases rather than two. Extracted so it can
/// be tested without a database.
enum class find_outcome { resolved, not_resolved, not_found, real_error };

[[nodiscard]] find_outcome classify(utxoz::result<utxoz::full_find_result> const& r) {
    if (r) return find_outcome::resolved;
    switch (r.error()) {
        case utxoz::error_code::not_resolved: return find_outcome::not_resolved;
        case utxoz::error_code::not_found:    return find_outcome::not_found;
        default:                              return find_outcome::real_error;
    }
}

/// A resolution either covered the batch or it failed; there is no third case,
/// and reading a failure as "nothing was there" is the mistake this guards.
enum class resolve_outcome { covered, real_error };

[[nodiscard]] resolve_outcome classify_resolution(
    utxoz::result<utxoz::full_resolution> const& r) {
    return r ? resolve_outcome::covered : resolve_outcome::real_error;
}

/// Writes body to p. False when it could not be opened, written or closed: a
/// full disk shows up at fclose, when the buffer is flushed.
[[nodiscard]] bool write_file(fs::path const& p, std::string const& body) {
    FILE* f = std::fopen(p.string().c_str(), "w");
    if ( ! f) return false;
    bool const wrote = std::fputs(body.c_str(), f) != EOF;
    return std::fclose(f) == 0 && wrote;
}

/// write_file must report a full device and a missing directory, and accept a
/// writable path. 0 when it does.
[[nodiscard]] int selftest_write_file() {
    int bad = 0;
    if (fs::exists("/dev/full") && write_file("/dev/full", "x")) {
        fmt::println(stderr, "write_file accepted a write to /dev/full"); ++bad;
    }
    if (write_file("/nonexistent-utxoz-dir/shape.json", "x")) {
        fmt::println(stderr, "write_file accepted a missing directory"); ++bad;
    }
    auto const ok_path = fs::temp_directory_path()
                       / fmt::format("utxoz-write-file-selftest-{}", std::random_device{}());
    if ( ! write_file(ok_path, "x")) {
        fmt::println(stderr, "write_file refused {}", ok_path.string()); ++bad;
    }
    std::error_code ec;
    fs::remove(ok_path, ec);
    return bad;
}

[[nodiscard]] int selftest_classifier() {
    int bad = 0;
    auto check = [&](char const* what, bool ok) {
        if ( ! ok) { fmt::println(stderr, "classifier: {} misclassified", what); ++bad; }
        else fmt::println("  classifier {:<42} ok", what);
    };

    utxoz::full_find_result const resolved_value{};
    check("a resolved find", classify(utxoz::result<utxoz::full_find_result>{resolved_value})
                             == find_outcome::resolved);
    check("find not_resolved is not an error",
          classify(std::unexpected(utxoz::error_code::not_resolved))
          == find_outcome::not_resolved);
    check("find not_found is its own case",
          classify(std::unexpected(utxoz::error_code::not_found))
          == find_outcome::not_found);
    check("find version_unreadable is a real error",
          classify(std::unexpected(utxoz::error_code::version_unreadable))
          == find_outcome::real_error);
    check("find file_open_failed is a real error",
          classify(std::unexpected(utxoz::error_code::file_open_failed))
          == find_outcome::real_error);
    check("find closed is a real error",
          classify(std::unexpected(utxoz::error_code::closed))
          == find_outcome::real_error);

    // The resolution classifier, and the case that matters: an unreadable
    // version is a fault, never an empty answer.
    utxoz::full_resolution const empty{};
    check("a covered resolution",
          classify_resolution(utxoz::result<utxoz::full_resolution>{empty})
          == resolve_outcome::covered);
    check("resolve version_unreadable is a real error",
          classify_resolution(std::unexpected(utxoz::error_code::version_unreadable))
          == resolve_outcome::real_error);
    check("resolve catalog_unreadable is a real error",
          classify_resolution(std::unexpected(utxoz::error_code::catalog_unreadable))
          == resolve_outcome::real_error);
    return bad;
}

struct file_counts {
    size_t all = 0;
    size_t sharded = 0;
};

file_counts count_version_files(fs::path const& dir) {
    file_counts c;
    std::error_code ec;
    for (auto const& e : fs::directory_iterator(dir, ec)) {
        auto const name = e.path().filename().string();
        if (name.rfind("cont_", 0) != 0 || ! name.ends_with(".dat")) continue;
        ++c.all;
        if (name.rfind("cont_0_v", 0) == 0 || name.rfind("cont_0s", 0) == 0) ++c.sharded;
    }
    return c;
}

// ---------------------------------------------------------------- fixture shape

/// What a measuring run needs to know about an image it did not build.
///
/// `historical_prefix` is the one figure that cannot be guessed. It is the
/// smallest `n` whose key the active versions answer, so every key below it is
/// historical in this image. At N=1 the active container holds a contiguous
/// suffix of `n` and this is where it starts; with several shards each shard's
/// active container holds its own suffix, the union is not contiguous, and the
/// only honest way to find the prefix is to ask find() about every key. So the
/// build does exactly that, and records how many keys the active versions hold
/// in total as a check on the scan.
struct shape {
    uint64_t population = 0;
    uint64_t historical_prefix = 0;
    uint64_t active_entries = 0;
    uint64_t generations_class_48 = 0;
    uint64_t shard_count = 0;
    uint64_t geometry_id = 0;
    size_t value_size = 0;
};

struct prefix_scan {
    uint64_t historical_prefix = 0;
    uint64_t active_entries = 0;
    uint64_t other_outcomes = 0;   ///< not_found or a real error: must be zero
};

prefix_scan scan_historical_prefix(utxoz::full_db const& db, uint64_t population) {
    prefix_scan s;
    s.historical_prefix = population;
    for (uint64_t n = 0; n != population; ++n) {
        switch (classify(db.find(key_of(n), query_height))) {
            case find_outcome::resolved:
                ++s.active_entries;
                if (s.historical_prefix == population) s.historical_prefix = n;
                break;
            case find_outcome::not_resolved: break;
            default: ++s.other_outcomes; break;
        }
    }
    return s;
}

std::string shape_json(shape const& s) {
    return fmt::format(
        "{{\n"
        "  \"schema\": \"utxoz-hist-shape-2\",\n"
        "  \"population\": {},\n"
        "  \"historical_prefix\": {},\n"
        "  \"active_entries\": {},\n"
        "  \"generations_class_48\": {},\n"
        "  \"shard_count\": {},\n"
        "  \"geometry_id\": {},\n"
        "  \"value_size\": {}\n"
        "}}\n",
        s.population, s.historical_prefix, s.active_entries, s.generations_class_48,
        s.shard_count, s.geometry_id, s.value_size);
}

/// Reads back the fields written above. Deliberately not a JSON parser: the file
/// is written by this program and read by this program, and a dependency on a
/// parser for a handful of integers would be the larger risk. A shape written
/// by the first version of this file has no `historical_prefix` and is refused.
bool read_shape(fs::path const& p, shape& s) {
    FILE* f = std::fopen(p.string().c_str(), "r");
    if ( ! f) return false;
    std::string text;
    char buf[4096];
    while (auto got = std::fread(buf, 1, sizeof buf, f)) text.append(buf, got);
    std::fclose(f);
    if (text.find("\"utxoz-hist-shape-2\"") == std::string::npos) return false;
    auto field = [&](char const* name, uint64_t& dst) {
        auto const at = text.find(std::string("\"") + name + "\":");
        if (at == std::string::npos) return false;
        dst = std::strtoull(text.c_str() + at + std::strlen(name) + 3, nullptr, 10);
        return true;
    };
    uint64_t vs = 0;
    if ( ! field("population", s.population)) return false;
    if ( ! field("historical_prefix", s.historical_prefix)) return false;
    if ( ! field("active_entries", s.active_entries)) return false;
    if ( ! field("generations_class_48", s.generations_class_48)) return false;
    if ( ! field("shard_count", s.shard_count)) return false;
    if ( ! field("geometry_id", s.geometry_id)) return false;
    if ( ! field("value_size", vs)) return false;
    s.value_size = size_t(vs);
    return true;
}

/// FNV-1a over the probe sequence, so two sides of a pair can show they asked
/// for the same keys in the same order without shipping the keys.
struct probe_digest {
    uint64_t h = 0xcbf29ce484222325ULL;
    void add(uint64_t v) {
        for (int b = 0; b != 8; ++b) {
            h ^= uint8_t(v >> (8 * b));
            h *= 0x100000001b3ULL;
        }
    }
};

/// The per-generation detail Codex asked to be recorded: capacity, load and size
/// of every generation of the sharded class, plus the totals.
std::string census_json(utxoz::census_report const& c) {
    std::string gens;
    uint64_t class_48_generations = 0;
    for (auto const& cl : c.classes) {
        if (cl.container_size != 48) continue;
        class_48_generations = cl.generations;
        for (auto const& g : cl.generations_detail) {
            gens += fmt::format(
                "    {{\"shard\": {}, \"generation\": {}, \"active\": {}, "
                "\"entries\": {}, \"bucket_count\": {}, "
                "\"load_factor\": {:.6f}, \"segment_size_bytes\": {}, "
                "\"segment_free_bytes\": {}, "
                "\"logical_file_bytes\": {}, \"occupied_slot_bytes\": {}, "
                "\"empty_slot_bytes\": {}}},\n",
                g.shard, g.generation, g.active ? "true" : "false", g.entries,
                g.bucket_count, g.load_factor(),
                g.segment_size_bytes, g.segment_free_bytes, g.logical_file_bytes,
                g.occupied_slot_bytes, g.empty_slot_bytes);
        }
    }
    if (gens.size() >= 2) gens.erase(gens.size() - 2, 1);   // the trailing comma

    // One line per shard. Empty in an unsharded build, where the class is its
    // own single container and generations_detail already says everything.
    std::string per_shard;
    for (auto const& cl : c.classes) {
        if (cl.container_size != 48) continue;
        for (auto const& sh : cl.shards_detail) {
            per_shard += fmt::format(
                "    {{\"shard\": {}, \"entries\": {}, \"generations\": {}, "
                "\"active_generation\": {}, \"logical_file_bytes\": {}}},\n",
                sh.shard, sh.entries, sh.generations, sh.active_generation,
                sh.logical_file_bytes);
        }
    }
    if (per_shard.size() >= 2) per_shard.erase(per_shard.size() - 2, 1);

    std::string classes;
    for (auto const& cl : c.classes) {
        classes += fmt::format(
            "    {{\"container_class\": {}, \"container_size\": {}, \"value_size\": {}, "
            "\"pair_size\": {}, \"entries\": {}, \"generations\": {}, "
            "\"active_generation\": {}, \"logical_file_bytes\": {}}},\n",
            cl.container_class, cl.container_size, cl.value_size, cl.pair_size,
            cl.entries, cl.generations, cl.active_generation, cl.logical_file_bytes);
    }
    if (classes.size() >= 2) classes.erase(classes.size() - 2, 1);

    return fmt::format(
        "{{\n"
        "  \"schema\": \"utxoz-hist-census-2\",\n"
        "  \"geometry_id\": {},\n"
        "  \"shard_count\": {},\n"
        "  \"entries\": {},\n"
        "  \"generations_class_48\": {},\n"
        "  \"logical_file_bytes\": {},\n"
        "  \"occupied_slot_bytes\": {},\n"
        "  \"empty_slot_bytes\": {},\n"
        "  \"estimated_group_metadata_bytes\": {},\n"
        "  \"files_examined\": {},\n"
        "  \"classes\": [\n{}  ],\n"
        "  \"shards_detail_class_48\": [\n{}  ],\n"
        "  \"generations_detail_class_48\": [\n{}  ]\n"
        "}}\n",
        c.geometry_id, utxoz::shard_count, c.entries, class_48_generations,
        c.logical_file_bytes, c.occupied_slot_bytes, c.empty_slot_bytes,
        c.estimated_group_metadata_bytes, c.files_examined, classes, per_shard, gens);
}

// -------------------------------------------------------------------- options

struct options {
    std::string metric;              ///< hist_hit | hist_miss
    std::string fixture;             ///< an image built by --build-fixture
    std::string build_fixture;       ///< build one here and exit
    uint64_t ops = 40'000;           ///< BASE lookups: the denominator
    uint64_t warmup = 8'000;
    uint64_t batch = 4'000;          ///< requests per resolve() call
    uint64_t population = 100'000;   ///< build only
    uint64_t extra_pct = 0;          ///< positive control: extra real lookups
    uint64_t probe_below = 0;        ///< hist_hit draws from [0, this); 0 = the fixture's prefix
    uint64_t seed = 1;
    size_t value_size = 33;
    std::string out;
    bool selftest_classifier_only = false;
    bool selftest_write_file_only = false;
    bool selftest_draw_only = false;
    bool selftest_wrong_payload = false;
    bool selftest_miss_as_hit = false;
    bool diagnose = false;           ///< count one batch's work, untimed, and exit
};

int usage(FILE* to) {
    std::fputs(
        "usage: utxoz_calib_hist --metric hist_hit|hist_miss --fixture DIR [options]\n"
        "       utxoz_calib_hist --build-fixture DIR --population N [options]\n"
        "\n"
        "  --ops N          BASE lookups in the timed region; the denominator\n"
        "  --warmup N       lookups before it, not timed\n"
        "  --batch N        requests per resolve() call; must divide ops and warmup\n"
        "  --extra-pct N    positive control: N% more real lookups of the SAME\n"
        "                   distribution inside the timed region, NOT in the\n"
        "                   denominator. +N% work is not +N% time; see the report.\n"
        "  --population N   entries the fixture holds (--build-fixture only)\n"
        "  --probe-below P  hist_hit draws n from [0, P); P must not exceed the\n"
        "                   fixture's historical prefix. Comparing two fixtures,\n"
        "                   pass the smaller prefix to both, so both sides ask for\n"
        "                   the same keys and neither asks for an active one\n"
        "  --seed N         fixes the probe order\n"
        "  --value-size N   payload bytes, selects the size class (default 33)\n"
        "  --out FILE       write the JSON here as well as to stdout\n"
        "  --selftest-classifier     test both classifiers and exit\n"
        "  --selftest-write-file     test that a failed write is reported, and exit\n"
        "  --selftest-draw           test that the warmup draws its own stream, and exit\n"
        "  --selftest-wrong-payload  expect each hit to carry its NEIGHBOUR's\n"
        "                            payload; a correct store must fail this\n"
        "  --diagnose       resolve ONE batch (the first of the probe sequence) in a\n"
        "                   fresh process, untimed, and print the library's lookup\n"
        "                   counters and a digest of the answers. Counters only:\n"
        "                   needs a build with UTXOZ_STATISTICS_LEVEL=lookup\n"
        "  --selftest-miss-as-hit    require hist_miss keys to be found; a correct\n"
        "                            store must fail this\n",
        to);
    return to == stdout ? exit_ok : exit_usage;
}

/// One batch of probes: the indices, and the keys handed to the library.
struct probe_batch {
    std::vector<uint64_t> index;                 ///< `n` of each probe
    std::vector<utxoz::raw_outpoint> key;
    bool extra = false;                          ///< control work, not in the denominator
};

/// The warmup's own stream (splitmix64 of the seed). With the timed run's seed
/// the warmup would resolve exactly the timed run's first batches beforehand.
/// Reproducible, but independent, not disjoint: the two can share keys by chance.
constexpr uint64_t warmup_seed(uint64_t seed) {
    uint64_t z = seed + 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

/// Probes are drawn uniformly, and **without replacement inside a batch**.
///
/// That is not a detail. `resolve` answers one entry per *distinct* key of the
/// request span, so a batch holding the same key twice comes back with fewer
/// answers than requests and the accounting silently stops adding up. Drawing
/// distinct indices per batch keeps `found + absent == requests` exact, which is
/// what lets a shortfall be reported as the fault it is.
std::vector<probe_batch> draw_batches(options const& o, shape const& s,
                                      uint64_t total_ops, uint64_t extra_ops, uint64_t seed) {
    auto const base_batches = total_ops / o.batch;
    auto const extra_batches = extra_ops / o.batch;
    std::mt19937_64 rng(seed);
    // Historical probes come from below the active boundary; miss probes from a
    // key space never inserted.
    uint64_t const span = o.metric == "hist_hit" ? o.probe_below : s.population;
    std::uniform_int_distribution<uint64_t> pick(0, span - 1);

    // The extra batches are spread through the run rather than appended, so the
    // control arm's work has the same temporal shape as the baseline's and not a
    // five-percent tail with its own cache state.
    std::vector<probe_batch> out;
    out.reserve(base_batches + extra_batches);
    uint64_t placed_extra = 0;
    for (uint64_t b = 0; b != base_batches; ++b) {
        out.push_back({});
        out.back().extra = false;
        // Interleave: by batch b, (b+1)*extra/base extra batches are due.
        auto const due = extra_batches == 0 ? 0
                       : (b + 1) * extra_batches / base_batches;
        while (placed_extra < due) { out.push_back({}); out.back().extra = true; ++placed_extra; }
    }
    while (placed_extra < extra_batches) {
        out.push_back({}); out.back().extra = true; ++placed_extra;
    }

    for (auto& pb : out) {
        pb.index.reserve(o.batch);
        pb.key.reserve(o.batch);
        std::unordered_set<uint64_t> seen;
        seen.reserve(o.batch * 2);
        while (pb.index.size() != o.batch) {
            auto const n = pick(rng);
            if ( ! seen.insert(n).second) continue;
            pb.index.push_back(n);
            pb.key.push_back(o.metric == "hist_hit" ? key_of(n)
                                                    : key_of(miss_key_base + n));
        }
    }
    return out;
}

/// What the warmup and the timed run draw, for --selftest-draw: a digest of the
/// indices of each list, for the arm without and with a 20% control (two extra batches).
uint64_t index_digest(std::vector<probe_batch> const& list) {
    probe_digest d;
    for (auto const& pb : list) { d.add(pb.extra ? 1 : 0); for (auto const n : pb.index) d.add(n); }
    return d.h;
}

[[nodiscard]] int selftest_draw() {
    int bad = 0;
    shape s; s.population = 200'000;
    for (auto const* metric : {"hist_hit", "hist_miss"}) {
        for (uint64_t const extra_pct : {0, 20}) {
            options o; o.metric = metric; o.probe_below = 100'000;
            auto const extra_ops = o.ops * extra_pct / 100;
            auto const timed = draw_batches(o, s, o.ops, extra_ops, o.seed);
            auto const warm = draw_batches(o, s, o.warmup, 0, warmup_seed(o.seed));
            fmt::println("{} extra={}% timed={:016x} warmup={:016x}", metric, extra_pct,
                         index_digest(timed), index_digest(warm));
            // The warmup keeps its size, is reproducible, and is not the
            // timed run's first batches.
            bool same_prefix = warm.size() <= timed.size();
            for (size_t b = 0; same_prefix && b != warm.size(); ++b)
                same_prefix = warm[b].index == timed[b].index;
            if (warm.size() != o.warmup / o.batch || same_prefix
                    || index_digest(warm) != index_digest(draw_batches(o, s, o.warmup, 0,
                                                                       warmup_seed(o.seed)))) {
                fmt::println(stderr, "{} extra={}%: the warmup is not its own stream", metric, extra_pct);
                ++bad;
            }
        }
    }
    return bad;
}

/// What one pass over a list of batches did, counted by outcome.
struct tally {
    uint64_t resolved_by_find = 0;   ///< answered by the active versions
    uint64_t found = 0;              ///< answered by resolve()
    uint64_t absent = 0;             ///< PROVEN absent by resolve()
    uint64_t not_found_from_find = 0;
    uint64_t real_errors = 0;
    uint64_t base_ops = 0;
    uint64_t extra_ops = 0;
    int first_error = 0;
};

}  // namespace

int main(int argc, char** argv) {
    options o;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](uint64_t& dst) {
            if (i + 1 < argc) dst = std::strtoull(argv[++i], nullptr, 10);
        };
        if (a == "--metric" && i + 1 < argc) o.metric = argv[++i];
        else if (a == "--fixture" && i + 1 < argc) o.fixture = argv[++i];
        else if (a == "--build-fixture" && i + 1 < argc) o.build_fixture = argv[++i];
        else if (a == "--ops") next(o.ops);
        else if (a == "--warmup") next(o.warmup);
        else if (a == "--batch") next(o.batch);
        else if (a == "--population") next(o.population);
        else if (a == "--extra-pct") next(o.extra_pct);
        else if (a == "--probe-below") next(o.probe_below);
        else if (a == "--seed") next(o.seed);
        else if (a == "--value-size") { uint64_t v = 0; next(v); o.value_size = size_t(v); }
        else if (a == "--out" && i + 1 < argc) o.out = argv[++i];
        else if (a == "--selftest-classifier") o.selftest_classifier_only = true;
        else if (a == "--selftest-write-file") o.selftest_write_file_only = true;
        else if (a == "--selftest-draw") o.selftest_draw_only = true;
        else if (a == "--selftest-wrong-payload") o.selftest_wrong_payload = true;
        else if (a == "--selftest-miss-as-hit") o.selftest_miss_as_hit = true;
        else if (a == "--diagnose") o.diagnose = true;
        else if (a == "--help" || a == "-h") return usage(stdout);
        else { fmt::println(stderr, "unknown argument {}", a); return usage(stderr); }
    }

    if (o.selftest_classifier_only)
        return selftest_classifier() == 0 ? exit_ok : exit_operation_failed;
    if (o.selftest_write_file_only)
        return selftest_write_file() == 0 ? exit_ok : exit_operation_failed;
    if (o.selftest_draw_only)
        return selftest_draw() == 0 ? exit_ok : exit_operation_failed;

    // ----------------------------------------------------------- build a fixture
    if ( ! o.build_fixture.empty()) {
        if ( ! o.fixture.empty() || ! o.metric.empty()) {
            fmt::println(stderr, "--build-fixture does not measure; drop --fixture/--metric");
            return exit_usage;
        }
        if (o.population < 1) { fmt::println(stderr, "--population must be positive"); return exit_usage; }
        fs::path dir(o.build_fixture);
        if (fs::exists(dir)) {
            fmt::println(stderr, "{} already exists; refusing to reuse or remove it",
                         dir.string());
            return exit_setup_failed;
        }
        std::error_code ec;
        if ( ! fs::create_directories(dir, ec) || ec) {
            fmt::println(stderr, "could not create {}: {}", dir.string(), ec.message());
            return exit_setup_failed;
        }

        auto opened = utxoz::full_db::open_for_testing(dir.string(), true);
        if ( ! opened) { fmt::println(stderr, "could not create the fixture"); return exit_setup_failed; }
        auto db = std::move(*opened);
        auto const t0 = std::chrono::steady_clock::now();
        for (uint64_t n = 0; n != o.population; ++n) {
            auto const payload = value_for(n, o.value_size);
            auto const stored = db.insert(key_of(n), utxoz::output_data_span{payload},
                                          height_of(n));
            if ( ! stored || ! *stored) {
                fmt::println(stderr, "fixture insert {} did not store", n);
                return exit_setup_failed;
            }
            if ((n + 1) % 500'000 == 0)
                fmt::println(stderr, "  inserted {}/{}", n + 1, o.population);
        }
        auto const build_s = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - t0).count();
        db.close();

        // Reopen to read the shape through the published contract.
        auto reopened = utxoz::full_db::open_for_testing(dir.string(), false);
        if ( ! reopened) { fmt::println(stderr, "could not reopen the fixture"); return exit_setup_failed; }
        auto rdb = std::move(*reopened);
        auto const censused = rdb.census();
        if ( ! censused) {
            fmt::println(stderr, "census failed: error_code={}", int(censused.error()));
            return exit_setup_failed;
        }
        shape s;
        s.population = rdb.size();
        auto const scan_t0 = std::chrono::steady_clock::now();
        auto const scan = scan_historical_prefix(rdb, s.population);
        auto const scan_s = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - scan_t0).count();
        s.historical_prefix = scan.historical_prefix;
        s.active_entries = scan.active_entries;
        s.value_size = o.value_size;
        s.shard_count = utxoz::shard_count;
        s.geometry_id = censused->geometry_id;
        uint64_t active_in_census = 0;
        for (auto const& cl : censused->classes) {
            if (cl.container_size != 48) continue;
            s.generations_class_48 = cl.generations;
            for (auto const& g : cl.generations_detail)
                if (g.active) active_in_census += g.entries;
        }
        rdb.close();

        if (scan.other_outcomes != 0) {
            fmt::println(stderr, "the prefix scan saw {} lookups that were neither "
                         "answered nor not_resolved", scan.other_outcomes);
            return exit_setup_failed;
        }
        // Two independent counts of the same thing: keys find() answered, and
        // entries the census found in active generations.
        if (s.active_entries != active_in_census) {
            fmt::println(stderr, "find() answered {} keys but the active generations "
                         "hold {} entries", s.active_entries, active_in_census);
            return exit_setup_failed;
        }
        fmt::println(stderr, "  prefix scan: {} finds in {:.1f}s", s.population, scan_s);

        if (s.population != o.population) {
            fmt::println(stderr, "the fixture holds {} entries, not the {} asked for",
                         s.population, o.population);
            return exit_setup_failed;
        }
        if (s.historical_prefix == 0 || s.historical_prefix >= s.population) {
            fmt::println(stderr, "no usable history: the first key the active versions "
                         "answer is {} of {}, so there is nothing below it to measure",
                         s.historical_prefix, s.population);
            return exit_setup_failed;
        }

        auto const files = count_version_files(dir);
        // A fixture without its shape, or with a truncated one, must not look
        // built: a partial number still parses. The shape goes last.
        for (auto const& [name, body] : {std::pair{"census.json", census_json(*censused)},
                                         std::pair{"shape.json", shape_json(s)}}) {
            if ( ! write_file(dir / name, body)) {
                fmt::println(stderr, "could not write {}", (dir / name).string());
                return exit_setup_failed;
            }
        }
        fmt::println("{}", shape_json(s));
        fmt::println(stderr,
            "  built {} entries in {:.1f}s; shards={} class-48 generations={} "
            "historical_prefix={} active_entries={}; version files all={} class48={}",
            s.population, build_s, s.shard_count, s.generations_class_48,
            s.historical_prefix, s.active_entries, files.all, files.sharded);
        return exit_ok;
    }

    // --------------------------------------------------------------- measure
    if (o.metric != "hist_hit" && o.metric != "hist_miss") {
        fmt::println(stderr, "--metric must be hist_hit or hist_miss");
        return exit_usage;
    }
    if (o.fixture.empty()) {
        fmt::println(stderr, "--fixture is required; build one with --build-fixture");
        return exit_usage;
    }
    if (o.batch < 1) { fmt::println(stderr, "--batch must be positive"); return exit_usage; }
    if (o.ops < 1 || o.ops % o.batch != 0) {
        fmt::println(stderr, "--ops {} must be positive and a multiple of --batch {}",
                     o.ops, o.batch);
        return exit_usage;
    }
    if (o.warmup % o.batch != 0) {
        fmt::println(stderr, "--warmup {} must be a multiple of --batch {}",
                     o.warmup, o.batch);
        return exit_usage;
    }
    if (o.extra_pct > 100) {
        fmt::println(stderr, "--extra-pct {} is out of range", o.extra_pct);
        return exit_usage;
    }
    // The control's extra work has to be a whole number of batches, so that both
    // arms issue whole resolve() calls and the only difference is how many.
    auto const base_batches = o.ops / o.batch;
    if (o.extra_pct != 0 && (base_batches * o.extra_pct) % 100 != 0) {
        fmt::println(stderr, "--extra-pct {} of {} batches is not a whole number of "
                     "batches; pick ops/batch so that it is", o.extra_pct, base_batches);
        return exit_usage;
    }
    auto const extra_ops = o.ops * o.extra_pct / 100;

    shape s;
    fs::path const fixture(o.fixture);
    if ( ! read_shape(fixture / "shape.json", s)) {
        fmt::println(stderr, "could not read {}/shape.json", fixture.string());
        return exit_setup_failed;
    }
    if (s.value_size != o.value_size) {
        fmt::println(stderr, "the fixture was built with --value-size {}, not {}",
                     s.value_size, o.value_size);
        return exit_usage;
    }
    if (s.shard_count != utxoz::shard_count) {
        fmt::println(stderr, "the fixture was built by an N={} binary and this one is N={}",
                     s.shard_count, utxoz::shard_count);
        return exit_usage;
    }
    if (o.probe_below == 0) o.probe_below = s.historical_prefix;
    if (o.probe_below > s.historical_prefix) {
        fmt::println(stderr, "--probe-below {} exceeds this fixture's historical prefix {}: "
                     "some probes would be answered by the active versions",
                     o.probe_below, s.historical_prefix);
        return exit_usage;
    }
    {
        // draw_batches() draws o.batch distinct indices from [0, span): a batch
        // larger than the span can never be filled, and the draw would not end.
        uint64_t const span = o.metric == "hist_hit" ? o.probe_below : s.population;
        if (o.batch > span) {
            fmt::println(stderr, "--batch {} exceeds the {} distinct keys a {} batch can draw from",
                         o.batch, span, o.metric);
            return exit_usage;
        }
    }

    // ------------------------------------------------------------------ setup
    // Read-only from here on: no insert, no erase, no compaction. The image is
    // shared by every sample of the campaign on purpose.
    auto opened = utxoz::full_db::open_for_testing(fixture.string(), false);
    if ( ! opened) {
        fmt::println(stderr, "could not open the fixture at {}", fixture.string());
        return exit_setup_failed;
    }
    auto db = std::move(*opened);

    auto const population = db.size();
    auto const files = count_version_files(fixture);

    // The shape is a claim about an image this process did not build, so it is
    // checked against the image rather than trusted: the entry below the boundary
    // must not be answerable from the active versions, and the one at it must be.
    if (population != s.population) {
        fmt::println(stderr, "the image holds {} entries, not the {} shape.json claims",
                     population, s.population);
        return exit_setup_failed;
    }
    {
        // The prefix is where find() first answers; the key just below it must not
        // be answerable, and the one at it must be. A full re-scan per sample would
        // cost seconds; this is the check that the shape describes this image.
        auto const below = classify(db.find(key_of(s.historical_prefix - 1), query_height));
        auto const at = classify(db.find(key_of(s.historical_prefix), query_height));
        if (below != find_outcome::not_resolved || at != find_outcome::resolved) {
            fmt::println(stderr, "the historical prefix {} does not hold in this image "
                         "(below={} at={}); the shape does not describe it",
                         s.historical_prefix, int(below), int(at));
            return exit_setup_failed;
        }
    }

    auto const batches = draw_batches(o, s, o.ops, extra_ops, o.seed);
    auto const warm_batches = draw_batches(o, s, o.warmup == 0 ? o.batch : o.warmup, 0,
                                           warmup_seed(o.seed));

    // The identity of what will be asked: every index of every batch, the
    // extra flag included, warmup first. Equal on both sides of a pair or the
    // pair is not comparing the same lookups.
    probe_digest digest;
    digest.add(o.metric == "hist_hit" ? 1 : 2);
    for (auto const* list : {&warm_batches, &batches}) {
        digest.add(list->size());
        for (auto const& pb : *list) {
            digest.add(pb.extra ? 1 : 0);
            for (auto const n : pb.index) digest.add(n);
        }
    }

    // ------------------------------------------------------------- diagnose
    // Counts, not time. One batch - the first of the probe sequence, so the same
    // keys at every N for the same seed and span - resolved in a process whose
    // file cache is empty, and the library's own counters read back. The answers
    // are digested so that two geometries can be shown to have answered alike.
    if (o.diagnose) {
        db.reset_search_stats();
        auto const& pb = batches.front();
        std::vector<utxoz::lookup_request> reqs;
        uint64_t by_find = 0;
        for (auto const& k : pb.key) {
            auto const r = db.find(k, query_height);
            if (r) { ++by_find; continue; }
            if (r.error() != utxoz::error_code::not_resolved) {
                fmt::println(stderr, "find failed: error_code={}", int(r.error()));
                return exit_operation_failed;
            }
            reqs.emplace_back(k, query_height);
        }
        auto const res = db.resolve(reqs);
        if ( ! res) {
            fmt::println(stderr, "resolve failed: error_code={}", int(res.error()));
            return exit_operation_failed;
        }
        // Both lists sorted by key first. `found` is an unordered map whose
        // iteration order follows insertion, which follows the walk - so an
        // unsorted digest would differ between geometries that answered alike.
        probe_digest answers;
        std::vector<std::pair<utxoz::raw_outpoint, utxoz::full_find_result const*>> found;
        for (auto const& [k, v] : res->found) found.emplace_back(k, &v);
        std::ranges::sort(found, {}, &decltype(found)::value_type::first);
        for (auto const& [k, v] : found) {
            for (auto const b : k) answers.add(b);
            answers.add(v->block_height);
            for (auto const b : v->data) answers.add(b);
        }
        std::vector<utxoz::raw_outpoint> absent;
        for (auto const& a : res->absent) absent.push_back(a.key);
        std::ranges::sort(absent);
        for (auto const& k : absent) for (auto const b : k) answers.add(b);

        auto const stats = db.get_statistics();
        std::string classes;
        uint64_t files = 0, hits = 0, probes = 0, answered = 0;
        for (auto const& c : stats.lookups.classes) {
            files += c.files_opened; hits += c.cache_hits;
            probes += c.generations_probed; answered += c.resolved_historical;
            classes += fmt::format(
                "    {{\"class\": {}, \"files_opened\": {}, \"cache_hits\": {}, "
                "\"generations_probed\": {}, \"resolved_historical\": {}}},\n",
                c.container_class, c.files_opened, c.cache_hits, c.generations_probed,
                c.resolved_historical);
        }
        if (classes.size() >= 2) classes.erase(classes.size() - 2, 1);
        fmt::print(
            "{{\n"
            "  \"schema\": \"utxoz-calib-hist-diagnose-1\",\n"
            "  \"metric\": \"{}\",\n  \"shard_count\": {},\n  \"statistics_level\": {},\n"
            "  \"seed\": {},\n  \"probe_below\": {},\n  \"batch\": {},\n"
            "  \"probe_digest\": \"{:016x}\",\n  \"answered_by_find\": {},\n"
            "  \"found\": {},\n  \"absent\": {},\n  \"answer_digest\": \"{:016x}\",\n"
            "  \"files_opened\": {},\n  \"cache_hits\": {},\n"
            "  \"generations_probed\": {},\n  \"resolved_historical\": {},\n"
            "  \"classes\": [\n{}  ]\n}}\n",
            o.metric, utxoz::shard_count, UTXOZ_STATISTICS_LEVEL, o.seed, o.probe_below,
            o.batch, digest.h, by_find, res->found.size(), res->absent.size(), answers.h,
            files, hits, probes, answered, classes);
        db.close();
        return exit_ok;
    }

    // One pass over a batch list. The only work inside it beyond the two library
    // calls is the outcome counting, which is declared: a single has_value() is
    // what cannot tell a fault from an unanswerable lookup.
    auto run = [&](std::vector<probe_batch> const& list, bool count_extra) {
        tally t;
        std::vector<utxoz::lookup_request> reqs;
        reqs.reserve(o.batch);
        for (auto const& pb : list) {
            if (pb.extra) t.extra_ops += o.batch; else t.base_ops += o.batch;
            reqs.clear();
            for (auto const& k : pb.key) {
                auto const r = db.find(k, query_height);
                switch (classify(r)) {
                    case find_outcome::resolved:     ++t.resolved_by_find; break;
                    case find_outcome::not_resolved: reqs.emplace_back(k, query_height); break;
                    case find_outcome::not_found:    ++t.not_found_from_find; break;
                    case find_outcome::real_error:
                        if (t.real_errors == 0) t.first_error = int(r.error());
                        ++t.real_errors;
                        break;
                }
            }
            if (reqs.empty()) continue;
            auto const res = db.resolve(reqs);
            if (classify_resolution(res) == resolve_outcome::real_error) {
                if (t.real_errors == 0) t.first_error = int(res.error());
                t.real_errors += reqs.size();
                continue;
            }
            t.found += res->found.size();
            t.absent += res->absent.size();
        }
        if ( ! count_extra) { t.base_ops += t.extra_ops; t.extra_ops = 0; }
        return t;
    };

    // ----------------------------------------------------------------- warmup
    // Not timed. Its outcomes are counted, because a warmup that was not doing
    // what the measurement does is not a warmup.
    auto const warm = o.warmup == 0 ? tally{} : run(warm_batches, false);
    if (warm.real_errors != 0) {
        fmt::println(stderr, "the warmup saw {} real errors (first: error_code={}); "
                     "no measurement was attempted", warm.real_errors, warm.first_error);
        return exit_operation_failed;
    }

    // ---------------------------------------------------------------- measure
    auto const t0 = std::chrono::steady_clock::now();
    auto const m = run(batches, true);
    auto const t1 = std::chrono::steady_clock::now();
    auto const elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    // --------------------------------------------------------------- validate
    // Outside the timing, and a failure here is an exit code rather than a note.
    bool valid = true;
    std::string why;
    auto const total_ops = o.ops + extra_ops;

    if (m.real_errors != 0) {
        valid = false;
        why += fmt::format("{} lookups failed with a real error (first: error_code={}); ",
                           m.real_errors, m.first_error);
    }
    if (m.base_ops != o.ops || m.extra_ops != extra_ops) {
        valid = false;
        why += fmt::format("the loop made {} base and {} extra lookups, not {} and {}; ",
                           m.base_ops, m.extra_ops, o.ops, extra_ops);
    }
    // Every probe is historical by construction, so nothing may be answered by
    // the active versions. One that is means the probe set was drawn wrong, and
    // the figure would be a blend of two different operations.
    if (m.resolved_by_find != 0) {
        valid = false;
        why += fmt::format("{} probes were answered by the active versions, so they "
                           "were not historical; ", m.resolved_by_find);
    }
    if (m.not_found_from_find != 0) {
        valid = false;
        why += fmt::format("{} find() calls returned not_found, which this fixture "
                           "cannot produce; ", m.not_found_from_find);
    }

    uint64_t verified_payload = 0;
    uint64_t verified_height = 0;
    uint64_t verified_absent = 0;

    if (o.metric == "hist_hit") {
        if (m.found != total_ops) {
            valid = false;
            why += fmt::format("resolve() found {} of {} historical entries; ",
                               m.found, total_ops);
        }
        if (m.absent != 0) {
            valid = false;
            why += fmt::format("resolve() called {} stored entries absent; ", m.absent);
        }
        if (warm.found != o.warmup && o.warmup != 0) {
            valid = false;
            why += fmt::format("the warmup found {} of {}; ", warm.found, o.warmup);
        }

        // A SECOND PASS, after the clock stopped. It does not inspect what the
        // timed loop received — those results are gone — it repeats the same
        // probes and checks the answers, so it establishes that the image answers
        // correctly for this sequence, not that the measured calls did.
        //
        // Two distinguishable fields per key: the payload encodes its own index
        // and the height encodes its block. An answer carrying another entry's
        // value or another entry's height fails here.
        std::vector<utxoz::lookup_request> reqs;
        for (auto const& pb : batches) {
            reqs.clear();
            for (auto const& k : pb.key) reqs.emplace_back(k, query_height);
            auto const res = db.resolve(reqs);
            if ( ! res) {
                valid = false;
                why += fmt::format("the verification resolve failed (error_code={}); ",
                                   int(res.error()));
                break;
            }
            for (size_t i = 0; i != pb.key.size(); ++i) {
                auto const n = pb.index[i];
                auto const it = res->found.find(pb.key[i]);
                if (it == res->found.end()) continue;
                auto const expected = value_for(
                    o.selftest_wrong_payload ? n + 1 : n, o.value_size);
                auto const& data = it->second.data;
                if (data.size() == expected.size()
                    && std::equal(data.begin(), data.end(), expected.begin()))
                    ++verified_payload;
                if (it->second.block_height == height_of(n)) ++verified_height;
            }
        }
        if (valid && (verified_payload != total_ops || verified_height != total_ops)) {
            valid = false;
            why += fmt::format("the verification pass confirmed {} payloads and {} "
                               "heights of {}; ", verified_payload, verified_height,
                               total_ops);
        }
    } else {
        // A miss is PROVEN, or it is not a miss. `not_resolved` from find() only
        // says the active versions could not answer; `absent` from resolve() says
        // every version that could hold the key was read and none did.
        auto const want_found = o.selftest_miss_as_hit;
        if ( ! want_found && m.absent != total_ops) {
            valid = false;
            why += fmt::format("resolve() proved {} of {} absences; the rest were not "
                               "established; ", m.absent, total_ops);
        }
        if (want_found && m.found != total_ops) {
            valid = false;
            why += fmt::format("--selftest-miss-as-hit: resolve() found {} of {}; ",
                               m.found, total_ops);
        }
        if ( ! want_found && m.found != 0) {
            valid = false;
            why += fmt::format("resolve() found {} keys that were never inserted; ",
                               m.found);
        }
        if (o.warmup != 0 && warm.absent != o.warmup) {
            valid = false;
            why += fmt::format("the warmup proved {} of {} absences; ",
                               warm.absent, o.warmup);
        }
        verified_absent = m.absent;
    }

    auto const json = fmt::format(
        "{{\n"
        "  \"schema\": \"utxoz-calib-hist-2\",\n"
        "  \"metric\": \"{}\",\n"
        "  \"shard_count\": {},\n"
        "  \"ops\": {},\n"
        "  \"extra_ops\": {},\n"
        "  \"extra_pct\": {},\n"
        "  \"total_ops\": {},\n"
        "  \"warmup\": {},\n"
        "  \"batch\": {},\n"
        "  \"resolve_calls\": {},\n"
        "  \"population_observed\": {},\n"
        "  \"historical_prefix\": {},\n"
        "  \"active_entries\": {},\n"
        "  \"probe_below\": {},\n"
        "  \"probe_digest\": \"{:016x}\",\n"
        "  \"geometry_id\": {},\n"
        "  \"generations_class_48\": {},\n"
        "  \"version_files_all_classes\": {},\n"
        "  \"version_files_class_48\": {},\n"
        "  \"value_size\": {},\n"
        "  \"seed\": {},\n"
        "  \"query_height\": {},\n"
        "  \"cache\": \"warm per process; the warmup walks the same generations. "
        "The library's file cache holds one file (its default), so every batch "
        "re-maps the history: that is the code as it stands, not a tuning.\",\n"
        "  \"denominator\": \"elapsed / ops, where ops counts BASE lookups only. "
        "Extra control work is real and inside the timing but outside the "
        "denominator, so +N% work raises ns_per_op by whatever it costs - NOT by "
        "a declared N%.\",\n"
        "  \"elapsed_ns\": {},\n"
        "  \"ns_per_op\": {:.4f},\n"
        "  \"resolved_by_find\": {},\n"
        "  \"found\": {},\n"
        "  \"absent\": {},\n"
        "  \"not_found_from_find\": {},\n"
        "  \"real_errors\": {},\n"
        "  \"first_error_code\": {},\n"
        "  \"warmup_found\": {},\n"
        "  \"warmup_absent\": {},\n"
        "  \"warmup_resolved_by_find\": {},\n"
        "  \"verified_payload\": {},\n"
        "  \"verified_height\": {},\n"
        "  \"verified_absent\": {},\n"
        "  \"valid\": {},\n"
        "  \"invalid_because\": \"{}\"\n"
        "}}\n",
        o.metric, utxoz::shard_count, o.ops, extra_ops, o.extra_pct, total_ops,
        o.warmup, o.batch, batches.size(), population, s.historical_prefix,
        s.active_entries, o.probe_below, digest.h, s.geometry_id,
        s.generations_class_48, files.all, files.sharded,
        o.value_size, o.seed, query_height, elapsed_ns,
        double(elapsed_ns) / double(o.ops), m.resolved_by_find, m.found, m.absent,
        m.not_found_from_find, m.real_errors, m.first_error,
        warm.found, warm.absent, warm.resolved_by_find,
        verified_payload, verified_height, verified_absent,
        valid ? "true" : "false", why);

    std::fputs(json.c_str(), stdout);
    if ( ! o.out.empty() && ! write_file(o.out, json)) {
        fmt::println(stderr, "could not write {}", o.out);
        return exit_setup_failed;
    }

    // The image is shared and is NOT removed: the next sample measures the same
    // bytes. Nothing here wrote to it.
    db.close();

    if ( ! valid) {
        fmt::println(stderr, "the operation did not do what was asked: {}", why);
        return exit_operation_failed;
    }
    return exit_ok;
}
