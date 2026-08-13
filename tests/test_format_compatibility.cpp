// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_format_compatibility.cpp
 * @brief That this build still reads what an earlier one wrote.
 *
 * The fixtures under `tests/fixtures/` were written once, by the code that
 * introduced the format barrier, against Boost 1.91 on a 64-bit little-endian
 * target. They are artefacts, not outputs: nothing here regenerates them, and a
 * change to their bytes is a change that has to be read and justified by a
 * person.
 *
 * @par What a green run here does and does not mean
 * A fixture written under epoch 1 proves that the current reader still reads
 * epoch 1. It does **not** prove that the current writer still produces the same
 * bytes for every state a database can reach — the fixture only contains the
 * states it contains. That asymmetry is why the manifest carries per-segment
 * digests as well as a global one, and why the generator exists separately: a
 * candidate regenerated now can be compared against what was committed, by
 * somebody looking at it.
 *
 * @par What is checked, and why in that order
 * Physical first — the file opens, its config and stamps are accepted — and then
 * logical: every entry, in a canonical order, digested. "open() succeeded" is
 * not compatibility, and neither is a matching entry count: both stay green
 * while the values are wrong.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <boost/json/basic_parser_impl.hpp>
#include <boost/json/src.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>

#include "detail/durability.hpp"
#include "detail/file_cache.hpp"
#include "detail/format_identity.hpp"
#include "detail/segment_open.hpp"
#include "detail/segment_stamp.hpp"
#include "detail/store_config_io.hpp"
#include "detail/utxo_value.hpp"
#include "detail/version_catalog.hpp"

#include "support/logical_digest.hpp"
#include "support/read_file.hpp"

namespace fs = std::filesystem;
namespace bip = boost::interprocess;

using utxoz::testing::logical_entry;

namespace {

std::atomic<uint64_t> compat_counter{0};

fs::path make_unique_path(std::string_view tag) {
    auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fs::path(fmt::format("./test_compat_{}_{}_{}_{}", tag, getpid(), ts,
                                compat_counter.fetch_add(1)));
}

/// Where the fixtures live, relative to this source file. Passed in by CMake so
/// the test does not have to guess at the working directory ctest gives it.
fs::path fixtures_root() { return fs::path(UTXOZ_FIXTURES_DIR); }

/// Says where a case has got to, if asked.
///
/// A case killed by a timeout reports nothing: Catch2 buffers until it has a
/// result, and there is no result. The only thing that survives is what was
/// already written and flushed, so this goes to std::cerr — and it stays off
/// unless UTXOZ_TEST_TRACE is set, because a suite that narrates itself on every
/// run is a suite nobody reads.
void trace(std::string_view what) {
    static bool const on = std::getenv("UTXOZ_TEST_TRACE") != nullptr;
    if ( ! on) return;
    auto const now = std::chrono::steady_clock::now().time_since_epoch();
    std::cerr << "[compat +"
              << std::chrono::duration_cast<std::chrono::milliseconds>(now).count() % 1000000
              << "ms] " << what << std::endl;
}

std::string read_text(fs::path const& p) {
    auto const text = utxoz::testing::read_file_text(p);
    REQUIRE_FALSE(text.empty());
    return text;
}

std::vector<uint8_t> read_bytes(fs::path const& p) {
    auto bytes = utxoz::testing::read_file_bytes(p);
    REQUIRE_FALSE(bytes.empty());
    return bytes;
}

/// Finds keys repeated within one object.
///
/// Boost.JSON parses such a document without complaint and keeps the last of the
/// duplicates, so `parse()` succeeding says nothing about this. That matters for
/// a file the suite certifies against: a second `map_layout_epoch` further down
/// would silently win over the first, and every check would be made against the
/// value a reader of the file would not expect. Nothing in Boost.JSON's public
/// surface reports it, which is why this walks the token stream.
struct duplicate_key_finder {
    static constexpr std::size_t max_array_size = std::size_t(-1);
    static constexpr std::size_t max_object_size = std::size_t(-1);
    static constexpr std::size_t max_string_size = std::size_t(-1);
    static constexpr std::size_t max_key_size = std::size_t(-1);

    std::vector<std::set<std::string>> open_objects;
    std::string partial_key;
    std::vector<std::string> repeated;

    bool on_object_begin(boost::system::error_code&) {
        open_objects.emplace_back();
        return true;
    }
    bool on_object_end(std::size_t, boost::system::error_code&) {
        open_objects.pop_back();
        return true;
    }
    bool on_key_part(boost::core::string_view s, std::size_t, boost::system::error_code&) {
        partial_key.append(s.data(), s.size());
        return true;
    }
    bool on_key(boost::core::string_view s, std::size_t, boost::system::error_code&) {
        partial_key.append(s.data(), s.size());
        if ( ! open_objects.empty() && ! open_objects.back().insert(partial_key).second) {
            repeated.push_back(partial_key);
        }
        partial_key.clear();
        return true;
    }

