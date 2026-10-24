// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file calib_mutation.cpp
 * @brief Insertion and compaction, measured on a private copy of a known image.
 *
 * The read-path harness (calib_historical.cpp) never changes the store it
 * measures, so every sample can share one image. A mutation cannot: the second
 * sample would start where the first left off. So every run here starts by
 * copying an immutable source image into a directory of its own, verifying that
 * the copy is the source - entry for entry, by a digest computed from the rule
 * that built it rather than from the database - and only then starting the
 * clock. Nothing a sample does reaches the source or any other sample.
 *
 * Two metrics, one timed region each:
 *
 *   insert_lifecycle  K new keys, inserted one by one through the public API.
 *                     K is chosen large enough that every container of every
 *                     geometry rotates several times: rotation is part of what
 *                     a shard count changes, so it is part of the workload, not
 *                     an exception to it. The batch timestamps inside the loop
 *                     are diagnostics; the total is the measurement.
 *   insert_batch      the same K keys, payloads and heights, through
 *                     full_db::insert_batch: --batch-size requests per call (a
 *                     prepared phase of several blocks, heights per request),
 *                     --workers W, the library's own threads. The timed region is
 *                     every call, so it includes what the API does inside one:
 *                     routing, the preparation, the threads it starts and the
 *                     join. The requests are built before the clock.
 *   compact           compact_all() on the image after a deterministic deletion
 *                     of half of its original keys. The deletion is setup and is
 *                     not timed.
 *
 * For both insert metrics the harness also reports how the work was actually
 * spread. Inside the timed region there is only a light diagnostic: one clock
 * read per call (or per pause_every inserts) and, for insert_batch, a copy of
 * each call's counters into a vector reserved before the clock. The rest is
 * read outside it: the entries every slot (class and shard)
 * gained, from the census before and after, and the CPU time the process spent
 * over the timed region beside its wall time. For insert_batch it adds what
 * every call returned - the tasks it ran and how many ran on the calling
 * thread - so participation is read from what happened, not from the W asked
 * for. CPU time above wall time says the process ran threads at once; it does
 * not say which threads, and nothing here proves the workers overlapped.
 *
 * Every result is validated outside the timing and a failure is an exit code:
 * every insert stored, every deletion applied, entry counts, and a digest of the
 * full content compared with the digest the rule predicts. Sizes, generations and
 * files before and after are reported from the census.
 *
 * The copy is made with plain reads and writes and fsynced before the clock
 * starts. Not std::filesystem::copy_file: it may use copy_file_range, which on a
 * copy-on-write filesystem can share extents with the source, and then the first
 * write to every page would pay a copy that is not part of the work measured.
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

#include <atomic>
#include <string_view>
#include <thread>
#include <tuple>

#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>

#include <fmt/format.h>

#include "calib_args.hpp"

#include <span>

#include <utxoz/census.hpp>
#include <utxoz/database.hpp>
#include <utxoz/statistics.hpp>
#include <utxoz/logging.hpp>
#include <utxoz/types.hpp>

namespace fs = std::filesystem;

