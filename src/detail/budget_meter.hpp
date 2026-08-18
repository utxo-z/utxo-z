// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file budget_meter.hpp
 * @brief A ceiling every proportional allocation passes through, rather than a
 *        sum of the ones somebody remembered to add up.
 *
 * The first version of this budget was arithmetic: the walk computed what it
 * expected to hold and reported it. That is a description, not a limit. Two
 * things follow from the difference, and both are why this exists:
 *
 *  - a second container allocated beside the records would not appear in the
 *    figure, so the report could say the ceiling was honoured while the process
 *    held considerably more. The contract said "budget"; the code delivered an
 *    estimate;
 *  - a reservation known to be too large was refused *after* being described,
 *    which on a real database is the difference between a diagnostic and a
 *    `bad_alloc`.
 *
 * So the meter is asked before memory is taken, and it is the allocator that
 * asks. Anything the walk holds in a `counted_vector` is inside the ceiling by
 * construction, not by anybody's care.
 *
 * ## What this is not
 *
 * It is not a sandbox. Code that calls the global allocator directly is outside
 * it, and no in-process budget can be otherwise. What it changes is which one is
 * the natural thing to write: the walk's storage is a `counted_vector`, the
 * duplicate group can only be built from one, and reaching around that is a
 * deliberate act rather than an oversight.
 *
 * It also does not bound the memory-mapped segments. Those are the operating
 * system's pages, mapped and reclaimed on its terms; see doc/uniqueness.md.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>

#include "detail/census_arithmetic.hpp"

namespace utxoz::detail {

/**
 * @brief The ceiling, and what is currently taken against it.
 *
 * Deliberately not thread-safe and deliberately not atomic. The walk is
 * single-threaded and holds the exclusive claim; an atomic here would suggest a
 * concurrency this operation does not have.
 */
class budget_meter {
public:
    explicit constexpr budget_meter(uint64_t ceiling) noexcept : ceiling_(ceiling) {}

    /// Takes `bytes` against the ceiling, or refuses and takes nothing.
    ///
    /// Refusing is the whole point: the caller does not allocate, so the memory
    /// is never asked of the system. A meter that logged an overrun after the
    /// fact would be the arithmetic this replaces.
    [[nodiscard]] constexpr bool acquire(uint64_t bytes) noexcept {
        // A meter whose books do not balance cannot authorise anything. See
        // release(): once a mismatch is seen, what is actually held is unknown,
        // and handing out more against an unknown figure is the failure this is
        // supposed to prevent.
        if (unbalanced_) {
            ++refusals_;
            return false;
        }
        if (bytes > ceiling_ - outstanding_) {   // ceiling_ >= outstanding_ always
            ++refusals_;
            return false;
        }
        outstanding_ += bytes;
        if (outstanding_ > peak_) peak_ = outstanding_;
        return true;
    }

    /// Gives `bytes` back.
    ///
    /// A release larger than what is outstanding is a defect in the caller, and
    /// it fails **closed**: the meter is marked unbalanced and refuses every
    /// acquisition from then on. An earlier version clamped to zero, which is
    /// worse than useless — it hides the mismatch and then authorises new
    /// allocations against a figure that no longer describes what is held, which
    /// is exactly the overrun the meter exists to prevent.
    ///
    /// It does not terminate: `deallocate` is `noexcept` and calls this, and
    /// bringing the process down inside a destructor to report a bookkeeping bug
    /// is a worse outcome than refusing everything and being asked why.
    constexpr void release(uint64_t bytes) noexcept {
        if (bytes > outstanding_) {
            unbalanced_ = true;
            outstanding_ = 0;
            return;
        }
        outstanding_ -= bytes;
    }

    [[nodiscard]] constexpr uint64_t ceiling() const noexcept { return ceiling_; }
    [[nodiscard]] constexpr uint64_t outstanding() const noexcept { return outstanding_; }
    /// The most that was ever held at once. This is what the report calls the
    /// peak, and it is observed rather than predicted.
    [[nodiscard]] constexpr uint64_t peak() const noexcept { return peak_; }
    [[nodiscard]] constexpr uint64_t refusals() const noexcept { return refusals_; }
    /// False once a release did not match an acquisition. A meter in that state
    /// authorises nothing further.
    [[nodiscard]] constexpr bool balanced() const noexcept { return ! unbalanced_; }

private:
    uint64_t ceiling_ = 0;
    uint64_t outstanding_ = 0;
    uint64_t peak_ = 0;
    uint64_t refusals_ = 0;
    bool unbalanced_ = false;
};

/// What an allocation past the ceiling throws. Its own type and not
/// `std::bad_alloc`: the system was never asked, so nothing ran out. The walk
/// catches it and answers `insufficient_space`.
struct budget_exceeded {};

/**
 * @brief An allocator that has to ask.
 *
 * Holds the meter by pointer, so a `counted_vector` is movable and assignable in
 * the ordinary way. The meter outlives every container that shares it — it is a
 * local of the walk, declared before them.
 */
template <typename T>
class counted_allocator {
public:
    using value_type = T;

    explicit constexpr counted_allocator(budget_meter& meter) noexcept : meter_(&meter) {}

    template <typename U>
    constexpr counted_allocator(counted_allocator<U> const& other) noexcept
        : meter_(other.meter()) {}

    [[nodiscard]] T* allocate(std::size_t n) {
        uint64_t bytes = 0;
        // Checked, because a count times a size is arithmetic on a number that
        // came from a file, and one that wrapped would ask the meter for a
        // plausible small figure and then allocate an enormous one.
        if ( ! checked_mul(uint64_t(n), uint64_t(sizeof(T)), bytes)) throw budget_exceeded{};
        if ( ! meter_->acquire(bytes)) throw budget_exceeded{};
        try {
            return static_cast<T*>(::operator new(std::size_t(bytes)));
        } catch (...) {
            meter_->release(bytes);
            throw;
        }
    }

    void deallocate(T* p, std::size_t n) noexcept {
        ::operator delete(p);
        meter_->release(uint64_t(n) * uint64_t(sizeof(T)));
    }

    [[nodiscard]] constexpr budget_meter* meter() const noexcept { return meter_; }

    template <typename U>
    [[nodiscard]] constexpr bool operator==(counted_allocator<U> const& other) const noexcept {
        return meter_ == other.meter();
    }

private:
    budget_meter* meter_;
};

/// The walk's vocabulary container. Everything it holds proportional to the data
/// is one of these, which is what makes the ceiling structural.
template <typename T>
using counted_vector = std::vector<T, counted_allocator<T>>;

} // namespace utxoz::detail