    bool on_document_begin(boost::system::error_code&) { return true; }
    bool on_document_end(boost::system::error_code&) { return true; }
    bool on_array_begin(boost::system::error_code&) { return true; }
    bool on_array_end(std::size_t, boost::system::error_code&) { return true; }
    bool on_string_part(boost::core::string_view, std::size_t, boost::system::error_code&) { return true; }
    bool on_string(boost::core::string_view, std::size_t, boost::system::error_code&) { return true; }
    bool on_number_part(boost::core::string_view, boost::system::error_code&) { return true; }
    bool on_int64(int64_t, boost::core::string_view, boost::system::error_code&) { return true; }
    bool on_uint64(uint64_t, boost::core::string_view, boost::system::error_code&) { return true; }
    bool on_double(double, boost::core::string_view, boost::system::error_code&) { return true; }
    bool on_bool(bool, boost::system::error_code&) { return true; }
    bool on_null(boost::system::error_code&) { return true; }
    bool on_comment_part(boost::core::string_view, boost::system::error_code&) { return true; }
    bool on_comment(boost::core::string_view, boost::system::error_code&) { return true; }
};

std::vector<std::string> duplicate_keys(std::string const& text) {
    boost::json::basic_parser<duplicate_key_finder> parser{boost::json::parse_options{}};
    boost::system::error_code ec;
    parser.write_some(false, text.data(), text.size(), ec);
    if (ec.failed()) return {};
    return parser.handler().repeated;
}

uint64_t as_u64(boost::json::value const& v) {
    // Not `uint64_t(as_int64())`: JSON has one number type and a negative would
    // arrive here as a very large positive, which is how a nonsense entry count
    // turns into a plausible one.
    REQUIRE((v.is_uint64() || (v.is_int64() && v.as_int64() >= 0)));
    return v.is_uint64() ? v.as_uint64() : uint64_t(v.as_int64());
}

std::string as_str(boost::json::value const& v) {
    REQUIRE(v.is_string());
    return std::string(v.as_string());
}

/// A SHA-256 or a logical digest, as this project writes them.
bool is_digest(std::string const& s) {
    return s.size() == 64
        && std::ranges::all_of(s, [](char c) {
               return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
           });
}

/// The storage mode, by exact name.
///
/// This used to be `== "full" ? full : reference`, which is not a decision but a
/// default: any string at all that was not "full" — a typo, an empty value, a
/// mode invented later — selected reference mode and the suite went on to check
/// the wrong half of the format.
utxoz::storage_mode mode_from(std::string const& s) {
    if (s == "full") return utxoz::storage_mode::full;
    if (s == "reference") return utxoz::storage_mode::reference;
    FAIL("the manifest declares a storage mode that does not exist: " << s);
    return utxoz::storage_mode::full;
}

/// The first thing wrong with a manifest, or nothing.
///
/// Returns a description rather than failing, so that the checks below can be
/// pointed at deliberately broken documents and asked to reject them. A
/// validator nothing ever sees reject anything is indistinguishable from one
/// that accepts everything.
std::optional<std::string> schema_error(std::string const& text) try {
    boost::system::error_code ec;
    auto const value = boost::json::parse(text, ec);
    if (ec.failed()) return "not JSON: " + ec.message();
    if ( ! value.is_object()) return "the document is not an object";

    auto const repeated = duplicate_keys(text);
    if ( ! repeated.empty()) return "a key is repeated within one object: " + repeated.front();

    auto num = [](boost::json::value const& v) -> std::optional<uint64_t> {
        if (v.is_uint64()) return v.as_uint64();
        // JSON has one number type, so a negative arrives here and would become
        // a very large positive if simply cast — which is how a nonsense count
        // turns into a plausible one.
        if (v.is_int64() && v.as_int64() >= 0) return uint64_t(v.as_int64());
        return std::nullopt;
    };
    auto text_of = [](boost::json::value const& v) -> std::optional<std::string> {
        if ( ! v.is_string()) return std::nullopt;
        return std::string(v.as_string());
    };

    if ( ! num(value.at("manifest_format"))) return "manifest_format is not a count";

    auto const& written = value.at("written_by").as_object();
    for (auto const* field : {"boost_version", "config_format", "geometry_id",
                              "map_layout_epoch", "hash_epoch", "platform_abi_id",
                              "platform_abi_encoding", "data_abi_id", "sync_abi_family",
                              "sizeof_interprocess_mutex", "alignof_interprocess_mutex",
                              "sizeof_segment_manager", "alignof_segment_manager",
                              "platform_tag",
                              "sizeof_size_t", "sizeof_pointer", "sizeof_offset_ptr"}) {
        if ( ! num(written.at(field))) return std::string(field) + " is not a count";
    }

    // Enumerations by exact equality. An `else` that takes everything not equal
    // to one spelling is not a decision, it is a default: a typo would have
    // selected the other value and the checks would have gone on to verify the
    // wrong half of the format.
    auto const endian = text_of(written.at("endian"));
    if ( ! endian) return "endian is not a string";
    if (*endian != "little" && *endian != "big") return "endian is neither: " + *endian;

    std::set<std::string> fixture_names;
    auto const& fixtures = value.at("fixtures").as_array();
    if (fixtures.empty()) return "the manifest describes no fixtures";

    for (auto const& entry : fixtures) {
        auto const& o = entry.as_object();
        auto const name = text_of(o.at("name"));
        if ( ! name) return "a fixture has no name";
        if ( ! fixture_names.insert(*name).second) return "two fixtures are called " + *name;

        auto const mode = text_of(o.at("storage_mode"));
        if ( ! mode) return *name + ": storage_mode is not a string";
        if (*mode != "full" && *mode != "reference") return *name + ": no such storage mode: " + *mode;

        if ( ! num(o.at("entries"))) return *name + ": entries is not a count";

        auto const digest = text_of(o.at("global_logical_digest"));
        if ( ! digest || ! is_digest(*digest)) return *name + ": the global digest is not one";

        // A segment is identified by which container it is and which generation.
        // Two entries claiming one pair describe a single file twice, and the
        // per-segment checks would then verify it twice and another not at all.
        std::set<std::pair<uint64_t, uint64_t>> segments;
        auto const& segs = o.at("segments").as_array();
        if (segs.empty()) return *name + ": no segments";
        for (auto const& seg : segs) {
            auto const& so = seg.as_object();
            auto const kind = num(so.at("container_kind"));
            auto const generation = num(so.at("generation"));
            if ( ! kind || ! generation) return *name + ": a segment is not identified";
            if ( ! segments.insert({*kind, *generation}).second) {
                return *name + ": two segments claim the same container and generation";
            }
            if ( ! num(so.at("entries"))) return *name + ": a segment count is not one";
            auto const sd = text_of(so.at("logical_digest"));
            if ( ! sd || ! is_digest(*sd)) return *name + ": a segment digest is not one";
        }

        std::set<std::string> files;
        auto const& listed = o.at("files").as_array();
        if (listed.empty()) return *name + ": no files";
        for (auto const& file : listed) {
            auto const& fo = file.as_object();
            auto const file_name = text_of(fo.at("name"));
            if ( ! file_name) return *name + ": a file has no name";
            if ( ! files.insert(*file_name).second) return *name + ": listed twice: " + *file_name;
            if ( ! num(fo.at("bytes"))) return *name + ": a file size is not one";
            auto const hash = text_of(fo.at("sha256"));
            if ( ! hash || ! is_digest(*hash)) return *name + ": " + *file_name + ": not a sha256";
        }
    }
    return std::nullopt;
} catch (std::exception const& e) {
    // A missing key or a value of the wrong shape lands here.
    return std::string("not shaped like a manifest: ") + e.what();
}

/// The manifest, parsed as JSON and then held to its schema.
///
/// It stopped being documentation the moment the suite began certifying against
/// it, and parsing is the smaller half of that: a well-formed document can still
/// carry a repeated key, a negative count, two fixtures under one name or a
/// digest that is not a digest, and each of those makes a check pass for a
/// reason unrelated to the bytes.
boost::json::value parse_manifest() {
    auto const text = read_text(fixtures_root() / "manifest.json");
    INFO("manifest.json");
    auto const problem = schema_error(text);
    if (problem) FAIL(*problem);
    return boost::json::parse(text);
}

struct expected_segment {
    uint32_t kind = 0;
    uint64_t generation = 0;
    size_t entries = 0;
    std::string digest;
};

struct expected_fixture {
    std::string name;
    utxoz::storage_mode mode = utxoz::storage_mode::full;
    size_t entries = 0;
    std::string global_digest;
    std::vector<expected_segment> segments;
};

/// Reads the manifest entry for one fixture.
expected_fixture manifest_for(std::string const& name) {
    auto const manifest = parse_manifest();
    auto const& fixtures = manifest.at("fixtures").as_array();

    for (auto const& entry : fixtures) {
        auto const& o = entry.as_object();
        if (as_str(o.at("name")) != name) continue;

        expected_fixture f;
        f.name = name;
        f.mode = mode_from(as_str(o.at("storage_mode")));
        f.entries = size_t(as_u64(o.at("entries")));
        f.global_digest = as_str(o.at("global_logical_digest"));
        for (auto const& seg : o.at("segments").as_array()) {
            auto const& so = seg.as_object();
            f.segments.push_back({uint32_t(as_u64(so.at("container_kind"))),
                                  as_u64(so.at("generation")),
                                  size_t(as_u64(so.at("entries"))),
                                  as_str(so.at("logical_digest"))});
        }
        REQUIRE_FALSE(f.segments.empty());
        return f;
    }
    FAIL("the manifest does not describe " << name);
    return {};
}

/// Copies a fixture out of the tree before anything opens it. The originals are
/// never opened in place: an open takes the directory's claim and writes a lock
/// file, which would be a test modifying its own evidence.
struct fixture_copy {
    fs::path dir;
    fs::path source;

