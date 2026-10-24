// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file calib_stationary.cpp
 * @brief One stationary metric, measured once, with its conditions declared.
 *
 * This exists because the regular benchmark suite cannot answer the question a
 * calibration asks. Its insert and erase cases mutate the fixture they measure,
 * so every epoch runs against a different store: the dispersion then describes
 * the fixture's decay as much as the code's cost, and no number of repetitions
 * separates the two. What a calibration needs is an operation that leaves the
 * store exactly as it found it, repeated against the same state.
 *
 * So this program measures read-only metrics only, and says so. It prints one
 * JSON document per run and nothing else on stdout.
 *
 * Three rules it follows, each because their absence has misled before:
 *
 * 1. **Validation happens outside the timing, and a failure is an exit code.**
 *    A harness that prints a warning and exits 0 produces a figure for an
 *    operation that did not happen, and a caller reading the status cannot tell.
 * 2. **Setup and warmup are not in the elapsed time.** The fixture is built and
 *    closed, reopened, and warmed, before the clock starts.
 * 3. **The operating point is part of the result.** A population that fits in
 *    cache calibrates that population. It says nothing about a store of a
 *    hundred million entries across thirty-seven generations, and the JSON
 *    carries the numbers needed to see which one was measured.
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

#include <unistd.h>

#include <fmt/format.h>

#include "calib_args.hpp"

#include <utxoz/database.hpp>
#include <utxoz/types.hpp>

namespace fs = std::filesystem;

