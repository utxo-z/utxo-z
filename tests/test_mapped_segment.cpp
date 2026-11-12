// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_mapped_segment.cpp
 * @brief What a version file is, below everything the store puts in it.
 *
 * Every other suite reaches a segment through a database, where a stamp and a
 * map stand between the test and the file. What is pinned here is the layer
 * under both: that a file created is the file opened, that an object written
 * into a slot is found there again, and that a file which is not a segment —
 * or whose header describes something other than the bytes that follow it — is
 * refused when it is opened rather than believed.
 *
 * The refusals matter more than the round trips. A header is read where it
 * lies, and every offset in it is added to the address of the mapping: one
 * that pointed past the end would not fail, it would read whatever is mapped
 * next. So each field a reader goes on to trust has a case that damages that
 * field and nothing else.
 */

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/types.hpp>

#include "detail/mapped_segment.hpp"
#include "detail/segment_open.hpp"

#include "support/read_file.hpp"

namespace fs = std::filesystem;

using utxoz::detail::mapped_segment;
using utxoz::detail::offset_ptr;
using utxoz::detail::segment_full;
using utxoz::detail::segment_header;
using utxoz::detail::segment_slot;

namespace {

std::atomic<uint64_t> segment_test_counter{0};

/// A file name nothing else is using, removed on the way out.
///
/// Declared before any segment mapped from it, so that the mapping goes first:
/// a file still mapped cannot be removed on Windows, and the removal here is
/// the overload that would not say so.
struct scratch_file {
    fs::path path;

    explicit scratch_file(std::string_view tag) {
        auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto const id = segment_test_counter.fetch_add(1);
        path = fs::path(fmt::format("./test_segment_{}_{}_{}_{}.dat", tag, getpid(), ts, id));
        std::error_code ec;
        fs::remove(path, ec);
    }

    scratch_file(scratch_file const&) = delete;
    scratch_file& operator=(scratch_file const&) = delete;

    ~scratch_file() {
        std::error_code ec;
        fs::remove(path, ec);
    }
};

/// Small enough to be cheap, large enough that no case here fills it by accident.
constexpr size_t segment_bytes = 64u * 1024u;

/// A slot the header has room for and the store assigns to nothing.
constexpr auto spare_slot = segment_slot(utxoz::detail::segment_slot_count - 1);

/// Overwrites one field of a file that is not mapped, leaving its size alone.
template <typename T>
void write_at(fs::path const& path, size_t offset, T const& value) {
    std::fstream f(path, std::ios::binary | std::ios::in | std::ios::out);
    REQUIRE(f);
    f.seekp(std::streamoff(offset));
    f.write(reinterpret_cast<char const*>(&value), sizeof(value));
    f.close();
    REQUIRE_FALSE(f.fail());
}

template <typename T>
T read_at(fs::path const& path, size_t offset) {
    auto const bytes = utxoz::testing::read_file_bytes(path);
    REQUIRE(bytes.size() >= offset + sizeof(T));
    T value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(T));
    return value;
}

/// Where the entry of a slot lies in the file.
size_t entry_offset(segment_slot slot) {
    return offsetof(segment_header, objects) + size_t(slot) * sizeof(segment_header::object);
}

/// A closed segment holding one uint64_t in the map slot: the smallest file
/// that has something for a damaged header to mislead a reader about.
void make_segment_with_one_object(fs::path const& path) {
    auto created = mapped_segment::create(path, segment_bytes);
    REQUIRE(created.has_value());
    REQUIRE((*created)->construct<uint64_t>(segment_slot::map, uint64_t(0xC0FFEE)) != nullptr);
}

void refused(fs::path const& path) {
    auto const opened = mapped_segment::open(path);
    REQUIRE_FALSE(opened.has_value());
    CHECK(opened.error() == utxoz::error_code::file_open_failed);
}

/// Stored in a segment and pointing at something else stored in it.
struct stored_link {
    explicit stored_link(uint64_t* to) : target(to) {}

    offset_ptr<uint64_t> target;
    offset_ptr<uint64_t> nothing;
};

} // namespace

// =============================================================================
// Creating and opening
// =============================================================================

