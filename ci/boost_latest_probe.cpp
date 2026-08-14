// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file boost_latest_probe.cpp
 * @brief Opens the committed fixtures with whatever Boost it was built against,
 *        and says what happened.
 *
 * Deliberately not a Catch2 test. It is built outside the project's normal
 * dependency wiring, against a Boost the project does not pin, so it carries no
 * test framework and reports through its exit code:
 *
 *     0  opened, and every entry matched the digest recorded for it
 *     3  refused to open — the format barrier declining, which is an answer
 *     4  opened, and the contents are not what was stored
 *     1  something else went wrong; the run says nothing about the format
 *
 * The digests come from the same header the suite uses, so the two cannot drift
 * into disagreeing about what a database logically holds.
 */

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <boost/version.hpp>

#include <utxoz/database.hpp>

#include "support/logical_digest.hpp"

namespace fs = std::filesystem;
using utxoz::testing::logical_entry;

namespace {

std::string read_text(fs::path const& p) {
    std::ifstream ifs(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
}

std::string field_after(std::string const& text, size_t from, std::string const& key) {
    auto const at = text.find("\"" + key + "\":", from);
    if (at == std::string::npos) return {};
    auto cursor = text.find_first_not_of(" \t", at + key.size() + 3);
    if (cursor == std::string::npos) return {};
    if (text[cursor] == '"') {
        auto const end = text.find('"', cursor + 1);
        return text.substr(cursor + 1, end - cursor - 1);
    }
    auto const end = text.find_first_of(",\n}", cursor);
    return text.substr(cursor, end - cursor);
}

/// The fixtures are never opened where they live: an open takes the directory's
/// claim and writes a lock file.
fs::path copy_out(fs::path const& source, fs::path const& into) {
    std::error_code ec;
    fs::create_directories(into, ec);
    for (auto const& e : fs::directory_iterator(source)) {
        if ( ! e.is_regular_file()) continue;
        fs::copy_file(e.path(), into / e.path().filename(),
                      fs::copy_options::overwrite_existing, ec);
        if (ec) return {};
    }
    return into;
}

} // namespace

int main() {
    std::cout << "probe built against Boost " << BOOST_VERSION << "\n";

    fs::path const root = UTXOZ_FIXTURES_DIR;
    auto const manifest = read_text(root / "manifest.json");
    if (manifest.empty()) {
        std::cerr << "no manifest at " << root << "\n";
        return 1;
    }

    auto const at = manifest.find("\"name\": \"full-two-generations\"");
    if (at == std::string::npos) {
        std::cerr << "the manifest does not describe the full-mode fixture\n";
        return 1;
    }
    auto const expected_entries = std::stoull(field_after(manifest, at, "entries"));
    auto const expected_digest = field_after(manifest, at, "global_logical_digest");

    auto const work = fs::temp_directory_path() / "utxoz-boost-probe";
    std::error_code ec;
    fs::remove_all(work, ec);
    if (copy_out(root / "full-two-generations", work).empty()) {
        std::cerr << "could not copy the fixture out\n";
        return 1;
    }

    auto opened = utxoz::full_db::open_for_testing(work, false);
    if ( ! opened) {
        std::cout << "refused to open: error " << int(opened.error()) << "\n";
        fs::remove_all(work, ec);
        return 3;
    }
    auto db = std::move(*opened);

    std::vector<logical_entry> entries;
    auto const scanned = db.for_each_entry(
        [&](utxoz::raw_outpoint const& k, uint32_t h, std::span<uint8_t const> v) {
            logical_entry e;
            e.key = k;
            e.height = h;
            e.value.assign(v.begin(), v.end());
            entries.push_back(std::move(e));
        });
    if ( ! scanned) {
        std::cout << "opened, but the traversal failed: error " << int(scanned.error()) << "\n";
        db.close();
        fs::remove_all(work, ec);
        return 4;
    }

    auto const digest = utxoz::testing::global_digest(utxoz::storage_mode::full, entries);
    std::cout << "entries: " << entries.size() << " (expected " << expected_entries << ")\n";
    std::cout << "digest:   " << digest << "\n";
    std::cout << "expected: " << expected_digest << "\n";

    db.close();
    fs::remove_all(work, ec);

    if (entries.size() != expected_entries || digest != expected_digest) return 4;
    return 0;
}
