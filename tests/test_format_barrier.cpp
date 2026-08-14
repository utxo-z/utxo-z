// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_format_barrier.cpp
 * @brief What a database says about itself, and what happens when it says
 *        something this build cannot serve.
 *
 * The store keeps its maps inside memory-mapped files, so the internal layout of
 * `boost::unordered_flat_map` is the file format — and Boost.Interprocess
 * resolves a named object by name alone, so a build whose idea of that layout
 * differs would reinterpret a file rather than refuse it. Nothing about that is
 * observable afterwards; it has to be caught before anything reads the map.
 *
 * Two things stand in the way and both are tested here:
 *
 * - the config, which is the authority and is checked before a single segment is
 *   mapped;
 * - a stamp in every segment, checked after mapping and before `db_map` is so
 *   much as looked for.
 *
 * The cases that matter most are the ones where the map is perfectly readable
 * and the answer is still no. A refusal that only happens when the data is also
 * broken would not be a barrier.
 */

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/unordered/detail/foa/core.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>

#include "detail/format_identity.hpp"
#include "detail/segment_stamp.hpp"
#include "detail/durability.hpp"
#include "detail/record_bytes.hpp"
#include "detail/store_config_io.hpp"

namespace bip = boost::interprocess;
namespace fs = std::filesystem;

using utxoz::detail::database_id_t;
using utxoz::detail::encode_config;
using utxoz::detail::encode_stamp;
using utxoz::detail::local_identity;
using utxoz::detail::read_config_file;
using utxoz::detail::segment_identity;
using utxoz::detail::segment_stamp;
using utxoz::detail::store_config;

namespace {

std::atomic<uint64_t> barrier_counter{0};

fs::path make_unique_path(std::string_view tag) {
    auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto const id = barrier_counter.fetch_add(1);
    return fs::path(fmt::format("./test_fmt_{}_{}_{}_{}", tag, getpid(), ts, id));
}

utxoz::raw_outpoint outpoint_of(uint64_t n) {
    utxoz::raw_outpoint key{};
    std::memcpy(key.data(), &n, sizeof(n));
    return key;
}

fs::path config_of(fs::path const& dir) { return dir / "utxoz_config.dat"; }
fs::path v0_of(fs::path const& dir) { return dir / "cont_0_v00000.dat"; }

/// A small full-mode database with one version and a handful of entries.
struct fresh_db {
    fs::path dir;
    size_t entries = 6;