TEST_CASE("a segment created is the segment opened", "[segment]") {
    scratch_file const file("round_trip");

    struct record {
        uint64_t a;
        uint32_t b;
        std::array<uint8_t, 12> c;
    };
    record const written{0x0123456789ABCDEFull, 0xDEADBEEFu, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}};

    size_t free_when_closed = 0;
    {
        auto created = mapped_segment::create(file.path, segment_bytes);
        REQUIRE(created.has_value());
        auto& segment = **created;
        CHECK(segment.get_address() != nullptr);
        CHECK(segment.get_size() == segment_bytes);

        auto* const placed = segment.construct<record>(segment_slot::map, written);
        REQUIRE(placed != nullptr);
        CHECK(placed->a == written.a);
        free_when_closed = segment.get_free_memory();
    }

    // The file is exactly the size asked for: nothing is appended to it, and
    // nothing about closing it changes that.
    CHECK(fs::file_size(file.path) == segment_bytes);

    auto opened = mapped_segment::open(file.path);
    REQUIRE(opened.has_value());
    auto const& segment = **opened;
    CHECK(segment.get_size() == segment_bytes);

    auto const found = segment.find<record>(segment_slot::map);
    REQUIRE(found.first != nullptr);
    CHECK(found.second == 1);
    CHECK(found.first->a == written.a);
    CHECK(found.first->b == written.b);
    CHECK(found.first->c == written.c);

    // The allocation mark came back with it, so a reopened file does not hand
    // out again what it already gave.
    CHECK(segment.get_free_memory() == free_when_closed);

    // And the slots nothing was put in are still empty, not merely unread.
    CHECK(segment.find<record>(segment_slot::stamp).first == nullptr);
    CHECK(segment.find<record>(segment_slot::stamp).second == 0);
    CHECK(segment.find<record>(segment_slot::merge_marker).first == nullptr);
}

TEST_CASE("an object written through one mapping is read through another", "[segment]") {
    // What makes a mapped file a store rather than a buffer: the bytes are the
    // file's, not the mapping's. Two mappings of one file are held at once, so
    // this does not depend on anything having been written back in between.
    scratch_file const file("shared");

    auto first = mapped_segment::create(file.path, segment_bytes);
    REQUIRE(first.has_value());
    auto* const written = (*first)->construct<uint64_t>(segment_slot::map, uint64_t(41));
    REQUIRE(written != nullptr);

    auto second = mapped_segment::open(file.path);
    REQUIRE(second.has_value());
    auto const seen = (*second)->find<uint64_t>(segment_slot::map);
    REQUIRE(seen.first != nullptr);
    CHECK(seen.first != written);   // another mapping, another address
    CHECK(*seen.first == 41);

    *written = 42;
    CHECK(*seen.first == 42);
}

TEST_CASE("creating a segment refuses a name that is taken", "[segment]") {
    scratch_file const file("collision");

    // A real segment with something in it, so that "refused" can be told from
    // "refused after emptying it".
    make_segment_with_one_object(file.path);
    auto const before = utxoz::testing::read_file_bytes(file.path);
    REQUIRE(before.size() == segment_bytes);

    auto const again = mapped_segment::create(file.path, segment_bytes);
    REQUIRE_FALSE(again.has_value());
    CHECK(again.error() == utxoz::error_code::identity_collision);

    // The caller believed the version was new. It was wrong about that, and the
    // file it was wrong about is the one thing that must not pay for it: a
    // creation that cleans up after its own failure removes what it made, and
    // this file is not something it made.
    REQUIRE(fs::exists(file.path));
    CHECK(utxoz::testing::read_file_bytes(file.path) == before);

    // Any file at the name, not only a segment: the collision is about the
    // name.
    scratch_file const other("collision_foreign");
    {
        std::ofstream ofs(other.path, std::ios::binary | std::ios::trunc);
        ofs << "not a segment at all";
    }
    auto const over_foreign = mapped_segment::create(other.path, segment_bytes);
    REQUIRE_FALSE(over_foreign.has_value());
    CHECK(over_foreign.error() == utxoz::error_code::identity_collision);
    CHECK(utxoz::testing::read_file_text(other.path) == "not a segment at all");
}

TEST_CASE("a segment too small to hold its own header is not created", "[segment]") {
    scratch_file const file("undersized");

    auto const created = mapped_segment::create(file.path, sizeof(segment_header) - 1);
    REQUIRE_FALSE(created.has_value());
    CHECK(created.error() == utxoz::error_code::file_open_failed);

    // Nothing left at the name to collide with the attempt that gets it right.
    CHECK_FALSE(fs::exists(file.path));

    // The header alone is a segment, if a useless one: it opens and has no room.
    auto const bare = mapped_segment::create(file.path, sizeof(segment_header));
    REQUIRE(bare.has_value());
    CHECK((*bare)->get_free_memory() == 0);
}

