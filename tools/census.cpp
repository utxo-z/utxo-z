// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file census.cpp
 * @brief Count what a database holds, from outside the process that wrote it.
 *
 * The library can census itself; this exists because the database worth
 * censusing usually belongs to a node, and the person who wants the numbers is
 * not the node. It opens the directory through the library — `open_for_inspection()`,
 * which is what validates the config and the stamps and takes the directory
 * claim — and then asks for the census. **It does not read the format itself.** A tool with
 * its own idea of what a segment looks like is a second implementation of the
 * format, and the day they disagree the one nobody is testing wins.
 *
 * Consequences, stated because they will bite otherwise:
 *
 *  - the claim is exclusive, so a running node holds it and this will refuse.
 *    That is the intended behaviour. Stop the node, or copy the directory;
 *  - if you copy it, the report describes the copy. Whether the copy is a
 *    consistent moment of the original is a property of how it was made — of
 *    whether the node was writing at the time — and nothing here can see that.
 *    Pass --snapshot to have it recorded in the output;
 *  - `open_for_inspection()` may write. It does not create a database, which is the
 *    promise it makes; it does take the lock file, and it does settle an
 *    interrupted merge in a database that is there. It is not a read-only
 *    inspection and is not offered as one.
 */

#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <utxoz/census.hpp>
#include <utxoz/database.hpp>
#include <utxoz/types.hpp>

namespace fs = std::filesystem;

namespace {

void usage() {
    std::fputs(
        "utxoz_census — what a database holds, counted from its files\n"
        "\n"
        "  utxoz_census <directory> [options]\n"
        "\n"
        "  --mode=full|reference   which storage mode the database is in (default full)\n"
        "  --json                  machine-readable output (default)\n"
        "  --text                  the same report for a person\n"
        "  --no-physical-blocks    skip the per-file stat that asks the filesystem\n"
        "                          how many blocks it actually gave the file\n"
        "  --no-generation-detail  per-class totals only\n"
        "  --snapshot              record that this is a copy whose consistency\n"
        "                          depends on how it was taken\n"
        "\n"
        "The directory claim is exclusive: a running node holds it and this will\n"
        "refuse rather than read underneath it.\n",
        stderr);
}

template <typename Db>
int run(fs::path const& dir, utxoz::census_options const& options, bool as_json) {
    // open_for_inspection(), not open(). An instrument must not create the thing it
    // was asked to measure, and the check belongs inside the library, under the
    // directory claim: a check out here could only happen before the open, and
    // whatever it found could stop being true in between.
    //
    // This is also why this program knows no file names. It reads the format
    // through the library or not at all.
    auto opened = Db::open_for_inspection(dir);
    if ( ! opened) {
        if (opened.error() == utxoz::error_code::database_not_found) {
            std::fprintf(stderr, "census: there is no database at %s. Refusing rather "
                                 "than creating one.\n", dir.string().c_str());
        } else {
            std::fprintf(stderr, "census: cannot open %s: error %d\n",
                         dir.string().c_str(), int(opened.error()));
        }
        return 2;
    }
    auto db = std::move(*opened);

    auto report = db.census(options);
    if ( ! report) {
        std::fprintf(stderr, "census: the walk failed: error %d. No partial report is "
                             "produced, because a partial one reads as a complete one.\n",
                     int(report.error()));
        return 3;
    }

    auto const out = as_json ? utxoz::to_json(*report) : utxoz::to_text(*report);
    std::fwrite(out.data(), 1, out.size(), stdout);
    db.close();
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }

    fs::path dir;
    bool as_json = true;
    bool reference = false;
    utxoz::census_options options;

    for (int i = 1; i < argc; ++i) {
        std::string_view const arg = argv[i];
        if (arg == "--json") as_json = true;
        else if (arg == "--text") as_json = false;
        else if (arg == "--mode=full") reference = false;
        else if (arg == "--mode=reference") reference = true;
        else if (arg == "--no-physical-blocks") options.measure_physical_blocks = false;
        else if (arg == "--no-generation-detail") options.per_generation_detail = false;
        else if (arg == "--snapshot") options.declared_external_snapshot = true;
        else if (arg == "--help" || arg == "-h") { usage(); return 0; }
        else if (arg.starts_with("--")) {
            std::fprintf(stderr, "census: unknown option %.*s\n", int(arg.size()), arg.data());
            return 1;
        }
        else if (dir.empty()) dir = fs::path(arg);
        else { std::fputs("census: more than one directory given\n", stderr); return 1; }
    }

    if (dir.empty()) { usage(); return 1; }

    // The error_code overload: "the filesystem would not answer" and "there is
    // nothing there" are different problems and send a person to different
    // places. Asked the throwing way, the first one would come out as a crash;
    // asked the swallowing way, it would come out as the second.
    std::error_code ec;
    if ( ! fs::exists(dir, ec)) {
        if (ec) {
            std::fprintf(stderr, "census: cannot examine %s: %s\n",
                         dir.string().c_str(), ec.message().c_str());
            return 1;
        }
        std::fprintf(stderr, "census: %s does not exist\n", dir.string().c_str());
        return 1;
    }

    return reference ? run<utxoz::reference_db>(dir, options, as_json)
                     : run<utxoz::full_db>(dir, options, as_json);
}