    explicit fresh_db(std::string_view tag) : dir(make_unique_path(tag)) {
        auto opened = utxoz::full_db::open_for_testing(dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        std::vector<uint8_t> const value(8, 0x5A);
        for (uint64_t i = 0; i < entries; ++i) {
            REQUIRE(db.insert(outpoint_of(i), value, 700000).has_value());
        }
        db.close();
    }

    ~fresh_db() {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
};

void write_bytes(fs::path const& path, std::vector<uint8_t> const& bytes) {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    REQUIRE(ofs);
    ofs.write(reinterpret_cast<char const*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    ofs.close();
    REQUIRE_FALSE(ofs.fail());
}

/// Recomputes the trailing checksum over everything before it, so a hand-edited
/// config is genuinely well formed rather than merely accepted by a check that
/// happens to run earlier.
void reseal(std::vector<uint8_t>& bytes) {
    auto const covered = std::span<uint8_t const>(bytes.data(), bytes.size() - sizeof(uint32_t));
    uint32_t const sum = utxoz::detail::record_bytes::checksum(covered);
    std::memcpy(bytes.data() + bytes.size() - sizeof(uint32_t), &sum, sizeof(sum));
}

std::vector<uint8_t> read_bytes(fs::path const& path) {
    std::ifstream ifs(path, std::ios::binary);
    REQUIRE(ifs);
    return {std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
}

/// Rewrites the config with one field changed, leaving everything else as the
/// database actually wrote it.
void rewrite_config(fs::path const& dir, auto&& mutate) {
    auto config = read_config_file(config_of(dir));
    REQUIRE(config.has_value());
    mutate(*config);
    write_bytes(config_of(dir), encode_config(*config));
}

/// Replaces a version file with a segment built to order: with or without a
/// stamp, with or without a map, and with whatever the stamp should say.
///
/// The map matters wherever a case is about the stamp: without it those cases
/// would only prove that a damaged file is refused, which is not the property.
/// The property is that a *readable* file is refused on what it says about
/// itself.
struct replacement {
    std::optional<segment_identity> stamp;   ///< absent means: carry no stamp
    bool with_map = true;
};

void rebuild(fs::path const& data_file, replacement const& how) {
    constexpr size_t bytes = 16u * 1024u * 1024u;
    std::error_code ec;
    fs::remove(data_file, ec);

    bip::managed_mapped_file segment(bip::create_only, data_file.c_str(), bytes);
    if (how.stamp) {
        segment.construct<segment_stamp>(segment_stamp::object_name)(encode_stamp(*how.stamp));
    }
    if (how.with_map) {
        using map_t = utxoz::detail::utxo_map<utxoz::container_sizes[0]>;
        segment.construct<map_t>(utxoz::detail::map_object_name)(
            64, utxoz::detail::outpoint_hash{}, utxoz::detail::outpoint_equal{},
            segment.get_allocator<map_t::value_type>());
    }
}

segment_identity identity_of(fs::path const& dir, uint32_t kind, uint64_t version) {
    auto const config = read_config_file(config_of(dir));
    REQUIRE(config.has_value());
    return local_identity(config->database_id, kind, version);
}

} // namespace

// =============================================================================
// A. A database this build wrote
// =============================================================================

TEST_CASE("a new database records what it was written under", "[format]") {
    fresh_db f("new");

    auto const config = read_config_file(config_of(f.dir));
    REQUIRE(config.has_value());
    CHECK(config->mode == utxoz::storage_mode::full);
    CHECK(config->geometry_id == utxoz::detail::geometry_id);
    CHECK(config->map_layout_epoch == utxoz::detail::map_layout_epoch);
    CHECK(config->hash_epoch == utxoz::detail::hash_epoch);
    CHECK(config->platform_abi_id == utxoz::detail::platform_abi_id);
    CHECK(config->database_id != database_id_t{});   // not left zeroed

    // And it reopens, which is the half that would otherwise go unnoticed if the
    // barrier were simply refusing everything.
    auto reopened = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE(reopened.has_value());
    CHECK(reopened->size() == f.entries);
    CHECK(reopened->find(outpoint_of(0), 700000).has_value());
}

TEST_CASE("two databases do not share an identity", "[format]") {
    fresh_db a("id_a");
    fresh_db b("id_b");

    auto const ca = read_config_file(config_of(a.dir));
    auto const cb = read_config_file(config_of(b.dir));
    REQUIRE(ca.has_value());
    REQUIRE(cb.has_value());
    CHECK(ca->database_id != cb->database_id);
}

TEST_CASE("reference mode records what it was written under", "[format]") {
    auto const dir = make_unique_path("ref_new");
    {
        auto opened = utxoz::reference_db::open_for_testing(dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        for (uint64_t i = 0; i < 5; ++i) {
            REQUIRE(db.insert(outpoint_of(i), 1, uint32_t(i), 700000).has_value());
        }
        db.close();
    }

    auto const config = read_config_file(config_of(dir));
    REQUIRE(config.has_value());
    CHECK(config->mode == utxoz::storage_mode::reference);

    auto reopened = utxoz::reference_db::open_for_testing(dir, false);
    REQUIRE(reopened.has_value());
    CHECK(reopened->size() == 5);

    reopened->close();
    std::error_code ec;
    fs::remove_all(dir, ec);
}

// =============================================================================
// B. Refused on what the config says, before a segment is mapped
// =============================================================================

TEST_CASE("the config refuses a geometry this build does not write", "[format]") {
    fresh_db f("cfg_geometry");
    rewrite_config(f.dir, [](store_config& c) { c.geometry_id += 1; });

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::geometry_mismatch);
}

TEST_CASE("the config refuses a map layout this build is not certified for", "[format]") {
    fresh_db f("cfg_layout");
    rewrite_config(f.dir, [](store_config& c) { c.map_layout_epoch += 1; });

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::layout_mismatch);
}

TEST_CASE("the config refuses a hash that would put every key elsewhere", "[format]") {
    fresh_db f("cfg_hash");
    rewrite_config(f.dir, [](store_config& c) { c.hash_epoch += 1; });

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::hash_mismatch);
}

TEST_CASE("the config refuses another platform's bytes", "[format]") {
    fresh_db f("cfg_abi");
    rewrite_config(f.dir, [](store_config& c) { c.platform_abi_id ^= 0x01000000u; });

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::abi_mismatch);
}

TEST_CASE("a file whose data ABI matches but whose machinery does not is refused",
          "[format]") {
    // The case the identity used to miss, and the reason it was widened. Linux
    // and macOS agree on endianness and on the width of a size_t, a pointer and
    // an offset_ptr — and write files the other cannot read, because a segment
    // keeps its allocator's mutex inside the mapped file and Boost compiles a
    // different one for each. What followed was not a wrong answer but no answer:
    // the segment mapped, and the first named-object lookup never returned.
    fresh_db f("cfg_sync_abi");

    // Same data ABI, different persisted machinery — exactly what one of those
    // files looks like from here.
    auto const foreign = utxoz::detail::compute_platform_abi_id(
        utxoz::detail::sync_abi_family == 1u ? 3u : 1u, utxoz::detail::platform_tag);
    REQUIRE(foreign != utxoz::detail::platform_abi_id);
    rewrite_config(f.dir, [&](store_config& c) { c.platform_abi_id = foreign; });

    // Armed so that any attempt to open a container fails with its own code. The
    // refusal below therefore proves an ordering rather than merely an outcome:
    // the config is read and rejected before anything is mapped. Asserting that
    // by timing would prove nothing — a fast answer is not an early one.
    utxoz::detail::failpoints::scoped_reset const disarm;
    utxoz::detail::failpoints::fail_container_open.store(true, std::memory_order_relaxed);

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::abi_mismatch);
}