namespace {

constexpr int exit_ok = 0;
constexpr int exit_usage = 2;
constexpr int exit_operation_failed = 3;
constexpr int exit_setup_failed = 4;

/// A key derived from `n` alone, so that a run is reproducible from its seed and
/// a hit set and a miss set can be kept provably disjoint.
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

/// What one find() answered, as four cases rather than two.
///
/// Extracted from the measured loop so that it can be tested without a
/// database: a classifier that only ever runs inside a timed loop against a
/// healthy store has never been shown to tell a fault from an unanswerable
/// lookup, which is the one distinction it exists to make.
enum class find_outcome { resolved, not_resolved, not_found, real_error };

[[nodiscard]] find_outcome classify(utxoz::result<utxoz::full_find_result> const& r) {
    if (r) return find_outcome::resolved;
    switch (r.error()) {
        case utxoz::error_code::not_resolved: return find_outcome::not_resolved;
        case utxoz::error_code::not_found:    return find_outcome::not_found;
        default:                              return find_outcome::real_error;
    }
}

/// Feeds the classifier each case it has to tell apart, including a real error,
/// without needing a database in any particular state.
/// Writes body to p. False when it could not be opened, written in full or
/// closed: a full disk shows up at fclose, when the buffer is flushed.
[[nodiscard]] bool write_file(fs::path const& p, std::string const& body) {
    FILE* f = std::fopen(p.string().c_str(), "w");
    if ( ! f) return false;
    bool const wrote = std::fwrite(body.data(), 1, body.size(), f) == body.size();
    return std::fclose(f) == 0 && wrote;
}

/// write_file must report a full device and a missing directory, and write a
/// writable path in full. 0 when it does.
[[nodiscard]] int selftest_write_file() {
    int bad = 0;
    if (fs::exists("/dev/full") && write_file("/dev/full", "{}\n")) {
        fmt::println(stderr, "write_file accepted a write to /dev/full"); ++bad;
    }
    if (write_file("/nonexistent-utxoz-dir/out.json", "{}\n")) {
        fmt::println(stderr, "write_file accepted a missing directory"); ++bad;
    }
    auto const path = fs::temp_directory_path()
                    / fmt::format("utxoz-calib-write-selftest-{}", std::random_device{}());
    std::string const body = "{\"schema\": \"selftest\"}\n";
    if ( ! write_file(path, body)) {
        fmt::println(stderr, "write_file refused {}", path.string()); ++bad;
    } else if (FILE* f = std::fopen(path.string().c_str(), "r")) {
        std::string back(body.size() + 1, '\0');
        back.resize(std::fread(back.data(), 1, back.size(), f)); std::fclose(f);
        if (back != body) { fmt::println(stderr, "write_file wrote something else"); ++bad; }
    }
    std::error_code ec;
    fs::remove(path, ec);
    return bad;
}

[[nodiscard]] int selftest_classifier() {
    int bad = 0;
    auto check = [&](char const* what, find_outcome got, find_outcome want) {
        if (got != want) { fmt::println(stderr, "classifier: {} misclassified", what); ++bad; }
        else fmt::println("  classifier {:<34} ok", what);
    };

    utxoz::full_find_result const resolved_value{};
    check("a resolved result", classify(utxoz::result<utxoz::full_find_result>{resolved_value}),
          find_outcome::resolved);
    check("not_resolved is not an error",
          classify(std::unexpected(utxoz::error_code::not_resolved)),
          find_outcome::not_resolved);
    check("not_found is its own case",
          classify(std::unexpected(utxoz::error_code::not_found)),
          find_outcome::not_found);
    // Three genuinely different faults, none of which may read as a miss.
    check("version_unreadable is a real error",
          classify(std::unexpected(utxoz::error_code::version_unreadable)),
          find_outcome::real_error);
    check("file_open_failed is a real error",
          classify(std::unexpected(utxoz::error_code::file_open_failed)),
          find_outcome::real_error);
    check("closed is a real error",
          classify(std::unexpected(utxoz::error_code::closed)),
          find_outcome::real_error);
    return bad;
}

/// The payload stored for entry `n`.
///
/// It encodes `n`, and that is the point. With one payload shared by every
/// entry, a verification pass that compares bytes cannot tell a lookup that
/// answered with another key's value from one that answered correctly — the
/// bytes are identical. An earlier version of this file claimed to catch that
/// and could not. The size is fixed so the size class does not move.
std::vector<uint8_t> value_for(uint64_t n, size_t size) {
    std::vector<uint8_t> v(size);
    for (size_t i = 0; i != size; ++i) v[i] = uint8_t(i * 7 + 1);
    for (size_t b = 0; b != 8 && b < size; ++b) v[b] = uint8_t(n >> (8 * b));
    return v;
}

/// Keys stored by the fixture occupy [0, population). Miss keys start well past
/// that, so a miss is a miss by construction rather than by luck.
constexpr uint64_t miss_key_base = 1ull << 40;

/// Version files, split two ways because one number would mislead. Every class
/// has at least one container, so a count over all of them is five at N=1 before
/// anything rotates — which is not "five generations" of anything.
struct file_counts {
    size_t all = 0;        ///< every container of every class
    size_t sharded = 0;    ///< the 48-byte class only, across its shards
};

file_counts count_version_files(fs::path const& dir) {
    file_counts c;
    std::error_code ec;
    for (auto const& e : fs::directory_iterator(dir, ec)) {
        auto const name = e.path().filename().string();
        if (name.rfind("cont_", 0) != 0 || ! name.ends_with(".dat")) continue;
        ++c.all;
        // "cont_0_v…" unsharded, "cont_0s<k>_v…" sharded: both name class 0.
        if (name.rfind("cont_0_v", 0) == 0 || name.rfind("cont_0s", 0) == 0) ++c.sharded;
    }
    return c;
}

struct options {
    std::string metric;
    uint64_t ops = 200'000;
    uint64_t warmup = 20'000;
    uint64_t population = 90'000;
    uint64_t seed = 1;
    size_t value_size = 33;
    std::string out;
    std::string work = ".";
    /// Self-test only: compare hits against a payload the fixture was
    /// not built with, so a passing content check must now fail. A check
    /// that has never been seen to fail has not been shown to work.
    bool selftest_wrong_payload = false;
};

int usage(FILE* to) {
    fmt::println(to,
        "usage: utxoz_calib --metric find_hit|find_miss [options]\n"
        "\n"
        "  --ops N          lookups inside the timed region (default 200000)\n"
        "  --warmup N       lookups before it, not timed (default 20000)\n"
        "  --population N   entries the fixture holds (default 90000)\n"
        "  --seed N         fixes the lookup order (default 1)\n"
        "  --value-size N   payload bytes, selects the size class (default 33)\n"
        "  --work DIR       where the fixture is built (default .)\n"
        "  --out FILE       write the JSON here as well as to stdout\n"
        "  --selftest-wrong-payload  expect each answer to carry its NEIGHBOUR's\n"
        "                   payload, so the content check is seen to fail (exit 3)\n"
        "  --selftest-classifier     test the outcome classifier and exit\n"
        "  --selftest-write-file     test that a failed --out write is reported, and exit\n"
        "\n"
        "Measures read-only metrics only. insert, apply_deletes and resolve are\n"
        "not implemented here: each needs the fixture restored between\n"
        "repetitions, which is a different harness and is declared pending.\n"
        "\n"
        "exit: 0 measured, 2 usage, 3 an operation did not do what was asked,\n"
        "      4 the fixture could not be built\n");
    return to == stdout ? exit_ok : exit_usage;
}

} // namespace

