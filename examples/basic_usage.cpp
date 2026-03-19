// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file basic_usage.cpp
 * @brief Basic usage example of UTXO database
 */

#include <array>
#include <random>
#include <vector>

#include <fmt/core.h>

#include <utxoz/database.hpp>
#include <utxoz/utils.hpp>

int main() {
    try {
        fmt::println("UTXO Database Basic Usage Example");
        fmt::println("==================================\n");

        // Create and configure database
        utxoz::db db;
        db.configure("./example_utxo_data", true); // Remove existing data

        fmt::println("Database configured successfully");
        fmt::println("Initial size: {} UTXOs\n", db.size());

        // Generate some sample data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> byte_dist(0, 255);
        std::uniform_int_distribution<uint32_t> height_dist(700000, 800000);
        std::uniform_int_distribution<size_t> value_size_dist(25, 100);

        // Insert some UTXOs
        fmt::println("Inserting sample UTXOs...");
        std::vector<utxoz::raw_outpoint> inserted_keys;

        for (int i = 0; i < 1000; ++i) {
            // Generate random transaction hash
            std::array<uint8_t, 32> tx_hash;
            for (auto& byte : tx_hash) {
                byte = byte_dist(gen);
            }

            // Create key
            uint32_t output_index = i % 10;
            auto key = utxoz::make_outpoint(tx_hash, output_index);
            inserted_keys.push_back(key);

            // Generate random value
            size_t value_size = value_size_dist(gen);
            std::vector<uint8_t> value(value_size);
            for (auto& byte : value) {
                byte = byte_dist(gen);
            }

            // Insert UTXO
            uint32_t height = height_dist(gen);
            bool inserted = db.insert(key, value, height);

            if (!inserted) {
                fmt::println("Warning: UTXO already exists");
            }

            if (i % 100 == 0) {
                fmt::println("Inserted {} UTXOs...", i);
            }
        }

        fmt::println("Inserted {} UTXOs", inserted_keys.size());
        fmt::println("Database size: {} UTXOs\n", db.size());

        // Find some UTXOs
        fmt::println("Testing UTXO lookups...");
        size_t found_count = 0;

        for (size_t i = 0; i < std::min(size_t(10), inserted_keys.size()); ++i) {
            auto result = db.find(inserted_keys[i], 800000);
            if (result) {
                ++found_count;
                fmt::println("Found UTXO {}, size: {} bytes", i, result->data.size());
            } else {
                fmt::println("UTXO {} not found", i);
            }
        }

        fmt::println("Found {} out of 10 UTXOs\n", found_count);

        // Erase some UTXOs
        fmt::println("Testing UTXO deletion...");
        size_t erased_count = 0;

        for (size_t i = 0; i < std::min(size_t(100), inserted_keys.size()); i += 2) {
            size_t erased = db.erase(inserted_keys[i], 800000);
            erased_count += erased;
        }

        fmt::println("Erased {} UTXOs", erased_count);
        fmt::println("Database size after erasure: {} UTXOs\n", db.size());

        // Process pending deletions
        fmt::println("Processing pending deletions...");
        auto [deleted, failed] = db.process_pending_deletions();
        fmt::println("Successfully deleted: {}", deleted);
        fmt::println("Failed deletions: {}\n", failed.size());

        // Show statistics
        fmt::println("Database Statistics:");
        fmt::println("===================");
        db.print_statistics();

        // Test compaction
        fmt::println("\nRunning database compaction...");
        db.compact_all();
        fmt::println("Compaction completed\n");

        // Final statistics
        auto stats = db.get_statistics();
        fmt::println("Final Statistics:");
        fmt::println("Total entries: {}", stats.total_entries);
        fmt::println("Total inserts: {}", stats.total_inserts);
        fmt::println("Total deletes: {}", stats.total_deletes);
        fmt::println("Cache hit rate: {}%", stats.cache_hit_rate * 100);

        // Close database
        db.close();
        fmt::println("\nDatabase closed successfully");

    } catch (std::exception const& e) {
        fmt::println(stderr, "Error: {}", e.what());
        return 1;
    }

    return 0;
}
