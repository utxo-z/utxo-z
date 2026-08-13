// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file format_identity.hpp
 * @brief What this build can read, expressed as four numbers it compares.
 * @internal
 *
 * The store keeps its maps inside memory-mapped files, so the internal layout of
 * `boost::unordered_flat_map` *is* the file format. Boost promises nothing about
 * that layout, and Boost.Interprocess resolves a named object by name alone —
 * `priv_generic_find<T>` checks nothing about the type in a release build and
 * hands back a `static_cast<T*>`. A build whose idea of the layout differs from
 * the one that wrote the file would therefore reinterpret it rather than refuse
 * it, and every symptom of that would appear somewhere else.
 *
 * These four identities are what stands in the way. They are separate because
 * they fail for different reasons and send an operator to different places:
 *
 * - geometry_id      — our own sizing decisions changed.
 * - map_layout_epoch — the Boost implementation we certified changed.
 * - hash_epoch       — the effective hash changed, so every key moved.
 * - platform_abi_id  — this file was written on an incompatible platform.
 *
 * None of them is derived from a type name, a mangled symbol or `std::hash`:
 * all three vary between compilers and versions without meaning anything about
 * compatibility. They are constants, bumped deliberately, each pinned by an
 * assertion that fails to compile if the thing it stands for moves underneath
 * it.
 *
 * `BOOST_VERSION` is deliberately absent from all of this. Several Boost
 * versions can share one `map_layout_epoch`, and treating a version bump as an
 * incompatibility would invalidate every database for no reason. It is recorded
 * in the config for diagnosis and never compared.
 */

#pragma once

#include <array>
#include <bit>
#include <cstdint>

#include <boost/container_hash/hash.hpp>
#include <boost/interprocess/offset_ptr.hpp>

#include <utxoz/types.hpp>

#include "utxo_value.hpp"

namespace utxoz::detail {

/**
 * @brief The platform facts that decide whether these bytes mean the same thing.
 *
 * Composed from properties that are measurable and stable, not from a compiler
 * name or a mangled type. Each occupies one byte, which every one of them fits
 * in comfortably, and the packing is fixed so the number is reproducible.
 *
 * Compiler and standard library are deliberately **not** here. Nothing shows
 * they change what gets written, and folding them in would invalidate every
 * database on a toolchain upgrade for no demonstrated reason. If a compiler ever
 * does change the layout, it changes one of the sizes below or it is caught by
 * the compatibility fixtures — which is exactly the job the fixtures exist for.
 */
inline constexpr uint32_t platform_abi_id = []() constexpr {
    uint32_t const endian = std::endian::native == std::endian::little ? 1u : 2u;
    return (endian << 24)
         | (uint32_t(sizeof(size_t)) << 16)
         | (uint32_t(sizeof(void*)) << 8)
         | uint32_t(sizeof(boost::interprocess::offset_ptr<void>));
}();

static_assert(sizeof(size_t) <= 255 && sizeof(void*) <= 255
              && sizeof(boost::interprocess::offset_ptr<void>) <= 255,
              "platform_abi_id packs each of these into one byte");

/**
 * @brief The container sizing this build writes.
 *
 * Bumped whenever `container_sizes` changes, whenever a container is added or
 * removed, or whenever a value moves from one container to another. The
 * assertion below is what makes forgetting impossible: change the geometry
 * without changing this number and the build stops.
 */
inline constexpr uint32_t geometry_id = 1;

static_assert(container_sizes == std::array<size_t, 5>{48, 94, 128, 256, 10240},
              "the container geometry changed; bump geometry_id and update this assertion, "
              "because existing databases were written under the old one");

/**
 * @brief The `boost::unordered_flat_map` layout this build is certified against.
 *
 * Epoch 1 is the implementation shipped in Boost 1.91: `group15<plain_integral>`
 * groups of fifteen, `plain_size_control`, `pow2_size_policy`, elements inline in
 * one array reached through `offset_ptr`.
 *
 * A new Boost release does **not** bump this on its own. It is bumped when the
 * compatibility fixtures show that what the new implementation writes or reads
 * is not what epoch 1 wrote — which is why the fixtures exist, and why this
 * number is ours rather than Boost's.
 *
 * The sizes below are not the certification; the fixtures are. They are a cheap
 * tripwire for the most obvious kind of change, and they cost nothing.
 */
inline constexpr uint32_t map_layout_epoch = 1;

/// The ABI epoch 1's sizes were measured on: little-endian, 64-bit `size_t`,
/// 64-bit pointers, 64-bit `offset_ptr`. Spelled out rather than derived so the
/// tripwire below cannot silently start describing a different platform.
inline constexpr uint32_t lp64_le_abi = 0x01080808u;

/// The other ABI this project builds for: wasm32, little-endian, 32-bit
/// `size_t`, pointers and `offset_ptr`. Named because the hash vectors are
/// pinned per ABI and a number in a test is not a reason.
inline constexpr uint32_t wasm32_le_abi = 0x01040404u;

static_assert(platform_abi_id != lp64_le_abi
                  || (sizeof(utxo_map<48>) == 56 && alignof(utxo_map<48>) == 8),
              "on the ABI epoch 1 was measured on, the map object changed size or "
              "alignment; check the compatibility fixtures before deciding whether to "
              "bump map_layout_epoch");

/**
 * @brief The effective hash whose output is baked into every stored file.
 *
 * Epoch 1 is `mulx_mix(hash_outpoint(key))`.
 *
 * The mixing is not ours and is not optional: Boost 1.91 picks it in
 * `table_core` with
 *
 *     using mix_policy = conditional_t<hash_is_avalanching<Hash>::value,
 *                                      no_mix, mulx_mix>;
 *
 * and `outpoint_hash` does not declare the trait, so `mulx_mix` applies. Marking
 * it avalanching would look like an optimisation — the hash does come from a
 * SHA-256, so it *is* well distributed — and would relocate every key in every
 * existing database while leaving `sizeof`, `alignof`, the bucket ladder and the
 * load factor all exactly as they were. Nothing structural would notice.
 *
 * So the trait is pinned here. Changing it means bumping `hash_epoch`, and every
 * database written under the old one has to be rebuilt.
 */
inline constexpr uint32_t hash_epoch = 1;

static_assert( ! boost::hash_is_avalanching<outpoint_hash>::value,
              "outpoint_hash must not declare is_avalanching: Boost would stop applying "
              "mulx_mix and every key in every existing database would move. If this is "
              "deliberate, bump hash_epoch and require a rebuild");

/// A database's identity: sixteen random bytes, made once when it is created and
/// written into its config and into every segment it owns. Two databases never
/// share one, so a version file that wandered in from somewhere else is caught
/// before anything reads it.
using database_id_t = std::array<uint8_t, 16>;

/// The container a segment belongs to. Full mode uses the container index;
/// reference mode has one container and uses this instead. Fixed width, because
/// it is written to disk — `reference_sentinel_index` is `SIZE_MAX`, which is
/// not the same number on every platform.
inline constexpr uint32_t reference_container_kind = 0xFFFFFFFFu;

} // namespace utxoz::detail
