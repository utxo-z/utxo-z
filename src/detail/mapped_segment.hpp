// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file mapped_segment.hpp
 * @brief A version file: one mapping, a fixed header, and the objects in it.
 * @internal
 *
 * A version file holds very little: a format stamp, one map, and on a merge
 * target a marker. Each is written once, by the mutation that creates the file,
 * and reached again every time the file is opened. That is all this has to do,
 * and it is all it does.
 *
 * What it deliberately does not have:
 *
 * - **No lock in the file.** A lock that lives in a mapping is a dirty page when
 *   taken, and a taken state that reaches the disk before a power loss comes
 *   back held by nobody. One process owns a database, and its mutations are
 *   serialised by the caller; the only shared word here is the allocation mark,
 *   which is an atomic so that the threads of one insert_batch() may each claim
 *   space, and which has no state a crash could leave stuck.
 * - **No name index.** The objects sit in numbered slots of the header, so
 *   reaching one is two reads and cannot be confused with a search that might
 *   change underneath it.
 * - **Nothing platform-specific in the bytes.** The header is integers of fixed
 *   width, and the pointers the map stores are offsets from themselves.
 *
 * Space is handed out by advancing a mark and never taken back, except for the
 * block handed out last. The maps here are built with their final capacity and
 * are never grown, so nothing asks for more.
 */

#pragma once

#include <array>
#include <atomic>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <cerrno>
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

#include <utxoz/types.hpp>

#include "log.hpp"
#include "path_display.hpp"

namespace utxoz::detail {

namespace fs = std::filesystem;

// =============================================================================
// offset_ptr
// =============================================================================

/**
 * @brief A pointer stored as the distance from itself to what it points at.
 *
 * The map lives in a mapping whose address differs from one open to the next,
 * so anything it stores that points into the same mapping has to survive being
 * moved as a whole. A distance from the pointer's own address does.
 *
 * One distance cannot mean "points at itself" and "points at nothing" both, and
 * a pointer to oneself is the rarer need: 1 is null, as no object starts one
 * byte into a pointer.
 */
template <typename T>
struct offset_ptr {
    using element_type = T;
    using difference_type = std::ptrdiff_t;
    template <typename U> using rebind = offset_ptr<U>;

    offset_ptr() noexcept = default;
    offset_ptr(std::nullptr_t) noexcept {}
    offset_ptr(T* p) noexcept { set(p); }
    offset_ptr(offset_ptr const& other) noexcept { set(other.get()); }
    template <typename U>
        requires std::is_convertible_v<U*, T*>
    offset_ptr(offset_ptr<U> const& other) noexcept { set(other.get()); }

    offset_ptr& operator=(offset_ptr const& other) noexcept { set(other.get()); return *this; }
    offset_ptr& operator=(std::nullptr_t) noexcept { offset_ = null_offset; return *this; }

    [[nodiscard]] T* get() const noexcept {
        if (offset_ == null_offset) return nullptr;
        return reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(this) + std::uintptr_t(offset_));
    }

    T* operator->() const noexcept { return get(); }
    template <typename U = T>
        requires ( ! std::is_void_v<U>)
    U& operator*() const noexcept { return *get(); }
    explicit operator bool() const noexcept { return offset_ != null_offset; }

    template <typename U = T>
        requires ( ! std::is_void_v<U>)
    [[nodiscard]] static offset_ptr pointer_to(U& r) noexcept { return offset_ptr(std::addressof(r)); }

    friend bool operator==(offset_ptr const& a, offset_ptr const& b) noexcept { return a.get() == b.get(); }
    friend bool operator==(offset_ptr const& a, std::nullptr_t) noexcept { return ! a; }
    friend auto operator<=>(offset_ptr const& a, offset_ptr const& b) noexcept { return a.get() <=> b.get(); }