// =============================================================================
// What opening refuses
// =============================================================================

TEST_CASE("opening refuses a file that is not there", "[segment]") {
    scratch_file const file("absent");

    refused(file.path);
    // Opening is not creating: the name is still free afterwards.
    CHECK_FALSE(fs::exists(file.path));
}

TEST_CASE("opening refuses a file too small to be a segment", "[segment]") {
    scratch_file const file("small");

    SECTION("empty") {
        { std::ofstream const ofs(file.path, std::ios::binary | std::ios::trunc); }
        REQUIRE(fs::file_size(file.path) == 0);
        refused(file.path);
    }
    SECTION("a few bytes") {
        {
            std::ofstream ofs(file.path, std::ios::binary | std::ios::trunc);
            ofs << "far too small to be a segment";
        }
        refused(file.path);
    }
    SECTION("a real segment cut one byte short of its header") {
        // The beginning is genuine, magic and all. What is missing is the end
        // of the header, which is where the last slots are: reading them would
        // be reading past the mapping.
        make_segment_with_one_object(file.path);
        fs::resize_file(file.path, sizeof(segment_header) - 1);
        refused(file.path);
    }

    // Refused, not repaired and not removed.
    CHECK(fs::exists(file.path));
}

TEST_CASE("opening refuses a file that does not begin as a segment does", "[segment]") {
    scratch_file const file("magic");
    make_segment_with_one_object(file.path);

    SECTION("one byte of the magic") {
        auto magic = segment_header::expected_magic;
        magic.back() ^= 0x01;
        write_at(file.path, offsetof(segment_header, magic), magic);
        refused(file.path);
    }
    SECTION("no magic at all") {
        // What a file reads as when it was sized and its creation never got as
        // far as the last thing it writes.
        write_at(file.path, offsetof(segment_header, magic), std::array<char, 8>{});
        refused(file.path);
    }
    SECTION("a file of the right size and nothing else") {
        std::error_code ec;
        fs::remove(file.path, ec);
        { std::ofstream const ofs(file.path, std::ios::binary | std::ios::trunc); }
        fs::resize_file(file.path, segment_bytes);
        refused(file.path);
    }
}

TEST_CASE("opening refuses a header format this build does not read", "[segment]") {
    scratch_file const file("format");
    make_segment_with_one_object(file.path);

    SECTION("a later format") {
        write_at(file.path, offsetof(segment_header, format),
                 uint32_t(segment_header::current_format + 1));
        refused(file.path);
    }
    SECTION("a header of another size") {
        // Same format number, different shape: the slots would be read from
        // where this build keeps them, not from where that one put them.
        write_at(file.path, offsetof(segment_header, header_bytes),
                 uint32_t(sizeof(segment_header) + sizeof(segment_header::object)));
        refused(file.path);
    }
}

TEST_CASE("opening refuses a header that disagrees with the size of its file", "[segment]") {
    scratch_file const file("size");
    make_segment_with_one_object(file.path);
    REQUIRE(read_at<uint64_t>(file.path, offsetof(segment_header, file_bytes)) == segment_bytes);

    SECTION("the file was cut short") {
        // The header is whole and says what it always said. The space it
        // describes is no longer all there, and an allocation would be handed
        // addresses beyond the mapping.
        fs::resize_file(file.path, segment_bytes - 4096);
        refused(file.path);
    }
    SECTION("the file grew") {
        fs::resize_file(file.path, segment_bytes + 4096);
        refused(file.path);
    }
    SECTION("the header claims more than the file") {
        write_at(file.path, offsetof(segment_header, file_bytes), uint64_t(segment_bytes + 1));
        refused(file.path);
    }
    SECTION("the header claims less than the file") {
        write_at(file.path, offsetof(segment_header, file_bytes), uint64_t(segment_bytes - 1));
        refused(file.path);
    }
}

TEST_CASE("opening refuses an allocation mark that cannot be true", "[segment]") {
    scratch_file const file("mark");
    make_segment_with_one_object(file.path);

    SECTION("inside the header") {
        // Everything below the mark is taken, and the header is the first thing
        // taken. A mark inside it would hand the header out as free space.
        write_at(file.path, offsetof(segment_header, allocated),
                 uint64_t(sizeof(segment_header) - 1));
        refused(file.path);
    }
    SECTION("zero") {
        write_at(file.path, offsetof(segment_header, allocated), uint64_t(0));
        refused(file.path);
    }
    SECTION("past the end of the file") {
        // get_free_memory() is a subtraction of this from the size: past the
        // end it would wrap, and report almost all of the address space free.
        write_at(file.path, offsetof(segment_header, allocated), uint64_t(segment_bytes + 1));
        refused(file.path);
    }
}