TEST_CASE("a config format from the future is not corruption", "[format]") {
    fresh_db f("cfg_future");

    // Well formed, correctly checksummed, and simply not something this build
    // knows. Reporting it as damage would send an operator looking for a fault
    // that is not there.
    //
    // Resealed on purpose: the format is checked before the checksum, so an
    // edit that left the checksum stale would pass this case while proving
    // nothing about a genuinely well-formed future file.
    auto bytes = read_bytes(config_of(f.dir));
    uint32_t const future = store_config::current_format + 1;
    std::memcpy(bytes.data() + 4, &future, sizeof(future));
    reseal(bytes);
    write_bytes(config_of(f.dir), bytes);

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::format_unsupported);
}

TEST_CASE("a config that is both unknown and damaged is still reported as unknown", "[format]") {
    // The ordering made explicit. A file whose format this build does not know
    // cannot be checked any further — the layout of what follows is exactly what
    // is unknown — so the format answer comes first and the checksum is never
    // consulted. Pinned so the ordering is a decision rather than an accident.
    fresh_db f("cfg_future_torn");

    auto bytes = read_bytes(config_of(f.dir));
    uint32_t const future = store_config::current_format + 1;
    std::memcpy(bytes.data() + 4, &future, sizeof(future));
    bytes[20] ^= 0xFF;   // and left unsealed
    write_bytes(config_of(f.dir), bytes);

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::format_unsupported);
}

TEST_CASE("a config damaged in flight is corruption", "[format]") {
    fresh_db f("cfg_torn");

    auto bytes = read_bytes(config_of(f.dir));
    bytes[20] ^= 0xFF;   // inside the identities, so the checksum no longer holds
    write_bytes(config_of(f.dir), bytes);

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::config_file_corrupt);
}

// =============================================================================
// C. Refused on what a segment says, with a perfectly good map inside it
// =============================================================================