    // The arithmetic of a pointer into an array, which is what the map keeps.
    // Constrained on the argument so that offset_ptr<void> simply lacks it.
    offset_ptr& operator+=(difference_type n) noexcept requires ( ! std::is_void_v<T>) { set(get() + n); return *this; }
    offset_ptr& operator-=(difference_type n) noexcept requires ( ! std::is_void_v<T>) { set(get() - n); return *this; }
    offset_ptr& operator++() noexcept requires ( ! std::is_void_v<T>) { return *this += 1; }
    offset_ptr& operator--() noexcept requires ( ! std::is_void_v<T>) { return *this -= 1; }
    offset_ptr operator++(int) noexcept requires ( ! std::is_void_v<T>) { auto const was = *this; ++*this; return was; }
    offset_ptr operator--(int) noexcept requires ( ! std::is_void_v<T>) { auto const was = *this; --*this; return was; }
    friend offset_ptr operator+(offset_ptr const& p, difference_type n) noexcept requires ( ! std::is_void_v<T>) { return offset_ptr(p.get() + n); }
    friend offset_ptr operator+(difference_type n, offset_ptr const& p) noexcept requires ( ! std::is_void_v<T>) { return offset_ptr(p.get() + n); }
    friend offset_ptr operator-(offset_ptr const& p, difference_type n) noexcept requires ( ! std::is_void_v<T>) { return offset_ptr(p.get() - n); }
    friend difference_type operator-(offset_ptr const& a, offset_ptr const& b) noexcept requires ( ! std::is_void_v<T>) { return a.get() - b.get(); }
    template <typename U = T>
        requires ( ! std::is_void_v<U>)
    U& operator[](difference_type n) const noexcept { return get()[n]; }

private:
    static constexpr std::ptrdiff_t null_offset = 1;

    void set(T* p) noexcept {
        offset_ = p == nullptr
            ? null_offset
            : std::ptrdiff_t(reinterpret_cast<std::uintptr_t>(p) - reinterpret_cast<std::uintptr_t>(this));
    }

    std::ptrdiff_t offset_ = null_offset;
};

static_assert(sizeof(offset_ptr<void>) == sizeof(std::ptrdiff_t));

// =============================================================================
// The header
// =============================================================================

/// The objects a version file may hold. The number is the slot, and it is
/// persisted: add at the end, never reorder.
enum class segment_slot : uint32_t {
    stamp = 0,          ///< segment_stamp
    map = 1,            ///< the container
    merge_marker = 2,   ///< on a merge target only
};

inline constexpr size_t segment_slot_count = 8;

/// What a full segment says to whoever asked it for space. Distinct from
/// std::bad_alloc, which is the heap's: the remedy is a new version file, not
/// less memory.
struct segment_full : std::exception {
    [[nodiscard]] char const* what() const noexcept override { return "segment full"; }
};

/**
 * @brief The first bytes of every version file.
 *
 * Fixed-width integers and nothing else, so that it means the same wherever it
 * is read. `allocated` is the only field written after the file is created.
 */
struct segment_header {
    static constexpr std::array<char, 8> expected_magic{'U', 'T', 'X', 'O', 'Z', 'S', 'E', 'G'};
    static constexpr uint32_t current_format = 1;

    struct object {
        uint64_t offset = 0;   ///< From the start of the file. Zero: the slot is empty.
        uint64_t bytes = 0;    ///< What was constructed there, as sizeof said then.
    };

    std::array<char, 8> magic{};
    uint32_t format = 0;
    uint32_t header_bytes = 0;
    uint64_t file_bytes = 0;
    std::atomic<uint64_t> allocated{0};   ///< Everything below this offset is taken.
    std::array<object, segment_slot_count> objects{};

    /// Claims `bytes` aligned to `alignment`, or throws segment_full.
    [[nodiscard]] void* allocate(size_t bytes, size_t alignment) {
        auto* const base = reinterpret_cast<std::byte*>(this);
        auto mark = allocated.load(std::memory_order_relaxed);
        for (;;) {
            auto const start = (mark + (alignment - 1)) & ~uint64_t(alignment - 1);
            if (start > file_bytes || bytes > file_bytes - start) throw segment_full{};
            if (allocated.compare_exchange_weak(mark, start + bytes, std::memory_order_relaxed)) {
                return base + start;
            }
        }
    }

    /// Gives a block back if it was the last one handed out; otherwise the
    /// space stays taken until the file is retired. Enough for a map that
    /// failed to grow to return what it asked for.
    void deallocate(void* p, size_t bytes) noexcept {
        auto* const base = reinterpret_cast<std::byte*>(this);
        auto const start = uint64_t(static_cast<std::byte*>(p) - base);
        auto expected = start + bytes;
        allocated.compare_exchange_strong(expected, start, std::memory_order_relaxed);
    }
};

static_assert(std::atomic<uint64_t>::is_always_lock_free,
              "the allocation mark lives in the file: it has to be the eight bytes it looks like");
static_assert(sizeof(std::atomic<uint64_t>) == sizeof(uint64_t));
static_assert(sizeof(segment_header) == 8 + 4 + 4 + 8 + 8 + 16 * segment_slot_count,
              "the header is exactly its fields: padding would be bytes nobody chose");
