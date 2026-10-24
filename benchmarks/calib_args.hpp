#pragma once
// Whole-number command-line values for the calibration harnesses.
//
// std::strtoull alone takes "-1" as a huge value, stops quietly at trailing
// text, reads "abc" as 0 and saturates on overflow. Here the whole token has to
// be digits, convert without overflow, and not exceed the option's maximum.

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <string>

namespace calib {

/// The largest count any of these harnesses takes: far above any real run, and
/// small enough that what the harnesses compute from counts stays in 64 bits -
/// sums of a few counts, and a count times a percentage, which the harness
/// bounds to 100 (2^40 * 100 < 2^47). Two counts multiplied together could
/// overflow; no harness does that.
inline constexpr uint64_t max_count = uint64_t(1) << 40;
/// A seed is any 64-bit value.
inline constexpr uint64_t max_seed = std::numeric_limits<uint64_t>::max();
/// A payload is at most 1 MiB; one is allocated per entry written.
inline constexpr uint64_t max_value_size = uint64_t(1) << 20;

/// The value of text, or nothing if it is not a whole number in [0, max].
inline std::optional<uint64_t> parse_whole(std::string const& text, uint64_t max) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) return std::nullopt;
    errno = 0;
    char* end = nullptr;
    auto const v = std::strtoull(text.c_str(), &end, 10);
    if (errno == ERANGE || end != text.c_str() + text.size() || v > max) return std::nullopt;
    return v;
}

} // namespace calib
