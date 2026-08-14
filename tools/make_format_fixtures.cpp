// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file make_format_fixtures.cpp
 * @brief Builds compatibility fixtures and the manifest that certifies them.
 *
 * Run by hand, never by the test suite. It writes to a directory named on the
 * command line and refuses one that already holds a manifest, so the official
 * fixtures cannot be replaced by running it in the wrong place. Promoting a
 * candidate is a person copying files after reading the diff.
 *
 *     make_format_fixtures <output-dir>
 *
 * The databases are built through the ordinary public API. The one exception is
 * the rotation seam, used so that a fixture can have more than one generation
 * without the hundred thousand entries a natural rotation needs — and it drives
 * the real rotation path rather than assembling a second file, because a fixture
 * built by hand would attest to our ability to produce plausible bytes and not to
 * what the writer writes.
 *
 * Every fixture carries a freshly drawn `database_id`, so regenerating produces
 * different bytes. That is expected: the manifest records the checksums of the
 * files that were actually committed, and a regenerated candidate is compared
 * against them by a person, not by a test.
 */

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/version.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>

#include "detail/database_lock.hpp"
#include "detail/durability.hpp"
#include "detail/file_cache.hpp"
#include "detail/format_identity.hpp"
#include "detail/segment_open.hpp"
#include "detail/segment_stamp.hpp"
#include "detail/store_config_io.hpp"
#include "detail/utxo_value.hpp"
#include "detail/version_catalog.hpp"

#include "logical_digest.hpp"

namespace fs = std::filesystem;
namespace bip = boost::interprocess;

using namespace utxoz;
using namespace utxoz::detail;
using utxoz::testing::logical_entry;

namespace {

/// The five containers, unrolled. database_impl has its own version of this and
/// keeps it private, which is right: this tool is not the library.
template <typename F, size_t... Is>
void for_each_container(F&& f, std::index_sequence<Is...>) {
    (f(std::integral_constant<size_t, Is>{}), ...);
}

raw_outpoint key_of(uint64_t n) {
    raw_outpoint k{};
    std::memcpy(k.data(), &n, sizeof(n));
    uint32_t const idx = uint32_t(n % 7);
    std::memcpy(k.data() + 32, &idx, sizeof(idx));
    return k;
}

/// Value sizes chosen so every container is exercised: 8 and 43 land in
/// container 0, 60 in 1, 100 in 2, 200 in 3, 4000 in 4.
constexpr size_t value_sizes[] = {8, 43, 60, 100, 200, 4000};

std::vector<uint8_t> value_of(uint64_t n) {
    return std::vector<uint8_t>(value_sizes[n % 6], uint8_t(n & 0xFF));
}

struct segment_record {
    uint32_t kind = 0;
    uint64_t generation = 0;
    size_t entries = 0;
    std::string digest;
};

/// Walks one version file the way a reader would, and says what it holds.
template <size_t Index>
std::vector<logical_entry> read_full_segment(fs::path const& file, database_id_t const& id,
                                             uint64_t version) {
    auto opened = open_existing_segment(file);
    if ( ! opened) throw std::runtime_error("cannot open " + file.string());
    if (auto const ok = validate_stamp(**opened, file,
                                       local_identity(id, uint32_t(Index), version));
        ! ok) {
        throw std::runtime_error("stamp refused for " + file.string());
    }
    auto const found = find_single_named<utxo_map<container_sizes[Index]>>(
        **opened, map_object_name, file);
    if ( ! found) throw std::runtime_error("no map in " + file.string());

    std::vector<logical_entry> out;
    for (auto const& [key, val] : **found) {
        auto const data = val.get_data();
        out.push_back({key, val.block_height, std::vector<uint8_t>(data.begin(), data.end())});
    }
    return out;
}

std::vector<logical_entry> read_reference_segment(fs::path const& file, database_id_t const& id,
                                                  uint64_t version) {
    auto opened = open_existing_segment(file);
    if ( ! opened) throw std::runtime_error("cannot open " + file.string());
    if (auto const ok = validate_stamp(**opened, file,
                                       local_identity(id, reference_container_kind, version));
        ! ok) {
        throw std::runtime_error("stamp refused for " + file.string());
    }
    auto const found = find_single_named<reference_map_t>(**opened, map_object_name, file);
    if ( ! found) throw std::runtime_error("no map in " + file.string());

    std::vector<logical_entry> out;
    for (auto const& [key, val] : **found) {
        out.push_back({key, val.height,
                       utxoz::testing::reference_value_bytes(val.file_number, val.offset)});
    }
    return out;
}

/// The whole file, or an exception.
///
/// These bytes become the SHA-256 the manifest certifies, so a read that stopped
/// early would not produce a wrong file — it would produce a correct checksum of
/// the wrong contents, and the fixtures would certify that instead. Read in one
/// block against the size the filesystem reports, and every way of falling short
/// of it is an error rather than a shorter answer.
std::vector<uint8_t> read_file(fs::path const& p) {
    std::error_code ec;
    auto const size = fs::file_size(p, ec);
    if (ec) throw std::runtime_error("cannot size " + p.string() + ": " + ec.message());

    std::ifstream ifs(p, std::ios::binary);
    if ( ! ifs) throw std::runtime_error("cannot read " + p.string());

    std::vector<uint8_t> out(static_cast<size_t>(size));
    if (size != 0) {
        ifs.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size));
        if (ifs.gcount() != static_cast<std::streamsize>(size)) {
            throw std::runtime_error("short read on " + p.string());
        }
        if (ifs.bad()) throw std::runtime_error("I/O error reading " + p.string());
    }

    // Clean end of file and nothing beyond it: a file that grew between the size
    // and the read would leave bytes this digest does not cover.
    ifs.peek();
    if ( ! ifs.eof()) throw std::runtime_error(p.string() + " is longer than it measured");
    return out;
}

