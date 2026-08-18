// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file verify_unique.cpp
 * @brief Does this database hold any outpoint twice? Answered from outside.
 *
 * Like the census tool, it opens the directory through the library —
 * `open_for_inspection()`, which validates the config and the stamps and takes
 * the directory claim — and asks the library the question. It does not read the
 * format itself: a tool with its own idea of what a segment looks like is a
 * second implementation of the format, and the day they disagree the one nobody
 * is testing wins.
 *
 * ## The exit status carries the verdict
 *
 *     0   no outpoint is stored more than once
 *     2   some outpoint is stored more than once
 *     1   the verification could not be completed
 *
 * Three states and not two, because "the answer is no" and "there is no answer"
 * send an operator to entirely different places, and a script that treats every
 * non-zero status alike would take a database it could not open for a database
 * it had condemned.
 *
 * This is also why it is a separate binary rather than a flag on the census: a
 * census that started returning 2 because of what it found would break every
 * caller that reads a non-zero status as "the tool failed".
 *
 * ## Outpoints are withheld unless asked for
 *
 * An outpoint identifies a transaction output on a public chain. By default the
 * findings say how many copies a key has and where they are, and not which key
 * it is, so that the output can be pasted into an issue without a second
 * thought. `--include-outpoints` puts them in, for whoever is actually going to
 * look at the entries.
 */

#include <cstdio>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

#include <utxoz/database.hpp>
#include <utxoz/types.hpp>
#include <utxoz/uniqueness.hpp>

namespace fs = std::filesystem;

namespace {

/// Exit statuses, named where they are decided so that the meaning and the
/// number cannot drift apart.
constexpr int exit_unique = 0;
constexpr int exit_cannot_tell = 1;
constexpr int exit_duplicates = 2;

/// A plain decimal count. Refused rather than clamped: a budget that silently
/// became zero would turn into a refusal much later, blamed on the database.
bool parse_count(std::string_view text, uint64_t& out) {
    if (text.empty()) return false;
    uint64_t value = 0;
    for (char const c : text) {
        if (c < '0' || c > '9') return false;
        uint64_t const digit = uint64_t(c - '0');
        if (value > (std::numeric_limits<uint64_t>::max() - digit) / 10) return false;
        value = value * 10 + digit;
    }
    out = value;
    return true;
}

/// The options that take a number, and where each one lands. One place, so the
/// name and the field cannot drift apart and the value offset is never written
/// down twice.
constexpr std::pair<char const*, uint64_t utxoz::verify_options::*> numeric_options[] = {
    {"--memory-budget=", &utxoz::verify_options::memory_budget},
    {"--max-passes=",    &utxoz::verify_options::max_data_passes},
    {"--max-findings=",  &utxoz::verify_options::max_findings},
    {"--max-locations=", &utxoz::verify_options::max_locations_per_finding},
};

/// Takes the stream so that `--help` goes to stdout and a refusal goes to
/// stderr. Asking for the help text is not an error, and a caller who pipes it
/// should get it in the pipe.
///
/// The defaults are printed from `verify_options` rather than written out again.
/// A help text that restates them is correct until somebody changes one, and
/// then it is a document that confidently says the wrong number.
void usage(std::FILE* to) {
    utxoz::verify_options const defaults;
    std::fputs(
        "utxoz_verify_unique — is any outpoint stored more than once?\n"
        "\n"
        "usage: utxoz_verify_unique [options] <database-directory>\n"
        "\n"
        "exit: 0 unique, 2 duplicates found, 1 could not be verified\n"
        "\n"
        "options:\n"
        "  --json                  machine-readable (default)\n"
        "  --text                  for a person\n"
        "  --mode=full|reference   which storage mode to open as (default full)\n"
        "  --include-outpoints     put the outpoints in the findings. Off by\n"
        "                          default: they identify outputs on a public chain\n",
        to);
    std::fprintf(to,
        "  --memory-budget=BYTES   ceiling for the walk (default %llu).\n"
        "                          It refuses rather than exceeding it\n"
        "  --max-passes=N          refuse before starting if the budget would need\n"
        "                          more passes than this (default %llu)\n"
        "  --max-findings=N        how many duplicated keys to describe (default %llu)\n"
        "  --max-locations=N       how many copies of one key to locate (default %llu)\n",
        (unsigned long long) defaults.memory_budget,
        (unsigned long long) defaults.max_data_passes,
        (unsigned long long) defaults.max_findings,
        (unsigned long long) defaults.max_locations_per_finding);
    std::fputs(
        "\n"
        "The directory claim is exclusive: a running node holds it and this will\n"
        "refuse rather than read underneath it. Re-reading the data once per\n"
        "partition group is what a small budget costs; the report says how many\n"
        "passes it took.\n",
        to);
}

template <typename Db>
int run(fs::path const& dir, utxoz::verify_options const& options, bool as_json) {
    auto opened = Db::open_for_inspection(dir);
    if ( ! opened) {
        if (opened.error() == utxoz::error_code::database_not_found) {
            std::fprintf(stderr, "verify: there is no database at %s. Refusing rather "
                                 "than creating one.\n", dir.string().c_str());
        } else {
            std::fprintf(stderr, "verify: cannot open %s: error %d\n",
                         dir.string().c_str(), int(opened.error()));
        }
        return exit_cannot_tell;
    }
    auto db = std::move(*opened);

    auto report = db.verify_unique_outpoints(options);
    if ( ! report) {
        if (report.error() == utxoz::error_code::insufficient_space) {
            // The one failure the person running this can act on, so it says how.
            // The same code covers three causes and the advice has to cover all
            // three, or an operator follows it and is refused again for the reason
            // it did not mention.
            std::fputs("verify: this run could not be made to fit. Raise --memory-budget; "
                       "or --max-passes, if the budget is fine and the number of passes "
                       "is what was refused; or lower --max-findings and --max-locations, "
                       "if what did not fit was the sample they ask for.\n", stderr);
        } else {
            std::fprintf(stderr, "verify: the walk failed: error %d.\n", int(report.error()));
        }
        std::fputs("No partial report is produced, and no verdict either: a verdict "
                   "computed from part of a database is not a verdict.\n", stderr);
        db.close();
        return exit_cannot_tell;
    }

    auto const out = as_json ? utxoz::to_json(*report) : utxoz::to_text(*report);
    // Checked, because the exit status is only half the answer. A full pipe or a
    // full disk truncates the report while the status still says "unique", and a
    // caller reading the status would act on a document it never fully received.
    bool const written = std::fwrite(out.data(), 1, out.size(), stdout) == out.size()
        && std::fflush(stdout) == 0;
    bool const unique = report->unique;
    db.close();
    if ( ! written) {
        std::fputs("verify: the report could not be written in full, so the verdict is "
                   "not reported either.\n", stderr);
        return exit_cannot_tell;
    }
    return unique ? exit_unique : exit_duplicates;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) { usage(stderr); return exit_cannot_tell; }