namespace {

constexpr int exit_ok = 0;
constexpr int exit_usage = 2;
constexpr int exit_operation_failed = 3;
constexpr int exit_setup_failed = 4;

// --- the rule the source images were built with (calib_historical.cpp) --------

utxoz::raw_outpoint key_of(uint64_t n) {
    utxoz::raw_outpoint k{};
    uint64_t x = n;
    for (size_t i = 0; i + 8 <= k.size(); i += 8) {
        uint64_t z = (x += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        z ^= z >> 31;
        for (size_t b = 0; b != 8; ++b) k[i + b] = uint8_t(z >> (8 * b));
    }
    for (size_t b = 0; b != 8; ++b) k[b] = uint8_t(n >> (8 * b));
    return k;
}

std::vector<uint8_t> value_for(uint64_t n, size_t size) {
    std::vector<uint8_t> v(size);
    for (size_t i = 0; i != size; ++i) v[i] = uint8_t(i * 7 + 1);
    for (size_t b = 0; b != 8 && b < size; ++b) v[b] = uint8_t(n >> (8 * b));
    return v;
}

constexpr uint32_t height_of(uint64_t n) { return uint32_t(1000 + n / 1000); }

/// New keys start here: past the source population and past the read harness's
/// miss keys (2^40), so a new key is new by construction.
constexpr uint64_t insert_base = 1ull << 41;
constexpr uint32_t insert_height_base = 2'000'000;
constexpr uint32_t insert_height(uint64_t i) { return insert_height_base + uint32_t(i / 1000); }
constexpr uint32_t delete_height = 3'000'000;

// --- an order-independent digest of content -----------------------------------

/// One entry's contribution. Summed over a set, so the order the database
/// iterates in does not matter - two stores with the same entries agree.
uint64_t entry_mix(utxoz::raw_outpoint const& k, uint32_t height,
                   std::span<uint8_t const> data) {
    uint64_t h = 0xcbf29ce484222325ULL;
    auto add = [&](uint8_t b) { h ^= b; h *= 0x100000001b3ULL; };
    for (auto b : k) add(b);
    for (int i = 0; i != 4; ++i) add(uint8_t(height >> (8 * i)));
    add(uint8_t(data.size()));
    for (auto b : data) add(b);
    h ^= h >> 30; h *= 0xBF58476D1CE4E5B9ULL;
    h ^= h >> 27; h *= 0x94D049BB133111EBULL;
    return h ^ (h >> 31);
}

struct content {
    uint64_t entries = 0;
    uint64_t digest = 0;
    bool operator==(content const&) const = default;
};

/// Room for the marks of an insertion run: one per pause_every inserts for
/// insert_lifecycle, one per call for insert_batch, plus the start and the end.
/// Each metric divides only by its own parameter, which main() has checked is
/// positive; the other one may be anything, 0 included.
constexpr uint64_t mark_capacity(bool lifecycle, uint64_t inserts, uint64_t pause_every, uint64_t batch_size) {
    uint64_t const per = lifecycle ? pause_every : batch_size;
    return (per == 0 ? 0 : inserts / per) + 2;
}
static_assert(mark_capacity(true, 1000, 10, 0) == 102);      // lifecycle ignores --batch-size 0
static_assert(mark_capacity(false, 1000, 0, 100) == 12);     // batch ignores --pause-every
static_assert(mark_capacity(true, 1000, 0, 0) == 2);         // never a division by zero

/// The original keys the deletion setup erases: every even n below population.
constexpr uint64_t even_keys_below(uint64_t population) { return (population + 1) / 2; }
static_assert(even_keys_below(0) == 0 && even_keys_below(1) == 1 && even_keys_below(2) == 1);
static_assert(even_keys_below(100000) == 50000 && even_keys_below(100001) == 50001);

/// What the rule says a set of original keys holds.
content rule_original(uint64_t population, size_t value_size, bool odd_only) {
    content c;
    for (uint64_t n = 0; n != population; ++n) {
        if (odd_only && n % 2 == 0) continue;
        auto const v = value_for(n, value_size);
        c.digest += entry_mix(key_of(n), height_of(n), v);
        ++c.entries;
    }
    return c;
}

content rule_inserted(uint64_t k, size_t value_size) {
    content c;
    for (uint64_t i = 0; i != k; ++i) {
        auto const n = insert_base + i;
        auto const v = value_for(n, value_size);
        c.digest += entry_mix(key_of(n), insert_height(i), v);
        ++c.entries;
    }
    return c;
}

bool read_content(utxoz::full_db const& db, content& out) {
    content c;
    auto const r = db.for_each_entry([&](utxoz::raw_outpoint const& k, uint32_t h,
                                         std::span<uint8_t const> d) {
        c.digest += entry_mix(k, h, d);
        ++c.entries;
    });
    if ( ! r) return false;
    out = c;
    return true;
}

// --- the census figures reported before and after ------------------------------

struct layout {
    uint64_t generations_class_48 = 0;
    uint64_t version_files_all = 0;
    uint64_t logical_file_bytes = 0;
    uint64_t entries = 0;
};

bool read_layout(utxoz::full_db const& db, layout& out) {
    auto const c = db.census();
    if ( ! c) return false;
    layout l;
    l.logical_file_bytes = c->logical_file_bytes;
    l.entries = c->entries;
    for (auto const& cl : c->classes) {
        l.version_files_all += cl.generations;
        if (cl.container_size == 48) l.generations_class_48 = cl.generations;
    }
    out = l;
    return true;
}

/// Entries per slot - one per shard of a sharded class, one per other class -
/// in census order. Inserts only add, so the difference before and after is
/// what each slot gained.
struct slot_entries {
    uint32_t cls = 0;
    uint32_t shard = 0;
    uint64_t entries = 0;
};

bool read_slots(utxoz::full_db const& db, std::vector<slot_entries>& out) {
    auto const c = db.census();
    if ( ! c) return false;
    out.clear();
    for (size_t i = 0; i != c->classes.size(); ++i) {
        auto const& cl = c->classes[i];
        if (cl.shards_detail.empty()) out.push_back({uint32_t(i), 0, cl.entries});
        else for (auto const& sh : cl.shards_detail) out.push_back({uint32_t(i), sh.shard, sh.entries});
    }
    return true;
}

uint64_t total_inserts(utxoz::full_db& db) {
    auto const st = db.get_statistics();
    uint64_t n = 0;
    for (auto const& c : st.containers) n += c.total_inserts;
    return n;
}

uint64_t cpu_ns() {
    rusage ru{};
    getrusage(RUSAGE_SELF, &ru);
    auto ns = [](timeval t) { return uint64_t(t.tv_sec) * 1'000'000'000ull + uint64_t(t.tv_usec) * 1000ull; };
    return ns(ru.ru_utime) + ns(ru.ru_stime);
}

std::vector<int> affinity_cpus() {
    cpu_set_t set; CPU_ZERO(&set);
    std::vector<int> v;
    if (sched_getaffinity(0, sizeof set, &set) != 0) return v;
    for (int c = 0; c != CPU_SETSIZE; ++c) if (CPU_ISSET(c, &set)) v.push_back(c);
    return v;
}

// --- the copy -------------------------------------------------------------------

/// The directory claim; rewritten on every open, so never part of an image.
constexpr char const* lock_file = ".utxoz.lock";
constexpr char const* owner_file = ".calibmut-owner";

/// Writes the owner marker - the proof the runners need before they remove a
/// work directory. False if it could not be opened, written in full or closed
/// (a full disk shows up at fclose).
[[nodiscard]] bool write_owner_marker(fs::path const& p, std::string const& body) {
    FILE* f = std::fopen(p.c_str(), "w");
    if ( ! f) return false;
    bool const wrote = std::fwrite(body.data(), 1, body.size(), f) == body.size();
    return std::fclose(f) == 0 && wrote;
}

/// A work directory this process has just created and could not mark. Removed
/// only when nothing but the (partial) marker is in it - fs::remove takes an
/// empty directory and nothing else - and kept otherwise. True if removed.
bool abandon_unmarked(fs::path const& work) {
    std::error_code ec;
    fs::remove(work / owner_file, ec);
    return fs::remove(work, ec) && ! ec;
}

/// write_owner_marker must report a full device and a missing directory, write
/// a marker in full, and abandon_unmarked must remove only an empty directory.
/// 0 when they do.
[[nodiscard]] int selftest_owner_marker() {
    int bad = 0;
    auto fail = [&](std::string_view what) { fmt::println(stderr, "owner marker: {}", what); ++bad; };
    if (fs::exists("/dev/full") && write_owner_marker("/dev/full", "token=x\n")) fail("accepted /dev/full");
    if (write_owner_marker("/nonexistent-utxoz-dir/.calibmut-owner", "token=x\n")) fail("accepted a missing directory");
    std::error_code ec;
    auto const base = fs::temp_directory_path() / fmt::format("utxoz-marker-selftest-{}", std::random_device{}());
    auto const empty = base / "only-marker", full = base / "with-data";
    fs::create_directories(empty, ec); fs::create_directories(full, ec);
    std::string const body = "token=0123456789abcdef\nsource=/x\npid=1\n";
    if ( ! write_owner_marker(empty / owner_file, body)) fail("refused a writable directory");
    if (FILE* f = std::fopen((empty / owner_file).c_str(), "r")) {
        std::string back(body.size() + 1, '\0');
        back.resize(std::fread(back.data(), 1, back.size(), f)); std::fclose(f);
        if (back != body) fail("the marker read back is not what was written");
    } else fail("the marker is not there");
    if ( ! abandon_unmarked(empty) || fs::exists(empty)) fail("an unmarked empty directory was not removed");
    if ( ! write_owner_marker(full / owner_file, body)) fail("refused a writable directory");
    if (FILE* f = std::fopen((full / "data.dat").c_str(), "w")) { std::fputs("x", f); std::fclose(f); }
    if (abandon_unmarked(full) || ! fs::exists(full / "data.dat")) fail("a directory holding data was removed");
    fs::remove_all(base, ec);
    return bad;
}

struct copy_stats {
    uint64_t files = 0;
    uint64_t bytes = 0;
};

bool copy_file_plain(fs::path const& from, fs::path const& to, copy_stats& st) {
    int const in = ::open(from.c_str(), O_RDONLY | O_CLOEXEC);
    if (in < 0) return false;
    int const out = ::open(to.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
    if (out < 0) { ::close(in); return false; }
    std::vector<char> buf(1 << 20);
    bool ok = true;
    for (;;) {
        auto const got = ::read(in, buf.data(), buf.size());
        if (got < 0) { ok = false; break; }
        if (got == 0) break;
        for (ssize_t done = 0; done < got; ) {
            auto const w = ::write(out, buf.data() + done, size_t(got - done));
            if (w <= 0) { ok = false; break; }
            done += w;
        }
        if ( ! ok) break;
        st.bytes += uint64_t(got);
    }
    if (ok && ::fsync(out) != 0) ok = false;
    ::close(out);
    ::close(in);
    if (ok) ++st.files;
    return ok;
}

/// Copies every regular file of the source but the lock, fsyncs, and checks
/// that the copy has the same names and sizes.
bool copy_image(fs::path const& src, fs::path const& dst, copy_stats& st, std::string& why) {
    std::error_code ec;
    std::vector<fs::path> files;
    for (auto const& e : fs::directory_iterator(src, ec)) {
        if ( ! e.is_regular_file()) continue;
        if (e.path().filename() == lock_file) continue;
        files.push_back(e.path());
    }
    if (ec) { why = "could not list the source"; return false; }
    for (auto const& f : files) {
        if ( ! copy_file_plain(f, dst / f.filename(), st)) {
            why = fmt::format("could not copy {}", f.filename().string());
            return false;
        }
        if (fs::file_size(f) != fs::file_size(dst / f.filename())) {
            why = fmt::format("{} copied to a different size", f.filename().string());
            return false;
        }
    }
    int const d = ::open(dst.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (d >= 0) { ::fsync(d); ::close(d); }
    return true;
}

// --- shape.json, as calib_historical.cpp writes it --------------------------------

struct shape {
    uint64_t population = 0;
    uint64_t shard_count = 0;
    uint64_t geometry_id = 0;
    uint64_t value_size = 0;
};

bool read_shape(fs::path const& p, shape& s) {
    FILE* f = std::fopen(p.c_str(), "r");
    if ( ! f) return false;
    std::string text;
    char buf[4096];
    while (auto got = std::fread(buf, 1, sizeof buf, f)) text.append(buf, got);
    std::fclose(f);
    if (text.find("\"utxoz-hist-shape-2\"") == std::string::npos) return false;
    auto field = [&](char const* name, uint64_t& dst) {
        auto const at = text.find(std::string("\"") + name + "\":");
        if (at == std::string::npos) return false;
        dst = std::strtoull(text.c_str() + at + std::strlen(name) + 3, nullptr, 10);
        return true;
    };
    return field("population", s.population) && field("shard_count", s.shard_count)
        && field("geometry_id", s.geometry_id) && field("value_size", s.value_size);
}

// --- compaction observation (diagnostic only) -----------------------------------
//
// compact_all() logs one line when it starts, one per container as that
// container's compaction STARTS ("Starting compaction for class C shard S..."),
// and one when it is complete. Nothing is logged when a container finishes.
// So the interval from one container's mark to the next mark covers that
// container's whole compact_container() - close, merges, reopen - plus the
// return, the loop step, and the formatting and dispatch of every message in
// between. The last container's interval ends at the completion line and so
// also covers the file-cache clear that follows the loop. These are intervals
// between marks, with that overhead in them; they are not exact durations.

enum class mark_kind : uint8_t { begin, container, end };

struct mark {
    mark_kind kind = mark_kind::begin;
    uint32_t cls = 0;
    uint32_t shard = 0;
    uint64_t t_ns = 0;          ///< since the observation started
    uint64_t rss_pages = 0;     ///< /proc/self/statm resident, at the mark
};

constexpr std::string_view begin_text = "Starting full database compaction...";
constexpr std::string_view end_text = "Full database compaction complete";
constexpr std::string_view container_prefix = "Starting compaction for class ";

/// 1 = a mark, parsed; 0 = not a mark; -1 = looks like a mark and does not parse.
/// No allocation: this runs inside the library's logging call.
int parse_mark(std::string_view msg, mark& m) {
    if (msg == begin_text) { m.kind = mark_kind::begin; return 1; }
    if (msg == end_text) { m.kind = mark_kind::end; return 1; }
    if ( ! msg.starts_with("Starting compaction")) return 0;
    if ( ! msg.starts_with(container_prefix)) return -1;
    auto rest = msg.substr(container_prefix.size());
    auto number = [&](uint32_t& out) {
        size_t i = 0; uint64_t v = 0;
        while (i < rest.size() && rest[i] >= '0' && rest[i] <= '9' && i < 10) v = v * 10 + uint64_t(rest[i++] - '0');
        if (i == 0) return false;
        out = uint32_t(v); rest = rest.substr(i); return true;
    };
    if ( ! number(m.cls) || ! rest.starts_with(" shard ")) return -1;
    rest = rest.substr(7);
    if ( ! number(m.shard) || rest != "...") return -1;
    m.kind = mark_kind::container;
    return 1;
}

/// The order compact_all() walks: every class, and every shard of it.
std::vector<std::pair<uint32_t, uint32_t>> expected_marks() {
    std::vector<std::pair<uint32_t, uint32_t>> e;
    for (uint32_t c = 0; c != utxoz::container_sizes.size(); ++c)
        for (uint32_t s = 0; s != (c == 0 ? utxoz::shard_count : 1u); ++s) e.emplace_back(c, s);
    return e;
}

/// Empty if the marks are exactly one begin, the expected containers in order,
/// one end, with clocks that never go back; otherwise why not.
std::string validate_marks(std::vector<mark> const& ms,
                           std::vector<std::pair<uint32_t, uint32_t>> const& want,
                           uint64_t incompatible) {
    if (incompatible != 0)
        return fmt::format("{} messages looked like compaction marks and did not parse, "
                           "or did not fit; the log is not the one this harness reads", incompatible);
    if (ms.size() != want.size() + 2)
        return fmt::format("{} marks, not the {} expected", ms.size(), want.size() + 2);
    if (ms.front().kind != mark_kind::begin) return "the first mark is not the start of compact_all";
    if (ms.back().kind != mark_kind::end) return "the last mark is not the completion of compact_all";
    for (size_t i = 0; i != want.size(); ++i) {
        auto const& m = ms[i + 1];
        if (m.kind != mark_kind::container || m.cls != want[i].first || m.shard != want[i].second)
            return fmt::format("mark {} is not class {} shard {}", i + 1, want[i].first, want[i].second);
    }
    for (size_t i = 1; i != ms.size(); ++i)
        if (ms[i].t_ns < ms[i - 1].t_ns) return fmt::format("mark {} goes back in time", i);
    return {};
}

int selftest_marks() {
    int bad = 0;
    auto want = std::vector<std::pair<uint32_t, uint32_t>>{{0, 0}, {0, 1}, {1, 0}};
    auto seq = [](std::initializer_list<std::tuple<mark_kind, uint32_t, uint32_t, uint64_t>> l) {
        std::vector<mark> v;
        for (auto [k, c, s, t] : l) v.push_back(mark{k, c, s, t, 0});
        return v;
    };
    using enum mark_kind;
    auto check = [&](char const* what, bool want_ok, std::vector<mark> const& v, uint64_t inc = 0) {
        bool const ok = validate_marks(v, want, inc).empty();
        if (ok != want_ok) { fmt::println(stderr, "marks: {} misjudged", what); ++bad; }
        else fmt::println("  marks {:<44} ok", what);
    };
    check("a complete sequence", true, seq({{begin,0,0,0},{container,0,0,1},{container,0,1,2},{container,1,0,3},{end,0,0,4}}));
    check("a missing container", false, seq({{begin,0,0,0},{container,0,0,1},{container,1,0,3},{end,0,0,4}}));
    check("a duplicated container", false, seq({{begin,0,0,0},{container,0,0,1},{container,0,0,1},{container,0,1,2},{container,1,0,3},{end,0,0,4}}));
    check("containers out of order", false, seq({{begin,0,0,0},{container,0,1,1},{container,0,0,2},{container,1,0,3},{end,0,0,4}}));
    check("no completion mark", false, seq({{begin,0,0,0},{container,0,0,1},{container,0,1,2},{container,1,0,3}}));
    check("clocks going back", false, seq({{begin,0,0,0},{container,0,0,5},{container,0,1,2},{container,1,0,3},{end,0,0,4}}));
    check("an incompatible message", false, seq({{begin,0,0,0},{container,0,0,1},{container,0,1,2},{container,1,0,3},{end,0,0,4}}), 1);
    mark m;
    auto parses = [&](char const* what, std::string_view msg, int want_r) {
        int const r = parse_mark(msg, m);
        if (r != want_r) { fmt::println(stderr, "parse: {} gave {}", what, r); ++bad; }
        else fmt::println("  parse {:<44} ok", what);
    };
    parses("the container mark", "Starting compaction for class 0 shard 7...", 1);
    parses("an unrelated message", "compaction: x filled early", 0);
    parses("a reworded container mark", "Starting compaction of class 0 shard 7", -1);
    parses("a mark with no shard", "Starting compaction for class 0...", -1);
    return bad;
}

/// Reads the resident page count of this process without allocating.
uint64_t resident_pages(int statm_fd) {
    char buf[128];
    auto const n = ::pread(statm_fd, buf, sizeof buf - 1, 0);
    if (n <= 0) return 0;
    buf[n] = 0;
    char* p = buf;
    std::strtoull(p, &p, 10);                // size
    return std::strtoull(p, nullptr, 10);    // resident
}

uint64_t status_kb(char const* field) {
    FILE* f = std::fopen("/proc/self/status", "r");
    if ( ! f) return 0;
    char line[256];
    uint64_t v = 0;
    auto const len = std::strlen(field);
    while (std::fgets(line, sizeof line, f))
        if (std::strncmp(line, field, len) == 0) { v = std::strtoull(line + len, nullptr, 10); break; }
    std::fclose(f);
    return v;
}

/// Samples the bytes the work directory occupies, on its own thread, at a fixed
/// interval. A transient shorter than the interval can be missed, so the peak it
/// reports is the largest value it saw, not the largest there was.
struct disk_sampler {
    fs::path dir;
    std::chrono::microseconds interval{10000};
    int cpu = -1;
    std::atomic<bool> stop{false};
    uint64_t samples = 0, alloc_peak = 0, logical_peak = 0, alloc_first = 0, alloc_last = 0;
    uint64_t peak_t_ns = 0;
    std::chrono::steady_clock::time_point t0{};
    /// pthread_setaffinity_np's result when a CPU was asked for. Written by the
    /// sampling thread, read only after it is joined.
    int pin_error = 0;

    void sample() {
        uint64_t alloc = 0, logical = 0;
        std::error_code ec;
        for (fs::directory_iterator it(dir, ec), end; ! ec && it != end; it.increment(ec)) {
            struct stat st{};
            if (::lstat(it->path().c_str(), &st) != 0 || ! S_ISREG(st.st_mode)) continue;
            alloc += uint64_t(st.st_blocks) * 512;
            logical += uint64_t(st.st_size);
        }
        if (samples == 0) alloc_first = alloc;
        alloc_last = alloc;
        if (alloc > alloc_peak) {
            alloc_peak = alloc;
            peak_t_ns = uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - t0).count());
        }
        logical_peak = std::max(logical_peak, logical);
        ++samples;
    }
    void run() {
        if (cpu >= 0) {
            cpu_set_t set; CPU_ZERO(&set); CPU_SET(cpu, &set);
            pin_error = pthread_setaffinity_np(pthread_self(), sizeof set, &set);
        }
        while ( ! stop.load(std::memory_order_relaxed)) {
            sample();
            std::this_thread::sleep_for(interval);
        }
        sample();
    }
};

// --- options -------------------------------------------------------------------

struct options {
    std::string metric;
    std::string source;
    std::string scratch_root;
    uint64_t inserts = 1'000'000;
    uint64_t pause_every = 10'000;
    uint64_t delete_batch = 100'000;
    uint64_t seed = 0;                  ///< recorded only: the workload has no randomness
    bool selftest_insert_existing = false;
    bool selftest_expect_wrong = false;
    bool observe = false;               ///< compact: per-container marks, disk and RSS peaks
    uint64_t sample_us = 10000;         ///< disk sampler interval
    int64_t sampler_cpu = -1;           ///< the disk sampler's CPU; -1 = wherever it lands
    bool skip_delete_setup = false;     ///< tests only: compact an image nothing was deleted from
    uint64_t workers = 1;               ///< insert_batch: insert_batch_options::workers
    uint64_t batch_size = 100'000;      ///< insert_batch: requests per call
};

int usage(FILE* to) {
    std::fputs(
        "usage: utxoz_calib_mut --metric insert_lifecycle|insert_batch|compact --source DIR\n"
        "                       --scratch-root DIR [options]\n"
        "\n"
        "  --source DIR        an image built by utxoz_calib_hist --build-fixture;\n"
        "                      read, never written\n"
        "  --scratch-root DIR  where this run creates its own new work directory\n"
        "  --inserts K         insert_lifecycle, insert_batch: new keys (default 1000000)\n"
        "  --workers W         insert_batch: workers per call (default 1); refused above\n"
        "                      the CPUs this process may run on\n"
        "  --batch-size B      insert_batch: requests per call (default 100000); the\n"
        "                      batch-time diagnostics take one mark per call\n"
        "  --pause-every N     insert_lifecycle: a timestamp every N inserts, for the\n"
        "                      batch-time diagnostics (default 10000)\n"
        "  --delete-batch N    compact: deletions per apply_deletes call in the\n"
        "                      untimed setup (default 100000)\n"
        "  --seed N            recorded; the workload is deterministic\n"
        "  --selftest-insert-existing  make the first insert an existing key; a\n"
        "                      correct harness reports it and exits 3\n"
        "  --observe-compaction  compact: record a mark per container from the public\n"
        "                      log callback, sample the work directory's bytes and read\n"
        "                      the RSS high-water mark. Diagnostic: not the primary\n"
        "  --sample-us N       disk sampler interval in microseconds (default 10000)\n"
        "  --sampler-cpu N     pin the disk sampler thread to CPU N\n"
        "  --skip-delete-setup compact without deleting first (tests: nothing to merge)\n"
        "  --selftest-marks    test the mark parser and validator and exit\n"
        "  --selftest-owner-marker  test the owner-marker write and its cleanup, and exit\n"
        "  --selftest-expect-wrong     expect one entry more than the rule says; a\n"
        "                      correct harness reports it and exits 3\n",
        to);
    return to == stdout ? exit_ok : exit_usage;
}

uint64_t maxrss_kb() {
    rusage ru{};
    getrusage(RUSAGE_SELF, &ru);
    return uint64_t(ru.ru_maxrss);
}

uint64_t pct(std::vector<uint64_t> v, double q) {
    if (v.empty()) return 0;
    std::ranges::sort(v);
    auto const i = size_t(q * double(v.size() - 1) + 0.5);
    return v[std::min(i, v.size() - 1)];
}

}  // namespace

int main(int argc, char** argv) {
    options o;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        // The whole value, digits only, in range; a missing or malformed one
        // is a usage error before any work (calib_args.hpp).
        auto next = [&](uint64_t& dst, uint64_t max = calib::max_count) {
            if (i + 1 >= argc) { fmt::println(stderr, "{} needs a value", a); std::exit(exit_usage); }
            auto const v = calib::parse_whole(argv[++i], max);
            if ( ! v) {
                fmt::println(stderr, "{} must be a whole number from 0 to {}, not '{}'", a, max, argv[i]);
                std::exit(exit_usage);
            }
            dst = *v;
        };
        if (a == "--metric" && i + 1 < argc) o.metric = argv[++i];
        else if (a == "--source" && i + 1 < argc) o.source = argv[++i];
        else if (a == "--scratch-root" && i + 1 < argc) o.scratch_root = argv[++i];
        else if (a == "--inserts") next(o.inserts);
        else if (a == "--pause-every") next(o.pause_every);
        else if (a == "--delete-batch") next(o.delete_batch);
        else if (a == "--seed") next(o.seed, calib::max_seed);
        else if (a == "--selftest-insert-existing") o.selftest_insert_existing = true;
        else if (a == "--selftest-expect-wrong") o.selftest_expect_wrong = true;
        else if (a == "--observe-compaction") o.observe = true;
        else if (a == "--sample-us") next(o.sample_us);
        else if (a == "--sampler-cpu") {
            // Digits only: through strtoull "-1" or an overflow would wrap to -1,
            // which means "do not pin", and text would read as CPU 0.
            std::string const text = i + 1 < argc ? argv[++i] : "";
            if (text.empty() || text.size() > 5 || text.find_first_not_of("0123456789") != std::string::npos
                    || std::stoul(text) >= CPU_SETSIZE) {
                fmt::println(stderr, "--sampler-cpu must be a CPU number, not '{}'", text);
                return exit_usage;
            }
            o.sampler_cpu = int64_t(std::stoul(text));
        }
        else if (a == "--skip-delete-setup") o.skip_delete_setup = true;
        else if (a == "--workers") next(o.workers);
        else if (a == "--batch-size") next(o.batch_size);
        else if (a == "--selftest-marks") return selftest_marks() == 0 ? exit_ok : exit_operation_failed;
        else if (a == "--selftest-owner-marker") return selftest_owner_marker() == 0 ? exit_ok : exit_operation_failed;
        else if (a == "--help" || a == "-h") return usage(stdout);
        else { fmt::println(stderr, "unknown argument {}", a); return usage(stderr); }
    }
    if ((o.observe || o.skip_delete_setup) && o.metric != "compact") {
        fmt::println(stderr, "--observe-compaction and --skip-delete-setup apply to compact only");
        return exit_usage;
    }
    if (o.observe && o.sample_us == 0) {
        fmt::println(stderr, "--sample-us must be positive"); return exit_usage;
    }
    if (o.metric != "insert_lifecycle" && o.metric != "insert_batch" && o.metric != "compact") {
        fmt::println(stderr, "--metric must be insert_lifecycle, insert_batch or compact"); return exit_usage;
    }
    bool const inserting = o.metric != "compact";
    auto const cpus = affinity_cpus();
    // Before any work: a CPU outside the set, or outside the ones this process may
    // run on, cannot be pinned, and the report would name an isolation that did
    // not happen. A failure that slips past this is caught after the run.
    if (o.sampler_cpu >= 0) {
        if (o.sampler_cpu >= CPU_SETSIZE
                || std::ranges::find(cpus, int(o.sampler_cpu)) == cpus.end()) {
            fmt::println(stderr, "--sampler-cpu {} is not one of the CPUs this process may run on",
                         o.sampler_cpu);
            return exit_usage;
        }
    }
    if (o.metric == "insert_batch") {
        if (o.workers == 0 || o.batch_size == 0) {
            fmt::println(stderr, "--workers and --batch-size must be positive"); return exit_usage;
        }
        if (o.workers > cpus.size()) {
            fmt::println(stderr, "--workers {} but this process may run on {} CPUs", o.workers, cpus.size());
            return exit_usage;
        }
    }
    if (o.source.empty() || o.scratch_root.empty()) {
        fmt::println(stderr, "--source and --scratch-root are required"); return exit_usage;
    }
    if (o.inserts == 0 || o.pause_every == 0 || o.delete_batch == 0) {
        fmt::println(stderr, "--inserts, --pause-every and --delete-batch must be positive");
        return exit_usage;
    }

    fs::path const source = fs::absolute(o.source);
    fs::path const root = fs::absolute(o.scratch_root);
    shape s;
    if ( ! read_shape(source / "shape.json", s)) {
        fmt::println(stderr, "{} has no readable shape.json (utxoz-hist-shape-2)", source.string());
        return exit_setup_failed;
    }
    if (s.shard_count != utxoz::shard_count) {
        fmt::println(stderr, "the source was built by an N={} binary and this one is N={}",
                     s.shard_count, utxoz::shard_count);
        return exit_usage;
    }
    if ( ! fs::is_directory(root)) {
        fmt::println(stderr, "--scratch-root {} is not a directory", root.string());
        return exit_usage;
    }

    // ------------------------------------------------- a work directory of its own
    // Never an inherited one: the name carries fresh entropy and an existing
    // directory is refused rather than reused or cleared.
    std::random_device rd;
    auto const token = fmt::format("{:016x}", (uint64_t(rd()) << 32) ^ uint64_t(rd()));
    fs::path const work = root / fmt::format("calibmut_{}_n{}_{}_{}", o.metric,
                                             utxoz::shard_count, getpid(), token);
    std::error_code ec;
    if (fs::exists(work) || ! fs::create_directory(work, ec) || ec) {
        fmt::println(stderr, "could not create a new work directory {}", work.string());
        return exit_setup_failed;
    }
    // Without its marker no runner may ever remove this directory, so a copy
    // into it could only pile up: stop here instead.
    if ( ! write_owner_marker(work / owner_file,
                              fmt::format("token={}\nsource={}\npid={}\n", token, source.string(), getpid()))) {
        bool const removed = abandon_unmarked(work);
        fmt::println(stderr, "could not write the owner marker in {}; {}", work.string(),
                     removed ? "the empty work directory was removed" : "the work directory is kept");
        return exit_setup_failed;
    }

    auto const t_setup = std::chrono::steady_clock::now();
    copy_stats cs;
    std::string why_copy;
    if ( ! copy_image(source, work, cs, why_copy)) {
        fmt::println(stderr, "{}; the work directory {} is kept", why_copy, work.string());
        return exit_setup_failed;
    }
    auto const copy_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t_setup).count();

