// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file sizing.cpp
 * @brief How many bytes a segment needs to hold a given map, measured.
 *
 * A diagnostic instrument, never installed and never part of the library. It
 * answers one question — "given a class and a target bucket count, how large must
 * the segment be to build and operate that map safely?" — and it answers it the
 * way round that lets a file size be chosen rather than inherited.
 *
 * @par Why it is allowed to see bad_alloc
 * Finding the smallest segment that works means asking segments that do not. That
 * is fine here and nowhere else: this runs offline, by hand or in one CI lane, and
 * nothing it does is on a path a database takes. `configure()` must not learn its
 * sizes this way, which is the reason this exists separately.
 *
 * @par What it reports, and why in three parts
 * A single number from a binary search hides its own assumptions. So the estimate
 * is computed and broken down, the minimum is measured, and the difference between
 * them is reported as overhead rather than smoothed away. A reader can check the
 * arithmetic against the measurement and see which part they are trusting.
 */

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#else
#include <process.h>
#define getpid _getpid
#endif

#include <boost/version.hpp>
#include <fmt/format.h>

#include <utxoz/types.hpp>

#include "detail/format_identity.hpp"
#include "detail/segment_stamp.hpp"
#include "detail/utxo_value.hpp"

namespace fs = std::filesystem;
namespace bip = boost::interprocess;

using namespace utxoz;
using namespace utxoz::detail;

namespace {

// =============================================================================
// The classes, taken from the geometry rather than listed again
// =============================================================================

/// The dispatch below unrolls the geometry by hand, because each class is a
/// distinct type. That is a second place the sizes live, so it is pinned: change
/// `container_sizes` and this stops compiling rather than quietly measuring a
/// geometry that no longer exists.
static_assert(container_sizes.size() == 5,
              "the sizing dispatch enumerates the containers one by one; a class added "
              "or removed has to be added or removed there too");
static_assert(container_sizes == std::array<size_t, 5>{48, 96, 128, 256, 10240},
              "the geometry changed; update the dispatch in tools/sizing.cpp and "
              "re-measure, because every number this tool prints depends on it");

/// One growth step of `unordered_flat_map`: the bucket counts it will actually
/// use are `15·2^k − 1`, and each growth doubles.
constexpr size_t bucket_step(unsigned k) { return (size_t(15) << k) - 1; }

/// Which step a bucket count is, or nothing if it is not one.
std::optional<unsigned> step_of(size_t buckets) {
    for (unsigned k = 0; k < 40; ++k) {
        if (bucket_step(k) == buckets) return k;
    }
    return std::nullopt;
}

// =============================================================================
// One measurement
// =============================================================================

struct sizing_report {
    std::string class_name;
    uint32_t container_kind = 0;
    size_t sizeof_value = 0;
    size_t sizeof_pair = 0;
    size_t asked = 0;
    size_t bucket_count = 0;
    std::optional<unsigned> step;
    double max_load_factor = 0;
    size_t entries_at_max_load = 0;
    size_t rotation_threshold = 0;

    size_t slot_bytes = 0;
    size_t metadata_bytes = 0;
    size_t estimated_overhead = 0;
    size_t estimated_minimum = 0;

    /// The bracket the search actually verified. `lower_bound` was tried and did
    /// not build; `observed_minimum` was tried and did. The true minimum is
    /// between them, and calling the upper end "the minimum" would claim a byte
    /// nothing tested.
    size_t lower_bound = 0;
    bool lower_bound_builds = false;
    size_t observed_minimum = 0;
    bool observed_minimum_builds = false;
    size_t precision = 0;
    size_t attempts = 0;
    size_t measured_overhead = 0;

    size_t margin_bytes = 0;
    size_t recommended = 0;
    /// Non-zero when a size was given rather than recommended.
    size_t segment_override = 0;

    size_t logical_size = 0;
    std::optional<size_t> physical_bytes;
    bool sparse = false;

    size_t next_bucket_count = 0;
    bool next_builds_alone = false;
    /// Measured by asking the map to grow, not inferred from free bytes.
    bool next_coexists_via_rehash = false;
    std::string coexistence_method;

    bool filled = false;
    size_t entries_inserted = 0;
    size_t bucket_count_after_fill = 0;
    bool rehashed_while_filling = false;
    std::optional<size_t> physical_after_fill;

