// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_path_encoding.cpp
 * @brief A database directory named outside the machine's code page (#109).
 *
 * `open()` used to take `std::string_view`. On POSIX that was exact — a path is
 * bytes and `path::string()` returns those bytes — so nothing here could have
 * failed on Linux or macOS, then or now. On Windows a `std::filesystem::path`
 * holds `wchar_t` and `path::string()` converts through the active code page:
 * outside that page the conversion throws or substitutes, and a user whose data
 * directory was named in Cyrillic or Japanese could not open the database.
 *
 * That makes this file asymmetric on purpose, and it is worth being plain about
 * which half proves what:
 *
 *   - The signature controls fail on every platform. They are compile-time, and
 *     a return to `std::string_view` — or an added overload that makes
 *     `open("literal")` ambiguous — breaks the build on Linux too.
 *   - The round-trip controls prove no regression everywhere, and prove the fix
 *     only on Windows with a code page that cannot represent the name. On POSIX
 *     they pass against the old implementation as well, because on POSIX there
 *     was never a bug. Nothing here pretends otherwise; the case that reports
 *     the conversion says exactly which of the two it observed.
 *
 * The directory name is built from universal character names inside a `u8`
 * literal rather than from literal bytes in this file. A `u8` literal encodes
 * `\uXXXX` as UTF-8 whatever encoding the compiler reads the source in, so the
 * name under test is the same on a compiler invoked without `/utf-8` as on one
 * invoked with it. Written as raw characters, MSVC would read them in the
 * machine's code page and the test would silently exercise a different name
 * than the one intended — which is the same class of bug as the one under test.
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>

namespace fs = std::filesystem;

namespace {

inline std::atomic<uint64_t> path_test_counter{0};

/// U+00F1 LATIN SMALL LETTER N WITH TILDE — in Windows-1252.
/// U+0434 CYRILLIC SMALL LETTER DE      — not in Windows-1252.
/// U+65E5 CJK UNIFIED IDEOGRAPH 日      — not in any single-byte code page.
///
/// The last two are the point: on a runner whose active code page is 1252, this
/// name cannot survive `path::string()`, so the directory the old code would
/// have reached is not the directory the caller named.
fs::path non_ascii_name() {
    return fs::path(std::u8string(u8"utxoz-\u00f1-\u0434-\u65e5"));
}

/// A unique directory whose *final component* carries the non-ASCII name. The
/// parent stays ASCII so that a failure is unambiguously about the component
/// under test rather than about wherever the test happens to be run from.
fs::path make_non_ascii_dir(std::string_view tag) {
    auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto const unique = path_test_counter.fetch_add(1);
    fs::path dir = fs::path(".") / fmt::format("utxoz_path_{}_{}_{}_{}", tag, getpid(), ts, unique);
    dir /= non_ascii_name();
    std::error_code ec;
    fs::remove_all(dir, ec);
    return dir;
}

utxoz::raw_outpoint key_of(uint8_t seed) {
    utxoz::raw_outpoint key{};
    key[0] = seed;
    key[1] = static_cast<uint8_t>(seed + 1);
    return key;
}

/// Deletes the whole unique tree, not just the leaf.
void clean_up(fs::path const& dir) {
    std::error_code ec;
    fs::remove_all(dir.parent_path(), ec);
}

/// How many regular files the database left in this exact directory.
size_t files_in(fs::path const& dir) {
    std::error_code ec;
    size_t n = 0;
    for (fs::directory_iterator it(dir, ec); ! ec && it != fs::directory_iterator{}; it.increment(ec)) {
        if (it->is_regular_file(ec)) ++n;
    }
    return n;
}

/// The conversion the old signature forced on every caller, and whether it
/// survived it. `nullopt` means `path::string()` threw, which is the strongest
/// form of the loss: the old API could not be called with this path at all.
std::optional<std::string> old_style_narrowing(fs::path const& p) {
    try {
        return p.string();
    } catch (std::exception const&) {
        return std::nullopt;
    }
}

} // anonymous namespace

// =============================================================================
// The signature. These fail on every platform.
// =============================================================================