    auto opened = utxoz::full_db::open_for_testing(work.string(), false);
    if ( ! opened) {
        fmt::println(stderr, "could not open the copy: error_code={}", int(opened.error()));
        return exit_setup_failed;
    }
    auto db = std::move(*opened);

    // The copy is the source, entry for entry, before anything is timed. The
    // expected digest comes from the rule, not from reading the source: a copy of
    // an image some earlier run had mutated would fail here.
    auto const expected_initial = rule_original(s.population, s.value_size, false);
    content initial;
    if ( ! read_content(db, initial)) {
        fmt::println(stderr, "could not read the copy's content"); return exit_setup_failed;
    }
    if (initial != expected_initial) {
        fmt::println(stderr, "the copy holds {} entries with digest {:016x}; the rule says {} "
                     "and {:016x}. The source is not the image it claims to be.",
                     initial.entries, initial.digest, expected_initial.entries,
                     expected_initial.digest);
        return exit_setup_failed;
    }

    bool valid = true;
    std::string why;
    uint64_t elapsed_ns = 0;
    uint64_t failed_ops = 0;
    uint64_t setup_ops = 0;
    std::vector<uint64_t> batch_ns;
    layout before{}, after{};
    content expected_final{}, final_content{};
    auto rotations = [&] { return db.get_statistics().rotations_per_container[0]; };
    uint64_t rotations_before = 0, rotations_after = 0;
    std::vector<slot_entries> slots_before, slots_after;
    uint64_t inserts_stat_before = 0, inserts_stat_after = 0, cpu_elapsed_ns = 0;
    struct {
        uint64_t calls = 0, tasks = 0, inline_tasks = 0, workers_min = 0, workers_max = 0;
        uint64_t calls_short_of_threads = 0, inserted = 0, duplicates = 0, failed = 0, not_applied = 0;
        uint64_t calls_with_error = 0;
    } batch;
    struct {
        std::vector<mark> marks;
        uint64_t other_messages = 0, incompatible = 0, rss_before_kb = 0, hwm_kb = 0;
        bool hwm_available = false;
        int statm_fd = -1;
        std::chrono::steady_clock::time_point t0{};
        std::string why;
    } obs;
    disk_sampler sampler;