    explicit fixture_copy(std::string const& name)
        : dir(make_unique_path(name)), source(fixtures_root() / name) {
        REQUIRE(fs::exists(source));
        std::error_code ec;
        fs::create_directories(dir, ec);
        REQUIRE_FALSE(ec);

        // File by file, and counted. A copy that silently brought nothing would
        // leave an empty directory, which open() would take for a new database
        // and create — so every case downstream would run against a fresh empty
        // store instead of the fixture, and the ones that only look at counts
        // would not notice.
        trace("copying " + source.string());
        size_t copied = 0, available = 0;
        for (auto const& e : fs::directory_iterator(source)) {
            if ( ! e.is_regular_file()) continue;
            ++available;
            fs::copy_file(e.path(), dir / e.path().filename(),
                          fs::copy_options::overwrite_existing, ec);
            REQUIRE_FALSE(ec);
            ++copied;
        }
        REQUIRE(available > 0);
        REQUIRE(copied == available);
        trace("copied " + std::to_string(copied) + " files");
    }

    ~fixture_copy() {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
};

/// Every entry the database holds, read through the public API.
std::vector<logical_entry> read_all_full(utxoz::full_db const& db) {
    std::vector<logical_entry> out;
    auto const scanned = db.for_each_entry(
        [&](utxoz::raw_outpoint const& k, uint32_t h, std::span<uint8_t const> v) {
            out.push_back({k, h, std::vector<uint8_t>(v.begin(), v.end())});
        });
    REQUIRE(scanned.has_value());
    return out;
}

std::vector<logical_entry> read_all_reference(utxoz::reference_db const& db) {
    std::vector<logical_entry> out;
    auto const scanned = db.for_each_entry(
        [&](utxoz::raw_outpoint const& k, uint32_t h, uint32_t file_number, uint32_t offset) {
            out.push_back({k, h, utxoz::testing::reference_value_bytes(file_number, offset)});
        });
    REQUIRE(scanned.has_value());
    return out;
}

utxoz::raw_outpoint key_of(uint64_t n) {
    utxoz::raw_outpoint k{};
    std::memcpy(k.data(), &n, sizeof(n));
    uint32_t const idx = uint32_t(n % 7);
    std::memcpy(k.data() + 32, &idx, sizeof(idx));
    return k;
}

bool this_abi_wrote_the_fixtures() {
    auto const manifest = parse_manifest();
    return as_u64(manifest.at("written_by").at("platform_abi_id"))
           == utxoz::detail::platform_abi_id;
}

} // namespace

// =============================================================================
// An ABI the fixtures were not written on
// =============================================================================

TEST_CASE("a fixture from another ABI is refused, not misread", "[compat]") {
    // Not a missing fixture: the fixture declares the ABI it was written under,
    // and a build that does not share it must say so rather than read the bytes
    // as if they meant the same thing. This is the whole test on such a
    // platform, and it is a real result — unlike the hash vectors, which
    // certify an algorithm per ABI and must fail where none is pinned.
    if (this_abi_wrote_the_fixtures()) {
        SUCCEED("this ABI wrote the fixtures; the refusal case does not apply here");
        return;
    }

    fixture_copy f("full-two-generations");
    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::abi_mismatch);
}