    double seconds = 0;
};

/// A directory that removes itself, whatever leaves the scope.
struct scoped_dir {
    explicit scoped_dir(fs::path base) {
        static uint64_t counter = 0;
        path = std::move(base) / fmt::format("utxoz-sizing-{}-{}", getpid(), counter++);
        std::error_code ec;
        fs::remove_all(path, ec);
        fs::create_directories(path);
    }
    ~scoped_dir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
    scoped_dir(scoped_dir const&) = delete;
    scoped_dir& operator=(scoped_dir const&) = delete;
    fs::path path;
};

/// Blocks actually allocated to the file, where the platform will say.
///
/// Deliberately absent rather than approximated on Windows: reporting the logical
/// size as consumption is worse than reporting nothing, because it reads like a
/// measurement.
std::optional<size_t> physical_bytes_of(fs::path const& p) {
#ifndef _WIN32
    struct stat st{};
    if (::stat(p.string().c_str(), &st) != 0) return std::nullopt;
    return size_t(st.st_blocks) * 512u;
#else
    (void)p;
    return std::nullopt;
#endif
}

/// Anything that is not the segment running out of room.
///
/// A tool that treats every exception as "does not fit" reports a permission
/// error, a full filesystem or a bug as a capacity measurement, and the number it
/// prints looks exactly like a real one.
struct measurement_failed : std::runtime_error {
    using std::runtime_error::runtime_error;
};

template <class Map>
struct built_segment {
    std::unique_ptr<bip::managed_mapped_file> segment;
    Map* map = nullptr;
};

/// The identity a measured segment carries. Fixed, because the point is bytes and
/// a different identity would only make two runs differ.
inline segment_identity sizing_identity(uint32_t kind) {
    database_id_t id{};
    id.fill(0xA5);
    return local_identity(id, kind, 0);
}

/// Builds what production builds, in the order production builds it.
///
/// Not an approximation: the segment carries a stamp before it carries a map, and
/// the map is constructed under the name every reader looks for. Both consume
/// space, and a floor measured without them is a floor for a file the store would
/// never have written. One helper, used by the search, the physical measurement,
/// the fill and the growth probe, so those cannot drift apart.
///
/// Returns nothing when the segment could not hold it. Throws for anything that
/// is not a capacity answer.
template <class Map, class Alloc>
std::optional<built_segment<Map>> build_like_production(fs::path const& file, size_t size,
                                                        size_t asked, uint32_t kind,
                                                        bool fail_stamp = false) {
    std::error_code ec;
    fs::remove(file, ec);

    built_segment<Map> out;
    try {
        out.segment = std::make_unique<bip::managed_mapped_file>(bip::create_only, file.c_str(), size);
    } catch (bip::bad_alloc const&) {
        return std::nullopt;
    } catch (std::exception const& e) {
        throw measurement_failed(fmt::format("creating a {} byte segment: {}", size, e.what()));
    }

    // A stamp that will not go down is not a capacity answer. It is fifty-six
    // bytes placed at the start of a segment already sized for a map thousands of
    // times larger, so "there was no room" is not what a failure here means —
    // and treating it as one would report a permission error or a name collision
    // as a measurement. `nullopt` is reserved for the map, below.
    auto const identity = sizing_identity(kind);
    if (auto const stamped = place_stamp(*out.segment, file, identity); ! stamped) {
        throw measurement_failed(fmt::format("the segment could not be stamped: error {}",
                                             int(stamped.error())));
    }

    // Placed twice on request, so the failure path above is reachable from a test
    // for a reason that has nothing to do with space: the second attempt finds the
    // name taken. Nothing else uses this.
    if (fail_stamp) {
        if (auto const again = place_stamp(*out.segment, file, identity); ! again) {
            throw measurement_failed(fmt::format("the segment could not be stamped: error {}",
                                                 int(again.error())));
        }
    }

    try {
        out.map = out.segment->template construct<Map>(map_object_name, std::nothrow)(
            asked, outpoint_hash{}, outpoint_equal{}, out.segment->template get_allocator<Alloc>());
    } catch (bip::bad_alloc const&) {
        return std::nullopt;
    }
    if (out.map == nullptr) return std::nullopt;

    // And the stamp reads back as the one that was meant. The segment is supposed
    // to be identified as well as sized: a measurement of a segment nothing would
    // accept is a measurement of the wrong thing.
    if (auto const ok = validate_stamp(*out.segment, file, identity); ! ok) {
        throw measurement_failed(fmt::format("the stamp does not read back: error {}",
                                             int(ok.error())));
    }
    return out;
}

/// Does this size hold the map production would have written?
template <class Map, class Alloc>
bool fits_at(fs::path const& dir, size_t size, size_t asked, size_t expected_buckets,
             uint32_t kind) {
    auto const file = dir / "probe.dat";
    bool ok = false;
    {
        auto built = build_like_production<Map, Alloc>(file, size, asked, kind);
        // A segment that built a *different* map has not answered the question.
        ok = built && built->map->bucket_count() == expected_buckets;
    }
    std::error_code ec;
    fs::remove(file, ec);
    return ok;
}

/// Brackets the smallest segment that builds this map.
///
/// Both ends were tried: `lower` did not build and `upper` did. The true minimum
/// is between them and the width is the precision, which is reported — naming the
/// upper end "the minimum" would claim a byte nothing tested.
///
/// Started from the arithmetic rather than from nothing: bisecting the whole
/// plausible range would build a 1.2 GiB table thirty times to learn what one
/// calculation and a handful of trials already say.
struct bracket {
    size_t lower = 0;      // tried, did not build
    size_t upper = 0;      // tried, did build
};

template <class Map, class Alloc>
bracket find_minimum(fs::path const& dir, size_t asked, size_t expected_buckets, uint32_t kind,
                     size_t estimate, size_t precision, size_t& attempts) {
    auto fits = [&](size_t size) {
        ++attempts;
        return fits_at<Map, Alloc>(dir, size, asked, expected_buckets, kind);
    };

    size_t lo = estimate / 2;
    size_t hi = estimate;
    while ( ! fits(hi)) {
        lo = hi;
        hi += hi / 2;
        if (hi > (size_t(1) << 42)) return {};
    }
    while (lo > 0 && fits(lo)) {
        hi = lo;
        lo /= 2;
    }
    if (lo == 0) return {0, hi};

    // At least one byte of width, whatever was asked for. With a precision of
    // zero and a bracket one byte wide, `mid` is `lo`, `lo` is set to `lo`, and
    // the loop never ends — a hang rather than an answer. The command line
    // refuses zero as well; this is so an internal caller cannot reintroduce it.
    size_t const effective = std::max<size_t>(1, precision);
    while (hi - lo > effective) {
        size_t const mid = lo + (hi - lo) / 2;
        if (fits(mid)) hi = mid; else lo = mid;
    }
    return {lo, hi};
}

/// `value * percent / 100`, rounded up and refusing to overflow. The obvious
/// spelling divides first and loses up to a hundred bytes per point; this is a
/// safety margin, so it rounds towards more of it.
size_t margin_of(size_t value, size_t percent) {
    if (percent == 0) return 0;
    if (value > (std::numeric_limits<size_t>::max() - 99) / percent) {
        throw measurement_failed("the margin does not fit in a size_t");
    }
    return (value * percent + 99) / 100;
}

template <size_t ClassSize, class Map, class Alloc>
sizing_report measure(fs::path const& work, std::string class_name, size_t asked,
                      size_t margin_percent, size_t precision, bool fill,
                      std::optional<size_t> segment_override, uint32_t kind, bool fail_stamp) {
    auto const started = std::chrono::steady_clock::now();
    scoped_dir dir(work);

    sizing_report r;
    r.class_name = std::move(class_name);
    r.container_kind = kind;
    r.sizeof_value = ClassSize == 0 ? sizeof(reference_value) : sizeof(utxo_value<ClassSize ? ClassSize : 48>);
    r.sizeof_pair = sizeof(typename Map::value_type);
    r.asked = asked;

    // What Boost will actually give, learned by asking it rather than by trusting
    // the formula — the formula is then checked against the answer below.
    //
    // Asked of a map with the ordinary allocator, because the one in the segment
    // cannot be default-constructed and does not need to be: how many buckets a
    // capacity turns into is the size policy's decision, and the allocator has no
    // part in it. The measurement that follows uses the real one.
    {
        using probe_map = boost::unordered_flat_map<
            typename Map::key_type, typename Map::mapped_type,
            typename Map::hasher, typename Map::key_equal>;
        probe_map probe(asked);
        r.bucket_count = probe.bucket_count();
        r.max_load_factor = probe.max_load_factor();
    }
    r.step = step_of(r.bucket_count);
    r.entries_at_max_load = size_t(double(r.bucket_count) * r.max_load_factor);

    // The threshold the store rotates at: five per cent below the load Boost grows
    // at. Mirrored from can_insert_safely(), and the whole point is that it is
    // below the number above.
    r.rotation_threshold = size_t(double(r.bucket_count) * r.max_load_factor * 0.95);

    r.slot_bytes = r.bucket_count * r.sizeof_pair;
    r.metadata_bytes = (r.bucket_count / 15 + 1) * 16;

    // Named object, stamp, segment manager, allocator bookkeeping and alignment.
    // A starting point for the search, not a claim: the measurement below is what
    // is reported, and the difference is printed as overhead.
    r.estimated_overhead = sizeof(segment_manager_t) + 4096;
    r.estimated_minimum = r.slot_bytes + r.metadata_bytes + r.estimated_overhead;


    auto const found = find_minimum<Map, Alloc>(dir.path, asked, r.bucket_count, kind,
                                                r.estimated_minimum, precision, r.attempts);
    if (found.upper == 0) throw measurement_failed("no segment size builds this map");

    r.lower_bound = found.lower;
    r.observed_minimum = found.upper;
    r.precision = found.upper - found.lower;

    // Both ends re-tried and reported. The claim is not "this is the minimum" but
    // "this size built it and that one did not", which is what was observed.
    r.observed_minimum_builds =
        fits_at<Map, Alloc>(dir.path, r.observed_minimum, asked, r.bucket_count, kind);
    r.lower_bound_builds = found.lower > 0
        && fits_at<Map, Alloc>(dir.path, r.lower_bound, asked, r.bucket_count, kind);
    r.attempts += 2;
    if ( ! r.observed_minimum_builds) throw measurement_failed("the measured floor does not build");
    if (r.lower_bound_builds) throw measurement_failed("the size below the floor built anyway");

    r.measured_overhead = r.observed_minimum > (r.slot_bytes + r.metadata_bytes)
                        ? r.observed_minimum - r.slot_bytes - r.metadata_bytes : 0;

    r.margin_bytes = margin_of(r.observed_minimum, margin_percent);
    r.recommended = r.observed_minimum + r.margin_bytes;

    // A segment size given on the command line replaces the recommendation for
    // everything measured below. That is how the question "does today's file let
    // this map grow behind our back?" gets asked of the file that exists, rather
    // than of the one this tool would have chosen.
    if (segment_override) {
        r.segment_override = *segment_override;
        r.recommended = *segment_override;
    }

    // Built once more at the recommended size, so the physical figures describe a
    // file that really holds the map. A failure here is not a size answer: it
    // would mean reporting on a file with no map in it.
    {
        auto const file = dir.path / "final.dat";
        {
            auto built = build_like_production<Map, Alloc>(file, r.recommended, asked, kind,
                                                          fail_stamp);
            if ( ! built) throw measurement_failed("the recommended size does not hold the map");
            built->segment->flush();
        }
        r.logical_size = fs::exists(file) ? size_t(fs::file_size(file)) : 0;
        r.physical_bytes = physical_bytes_of(file);
        r.sparse = r.physical_bytes && *r.physical_bytes < r.logical_size;
        std::error_code ec;
        fs::remove(file, ec);
    }

    // Building is not operating. A segment can hold the arrays and still run out
    // when the entries arrive, and the number that matters to a file size is the
    // one where the store rotates rather than the one where the map exists. Off by
    // default because it writes every slot up to that threshold.
    if (fill) {
        auto const file = dir.path / "fill.dat";
        std::string reason;
        try {
            auto built = build_like_production<Map, Alloc>(file, r.recommended, asked, kind);
            if ( ! built) throw measurement_failed("the recommended size does not hold the map");
            size_t const before = built->map->bucket_count();
            typename Map::mapped_type value{};
            for (uint64_t i = 0; i < r.rotation_threshold; ++i) {
                raw_outpoint k{};
                std::memcpy(k.data(), &i, sizeof(i));
                built->map->emplace(k, value);
            }
            r.entries_inserted = built->map->size();
            r.bucket_count_after_fill = built->map->bucket_count();
            r.rehashed_while_filling = r.bucket_count_after_fill != before;
            built->segment->flush();
            r.physical_after_fill = physical_bytes_of(file);
            r.filled = true;
        } catch (std::exception const& e) {
            // Asked for and not done is a failed measurement, not a footnote on a
            // successful one: a script that took the exit code for an answer would
            // accept a report that never did what it was told to.
            r.filled = false;
            reason = e.what();
        }
        std::error_code ec;
        fs::remove(file, ec);
        if ( ! r.filled) {
            throw measurement_failed(fmt::format("the fill was asked for and did not finish: {}",
                                                 reason));
        }
    }

    r.next_bucket_count = r.bucket_count * 2 + 1;

    // Can the step above be built at all in the recommended size?
    r.next_builds_alone =
        fits_at<Map, Alloc>(dir.path, r.recommended, r.next_bucket_count, r.next_bucket_count, kind);

    // And can it be reached the way Boost would reach it — by growing the map that
    // is already there? That is what the policy rests on, and it is asked rather
    // than inferred from free bytes: a growth allocates the new arrays before
    // releasing the old, and only a real attempt shows whether the allocator can
    // satisfy that.
    {
        auto const file = dir.path / "grow.dat";
        try {
            auto built = build_like_production<Map, Alloc>(file, r.recommended, asked, kind);
            if ( ! built) throw measurement_failed("the recommended size does not hold the map");

            // Entries first: a rehash of an empty map is the case an
            // implementation is most likely to shortcut, and a shortcut would
            // answer a question nobody asked.
            // Enough to be past any empty-map shortcut, and never past the
            // threshold the store rotates at — filling it here would trigger the
            // very growth this is about to ask for on purpose.
            auto const seed = std::min<size_t>(1000, r.rotation_threshold);
            typename Map::mapped_type value{};
            for (uint64_t i = 0; i < seed; ++i) {
                raw_outpoint k{};
                std::memcpy(k.data(), &i, sizeof(i));
                built->map->emplace(k, value);
            }
            try {
                built->map->rehash(r.next_bucket_count);
                r.next_coexists_via_rehash = built->map->bucket_count() >= r.next_bucket_count;
            } catch (bip::bad_alloc const&) {
                r.next_coexists_via_rehash = false;
            }
            r.coexistence_method = "rehash on a populated map inside the recommended segment";
        } catch (measurement_failed const&) {
            throw;
        } catch (std::exception const& e) {
            throw measurement_failed(fmt::format("measuring coexistence: {}", e.what()));
        }
        std::error_code ec;
        fs::remove(file, ec);
    }

    r.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    return r;
}

// =============================================================================
// Output
// =============================================================================

std::string mib(size_t bytes) { return fmt::format("{:.2f} MiB", double(bytes) / 1048576.0); }

/// A JSON string body.
///
/// Every string this tool emits is either validated or a literal, so nothing here
/// is reachable today. That is a fact about the current callers and not about the
/// writer, and a writer that produces valid JSON only while its inputs behave is
/// one nobody can safely add a field to.
std::string json_string(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (unsigned char const c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) out += fmt::format("\\u{:04x}", unsigned(c));
                else          out += char(c);
        }
    }
    return out;
}

