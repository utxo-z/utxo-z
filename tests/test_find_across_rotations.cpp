// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file test_find_across_rotations.cpp
 * @brief Locks down the deferred lookup / deferred deletion contract across a
 *        container rotation.
 *
 * Design being pinned here: containers are generational. Only the latest
 * version of each container is mapped, and find()/erase() look there (plus, for
 * erase, the cached files). Anything left behind in a previous version is NOT
 * reported by find()/erase(); it is queued and answered definitively by
 * process_pending_lookups() / process_pending_deletions().
 *
 * This means a not_found from find() (or a 0 from erase()) is not authoritative
 * once a container has rotated. Integrators that treat it as authoritative see
 * a "UTXO disappeared" failure the moment the first rotation happens — with
 * production file sizes that is container 0 rolling over at ~6.6M live UTXOs
 * (BCH mainnet ~block 245'000), which is exactly when a node sync breaks.
 *
 * No test in the suite used to cross a rotation, so the contract was never
 * pinned. These tests do.
 */

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <numeric>
#include <optional>
#include <random>
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
#include <utxoz/utils.hpp>

namespace {

inline std::atomic<uint64_t> rotation_test_counter{0};

std::string make_unique_path(std::string_view tag) {
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto unique_id = rotation_test_counter.fetch_add(1);
    return fmt::format("./test_rot_{}_{}_{}_{}", tag, getpid(), ts, unique_id);
}

/// Random 36-byte outpoint: 32 random bytes (txid) + 4-byte LE output index.
utxoz::raw_outpoint random_outpoint(std::mt19937_64& rng, uint32_t output_index) {
    utxoz::raw_outpoint key{};
    for (size_t i = 0; i < 32; i += 8) {
        uint64_t const chunk = rng();
        std::memcpy(key.data() + i, &chunk, sizeof(chunk));
    }
    std::memcpy(key.data() + 32, &output_index, sizeof(output_index));
    return key;
}

/// wide      : every one of the 5 size-tiered containers gets a healthy share.
/// realistic : p2pkh-dominated, like a real UTXO set. Keeps the number of
///             container-3/4 version files manageable in the large-scale run.
enum class size_profile { wide, realistic };

/// Container capacities are 43 / 89 / 123 / 250 / 10234 bytes.
size_t pick_value_size(std::mt19937_64& rng, size_profile profile) {
    std::uniform_int_distribution<int> pick(0, 999);
    int const p = pick(rng);
    if (profile == size_profile::wide) {
        if (p < 800) return 33;    // container 0 (p2pkh-ish)
        if (p < 900) return 70;    // container 1
        if (p < 960) return 110;   // container 2
        if (p < 995) return 200;   // container 3
        return 1500;               // container 4
    }
    if (p < 970) return 33;        // container 0
    if (p < 985) return 70;        // container 1
    if (p < 995) return 110;       // container 2
    if (p < 999) return 200;       // container 3
    return 1500;                   // container 4
}

std::vector<uint8_t> make_value(size_t size, uint8_t seed) {
    std::vector<uint8_t> v(size);
    std::iota(v.begin(), v.end(), seed);
    return v;
}

template<typename Db>
bool scan_contains(Db const& db, utxoz::raw_outpoint const& needle) {
    bool found = false;
    (void)db.for_each_key([&](utxoz::raw_outpoint const& k) {
        if (k == needle) found = true;
    });
    return found;
}

template<typename Db>
size_t scan_count(Db const& db) {
    size_t n = 0;
    (void)db.for_each_key([&](utxoz::raw_outpoint const&) { ++n; });
    return n;
}

size_t rotations_of(utxoz::full_db& db, size_t container) {
    return db.get_statistics().rotations_per_container[container];
}

/// Number of on-disk version files of a container.
size_t count_data_files(std::string const& path, size_t container) {
    auto const prefix = fmt::format("cont_{}_v", container);
    size_t n = 0;
    for (auto const& entry : std::filesystem::directory_iterator(path)) {
        if (entry.path().filename().string().rfind(prefix, 0) == 0) ++n;
    }
    return n;
}

/**
 * @brief Inserts (and spends part of) a growing UTXO set until container 0
 *        rotates to a new file.
 *
 * @return live entry count at the moment container 0 rotated, or 0 if it never
 *         rotated within max_batches.
 */
size_t fill_until_container0_rotates(utxoz::full_db& db,
                                     std::mt19937_64& rng,
                                     uint32_t& height,
                                     size_t batch_size,
                                     size_t max_batches,
                                     size_profile profile,
                                     bool spend_as_we_go) {
    std::vector<utxoz::raw_outpoint> spendable;
    std::vector<utxoz::raw_outpoint> created;

    for (size_t batch = 0; batch < max_batches; ++batch) {
        created.clear();
        created.reserve(batch_size);

        for (size_t i = 0; i < batch_size; ++i) {
            auto const key = random_outpoint(rng, static_cast<uint32_t>(i % 4));
            auto const value = make_value(pick_value_size(rng, profile), static_cast<uint8_t>(i));
            REQUIRE(db.insert(key, value, height).has_value());
            created.push_back(key);
        }

        if (spend_as_we_go) {
            // Spend ~60% of what the previous batch created, so the live set
            // keeps growing while the erase/deferred-deletion path is exercised.
            size_t const to_spend = spendable.size() * 6 / 10;
            for (size_t i = 0; i < to_spend; ++i) {
                (void)db.erase(spendable[i], height);
            }
            auto const [deleted, failed] = db.process_pending_deletions().value();
            (void)deleted;
            (void)failed;
            spendable = created;
        }

        ++height;

        if (rotations_of(db, 0) >= 1) {
            return db.size();
        }
    }

    return 0;
}

} // anonymous namespace

// =============================================================================
// Full mode
// =============================================================================

TEST_CASE("find(): before rotation resolves inline, after rotation it defers",
          "[database][rotation][deferred][contract]") {
    auto const path = make_unique_path("contract");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        std::mt19937_64 rng(0x5EED'1234'ABCDULL);

        // Witness: inserted first, never spent. Small value -> container 0.
        auto const witness = random_outpoint(rng, 0);
        auto const witness_value = make_value(33, 0xAB);
        REQUIRE(db.insert(witness, witness_value, 1).value());

        // While container 0 still has a single version, find() answers inline.
        {
            auto const found = db.find(witness, 1);
            REQUIRE(found.has_value());
            CHECK(found->data == witness_value);
            CHECK(found->block_height == 1u);
            CHECK(db.deferred_lookups_size() == 0);
        }

        uint32_t height = 2;
        auto const entries_at_rotation = fill_until_container0_rotates(
            db, rng, height, /*batch_size=*/5'000, /*max_batches=*/400,
            size_profile::wide, /*spend_as_we_go=*/true);

        INFO("live entries at container-0 rotation: " << entries_at_rotation);
        REQUIRE(entries_at_rotation > 0);

        // The entry is still physically there: the full scan enumerates it.
        REQUIRE(scan_contains(db, witness));

        // ...but it now lives in a previous version, so find() defers instead of
        // answering. This not_found is NOT authoritative.
        auto const deferred = db.find(witness, height);
        REQUIRE_FALSE(deferred.has_value());
        CHECK(deferred.error() == utxoz::error_code::not_found);
        CHECK(db.deferred_lookups_size() == 1);

        // process_pending_lookups() is the definitive answer.
        auto const [found, missing] = db.process_pending_lookups().value();
        CHECK(missing.empty());
        REQUIRE(found.contains(witness));
        CHECK(found.at(witness).data == witness_value);
        CHECK(found.at(witness).block_height == 1u);

        // The call drains the queue.
        CHECK(db.deferred_lookups_size() == 0);

        db.close();
    }

    std::filesystem::remove_all(path);
}