// =============================================================================
// The ABI they were written on
// =============================================================================

TEST_CASE("the full-mode fixture still reads, entry for entry", "[compat]") {
    if ( ! this_abi_wrote_the_fixtures()) {
        SUCCEED("fixtures were written on another ABI; the refusal case covers this platform");
        return;
    }

    auto const expected = manifest_for("full-two-generations");
    fixture_copy f("full-two-generations");

    // Physical: it opens at all, which means the config and every stamp were
    // accepted before anything reached a map.
    auto opened = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);
    CHECK(db.size() == expected.entries);

    // Present and absent, because a reader that answers everything and a reader
    // that answers nothing both keep a count right.
    // find() answers from the active versions and nothing else, which is the
    // contract, so half of this fixture is deliberately out of its reach: the
    // forced rotation at key 210 left everything below it in container 0's
    // historical generation. Asking for those and getting `not_resolved` is the
    // correct answer, and resolve() is what turns it into a real one — which is
    // also what puts the historical file through the cache, stamp and all.
    // In an active version: container 0 above the rotation, and the containers
    // that never rotated at all.
    CHECK(db.find(key_of(210), 900000).has_value());   // container 0, generation 1
    CHECK(db.find(key_of(2), 900000).has_value());     // container 1, its only generation
    CHECK(db.find(key_of(3), 900000).has_value());     // container 2
    CHECK(db.find(key_of(4), 900000).has_value());     // container 3
    CHECK(db.find(key_of(5), 900000).has_value());     // container 4

    // In the historical generation: not found by find(), found by resolve().
    auto const historical = key_of(6);   // container 0, below the rotation
    auto const missed = db.find(historical, 900000);
    REQUIRE_FALSE(missed.has_value());
    CHECK(missed.error() == utxoz::error_code::not_resolved);

    std::vector<utxoz::lookup_request> const batch{{historical, 900000}, {key_of(999999), 900000}};
    auto const resolved = db.resolve(batch);
    REQUIRE(resolved.has_value());
    CHECK(resolved->found.size() == 1);
    CHECK(resolved->absent.size() == 1);

    // Logical: everything, canonically ordered, digested. This is the check that
    // notices values read back wrong while the count still agrees.
    auto const entries = read_all_full(db);
    CHECK(entries.size() == expected.entries);
    CHECK(utxoz::testing::global_digest(utxoz::storage_mode::full, entries)
          == expected.global_digest);

    db.close();
}

TEST_CASE("the reference-mode fixture still reads, entry for entry", "[compat]") {
    if ( ! this_abi_wrote_the_fixtures()) {
        SUCCEED("fixtures were written on another ABI");
        return;
    }

    auto const expected = manifest_for("reference-two-generations");
    fixture_copy f("reference-two-generations");

    auto opened = utxoz::reference_db::open_for_testing(f.dir, false);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);
    CHECK(db.size() == expected.entries);

    // find() answers from the active versions and nothing else, which is the
    // contract, so half of this fixture is deliberately out of its reach: the
    // forced rotation at key 210 left everything below it in container 0's
    // historical generation. Asking for those and getting `not_resolved` is the
    // correct answer, and resolve() is what turns it into a real one — which is
    // also what puts the historical file through the cache, stamp and all.
    CHECK(db.find(key_of(300), 900000).has_value());   // above the rotation

    auto const historical = key_of(6);
    auto const missed = db.find(historical, 900000);
    REQUIRE_FALSE(missed.has_value());
    CHECK(missed.error() == utxoz::error_code::not_resolved);

    std::vector<utxoz::lookup_request> const batch{{historical, 900000}, {key_of(999999), 900000}};
    auto const resolved = db.resolve(batch);
    REQUIRE(resolved.has_value());
    CHECK(resolved->found.size() == 1);
    CHECK(resolved->absent.size() == 1);

    auto const entries = read_all_reference(db);
    CHECK(entries.size() == expected.entries);
    CHECK(utxoz::testing::global_digest(utxoz::storage_mode::reference, entries)
          == expected.global_digest);

    db.close();
}