void print_text(std::ostream& out, sizing_report const& r) {
    out << fmt::format("class                    {}\n", r.class_name);
    out << fmt::format("container kind           {}\n", r.container_kind);
    out << fmt::format("sizeof(utxo_value)       {}\n", r.sizeof_value);
    out << fmt::format("sizeof(stored pair)      {}\n", r.sizeof_pair);
    out << fmt::format("capacity asked           {}\n", r.asked);
    out << fmt::format("bucket_count             {}{}\n", r.bucket_count,
                       r.step ? fmt::format("   (step {}: 15·2^{}−1)", *r.step, *r.step) : "");
    out << fmt::format("max_load_factor          {:.6f}\n", r.max_load_factor);
    out << fmt::format("entries at max load      {}\n", r.entries_at_max_load);
    out << fmt::format("rotation threshold       {}\n", r.rotation_threshold);
    out << fmt::format("slot bytes               {}\n", mib(r.slot_bytes));
    out << fmt::format("group metadata           {}\n", mib(r.metadata_bytes));
    out << fmt::format("estimated overhead       {} bytes\n", r.estimated_overhead);
    out << fmt::format("estimated minimum        {}\n", mib(r.estimated_minimum));
    out << fmt::format("verified bracket         {} did not build, {} did\n",
                       mib(r.lower_bound), mib(r.observed_minimum));
    out << fmt::format("upper bound on minimum   {}   (±{} bytes, {} attempts)\n",
                       mib(r.observed_minimum), r.precision, r.attempts);
    out << fmt::format("measured overhead        {} bytes   (below the precision above if 0)\n",
                       r.measured_overhead);
    out << fmt::format("margin                   {}\n", mib(r.margin_bytes));
    out << fmt::format("{:24} {}\n", r.segment_override ? "segment given" : "recommended",
                       mib(r.recommended));
    out << fmt::format("logical size             {}\n", mib(r.logical_size));
    out << fmt::format("physical blocks          {}\n",
                       r.physical_bytes ? mib(*r.physical_bytes) : "unavailable");
    out << fmt::format("sparse                   {}\n",
                       r.physical_bytes ? (r.sparse ? "yes" : "no") : "unknown");
    out << fmt::format("next bucket_count        {}\n", r.next_bucket_count);
    out << fmt::format("next builds alone        {}\n", r.next_builds_alone ? "yes" : "no");
    out << fmt::format("next coexists by growth  {}{}\n",
                       r.next_coexists_via_rehash ? "YES" : "no",
                       r.next_coexists_via_rehash ? "   — a growth could happen in place" : "");
    out << fmt::format("  measured by            {}\n", r.coexistence_method);
    if (r.filled) {
        out << fmt::format("filled to threshold      {} entries\n", r.entries_inserted);
        out << fmt::format("bucket_count after fill  {}{}\n", r.bucket_count_after_fill,
                           r.rehashed_while_filling ? "   — IT REHASHED" : "   (unchanged)");
        out << fmt::format("physical after fill      {}\n",
                           r.physical_after_fill ? mib(*r.physical_after_fill) : "unavailable");
    }
    out << fmt::format("seconds                  {:.1f}\n", r.seconds);
    out << fmt::format("platform_abi_id          {}\n", platform_abi_id);
    out << fmt::format("data_abi_id              {}\n", data_abi_id);
    out << fmt::format("sync_abi_family          {}\n", sync_abi_family);
    out << fmt::format("platform_tag             {}\n", platform_tag);
    out << fmt::format("interprocess_mutex       sizeof {} alignof {}\n",
                       sizeof(bip::interprocess_mutex), alignof(bip::interprocess_mutex));
    out << fmt::format("segment_manager_t        sizeof {} alignof {}\n",
                       sizeof(segment_manager_t), alignof(segment_manager_t));
    out << fmt::format("boost_version            {}\n", BOOST_VERSION);
}

