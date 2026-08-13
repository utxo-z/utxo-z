// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_fixture_generator.cpp
 * @brief That the generator does not announce a certificate it did not write.
 *
 * The manifest is what the compatibility suite certifies against, so a run that
 * reports success while the manifest is missing or half-written is worse than a
 * run that fails: the next person promotes a candidate on the strength of a
 * message.
 *
 * The tool is exercised as a process, because that is how it is used and because
 * the failure being checked is a failure of its exit path.
 */

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "support/read_file.hpp"

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

#ifdef UTXOZ_FIXTURE_GENERATOR

namespace {

struct run_result {
    int status = 0;
    std::string output;
};

run_result run_generator(fs::path const& out) {
    auto const log = fs::temp_directory_path()
                   / fmt::format("utxoz_gen_{}_{}.log", getpid(), out.filename().string());
    auto command = fmt::format("\"{}\" \"{}\" > \"{}\" 2>&1",
                               UTXOZ_FIXTURE_GENERATOR, out.string(), log.string());
#ifdef _WIN32
    // cmd.exe strips the outermost pair of quotes from what it is given, so a
    // command that begins with a quoted path loses that quote and the rest is
    // parsed as something else entirely. The documented way to keep a quoted
    // executable and quoted arguments is to wrap the whole line in one more
    // pair, which cmd then removes instead.
    command = "\"" + command + "\"";
#endif

    run_result r;
    r.status = std::system(command.c_str());
    r.output = utxoz::testing::read_file_text(log);

    std::error_code ec;
    fs::remove(log, ec);
    return r;
}

/// An output directory that removes itself.
///
/// These cases each leave 60 MB behind, and a failing REQUIRE leaves by throwing
/// — so a `remove_all` at the end of the case is exactly the cleanup that does
/// not happen on the runs where it matters most.
struct scoped_out {
    explicit scoped_out(std::string_view tag)
        : dir(fs::temp_directory_path() / fmt::format("utxoz_gen_{}_{}", tag, getpid())) {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
    ~scoped_out() {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
    scoped_out(scoped_out const&) = delete;
    scoped_out& operator=(scoped_out const&) = delete;

    fs::path dir;
};

} // namespace

TEST_CASE("a manifest that cannot be written is not reported as written", "[generator]") {
    scoped_out const o("blocked");
    auto const& out = o.dir;
    std::error_code ec;
    fs::create_directories(out);

    // A directory standing where the manifest's temporary file goes. Opening it
    // as a file fails on every platform, which is the point: this is about what
    // the tool does when the write fails, not about how it was made to fail.
    fs::create_directory(out / "manifest.json.tmp", ec);
    REQUIRE_FALSE(ec);

    auto const r = run_generator(out);

    INFO(r.output);
    CHECK(r.status != 0);
    CHECK(r.output.find("wrote") == std::string::npos);

    // And no manifest was left behind for the next run to trust — or to refuse to
    // overwrite, which is the trap: the tool declines a directory that already
    // holds one, so a truncated manifest would be unfixable by rerunning it.
    CHECK_FALSE(fs::exists(out / "manifest.json"));
}

TEST_CASE("a manifest is published whole or not at all", "[generator]") {
    scoped_out const o("clean");
    auto const& out = o.dir;

    auto const r = run_generator(out);
    INFO(r.output);
    REQUIRE(r.status == 0);
    CHECK(r.output.find("wrote") != std::string::npos);

    REQUIRE(fs::exists(out / "manifest.json"));
    // The temporary name never survives a successful run.
    CHECK_FALSE(fs::exists(out / "manifest.json.tmp"));

    // Whole: it parses, and it ends the way a finished document ends.
    auto const text = utxoz::testing::read_file_text(out / "manifest.json");
    REQUIRE(text.size() > 100);
    CHECK(text.front() == '{');
    CHECK(text.find("\"fixtures\"") != std::string::npos);
    CHECK(text.substr(text.size() - 2) == "}\n");
}

TEST_CASE("the generator refuses a directory that already holds a manifest", "[generator]") {
    scoped_out const o("occupied");
    auto const& out = o.dir;
    std::error_code ec;
    fs::create_directories(out);
    {
        // Binary, so what is written is what is compared. In text mode Windows
        // turns the newline into a carriage return and a newline, and the check
        // below then fails on a difference the test never made — while printing
        // two strings that look identical.
        std::ofstream existing(out / "manifest.json", std::ios::binary);
        existing << "{}\n";
    }

    auto const r = run_generator(out);
    INFO(r.output);
    CHECK(r.status != 0);
    CHECK(r.output.find("wrote") == std::string::npos);

    // Untouched: promoting fixtures is a person's decision, and the tool
    // overwriting a set in place would take it away from them.
    auto const text = utxoz::testing::read_file_text(out / "manifest.json");
    CHECK(text == "{}\n");
}

#endif // UTXOZ_FIXTURE_GENERATOR
