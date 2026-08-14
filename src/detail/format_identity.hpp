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
#include <initializer_list>

#include <boost/container_hash/hash.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

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
inline constexpr uint32_t data_abi_id = []() constexpr {
    uint32_t const endian = std::endian::native == std::endian::little ? 1u : 2u;
    return (endian << 24)
         | (uint32_t(sizeof(size_t)) << 16)
         | (uint32_t(sizeof(void*)) << 8)
         | uint32_t(sizeof(boost::interprocess::offset_ptr<void>));
}();

static_assert(sizeof(size_t) <= 255 && sizeof(void*) <= 255
              && sizeof(boost::interprocess::offset_ptr<void>) <= 255,
              "data_abi_id packs each of these into one byte");

/**
 * @brief Which interprocess mutex this build compiles, taken from Boost itself.
 *
 * Read from the macros `interprocess_mutex.hpp` leaves behind rather than by
 * re-deriving the predicate it switches on. The two agree today; a Boost that
 * changed the condition would leave a copy of it here quietly describing the
 * wrong thing, which is the failure this identity exists to prevent.
 */
inline constexpr uint32_t sync_abi_family =
#if defined(BOOST_INTERPROCESS_MUTEX_USE_POSIX)
    1u;   // pthread process-shared mutexes
#elif defined(BOOST_INTERPROCESS_MUTEX_USE_WINAPI)
    2u;   // the Windows API
#else
    3u;   // Boost's spinlock emulation
#endif

/// Which platform and C runtime this is.
///
/// The family is not enough on its own. Two targets can both take the POSIX
/// branch and still disagree about what a `pthread_mutex_t` looks like — glibc
/// and bionic do — so the same branch is not the same persisted layout, and only
/// the layout matters to a file.
///
/// There is deliberately no catch-all. A shared fallback would make two
/// unrecognised platforms compatible *with each other*, which is the same defect
/// as before wearing a different number: they would agree on a tag neither of
/// them earned, and on every other ingredient, and the barrier would let one open
/// the other's files. Grouping "Linux with some libc that is not glibc" had the
/// same problem one level down, since musl and bionic are exactly what the tag
/// exists to tell apart.
///
/// So an unrecognised target does not compile. A build that knows what it is can
/// say so with `UTXOZ_PLATFORM_TAG`, and is then responsible for the claim that
/// the number is unique to that platform and runtime — which is why explicit tags
/// live above every value this header will ever assign itself.
inline constexpr uint32_t explicit_platform_tag_floor = 1000;

#if defined(UTXOZ_PLATFORM_TAG)
inline constexpr uint32_t platform_tag = UTXOZ_PLATFORM_TAG;
static_assert(platform_tag >= explicit_platform_tag_floor,
              "UTXOZ_PLATFORM_TAG must be at least 1000: below that is reserved for the "
              "platforms this header recognises, and a collision with one of them would "
              "certify a compatibility nobody established");
#elif defined(_WIN32)
inline constexpr uint32_t platform_tag = 1;
#elif defined(__EMSCRIPTEN__)
inline constexpr uint32_t platform_tag = 2;
#elif defined(__APPLE__)
inline constexpr uint32_t platform_tag = 3;
#elif defined(__ANDROID__)
inline constexpr uint32_t platform_tag = 5;   // bionic, checked before glibc: it defines neither
#elif defined(__linux__) && defined(__GLIBC__)
inline constexpr uint32_t platform_tag = 4;
#elif defined(__FreeBSD__)
inline constexpr uint32_t platform_tag = 6;
#else
#error "utxoz: this platform and C runtime are not recognised, so nothing is known about \
the layout of the mutex a segment persists. Define UTXOZ_PLATFORM_TAG to a value of 1000 or \
more that is unique to this platform and runtime, or add it to format_identity.hpp. A shared \
fallback would make two unrecognised targets compatible with each other, which is the defect \
this identity exists to prevent."
#endif

/// Zero is not a tag. It cannot be reached through the branches above — an
/// unrecognised target does not compile at all — but an explicit tag arrives from
/// outside this file, and "unset" is the value a mistake there most often takes.
static_assert(platform_tag != 0,
              "platform_tag must identify something; zero is what an unset build "
              "definition looks like");


