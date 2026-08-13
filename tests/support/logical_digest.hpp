// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file logical_digest.hpp
 * @brief What a database holds, said in a way no layout can change.
 *
 * Test and tooling support. Deliberately not in `src/`: nothing the library does
 * at run time needs it, and a cryptographic digest has no business on the
 * production path just because the fixtures want one.
 *
 * Two digests, because they answer different questions.
 *
 * The **global** digest is over the entries and nothing else — key, creation
 * height, and the exact payload — sorted by key so the map's internal order
 * cannot reach it. It deliberately omits which container or generation an entry
 * lives in: compaction moves entries between files without changing what the
 * database holds, and a digest that noticed would report a state change where
 * there was none.
 *
 * The **per-segment** digest is the opposite, and is what certifies a fixture:
 * it names the container and generation and covers only that file's entries. A
 * generation that went missing, or one whose contents moved, shows up here and
 * cannot hide inside a global total.
 *
 * Everything is fixed-width and little-endian. No padding, no addresses, no type
 * names, no struct written through a cast — the same discipline the persisted
 * records follow, for the same reason.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <vector>

#include <boost/hash2/sha2.hpp>

#include <utxoz/types.hpp>

namespace utxoz::testing {

/// One entry, in the only form a digest sees it.
struct logical_entry {
    raw_outpoint key{};
    uint32_t height = 0;
    std::vector<uint8_t> value;
};

namespace digest_detail {

inline void put_u8(std::vector<uint8_t>& out, uint8_t v) { out.push_back(v); }

inline void put_u32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(uint8_t((v >> (8 * i)) & 0xFF));
}

inline void put_u64(std::vector<uint8_t>& out, uint64_t v) {
    for (int i = 0; i < 8; ++i) out.push_back(uint8_t((v >> (8 * i)) & 0xFF));
}

/// key | height | length | payload. The length is explicit so two entries
/// cannot run into each other, which is what would let a byte move from the end
/// of one value to the start of the next without changing the digest.
inline void put_entry(std::vector<uint8_t>& out, logical_entry const& e) {
    out.insert(out.end(), e.key.begin(), e.key.end());
    put_u32(out, e.height);
    put_u32(out, uint32_t(e.value.size()));
    out.insert(out.end(), e.value.begin(), e.value.end());
}

[[nodiscard]]
inline std::string sha256_hex(std::span<uint8_t const> bytes) {
    boost::hash2::sha2_256 h;
    h.update(bytes.data(), bytes.size());
    auto const r = h.result();

    static constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(r.size() * 2);
    for (auto const b : r) {
        out.push_back(digits[b >> 4]);
        out.push_back(digits[b & 0x0F]);
    }
    return out;
}

} // namespace digest_detail

/// A total order over an entry, so the digest is a function of the set alone.
///
/// Sorting by key is enough only while keys are unique, which is a property of
/// the store rather than of this function — and `std::ranges::sort` is not
/// stable, so two entries sharing a key would order by whatever the algorithm
/// happened to do and the digest would differ between runs for no reason a
/// reader could see. A digest is a certificate; it has to be defined for every
/// input it can be handed, not only the well-behaved ones.
inline void sort_canonically(std::vector<logical_entry>& entries) {
    std::ranges::sort(entries, [](logical_entry const& a, logical_entry const& b) {
        if (a.key != b.key) return a.key < b.key;
        if (a.height != b.height) return a.height < b.height;
        return a.value < b.value;
    });
}

/// The whole database, independent of where anything physically sits.
///
///     "UZLG" | format u32 | mode u8 | 0 u8 | count u64 | entries, canonical order
[[nodiscard]]
inline std::string global_digest(storage_mode mode, std::vector<logical_entry> entries) {
    using namespace digest_detail;

    sort_canonically(entries);

    std::vector<uint8_t> out;
    out.insert(out.end(), {'U', 'Z', 'L', 'G'});
    put_u32(out, 1);   // digest format
    put_u8(out, uint8_t(mode));
    put_u8(out, 0);
    put_u64(out, uint64_t(entries.size()));
    for (auto const& e : entries) put_entry(out, e);

    return sha256_hex(out);
}

/// One segment, named by what it is.
///
///     "UZLS" | format u32 | kind u32 | generation u64 | count u64 | entries
[[nodiscard]]
inline std::string segment_digest(uint32_t container_kind, uint64_t generation,
                                  std::vector<logical_entry> entries) {
    using namespace digest_detail;

    sort_canonically(entries);

    std::vector<uint8_t> out;
    out.insert(out.end(), {'U', 'Z', 'L', 'S'});
    put_u32(out, 1);
    put_u32(out, container_kind);
    put_u64(out, generation);
    put_u64(out, uint64_t(entries.size()));
    for (auto const& e : entries) put_entry(out, e);

    return sha256_hex(out);
}

/// The reference-mode value, in the order the contract names it. Kept here so
/// the generator and the tests cannot disagree about what a reference entry is.
[[nodiscard]]
inline std::vector<uint8_t> reference_value_bytes(uint32_t file_number, uint32_t offset) {
    std::vector<uint8_t> v;
    digest_detail::put_u32(v, file_number);
    digest_detail::put_u32(v, offset);
    return v;
}

/// SHA-256 of a file, for the physical side of the manifest.
[[nodiscard]]
inline std::string file_digest(std::span<uint8_t const> bytes) {
    return digest_detail::sha256_hex(bytes);
}

} // namespace utxoz::testing