// =============================================================================
// Each generation, separately
// =============================================================================

TEST_CASE("every generation the manifest names is there and holds what it held", "[compat]") {
    if ( ! this_abi_wrote_the_fixtures()) {
        SUCCEED("fixtures were written on another ABI");
        return;
    }

    // The global digest would stay green if a whole generation went missing and
    // its entries turned up somewhere else — which is exactly what a compaction
    // does legitimately, and exactly what a broken read would do silently. Per
    // segment, that cannot hide.
    auto const expected = manifest_for("full-two-generations");
    fixture_copy f("full-two-generations");

    auto const config = utxoz::detail::read_config_file(f.dir / "utxoz_config.dat");
    REQUIRE(config.has_value());

    size_t checked = 0;
    for (auto const& s : expected.segments) {
        INFO("container " << s.kind << " generation " << s.generation);
        auto const file = f.dir / fmt::format(utxoz::detail::data_file_format, s.kind, s.generation);
        REQUIRE(fs::exists(file));

        trace("opening segment " + file.string());
        auto opened = utxoz::detail::open_existing_segment(file);
        trace("opened segment");
        REQUIRE(opened.has_value());
        REQUIRE(utxoz::detail::validate_stamp(
                    **opened, file,
                    utxoz::detail::local_identity(config->database_id, s.kind, s.generation))
                    .has_value());

        // Read with the size class the container actually is; anything else
        // would be reinterpreting, which is what the stamp just refused to allow.
        std::vector<logical_entry> entries;
        auto collect = [&]<size_t I>(std::integral_constant<size_t, I>) {
            auto const found = utxoz::detail::find_single_named<
                utxoz::detail::utxo_map<utxoz::container_sizes[I]>>(
                **opened, utxoz::detail::map_object_name, file);
            REQUIRE(found.has_value());
            for (auto const& stored : **found) {
                auto const data = stored.second.get_data();
                logical_entry e;
                e.key = stored.first;
                e.height = stored.second.block_height;
                e.value.assign(data.begin(), data.end());
                entries.push_back(std::move(e));
            }
        };
        switch (s.kind) {
            case 0: collect(std::integral_constant<size_t, 0>{}); break;
            case 1: collect(std::integral_constant<size_t, 1>{}); break;
            case 2: collect(std::integral_constant<size_t, 2>{}); break;
            case 3: collect(std::integral_constant<size_t, 3>{}); break;
            case 4: collect(std::integral_constant<size_t, 4>{}); break;
            default: FAIL("the manifest names a container this build does not have");
        }

        CHECK(entries.size() == s.entries);
        CHECK(utxoz::testing::segment_digest(s.kind, s.generation, entries) == s.digest);
        ++checked;
    }
    CHECK(checked == expected.segments.size());
    CHECK(checked >= 6);   // five containers, and container 0 twice
}

TEST_CASE("every reference generation the manifest names holds what it held", "[compat]") {
    if ( ! this_abi_wrote_the_fixtures()) {
        SUCCEED("fixtures were written on another ABI");
        return;
    }

    // The same reason as the full-mode case: a global digest stays green while
    // entries move between generations, which is legitimate after a compaction
    // and silent after a bad read. Reference mode has its own generations and
    // its own value shape, so it needs its own check rather than inheriting one.
    auto const expected = manifest_for("reference-two-generations");
    fixture_copy f("reference-two-generations");

    auto const config = utxoz::detail::read_config_file(f.dir / "utxoz_config.dat");
    REQUIRE(config.has_value());

    size_t checked = 0;
    for (auto const& s : expected.segments) {
        INFO("reference generation " << s.generation);
        CHECK(s.kind == utxoz::detail::reference_container_kind);
        auto const file = f.dir / fmt::format(utxoz::detail::reference_data_file_format,
                                              s.generation);
        REQUIRE(fs::exists(file));

        trace("opening segment " + file.string());
        auto opened = utxoz::detail::open_existing_segment(file);
        trace("opened segment");
        REQUIRE(opened.has_value());
        REQUIRE(utxoz::detail::validate_stamp(
                    **opened, file,
                    utxoz::detail::local_identity(config->database_id, s.kind, s.generation))
                    .has_value());

        auto const found = utxoz::detail::find_single_named<utxoz::detail::reference_map_t>(
            **opened, utxoz::detail::map_object_name, file);
        REQUIRE(found.has_value());

        std::vector<logical_entry> entries;
        for (auto const& stored : **found) {
            logical_entry e;
            e.key = stored.first;
            e.height = stored.second.height;
            e.value = utxoz::testing::reference_value_bytes(stored.second.file_number,
                                                           stored.second.offset);
            entries.push_back(std::move(e));
        }

        CHECK(entries.size() == s.entries);
        CHECK(utxoz::testing::segment_digest(s.kind, s.generation, entries) == s.digest);
        ++checked;
    }
    CHECK(checked == expected.segments.size());
    CHECK(checked >= 2);
}

