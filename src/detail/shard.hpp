// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UTXOZ_DETAIL_SHARD_HPP_
#define UTXOZ_DETAIL_SHARD_HPP_

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#include <utxoz/types.hpp>

namespace utxoz::detail {

/**
 * @brief Which shard of the 48-byte class a key belongs to.
 *
 * PART OF THE PERSISTED FORMAT. A key's shard decides which file holds it, so
 * changing this function relocates every entry and requires a new geometry_id
 * and a rebuilt database. It is specified byte by byte for that reason.
 *
 * ## Domain
 *
 * The 36 stored bytes of an outpoint:
 *
 *     bytes  0..31   the txid in INTERNAL byte order — the reverse of the hex a
 *                    block explorer displays. Byte 0 is `raw_outpoint[0]`.
 *     bytes 32..35   the output index, read as a little-endian uint32 BYTE BY
 *                    BYTE, never by memcpy and never by reinterpretation.
 *
 * ## Why bytes 24..31 and not the first eight
 *
 * `hash_outpoint` builds the map's hash seed from the FIRST eight bytes of the
 * txid, and Boost mixes that with `mulx_mix` to pick a bucket. Taking the shard
 * from the same quantity would tie shard to bucket: every key in a shard would
 * share bits of what selects its slot, and the map's occupancy would stop being
 * uniform.
 *
 * Bytes 24..31 are disjoint from that seed. Under the pseudorandom model of
 * SHA-256 disjoint byte ranges of a digest are independent — but that is an
 * expectation, not a proof, and it was **measured**: over the 43,802,458 live
 * class-48 keys of a full mainnet IBD, the joint chi-square of shard against the
 * map hash is consistent with independence at every N, and the per-shard
 * distribution has a maximum deviation of 0.0973% at N=8 (chi-square 12.90 with
 * 7 degrees of freedom). See doc/sharding-class-48-design.md.
 *
 * ## Why the output index is mixed in
 *
 * A function of the txid alone puts every output of one transaction in the same
 * shard. That is not hypothetical: in the same measurement one transaction
 * contributed 31,243 live entries, and a txid-only function inflated the
 * chi-square by a factor of 76 to 238. The index is what breaks the clustering.
 *
 * ## Stability
 *
 * Every read is byte-wise with explicit weights, so the result does not depend
 * on endianness, on `sizeof(size_t)`, on the compiler, or on Boost. All
 * arithmetic is on `uint64_t` with defined wrapping.
 *
 * Known fixed point: an all-zero outpoint yields shard 0 for every N. That is
 * the null outpoint, which is not in any live set; it is documented so nobody
 * reads it as a defect.
 */

/// The first byte of the range mixed into the shard.
inline constexpr size_t shard_mix_offset = 24;

/// splitmix64's constants, and the golden-ratio multiplier for the index. None
/// of them is ours to choose again: they are part of the format.
inline constexpr uint64_t shard_index_multiplier = 0x9E3779B97F4A7C15ULL;
inline constexpr uint64_t shard_mix_a            = 0xBF58476D1CE4E5B9ULL;
inline constexpr uint64_t shard_mix_b            = 0x94D049BB133111EBULL;

static_assert(outpoint_size == 36,
              "shard_of reads bytes 24..31 and 32..35 of a 36-byte outpoint; a "
              "different outpoint size means a different function and a new "
              "geometry_id");

/// Whether `n` may be used as a shard count: a power of two, at least one, at
/// most 32. Not a tuning range — every value is a different persisted format.
[[nodiscard]]
inline constexpr bool valid_shard_count(uint32_t n) noexcept {
    return n != 0 && (n & (n - 1)) == 0 && n <= 32;
}

/// Eight bytes at `offset`, little-endian, read one byte at a time.
[[nodiscard]]
inline constexpr uint64_t le64_at(raw_outpoint const& key, size_t offset) noexcept {
    return uint64_t(key[offset])
         | (uint64_t(key[offset + 1]) << 8)
         | (uint64_t(key[offset + 2]) << 16)
         | (uint64_t(key[offset + 3]) << 24)
         | (uint64_t(key[offset + 4]) << 32)
         | (uint64_t(key[offset + 5]) << 40)
         | (uint64_t(key[offset + 6]) << 48)
         | (uint64_t(key[offset + 7]) << 56);
}

/// Four bytes at `offset`, little-endian, read one byte at a time.
[[nodiscard]]
inline constexpr uint64_t le32_at(raw_outpoint const& key, size_t offset) noexcept {
    return uint64_t(key[offset])
         | (uint64_t(key[offset + 1]) << 8)
         | (uint64_t(key[offset + 2]) << 16)
         | (uint64_t(key[offset + 3]) << 24);
}

/// The shard of `key` among `n`. See the contract above.
[[nodiscard]]
inline constexpr uint32_t shard_of(raw_outpoint const& key, uint32_t n) noexcept {
    uint64_t h = le64_at(key, shard_mix_offset)
               ^ (le32_at(key, 32) * shard_index_multiplier);
    h ^= h >> 30;
    h *= shard_mix_a;
    h ^= h >> 27;
    h *= shard_mix_b;
    h ^= h >> 31;
    return uint32_t(h) & (n - 1);
}


// =============================================================================
// Slots: how a sharded class maps onto the per-container state
// =============================================================================
//
// The CLASS index stays a compile-time parameter, because it selects the map
// type: every shard of the 48-byte class is a `utxo_map<48>` and shares one
// instantiation. What the shard selects is a SLOT — which segment, which
// catalog, which active version — and that is a run-time index.
//
// Slots are laid out with the sharded class first, so that a slot below
// `shard_count` is a shard of it and everything above is one of the other
// classes, one slot each:
//
//     slot 0 .. shard_count-1     shards of the 48-byte class
//     slot shard_count            the 96-byte class
//     slot shard_count+1          the 128-byte class
//     ...
//
// With shard_count == 1 the layout is the identity: slot i is class i, and every
// figure below equals what it was before sharding existed. That is what makes it
// possible to land the structure without changing behaviour.

/// Which class is split. Only one is, and it is the one that does all the work.
inline constexpr size_t sharded_class = 0;

/// How many slots the whole geometry needs.
inline constexpr size_t slot_count = shard_count + container_count - 1;

static_assert(sharded_class == 0,
              "the slot layout puts the sharded class first; moving it means "
              "rewriting slot_of and class_of_slot together");

/// The slot that holds `shard` of `cls`. `shard` is ignored for classes that are
/// not sharded, and must be zero for them.
[[nodiscard]]
inline constexpr size_t slot_of(size_t cls, uint32_t shard) noexcept {
    return cls == sharded_class ? size_t(shard) : shard_count + cls - 1;
}

/// The class a slot belongs to.
[[nodiscard]]
inline constexpr size_t class_of_slot(size_t slot) noexcept {
    return slot < shard_count ? sharded_class : slot - shard_count + 1;
}

/// The shard a slot holds. Always zero for the classes that are not sharded.
[[nodiscard]]
inline constexpr uint32_t shard_of_slot(size_t slot) noexcept {
    return slot < shard_count ? uint32_t(slot) : 0u;
}

/// How many shards a class is split into: `shard_count` for the sharded one, one
/// for every other.
[[nodiscard]]
inline constexpr uint32_t shards_in_class(size_t cls) noexcept {
    return cls == sharded_class ? shard_count : 1u;
}

/// The slot a key belongs to, given its class. The one place that decides.
[[nodiscard]]
inline constexpr size_t slot_for_key(size_t cls, raw_outpoint const& key) noexcept {
    return cls == sharded_class ? size_t(shard_of(key, shard_count))
                                : shard_count + cls - 1;
}

/// Whether a file of `slot` can hold a key whose own shard is `key_shard`.
///
/// What the historical walks ask, and the only thing routing rules out. A lookup
/// or a deletion carries a key but not its size class, so every class stays a
/// candidate; inside the sharded class, though, the key's hash names exactly one
/// shard, and every other shard's files are work with no possible result.
[[nodiscard]]
inline constexpr bool slot_may_hold(size_t slot, uint32_t key_shard) noexcept {
    return slot >= shard_count || uint32_t(slot) == key_shard;
}

static_assert([] {
    for (size_t s = 0; s != slot_count; ++s) {
        for (uint32_t k = 0; k != shard_count; ++k) {
            bool const want = class_of_slot(s) != sharded_class || shard_of_slot(s) == k;
            if (slot_may_hold(s, k) != want) return false;
        }
    }
    return true;
}(), "slot_may_hold must rule out exactly the other shards of the sharded class");

// The layout has to be a bijection, and these check it for every slot the build
// can have rather than for the one the developer happened to think of.
static_assert([] {
    for (size_t s = 0; s != slot_count; ++s) {
        if (slot_of(class_of_slot(s), shard_of_slot(s)) != s) return false;
    }
    for (size_t c = 0; c != container_count; ++c) {
        for (uint32_t sh = 0; sh != shards_in_class(c); ++sh) {
            if (class_of_slot(slot_of(c, sh)) != c) return false;
            if (shard_of_slot(slot_of(c, sh)) != sh) return false;
        }
    }
    return true;
}(), "the slot layout is not a bijection between slots and (class, shard)");

static_assert(valid_shard_count(shard_count),
              "shard_count must be a power of two in [1, 32]");


// =============================================================================
// File names
// =============================================================================
//
// With shard_count == 1 the names are EXACTLY what they were before sharding
// existed: `cont_0_v00013.dat`, `meta_0_v00013.dat`. That is not a nicety — it
// is what lets the structure land without rewriting a single byte of an existing
// database, and it is checked by a test.
//
// With shard_count > 1 the sharded class gains an `s{shard}` between the class
// and the version:
//
//     cont_0s3_v00013.dat        meta_0s3_v00013.dat
//
// The character right after the class index is what tells them apart: `_` in the
// unsharded form, `s` in the sharded one. A parser anchored at that position
// cannot read one as the other, and it must REFUSE rather than ignore anything
// that fits neither. Above that sits the real defence: a database written under
// a different shard count carries a different geometry identity and is refused
// before the enumerator sees a single name.

/// The prefix a slot's container files share, up to and including the `_v`.
[[nodiscard]]
inline std::string container_prefix(size_t slot) {
    auto const cls = class_of_slot(slot);
    if (shards_in_class(cls) == 1) {
        return "cont_" + std::to_string(cls) + "_v";
    }
    return "cont_" + std::to_string(cls) + "s" + std::to_string(shard_of_slot(slot)) + "_v";
}

/// The same for the metadata sidecar of a slot.
[[nodiscard]]
inline std::string metadata_prefix(size_t slot) {
    auto const cls = class_of_slot(slot);
    if (shards_in_class(cls) == 1) {
        return "meta_" + std::to_string(cls) + "_v";
    }
    return "meta_" + std::to_string(cls) + "s" + std::to_string(shard_of_slot(slot)) + "_v";
}

/// The full container file name for a slot and version.
[[nodiscard]]
inline std::string container_file_name(size_t slot, size_t version) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%05zu", version);
    return container_prefix(slot) + buf + ".dat";
}

/// The full metadata file name for a slot and version.
[[nodiscard]]
inline std::string metadata_file_name(size_t slot, size_t version) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%05zu", version);
    return metadata_prefix(slot) + buf + ".dat";
}

/// The full sidecar file name for a slot and version. A sidecar marks a merge
/// that started and has not been finished, so it shares the container's prefix:
/// recovery finds it by listing that prefix, and a sidecar belonging to another
/// shard sorts under another prefix and is left to that shard.
[[nodiscard]]
inline std::string sidecar_file_name(size_t slot, size_t version) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%05zu", version);
    return container_prefix(slot) + buf + ".merge";
}

} // namespace utxoz::detail

#endif // UTXOZ_DETAIL_SHARD_HPP_
