// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file read_file.hpp
 * @brief Reading a whole file, in blocks.
 *
 * The obvious spelling — constructing a container from a pair of
 * `istreambuf_iterator`s — reads one character at a time through a virtual call,
 * and reallocates as it goes. That is tolerable for a manifest and not for a
 * container file: these fixtures are 10 MiB each and the suite reads eighteen of
 * them several times over. Implementations differ by more than an order of
 * magnitude on that path, which is how a suite that finishes in a second on one
 * platform stops finishing on another.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace utxoz::testing {

/// The whole file, or an empty vector if it cannot be read.
inline std::vector<uint8_t> read_file_bytes(std::filesystem::path const& p) {
    std::error_code ec;
    auto const size = std::filesystem::file_size(p, ec);
    if (ec) return {};

    std::ifstream ifs(p, std::ios::binary);
    if ( ! ifs) return {};

    std::vector<uint8_t> out(static_cast<size_t>(size));
    if (size != 0) {
        ifs.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size));
        if (ifs.gcount() != static_cast<std::streamsize>(size)) return {};
    }
    return out;
}

inline std::string read_file_text(std::filesystem::path const& p) {
    auto const bytes = read_file_bytes(p);
    return {bytes.begin(), bytes.end()};
}

} // namespace utxoz::testing