TEST_CASE("a segment is refused on its stamp, not on its contents", "[format]") {
    // Each of these files opens, and holds a map this build would read happily.
    // What is wrong with them is only what they say about themselves — which is
    // the whole point: if the data had to be broken too, the barrier would be
    // catching something else.
    struct damage {
        char const* what;
        utxoz::error_code expected;
        void (*apply)(segment_identity&);
    };

    auto const cases = std::vector<damage>{
        {"geometry", utxoz::error_code::geometry_mismatch,
         [](segment_identity& i) { i.geometry_id += 1; }},
        {"layout", utxoz::error_code::layout_mismatch,
         [](segment_identity& i) { i.map_layout_epoch += 1; }},
        {"hash", utxoz::error_code::hash_mismatch,
         [](segment_identity& i) { i.hash_epoch += 1; }},
        {"abi", utxoz::error_code::abi_mismatch,
         [](segment_identity& i) { i.platform_abi_id ^= 0x01000000u; }},
        {"identity", utxoz::error_code::database_identity_mismatch,
         [](segment_identity& i) { i.database_id[0] ^= 0xFF; }},
        {"container", utxoz::error_code::segment_misplaced,
         [](segment_identity& i) { i.container_kind = 3; }},
        {"generation", utxoz::error_code::segment_misplaced,
         [](segment_identity& i) { i.version = 7; }},
    };

    for (auto const& c : cases) {
        INFO("stamp damaged in: " << c.what);
        fresh_db f("stamp");

        auto identity = identity_of(f.dir, 0, 0);
        c.apply(identity);
        rebuild(v0_of(f.dir), {identity, true});

        auto const db = utxoz::full_db::open_for_testing(f.dir, false);
        REQUIRE_FALSE(db.has_value());
        CHECK(db.error() == c.expected);
    }
}

TEST_CASE("a segment with no stamp is not a segment this build wrote", "[format]") {
    fresh_db f("stamp_absent");

    rebuild(v0_of(f.dir), {std::nullopt, true});

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::segment_stamp_missing);
}

TEST_CASE("a stamp damaged in flight is refused as damage", "[format]") {
    fresh_db f("stamp_torn");

    auto identity = identity_of(f.dir, 0, 0);
    auto stamp = encode_stamp(identity);
    stamp.raw[10] ^= 0xFF;   // inside the identities; the checksum no longer holds

    // Written by hand rather than through rebuild(), because the point is a stamp
    // whose bytes do not decode — which encode_stamp() cannot produce.
    {
        constexpr size_t bytes = 16u * 1024u * 1024u;
        std::error_code ec;
        fs::remove(v0_of(f.dir), ec);
        bip::managed_mapped_file segment(bip::create_only, v0_of(f.dir).c_str(), bytes);
        segment.construct<segment_stamp>(segment_stamp::object_name)(stamp);
    }

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::segment_stamp_corrupt);
}

// =============================================================================
// E. A file that wandered in from another database
// =============================================================================

TEST_CASE("a version file from another database is refused", "[format]") {
    fresh_db a("swap_a");
    fresh_db b("swap_b");

    // Both are healthy databases of the same shape, written by the same build.
    // Everything about the file is right except whose it is.
    std::error_code ec;
    fs::remove(v0_of(a.dir), ec);
    fs::copy_file(v0_of(b.dir), v0_of(a.dir), ec);
    REQUIRE_FALSE(ec);

    auto const db = utxoz::full_db::open_for_testing(a.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::database_identity_mismatch);
}

// =============================================================================
// F. Databases from before the format said what it was
// =============================================================================

TEST_CASE("a format-1 database is refused and left exactly as it was", "[format]") {
    fresh_db f("legacy");

    // Its own config, rewritten in the shape v0.10.0 wrote: magic, version 1,
    // mode. Nothing else about the database is touched.
    {
        std::ofstream ofs(config_of(f.dir), std::ios::binary | std::ios::trunc);
        REQUIRE(ofs);
        char const magic[4] = {'U', 'T', 'X', 'O'};
        ofs.write(magic, 4);
        uint32_t const version = 1;
        ofs.write(reinterpret_cast<char const*>(&version), sizeof(version));
        uint8_t const mode_byte = uint8_t(utxoz::storage_mode::full);
        ofs.write(reinterpret_cast<char const*>(&mode_byte), sizeof(mode_byte));
    }

    // Every file, byte for byte, before the attempt.
    // Every file except the claim. The lock records its holder, and the claim is
    // taken before the config is read on purpose — so that a database which is
    // both in use and unreadable reports the one that stops you either way. Its
    // contents changing is that working, not the database being touched.
    std::vector<std::pair<fs::path, std::vector<uint8_t>>> before;
    for (auto const& entry : fs::directory_iterator(f.dir)) {
        if ( ! entry.is_regular_file()) continue;
        if (entry.path().filename() == ".utxoz.lock") continue;
        before.emplace_back(entry.path(), read_bytes(entry.path()));
    }
    REQUIRE(before.size() >= 2);   // at least the config and one version file

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::migration_required);

    // Refused, and nothing about it repaired, upgraded or half-upgraded. A
    // partial in-place migration is the one outcome worse than refusing.
    size_t after_count = 0;
    for (auto const& entry : fs::directory_iterator(f.dir)) {
        if (entry.is_regular_file() && entry.path().filename() != ".utxoz.lock") ++after_count;
    }
    CHECK(after_count == before.size());
    for (auto const& [path, bytes] : before) {
        INFO("unchanged: " << path.filename().string());
        REQUIRE(fs::exists(path));
        CHECK(read_bytes(path) == bytes);
    }
}