// =============================================================================
// Writing to it, and the originals staying put
// =============================================================================

TEST_CASE("an old database is still writable, and reopens with what was added", "[compat]") {
    if ( ! this_abi_wrote_the_fixtures()) {
        SUCCEED("fixtures were written on another ABI");
        return;
    }

    auto const expected = manifest_for("full-two-generations");
    fixture_copy f("full-two-generations");

    auto const added = key_of(500000);
    std::vector<uint8_t> const value(43, 0xC3);
    {
        auto opened = utxoz::full_db::open_for_testing(f.dir, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        REQUIRE(db.insert(added, value, 800000).has_value());
        db.close();
    }

    auto opened = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    CHECK(db.size() == expected.entries + 1);
    auto const found = db.find(added, 800001);
    REQUIRE(found.has_value());
    CHECK(found->block_height == 800000);
    CHECK(std::ranges::equal(found->data, value));

    // Everything that was there before still is.
    auto entries = read_all_full(db);
    CHECK(entries.size() == expected.entries + 1);
    std::erase_if(entries, [&](logical_entry const& e) { return e.key == added; });
    CHECK(utxoz::testing::global_digest(utxoz::storage_mode::full, entries)
          == expected.global_digest);

    db.close();
}

TEST_CASE("an old reference database is still writable, and reopens with what was added",
          "[compat]") {
    if ( ! this_abi_wrote_the_fixtures()) {
        SUCCEED("fixtures were written on another ABI");
        return;
    }

    auto const expected = manifest_for("reference-two-generations");
    fixture_copy f("reference-two-generations");

    auto const added = key_of(500000);
    {
        auto opened = utxoz::reference_db::open_for_testing(f.dir, false);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        REQUIRE(db.insert(added, 4242, 777777, 800000).has_value());
        db.close();
    }

    auto opened = utxoz::reference_db::open_for_testing(f.dir, false);
    REQUIRE(opened.has_value());
    auto db = std::move(*opened);

    CHECK(db.size() == expected.entries + 1);
    auto const found = db.find(added, 800001);
    REQUIRE(found.has_value());
    // Field by field: a reference value is three numbers, and a reader that
    // returned the right height with the wrong position would be no use.
    CHECK(found->block_height == 800000);
    CHECK(found->file_number == 4242);
    CHECK(found->offset == 777777);

    auto entries = read_all_reference(db);
    CHECK(entries.size() == expected.entries + 1);
    std::erase_if(entries, [&](logical_entry const& e) { return e.key == added; });
    CHECK(utxoz::testing::global_digest(utxoz::storage_mode::reference, entries)
          == expected.global_digest);

    db.close();
}

TEST_CASE("nothing in the fixture tree was touched", "[compat]") {
    // Every case above works on a copy. This is what makes that a property
    // rather than an intention: the committed artefacts are compared against the
    // sizes and digests the manifest recorded for them.
    auto const manifest = parse_manifest();

    size_t files_checked = 0;
    for (auto const& entry : manifest.at("fixtures").as_array()) {
        auto const& o = entry.as_object();
        auto const name = as_str(o.at("name"));
        for (auto const& file : o.at("files").as_array()) {
            auto const& fo = file.as_object();
            auto const path = fixtures_root() / name / as_str(fo.at("name"));
            INFO(name << "/" << as_str(fo.at("name")));
            REQUIRE(fs::exists(path));

            auto const bytes = read_bytes(path);
            CHECK(bytes.size() == as_u64(fo.at("bytes")));
            CHECK(utxoz::testing::file_digest(bytes) == as_str(fo.at("sha256")));
            ++files_checked;
        }

        // And nothing on disk that the manifest does not name. A file that
        // appeared beside the fixtures would otherwise travel with them,
        // uncertified.
        size_t on_disk = 0;
        for (auto const& e : fs::directory_iterator(fixtures_root() / name)) {
            if (e.is_regular_file()) ++on_disk;
        }
        CHECK(on_disk == o.at("files").as_array().size());
    }

    // Non-vacuity: a manifest whose file list stopped parsing would otherwise
    // pass by checking nothing at all.
    CHECK(files_checked >= 18);
}

// =============================================================================
// The manifest as a certificate rather than a description
// =============================================================================

TEST_CASE("a manifest is held to its schema, not merely parsed", "[compat]") {
    auto const original = read_text(fixtures_root() / "manifest.json");
    REQUIRE_FALSE(schema_error(original).has_value());

    // Each mutation replaces text that must be there. A replacement that found
    // nothing would leave the document valid and the case green while testing
    // nothing at all, so the substitution itself is required to have happened.
    auto broken = [&](std::string const& from, std::string const& to) {
        auto const at = original.find(from);
        REQUIRE(at != std::string::npos);
        auto copy = original;
        copy.replace(at, from.size(), to);
        return copy;
    };

    SECTION("a key repeated within one object") {
        // Boost.JSON accepts this and keeps the last of the two, so parsing
        // cleanly says nothing: the second value would have won every check.
        auto const doc = broken("\"written_by\": {",
                                "\"written_by\": {\n    \"map_layout_epoch\": 27,");
        REQUIRE_FALSE(boost::json::parse(doc).is_null());
        CHECK(schema_error(doc).has_value());
    }
    SECTION("a storage mode that does not exist") {
        CHECK(schema_error(broken("\"storage_mode\": \"full\"",
                                  "\"storage_mode\": \"ful\"")).has_value());
    }
    SECTION("an endianness that does not exist") {
        CHECK(schema_error(broken("\"endian\": \"little\"",
                                  "\"endian\": \"middle\"")).has_value());
    }
    SECTION("a negative count") {
        CHECK(schema_error(broken("\"entries\": 420", "\"entries\": -1")).has_value());
    }
    SECTION("two fixtures under one name") {
        CHECK(schema_error(broken("\"name\": \"reference-two-generations\"",
                                  "\"name\": \"full-two-generations\"")).has_value());
    }
    SECTION("two segments claiming one generation") {
        CHECK(schema_error(broken("{\"container_kind\": 0, \"generation\": 1,",
                                  "{\"container_kind\": 0, \"generation\": 0,")).has_value());
    }
    SECTION("a digest that is not one") {
        CHECK(schema_error(broken("\"global_logical_digest\": \"0",
                                  "\"global_logical_digest\": \"Z")).has_value());
    }
    SECTION("a digest of the wrong length") {
        CHECK(schema_error(broken("\"global_logical_digest\": \"0",
                                  "\"global_logical_digest\": \"")).has_value());
    }
    SECTION("a truncated document") {
        CHECK(schema_error(original.substr(0, original.size() / 2)).has_value());
    }
    SECTION("a field that is gone") {
        CHECK(schema_error(broken("\"hash_epoch\"", "\"hash_epock\"")).has_value());
    }
}

TEST_CASE("the manifest agrees with the bytes it certifies", "[compat]") {
    // Until this existed the header was prose: `map_layout_epoch` could read 27
    // and every case stayed green, because nothing compared it to anything. What
    // makes it a certificate is that each field is held against the binary
    // configs it claims to describe.
    auto const manifest = parse_manifest();
    auto const& written = manifest.at("written_by").as_object();

    auto const declared_abi = uint32_t(as_u64(written.at("platform_abi_id")));
    auto const declared_endian = as_str(written.at("endian"));
    auto const declared_size_t = as_u64(written.at("sizeof_size_t"));
    auto const declared_pointer = as_u64(written.at("sizeof_pointer"));
    auto const declared_offset_ptr = as_u64(written.at("sizeof_offset_ptr"));

    // The ABI id is a packing of the three widths and the endianness, so the
    // header cannot claim one thing in the number and another in the words.
    // The data ABI still decomposes, and the header cannot claim one thing in the
    // number and another in the words.
    auto const declared_data_abi = uint32_t(as_u64(written.at("data_abi_id")));
    uint32_t const recomposed_data = ((declared_endian == "little" ? 1u : 2u) << 24)
                                   | (uint32_t(declared_size_t) << 16)
                                   | (uint32_t(declared_pointer) << 8)
                                   | uint32_t(declared_offset_ptr);
    CHECK(recomposed_data == declared_data_abi);

    // The full identity is a fold, because what goes into it does not fit a byte
    // each. Recomputed from the ingredients the header declares: an id that meant
    // something other than what is written beside it would not survive this.
    auto const declared_sync = uint32_t(as_u64(written.at("sync_abi_family")));
    CHECK(declared_sync >= 1);
    CHECK(declared_sync <= 3);
    CHECK(utxoz::detail::mix_identity({
              uint32_t(as_u64(written.at("platform_abi_encoding"))),
              declared_data_abi,
              declared_sync,
              uint32_t(as_u64(written.at("sizeof_interprocess_mutex"))),
              uint32_t(as_u64(written.at("alignof_interprocess_mutex"))),
              uint32_t(as_u64(written.at("sizeof_segment_manager"))),
              uint32_t(as_u64(written.at("alignof_segment_manager"))),
              uint32_t(as_u64(written.at("platform_tag"))),
          })
          == declared_abi);

    if ( ! this_abi_wrote_the_fixtures()) {
        SUCCEED("the rest compares against configs this build cannot open");
        return;
    }

    auto const declared_config_format = as_u64(written.at("config_format"));
    auto const declared_geometry = uint32_t(as_u64(written.at("geometry_id")));
    auto const declared_layout = uint32_t(as_u64(written.at("map_layout_epoch")));
    auto const declared_hash = uint32_t(as_u64(written.at("hash_epoch")));
    auto const declared_boost = as_u64(written.at("boost_version"));

    CHECK(declared_config_format == utxoz::detail::store_config::current_format);

    size_t configs = 0;
    for (auto const& entry : manifest.at("fixtures").as_array()) {
        auto const& o = entry.as_object();
        auto const name = as_str(o.at("name"));
        INFO(name);

        auto const config = utxoz::detail::read_config_file(
            fixtures_root() / name / "utxoz_config.dat");
        REQUIRE(config.has_value());

        // Every fixture in the set shares the identities the header declares.
        CHECK(config->geometry_id == declared_geometry);
        CHECK(config->map_layout_epoch == declared_layout);
        CHECK(config->hash_epoch == declared_hash);
        CHECK(config->platform_abi_id == declared_abi);
        CHECK(config->boost_version == declared_boost);

        auto const declared_mode = as_str(o.at("storage_mode"));
        CHECK(config->mode == mode_from(declared_mode));

        // And the segments: every one declared is on disk with a stamp that
        // agrees, and nothing on disk is left undeclared.
        auto const prefix = declared_mode == "full" ? std::string("cont_") : std::string("compact_v");
        size_t physical = 0;
        for (auto const& e : fs::directory_iterator(fixtures_root() / name)) {
            auto const filename = e.path().filename().string();
            if (filename.starts_with(prefix) && filename.ends_with(".dat")) ++physical;
        }
        CHECK(physical == o.at("segments").as_array().size());

        for (auto const& seg : o.at("segments").as_array()) {
            auto const& so = seg.as_object();
            auto const kind = uint32_t(as_u64(so.at("container_kind")));
            auto const generation = as_u64(so.at("generation"));
            auto const file = kind == utxoz::detail::reference_container_kind
                ? fixtures_root() / name / fmt::format(utxoz::detail::reference_data_file_format,
                                                       generation)
                : fixtures_root() / name / fmt::format(utxoz::detail::data_file_format, kind,
                                                       generation);
            INFO("segment " << kind << "/" << generation);
            REQUIRE(fs::exists(file));

            trace("opening segment " + file.string());
            auto opened = utxoz::detail::open_existing_segment(file);
            trace("opened segment");
            REQUIRE(opened.has_value());
            CHECK(utxoz::detail::validate_stamp(
                      **opened, file,
                      utxoz::detail::local_identity(config->database_id, kind, generation))
                      .has_value());
        }
        ++configs;
    }
    CHECK(configs == manifest.at("fixtures").as_array().size());
    CHECK(configs >= 2);
}

TEST_CASE("the Boost a fixture was written with is recorded, not enforced", "[compat]") {
    // Several Boost releases can share one map_layout_epoch, so the version is
    // there to tell an operator which bytes they are looking at. A build that
    // consulted it would invalidate every database on an upgrade — this is what
    // says it does not.
    if ( ! this_abi_wrote_the_fixtures()) {
        SUCCEED("fixtures were written on another ABI");
        return;
    }

    fixture_copy f("full-two-generations");

    auto config = utxoz::detail::read_config_file(f.dir / "utxoz_config.dat");
    REQUIRE(config.has_value());
    auto const original = config->boost_version;

    config->boost_version = 999999;   // a Boost that has never existed
    auto const encoded = utxoz::detail::encode_config(*config);
    {
        std::ofstream ofs(f.dir / "utxoz_config.dat", std::ios::binary | std::ios::trunc);
        REQUIRE(ofs);
        ofs.write(reinterpret_cast<char const*>(encoded.data()),
                  static_cast<std::streamsize>(encoded.size()));
    }
    CHECK(original != 999999);

    auto opened = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE(opened.has_value());
    CHECK(opened->size() == manifest_for("full-two-generations").entries);
    opened->close();
}

// =============================================================================
// The seam the fixtures are built with
// =============================================================================

TEST_CASE("the rotation seam rotates, and only when asked", "[compat][seam]") {
    // The multi-generation fixtures exist because of this seam, so a seam that
    // quietly did nothing would leave them single-generation and every case
    // above would certify less than it says. Both halves are needed: that it
    // rotates when set, and that nothing rotates when it is not.
    using utxoz::detail::failpoints;

    auto count_versions = [](fs::path const& dir) {
        size_t n = 0;
        for (auto const& e : fs::directory_iterator(dir)) {
            if (e.path().filename().string().starts_with("cont_0_v")) ++n;
        }
        return n;
    };

    std::vector<uint8_t> const value(8, 0x11);

    SECTION("off by default, nothing rotates") {
        // Guarded like its sibling. This section is the one that would pass for
        // the wrong reason if a seam were left armed elsewhere — and it is the
        // one that proves the default, so it has the most to lose from that.
        failpoints::scoped_reset const disarm;

        auto const dir = make_unique_path("seam_off");
        std::error_code ec;
        fs::remove_all(dir, ec);

        auto opened = utxoz::full_db::open_for_testing(dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        for (uint64_t i = 0; i < 50; ++i) {
            REQUIRE(db.insert(key_of(i * 6), value, 700000).has_value());
        }
        db.close();

        CHECK(count_versions(dir) == 1);
        fs::remove_all(dir, ec);
    }

    SECTION("set, it rotates exactly as many times as asked") {
        auto const dir = make_unique_path("seam_on");
        std::error_code ec;
        fs::remove_all(dir, ec);

        // Before the seam is armed, not after: a failing REQUIRE below leaves by
        // throwing, and a rotation seam left armed would follow the next test in.
        failpoints::scoped_reset const disarm;

        auto opened = utxoz::full_db::open_for_testing(dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        REQUIRE(db.insert(key_of(0), value, 700000).has_value());
        failpoints::force_rotations.store(2, std::memory_order_relaxed);
        for (uint64_t i = 1; i < 50; ++i) {
            REQUIRE(db.insert(key_of(i * 6), value, 700000).has_value());
        }
        CHECK(failpoints::force_rotations.load(std::memory_order_relaxed) == 0);
        db.close();

        // Three generations: the original and one per forced rotation. Not more,
        // which is what says the seam is consumed rather than sticky.
        CHECK(count_versions(dir) == 3);

        // And the entries survived the rotations, spread across the generations
        // — a seam that rotated by losing what was there would be no use for
        // building a fixture.
        auto reopened = utxoz::full_db::open_for_testing(dir, false);
        REQUIRE(reopened.has_value());
        CHECK(reopened->size() == 50);
        reopened->close();

        fs::remove_all(dir, ec);
    }
}