TEST_CASE("opening refuses an object that lies outside what was allocated", "[segment]") {
    scratch_file const file("object");
    make_segment_with_one_object(file.path);

    auto const entry = entry_offset(segment_slot::map);
    auto const offset_field = entry + offsetof(segment_header::object, offset);
    auto const bytes_field = entry + offsetof(segment_header::object, bytes);

    auto const allocated = read_at<uint64_t>(file.path, offsetof(segment_header, allocated));
    auto const offset = read_at<uint64_t>(file.path, offset_field);
    REQUIRE(offset >= sizeof(segment_header));
    REQUIRE(read_at<uint64_t>(file.path, bytes_field) == sizeof(uint64_t));
    REQUIRE(offset + sizeof(uint64_t) == allocated);   // the last thing handed out

    SECTION("it begins past the mark") {
        // Inside the file, so nothing would fault: it would be read as an
        // object out of space nobody ever wrote to.
        write_at(file.path, offset_field, uint64_t(allocated + 8));
        refused(file.path);
    }
    SECTION("it begins past the end of the file") {
        write_at(file.path, offset_field, uint64_t(segment_bytes + 8));
        refused(file.path);
    }
    SECTION("it begins inside the header") {
        write_at(file.path, offset_field, uint64_t(sizeof(segment_header) - 8));
        refused(file.path);
    }
    SECTION("it begins where it should and runs past the mark") {
        write_at(file.path, bytes_field, uint64_t(sizeof(uint64_t) + 1));
        refused(file.path);
    }
    SECTION("its size is one that wraps when added to where it begins") {
        // The check has to be a subtraction on the side that cannot overflow.
        // Written as `offset + bytes > allocated`, this is the value that
        // would have passed it.
        write_at(file.path, bytes_field, ~uint64_t(0) - offset + 1);
        refused(file.path);
    }
    SECTION("a slot the store does not use is checked like the rest") {
        // The check is of the header against the file, so it cannot know which
        // slots somebody will ask for.
        write_at(file.path, entry_offset(spare_slot) + offsetof(segment_header::object, offset),
                 uint64_t(allocated + 8));
        refused(file.path);
    }
    SECTION("and the same file, undamaged, opens") {
        // The control. Without it every section above would equally be pinning
        // "a file that write_at() touched does not open".
        write_at(file.path, offset_field, offset);
        auto const opened = mapped_segment::open(file.path);
        REQUIRE(opened.has_value());
        auto const found = (*opened)->find<uint64_t>(segment_slot::map);
        REQUIRE(found.first != nullptr);
        CHECK(*found.first == 0xC0FFEE);
    }
}

// =============================================================================
// Slots
// =============================================================================

TEST_CASE("a slot is constructed once", "[segment]") {
    scratch_file const file("once");
    auto created = mapped_segment::create(file.path, segment_bytes);
    REQUIRE(created.has_value());
    auto& segment = **created;

    auto* const first = segment.construct<uint64_t>(segment_slot::map, uint64_t(1));
    REQUIRE(first != nullptr);
    auto const free_after_first = segment.get_free_memory();

    // Null, not the object that is there: the caller asked for a new one, and
    // being handed the old one is how a file that already holds a map comes
    // back looking like the empty one just made.
    auto* const second = segment.construct<uint64_t>(segment_slot::map, uint64_t(2));
    CHECK(second == nullptr);

    // And the refusal cost nothing: the first object is as it was, and no
    // space was claimed for the one that was not built.
    CHECK(*first == 1);
    CHECK(segment.get_free_memory() == free_after_first);
    auto const found = segment.find<uint64_t>(segment_slot::map);
    CHECK(found.first == first);
    CHECK(found.second == 1);

    // Whatever the type asked for the second time. A slot is taken by what is
    // in it, not by what the caller hoped to put there.
    CHECK(segment.construct<std::array<uint64_t, 4>>(segment_slot::map) == nullptr);
    CHECK(segment.get_free_memory() == free_after_first);

    // The other slots are their own.
    CHECK(segment.construct<uint64_t>(segment_slot::stamp, uint64_t(3)) != nullptr);
    CHECK(*first == 1);
}