void print_json(std::ostream& out, sizing_report const& r) {
    auto const b = [](bool v) { return v ? "true" : "false"; };
    out << "{\n";
    out << fmt::format("  \"class\": \"{}\",\n", json_string(r.class_name));
    out << fmt::format("  \"container_kind\": {},\n", r.container_kind);
    out << fmt::format("  \"sizeof_value\": {},\n", r.sizeof_value);
    out << fmt::format("  \"sizeof_pair\": {},\n", r.sizeof_pair);
    out << fmt::format("  \"asked\": {},\n", r.asked);
    out << fmt::format("  \"bucket_count\": {},\n", r.bucket_count);
    out << fmt::format("  \"step\": {},\n", r.step ? std::to_string(*r.step) : "null");
    out << fmt::format("  \"max_load_factor\": {:.6f},\n", r.max_load_factor);
    out << fmt::format("  \"entries_at_max_load\": {},\n", r.entries_at_max_load);
    out << fmt::format("  \"rotation_threshold\": {},\n", r.rotation_threshold);
    out << fmt::format("  \"slot_bytes\": {},\n", r.slot_bytes);
    out << fmt::format("  \"metadata_bytes\": {},\n", r.metadata_bytes);
    out << fmt::format("  \"estimated_overhead\": {},\n", r.estimated_overhead);
    out << fmt::format("  \"estimated_minimum\": {},\n", r.estimated_minimum);
    out << fmt::format("  \"lower_bound\": {},\n", r.lower_bound);
    out << fmt::format("  \"lower_bound_builds\": {},\n", b(r.lower_bound_builds));
    out << fmt::format("  \"observed_minimum\": {},\n", r.observed_minimum);
    out << fmt::format("  \"observed_minimum_builds\": {},\n", b(r.observed_minimum_builds));
    out << fmt::format("  \"precision\": {},\n", r.precision);
    out << fmt::format("  \"attempts\": {},\n", r.attempts);
    out << fmt::format("  \"measured_overhead\": {},\n", r.measured_overhead);
    out << fmt::format("  \"margin_bytes\": {},\n", r.margin_bytes);
    out << fmt::format("  \"recommended\": {},\n", r.recommended);
    out << fmt::format("  \"segment_override\": {},\n", r.segment_override);
    out << fmt::format("  \"logical_size\": {},\n", r.logical_size);
    out << fmt::format("  \"physical_bytes\": {},\n",
                       r.physical_bytes ? std::to_string(*r.physical_bytes) : "null");
    out << fmt::format("  \"sparse\": {},\n", r.physical_bytes ? b(r.sparse) : "null");
    out << fmt::format("  \"next_bucket_count\": {},\n", r.next_bucket_count);
    out << fmt::format("  \"next_builds_alone\": {},\n", b(r.next_builds_alone));
    out << fmt::format("  \"next_coexists_via_rehash\": {},\n", b(r.next_coexists_via_rehash));
    out << fmt::format("  \"coexistence_method\": \"{}\",\n", json_string(r.coexistence_method));
    out << fmt::format("  \"filled\": {},\n", b(r.filled));
    out << fmt::format("  \"entries_inserted\": {},\n", r.entries_inserted);
    out << fmt::format("  \"bucket_count_after_fill\": {},\n", r.bucket_count_after_fill);
    out << fmt::format("  \"rehashed_while_filling\": {},\n", b(r.rehashed_while_filling));
    out << fmt::format("  \"physical_after_fill\": {},\n",
                       r.physical_after_fill ? std::to_string(*r.physical_after_fill) : "null");
    out << fmt::format("  \"seconds\": {:.1f},\n", r.seconds);
    out << fmt::format("  \"platform_abi_id\": {},\n", platform_abi_id);
    out << fmt::format("  \"data_abi_id\": {},\n", data_abi_id);
    out << fmt::format("  \"sync_abi_family\": {},\n", sync_abi_family);
    out << fmt::format("  \"platform_tag\": {},\n", platform_tag);
    out << fmt::format("  \"sizeof_interprocess_mutex\": {},\n", sizeof(bip::interprocess_mutex));
    out << fmt::format("  \"alignof_interprocess_mutex\": {},\n", alignof(bip::interprocess_mutex));
    out << fmt::format("  \"sizeof_segment_manager\": {},\n", sizeof(segment_manager_t));
    out << fmt::format("  \"alignof_segment_manager\": {},\n", alignof(segment_manager_t));
    out << fmt::format("  \"boost_version\": {}\n", BOOST_VERSION);
    out << "}\n";
}