/// FNV-1a over a list of 32-bit ingredients. A fold rather than a packing,
/// because what has to go into the identity below does not fit a byte each.
inline constexpr uint32_t mix_identity(std::initializer_list<uint32_t> parts) {
    uint32_t h = 0x811c9dc5u;
    for (uint32_t const part : parts) {
        for (int byte = 0; byte < 4; ++byte) {
            h ^= (part >> (byte * 8)) & 0xFFu;
            h *= 0x01000193u;
        }
    }
    return h;
}

/// Bumped if the ingredient list changes, so an old id and a new one cannot
/// collide into looking compatible.
inline constexpr uint32_t platform_abi_encoding = 1;

/**
 * @brief The whole persisted ABI: the data, and the segment's own machinery.
 *
 * A managed segment keeps its allocator's mutex — and the segment manager that
 * owns it — **inside the mapped file**. Boost picks a different mutex per
 * platform: POSIX process-shared mutexes where they work, the Windows API where
 * that is the choice, a spinlock emulation otherwise. macOS takes the third road
 * because Boost marks Apple's process-shared mutexes as broken, in
 * `detail/workaround.hpp`, with the reason written out. Different types,
 * different sizes, at one offset, and everything after them moves.
 *
 * Linux x86_64 and macOS arm64 agree on every number in `data_abi_id`. So the
 * barrier accepted a file it had to refuse, the segment mapped cleanly, and the
 * first named-object lookup never returned — a lock taken on another platform's
 * bytes. No diagnostic, no failure, no end.
 *
 * The family alone would not have been enough either. What goes in is the
 * measurable shape of the persisted machinery — the size and alignment of the
 * mutex and of the segment manager — together with the platform and C runtime,
 * so that two POSIX targets are not taken for each other merely because their
 * mutexes happen to be the same size.
 *
 * Parameterised so there is exactly one ingredient list. A test asking what a
 * differently-built target's identity would be, and the constant this build uses,
 * are then the same arithmetic rather than two copies of it that can drift.
 */
inline constexpr uint32_t compute_platform_abi_id(uint32_t family, uint32_t tag) {
    return mix_identity({
        platform_abi_encoding,
        data_abi_id,
        family,
        uint32_t(sizeof(boost::interprocess::interprocess_mutex)),
        uint32_t(alignof(boost::interprocess::interprocess_mutex)),
        uint32_t(sizeof(segment_manager_t)),
        uint32_t(alignof(segment_manager_t)),
        tag,
    });
}

inline constexpr uint32_t platform_abi_id =
    compute_platform_abi_id(sync_abi_family, platform_tag);

static_assert(sync_abi_family >= 1 && sync_abi_family <= 3);
static_assert(compute_platform_abi_id(1, platform_tag) != compute_platform_abi_id(3, platform_tag),
              "the mutex family has to change the identity, or the barrier cannot tell "
              "a POSIX-built file from a spinlock-built one");
static_assert(compute_platform_abi_id(sync_abi_family, 1) != compute_platform_abi_id(sync_abi_family, 2),
              "so does the platform: two targets on the same branch can still disagree "
              "about what a mutex looks like");
static_assert(sizeof(segment_manager_t) > 0,
              "the segment manager is the header this format is written around; "
              "its shape belongs in the identity");

/**
 * @brief The container sizing this build writes.
 *
 * Bumped whenever `container_sizes` changes, whenever a container is added or
 * removed, or whenever a value moves from one container to another. The
 * assertion below is what makes forgetting impossible: change the geometry
 * without changing this number and the build stops.
 */
/// 2: the second class became 96 rather than 94. The object already occupied 96
/// — 94 rounds up — so the two bytes it gained were padding nobody could use.
/// Naming the class 96 turns them into payload and costs nothing, but it moves
/// where a value lives: a 90- or 91-byte output used to go to container 2 and now
/// fits in container 1. Databases written under geometry 1 are refused.
inline constexpr uint32_t geometry_id = 2;

static_assert(container_sizes == std::array<size_t, 5>{48, 96, 128, 256, 10240},
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
///
/// A *data* ABI, deliberately: what a key hashes to and how wide a stored field
/// is do not depend on which mutex the segment carries, so the vectors pinned
/// against this stay pinned on every platform that shares these four numbers.
inline constexpr uint32_t lp64_le_abi = 0x01080808u;

/// The other ABI this project builds for: wasm32, little-endian, 32-bit
/// `size_t`, pointers and `offset_ptr`. Named because the hash vectors are
/// pinned per ABI and a number in a test is not a reason.
inline constexpr uint32_t wasm32_le_abi = 0x01040404u;

static_assert(data_abi_id != lp64_le_abi
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