TEST_CASE("find(): one process_pending_lookups() resolves deferrals from every container",
          "[database][rotation][deferred][contract]") {
    auto const path = make_unique_path("allc");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        std::mt19937_64 rng(0xC0FFEEULL);

        // One witness per size tier, inserted early, never spent.
        std::array<size_t, utxoz::container_count> const sizes{33, 70, 110, 200, 1500};
        std::array<utxoz::raw_outpoint, utxoz::container_count> witnesses{};
        std::array<std::vector<uint8_t>, utxoz::container_count> values{};

        for (size_t i = 0; i < utxoz::container_count; ++i) {
            witnesses[i] = random_outpoint(rng, static_cast<uint32_t>(i));
            values[i] = make_value(sizes[i], static_cast<uint8_t>(i));
            REQUIRE(db.insert(witnesses[i], values[i], 1).value());
        }

        // Fill until every container has rotated at least once. Sizes are
        // cycled round-robin so no tier is starved.
        uint32_t height = 2;
        for (size_t batch = 0; batch < 1'000; ++batch) {
            for (size_t i = 0; i < 2'000; ++i) {
                auto const key = random_outpoint(rng, static_cast<uint32_t>(i % 4));
                auto const value = make_value(sizes[i % utxoz::container_count],
                                              static_cast<uint8_t>(i));
                REQUIRE(db.insert(key, value, height).has_value());
            }
            ++height;

            auto const stats = db.get_statistics();
            bool all_rotated = true;
            for (size_t i = 0; i < utxoz::container_count; ++i) {
                if (stats.rotations_per_container[i] < 1) all_rotated = false;
            }
            if (all_rotated) break;
        }

        auto const stats = db.get_statistics();
        for (size_t i = 0; i < utxoz::container_count; ++i) {
            INFO("container " << i << " rotations: " << stats.rotations_per_container[i]);
            REQUIRE(stats.rotations_per_container[i] >= 1);
        }

        // Queue all five lookups, then resolve them in a single batch call.
        for (size_t i = 0; i < utxoz::container_count; ++i) {
            INFO("container " << i);
            CHECK_FALSE(db.find(witnesses[i], height).has_value());
        }
        CHECK(db.deferred_lookups_size() == utxoz::container_count);

        auto const [found, missing] = db.process_pending_lookups().value();
        CHECK(missing.empty());
        for (size_t i = 0; i < utxoz::container_count; ++i) {
            INFO("container " << i);
            REQUIRE(found.contains(witnesses[i]));
            CHECK(found.at(witnesses[i]).data == values[i]);
            CHECK(found.at(witnesses[i]).block_height == 1u);
        }
        CHECK(db.deferred_lookups_size() == 0);

        db.close();
    }

    std::filesystem::remove_all(path);
}