// =============================================================================
// Command line
// =============================================================================

[[noreturn]] void usage(int code) {
    std::cerr <<
        "usage: utxoz_sizing --class <48|96|128|256|10240|reference> [options]\n"
        "\n"
        "  --buckets <n>       capacity to ask the map for\n"
        "  --step <k>          ask for step k instead: 15·2^k − 1\n"
        "  --margin <pct>      per cent added to the observed minimum (default 5)\n"
        "  --precision <bytes> how tightly to close on the minimum (default 1048576)\n"
        "  --format <text|json>\n"
        "  --output <file>     write there instead of stdout\n"
        "  --work <dir>        where to put the temporary segments\n"
        "  --fill              also insert up to the rotation threshold, to show the\n"
        "                      map operates rather than merely exists\n"
        "  --segment <bytes>   measure against this segment size instead of the one\n"
        "                      it would recommend — for asking what a file that\n"
        "                      already exists allows\n"
        "  --fail-stamp        stamp twice, so the stamping failure path is reachable\n"
        "                      for a reason that is not lack of space (testing only)\n";
    std::exit(code);
}

size_t to_size(std::string_view s, char const* what) {
    size_t v = 0;
    auto const [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || p != s.data() + s.size()) {
        std::cerr << "not a number for " << what << ": " << s << "\n";
        std::exit(2);
    }
    return v;
}

} // namespace