TEST_CASE("a slot is still taken after the file is reopened", "[segment]") {
    scratch_file const file("once_reopened");
    make_segment_with_one_object(file.path);

    auto opened = mapped_segment::open(file.path);
    REQUIRE(opened.has_value());
    CHECK((*opened)->construct<uint64_t>(segment_slot::map, uint64_t(2)) == nullptr);

    auto const found = (*opened)->find<uint64_t>(segment_slot::map);
    REQUIRE(found.first != nullptr);
    CHECK(*found.first == 0xC0FFEE);
}

TEST_CASE("find measures what a slot holds in units of what it is asked for", "[segment]") {
    // The count is the only thing find() can say about the type, and it is what
    // find_single_named() refuses on. Pinned here at the source.
    scratch_file const file("count");
    auto created = mapped_segment::create(file.path, segment_bytes);
    REQUIRE(created.has_value());
    auto& segment = **created;

    CHECK(segment.find<uint64_t>(segment_slot::map).first == nullptr);
    CHECK(segment.find<uint64_t>(segment_slot::map).second == 0);

    REQUIRE(segment.construct<std::array<uint64_t, 3>>(segment_slot::map) != nullptr);

    CHECK(segment.find<std::array<uint64_t, 3>>(segment_slot::map).second == 1);
    CHECK(segment.find<uint64_t>(segment_slot::map).second == 3);
    // Larger than what is there: zero of them, which is not one either.
    CHECK(segment.find<std::array<uint64_t, 4>>(segment_slot::map).second == 0);
    CHECK(segment.find<std::array<uint64_t, 4>>(segment_slot::map).first != nullptr);
}

TEST_CASE("an object that does not begin where its type can is not one of them", "[segment]") {
    // What open() cannot refuse. The slot's object is followed by another, so
    // moving its offset one byte on leaves all of it inside what was allocated
    // and the header agrees with the file. Only the type knows it cannot begin
    // there.
    scratch_file const file("misaligned");
    uint64_t offset = 0;
    {
        auto created = mapped_segment::create(file.path, segment_bytes);
        REQUIRE(created.has_value());
        REQUIRE((*created)->construct<uint64_t>(segment_slot::map, uint64_t(0xC0FFEE)) != nullptr);
        REQUIRE((*created)->construct<std::array<uint64_t, 4>>(spare_slot) != nullptr);
    }
    auto const offset_field = entry_offset(segment_slot::map) + offsetof(segment_header::object, offset);
    offset = read_at<uint64_t>(file.path, offset_field);
    REQUIRE(offset % alignof(uint64_t) == 0);

    SECTION("moved one byte on, it opens and measures as none") {
        write_at(file.path, offset_field, uint64_t(offset + 1));

        auto opened = mapped_segment::open(file.path);
        REQUIRE(opened.has_value());
        auto const found = (*opened)->find<uint64_t>(segment_slot::map);
        // Not null: the slot is taken, and reading it as empty would turn
        // damage into absence.
        CHECK(found.first != nullptr);
        CHECK(found.second == 0);

        auto const guarded = utxoz::detail::find_single_named<uint64_t>(
            **opened, segment_slot::map, file.path);
        REQUIRE_FALSE(guarded.has_value());
        CHECK(guarded.error() == utxoz::error_code::version_unreadable);
    }
    SECTION("a type that may begin anywhere still reads it") {
        // The refusal is the type's alignment and nothing coarser.
        write_at(file.path, offset_field, uint64_t(offset + 1));
        auto opened = mapped_segment::open(file.path);
        REQUIRE(opened.has_value());
        CHECK((*opened)->find<unsigned char>(segment_slot::map).second == sizeof(uint64_t));
    }
    SECTION("and where it was, it is one") {
        auto opened = mapped_segment::open(file.path);
        REQUIRE(opened.has_value());
        auto const found = (*opened)->find<uint64_t>(segment_slot::map);
        REQUIRE(found.second == 1);
        CHECK(*found.first == 0xC0FFEE);
    }
}

// =============================================================================
// Space
// =============================================================================