TEST_CASE("find(): a key that exists nowhere comes back in the failed list",
          "[database][deferred][contract]") {
    auto const path = make_unique_path("absent");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        std::mt19937_64 rng(0x1234'5678ULL);
        for (size_t i = 0; i < 1'000; ++i) {
            REQUIRE(db.insert(random_outpoint(rng, 0), make_value(33, 1), 10).has_value());
        }

        auto const absent = random_outpoint(rng, 99);
        CHECK_FALSE(db.find(absent, 11).has_value());

        auto const [found, missing] = db.process_pending_lookups().value();
        CHECK(found.empty());
        REQUIRE(missing.size() == 1);
        CHECK(missing[0].key == absent);

        db.close();
    }

    std::filesystem::remove_all(path);
}

TEST_CASE("erase(): after rotation it defers, process_pending_deletions() is definitive",
          "[database][rotation][deferred][contract]") {
    auto const path = make_unique_path("erase");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        std::mt19937_64 rng(0xBEEF'0001ULL);

        // Early keys, spent only once rotations have happened.
        std::vector<utxoz::raw_outpoint> early;
        for (size_t i = 0; i < 100; ++i) {
            auto const key = random_outpoint(rng, 0);
            REQUIRE(db.insert(key, make_value(33, 0x11), 1).value());
            early.push_back(key);
        }

        uint32_t height = 2;
        auto const entries_at_rotation = fill_until_container0_rotates(
            db, rng, height, /*batch_size=*/5'000, /*max_batches=*/400,
            size_profile::wide, /*spend_as_we_go=*/false);
        REQUIRE(entries_at_rotation > 0);

        // They are all still stored...
        for (auto const& key : early) {
            REQUIRE(scan_contains(db, key));
        }

        // ...but erase() cannot see them, so it defers. A 0 here is not "absent".
        size_t erased_inline = 0;
        for (auto const& key : early) {
            auto const erased = db.erase(key, height);
            REQUIRE(erased);   // value_or would swallow closed and recovery_required
            erased_inline += *erased;
        }
        CHECK(db.deferred_deletions_size() == early.size() - erased_inline);

        auto const [deleted, failed] = db.process_pending_deletions().value();
        CHECK(failed.empty());
        CHECK(erased_inline + deleted == early.size());
        CHECK(db.deferred_deletions_size() == 0);

        // Now they are really gone, through both paths.
        for (auto const& key : early) {
            CHECK_FALSE(db.find(key, height).has_value());
            CHECK_FALSE(scan_contains(db, key));
        }
        auto const [found_after, missing_after] = db.process_pending_lookups().value();
        CHECK(found_after.empty());
        CHECK(missing_after.size() == early.size());

        db.close();
    }

    std::filesystem::remove_all(path);
}