static_assert(std::is_standard_layout_v<segment_header>);

// =============================================================================
// The allocator the maps are built with
// =============================================================================

/**
 * @brief Hands a map space inside the file it lives in.
 *
 * The map stores its allocator, so this is persisted too: it holds the way back
 * to the header as an offset, like every other pointer in the file.
 */
template <typename T>
struct segment_allocator {
    using value_type = T;
    using pointer = offset_ptr<T>;
    using const_pointer = offset_ptr<T const>;
    using void_pointer = offset_ptr<void>;
    using const_void_pointer = offset_ptr<void const>;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    template <typename U> struct rebind { using other = segment_allocator<U>; };

    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_swap = std::false_type;
    using is_always_equal = std::false_type;

    explicit segment_allocator(segment_header* header) noexcept : header_(header) {}
    segment_allocator(segment_allocator const&) noexcept = default;
    template <typename U>
    segment_allocator(segment_allocator<U> const& other) noexcept : header_(other.header()) {}
    segment_allocator& operator=(segment_allocator const&) noexcept = default;

    [[nodiscard]] pointer allocate(size_type n) {
        if (n > size_type(-1) / sizeof(T)) throw segment_full{};
        return pointer(static_cast<T*>(header_->allocate(n * sizeof(T), alignof(T))));
    }

    void deallocate(pointer p, size_type n) noexcept {
        header_->deallocate(p.get(), n * sizeof(T));
    }

    [[nodiscard]] segment_header* header() const noexcept { return header_.get(); }

    template <typename U>
    friend bool operator==(segment_allocator const& a, segment_allocator<U> const& b) noexcept {
        return a.header() == b.header();
    }

private:
    offset_ptr<segment_header> header_;
};

// =============================================================================
// The mapping
// =============================================================================

/**
 * @brief One version file, mapped whole and shared.
 *
 * Unmapped and closed by the destructor. Nothing here flushes on the way out:
 * unmapping is not a barrier, and the callers that need one make it.
 */
struct mapped_segment {
    mapped_segment(mapped_segment const&) = delete;
    mapped_segment& operator=(mapped_segment const&) = delete;

    ~mapped_segment() { unmap(); }

    /**
     * @brief Creates a version file of exactly `bytes`, which must not exist.
     *
     * @return error_code::identity_collision if the name is taken — the caller
     *         believes the version is new, so that is the catalogue and the
     *         directory disagreeing — and error_code::file_open_failed for
     *         anything else. A file this call created and could not finish is
     *         removed.
     */
    [[nodiscard]]
    static result<std::unique_ptr<mapped_segment>> create(fs::path const& path, size_t bytes) {
        if (bytes < sizeof(segment_header)) {
            log::error("{}: {} bytes cannot hold a segment", path_display(path), bytes);
            return std::unexpected(error_code::file_open_failed);
        }
        std::unique_ptr<mapped_segment> segment(new (std::nothrow) mapped_segment);
        if ( ! segment) return std::unexpected(error_code::out_of_memory);

        if (auto const made = segment->map_file(path, bytes, true); ! made) {
            return std::unexpected(made.error());
        }

        // A new file reads as zeros, so only what is not zero is written.
        auto* const header = new (segment->address_) segment_header;
        header->format = segment_header::current_format;
        header->header_bytes = uint32_t(sizeof(segment_header));
        header->file_bytes = bytes;
        header->allocated.store(sizeof(segment_header), std::memory_order_relaxed);
        // Last: a file without it is not a segment, whatever else it holds.
        header->magic = segment_header::expected_magic;
        return segment;
    }

