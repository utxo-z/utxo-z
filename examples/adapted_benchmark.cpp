/**
 * @file adapted_benchmark.cpp
 * @brief Adapter to use the original benchmark with new library API
 */

#include <array>
#include <cstring>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <fmt/core.h>

#include <utxoz/config.hpp>
#include <utxoz/database.hpp>
#include <utxoz/logging.hpp>
#include <utxoz/utils.hpp>

// Global log file for the example
std::ofstream g_log_file;

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

bool is_op_return(adapted_output const& output, [[maybe_unused]] uint32_t height) {
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
    fmt::println("Running adapted benchmark with new UTXO library");

#ifdef UTXOZ_LOG_CUSTOM
    // Initialize logging with custom callback
    g_log_file.open("adapted_benchmark.log");
    utxoz::set_log_callback([](utxoz::log_level level, std::string_view msg) {
        // Add [utxoz] prefix to differentiate from other modules in a node
        fmt::println("[utxoz] [{}] {}", utxoz::log_level_name(level), msg);
        if (g_log_file.is_open()) {
            g_log_file << "[utxoz] [" << utxoz::log_level_name(level) << "] " << msg << "\n";
            g_log_file.flush();
        }
    });
#elif defined(UTXOZ_LOG_SPDLOG)
    // Set prefix to differentiate from other modules in a node
    utxoz::set_log_prefix("utxoz");
#endif

    try {
        // Create database
        utxoz::db db;
        fmt::println("Opening database...");
        db.configure("utxo_adapted_benchmark", true);
        fmt::println("Database opened with size: {}", db.size());

        // Run benchmark simulation
        size_t height = 0;

        // Generate some mock data and process
        for (int block = 0; block < 10; ++block) {
            fmt::println("Processing block {}...", block);

            // Here you would integrate with your actual data processing
            // This is just a skeleton showing the structure

            // Example: Insert some UTXOs
            for (int i = 0; i < 100; ++i) {
                std::array<uint8_t, 32> tx_hash{};
                tx_hash[0] = static_cast<uint8_t>(i);
                auto key = utxoz::make_key(tx_hash, static_cast<uint32_t>(i));
                std::vector<uint8_t> value{1, 2, 3, 4, 5}; // sample data
                [[maybe_unused]] auto inserted = db.insert(key, value, static_cast<uint32_t>(height));
            }

            // Process pending deletions
            if (height % 5 == 0) {
                auto [deleted, failed] = db.process_pending_deletions();
                fmt::println("Processed deletions: {} successful, {} failed", deleted, failed.size());
            }

            // Compact periodically
            if (height % 20 == 0 && height > 0) {
                db.compact_all();
            }

            ++height;
        }

        // Final statistics
        fmt::println("\n=== Final Statistics ===");
        db.print_statistics();

        db.close();
        fmt::println("Database closed successfully");

    } catch (std::exception const& e) {
        fmt::println(stderr, "Error: {}", e.what());
        return 1;
    }

    g_log_file.close();
    return 0;
}

int main() {
    return run_adapted_benchmark();
}