TEST_CASE("free memory goes down by what was allocated", "[segment]") {
    scratch_file const file("free");
    auto created = mapped_segment::create(file.path, segment_bytes);
    REQUIRE(created.has_value());
    auto& segment = **created;

    // A new segment has given out its header and nothing else.
    auto free_bytes = segment.get_free_memory();
    CHECK(free_bytes == segment_bytes - sizeof(segment_header));

    // The header ends on an eight-byte boundary, so this costs exactly itself.
    REQUIRE(sizeof(segment_header) % alignof(uint64_t) == 0);
    REQUIRE(segment.construct<uint64_t>(segment_slot::stamp, uint64_t(1)) != nullptr);
    CHECK(segment.get_free_memory() == free_bytes - sizeof(uint64_t));
    free_bytes = segment.get_free_memory();

    // One byte, which leaves the mark off the boundary...
    REQUIRE(segment.construct<uint8_t>(segment_slot::map, uint8_t(2)) != nullptr);
    CHECK(segment.get_free_memory() == free_bytes - 1);
    free_bytes = segment.get_free_memory();

    // ...so the next aligned object pays for the gap as well as for itself.
    // The gap is space nobody can use, and reporting it as free would be a
    // figure the next allocation could not honour.
    auto* const aligned = segment.construct<uint64_t>(segment_slot::merge_marker, uint64_t(3));
    REQUIRE(aligned != nullptr);
    CHECK(reinterpret_cast<std::uintptr_t>(aligned) % alignof(uint64_t) == 0);
    CHECK(segment.get_free_memory() == free_bytes - (alignof(uint64_t) - 1) - sizeof(uint64_t));
    free_bytes = segment.get_free_memory();

    // Through the allocator the maps use, it is the same mark.
    auto allocator = segment.get_allocator<uint64_t>();
    auto const block = allocator.allocate(100);
    REQUIRE(block.get() != nullptr);
    CHECK(segment.get_free_memory() == free_bytes - 100 * sizeof(uint64_t));

    // Every object landed inside the mapping and none on top of another.
    auto const* const base = static_cast<std::byte const*>(segment.get_address());
    auto const* const end = base + segment.get_size();
    auto const* const one = reinterpret_cast<std::byte const*>(
        segment.find<uint64_t>(segment_slot::stamp).first);
    auto const* const two = reinterpret_cast<std::byte const*>(
        segment.find<uint8_t>(segment_slot::map).first);
    auto const* const three = reinterpret_cast<std::byte const*>(aligned);
    auto const* const four = reinterpret_cast<std::byte const*>(block.get());
    CHECK(base + sizeof(segment_header) <= one);
    CHECK(one + sizeof(uint64_t) <= two);
    CHECK(two + 1 <= three);
    CHECK(three + sizeof(uint64_t) <= four);
    CHECK(four + 100 * sizeof(uint64_t) <= end);
}

TEST_CASE("only the block handed out last is taken back", "[segment]") {
    // Enough for a map that failed to grow to return what it asked for, and
    // deliberately no more: there is no list of holes to come back damaged.
    scratch_file const file("giveback");
    auto created = mapped_segment::create(file.path, segment_bytes);
    REQUIRE(created.has_value());
    auto& segment = **created;
    auto allocator = segment.get_allocator<uint64_t>();

    auto const fresh = segment.get_free_memory();
    auto const first = allocator.allocate(10);
    auto const after_first = segment.get_free_memory();
    auto const second = allocator.allocate(20);
    auto const after_second = segment.get_free_memory();
    REQUIRE(after_first == fresh - 10 * sizeof(uint64_t));
    REQUIRE(after_second == after_first - 20 * sizeof(uint64_t));

    // Not the last one: the space stays taken.
    allocator.deallocate(first, 10);
    CHECK(segment.get_free_memory() == after_second);

    // The last one: the mark goes back to where it was before it.
    allocator.deallocate(second, 20);
    CHECK(segment.get_free_memory() == after_first);

    // And what comes next is given the same place.
    auto const third = allocator.allocate(20);
    CHECK(third == second);
}