    fs::path dir;
    bool as_json = true;
    bool reference = false;
    utxoz::verify_options options;

    for (int i = 1; i < argc; ++i) {
        std::string_view const arg = argv[i];
        if (arg == "--json") as_json = true;
        else if (arg == "--text") as_json = false;
        else if (arg == "--mode=full") reference = false;
        else if (arg == "--mode=reference") reference = true;
        else if (arg == "--include-outpoints") options.include_outpoints = true;
        else if (arg == "--help" || arg == "-h") { usage(stdout); return exit_unique; }
        else if (arg.starts_with("-")) {
            // Everything that starts with a dash, not only a double one. `-json`
            // or a bare `-` would otherwise be taken for the directory, and the
            // tool would go looking for a database by that name instead of saying
            // the option was not understood. A path that really begins with a
            // dash can be given as ./-name.
            //
            // The value starts where the prefix ends, taken from the prefix
            // rather than written out again: a hard-coded offset is correct until
            // somebody renames the option, and then it silently eats a digit.
            bool matched = false;
            for (auto const& [prefix, field] : numeric_options) {
                if ( ! arg.starts_with(prefix)) continue;
                matched = true;
                if ( ! parse_count(arg.substr(std::string_view(prefix).size()),
                                   options.*field)) {
                    std::fprintf(stderr, "verify: %s needs a number\n", prefix);
                    return exit_cannot_tell;
                }
                break;
            }
            if ( ! matched) {
                std::fprintf(stderr, "verify: unknown option %.*s\n",
                             int(arg.size()), arg.data());
                return exit_cannot_tell;
            }
        }
        else if (dir.empty()) dir = fs::path(arg);
        else { std::fputs("verify: more than one directory given\n", stderr); return exit_cannot_tell; }
    }

    if (dir.empty()) { usage(stderr); return exit_cannot_tell; }

    // The error_code overload: "the filesystem would not answer" and "there is
    // nothing there" are different problems and send a person to different
    // places.
    std::error_code ec;
    if ( ! fs::exists(dir, ec)) {
        if (ec) {
            std::fprintf(stderr, "verify: cannot examine %s: %s\n",
                         dir.string().c_str(), ec.message().c_str());
        } else {
            std::fprintf(stderr, "verify: %s does not exist\n", dir.string().c_str());
        }
        return exit_cannot_tell;
    }

    return reference ? run<utxoz::reference_db>(dir, options, as_json)
                     : run<utxoz::full_db>(dir, options, as_json);
}