int main(int argc, char** argv) {
    options o;
    for (int i = 1; i < argc; ++i) {
        std::string const a = argv[i];
        // The whole value, digits only, in range; a missing or malformed one
        // is a usage error before any work (calib_args.hpp).
        auto next = [&](uint64_t& dst, uint64_t max = calib::max_count) {
            if (i + 1 >= argc) { fmt::println(stderr, "{} needs a value", a); std::exit(exit_usage); }
            auto const v = calib::parse_whole(argv[++i], max);
            if ( ! v) {
                fmt::println(stderr, "{} must be a whole number from 0 to {}, not '{}'", a, max, argv[i]);
                std::exit(exit_usage);
            }
            dst = *v;
        };
        if (a == "--metric" && i + 1 < argc) o.metric = argv[++i];
        else if (a == "--ops") next(o.ops);
        else if (a == "--warmup") next(o.warmup);
        else if (a == "--population") next(o.population);
        else if (a == "--seed") next(o.seed, calib::max_seed);
        else if (a == "--value-size") { uint64_t v = 0; next(v, calib::max_value_size); o.value_size = size_t(v); }
        else if (a == "--work" && i + 1 < argc) o.work = argv[++i];
        else if (a == "--out" && i + 1 < argc) o.out = argv[++i];
        else if (a == "--selftest-wrong-payload") o.selftest_wrong_payload = true;
        else if (a == "--selftest-classifier") return selftest_classifier() == 0 ? exit_ok : exit_operation_failed;
        else if (a == "--selftest-write-file") return selftest_write_file() == 0 ? exit_ok : exit_operation_failed;
        else if (a == "--help" || a == "-h") return usage(stdout);
        else { fmt::println(stderr, "unknown argument: {}", a); return usage(stderr); }
    }
    if (o.metric != "find_hit" && o.metric != "find_miss") {
        fmt::println(stderr, "--metric must be find_hit or find_miss");
        return usage(stderr);
    }
    if (o.ops == 0 || o.population == 0) {
        fmt::println(stderr, "--ops and --population must be positive");
        return exit_usage;
    }
    // Refused now rather than after the measurement: --out in a directory that
    // does not exist. A write that fails anyway is reported after cleanup.
    if ( ! o.out.empty()) {
        auto const parent = fs::path(o.out).parent_path();
        if ( ! fs::is_directory(parent.empty() ? fs::path(".") : parent)) {
            fmt::println(stderr, "--out {}: its directory does not exist", o.out);
            return exit_usage;
        }
    }

    // ---------------------------------------------------------------- setup
    // Not timed. A fresh directory per run, so no run inherits another's cache
    // state or file layout.
    // A scratch this run creates, and refuses to inherit.
    //
    // The previous version built a path from the pid and removed it first. A pid
    // is reused, so that is a recipe for deleting a directory somebody else left
    // behind — possibly the evidence of a failure being read. This one mixes in
    // entropy and fails if the name is taken rather than clearing it.
    std::error_code ec;
    fs::path dir;
    {
        std::random_device rd;
        auto const token = (uint64_t(rd()) << 32) ^ uint64_t(rd());
        dir = fs::path(o.work) /
              fmt::format("calib_{}_{}_{:016x}", o.metric, getpid(), token);
        if (fs::exists(dir)) {
            fmt::println(stderr, "{} already exists; refusing to reuse or remove it",
                         dir.string());
            return exit_setup_failed;
        }
        if ( ! fs::create_directories(dir, ec) || ec) {
            fmt::println(stderr, "could not create the scratch {}: {}",
                         dir.string(), ec.message());
            return exit_setup_failed;
        }
    }

    // Each entry gets its own payload, so a verification pass can tell which
    // entry answered. See value_for().
    {
        auto opened = utxoz::full_db::open_for_testing(dir.string(), true);
        if ( ! opened) {
            fmt::println(stderr, "could not create the fixture at {}", dir.string());
            return exit_setup_failed;
        }
        auto db = std::move(*opened);
        for (uint64_t n = 0; n != o.population; ++n) {
            auto const payload = value_for(n, o.value_size);
            auto const stored = db.insert(key_of(n), utxoz::output_data_span{payload}, 500);
            if ( ! stored || ! *stored) {
                fmt::println(stderr, "fixture insert {} did not store", n);
                return exit_setup_failed;
            }
        }
        db.close();
    }

    auto opened = utxoz::full_db::open_for_testing(dir.string(), false);
    if ( ! opened) {
        fmt::println(stderr, "could not reopen the fixture at {}", dir.string());
        return exit_setup_failed;
    }
    auto db = std::move(*opened);

    auto const population = db.size();
    auto const files = count_version_files(dir);

    // The lookup order, drawn once so that both sides of a pair see the same
    // sequence and the clock never waits on a generator.
    std::mt19937_64 rng(o.seed);
    std::vector<utxoz::raw_outpoint> probes;
    std::vector<uint64_t> probe_index;   // used only after the clock stops
    probes.reserve(o.ops + o.warmup);
    probe_index.reserve(o.ops + o.warmup);
    std::uniform_int_distribution<uint64_t> pick(0, o.population - 1);
    for (uint64_t i = 0; i != o.ops + o.warmup; ++i) {
        auto const n = pick(rng);
        probe_index.push_back(n);
        probes.push_back(o.metric == "find_hit" ? key_of(n)
                                                : key_of(miss_key_base + n));
    }

    // --------------------------------------------------------------- warmup
    // Not timed either. Its outcomes are counted, because a warmup that was not
    // doing what the measurement does is not a warmup.
    //
    // `has_value()` alone would be wrong here, and was. find() answers three
    // ways: it resolved, it could not answer from the active versions
    // (`not_resolved`), or something actually failed. Treating the third as the
    // second reports a figure for an operation that did not happen.
    uint64_t warm_hits = 0;
    uint64_t warm_not_resolved = 0;
    uint64_t warm_not_found = 0;
    uint64_t warm_errors = 0;
    int warm_first_error = 0;
    for (uint64_t i = 0; i != o.warmup; ++i) {
        auto const r = db.find(probes[i], 600);
        switch (classify(r)) {
            case find_outcome::resolved:     ++warm_hits; break;
            case find_outcome::not_resolved: ++warm_not_resolved; break;
            case find_outcome::not_found:    ++warm_not_found; break;
            case find_outcome::real_error:
                if (warm_errors == 0) warm_first_error = int(r.error());
                ++warm_errors;
                break;
        }
    }
    if (warm_errors != 0) {
        fmt::println(stderr, "the warmup saw {} real errors (first: error_code={}); "
                     "no measurement was attempted", warm_errors, warm_first_error);
        return exit_operation_failed;
    }

    uint64_t verified_payload = 0;
    uint64_t verified_height = 0;

    // -------------------------------------------------------------- measure
    // Three counters and one branch on the error code. That is the only work
    // added inside the timed region, and it is declared: the alternative — a
    // single `has_value()` — cannot tell a fault from an unanswerable lookup.
    // The *contents* of a hit are not checked here; that happens afterwards, so
    // that the comparison does not sit in the measured loop.
    uint64_t hits = 0;
    uint64_t not_resolved = 0;
    uint64_t not_found = 0;
    uint64_t real_errors = 0;
    int first_error = 0;
    auto const t0 = std::chrono::steady_clock::now();
    for (uint64_t i = 0; i != o.ops; ++i) {
        auto const r = db.find(probes[o.warmup + i], 600);
        switch (classify(r)) {
            case find_outcome::resolved:     ++hits; break;
            case find_outcome::not_resolved: ++not_resolved; break;
            case find_outcome::not_found:    ++not_found; break;
            case find_outcome::real_error:
                if (real_errors == 0) first_error = int(r.error());
                ++real_errors;
                break;
        }
    }
    auto const t1 = std::chrono::steady_clock::now();
    auto const elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    // ------------------------------------------------------------- validate
    // Outside the timing, and a failure here is an exit code rather than a note.
    bool valid = true;
    std::string why;

    if (real_errors != 0) {
        valid = false;
        why += fmt::format("{} lookups failed with a real error (first: error_code={}); ",
                           real_errors, first_error);
    }

    if (o.metric == "find_hit") {
        if (hits != o.ops) {
            valid = false;
            why += fmt::format("{} of {} lookups did not resolve; ",
                               o.ops - hits, o.ops);
        }
        if (warm_hits != o.warmup) {
            valid = false;
            why += fmt::format("the warmup resolved {} of {}; ", warm_hits, o.warmup);
        }

        // A SECOND PASS, after the clock stopped.
        //
        // It does not inspect what the timed loop received — those results are
        // gone — it repeats the same probe sequence and checks the answers. So
        // it establishes that the fixture answers correctly for this sequence,
        // not that the measured calls did. Saying otherwise would be claiming
        // more than the code does.
        //
        // What it can now catch, and could not before: an answer carrying
        // another entry's value. Each payload encodes its own index, so the
        // comparison is against the payload of the key that was asked for
        // rather than against one buffer every entry shares.
        uint64_t wrong_payload = 0;
        uint64_t wrong_height = 0;
        for (uint64_t i = 0; i != o.ops; ++i) {
            auto const n = probe_index[o.warmup + i];
            auto const expected = o.selftest_wrong_payload
                ? value_for(n + 1, o.value_size)   // self-test: a neighbour's payload
                : value_for(n, o.value_size);
            auto const r = db.find(probes[o.warmup + i], 600);
            if ( ! r) { ++wrong_payload; continue; }
            auto const data = r->data;
            if (data.size() != expected.size()
                || ! std::equal(data.begin(), data.end(), expected.begin())) {
                ++wrong_payload;
            }
            if (r->block_height != 500) ++wrong_height;
        }
        if (wrong_payload != 0 || wrong_height != 0) {
            valid = false;
            why += fmt::format("{} answers carried the wrong entry's payload and {} "
                               "the wrong height; ", wrong_payload, wrong_height);
        }
        verified_payload = o.ops - wrong_payload;
        verified_height = o.ops - wrong_height;
    } else {
        if (not_resolved + not_found != o.ops) {
            valid = false;
            why += fmt::format("{} of {} lookups resolved when none should; ",
                               hits, o.ops);
        }
        if (warm_not_resolved + warm_not_found != o.warmup) {
            valid = false;
            why += fmt::format("the warmup resolved {} of {}; ",
                               warm_hits, o.warmup);
        }
    }

    if (population != o.population) {
        valid = false;
        why += fmt::format("population {} is not the {} requested; ",
                           population, o.population);
    }

    auto const json = fmt::format(
        "{{\n"
        "  \"schema\": \"utxoz-calib-1\",\n"
        "  \"metric\": \"{}\",\n"
        "  \"shard_count\": {},\n"
        "  \"ops\": {},\n"
        "  \"warmup\": {},\n"
        "  \"population_requested\": {},\n"
        "  \"population_observed\": {},\n"
        "  \"version_files_all_classes\": {},\n"
        "  \"version_files_class_48\": {},\n"
        "  \"value_size\": {},\n"
        "  \"seed\": {},\n"
        "  \"cache\": \"warm (the warmup touches the same distribution)\",\n"
        "  \"elapsed_ns\": {},\n"
        "  \"ns_per_op\": {:.4f},\n"
        "  \"hits\": {},\n"
        "  \"not_resolved\": {},\n"
        "  \"not_found\": {},\n"
        "  \"real_errors\": {},\n"
        "  \"first_error_code\": {},\n"
        "  \"warmup_hits\": {},\n"
        "  \"warmup_not_resolved\": {},\n"
        "  \"warmup_not_found\": {},\n"
        "  \"verified_payload\": {},\n"
        "  \"verified_height\": {},\n"
        "  \"valid\": {},\n"
        "  \"invalid_because\": \"{}\"\n"
        "}}\n",
        o.metric, utxoz::shard_count, o.ops, o.warmup, o.population, population,
        files.all, files.sharded, o.value_size, o.seed, elapsed_ns,
        double(elapsed_ns) / double(o.ops), hits, not_resolved, not_found,
        real_errors, first_error, warm_hits, warm_not_resolved, warm_not_found,
        verified_payload, verified_height,
        valid ? "true" : "false", why);

    std::fputs(json.c_str(), stdout);
    bool const out_written = o.out.empty() || write_file(o.out, json);

    db.close();
    fs::remove_all(dir, ec);

    // After the cleanup, so a failed write leaves no scratch behind; a report
    // that was asked for and not written is never a success.
    if ( ! out_written)
        fmt::println(stderr, "could not write --out {}: the report is only on stdout", o.out);
    if ( ! valid) {
        fmt::println(stderr, "the operation did not do what was asked: {}", why);
        return exit_operation_failed;
    }
    return out_written ? exit_ok : exit_setup_failed;
}