// =============================================================================
// D. Creating is not the same as opening
// =============================================================================

TEST_CASE("a version file is stamped once and holds one map", "[format]") {
    fresh_db f("once");

    bip::managed_mapped_file segment(bip::open_only, v0_of(f.dir).c_str());

    auto const stamp = segment.find<segment_stamp>(segment_stamp::object_name);
    REQUIRE(stamp.first != nullptr);
    CHECK(stamp.second == 1);

    auto const map = segment.find<utxoz::detail::utxo_map<utxoz::container_sizes[0]>>(utxoz::detail::map_object_name);
    REQUIRE(map.first != nullptr);
    CHECK(map.second == 1);
}

TEST_CASE("reopening does not build a second anything", "[format]") {
    fresh_db f("reopen");

    auto const before = read_bytes(v0_of(f.dir)).size();
    {
        auto db = utxoz::full_db::open_for_testing(f.dir, false);
        REQUIRE(db.has_value());
        CHECK(db->size() == f.entries);
        db->close();
    }

    bip::managed_mapped_file segment(bip::open_only, v0_of(f.dir).c_str());
    CHECK(segment.find<segment_stamp>(segment_stamp::object_name).second == 1);
    CHECK(segment.find<utxoz::detail::utxo_map<utxoz::container_sizes[0]>>(utxoz::detail::map_object_name).second == 1);
    CHECK(read_bytes(v0_of(f.dir)).size() == before);
}

TEST_CASE("a stamped file with no map is not an empty version", "[format]") {
    // The failure this replaces: find_or_construct saw no map and made one, so a
    // file whose map could not be reached came back as a fresh empty container
    // and the entries it held stopped existing.
    fresh_db f("stamped_no_map");

    rebuild(v0_of(f.dir), {identity_of(f.dir, 0, 0), false});

    auto const db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE_FALSE(db.has_value());
    CHECK(db.error() == utxoz::error_code::version_unreadable);
}

// =============================================================================
// The hash the format depends on
// =============================================================================