    /**
     * @brief Maps a version file that exists, and checks it is one.
     *
     * The checks are of the header against the file, not of what the file
     * holds: an object in a slot is the stamp's to vouch for.
     */
    [[nodiscard]]
    static result<std::unique_ptr<mapped_segment>> open(fs::path const& path) {
        std::unique_ptr<mapped_segment> segment(new (std::nothrow) mapped_segment);
        if ( ! segment) return std::unexpected(error_code::out_of_memory);

        if (auto const mapped = segment->map_file(path, 0, false); ! mapped) {
            return std::unexpected(mapped.error());
        }
        if (segment->size_ < sizeof(segment_header)) {
            log::error("{}: too small to be a segment", path_display(path));
            return std::unexpected(error_code::file_open_failed);
        }

        auto const& header = segment->header();
        if (header.magic != segment_header::expected_magic) {
            log::error("{}: does not begin as a segment does", path_display(path));
            return std::unexpected(error_code::file_open_failed);
        }
        if (header.format != segment_header::current_format
                || header.header_bytes != sizeof(segment_header)) {
            log::error("{}: segment format {} is not one this build reads", path_display(path),
                       header.format);
            return std::unexpected(error_code::file_open_failed);
        }
        auto const allocated = header.allocated.load(std::memory_order_relaxed);
        if (header.file_bytes != segment->size_ || allocated < sizeof(segment_header)
                || allocated > header.file_bytes) {
            log::error("{}: its header disagrees with its size", path_display(path));
            return std::unexpected(error_code::file_open_failed);
        }
        for (auto const& object : header.objects) {
            if (object.offset == 0) continue;
            if (object.offset < sizeof(segment_header) || object.offset > allocated
                    || object.bytes > allocated - object.offset) {
                log::error("{}: an object lies outside what was allocated", path_display(path));
                return std::unexpected(error_code::file_open_failed);
            }
        }
        return segment;
    }

    /// The whole mapping: what a page barrier has to cover.
    [[nodiscard]] void* get_address() const noexcept { return address_; }
    [[nodiscard]] size_t get_size() const noexcept { return size_; }

    /// Bytes never handed out. Two reads; cannot fail.
    [[nodiscard]] size_t get_free_memory() const noexcept {
        return size_t(header().file_bytes - header().allocated.load(std::memory_order_relaxed));
    }

    /// Asks the kernel to start writing the mapping back. A hint, not a barrier.
    void flush() noexcept {
        if (address_ == nullptr) return;
#if defined(_WIN32)
        ::FlushViewOfFile(address_, 0);
#elif ! defined(__EMSCRIPTEN__)
        ::msync(address_, size_, MS_ASYNC);
#endif
    }

    template <typename T>
    [[nodiscard]] segment_allocator<T> get_allocator() noexcept {
        return segment_allocator<T>(&header());
    }

    /**
     * @brief Constructs the object of a slot, which must be empty.
     *
     * @return the object, or null if the slot already holds one. Throws
     *         segment_full if there is no room, and whatever T's constructor
     *         throws; either way the slot stays empty.
     */
    template <typename T, typename... Args>
    [[nodiscard]] T* construct(segment_slot slot, Args&&... args) {
        auto& entry = header().objects[size_t(slot)];
        if (entry.offset != 0) return nullptr;

        void* const raw = header().allocate(sizeof(T), alignof(T));
        T* object = nullptr;
        try {
            object = new (raw) T(std::forward<Args>(args)...);
        } catch (...) {
            header().deallocate(raw, sizeof(T));
            throw;
        }
        entry.bytes = sizeof(T);
        entry.offset = uint64_t(static_cast<std::byte*>(raw) - static_cast<std::byte*>(address_));
        return object;
    }

    /**
     * @brief The object of a slot, and how many T its bytes make up.
     *
     * Null and zero for an empty slot. The count is one for an object written
     * as one T by a build that agrees on sizeof(T), and the caller is expected
     * to insist on it.
     *
     * An object that does not begin where a T can begin counts as none of
     * them. open() checks every slot against the file and cannot check this:
     * it does not know what a slot holds. A map whose offset is off by one
     * still lies inside what was allocated, because its buckets follow it, and
     * read as a map it is a pointer into the middle of one. The address comes
     * back all the same, so that an object which cannot be read is not
     * mistaken for a slot with nothing in it.
     */
    template <typename T>
    [[nodiscard]] std::pair<T*, size_t> find(segment_slot slot) const noexcept {
        auto const& entry = header().objects[size_t(slot)];
        if (entry.offset == 0) return {nullptr, 0};
        auto* const object = reinterpret_cast<T*>(static_cast<std::byte*>(address_) + entry.offset);
        if (entry.offset % alignof(T) != 0) return {object, 0};
        return {object, size_t(entry.bytes / sizeof(T))};
    }

private:
    mapped_segment() = default;

    [[nodiscard]] segment_header& header() const noexcept {
        return *static_cast<segment_header*>(address_);
    }

#if defined(_WIN32)
    [[nodiscard]]
    result<> map_file(fs::path const& path, size_t bytes, bool creating) {
        file_ = ::CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                              creating ? CREATE_NEW : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_ == INVALID_HANDLE_VALUE) {
            auto const why = ::GetLastError();
            log::error("{}: cannot be {}: error {}", path_display(path),
                       creating ? "created" : "opened", why);
            return std::unexpected(creating && why == ERROR_FILE_EXISTS
                ? error_code::identity_collision : error_code::file_open_failed);
        }
        bool finished = false;
        struct withdraw {
            mapped_segment& self; fs::path const& path; bool creating; bool& finished;
            ~withdraw() {
                if (finished) return;
                self.unmap();
                if (creating) { std::error_code ec; fs::remove(path, ec); }
            }
        } const guard{*this, path, creating, finished};