int main(int argc, char** argv) try {
    std::string cls, format = "text", output, work = fs::temp_directory_path().string();
    std::optional<size_t> buckets, step;
    size_t margin = 5;
    bool fill = false, fail_stamp = false;
    std::optional<size_t> segment_override;
    std::optional<size_t> precision;   // defaulted from the estimate, see below

    for (int i = 1; i < argc; ++i) {
        std::string_view const a = argv[i];
        auto next = [&]() -> std::string_view {
            if (i + 1 >= argc) { std::cerr << "missing value after " << a << "\n"; std::exit(2); }
            return argv[++i];
        };
        if (a == "--class") cls = next();
        else if (a == "--buckets") buckets = to_size(next(), "--buckets");
        else if (a == "--step") step = to_size(next(), "--step");
        else if (a == "--margin") margin = to_size(next(), "--margin");
        else if (a == "--precision") precision = to_size(next(), "--precision");
        else if (a == "--format") format = next();
        else if (a == "--output") output = next();
        else if (a == "--work") work = next();
        else if (a == "--fill") fill = true;
        else if (a == "--segment") segment_override = to_size(next(), "--segment");
        else if (a == "--fail-stamp") fail_stamp = true;
        else if (a == "--help" || a == "-h") usage(0);
        else { std::cerr << "unknown option: " << a << "\n"; usage(2); }
    }

    if (cls.empty()) { std::cerr << "--class is required\n"; usage(2); }
    if (format != "text" && format != "json") { std::cerr << "--format must be text or json\n"; std::exit(2); }
    if (buckets && step) { std::cerr << "--buckets and --step are alternatives\n"; std::exit(2); }
    if (step) {
        if (*step > 39) { std::cerr << "--step is out of range: " << *step << "\n"; std::exit(2); }
        buckets = bucket_step(unsigned(*step));
    }
    if ( ! buckets) { std::cerr << "one of --buckets or --step is required\n"; usage(2); }
    if (*buckets == 0) {
        std::cerr << "--buckets must be greater than zero\n";
        return 2;
    }
    if (precision && *precision == 0) {
        // Zero would leave the search bracketing forever rather than answering.
        std::cerr << "--precision must be greater than zero\n";
        return 2;
    }
    if (segment_override && *segment_override == 0) {
        std::cerr << "--segment must be greater than zero\n";
        return 2;
    }

    fs::path const work_dir = work;
    std::error_code ec;
    fs::create_directories(work_dir, ec);

    sizing_report report;
    auto const run = [&]<size_t S, class M, class A>(uint32_t kind) {
        // Precision relative to the answer rather than absolute: a fixed megabyte
        // is most of a small map and a rounding error on a large one, so it would
        // report a tight bound where it is meaningless and a loose one where it
        // matters. Roughly one part in two thousand, floored so the search cannot
        // chase bytes.
        size_t const p = precision ? *precision
                                   : std::max<size_t>(4096, (*buckets * sizeof(typename M::value_type)) / 2048);
        report = measure<S, M, A>(work_dir, cls, *buckets, margin, p, fill, segment_override,
                                  kind, fail_stamp);
    };

    // The container kind travels with the class, from the same dispatch the sizes
    // come from: a segment stamped as container 0 while holding a class-256 map is
    // identified as something it is not, and a tool that measures such a segment is
    // measuring a file the store would refuse.
    if (cls == "48")            run.operator()<48, utxo_map<48>, std::pair<raw_outpoint const, utxo_value<48>>>(0);
    else if (cls == "96")       run.operator()<96, utxo_map<96>, std::pair<raw_outpoint const, utxo_value<96>>>(1);
    else if (cls == "128")      run.operator()<128, utxo_map<128>, std::pair<raw_outpoint const, utxo_value<128>>>(2);
    else if (cls == "256")      run.operator()<256, utxo_map<256>, std::pair<raw_outpoint const, utxo_value<256>>>(3);
    else if (cls == "10240")    run.operator()<10240, utxo_map<10240>, std::pair<raw_outpoint const, utxo_value<10240>>>(4);
    else if (cls == "reference") run.operator()<0, reference_map_t, std::pair<raw_outpoint const, reference_value>>(reference_container_kind);
    else {
        std::cerr << "no such class: " << cls
                  << " (the geometry is 48, 96, 128, 256, 10240, or reference)\n";
        return 2;
    }

    if (output.empty()) {
        if (format == "json") print_json(std::cout, report); else print_text(std::cout, report);
    } else {
        std::ofstream ofs(output, std::ios::binary | std::ios::trunc);
        if ( ! ofs) { std::cerr << "cannot write " << output << "\n"; return 1; }
        if (format == "json") print_json(ofs, report); else print_text(ofs, report);
        ofs.close();
        if (ofs.fail()) { std::cerr << "failed writing " << output << "\n"; return 1; }
    }
    return 0;
} catch (measurement_failed const& e) {
    // Distinct from a bad command line and from success, so a caller can tell
    // "it does not fit" from "nothing was measured".
    std::cerr << "utxoz_sizing: measurement failed: " << e.what() << "\n";
    return 3;
} catch (std::exception const& e) {
    std::cerr << "utxoz_sizing: " << e.what() << "\n";
    return 1;
}