TEST_CASE("the effective hash is the one every stored file was written under", "[format][hash]") {
    // What every stored file was written under is not hash_outpoint() but
    //
    //     mulx_mix(hash_outpoint(key))
    //
    // because Boost 1.91 picks the mixing with
    //
    //     mix_policy = conditional_t<hash_is_avalanching<Hash>, no_mix, mulx_mix>
    //
    // and outpoint_hash does not declare the trait. Both halves are pinned, as
    // literals: an expected value computed by calling the function under test is
    // the function agreeing with itself, and stays green through any change.
    //
    // mulx_mix lives in Boost's detail namespace. Reaching it here — in a test,
    // never in the library — is the only way to pin the thing the format
    // actually depends on. If Boost reimplements mulx, this goes red, which is
    // the point: it would mean every key in every existing database had moved.
    static_assert( ! boost::hash_is_avalanching<utxoz::detail::outpoint_hash>::value);
    CHECK_FALSE(boost::hash_is_avalanching<utxoz::detail::outpoint_hash>::value);

    using mixer = boost::unordered::detail::foa::mulx_mix;

    auto key_from = [](std::initializer_list<std::pair<size_t, uint8_t>> set) {
        utxoz::raw_outpoint k{};
        for (auto const& [at, value] : set) k[at] = value;
        return k;
    };
    utxoz::raw_outpoint all_ones{};
    all_ones.fill(0xFF);

    struct sample {
        char const* what;
        utxoz::raw_outpoint key;
        uint64_t hashed;    // hash_outpoint(key)
        uint64_t mixed;     // mulx_mix(hash_outpoint(key)) — what FOA places by
    };

    // Per ABI, because hash_outpoint() returns size_t: on a 32-bit target it
    // reads four bytes of the txid instead of eight, and Boost's mulx uses a
    // different multiplier (0xE817FB2D rather than phi). Every value below was
    // measured by running on that ABI, not derived on paper.
    //
    // An ABI with no table is a failure, not a skip. hash_epoch is written into
    // and validated against databases on every platform, so certifying it on one
    // and passing green on the others would be claiming something that had never
    // been checked.
    std::vector<sample> samples;
    if (utxoz::detail::data_abi_id == utxoz::detail::lp64_le_abi) {
        samples = {
            {"all zero",     utxoz::raw_outpoint{},  0x0000000000000000ull, 0x0000000000000000ull},
            {"all ones",     all_ones,               0x1eecfda47f4a7c14ull, 0x61d8a19a3f903356ull},
            {"txid byte 0",  key_from({{0, 1}}),     0x0000000000000001ull, 0x9e3779b97f4a7c15ull},
            {"index byte",   key_from({{32, 1}}),    0x9e3779b97f4a7c15ull, 0xbe8cab644efdda51ull},
            {"index top",    key_from({{35, 1}}),    0xb97f4a7c15000000ull, 0x506a8adc900c1b40ull},
            {"index high",   key_from({{35, 0x80}}), 0xbfa53e0a80000000ull, 0x1155af3647490885ull},
            // Deliberate: hash_outpoint reads the first eight bytes of the txid
            // and the four index bytes, so byte 31 does not reach it and this
            // collides with the all-zero key. Equality still tells the two apart
            // — the hash only chooses a bucket. Pinned because "improving" the
            // hash to cover all 36 bytes would move every key in every database.
            {"txid byte 31", key_from({{31, 1}}),    0x0000000000000000ull, 0x0000000000000000ull},
        };
    } else if (utxoz::detail::data_abi_id == utxoz::detail::wasm32_le_abi) {
        samples = {
            {"all zero",     utxoz::raw_outpoint{},  0x0000000000000000ull, 0x0000000000000000ull},
            {"all ones",     all_ones,               0x000000007f4a7c14ull, 0x00000000deec0683ull},
            {"txid byte 0",  key_from({{0, 1}}),     0x0000000000000001ull, 0x00000000e817fb2dull},
            {"index byte",   key_from({{32, 1}}),    0x000000007f4a7c15ull, 0x00000000e6c40bb9ull},
            {"index top",    key_from({{35, 1}}),    0x0000000015000000ull, 0x00000000a209f79aull},
            {"index high",   key_from({{35, 0x80}}), 0x0000000080000000ull, 0x00000000f40bfd96ull},
            // Same collision, same reason: on this ABI only four txid bytes are
            // read, so byte 31 is even further outside what the hash sees.
            {"txid byte 31", key_from({{31, 1}}),    0x0000000000000000ull, 0x0000000000000000ull},
        };
    } else {
        FAIL("no hash vectors are pinned for data_abi_id "
             << utxoz::detail::data_abi_id
             << "; a database written here would claim hash_epoch "
             << utxoz::detail::hash_epoch
             << " without that ever having been verified. Measure the vectors on this "
                "ABI and add them, or exclude the ABI from the on-disk contract.");
    }

    for (auto const& s : samples) {
        INFO("key: " << s.what);
        CHECK(uint64_t(utxoz::hash_outpoint(s.key)) == s.hashed);
        CHECK(uint64_t(mixer::mix(utxoz::detail::outpoint_hash{}, s.key)) == s.mixed);
    }

    // The mixing is not the identity, so a build that stopped applying it would
    // fail the vectors above rather than merely produce different numbers.
    CHECK(uint64_t(mixer::mix(utxoz::detail::outpoint_hash{}, key_from({{0, 1}})))
          != uint64_t(utxoz::hash_outpoint(key_from({{0, 1}}))));

    // And end to end: a database written now finds the keys it wrote. This is
    // the weakest of the three — it writes and reads with the same hash, so it
    // stays green even if every key moved relative to an older database — which
    // is why the literals above carry the weight.
    fresh_db f("hash_roundtrip");
    auto db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE(db.has_value());
    for (uint64_t i = 0; i < f.entries; ++i) {
        INFO("key " << i);
        CHECK(db->find(outpoint_of(i), 700000).has_value());
    }
}

// =============================================================================
// A half-built version file is not left behind
// =============================================================================

