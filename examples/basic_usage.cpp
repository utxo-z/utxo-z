/**
 * @file basic_usage.cpp
 * @brief Basic usage example of UTXO database
 */

#include <chrono>
#include <iostream>
#include <random>

#include <utxoz/database.hpp>
#include <utxoz/utils.hpp>

int main() {
    try {
        std::cout << "UTXO Database Basic Usage Example\n";
        std::cout << "==================================\n\n";

        // Create and configure database
        utxoz::db db;
        db.configure("./example_utxo_data", true); // Remove existing data
        
        std::cout << "Database configured successfully\n";
        std::cout << "Initial size: " << db.size() << " UTXOs\n\n";
        
        // Generate some sample data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> byte_dist(0, 255);
        std::uniform_int_distribution<uint32_t> height_dist(700000, 800000);
        std::uniform_int_distribution<size_t> value_size_dist(25, 100);
        
        // Insert some UTXOs
        std::cout << "Inserting sample UTXOs...\n";
        std::vector<utxoz::key_t> inserted_keys;
        
        for (int i = 0; i < 1000; ++i) {
            // Generate random transaction hash
            std::array<uint8_t, 32> tx_hash;
            for (auto& byte : tx_hash) {
                byte = byte_dist(gen);
            }
            
            // Create key
            uint32_t output_index = i % 10;
            auto key = utxoz::make_key(tx_hash, output_index);
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
                std::cout << "Warning: UTXO already exists\n";
            }
            
            if (i % 100 == 0) {
                std::cout << "Inserted " << i << " UTXOs...\n";
            }
        }
        
        std::cout << "Inserted " << inserted_keys.size() << " UTXOs\n";
        std::cout << "Database size: " << db.size() << " UTXOs\n\n";
        
        // Find some UTXOs
        std::cout << "Testing UTXO lookups...\n";
        size_t found_count = 0;
        
        for (size_t i = 0; i < std::min(size_t(10), inserted_keys.size()); ++i) {
            auto result = db.find(inserted_keys[i], 800000);
            if (result) {
                ++found_count;
                std::cout << "Found UTXO " << i << ", size: " << result->size() << " bytes\n";
            } else {
                std::cout << "UTXO " << i << " not found\n";
            }
        }
        
        std::cout << "Found " << found_count << " out of 10 UTXOs\n\n";
        
        // Erase some UTXOs
        std::cout << "Testing UTXO deletion...\n";
        size_t erased_count = 0;
        
        for (size_t i = 0; i < std::min(size_t(100), inserted_keys.size()); i += 2) {
            size_t erased = db.erase(inserted_keys[i], 800000);
            erased_count += erased;
        }
        
        std::cout << "Erased " << erased_count << " UTXOs\n";
        std::cout << "Database size after erasure: " << db.size() << " UTXOs\n\n";
        
        // Process pending deletions
        std::cout << "Processing pending deletions...\n";
        auto [deleted, failed] = db.process_pending_deletions();
        std::cout << "Successfully deleted: " << deleted << "\n";
        std::cout << "Failed deletions: " << failed.size() << "\n\n";
        
        // Show statistics
        std::cout << "Database Statistics:\n";
        std::cout << "===================\n";
        db.print_statistics();
        
        // Test compaction
        std::cout << "\nRunning database compaction...\n";
        db.compact_all();
        std::cout << "Compaction completed\n\n";
        
        // Final statistics
        auto stats = db.get_statistics();
        std::cout << "Final Statistics:\n";
        std::cout << "Total entries: " << stats.total_entries << "\n";
        std::cout << "Total inserts: " << stats.total_inserts << "\n";
        std::cout << "Total deletes: " << stats.total_deletes << "\n";
        std::cout << "Cache hit rate: " << (stats.cache_hit_rate * 100) << "%\n";
        
        // Close database
        db.close();
        std::cout << "\nDatabase closed successfully\n";
        
    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