        LARGE_INTEGER size{};
        if (creating) {
            size.QuadPart = LONGLONG(bytes);
            if ( ! ::SetFilePointerEx(file_, size, nullptr, FILE_BEGIN) || ! ::SetEndOfFile(file_)) {
                log::error("{}: cannot be sized to {} bytes", path_display(path), bytes);
                return std::unexpected(error_code::file_open_failed);
            }
        } else if ( ! ::GetFileSizeEx(file_, &size)) {
            log::error("{}: cannot be sized", path_display(path));
            return std::unexpected(error_code::file_open_failed);
        }
        if (size.QuadPart <= 0) {
            log::error("{}: is empty", path_display(path));
            return std::unexpected(error_code::file_open_failed);
        }

        mapping_ = ::CreateFileMappingW(file_, nullptr, PAGE_READWRITE, 0, 0, nullptr);
        if (mapping_ == nullptr) {
            log::error("{}: cannot be mapped: error {}", path_display(path), ::GetLastError());
            return std::unexpected(error_code::file_open_failed);
        }
        address_ = ::MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (address_ == nullptr) {
            log::error("{}: cannot be mapped: error {}", path_display(path), ::GetLastError());
            return std::unexpected(error_code::file_open_failed);
        }
        size_ = size_t(size.QuadPart);
        finished = true;
        return {};
    }

    void unmap() noexcept {
        if (address_ != nullptr) ::UnmapViewOfFile(address_);
        if (mapping_ != nullptr) ::CloseHandle(mapping_);
        if (file_ != INVALID_HANDLE_VALUE) ::CloseHandle(file_);
        address_ = nullptr;
        mapping_ = nullptr;
        file_ = INVALID_HANDLE_VALUE;
        size_ = 0;
    }

    HANDLE file_ = INVALID_HANDLE_VALUE;
    HANDLE mapping_ = nullptr;
#else
    [[nodiscard]]
    result<> map_file(fs::path const& path, size_t bytes, bool creating) {
        int const flags = creating ? (O_RDWR | O_CREAT | O_EXCL) : O_RDWR;
        do {
            fd_ = ::open(path.c_str(), flags | O_CLOEXEC, 0644);
        } while (fd_ < 0 && errno == EINTR);
        if (fd_ < 0) {
            auto const why = errno;
            log::error("{}: cannot be {}: {}", path_display(path), creating ? "created" : "opened",
                       std::strerror(why));
            return std::unexpected(creating && why == EEXIST
                ? error_code::identity_collision : error_code::file_open_failed);
        }
        bool finished = false;
        struct withdraw {
            mapped_segment& self; fs::path const& path; bool creating; bool& finished;
            ~withdraw() {
                if (finished) return;
                self.unmap();
                if (creating) { std::error_code ec; fs::remove(path, ec); }
            }
        } const guard{*this, path, creating, finished};

        if (creating) {
            int sized = 0;
            do {
                sized = ::ftruncate(fd_, off_t(bytes));
            } while (sized != 0 && errno == EINTR);
            if (sized != 0) {
                log::error("{}: cannot be sized to {} bytes: {}", path_display(path), bytes,
                           std::strerror(errno));
                return std::unexpected(error_code::file_open_failed);
            }
        } else {
            struct stat st{};
            if (::fstat(fd_, &st) != 0 || st.st_size <= 0) {
                log::error("{}: cannot be sized", path_display(path));
                return std::unexpected(error_code::file_open_failed);
            }
            bytes = size_t(st.st_size);
        }

        void* const mapped = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (mapped == MAP_FAILED) {
            log::error("{}: cannot be mapped: {}", path_display(path), std::strerror(errno));
            return std::unexpected(error_code::file_open_failed);
        }
        address_ = mapped;
        size_ = bytes;
        finished = true;
        return {};
    }

    void unmap() noexcept {
        if (address_ != nullptr) ::munmap(address_, size_);
        if (fd_ >= 0) ::close(fd_);
        address_ = nullptr;
        fd_ = -1;
        size_ = 0;
    }

    int fd_ = -1;
#endif

    void* address_ = nullptr;
    size_t size_ = 0;
};

} // namespace utxoz::detail