namespace {

using utxoz::detail::failpoints;

/// Clears both creation failpoints however the case leaves.
struct creation_failpoints {
    ~creation_failpoints() {
        failpoints::fail_after_segment_create.store(false, std::memory_order_relaxed);
        failpoints::fail_after_segment_stamp.store(false, std::memory_order_relaxed);
    }
};

size_t count_data_files(fs::path const& dir, std::string_view prefix) {
    size_t n = 0;
    for (auto const& e : fs::directory_iterator(dir)) {
        if (e.path().filename().string().starts_with(prefix)) ++n;
    }
    return n;
}

} // namespace

TEST_CASE("a creation that fails leaves no file to block the retry", "[format][rollback]") {
    // Both windows between the file existing and the caller being told it does.
    // Before the fix the file survived either one, and since a rotation computes
    // the same next version every time, the retry met a name that was already
    // taken and the container was left with nothing active and no way back.
    struct where {
        char const* what;
        std::atomic<bool>* point;
    };
    auto const points = std::vector<where>{
        {"after the file exists, before it is stamped", &failpoints::fail_after_segment_create},
        {"after the stamp, before the map", &failpoints::fail_after_segment_stamp},
    };

    for (auto const& p : points) {
        INFO("failing " << p.what);

        SECTION("full mode") {
            creation_failpoints const reset;
            auto const dir = make_unique_path("rollback_full");
            std::error_code ec;
            fs::remove_all(dir, ec);
            fs::create_directories(dir, ec);

            p.point->store(true, std::memory_order_relaxed);
            auto const refused = utxoz::full_db::open_for_testing(dir, true);
            REQUIRE_FALSE(refused.has_value());

            // Nothing half-built survives, which is what makes the retry possible.
            CHECK(count_data_files(dir, "cont_0_v") == 0);

            p.point->store(false, std::memory_order_relaxed);
            auto db = utxoz::full_db::open_for_testing(dir, false);
            REQUIRE(db.has_value());
            CHECK(db->insert(outpoint_of(1), std::vector<uint8_t>(8, 1), 700000).has_value());
            db->close();
            fs::remove_all(dir, ec);
        }

        SECTION("reference mode") {
            creation_failpoints const reset;
            auto const dir = make_unique_path("rollback_ref");
            std::error_code ec;
            fs::remove_all(dir, ec);
            fs::create_directories(dir, ec);

            p.point->store(true, std::memory_order_relaxed);
            auto const refused = utxoz::reference_db::open_for_testing(dir, true);
            REQUIRE_FALSE(refused.has_value());
            CHECK(count_data_files(dir, "compact_v") == 0);

            p.point->store(false, std::memory_order_relaxed);
            auto db = utxoz::reference_db::open_for_testing(dir, false);
            REQUIRE(db.has_value());
            CHECK(db->insert(outpoint_of(1), 1, 0, 700000).has_value());
            db->close();
            fs::remove_all(dir, ec);
        }
    }
}

TEST_CASE("a creation that succeeds keeps its file", "[format][rollback]") {
    // The control. Without it the case above would pass just as well against a
    // rollback that fired unconditionally and removed every file it created.
    fresh_db f("rollback_control");
    CHECK(count_data_files(f.dir, "cont_0_v") == 1);
    CHECK(fs::exists(v0_of(f.dir)));

    auto db = utxoz::full_db::open_for_testing(f.dir, false);
    REQUIRE(db.has_value());
    CHECK(db->size() == f.entries);
}

TEST_CASE("reopening a version file does not rewrite it", "[format][rollback]") {
    // Named for what it checks. It does not exercise creating over an occupied
    // name, and cannot: the catalogue is built from the directory, so a version
    // file that is there puts the open on the existing-file path and the create
    // path is never entered.
    //
    // That unreachability is also why the rollback can never meet a file it did
    // not make. `create_only` refuses a name that is taken and throws before the
    // guard is armed, so the guard's `fs::remove` only ever sees a file this call
    // created moments earlier. The property is structural rather than tested,
    // and saying so is better than a case whose name claims otherwise.
    fresh_db f("reopen_intact");
    auto const before = read_bytes(v0_of(f.dir));

    {
        auto db = utxoz::full_db::open_for_testing(f.dir, false);
        REQUIRE(db.has_value());
        CHECK(db->size() == f.entries);
        db->close();
    }
    CHECK(read_bytes(v0_of(f.dir)) == before);
}