TEST_CASE("lookups must be processed before deletions within a batch",
          "[database][rotation][deferred][contract]") {
    auto const path = make_unique_path("order");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        std::mt19937_64 rng(0x0DDE'0042ULL);

        auto const key = random_outpoint(rng, 3);
        auto const value = make_value(33, 0x55);
        REQUIRE(db.insert(key, value, 1).value());

        uint32_t height = 2;
        REQUIRE(fill_until_container0_rotates(db, rng, height, 5'000, 400,
                                              size_profile::wide, false) > 0);

        // Same batch: the key is read and spent. Both operations defer.
        CHECK_FALSE(db.find(key, height).has_value());
        CHECK(db.erase(key, height).value() == 0);
        CHECK(db.deferred_lookups_size() == 1);
        CHECK(db.deferred_deletions_size() == 1);

        // Lookups first: the value is still readable.
        auto const [found, missing] = db.process_pending_lookups().value();
        CHECK(missing.empty());
        REQUIRE(found.contains(key));
        CHECK(found.at(key).data == value);

        // Deletions second: now it is removed.
        auto const [deleted, failed] = db.process_pending_deletions().value();
        CHECK(failed.empty());
        CHECK(deleted == 1u);
        CHECK_FALSE(scan_contains(db, key));

        db.close();
    }

    std::filesystem::remove_all(path);
}

// =============================================================================
// Reference mode — same contract
// =============================================================================

TEST_CASE("reference find(): after rotation it defers, process_pending_lookups() is definitive",
          "[database][reference][rotation][deferred][contract]") {
    auto const path = make_unique_path("reference");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::reference_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        std::mt19937_64 rng(0xA5A5'1234ULL);

        auto const witness = random_outpoint(rng, 7);
        REQUIRE(db.insert(witness, /*file_number=*/42, /*offset=*/1234, /*height=*/1).value());
        REQUIRE(db.find(witness, 1).has_value());

        uint32_t height = 2;
        size_t rotations = 0;
        for (size_t batch = 0; batch < 400 && rotations < 1; ++batch) {
            for (size_t i = 0; i < 5'000; ++i) {
                auto const key = random_outpoint(rng, static_cast<uint32_t>(i % 4));
                REQUIRE(db.insert(key, static_cast<uint32_t>(i),
                                  static_cast<uint32_t>(batch), height).has_value());
            }
            ++height;
            rotations = db.get_statistics().rotations_per_container[0];
        }
        INFO("rotations: " << rotations);
        REQUIRE(rotations >= 1);

        // Still stored, but no longer in the mapped version.
        REQUIRE(scan_contains(db, witness));
        CHECK_FALSE(db.find(witness, height).has_value());
        CHECK(db.deferred_lookups_size() == 1);

        auto const [found, missing] = db.process_pending_lookups().value();
        CHECK(missing.empty());
        REQUIRE(found.contains(witness));
        CHECK(found.at(witness).file_number == 42u);
        CHECK(found.at(witness).offset == 1234u);
        CHECK(found.at(witness).block_height == 1u);
        CHECK(db.deferred_lookups_size() == 0);

        db.close();
    }

    std::filesystem::remove_all(path);
}