// The exact parameter type, pinned. Reverting to std::string_view — or to any
// other type — stops this compiling, on Linux as much as on Windows.
static_assert(std::is_same_v<decltype(&utxoz::full_db::open),
                             utxoz::result<utxoz::full_db>(*)(fs::path, bool)>,
              "full_db::open must take std::filesystem::path by value");
static_assert(std::is_same_v<decltype(&utxoz::full_db::open_for_testing),
                             utxoz::result<utxoz::full_db>(*)(fs::path, bool)>,
              "full_db::open_for_testing must take std::filesystem::path by value");
static_assert(std::is_same_v<decltype(&utxoz::reference_db::open),
                             utxoz::result<utxoz::reference_db>(*)(fs::path, bool)>,
              "reference_db::open must take std::filesystem::path by value");
static_assert(std::is_same_v<decltype(&utxoz::reference_db::open_for_testing),
                             utxoz::result<utxoz::reference_db>(*)(fs::path, bool)>,
              "reference_db::open_for_testing must take std::filesystem::path by value");

// Every call form a caller might already have, checked unevaluated so that
// nothing here opens a database.
//
// The string literal is the one that matters. Adding a `std::string_view`
// overload beside the `std::filesystem::path` one would make `open("literal")`
// ambiguous — `const char[N]` converts to both by a user-defined conversion of
// equal rank — and this assertion is what says so before a caller finds out.
// It is why there is one parameter type rather than an overload set.
static_assert(requires { utxoz::full_db::open("literal", false); },
              "a string literal must still select a unique overload");
static_assert(requires { utxoz::full_db::open(std::declval<char const*>(), false); },
              "a const char* must still select a unique overload");
static_assert(requires { utxoz::full_db::open(std::declval<std::string>(), false); },
              "a std::string must still select a unique overload");
static_assert(requires { utxoz::full_db::open(std::declval<std::string_view>(), false); },
              "a std::string_view must still select a unique overload");
static_assert(requires { utxoz::full_db::open(std::declval<fs::path>(), false); },
              "a std::filesystem::path must be accepted directly");
static_assert(requires { utxoz::reference_db::open_for_testing(std::declval<fs::path>(), false); },
              "reference_db must accept a path directly too");

// The default argument survives, so one-argument calls still compile.
static_assert(requires { utxoz::full_db::open(std::declval<fs::path>()); },
              "remove_existing must keep its default");

// =============================================================================
// The round trip, in both storage modes.
// =============================================================================

