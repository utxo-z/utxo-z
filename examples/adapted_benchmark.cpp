/**
 * @file adapted_benchmark.cpp
 * @brief Adapter to use the original benchmark with new library API
 */

#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <utxoz/database.hpp>
#include <utxoz/utils.hpp>

#include "detail/log.hpp"

// Compatibility layer to adapt original benchmark code
namespace utxoz {

// Make the internal logging available
using detail::log_print;

// Alias for compatibility with original code
using utxo_key_t = key_t;

// Set log file function
void set_log_file(std::ofstream* file) {
    detail::set_log_file(file);
}

} // namespace utxoz

// Include original benchmark logic
// Note: You would need to modify bench_db.cpp to use the new API
// This is a template showing how the integration would work

namespace adapted_benchmark {

// Adapted version of your process_in_block function
struct adapted_output {
    std::vector<uint8_t> data;
    
    std::vector<uint8_t> to_data() const {
        return data;
    }
};

struct adapted_input {
    utxoz::key_t previous_key;
};

struct adapted_transaction {
    utxoz::key_t tx_hash;
    std::vector<adapted_output> outputs;
    std::vector<adapted_input> inputs;
    
    utxoz::key_t hash() const {
        return tx_hash;
    }
};

bool is_op_return(adapted_output const& output, uint32_t height) {
    return !output.data.empty() && output.data[0] == 0x6a;
}

// Simplified bucket calculation
size_t calculate_buckets(size_t n) {
    return utxoz::calculate_optimal_buckets(n);
}

// Process a block of transactions (adapted version)
std::tuple<
    std::unordered_map<utxoz::key_t, adapted_output>,
    std::unordered_map<utxoz::key_t, adapted_input>,
    size_t
> 
process_in_block(std::vector<adapted_transaction>& txs, uint32_t height) {
    using to_insert_utxos_t = std::unordered_map<utxoz::key_t, adapted_output>;
    using to_delete_utxos_t = std::unordered_map<utxoz::key_t, adapted_input>;
    using op_return_utxos_t = std::unordered_set<utxoz::key_t>;
    
    to_insert_utxos_t to_insert;
    op_return_utxos_t op_returns_to_store;
    
    // Insert all outputs
    for (auto const& tx : txs) {
        auto tx_hash = tx.hash();
        utxoz::key_t current_key = tx_hash;
        
        size_t output_index = 0;
        for (auto&& output : tx.outputs) {
            // Copy output index into key
            std::memcpy(current_key.data() + 32, &output_index, 4);
            ++output_index;
            
            if (is_op_return(output, height)) {
                op_returns_to_store.emplace(current_key);
                continue;
            }
            
            to_insert.emplace(current_key, std::move(output));
        }
    }
    
    size_t in_block_utxos = 0;
    to_delete_utxos_t to_delete;
    
    // Process inputs
    for (auto const& tx : txs) {
        for (auto&& input : tx.inputs) {
            auto const& key_to_remove = input.previous_key;
            
            auto removed = to_insert.erase(key_to_remove);
            if (removed == 0) {
                removed = op_returns_to_store.erase(key_to_remove);
                if (removed == 0) {
                    to_delete.emplace(key_to_remove, std::move(input));
                } else {
                    in_block_utxos += removed;
                }
            } else {
                in_block_utxos += removed;
            }
        }
    }
    
    return {std::move(to_insert), std::move(to_delete), in_block_utxos};
}

} // namespace adapted_benchmark

// Example of how to run the adapted benchmark
int run_adapted_benchmark() {
    std::cout << "Running adapted benchmark with new UTXO library\n";
    
    // Initialize logging
    std::ofstream log_file("adapted_benchmark.log");
    utxoz::set_log_file(&log_file);
    
    try {
        // Create database
        utxoz::db db;
        utxoz::log_print("Opening database...\n");
        db.configure("utxo_adapted_benchmark", true);
        utxoz::log_print("Database opened with size: {}\n", db.size());
        
        // Run benchmark simulation
        size_t height = 0;
        
        // Generate some mock data and process
        for (int block = 0; block < 10; ++block) {
            utxoz::log_print("Processing block {}...\n", block);
            
            // Here you would integrate with your actual data processing
            // This is just a skeleton showing the structure
            
            // Example: Insert some UTXOs
            for (int i = 0; i < 100; ++i) {
                auto key = utxoz::make_key(
                    std::span<uint8_t const, 32>{}, // tx hash
                    i  // output index
                );
                std::vector<uint8_t> value{1, 2, 3, 4, 5}; // sample data
                db.insert(key, value, height);
            }
            
            // Process pending deletions
            if (height % 5 == 0) {
                auto [deleted, failed] = db.process_pending_deletions();
                utxoz::log_print("Processed deletions: {} successful, {} failed\n", 
                               deleted, failed.size());
            }
            
            // Compact periodically
            if (height % 20 == 0 && height > 0) {
                db.compact_all();
            }
            
            ++height;
        }
        
        // Final statistics
        utxoz::log_print("\n=== Final Statistics ===\n");
        db.print_statistics();
        
        db.close();
        utxoz::log_print("Database closed successfully\n");
        
    } catch (std::exception const& e) {
        utxoz::log_print("Error: {}\n", e.what());
        return 1;
    }
    
    return 0;
}

int main() {
    return run_adapted_benchmark();
}