// =============================================================================
// Compaction must not leave the file cache pointing at renamed/removed files
// =============================================================================

/**
 * Consistency of the deferred paths across a compaction that runs with a warm
 * file cache.
 *
 * Compaction drains source files into targets, unlinks the emptied ones and
 * renumbers the rest, while the file cache is keyed by (container_index,
 * version) — a number compaction reassigns. compact_all() therefore drops every
 * cached mapping; this test is the guard for the surrounding behaviour: entry
 * count, full scan, deferred lookups and deferred deletions must all still
 * agree after compacting a multi-version container.
 *
 * Note: this test passes with and without that cache invalidation. It pins the
 * post-compaction contract, it does not isolate the stale-mapping path — no
 * workload was found that turns a surviving cache entry into a wrong answer,
 * because compaction's target/source rotation keeps landing the cached file on
 * the receiving side.
 */
TEST_CASE("compact_all() keeps the deferred paths consistent",
          "[database][compaction][cache]") {
    auto const path = make_unique_path("referenceall");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open_for_testing(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        std::mt19937_64 rng(0xCAFE'0007ULL);

        // Fill container 0 up to four versions, recording which version each
        // chunk of keys landed in. Chunks that straddle a rotation are dropped.
        std::array<std::vector<utxoz::raw_outpoint>, 3> by_version;
        uint32_t height = 1;
        constexpr size_t chunk = 1'000;

        for (size_t c = 0; c < 2'000 && rotations_of(db, 0) < 3; ++c) {
            size_t const version_before = rotations_of(db, 0);
            std::vector<utxoz::raw_outpoint> chunk_keys;
            chunk_keys.reserve(chunk);

            for (size_t i = 0; i < chunk; ++i) {
                auto const key = random_outpoint(rng, static_cast<uint32_t>(i % 4));
                REQUIRE(db.insert(key, make_value(33, static_cast<uint8_t>(i)), height).has_value());
                chunk_keys.push_back(key);
            }
            ++height;

            size_t const version_after = rotations_of(db, 0);
            if (version_before == version_after && version_after < by_version.size()) {
                auto& dst = by_version[version_after];
                dst.insert(dst.end(), chunk_keys.begin(), chunk_keys.end());
            }
        }
        // Every recorded version must hold at least the slice size used below,
        // so a change in test-mode file sizing fails here instead of running
        // past end() when the slices are taken.
        constexpr size_t survivors = 50;
        REQUIRE(rotations_of(db, 0) == 3);
        for (auto const& v : by_version) REQUIRE(v.size() >= survivors);

        auto const erase_all = [&](std::vector<utxoz::raw_outpoint> const& keys) {
            for (auto const& k : keys) (void)db.erase(k, height);
            auto const [deleted, failed] = db.process_pending_deletions().value();
            (void)deleted;
            CHECK(failed.empty());
        };

        // Free most of version 1 so that compaction actually has work to do:
        // files get drained, unlinked and renumbered.
        erase_all(std::vector<utxoz::raw_outpoint>(by_version[1].begin() + survivors,
                                                   by_version[1].end()));

        // Sample of keys that must remain reachable through the whole exercise.
        std::vector<utxoz::raw_outpoint> probes(by_version[0].begin(),
                                                by_version[0].begin() + survivors);
        probes.insert(probes.end(), by_version[1].begin(), by_version[1].begin() + survivors);
        probes.insert(probes.end(), by_version[2].begin(), by_version[2].begin() + survivors);

        // Warm the file cache: the sweep leaves a previous-version file mapped
        // under its (container_index, version) key.
        for (auto const& k : probes) (void)db.find(k, height);
        {
            auto const [found, missing] = db.process_pending_lookups().value();
            CHECK(missing.empty());
            CHECK(found.size() == probes.size());
        }
        REQUIRE_FALSE(db.get_cached_file_info().empty());

        auto const size_before = db.size();
        REQUIRE(scan_count(db) == size_before);
        auto const files_before = count_data_files(path, 0);

        REQUIRE(db.compact_all().has_value());

        // Compaction must have actually merged files, otherwise the test is not
        // exercising the renumbering it claims to.
        INFO("container-0 files: " << files_before << " -> " << count_data_files(path, 0));
        REQUIRE(count_data_files(path, 0) < files_before);

        CHECK(db.size() == size_before);
        CHECK(scan_count(db) == size_before);

        // Every probe is still stored and still reachable.
        for (auto const& k : probes) REQUIRE(scan_contains(db, k));

        size_t found_inline = 0;
        for (auto const& k : probes) {
            if (db.find(k, height).has_value()) ++found_inline;
        }
        auto const [found, missing] = db.process_pending_lookups().value();
        CHECK(missing.empty());
        CHECK(found_inline + found.size() == probes.size());

        // Deletions applied after compaction must land on the real files.
        size_t erased_inline = 0;
        for (auto const& k : probes) {
            auto const erased = db.erase(k, height);
            REQUIRE(erased);   // value_or would swallow closed and recovery_required
            erased_inline += *erased;
        }
        auto const [deleted, failed] = db.process_pending_deletions().value();
        CHECK(failed.empty());
        CHECK(erased_inline + deleted == probes.size());
        for (auto const& k : probes) {
            CHECK_FALSE(scan_contains(db, k));
        }
        CHECK(db.size() == size_before - probes.size());
        CHECK(scan_count(db) == db.size());

        db.close();
    }

    std::filesystem::remove_all(path);
}

// =============================================================================
// Production sizing
// =============================================================================

/**
 * The node's failure point: container 0 (2 GiB file, ~7.86M buckets) rolls over
 * when the load factor reaches max_load_factor * 0.95, i.e. around 6.5-6.7M live
 * UTXOs — BCH mainnet ~block 245'000. Everything found before that point was
 * served from the single mapped version, which is why find() appears to work for
 * thousands of blocks and then "breaks" all at once.
 *
 * Hidden by default ([.slow]): needs several GiB of disk and about a minute.
 * Run with: utxoz_tests "[scale]"
 */
TEST_CASE("the deferred contract holds at production sizing (~6.6M live UTXOs)",
          "[database][rotation][deferred][scale][.slow]") {
    auto const path = make_unique_path("scale");
    std::filesystem::remove_all(path);

    {
        auto r = utxoz::full_db::open(path, true);
        REQUIRE(r.has_value());
        auto db = std::move(*r);

        std::mt19937_64 rng(0xD00D'FACEULL);

        auto const witness = random_outpoint(rng, 0);
        auto const witness_value = make_value(33, 0xAB);
        REQUIRE(db.insert(witness, witness_value, 1).value());
        REQUIRE(db.find(witness, 1).has_value());

        uint32_t height = 2;
        auto const entries_at_rotation = fill_until_container0_rotates(
            db, rng, height, /*batch_size=*/100'000, /*max_batches=*/400,
            size_profile::realistic, /*spend_as_we_go=*/true);

        INFO("live entries at container-0 rotation: " << entries_at_rotation);
        REQUIRE(entries_at_rotation > 0);
        CHECK(entries_at_rotation > 6'000'000);

        // Same contract as at test sizing: deferred, then resolved.
        CHECK_FALSE(db.find(witness, height).has_value());
        CHECK(db.deferred_lookups_size() == 1);

        auto const [found, missing] = db.process_pending_lookups().value();
        CHECK(missing.empty());
        REQUIRE(found.contains(witness));
        CHECK(found.at(witness).data == witness_value);
        CHECK(found.at(witness).block_height == 1u);

        db.close();
    }

    std::filesystem::remove_all(path);
}