TEST_CASE("an allocation past the end of the segment is refused as segment_full", "[segment]") {
    scratch_file const file("full");

    // Room for exactly eight uint64_t after the header.
    constexpr size_t room = 8 * sizeof(uint64_t);
    auto created = mapped_segment::create(file.path, sizeof(segment_header) + room);
    REQUIRE(created.has_value());
    auto& segment = **created;
    REQUIRE(segment.get_free_memory() == room);

    SECTION("an object larger than what is left") {
        using too_big = std::array<uint64_t, 9>;
        CHECK_THROWS_AS((void)segment.construct<too_big>(segment_slot::map), segment_full);

        // Either way the slot stays empty, and nothing was claimed for an
        // object that does not exist: the next one that fits still fits.
        CHECK(segment.find<too_big>(segment_slot::map).first == nullptr);
        CHECK(segment.get_free_memory() == room);
        CHECK(segment.construct<std::array<uint64_t, 8>>(segment_slot::map) != nullptr);
        CHECK(segment.get_free_memory() == 0);
    }
    SECTION("exactly what is left fits, and then nothing does") {
        auto allocator = segment.get_allocator<uint64_t>();
        auto const all = allocator.allocate(8);
        REQUIRE(all.get() != nullptr);
        CHECK(segment.get_free_memory() == 0);

        // The last byte handed out is the last byte of the file.
        CHECK(reinterpret_cast<std::byte const*>(all.get()) + room
              == static_cast<std::byte const*>(segment.get_address()) + segment.get_size());

        CHECK_THROWS_AS((void)allocator.allocate(1), segment_full);
        CHECK_THROWS_AS((void)segment.construct<uint8_t>(segment_slot::map, uint8_t(1)),
                        segment_full);
        CHECK(segment.find<uint8_t>(segment_slot::map).first == nullptr);
        CHECK(segment.get_free_memory() == 0);
    }
    SECTION("a request whose size in bytes does not fit a size_t") {
        // Refused as full rather than wrapped into a small request that is
        // then granted.
        auto allocator = segment.get_allocator<uint64_t>();
        CHECK_THROWS_AS((void)allocator.allocate(size_t(-1) / sizeof(uint64_t) + 1), segment_full);
        CHECK_THROWS_AS((void)allocator.allocate(size_t(-1) / sizeof(uint64_t)), segment_full);
        CHECK(segment.get_free_memory() == room);
    }
    SECTION("a failed request is not remembered") {
        auto allocator = segment.get_allocator<uint64_t>();
        CHECK_THROWS_AS((void)allocator.allocate(9), segment_full);
        CHECK(segment.get_free_memory() == room);
        CHECK(allocator.allocate(8).get() != nullptr);
    }
}

TEST_CASE("a full segment is still full after it is reopened", "[segment]") {
    scratch_file const file("full_reopened");
    {
        auto created = mapped_segment::create(file.path,
                                              sizeof(segment_header) + sizeof(uint64_t));
        REQUIRE(created.has_value());
        REQUIRE((*created)->construct<uint64_t>(segment_slot::map, uint64_t(7)) != nullptr);
        REQUIRE((*created)->get_free_memory() == 0);
    }

    // A mark exactly at the end of the file is a true one, and the open has to
    // take it: refusing it would make every file that was filled unreadable.
    auto opened = mapped_segment::open(file.path);
    REQUIRE(opened.has_value());
    CHECK((*opened)->get_free_memory() == 0);
    CHECK_THROWS_AS((void)(*opened)->construct<uint8_t>(segment_slot::stamp, uint8_t(1)),
                    segment_full);
    auto const found = (*opened)->find<uint64_t>(segment_slot::map);
    REQUIRE(found.first != nullptr);
    CHECK(*found.first == 7);
}

// =============================================================================
// Pointers that live in the file
// =============================================================================

