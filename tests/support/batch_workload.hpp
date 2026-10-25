// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file batch_workload.hpp
 * @brief The workload and the comparisons the insert_batch tests share.
 *
 * Used by test_insert_batch.cpp and by the allocation test, which is its own
 * binary because it replaces operator new.
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include <utxoz/census.hpp>
#include <utxoz/database.hpp>
#include <utxoz/logging.hpp>
#include <utxoz/statistics.hpp>
#include <utxoz/types.hpp>

#if defined(UTXOZ_LOG_SPDLOG)
#include <spdlog/spdlog.h>
#include <spdlog/sinks/callback_sink.h>
#endif

#include "../shard_paths.hpp"

namespace utxoz::testing::batch {

namespace fs = std::filesystem;


struct temp_db {
    temp_db() {
        static std::atomic<uint64_t> counter{0};
        auto const ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        dir = fmt::format("./test_insert_batch_{}_{}_{}", getpid(), ts, counter.fetch_add(1));
        fs::remove_all(dir);
    }
    ~temp_db() { std::error_code ec; fs::remove_all(dir, ec); }
    temp_db(temp_db const&) = delete;
    temp_db& operator=(temp_db const&) = delete;
    fs::path dir;
};

/// A payload of `n`'s choosing that lands in class `index`.
inline std::vector<uint8_t> payload(size_t index, uint64_t n) {
    std::vector<uint8_t> v(index == 0 ? 8 : container_capacities[index - 1] + 1);
    for (size_t i = 0; i != v.size(); ++i) v[i] = uint8_t(i * 7 + 1);
    for (size_t b = 0; b != 8 && b < v.size(); ++b) v[b] = uint8_t(n >> (8 * b));
    return v;
}

/// Requests and the payloads they borrow.
struct workload {
    std::vector<std::vector<uint8_t>> values;
    std::vector<insert_request> requests;

    void add(raw_outpoint const& key, size_t cls, uint64_t n, uint32_t height) {
        values.push_back(payload(cls, n));
        requests.push_back({key, {}, height});
    }
    /// Spans are bound last: `values` reallocates while it grows.
    void bind() {
        for (size_t i = 0; i != requests.size(); ++i) requests[i].value = values[i];
    }
};

/// Every class, several blocks' heights, keys spread over every shard, two
/// classes that rotate in the same batch (classes 3 and 4 hold 25 535 and 797
/// entries per generation in the testing profile), duplicates in the same slot
/// in order, and one key stored in two classes.
inline workload mixed() {
    workload w;
    uint64_t n = 0;
    auto height = [&](uint64_t i) { return uint32_t(700000 + i / 1000); };
    for (uint64_t i = 0; i != 20000; ++i, ++n) w.add(spread_key(n), 0, n, height(i));
    for (uint64_t i = 0; i != 3000; ++i, ++n) w.add(spread_key(n), 1, n, height(i));
    for (uint64_t i = 0; i != 3000; ++i, ++n) w.add(spread_key(n), 2, n, height(i));
    for (uint64_t i = 0; i != 26000; ++i, ++n) w.add(spread_key(n), 3, n, height(i));
    for (uint64_t i = 0; i != 2500; ++i, ++n) w.add(spread_key(n), 4, n, height(i));
    // Duplicates, after their originals and at other heights: the first wins.
    for (uint64_t i = 0; i != 500; ++i) w.add(spread_key(i * 7), 0, 9000000 + i, uint32_t(800000 + i));
    for (uint64_t i = 0; i != 50; ++i) w.add(spread_key(20000 + 3000 + 3000 + 26000 + i), 4, 9100000 + i,
                                            uint32_t(800000 + i));
    // The same key in another class is another slot, and another entry.
    w.add(spread_key(1), 3, 9200000, 900000);
    // Interleave the classes, so no worker sees its slot's requests in one run.
    std::vector<size_t> order(w.requests.size());
    for (size_t i = 0; i != order.size(); ++i) order[i] = i;
    std::ranges::stable_sort(order, [](size_t a, size_t b) { return (a * 2654435761u) % 1000003 <
                                                                    (b * 2654435761u) % 1000003; });
    // ...but never reorder a duplicate before its original: stable within a key.
    workload out;
    std::map<raw_outpoint, std::vector<size_t>> by_key;
    for (auto const i : order) by_key[w.requests[i].key].push_back(i);
    for (auto& [_, v] : by_key) std::ranges::sort(v);
    std::map<raw_outpoint, size_t> next;
    for (auto const i : order) {
        auto const& k = w.requests[i].key;
        auto const j = by_key[k][next[k]++];
        out.values.push_back(w.values[j]);
        out.requests.push_back(w.requests[j]);
    }
    out.bind();
    return out;
}

inline full_db open_db(fs::path const& dir, bool create) {
    auto r = full_db::open_for_testing(dir, create);
    REQUIRE(r.has_value());
    return std::move(*r);
}

/// Order-independent: the same entries give the same digest however stored.
struct content { uint64_t entries = 0, digest = 0; bool operator==(content const&) const = default; };

inline content content_of(full_db const& db) {
    content c;
    auto const r = db.for_each_entry([&](raw_outpoint const& k, uint32_t h, std::span<uint8_t const> d) {
        uint64_t x = 0xcbf29ce484222325ULL;
        auto add = [&](uint8_t b) { x ^= b; x *= 0x100000001b3ULL; };
        for (auto b : k) add(b);
        for (int i = 0; i != 4; ++i) add(uint8_t(h >> (8 * i)));
        for (auto b : d) add(b);
        x ^= x >> 31; x *= 0x94D049BB133111EBULL;
        c.digest += x;
        ++c.entries;
    });
    REQUIRE(r.has_value());
    return c;
}

/// The figures that must agree, as one comparable value.
struct figures {
    size_t size = 0;
    content stored;
    std::vector<size_t> inserts, current, rehash, rotations, preventive;
    std::vector<std::map<size_t, size_t>> value_sizes;
    std::vector<std::pair<uint64_t, uint64_t>> generations;          ///< per class: entries, generations
    std::vector<std::tuple<uint64_t, uint32_t, uint64_t, uint64_t>> shards;  ///< class, shard, entries, gens
    std::vector<std::string> height_ranges;
    bool operator==(figures const&) const = default;
};

#if defined(UTXOZ_LOG_CUSTOM) || defined(UTXOZ_LOG_SPDLOG)
// Install only while the database is quiescent, before starting workers, and
// restore only after they have joined. This is test observation, not DB locking.
struct scoped_log_capture {
#if defined(UTXOZ_LOG_CUSTOM)
    log_callback_t previous = get_log_callback();
    explicit scoped_log_capture(std::function<void(std::string_view)> callback) {
        set_log_callback([callback = std::move(callback)](log_level, std::string_view msg) {
            callback(msg);
        });
    }
    void reset() { if (active) { set_log_callback(std::move(previous)); active = false; } }
#elif defined(UTXOZ_LOG_SPDLOG)
    std::shared_ptr<spdlog::logger> previous = spdlog::default_logger();
    std::shared_ptr<spdlog::logger> previous_named = spdlog::get("batch-capture");
    explicit scoped_log_capture(std::function<void(std::string_view)> callback) {
        auto sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
            [callback = std::move(callback)](spdlog::details::log_msg const& msg) {
                callback({msg.payload.data(), msg.payload.size()});
            });
        auto logger = std::make_shared<spdlog::logger>("batch-capture", sink);
        logger->set_level(spdlog::level::trace);
        spdlog::set_default_logger(logger);
    }
    void reset() {
        if (active) {
            // set_default_logger does not unregister the old named logger.
            // Remove our borrowed callback before any captured state dies.
            spdlog::drop("batch-capture");
            spdlog::set_default_logger(std::move(previous));
            if (previous_named && previous_named != spdlog::default_logger())
                spdlog::register_logger(std::move(previous_named));
            active = false;
        }
    }
#endif
    ~scoped_log_capture() { reset(); }
    scoped_log_capture(scoped_log_capture const&) = delete;
    scoped_log_capture& operator=(scoped_log_capture const&) = delete;
    bool active = true;
};
#endif