std::string quoted(std::string const& s) { return "\"" + s + "\""; }

} // namespace

int main(int argc, char** argv) try {
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: make_format_fixtures <output-dir> [database-id-hex]\n"
                     "\n"
                     "  database-id-hex  32 hex characters. Fixes the identity the databases\n"
                     "                   take instead of drawing one, so that what two builds\n"
                     "                   write can be compared byte for byte. Leave it out for\n"
                     "                   real fixtures: they should each have their own.\n";
        return 2;
    }
    fs::path const out = argv[1];

    // Every seam this tool arms is disarmed on the way out, including the way out
    // that a thrown exception takes. This normally runs as its own process and
    // exits either way, but the seams are process-wide state and this code is
    // reachable from a test harness that does not exit.
    failpoints::scoped_reset const disarm;

    if (argc == 3) {
        std::string const hex = argv[2];
        if (hex.size() != 32) throw std::runtime_error("the database id must be 32 hex characters");
        std::array<uint8_t, 16> id{};
        for (size_t i = 0; i < id.size(); ++i) {
            id[i] = uint8_t(std::stoul(hex.substr(i * 2, 2), nullptr, 16));
        }
        failpoints::forced_database_id = id;
        failpoints::force_database_id.store(true, std::memory_order_relaxed);
        std::cout << "using the identity given on the command line\n";
    }

    if (fs::exists(out / "manifest.json")) {
        std::cerr << out << " already holds a manifest. Generate candidates somewhere else and "
                     "promote them by hand after reading the diff.\n";
        return 2;
    }
    fs::create_directories(out);

    // Proved writable before anything is built, not after. The manifest is
    // written last, so a destination that cannot take it would otherwise be
    // discovered at the end of several minutes of work — and in CI, by a test
    // that has to do all of that work to reach the failure it is checking.
    {
        auto const probe = out / "manifest.json.tmp";
        std::ofstream check(probe, std::ios::binary | std::ios::trunc);
        if ( ! check) throw std::runtime_error("cannot write the manifest into " + out.string());
        check.close();
        std::error_code ignored;
        fs::remove(probe, ignored);
    }

    struct fixture {
        std::string name;
        storage_mode mode;
        std::string global_digest;
        std::vector<segment_record> segments;
        std::map<std::string, std::pair<size_t, std::string>> files;   // name -> (size, sha256)
        database_id_t id{};
        size_t entries = 0;
    };
    std::vector<fixture> built;

    // ---- full mode, two generations of container 0 -------------------------
    {
        fixture f{"full-two-generations", storage_mode::full, {}, {}, {}, {}, 0};
        auto const dir = out / f.name;
        fs::remove_all(dir);

        auto opened = full_db::open_for_testing(dir, true);
        if ( ! opened) throw std::runtime_error("could not create the full fixture");
        auto db = std::move(*opened);

        constexpr uint64_t count = 420;
        for (uint64_t i = 0; i < count; ++i) {
            // Halfway through, one forced rotation of container 0, so the
            // fixture has a historical generation as well as an active one.
            if (i == count / 2) {
                failpoints::force_rotations.store(1, std::memory_order_relaxed);
            }
            if ( ! db.insert(key_of(i), value_of(i), 700000 + uint32_t(i))) {
                throw std::runtime_error("insert failed");
            }
        }
        failpoints::force_rotations.store(0, std::memory_order_relaxed);
        f.entries = db.size();
        db.close();

        auto const config = read_config_file(dir / "utxoz_config.dat");
        if ( ! config) throw std::runtime_error("the fixture wrote no readable config");
        f.id = config->database_id;

        std::vector<logical_entry> all;
        for_each_container([&](auto I) {
            auto const listed = enumerate_versions(dir, fmt::format("cont_{}_v", I.value));
            if ( ! listed) throw std::runtime_error("cannot list versions");
            for (auto const v : *listed) {
                auto const file = dir / fmt::format(data_file_format, I.value, v);
                auto entries = read_full_segment<I.value>(file, f.id, v);
                f.segments.push_back({uint32_t(I.value), v, entries.size(),
                                      utxoz::testing::segment_digest(uint32_t(I.value), v, entries)});
                all.insert(all.end(), entries.begin(), entries.end());
            }
        }, std::make_index_sequence<container_count>{});
        f.global_digest = utxoz::testing::global_digest(f.mode, all);

        // The generator checks its own seam: a fixture that quietly came out
        // with one generation would certify nothing about multi-version reads.
        auto const c0 = std::ranges::count_if(f.segments, [](auto const& s) { return s.kind == 0; });
        if (c0 < 2) throw std::runtime_error("container 0 did not rotate; the fixture would be "
                                             "single-generation and prove less than it claims");

        for (auto const& e : fs::directory_iterator(dir)) {
            if ( ! e.is_regular_file()) continue;
            if (e.path().filename() == database_lock::file_name) continue;
            auto const bytes = read_file(e.path());
            f.files[e.path().filename().string()] = {bytes.size(),
                                                     utxoz::testing::file_digest(bytes)};
        }
        fs::remove(dir / database_lock::file_name);
        built.push_back(std::move(f));
    }

    // ---- reference mode, two generations -----------------------------------
    {
        fixture f{"reference-two-generations", storage_mode::reference, {}, {}, {}, {}, 0};
        auto const dir = out / f.name;
        fs::remove_all(dir);

        auto opened = reference_db::open_for_testing(dir, true);
        if ( ! opened) throw std::runtime_error("could not create the reference fixture");
        auto db = std::move(*opened);

        constexpr uint64_t count = 420;
        for (uint64_t i = 0; i < count; ++i) {
            if (i == count / 2) {
                failpoints::force_rotations.store(1, std::memory_order_relaxed);
            }
            if ( ! db.insert(key_of(i), uint32_t(i % 11), uint32_t(i * 17), 700000 + uint32_t(i))) {
                throw std::runtime_error("insert failed");
            }
        }
        failpoints::force_rotations.store(0, std::memory_order_relaxed);
        f.entries = db.size();
        db.close();

        auto const config = read_config_file(dir / "utxoz_config.dat");
        if ( ! config) throw std::runtime_error("the fixture wrote no readable config");
        f.id = config->database_id;

        auto const listed = enumerate_versions(dir, "compact_v");
        if ( ! listed) throw std::runtime_error("cannot list versions");

        std::vector<logical_entry> all;
        for (auto const v : *listed) {
            auto const file = dir / fmt::format(reference_data_file_format, v);
            auto entries = read_reference_segment(file, f.id, v);
            f.segments.push_back({reference_container_kind, v, entries.size(),
                                  utxoz::testing::segment_digest(reference_container_kind, v,
                                                                 entries)});
            all.insert(all.end(), entries.begin(), entries.end());
        }
        f.global_digest = utxoz::testing::global_digest(f.mode, all);

        if (f.segments.size() < 2) {
            throw std::runtime_error("the reference container did not rotate");
        }

        for (auto const& e : fs::directory_iterator(dir)) {
            if ( ! e.is_regular_file()) continue;
            if (e.path().filename() == database_lock::file_name) continue;
            auto const bytes = read_file(e.path());
            f.files[e.path().filename().string()] = {bytes.size(),
                                                     utxoz::testing::file_digest(bytes)};
        }
        fs::remove(dir / database_lock::file_name);
        built.push_back(std::move(f));
    }

    // ---- the manifest ------------------------------------------------------
    // Written beside the final name and renamed into place. A failure partway
    // through would otherwise leave a truncated manifest.json behind — and the
    // next run refuses a directory that already holds one, so the tool would be
    // unable to replace what it had just broken.
    auto const manifest_final = out / "manifest.json";
    auto const manifest_temp = out / "manifest.json.tmp";
    std::ofstream manifest(manifest_temp, std::ios::binary | std::ios::trunc);
    if ( ! manifest) throw std::runtime_error("could not create " + manifest_temp.string());
    manifest << "{\n";
    manifest << "  \"manifest_format\": 1,\n";
    manifest << "  \"written_by\": {\n";
    manifest << "    \"boost_version\": " << BOOST_VERSION << ",\n";
    manifest << "    \"config_format\": " << store_config::current_format << ",\n";
    manifest << "    \"geometry_id\": " << geometry_id << ",\n";
    manifest << "    \"map_layout_epoch\": " << map_layout_epoch << ",\n";
    manifest << "    \"hash_epoch\": " << hash_epoch << ",\n";
    manifest << "    \"platform_abi_id\": " << platform_abi_id << ",\n";
    // The ingredients beside the result, so the suite can recompute the fold and
    // a header cannot declare one thing while the id means another.
    manifest << "    \"platform_abi_encoding\": " << platform_abi_encoding << ",\n";
    manifest << "    \"data_abi_id\": " << data_abi_id << ",\n";
    manifest << "    \"sync_abi_family\": " << sync_abi_family << ",\n";
    manifest << "    \"sizeof_interprocess_mutex\": "
             << sizeof(boost::interprocess::interprocess_mutex) << ",\n";
    manifest << "    \"alignof_interprocess_mutex\": "
             << alignof(boost::interprocess::interprocess_mutex) << ",\n";
    manifest << "    \"sizeof_segment_manager\": " << sizeof(segment_manager_t) << ",\n";
    manifest << "    \"alignof_segment_manager\": " << alignof(segment_manager_t) << ",\n";
    manifest << "    \"platform_tag\": " << platform_tag << ",\n";
    manifest << "    \"endian\": " << quoted(std::endian::native == std::endian::little
                                             ? "little" : "big") << ",\n";
    manifest << "    \"sizeof_size_t\": " << sizeof(size_t) << ",\n";
    manifest << "    \"sizeof_pointer\": " << sizeof(void*) << ",\n";
    manifest << "    \"sizeof_offset_ptr\": " << sizeof(bip::offset_ptr<void>) << "\n";
    manifest << "  },\n";
    manifest << "  \"fixtures\": [\n";

    for (size_t fi = 0; fi < built.size(); ++fi) {
        auto const& f = built[fi];
        manifest << "    {\n";
        manifest << "      \"name\": " << quoted(f.name) << ",\n";
        manifest << "      \"storage_mode\": "
                 << quoted(f.mode == storage_mode::full ? "full" : "reference") << ",\n";
        manifest << "      \"entries\": " << f.entries << ",\n";
        manifest << "      \"global_logical_digest\": " << quoted(f.global_digest) << ",\n";
        manifest << "      \"segments\": [\n";
        for (size_t si = 0; si < f.segments.size(); ++si) {
            auto const& s = f.segments[si];
            manifest << "        {\"container_kind\": " << s.kind
                     << ", \"generation\": " << s.generation
                     << ", \"entries\": " << s.entries
                     << ", \"logical_digest\": " << quoted(s.digest) << "}"
                     << (si + 1 < f.segments.size() ? "," : "") << "\n";
        }
        manifest << "      ],\n";
        manifest << "      \"files\": [\n";
        size_t n = 0;
        for (auto const& [name, info] : f.files) {
            manifest << "        {\"name\": " << quoted(name)
                     << ", \"bytes\": " << info.first
                     << ", \"sha256\": " << quoted(info.second) << "}"
                     << (++n < f.files.size() ? "," : "") << "\n";
        }
        manifest << "      ]\n";
        manifest << "    }" << (fi + 1 < built.size() ? "," : "") << "\n";
    }
    manifest << "  ]\n}\n";
    manifest.close();

    // Both, and after the close: a write that failed on a full disk sets the
    // stream's bad bit and is only reliably visible once the buffer has been
    // flushed. Announcing success before this is how the tool comes to report a
    // certificate that does not exist.
    if (manifest.fail()) {
        std::error_code ignored;
        fs::remove(manifest_temp, ignored);
        throw std::runtime_error("could not write " + manifest_temp.string());
    }

    std::error_code renamed;
    fs::rename(manifest_temp, manifest_final, renamed);
    if (renamed) {
        std::error_code ignored;
        fs::remove(manifest_temp, ignored);
        throw std::runtime_error("could not publish the manifest: " + renamed.message());
    }

    std::cout << "wrote " << built.size() << " fixtures and a manifest to " << out << "\n";
    for (auto const& f : built) {
        std::cout << "  " << f.name << ": " << f.entries << " entries, "
                  << f.segments.size() << " segments, " << f.files.size() << " files\n";
    }
    return 0;
} catch (std::exception const& e) {
    std::cerr << "failed: " << e.what() << "\n";
    return 1;
}
