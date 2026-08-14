// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_sizing_tool.cpp
 * @brief That the sizing instrument measures what it says it measures.
 *
 * Run as a process, at small steps. The production figures are a lane that runs
 * once per platform; a suite that built 1.2 GiB tables in every case would be a
 * suite nobody runs, and slowing every build to re-measure a number that changes
 * only when the geometry does is the wrong trade.
 *
 * What is checked here is the instrument, not the sizes: the ladder it reports,
 * the capacity-to-bucket_count mapping, that the size it recommends builds the map
 * and one below its measured floor does not, that the next step cannot fit, and
 * that it leaves nothing behind.
 */

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <boost/json.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/types.hpp>

#include "detail/format_identity.hpp"

#include "support/read_file.hpp"

namespace fs = std::filesystem;

#ifdef UTXOZ_SIZING_TOOL

namespace {

struct run_result {
    int status = 0;
    std::string out;
};

/// A working directory of its own, removed however the scope is left.
struct scoped_work {
    scoped_work() {
        static int n = 0;
        dir = fs::temp_directory_path() / fmt::format("utxoz-sizing-test-{}-{}", getpid(), n++);
        std::error_code ec;
        fs::remove_all(dir, ec);
        fs::create_directories(dir);
    }
    ~scoped_work() {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
    scoped_work(scoped_work const&) = delete;
    scoped_work& operator=(scoped_work const&) = delete;
    fs::path dir;
};

run_result run(scoped_work const& w, std::string const& args) {
    auto const log = w.dir / "out.txt";
    auto command = fmt::format("\"{}\" {} --work \"{}\" > \"{}\" 2>&1",
                               UTXOZ_SIZING_TOOL, args, w.dir.string(), log.string());
#ifdef _WIN32
    // cmd.exe removes the outermost pair of quotes; one more keeps the rest.
    command = "\"" + command + "\"";
#endif
    run_result r;
    r.status = std::system(command.c_str());
    r.out = utxoz::testing::read_file_text(log);
    std::error_code ec;
    fs::remove(log, ec);
    return r;
}

boost::json::value json_of(scoped_work const& w, std::string const& args) {
    auto const r = run(w, args + " --format json");
    INFO(r.out);
    REQUIRE(r.status == 0);
    boost::system::error_code ec;
    auto v = boost::json::parse(r.out, ec);
    REQUIRE_FALSE(ec.failed());
    REQUIRE(v.is_object());
    return v;
}

uint64_t num(boost::json::value const& v, char const* key) {
    auto const& f = v.at(key);
    REQUIRE((f.is_int64() || f.is_uint64()));
    return f.is_uint64() ? f.as_uint64() : uint64_t(f.as_int64());
}

} // namespace

TEST_CASE("the steps it reports are the steps Boost uses", "[sizing]") {
    scoped_work w;
    // 15·2^k − 1, and the step number it prints has to be the k it was asked for.
    for (unsigned k : {5u, 8u, 11u}) {
        INFO("step " << k);
        auto const j = json_of(w, fmt::format("--class 48 --step {}", k));
        CHECK(num(j, "bucket_count") == (uint64_t(15) << k) - 1);
        CHECK(num(j, "step") == k);
    }
}

TEST_CASE("a capacity that is not a step becomes the step above it", "[sizing]") {
    scoped_work w;
    // The constructor takes a hint, not an instruction. Two different hints can
    // land on one bucket count, and a tool that reported the hint would be
    // describing a map that does not exist.
    auto const low = json_of(w, "--class 48 --buckets 4000");
    auto const high = json_of(w, "--class 48 --buckets 7000");
    CHECK(num(low, "bucket_count") == 7679);
    CHECK(num(high, "bucket_count") == 7679);
    CHECK(num(low, "asked") != num(high, "asked"));
    CHECK(num(low, "bucket_count") == num(high, "bucket_count"));
}

TEST_CASE("every class in the geometry can be measured", "[sizing]") {
    scoped_work w;
    STATIC_REQUIRE(utxoz::container_sizes == std::array<size_t, 5>{48, 96, 128, 256, 10240});
    for (auto const* cls : {"48", "96", "128", "256", "10240", "reference"}) {
        INFO(cls);
        auto const j = json_of(w, fmt::format("--class {} --step 6", cls));
        CHECK(num(j, "observed_minimum") > 0);
        CHECK(num(j, "recommended") >= num(j, "observed_minimum"));
        CHECK(num(j, "sizeof_pair") > 0);
    }
}

TEST_CASE("more buckets never need fewer bytes", "[sizing]") {
    scoped_work w;
    uint64_t previous = 0;
    for (unsigned k = 5; k <= 10; ++k) {
        INFO("step " << k);
        auto const j = json_of(w, fmt::format("--class 96 --step {}", k));
        auto const minimum = num(j, "observed_minimum");
        CHECK(minimum > previous);
        previous = minimum;
    }
}

TEST_CASE("the floor it reports is a bracket it actually tried", "[sizing]") {
    scoped_work w;
    auto const j = json_of(w, "--class 128 --step 10 --precision 4096");

    auto const lower = num(j, "lower_bound");
    auto const minimum = num(j, "observed_minimum");
    auto const recommended = num(j, "recommended");
    auto const precision = num(j, "precision");

    // Both ends were tried, and the tool says which way each went. Naming the
    // upper end "the minimum" would claim a byte nothing tested; what is claimed
    // is that this size built the map and that one did not.
    CHECK(j.at("observed_minimum_builds").as_bool());
    CHECK_FALSE(j.at("lower_bound_builds").as_bool());
    CHECK(lower < minimum);
    CHECK(precision == minimum - lower);
    CHECK(precision <= 4096);
    CHECK(recommended > minimum);

    // The margin rounds up rather than truncating: it is a safety bound, and the
    // obvious `x / 100 * pct` loses up to a hundred bytes per point.
    CHECK(num(j, "margin_bytes") == (minimum * 5 + 99) / 100);

    // Asked for by capacity instead of by step: same floor, because the floor
    // belongs to the map and not to how it was requested.
    CHECK(num(json_of(w, fmt::format("--class 128 --buckets {} --precision 4096",
                                     (uint64_t(15) << 10) - 1)),
              "observed_minimum") == minimum);
}

TEST_CASE("the step above does not fit in the size recommended for this one",
          "[sizing]") {
    scoped_work w;
    // The property the whole policy rests on: the map fits with room, and its
    // successor does not, so a growth Boost decides on has nowhere to go.
    for (auto const* cls : {"48", "96", "128"}) {
        INFO(cls);
        auto const j = json_of(w, fmt::format("--class {} --step 10", cls));
        CHECK_FALSE(j.at("next_builds_alone").as_bool());
        CHECK_FALSE(j.at("next_coexists_via_rehash").as_bool());
        CHECK(num(j, "next_bucket_count") == num(j, "bucket_count") * 2 + 1);
    }
}

TEST_CASE("filling to the rotation threshold does not rehash", "[sizing]") {
    scoped_work w;
    // Building is not operating. At a small step this is cheap, and it is the
    // half that says the recommended size holds the entries and not just the
    // arrays.
    auto const j = json_of(w, "--class 48 --step 10 --fill");
    REQUIRE(j.at("filled").as_bool());
    CHECK(num(j, "entries_inserted") == num(j, "rotation_threshold"));
    CHECK(num(j, "bucket_count_after_fill") == num(j, "bucket_count"));
    CHECK_FALSE(j.at("rehashed_while_filling").as_bool());
}

TEST_CASE("the rotation threshold is below where Boost grows", "[sizing]") {
    scoped_work w;
    auto const j = json_of(w, "--class 48 --step 12");
    CHECK(num(j, "rotation_threshold") < num(j, "entries_at_max_load"));
    CHECK(j.at("max_load_factor").as_double() > 0.5);
}

TEST_CASE("it leaves nothing behind", "[sizing]") {
    scoped_work w;
    (void)json_of(w, "--class 48 --step 9");

    // Only the log this test wrote, which it removes itself. Anything else is a
    // segment the tool created and did not clean up.
    size_t left = 0;
    for (auto const& e : fs::directory_iterator(w.dir)) {
        ++left;
        INFO("left behind: " << e.path().filename().string());
        CHECK(false);
    }
    CHECK(left == 0);
}

TEST_CASE("a stamp that will not go down is not a capacity answer", "[sizing]") {
    scoped_work w;
    // Fifty-six bytes at the start of a segment sized for a map thousands of times
    // larger: a failure there does not mean "no room". `--fail-stamp` reaches that
    // path for a reason that has nothing to do with space — the name is already
    // taken — and the tool has to stop rather than fold it into the search as one
    // more size that did not fit.
    auto const r = run(w, "--class 48 --step 9 --fail-stamp");
    INFO(r.out);
    CHECK(r.status != 0);
    CHECK(r.out.find("could not be stamped") != std::string::npos);

    // And no report: a measurement that was not made must not look like one.
    CHECK(r.out.find("observed minimum") == std::string::npos);
    CHECK(r.out.find("\"recommended\"") == std::string::npos);
}

TEST_CASE("the segment it measures is stamped as the class it claims", "[sizing]") {
    scoped_work w;
    // The kind travels with the class. A segment stamped as container 0 while
    // holding a class-256 map is identified as something it is not, and measuring
    // it would be measuring a file the store would refuse to open.
    struct { char const* cls; uint64_t kind; } const expected[] = {
        {"48", 0}, {"96", 1}, {"128", 2}, {"256", 3}, {"10240", 4},
    };
    for (auto const& e : expected) {
        INFO(e.cls);
        CHECK(num(json_of(w, fmt::format("--class {} --step 6", e.cls)), "container_kind")
              == e.kind);
    }
    CHECK(num(json_of(w, "--class reference --step 6"), "container_kind")
          == utxoz::detail::reference_container_kind);
}

TEST_CASE("a fill that was asked for and did not happen is a failure", "[sizing]") {
    scoped_work w;
    // Otherwise a script reads the exit code, gets zero, and accepts a report that
    // never did the thing it was told to do.
    auto const r = run(w, "--class 10240 --step 6 --fill --segment 100000");
    INFO(r.out);
    CHECK(r.status != 0);
    CHECK(r.out.find("\"recommended\"") == std::string::npos);

    // And nothing survives it.
    size_t left = 0;
    for (auto const& e : fs::directory_iterator(w.dir)) {
        if (e.path().filename() == "out.txt") continue;
        ++left;
        INFO("left behind: " << e.path().filename().string());
        CHECK(false);
    }
    CHECK(left == 0);
}

TEST_CASE("it refuses what it cannot measure, and says why", "[sizing]") {
    scoped_work w;
    struct { char const* args; char const* mentions; } const bad[] = {
        {"--class 77 --step 6",        "no such class"},
        {"--class 48",                 "--buckets or --step"},
        {"--step 6",                   "--class is required"},
        {"--class 48 --step 99",       "out of range"},
        {"--class 48 --step 6 --format yaml", "text or json"},
        {"--class 48 --buckets abc",   "not a number"},
        {"--class 48 --step 6 --buckets 100", "alternatives"},
        // Zero is not "unset" for any of these. A precision of zero in particular
        // would leave the search bracketing for ever rather than answering, which
        // is worse than a wrong number.
        {"--class 48 --buckets 0",             "greater than zero"},
        {"--class 48 --step 6 --precision 0",  "greater than zero"},
        {"--class 48 --step 6 --segment 0",    "greater than zero"},
    };
    for (auto const& c : bad) {
        INFO(c.args);
        auto const r = run(w, c.args);
        CHECK(r.status != 0);
        CHECK(r.out.find(c.mentions) != std::string::npos);
    }
}

TEST_CASE("a precision of zero is refused rather than pursued", "[sizing]") {
    scoped_work w;
    // Its own case because the failure it prevents is a hang, not a wrong answer:
    // with a bracket one byte wide and no width to close to, the midpoint is the
    // lower bound and the search never terminates. A test that only checked the
    // message would pass while the tool ran for ever, so this is timed.
    auto const started = std::chrono::steady_clock::now();
    auto const r = run(w, "--class 48 --step 8 --precision 0");
    auto const seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();

    INFO(r.out);
    CHECK(r.status != 0);
    CHECK(r.out.find("greater than zero") != std::string::npos);
    CHECK(seconds < 30.0);

    size_t left = 0;
    for (auto const& e : fs::directory_iterator(w.dir)) {
        if (e.path().filename() == "out.txt") continue;
        ++left;
    }
    CHECK(left == 0);
}

TEST_CASE("the report is written where it is asked for", "[sizing]") {
    scoped_work w;
    auto const out = w.dir / "report.json";
    auto const r = run(w, fmt::format("--class 96 --step 7 --format json --output \"{}\"",
                                      out.string()));
    INFO(r.out);
    REQUIRE(r.status == 0);
    REQUIRE(fs::exists(out));

    boost::system::error_code ec;
    auto const v = boost::json::parse(utxoz::testing::read_file_text(out), ec);
    REQUIRE_FALSE(ec.failed());
    CHECK(v.at("class").as_string() == "96");

    std::error_code fec;
    fs::remove(out, fec);
}

#endif // UTXOZ_SIZING_TOOL
