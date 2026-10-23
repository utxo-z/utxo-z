// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file shard_paths.hpp
 * @brief Where a class keeps its files, for tests that look at the directory.
 *
 * Before sharding, a class had one container and one file name per generation,
 * and a test could write "cont_0_v" and be right. A sharded class has one
 * container per shard, so "the class's file" is no longer a single thing: a test
 * has to say which shard it means, and a test that follows a particular key has
 * to ask where that key went.
 *
 * These are the only names tests should build. Spelling a file name by hand
 * makes a test that passes at one shard count and silently checks nothing at
 * another — it looks for a file that cannot exist and finds it absent, which is
 * exactly what several of the assertions here are worded to detect.
 */

#ifndef UTXOZ_TESTS_SHARD_PATHS_HPP_
#define UTXOZ_TESTS_SHARD_PATHS_HPP_

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include <utxoz/types.hpp>

#include "detail/shard.hpp"

namespace utxoz::testing {

namespace fs = std::filesystem;

/// The version-file prefix of one shard of one class.
[[nodiscard]]
inline std::string class_prefix(size_t cls, uint32_t shard = 0) {
    return detail::container_prefix(detail::slot_of(cls, shard));
}

/// The metadata prefix of one shard of one class.
[[nodiscard]]
inline std::string class_meta_prefix(size_t cls, uint32_t shard = 0) {
    return detail::metadata_prefix(detail::slot_of(cls, shard));
}

/// One shard's file for one generation.
[[nodiscard]]
inline std::string class_file(size_t cls, size_t version, uint32_t shard = 0) {
    return detail::container_file_name(detail::slot_of(cls, shard), version);
}

/// Which shard of `cls` this key belongs to. A class that is not sharded answers
/// 0 for every key, so a test written against this reads the same either way.
[[nodiscard]]
inline uint32_t shard_for(size_t cls, raw_outpoint const& key) {
    return detail::shard_of(key, detail::shards_in_class(cls));
}

/// The file the class would store this key's generation in.
[[nodiscard]]
inline std::string file_for_key(size_t cls, raw_outpoint const& key, size_t version) {
    return class_file(cls, version, shard_for(cls, key));
}

/// How many shards the class is split into; 1 for every class but the sharded
/// one, and 1 for that one too in an unsharded build.
[[nodiscard]]
inline uint32_t shards_of(size_t cls) {
    return detail::shards_in_class(cls);
}

/// Every version file the class has, across all of its shards. A test that used
/// to count one prefix wants this instead: the class's generations did not go
/// anywhere, they are spread over its containers.
[[nodiscard]]
inline size_t count_class_files(fs::path const& dir, size_t cls, char const* suffix = ".dat") {
    // The prefixes are built once and the directory read once, for the same
    // reason generations_per_shard() does: this sits inside fill loops.
    std::vector<std::string> prefixes;
    for (uint32_t sh = 0; sh != shards_of(cls); ++sh) prefixes.push_back(class_prefix(cls, sh));

    size_t found = 0;
    std::error_code ec;
    for (auto const& entry : fs::directory_iterator(dir, ec)) {
        auto const name = entry.path().filename().string();
        if ( ! name.ends_with(suffix)) continue;
        for (auto const& prefix : prefixes) {
            if (name.starts_with(prefix)) { ++found; break; }
        }
    }
    return found;
}

/// How many generations each shard of a class has, in one pass over the
/// directory.
///
/// One pass on purpose. These are called from inside insert loops that run to
/// hundreds of thousands of iterations, and a version that scanned once per
/// shard turned a suite that took eleven seconds into one that took an hour —
/// the work is in the directory, not in the inserts.
[[nodiscard]]
inline std::vector<size_t> generations_per_shard(fs::path const& dir, size_t cls) {
    std::vector<std::string> prefixes;
    for (uint32_t sh = 0; sh != shards_of(cls); ++sh) prefixes.push_back(class_prefix(cls, sh));

    std::vector<size_t> counts(prefixes.size(), 0);
    std::error_code ec;
    for (auto const& entry : fs::directory_iterator(dir, ec)) {
        auto const name = entry.path().filename().string();
        if ( ! name.ends_with(".dat")) continue;
        for (size_t i = 0; i != prefixes.size(); ++i) {
            if (name.starts_with(prefixes[i])) { ++counts[i]; break; }
        }
    }
    return counts;
}

/// How many generations one shard of a class has on disk.
[[nodiscard]]
inline size_t generations_in_shard(fs::path const& dir, size_t cls, uint32_t shard) {
    return generations_per_shard(dir, cls).at(shard);
}

/// The fewest generations any shard of the class has.
///
/// This is what a test means by "the class has reached N generations": with one
/// shard it is that shard's count, and with several it is the point at which
/// every shard has a history that deep. Taking the total instead would be
/// reached eight times sooner and prove nothing about any one container, and
/// taking shard zero's would leave the others behind.
///
/// It costs no more inserts than it did unsharded. The class's total capacity is
/// unchanged, so filling every shard to its threshold once takes the same number
/// of entries as filling the single container did — they are simply spread.
[[nodiscard]]
inline size_t generations_reached(fs::path const& dir, size_t cls) {
    auto const counts = generations_per_shard(dir, cls);
    if (counts.empty()) return 0;
    return *std::ranges::min_element(counts);
}

/// A distinct outpoint whose bytes all vary with `n`, the way a real txid's do.
///
/// The obvious generator — memcpy the counter over the first eight bytes and
/// leave the rest zero — produces keys that are distinct but identical in bytes
/// 24..35, which are the only ones shard_of() reads. Every such key lands in
/// shard zero. A test that fills until every shard has rotated then never
/// finishes, and one that merely fills is quietly exercising a single container
/// while believing it exercised the class.
///
/// This is not a workaround for the shard function; it is what a txid looks
/// like. Bitcoin outpoints carry a hash, and a hash has no low-entropy tail.
[[nodiscard]]
inline raw_outpoint spread_key(uint64_t n) {
    raw_outpoint key{};
    uint64_t x = n + 0x9E3779B97F4A7C15ULL;
    for (size_t i = 0; i + 8 <= key.size(); i += 8) {
        uint64_t z = (x += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        z ^= z >> 31;
        for (size_t b = 0; b != 8; ++b) key[i + b] = uint8_t(z >> (8 * b));
    }
    // The counter itself in the first bytes, so two calls can never collide even
    // if the mixer did: distinctness is what every caller relies on.
    for (size_t b = 0; b != 8; ++b) key[b] = uint8_t(n >> (8 * b));
    return key;
}

/// The `n`-th distinct key that belongs to a given shard of a class.
///
/// A test that means to build one container's history needs every key to land in
/// that container. Spreading them is right for a test about the class; stacking
/// them is right for a test about a container, and before sharding the two were
/// the same sentence.
///
/// Found by trying, not constructed: shard_of() is the authority on where a key
/// goes, and a key built to satisfy some inverse of it would stop agreeing the
/// moment either changed.
///
/// The stride has to be generous, and the reason is worth stating because a
/// smaller one already went wrong here. Each draw lands in the wanted shard with
/// probability 1/N, so a search of S candidates misses entirely with probability
/// (1 − 1/N)^S. At N = 8 and S = 64 that is 1.9·10⁻⁴ — which sounds negligible
/// and is not: a caller filling a container draws this ten thousand times or
/// more, so it happens twice or three times per run, and each time the helper
/// hands back a key from another shard. The caller then believes it filled one
/// container and quietly filled two, which is a wrong test rather than a failing
/// one.
///
/// With S = 4096 the same figure is (31/32)^4096 ≈ 10⁻⁵⁷ at the largest shard
/// count this build admits, which is small enough that the fallback below is
/// unreachable in any run that will ever exist.
[[nodiscard]]
inline raw_outpoint key_in_shard(size_t cls, uint32_t shard, uint64_t n) {
    auto const shards = shards_of(cls);
    // The stride also keeps two different `n` from ever landing on the same key:
    // the candidate ranges are disjoint, and spread_key() carries the candidate
    // in the first eight bytes.
    constexpr uint64_t stride = 4096;
    for (uint64_t candidate = n * stride; candidate != n * stride + stride; ++candidate) {
        auto const key = spread_key(candidate);
        if (detail::shard_of(key, shards) == shard) return key;
    }
    // Unreachable, per the bound above. Returning a key rather than throwing
    // keeps this usable anywhere; a caller that somehow got here would see its
    // own assertion about where the key went fail, which is the right place for
    // it to be noticed.
    return spread_key(n * stride);
}

/// Every metadata file the class has, across its shards. The counterpart of
/// count_class_files(), and it has to agree with it: a metadata file describes
/// one version of one container.
[[nodiscard]]
inline size_t count_class_metadata(fs::path const& dir, size_t cls) {
    std::vector<std::string> prefixes;
    for (uint32_t sh = 0; sh != shards_of(cls); ++sh) prefixes.push_back(class_meta_prefix(cls, sh));

    size_t found = 0;
    std::error_code ec;
    for (auto const& entry : fs::directory_iterator(dir, ec)) {
        auto const name = entry.path().filename().string();
        if ( ! name.ends_with(".dat")) continue;
        for (auto const& prefix : prefixes) {
            if (name.starts_with(prefix)) { ++found; break; }
        }
    }
    return found;
}

} // namespace utxoz::testing

/**
 * @brief Skip a test that only means something for the production geometry.
 *
 * The compatibility fixtures are written by a production build: geometry 4,
 * eight shards. An experimental build (UTXOZ_EXPERIMENTAL_SHARD_COUNT) is a
 * different geometry with a different id, and it is *supposed* to refuse those
 * files - that refusal is the barrier doing its job, and asserting it test by
 * test would turn the fixture tests inside out to restate one property.
 *
 * A production build never skips here: it reads the fixtures or the tests fail.
 */
#define UTXOZ_SKIP_UNLESS_SHIPPED_GEOMETRY()                                        \
    do {                                                                            \
        if (utxoz::experimental_build) {                                            \
            SKIP("the compatibility fixtures certify geometry 4, the production "   \
                 "one; this experimental build writes another geometry and must "   \
                 "refuse them.");                                                   \
        }                                                                           \
    } while (0)

#endif // UTXOZ_TESTS_SHARD_PATHS_HPP_
