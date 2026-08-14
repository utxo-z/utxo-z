// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_value_initialisation.cpp
 * @brief That a stored value is defined to its last byte.
 *
 * A `utxo_value` is stored inline in a memory-mapped file, and it is stored
 * whole: the container's size class, not the size of the output in it. A 10240
 * container holding a 200-byte output writes just over 10 KiB of something for
 * every entry, and whatever that something is goes to disk.
 *
 * It used to be whatever the writer's stack last held. Nothing read it back, so
 * nothing failed; it surfaced only when two runs of one binary, given identical
 * inputs, produced different files — the residue was full of pointers, and the
 * pointers moved with ASLR.
 *
 * Two things are checked here, because they fail separately:
 *
 *  - the object, built deliberately on contaminated storage, keeps none of it;
 *  - the bytes that actually reach the file are zero past the payload, which is
 *    the claim that matters and the one the first check only implies.
 *
 * The second covers a region no member names. `sizeof` rounds a size class up to
 * the alignment, so a 94 container occupies 96 and the two bytes it gains are
 * padding: copied, written, and reachable by nothing except the object
 * representation.
 */

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <new>
#include <span>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/database.hpp>
#include <utxoz/types.hpp>

#include "detail/durability.hpp"
#include "detail/file_cache.hpp"
#include "detail/segment_open.hpp"
#include "detail/utxo_value.hpp"

#include "support/read_file.hpp"

namespace fs = std::filesystem;
using utxoz::detail::utxo_value;