TEST_CASE("full: a database under a non-ASCII directory opens, writes, closes and reopens",
          "[path][encoding][full]") {
    auto const dir = make_non_ascii_dir("full");
    auto const key = key_of(0x11);
    std::vector<uint8_t> const value(32, 0xAB);

    {
        auto opened = utxoz::full_db::open_for_testing(dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        REQUIRE(db.insert(key, utxoz::output_data_span{value}, 100).has_value());

        auto const found = db.find(key, 200);
        REQUIRE(found.has_value());
        CHECK(found->block_height == 100);

        db.close();
    }

    // The database is where the caller said it was, under the name the caller
    // used. A path that had been narrowed and re-widened would have produced a
    // directory with a different name, or none at all.
    CHECK(fs::exists(dir));
    CHECK(files_in(dir) > 0);

    // Reopened — not recreated. remove_existing is false, so the data has to
    // come back off the disk, which is what makes this a round trip rather than
    // two independent writes.
    {
        auto reopened = utxoz::full_db::open_for_testing(dir, false);
        REQUIRE(reopened.has_value());
        auto db = std::move(*reopened);

        auto const found = db.find(key, 200);
        REQUIRE(found.has_value());
        CHECK(found->block_height == 100);
        CHECK(found->data == value);

        db.close();
    }

    clean_up(dir);
}

TEST_CASE("reference: a database under a non-ASCII directory opens, writes, closes and reopens",
          "[path][encoding][reference]") {
    auto const dir = make_non_ascii_dir("reference");
    auto const key = key_of(0x22);

    {
        auto opened = utxoz::reference_db::open_for_testing(dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        REQUIRE(db.insert(key, 7, 42, 100).has_value());

        auto const found = db.find(key, 200);
        REQUIRE(found.has_value());
        CHECK(found->file_number == 7);
        CHECK(found->offset == 42);

        db.close();
    }

    CHECK(fs::exists(dir));
    CHECK(files_in(dir) > 0);

    {
        auto reopened = utxoz::reference_db::open_for_testing(dir, false);
        REQUIRE(reopened.has_value());
        auto db = std::move(*reopened);

        auto const found = db.find(key, 200);
        REQUIRE(found.has_value());
        CHECK(found->file_number == 7);
        CHECK(found->offset == 42);

        db.close();
    }

    clean_up(dir);
}

// =============================================================================
// The negation: what the old conversion did to this path.
// =============================================================================

TEST_CASE("the conversion the old signature forced is reported, not assumed",
          "[path][encoding][negative]") {
    auto const dir = make_non_ascii_dir("narrowing");
    auto const narrowed = old_style_narrowing(dir);

    // Whatever the code page does, the library carries the path. This is the
    // assertion; the analysis below is evidence about how much it was worth.
    {
        auto opened = utxoz::full_db::open_for_testing(dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        REQUIRE(db.insert(key_of(0x33), utxoz::output_data_span{std::vector<uint8_t>(32, 1)}, 5)
                    .has_value());
        db.close();
    }
    REQUIRE(files_in(dir) > 0);

    if ( ! narrowed) {
        // Windows, and the code page cannot spell this directory. The old
        // signature could not be called with this path at all: producing the
        // argument threw.
        SUCCEED("path::string() threw — the old std::string_view signature could not carry this path");
    } else if (fs::path(*narrowed) != dir) {
        // It converted, and came back as something else. Both forms are printed
        // so the diff is on the record rather than described.
        auto const original = dir.u8string();
        INFO("original (UTF-8): " << std::string(reinterpret_cast<char const*>(original.data()),
                                                original.size()));
        INFO("after path::string() and back: " << fs::path(*narrowed).string());
        SUCCEED("path::string() was lossy for this directory — the old signature reached a different path");

        // And concretely: the narrowed name does not name the database that was
        // just created. This is the failure a user saw — a path that does not
        // exist, rather than an encoding error.
        CHECK_FALSE(fs::exists(fs::path(*narrowed)));
    } else {
        // POSIX, or Windows with a UTF-8 active code page. The narrowing was
        // faithful here, so this run has not exercised the bug — and says so
        // rather than counting as proof that it is fixed.
        WARN("path::string() round-tripped on this platform, so this case proves "
             "no-regression only; the loss it exists to demonstrate needs a "
             "non-UTF-8 active code page (Windows)");
    }

    clean_up(dir);
}

// =============================================================================
// Compatibility: the call forms that existed before still work at run time.
// =============================================================================

TEST_CASE("an ASCII path still opens when passed as the old narrow types",
          "[path][encoding][compatibility]") {
    auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto const base = fmt::format("./utxoz_path_compat_{}_{}", getpid(), ts);

    // std::string — what most callers hold, and what the fixtures in the rest of
    // this suite pass.
    {
        std::string const as_string = base + "_string";
        auto opened = utxoz::full_db::open_for_testing(as_string, true);
        REQUIRE(opened.has_value());
        opened->close();
        std::error_code ec;
        fs::remove_all(as_string, ec);
    }

    // std::string_view — the old parameter type itself.
    {
        std::string const owner = base + "_view";
        std::string_view const as_view = owner;
        auto opened = utxoz::full_db::open_for_testing(as_view, true);
        REQUIRE(opened.has_value());
        opened->close();
        std::error_code ec;
        fs::remove_all(owner, ec);
    }

    // The migration a caller holding a path makes: pass the path, not a string
    // made from it.
    {
        fs::path const as_path = base + "_path";
        auto opened = utxoz::full_db::open_for_testing(as_path, true);
        REQUIRE(opened.has_value());
        opened->close();
        std::error_code ec;
        fs::remove_all(as_path, ec);
    }
}