TEST_CASE("an offset_ptr stored in a segment follows the segment", "[segment]") {
    // The map keeps pointers into the file it lives in, and the file is mapped
    // wherever the kernel puts it. A pointer that held an address would be
    // right until the first reopen and then point at whatever is there now.
    scratch_file const file("offset_ptr");

    {
        auto created = mapped_segment::create(file.path, segment_bytes);
        REQUIRE(created.has_value());
        auto& segment = **created;

        // The target after the pointer and the target before it are different
        // signs of the same distance, so both directions are stored.
        auto* const before = segment.construct<uint64_t>(segment_slot::stamp, uint64_t(0xB4));
        REQUIRE(before != nullptr);
        auto* const back = segment.construct<stored_link>(segment_slot::map, before);
        REQUIRE(back != nullptr);
        // Built pointing at nothing and aimed afterwards, since what it is to
        // point at does not exist until space after it has been handed out.
        auto* const forward = segment.construct<stored_link>(segment_slot::merge_marker, nullptr);
        REQUIRE(forward != nullptr);
        auto* const after = segment.construct<uint64_t>(spare_slot, uint64_t(0xAF));
        REQUIRE(after != nullptr);
        forward->target = offset_ptr<uint64_t>(after);

        REQUIRE(reinterpret_cast<std::uintptr_t>(before) < reinterpret_cast<std::uintptr_t>(back));
        REQUIRE(reinterpret_cast<std::uintptr_t>(after) > reinterpret_cast<std::uintptr_t>(forward));
        CHECK(back->target.get() == before);
        CHECK(forward->target.get() == after);
        CHECK(back->nothing.get() == nullptr);
    }

    // Closed and opened again — and opened twice, both held, so that at least
    // one of the two mappings is certain to be somewhere the first was not.
    // "It happened to come back at the same address" would otherwise make
    // this case pass for a pointer that stored one.
    auto one = mapped_segment::open(file.path);
    auto two = mapped_segment::open(file.path);
    REQUIRE(one.has_value());
    REQUIRE(two.has_value());
    REQUIRE((*one)->get_address() != (*two)->get_address());

    for (auto const* const segment : {one->get(), two->get()}) {
        auto const before = segment->find<uint64_t>(segment_slot::stamp);
        auto const back = segment->find<stored_link>(segment_slot::map);
        auto const after = segment->find<uint64_t>(spare_slot);
        auto const forward = segment->find<stored_link>(segment_slot::merge_marker);
        REQUIRE(before.first != nullptr);
        REQUIRE(back.first != nullptr);
        REQUIRE(after.first != nullptr);
        REQUIRE(forward.first != nullptr);
        CHECK(back.second == 1);
        CHECK(forward.second == 1);

        // The same object, at the address it has in *this* mapping.
        CHECK(back.first->target.get() == before.first);
        CHECK(*back.first->target == 0xB4);
        CHECK(forward.first->target.get() == after.first);
        CHECK(*forward.first->target == 0xAF);

        // And inside it, which the equality above already implies for a
        // target that was found — stated on its own because it is the
        // property: a pointer in the file does not lead out of the file.
        auto const* const base = static_cast<std::byte const*>(segment->get_address());
        auto const* const target = reinterpret_cast<std::byte const*>(back.first->target.get());
        CHECK(target >= base);
        CHECK(target < base + segment->get_size());

        // Null is a value like any other and comes back as null, not as a
        // pointer to wherever the null used to be relative to.
        CHECK(back.first->nothing.get() == nullptr);
        CHECK_FALSE(back.first->nothing);
        CHECK(forward.first->nothing == nullptr);
    }

    // The two mappings are the same file: what one pointer reaches, the other
    // sees change.
    *(*one)->find<stored_link>(segment_slot::map).first->target = 0x1234;
    CHECK(*(*two)->find<stored_link>(segment_slot::map).first->target == 0x1234);
}

TEST_CASE("the allocator a map stores finds its segment again after a reopen", "[segment]") {
    // A map keeps its allocator inside itself, so the allocator is in the file
    // too and its way back to the header has to move with the mapping like
    // every other pointer there. An allocator that came back pointing at the
    // old mapping would hand out addresses in memory that is no longer this
    // file — or no longer mapped at all.
    scratch_file const file("allocator");
    using allocator_t = utxoz::detail::segment_allocator<uint64_t>;

    size_t free_when_closed = 0;
    {
        auto created = mapped_segment::create(file.path, segment_bytes);
        REQUIRE(created.has_value());
        auto& segment = **created;
        REQUIRE(segment.construct<allocator_t>(segment_slot::map,
                                               segment.get_allocator<uint64_t>()) != nullptr);
        free_when_closed = segment.get_free_memory();
    }

    auto one = mapped_segment::open(file.path);
    auto two = mapped_segment::open(file.path);
    REQUIRE(one.has_value());
    REQUIRE(two.has_value());
    REQUIRE((*one)->get_address() != (*two)->get_address());

    auto* const stored_one = (*one)->find<allocator_t>(segment_slot::map).first;
    auto* const stored_two = (*two)->find<allocator_t>(segment_slot::map).first;
    REQUIRE(stored_one != nullptr);
    REQUIRE(stored_two != nullptr);

    // Each leads to the header of the mapping it was read through.
    CHECK(static_cast<void*>(stored_one->header()) == (*one)->get_address());
    CHECK(static_cast<void*>(stored_two->header()) == (*two)->get_address());
    CHECK(*stored_one == (*one)->get_allocator<uint64_t>());
    CHECK_FALSE(*stored_one == (*two)->get_allocator<uint64_t>());

    // And what it hands out is in that mapping, taken from the one mark both
    // mappings share.
    auto const block = stored_one->allocate(4);
    auto const* const base = static_cast<std::byte const*>((*one)->get_address());
    auto const* const at = reinterpret_cast<std::byte const*>(block.get());
    CHECK(at >= base + sizeof(segment_header));
    CHECK(at + 4 * sizeof(uint64_t) <= base + (*one)->get_size());
    CHECK((*one)->get_free_memory() == free_when_closed - 4 * sizeof(uint64_t));
    CHECK((*two)->get_free_memory() == free_when_closed - 4 * sizeof(uint64_t));
}