namespace {

constexpr uint8_t poison = 0xA5;

/// Build a value on storage deliberately filled with a recognisable pattern,
/// give it a payload, and hand back the whole object representation — including
/// any padding `sizeof` added — for inspection.
template <size_t Size>
std::vector<uint8_t> value_over_poison(size_t payload_size) {
    alignas(utxo_value<Size>) std::array<uint8_t, sizeof(utxo_value<Size>)> storage;
    storage.fill(poison);

    // Default-initialised on purpose: for a trivial type this leaves the poison
    // exactly where it is, which is the situation being tested. The production
    // path value-initialises instead, and set_data() has to hold either way.
    auto* const val = new (storage.data()) utxo_value<Size>;
    val->block_height = 800000;

    std::vector<uint8_t> const payload(payload_size, 0x11);
    val->set_data(payload);

    // Read back through the object's own representation, not through `storage`.
    // The array's lifetime ended when the value was constructed in it, so reading
    // it afterwards is reading an object that is no longer there — which the
    // compiler is entitled to answer from what it last knew, and did: with the
    // padding fill removed on purpose, the padding still came back as zero and
    // the case stayed green. This is the access the standard blesses.
    auto const* const raw = reinterpret_cast<unsigned char const*>(val);
    return {raw, raw + sizeof(utxo_value<Size>)};
}

/// Offset of `data` within the object. Derived rather than assumed so that a
/// change to the members is caught by the checks below instead of quietly
/// moving where "past the payload" begins.
template <size_t Size>
size_t data_offset() {
    utxo_value<Size> val{};
    return static_cast<size_t>(reinterpret_cast<uint8_t const*>(val.data.data())
                               - reinterpret_cast<uint8_t const*>(&val));
}

template <size_t Size>
void check_tail_is_defined(size_t payload_size) {
    INFO("container size " << Size << ", payload " << payload_size);
    auto const bytes = value_over_poison<Size>(payload_size);
    auto const first_unused = data_offset<Size>() + payload_size;

    REQUIRE(bytes.size() == sizeof(utxo_value<Size>));
    REQUIRE(first_unused <= bytes.size());

    // Not one byte of the pattern survives anywhere past the payload — neither in
    // the unused part of `data` nor in the padding after it.
    auto const tail = std::span(bytes).subspan(first_unused);
    CHECK(std::ranges::none_of(tail, [](uint8_t b) { return b == poison; }));
    CHECK(std::ranges::all_of(tail, [](uint8_t b) { return b == 0; }));

    // And the payload itself is intact, so this is not passing by erasing
    // everything.
    auto const written = std::span(bytes).subspan(data_offset<Size>(), payload_size);
    CHECK(std::ranges::all_of(written, [](uint8_t b) { return b == 0x11; }));
}

struct temp_db {
    temp_db() {
        static std::atomic<uint64_t> counter{0};
        auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        dir = fmt::format("./test_value_init_{}_{}_{}", getpid(), ts, counter.fetch_add(1));
        fs::remove_all(dir);
    }
    // The error_code overload: a destructor runs during unwinding too, and a
    // cleanup that threw there would replace a failing test with a terminate.
    ~temp_db() {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
    temp_db(temp_db const&) = delete;
    temp_db& operator=(temp_db const&) = delete;

    fs::path dir;
};

/// Every entry in one container file, as stored: the object representation of
/// the value, straight out of the mapping.
template <size_t Index>
std::vector<std::vector<uint8_t>> stored_values(fs::path const& dir) {
    constexpr size_t Size = utxoz::container_sizes[Index];
    auto const file = dir / fmt::format(utxoz::detail::data_file_format, Index, 0);
    std::vector<std::vector<uint8_t>> out;
    if ( ! fs::exists(file)) return out;

    auto opened = utxoz::detail::open_existing_segment(file);
    REQUIRE(opened.has_value());
    auto const found = utxoz::detail::find_single_named<utxoz::detail::utxo_map<Size>>(
        **opened, utxoz::detail::map_object_name, file);
    REQUIRE(found.has_value());

    for (auto const& entry : **found) {
        auto const* const raw = reinterpret_cast<uint8_t const*>(&entry.second);
        out.emplace_back(raw, raw + sizeof(utxo_value<Size>));
    }
    return out;
}

template <size_t Index>
void check_stored_tails(fs::path const& dir, size_t payload_size) {
    constexpr size_t Size = utxoz::container_sizes[Index];
    INFO("container " << Index << " (size class " << Size << ")");

    auto const values = stored_values<Index>(dir);
    REQUIRE_FALSE(values.empty());

    auto const first_unused = data_offset<Size>() + payload_size;
    for (auto const& bytes : values) {
        REQUIRE(bytes.size() == sizeof(utxo_value<Size>));
        auto const tail = std::span(bytes).subspan(first_unused);
        CHECK(std::ranges::all_of(tail, [](uint8_t b) { return b == 0; }));
    }
}

utxoz::raw_outpoint key_of(uint32_t n) {
    utxoz::raw_outpoint key{};
    std::memcpy(key.data(), &n, sizeof(n));
    return key;
}

/// A payload that lands in container `Index`: one byte under what the class
/// below it can hold, so it cannot be absorbed by a smaller one.
constexpr size_t payload_for(size_t index) {
    return index == 0 ? 8 : utxoz::container_sizes[index - 1];
}

} // namespace

// =============================================================================
// The object
// =============================================================================

TEST_CASE("a value built on contaminated storage keeps none of it", "[value]") {
    check_tail_is_defined<48>(8);
    check_tail_is_defined<94>(8);
    check_tail_is_defined<128>(8);
    check_tail_is_defined<256>(8);
    check_tail_is_defined<10240>(8);
}

TEST_CASE("a full payload leaves nothing undefined either", "[value]") {
    // The boundary the other case does not reach: a payload that fills `data`
    // exactly, where the only bytes left are the padding.
    // Asked of the type rather than recomputed from its layout: a duplicate of
    // that arithmetic would go on compiling after the members changed, and would
    // then be testing a boundary that is no longer the boundary.
    check_tail_is_defined<94>(utxo_value<94>{}.data.size());
    check_tail_is_defined<10240>(utxo_value<10240>{}.data.size());
}

TEST_CASE("the padding a size class gains is real, and is defined", "[value]") {
    // 94 is the one that rounds up, and it is the reason this is checked through
    // the object representation rather than through the members.
    STATIC_REQUIRE(sizeof(utxo_value<94>) == 96);
    STATIC_REQUIRE(sizeof(utxo_value<48>) == 48);

    auto const bytes = value_over_poison<94>(8);
    CHECK(bytes[94] == 0);
    CHECK(bytes[95] == 0);
}

// =============================================================================
// The file
// =============================================================================

TEST_CASE("what a stored value occupies on disk is zero past the payload", "[value]") {
    temp_db t;
    {
        auto opened = utxoz::full_db::open_for_testing(t.dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);

        uint32_t n = 0;
        for (size_t index = 0; index < utxoz::container_sizes.size(); ++index) {
            std::vector<uint8_t> const payload(payload_for(index), 0x11);
            for (int i = 0; i < 4; ++i) {
                REQUIRE(db.insert(key_of(++n), payload, 800000).has_value());
            }
        }
        db.close();
    }

    // Unrolled by hand, so a container added to the geometry would go unchecked
    // here without anything saying so.
    static_assert(utxoz::container_sizes.size() == 5,
                  "a container was added or removed; the checks below cover each one by index");
    check_stored_tails<0>(t.dir, payload_for(0));
    check_stored_tails<1>(t.dir, payload_for(1));
    check_stored_tails<2>(t.dir, payload_for(2));
    check_stored_tails<3>(t.dir, payload_for(3));
    check_stored_tails<4>(t.dir, payload_for(4));
}

TEST_CASE("two databases written the same way are byte-identical", "[value]") {
    // The property the residue used to break, stated directly.
    //
    // The identity has to be pinned first: every database draws sixteen random
    // bytes and every segment stamp carries them, so without this the files
    // differ for a reason that has nothing to do with what is being tested.
    using utxoz::detail::failpoints;
    // The central guard rather than a local one: it also clears
    // forced_database_id, and a second policy for disarming seams is how the
    // first one comes to be incomplete.
    failpoints::scoped_reset const disarm;
    failpoints::forced_database_id = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                      0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    failpoints::force_database_id.store(true, std::memory_order_relaxed);

    temp_db a, b;
    for (auto const* t : {&a, &b}) {
        auto opened = utxoz::full_db::open_for_testing(t->dir, true);
        REQUIRE(opened.has_value());
        auto db = std::move(*opened);
        uint32_t n = 0;
        for (size_t index = 0; index < utxoz::container_sizes.size(); ++index) {
            std::vector<uint8_t> const payload(payload_for(index), 0x11);
            for (int i = 0; i < 4; ++i) {
                REQUIRE(db.insert(key_of(++n), payload, 800000).has_value());
            }
        }
        db.close();
    }

    size_t compared = 0;
    for (size_t index = 0; index < utxoz::container_sizes.size(); ++index) {
        auto const name = fmt::format(utxoz::detail::data_file_format, index, 0);
        INFO(name);
        REQUIRE(fs::exists(a.dir / name));
        REQUIRE(fs::exists(b.dir / name));

        auto const ba = utxoz::testing::read_file_bytes(a.dir / name);
        auto const bb = utxoz::testing::read_file_bytes(b.dir / name);
        REQUIRE_FALSE(ba.empty());

        // Compared as a bool and reported as an offset. A failing `ba == bb`
        // would ask Catch2 to render ten megabytes of container.
        REQUIRE(ba.size() == bb.size());
        auto const [ia, ib] = std::ranges::mismatch(ba, bb);
        if (ia != ba.end()) {
            FAIL("first difference at offset " << (ia - ba.begin()));
        }
        ++compared;
    }
    CHECK(compared == utxoz::container_sizes.size());
}