inline std::vector<std::string> height_range_lines(full_db& db) {
    std::vector<std::string> lines;
#if defined(UTXOZ_LOG_CUSTOM) || defined(UTXOZ_LOG_SPDLOG)
    std::mutex m;
    scoped_log_capture capture([&](std::string_view msg) {
        std::scoped_lock lock(m);
        lines.emplace_back(msg);
    });
    db.print_height_range_stats();
#else
    // log=none has no observable log output. The other figures still compare
    // stored contents, generations, shards, rotations and outcomes.
    db.print_height_range_stats();
#endif
    return lines;
}

inline figures figures_of(full_db& db) {
    figures f;
    f.size = db.size();
    f.stored = content_of(db);
    auto const s = db.get_statistics();
    for (size_t c = 0; c != container_count; ++c) {
        auto const& cs = s.containers[c];
        f.inserts.push_back(cs.total_inserts);
        f.current.push_back(cs.current_size);
        f.rehash.push_back(cs.rehash_count);
        f.value_sizes.emplace_back(cs.value_size_distribution.begin(), cs.value_size_distribution.end());
        f.rotations.push_back(s.rotations_per_container[c]);
        f.preventive.push_back(s.rotations_by_cause[c].preventive);
    }
    auto const census = db.census();
    REQUIRE(census.has_value());
    for (auto const& cl : census->classes) {
        f.generations.emplace_back(cl.entries, cl.generations);
        for (auto const& sh : cl.shards_detail)
            f.shards.emplace_back(cl.container_class, sh.shard, sh.entries, sh.generations);
    }
    f.height_ranges = height_range_lines(db);
    return f;
}

/// The reference: the same requests, one insert each, in order.
inline std::vector<insert_outcome> insert_sequentially(full_db& db, workload const& w) {
    std::vector<insert_outcome> out;
    for (auto const& r : w.requests) {
        auto const done = db.insert(r.key, r.value, r.height);
        if (done) out.push_back({*done ? insert_status::inserted : insert_status::duplicate, {}});
        else out.push_back({insert_status::failed, done.error()});
    }
    return out;
}

inline void require_same_outcomes(std::vector<insert_outcome> const& a, std::vector<insert_outcome> const& b) {
    REQUIRE(a.size() == b.size());
    size_t mismatches = 0;
    for (size_t i = 0; i != a.size(); ++i)
        if (a[i].status != b[i].status || (a[i].status == insert_status::failed && a[i].error != b[i].error))
            ++mismatches;
    CHECK(mismatches == 0);
}

inline size_t slots_touched(workload const& w) {
    std::set<std::pair<size_t, uint32_t>> s;
    for (auto const& r : w.requests) {
        size_t cls = 0;
        while (cls < container_count - 1 && r.value.size() > container_capacities[cls]) ++cls;
        s.emplace(cls, cls == 0 ? detail::shard_of(r.key, shard_count) : 0u);
    }
    return s.size();
}


} // namespace utxoz::testing::batch