    if (inserting) {
        if ( ! read_layout(db, before) || ! read_slots(db, slots_before)) {
            fmt::println(stderr, "census failed"); return exit_setup_failed;
        }
        rotations_before = rotations();
        inserts_stat_before = total_inserts(db);
        // Payloads built before the clock, so the timed loop is the inserts.
        std::vector<utxoz::raw_outpoint> keys(o.inserts);
        std::vector<std::vector<uint8_t>> values(o.inserts);
        for (uint64_t i = 0; i != o.inserts; ++i) {
            keys[i] = key_of(insert_base + i);
            values[i] = value_for(insert_base + i, s.value_size);
        }
        if (o.selftest_insert_existing) keys[0] = key_of(0);
        std::vector<std::chrono::steady_clock::time_point> marks;
        marks.reserve(mark_capacity(o.metric == "insert_lifecycle", o.inserts, o.pause_every, o.batch_size));

        if (o.metric == "insert_lifecycle") {
            auto const c0 = cpu_ns();
            auto const t0 = std::chrono::steady_clock::now();
            marks.push_back(t0);
            for (uint64_t i = 0; i != o.inserts; ++i) {
                auto const r = db.insert(keys[i], utxoz::output_data_span{values[i]}, insert_height(i));
                if ( ! r || ! *r) ++failed_ops;
                if ((i + 1) % o.pause_every == 0) marks.push_back(std::chrono::steady_clock::now());
            }
            auto const t1 = std::chrono::steady_clock::now();
            cpu_elapsed_ns = cpu_ns() - c0;
            elapsed_ns = uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        } else {
            // The same keys, payloads and heights, as requests. Built before the
            // clock; each call is one prepared phase of batch_size requests.
            std::vector<utxoz::insert_request> requests(o.inserts);
            for (uint64_t i = 0; i != o.inserts; ++i)
                requests[i] = {keys[i], utxoz::output_data_span{values[i]}, insert_height(i)};
            struct call_record { unsigned workers_used; size_t inline_tasks; size_t inserted, duplicates,
                                 failed, not_applied; bool error; };
            std::vector<call_record> calls;
            calls.reserve(o.inserts / o.batch_size + 2);
            utxoz::insert_batch_options const opts{.workers = unsigned(o.workers)};
            std::span<utxoz::insert_request const> const all(requests);

            auto const c0 = cpu_ns();
            auto const t0 = std::chrono::steady_clock::now();
            marks.push_back(t0);
            for (uint64_t at = 0; at < o.inserts; at += o.batch_size) {
                auto const r = db.insert_batch(all.subspan(at, std::min(o.batch_size, o.inserts - at)), opts);
                calls.push_back({r.workers_used, r.tasks_run_inline, r.inserted, r.duplicates, r.failed,
                                 r.not_applied, r.error.has_value()});
                marks.push_back(std::chrono::steady_clock::now());
            }
            auto const t1 = std::chrono::steady_clock::now();
            cpu_elapsed_ns = cpu_ns() - c0;
            elapsed_ns = uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

            // What the calls say, read after the clock. With the library's own
            // threads a call that used several workers runs exactly one task on
            // the calling thread; more means a thread was not started.
            batch.workers_min = calls.empty() ? 0 : calls.front().workers_used;
            for (auto const& c : calls) {
                ++batch.calls;
                batch.tasks += c.workers_used;
                batch.inline_tasks += c.inline_tasks;
                batch.workers_min = std::min<uint64_t>(batch.workers_min, c.workers_used);
                batch.workers_max = std::max<uint64_t>(batch.workers_max, c.workers_used);
                if (c.workers_used > 1 && c.inline_tasks != 1) ++batch.calls_short_of_threads;
                batch.inserted += c.inserted; batch.duplicates += c.duplicates;
                batch.failed += c.failed; batch.not_applied += c.not_applied;
                if (c.error) ++batch.calls_with_error;
            }
            failed_ops = o.inserts - batch.inserted;
        }
        for (size_t b = 1; b < marks.size(); ++b)
            batch_ns.push_back(uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
                marks[b] - marks[b - 1]).count()));

        rotations_after = rotations();
        inserts_stat_after = total_inserts(db);
        if ( ! read_slots(db, slots_after)) { valid = false; why += "the final census failed; "; }
        expected_final = expected_initial;
        auto const ins = rule_inserted(o.inserts, s.value_size);
        expected_final.entries += ins.entries;
        expected_final.digest += ins.digest;
    } else {
        // Setup, untimed: delete the even original keys, every one of them, and
        // prove it before the clock starts.
        std::vector<utxoz::deferred_deletion_entry> batch;
        uint64_t erased = 0;
        for (uint64_t start = 0; ! o.skip_delete_setup && start < s.population;
             start += o.delete_batch * 2) {
            batch.clear();
            for (uint64_t n = start; n < std::min(s.population, start + o.delete_batch * 2); n += 2)
                batch.emplace_back(key_of(n), delete_height);
            auto const p = db.apply_deletes(batch);
            if (p.error || ! p.absent.empty() || ! p.unresolved.empty()) {
                fmt::println(stderr, "the deletion setup did not complete: erased={} absent={} "
                             "unresolved={} error={}", p.erased.size(), p.absent.size(),
                             p.unresolved.size(), p.error ? int(*p.error) : -1);
                return exit_setup_failed;
            }
            erased += p.erased.size();
        }
        setup_ops = erased;
        auto const expected_after_delete = rule_original(s.population, s.value_size,
                                                         ! o.skip_delete_setup);
        content after_delete;
        if (erased != (o.skip_delete_setup ? 0 : even_keys_below(s.population)) || ! read_content(db, after_delete)
                || after_delete != expected_after_delete) {
            fmt::println(stderr, "after the deletion setup the content is not the odd keys");
            return exit_setup_failed;
        }
        if ( ! read_layout(db, before)) { fmt::println(stderr, "census failed"); return exit_setup_failed; }
        rotations_before = rotations();

        // Observation, when asked for: armed before the clock, disarmed after it,
        // and in its own runs - the callback and the sampler are work the primary
        // does not have.
        if (o.observe) {
            obs.statm_fd = ::open("/proc/self/statm", O_RDONLY | O_CLOEXEC);
            obs.marks.reserve(4096);
            obs.rss_before_kb = status_kb("VmRSS:");
            // Reset the kernel's high-water mark to the current RSS, so the next
            // reading is the peak over compact_all and not over the whole process.
            if (FILE* f = std::fopen("/proc/self/clear_refs", "w")) {
                obs.hwm_available = std::fputs("5", f) >= 0;
                if (std::fclose(f) != 0) obs.hwm_available = false;
            }
            sampler.dir = work;
            sampler.interval = std::chrono::microseconds(o.sample_us);
            sampler.cpu = int(o.sampler_cpu);
        }
        auto const t0 = std::chrono::steady_clock::now();
        std::thread sampling;
        if (o.observe) {
            obs.t0 = t0; sampler.t0 = t0;
            sampling = std::thread([&] { sampler.run(); });
            utxoz::set_log_callback([&](utxoz::log_level, std::string_view msg) {
                mark m{};
                int const r = parse_mark(msg, m);
                if (r == 0) { ++obs.other_messages; return; }
                if (r < 0 || obs.marks.size() == obs.marks.capacity()) { ++obs.incompatible; return; }
                m.t_ns = uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - obs.t0).count());
                m.rss_pages = obs.statm_fd >= 0 ? resident_pages(obs.statm_fd) : 0;
                obs.marks.push_back(m);
            });
        }
        auto const r = db.compact_all();
        auto const t1 = std::chrono::steady_clock::now();
        if (o.observe) {
            utxoz::set_log_callback(nullptr);
            sampler.stop.store(true, std::memory_order_relaxed);
            sampling.join();
            if (sampler.cpu >= 0 && sampler.pin_error != 0) {
                valid = false;
                why += fmt::format("observation: the disk sampler could not be pinned to CPU {} "
                                   "(error {}); ", sampler.cpu, sampler.pin_error);
            }
            obs.hwm_kb = status_kb("VmHWM:");
            if (obs.statm_fd >= 0) ::close(obs.statm_fd);
            obs.why = validate_marks(obs.marks, expected_marks(), obs.incompatible);
            if ( ! obs.why.empty()) {
                valid = false;
                why += "observation: " + obs.why + "; ";
            }
        }
        elapsed_ns = uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        if ( ! r) {
            ++failed_ops;
            why += fmt::format("compact_all failed: error_code={}; ", int(r.error()));
        }
        rotations_after = rotations();
        expected_final = expected_after_delete;
    }

    // ------------------------------------------------------------- validate
    if (o.selftest_expect_wrong) ++expected_final.entries;
    if (failed_ops != 0) {
        valid = false;
        why += fmt::format("{} operations did not complete; ", failed_ops);
    }
    if ( ! read_content(db, final_content)) {
        valid = false;
        why += "could not read the final content; ";
    } else if (final_content != expected_final) {
        valid = false;
        why += fmt::format("final content {} entries / {:016x}, the rule says {} / {:016x}; ",
                           final_content.entries, final_content.digest,
                           expected_final.entries, expected_final.digest);
    }
    if (inserting) {
        if (inserts_stat_after - inserts_stat_before != o.inserts) {
            valid = false;
            why += fmt::format("the statistics count {} inserts, not {}; ",
                               inserts_stat_after - inserts_stat_before, o.inserts);
        }
        uint64_t gained = 0;
        for (size_t i = 0; i != slots_after.size() && i != slots_before.size(); ++i)
            gained += slots_after[i].entries - slots_before[i].entries;
        if (slots_after.size() != slots_before.size() || gained != o.inserts) {
            valid = false;
            why += fmt::format("the slots gained {} entries, not {}; ", gained, o.inserts);
        }
    }
    if (o.metric == "insert_batch") {
        uint64_t slots_used = 0;
        for (size_t i = 0; i != slots_after.size() && i != slots_before.size(); ++i)
            if (slots_after[i].entries != slots_before[i].entries) ++slots_used;
        if (batch.calls_with_error || batch.failed || batch.not_applied || batch.duplicates
                || batch.inserted != o.inserts) {
            valid = false;
            why += fmt::format("the calls report {} inserted, {} duplicates, {} failed, {} not applied, "
                               "{} with an error; ", batch.inserted, batch.duplicates, batch.failed,
                               batch.not_applied, batch.calls_with_error);
        }
        if (batch.calls_short_of_threads != 0) {
            valid = false;
            why += fmt::format("{} calls ran more than one task on the calling thread: a worker thread "
                               "was not started; ", batch.calls_short_of_threads);
        }
        if (batch.workers_max != std::min<uint64_t>(o.workers, slots_used)) {
            valid = false;
            why += fmt::format("the calls used at most {} workers; {} asked over {} slots with inserts; ",
                               batch.workers_max, o.workers, slots_used);
        }
    }
    if (db.size() != expected_final.entries) {
        valid = false;
        why += fmt::format("size() is {}, not {}; ", db.size(), expected_final.entries);
    }
    if ( ! read_layout(db, after)) {
        valid = false;
        why += "the final census failed; ";
    } else if (after.entries != expected_final.entries) {
        valid = false;
        why += fmt::format("the census counts {} entries, not {}; ", after.entries,
                           expected_final.entries);
    }
    db.close();

    // The observation, if there was one. Intervals run from a container's mark
    // to the next mark, and are labelled as such: see the comment on mark.
    std::string observation = "null";
    if (o.observe) {
        long const page_kb = ::sysconf(_SC_PAGESIZE) / 1024;
        std::string intervals;
        for (size_t i = 1; i + 1 < obs.marks.size(); ++i) {
            auto const& m = obs.marks[i];
            intervals += fmt::format(
                "      {{\"class\": {}, \"shard\": {}, \"start_ns\": {}, "
                "\"interval_to_next_mark_ns\": {}, \"rss_kb_at_mark\": {}}},\n",
                m.cls, m.shard, m.t_ns, obs.marks[i + 1].t_ns - m.t_ns, m.rss_pages * uint64_t(page_kb));
        }
        if (intervals.size() >= 2) intervals.erase(intervals.size() - 2, 1);
        observation = fmt::format(
            "{{\n"
            "    \"marks_valid\": {},\n"
            "    \"marks_invalid_because\": \"{}\",\n"
            "    \"marks\": {},\n"
            "    \"other_messages\": {},\n"
            "    \"interval_meaning\": \"from this container's start mark to the next mark: its whole "
            "compact_container plus loop, return and logging overhead; the last one also covers "
            "the file-cache clear before the completion mark. Not an exact duration.\",\n"
            "    \"intervals\": [\n{}    ],\n"
            "    \"rss_before_kb\": {},\n"
            "    \"rss_hwm_available\": {},\n"
            "    \"rss_hwm_kb\": {},\n"
            "    \"rss_meaning\": \"process resident set, including mapped file pages; not the "
            "store's logical active RAM\",\n"
            "    \"disk_sample_us\": {},\n"
            "    \"disk_sampler_cpu\": {},\n"
            "    \"disk_samples\": {},\n"
            "    \"disk_alloc_first_bytes\": {},\n"
            "    \"disk_alloc_peak_bytes\": {},\n"
            "    \"disk_alloc_peak_at_ns\": {},\n"
            "    \"disk_alloc_last_bytes\": {},\n"
            "    \"disk_logical_peak_bytes\": {},\n"
            "    \"disk_meaning\": \"the largest allocated size of the work directory among the "
            "samples taken; a transient shorter than the interval can be missed\"\n"
            "  }}",
            obs.why.empty() ? "true" : "false", obs.why, obs.marks.size(), obs.other_messages,
            intervals, obs.rss_before_kb, obs.hwm_available ? "true" : "false",
            obs.hwm_available ? obs.hwm_kb : 0, o.sample_us, o.sampler_cpu, sampler.samples,
            sampler.alloc_first, sampler.alloc_peak, sampler.peak_t_ns, sampler.alloc_last,
            sampler.logical_peak);
    }

    // How the work was spread, all of it read outside the timed region.
    std::string spread = "null";
    if (inserting) {
        std::string slot_list, cpu_list;
        for (size_t i = 0; i != slots_after.size() && i != slots_before.size(); ++i)
            slot_list += fmt::format("{}{{\"class\": {}, \"shard\": {}, \"gained\": {}}}",
                                     i ? ", " : "", slots_after[i].cls, slots_after[i].shard,
                                     slots_after[i].entries - slots_before[i].entries);
        for (size_t i = 0; i != cpus.size(); ++i) cpu_list += fmt::format("{}{}", i ? ", " : "", cpus[i]);
        spread = fmt::format(
            "{{\n"
            "    \"affinity_cpus\": [{}],\n"
            "    \"cpu_ns_over_timed_region\": {},\n"
            "    \"cpu_meaning\": \"user+system CPU time of every thread of the process over the timed "
            "region, read just outside it; above elapsed_ns only if threads ran at once\",\n"
            "    \"inserts_counted_by_statistics\": {},\n"
            "    \"slots\": [{}],\n"
            "    \"workers_requested\": {},\n"
            "    \"batch_size\": {},\n"
            "    \"calls\": {},\n"
            "    \"tasks\": {},\n"
            "    \"tasks_on_calling_thread\": {},\n"
            "    \"workers_used_min\": {},\n"
            "    \"workers_used_max\": {},\n"
            "    \"calls_short_of_threads\": {},\n"
            "    \"inserted\": {},\n"
            "    \"duplicates\": {},\n"
            "    \"failed\": {},\n"
            "    \"not_applied\": {},\n"
            "    \"calls_with_error\": {}\n"
            "  }}",
            cpu_list, cpu_elapsed_ns, inserts_stat_after - inserts_stat_before, slot_list,
            o.metric == "insert_batch" ? o.workers : 0, o.metric == "insert_batch" ? o.batch_size : 0,
            batch.calls, batch.tasks, batch.inline_tasks, batch.workers_min, batch.workers_max,
            batch.calls_short_of_threads, batch.inserted, batch.duplicates, batch.failed,
            batch.not_applied, batch.calls_with_error);
    }

    auto const json = fmt::format(
        "{{\n"
        "  \"schema\": \"utxoz-calib-mut-1\",\n"
        "  \"metric\": \"{}\",\n"
        "  \"shard_count\": {},\n"
        "  \"geometry_id\": {},\n"
        "  \"population\": {},\n"
        "  \"value_size\": {},\n"
        "  \"inserts\": {},\n"
        "  \"pause_every\": {},\n"
        "  \"delete_batch\": {},\n"
        "  \"setup_deletions\": {},\n"
        "  \"seed\": {},\n"
        "  \"source\": \"{}\",\n"
        "  \"work_dir\": \"{}\",\n"
        "  \"token\": \"{}\",\n"
        "  \"copy_files\": {},\n"
        "  \"copy_bytes\": {},\n"
        "  \"copy_ms\": {:.1f},\n"
        "  \"initial_digest\": \"{:016x}\",\n"
        "  \"elapsed_ns\": {},\n"
        "  \"failed_ops\": {},\n"
        "  \"batches\": {},\n"
        "  \"batch_ns_p50\": {},\n"
        "  \"batch_ns_p99\": {},\n"
        "  \"batch_ns_max\": {},\n"
        "  \"rotations_class48\": {},\n"
        "  \"generations_class48_before\": {},\n"
        "  \"generations_class48_after\": {},\n"
        "  \"version_files_before\": {},\n"
        "  \"version_files_after\": {},\n"
        "  \"logical_bytes_before\": {},\n"
        "  \"logical_bytes_after\": {},\n"
        "  \"entries_after\": {},\n"
        "  \"final_digest\": \"{:016x}\",\n"
        "  \"expected_digest\": \"{:016x}\",\n"
        "  \"maxrss_kb\": {},\n"
        "  \"observed\": {},\n"
        "  \"observation\": {},\n"
        "  \"spread\": {},\n"
        "  \"valid\": {},\n"
        "  \"invalid_because\": \"{}\"\n"
        "}}\n",
        o.metric, utxoz::shard_count, s.geometry_id, s.population, s.value_size,
        inserting ? o.inserts : 0, o.pause_every, o.delete_batch,
        setup_ops, o.seed, source.string(), work.string(), token, cs.files, cs.bytes,
        copy_ms, initial.digest, elapsed_ns, failed_ops, batch_ns.size(),
        pct(batch_ns, 0.50), pct(batch_ns, 0.99),
        batch_ns.empty() ? 0 : *std::ranges::max_element(batch_ns),
        rotations_after - rotations_before, before.generations_class_48,
        after.generations_class_48, before.version_files_all, after.version_files_all,
        before.logical_file_bytes, after.logical_file_bytes, final_content.entries,
        final_content.digest, expected_final.digest, maxrss_kb(),
        o.observe ? "true" : "false", observation, spread,
        valid ? "true" : "false", why);
    std::fputs(json.c_str(), stdout);

    if ( ! valid) {
        fmt::println(stderr, "the operation did not do what was asked: {}", why);
        return exit_operation_failed;
    }
    return exit_ok;
}
