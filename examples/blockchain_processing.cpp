// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file blockchain_processing.cpp
 * @brief Blockchain processing example adapted from original benchmark
 */

#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/core.h>

#include <utxoz/database.hpp>
#include <utxoz/utils.hpp>

// Logging utilities
std::ofstream log_file;

void init_log_file(std::string const& benchmark_name) {
    auto now = std::chrono::system_clock::now();
    auto filename = fmt::format("blockchain_processing_{}_{:%Y%m%d_%H%M%S}.log",
                                benchmark_name, now);

    log_file.open(filename);
    if (log_file.is_open()) {
        auto msg = fmt::format("Log started at: {:%Y-%m-%d %H:%M:%S}\n", now);
        fmt::print("{}", msg);
        log_file << msg;

        msg = fmt::format("Benchmark: {}\n", benchmark_name);
        fmt::print("{}", msg);
        log_file << msg;
    }
}

void close_log_file() {
    if (log_file.is_open()) {
        fmt::println("Log completed.");
        log_file << "Log completed.\n";
        log_file.close();
    }
}

template<typename... Args>
void log_print(fmt::format_string<Args...> format_str, Args&&... args) {
    auto msg = fmt::format(format_str, std::forward<Args>(args)...);
    fmt::print("{}", msg);
    if (log_file.is_open()) {
        log_file << msg;
        log_file.flush();
    }
}

// Mock transaction structures (simplified)
struct mock_output {
    std::vector<uint8_t> script_bytes;
    uint64_t value;

    std::vector<uint8_t> to_data() const {
        std::vector<uint8_t> data;
        data.reserve(script_bytes.size() + 8);

        // Add value (8 bytes)
        for (int i = 0; i < 8; ++i) {
            data.push_back((value >> (i * 8)) & 0xFF);
        }

        // Add script
        data.insert(data.end(), script_bytes.begin(), script_bytes.end());
        return data;
    }
};

struct mock_input {
    utxoz::raw_outpoint previous_key;
    std::vector<uint8_t> script_sig;
};

struct mock_transaction {
    utxoz::raw_outpoint tx_key;
    std::vector<mock_output> outputs;
    std::vector<mock_input> inputs;

    utxoz::raw_outpoint hash() const {
        return tx_key; // Simplified
    }
};

bool is_op_return(mock_output const& output) {
    return !output.script_bytes.empty() && output.script_bytes[0] == 0x6a;
}

size_t calculate_buckets(size_t n) {
    if (n == 0) return 0;
    if (n < 15) return 29;

    constexpr float max_load = 0.88f;
    constexpr float margin = 0.95f;
    constexpr float safe_load = max_load * margin;

    size_t required_buckets = static_cast<size_t>(std::ceil(n / safe_load));
    size_t k = (required_buckets + 1) / 15;
    size_t ceil_pow2 = 1;
    while (ceil_pow2 < k) ceil_pow2 <<= 1;

    return ceil_pow2 * 15 - 1;
}

// Generate mock blockchain data
std::vector<mock_transaction> generate_mock_block(size_t tx_count, [[maybe_unused]] uint32_t block_height) {
    std::vector<mock_transaction> transactions;
    transactions.reserve(tx_count);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> byte_dist(0, 255);
    std::uniform_int_distribution<size_t> output_count_dist(1, 5);
    std::uniform_int_distribution<size_t> input_count_dist(1, 3);
    std::uniform_int_distribution<size_t> script_size_dist(20, 100);
    std::uniform_int_distribution<uint64_t> value_dist(1000, 100000000);

    for (size_t i = 0; i < tx_count; ++i) {
        mock_transaction tx;

        // Generate transaction hash
        for (auto& byte : tx.tx_key) {
            byte = byte_dist(gen);
        }

        // Generate outputs
        size_t output_count = output_count_dist(gen);
        for (size_t j = 0; j < output_count; ++j) {
            mock_output output;

            // 5% chance of OP_RETURN
            if (gen() % 100 < 5) {
                output.script_bytes = {0x6a}; // OP_RETURN
                output.value = 0;
            } else {
                size_t script_size = script_size_dist(gen);
                output.script_bytes.resize(script_size);
                for (auto& byte : output.script_bytes) {
                    byte = byte_dist(gen);
                }
                output.value = value_dist(gen);
            }

            tx.outputs.push_back(std::move(output));
        }

        // Generate inputs (except for coinbase)
        if (i > 0) {
            size_t input_count = input_count_dist(gen);
            for (size_t j = 0; j < input_count; ++j) {
                mock_input input;

                // Generate random previous output reference
                for (auto& byte : input.previous_key) {
                    byte = byte_dist(gen);
                }

                // Set output index
                uint32_t output_index = static_cast<uint32_t>(j);
                std::memcpy(input.previous_key.data() + 32, &output_index, 4);

                tx.inputs.push_back(std::move(input));
            }
        }

        transactions.push_back(std::move(tx));
    }

    return transactions;
}

int main() {
    fmt::println("UTXO Database Blockchain Processing Example");
    fmt::println("==========================================\n");

    init_log_file("blockchain_processing");

    try {
        // Initialize database
        log_print("Opening database...\n");
        auto r = utxoz::db::open("utxo_blockchain_example", true);
        if (!r) {
            log_print("Error: failed to open database\n");
            close_log_file();
            return 1;
        }
        auto& db = *r;
        log_print("Database opened with size: {}\n", db.size());

        // Simulation parameters
        constexpr size_t num_blocks = 10;
        constexpr size_t transactions_per_block = 100;

        size_t total_insertions = 0;
        size_t total_deletions = 0;

        // The deletion batch belongs to this loop. UTXO-Z stores no pending
        // deletions, so anything not in here is not scheduled for removal.
        std::vector<utxoz::deferred_deletion_entry> pending_deletes;

        // Process blocks
        for (uint32_t block_height = 0; block_height < num_blocks; ++block_height) {
            log_print("Processing block {}...\n", block_height);

            // Generate mock block
            auto transactions = generate_mock_block(transactions_per_block, block_height);

            // Process transactions
            std::unordered_map<utxoz::raw_outpoint, mock_output> to_insert;
            std::unordered_set<utxoz::raw_outpoint> to_delete;
            std::unordered_set<utxoz::raw_outpoint> op_returns;

            // First pass: collect all outputs
            for (auto const& tx : transactions) {
                for (size_t i = 0; i < tx.outputs.size(); ++i) {
                    auto key = tx.tx_key;
                    uint32_t output_index = static_cast<uint32_t>(i);
                    std::memcpy(key.data() + 32, &output_index, 4);

                    if (is_op_return(tx.outputs[i])) {
                        op_returns.insert(key);
                    } else {
                        to_insert[key] = tx.outputs[i];
                    }
                }
            }

            // Second pass: process inputs
            size_t in_block_spends = 0;
            for (auto const& tx : transactions) {
                for (auto const& input : tx.inputs) {
                    auto key = input.previous_key;

                    // Check if spending from this block's outputs
                    if (to_insert.erase(key) > 0) {
                        ++in_block_spends;
                    } else if (op_returns.erase(key) > 0) {
                        ++in_block_spends;
                    } else {
                        // External spend
                        to_delete.insert(key);
                    }
                }
            }

            log_print("Block {} analysis:\n", block_height);
            log_print("  Regular outputs: {}\n", to_insert.size());
            log_print("  OP_RETURN outputs: {}\n", op_returns.size());
            log_print("  External deletes: {}\n", to_delete.size());
            log_print("  In-block spends: {}\n", in_block_spends);

            // Collect the block's deletions. They stay here — UTXO-Z keeps no
            // queue of its own — and are applied below.
            for (auto const& key : to_delete) {
                pending_deletes.emplace_back(key, block_height);
                ++total_deletions;
            }

            // Insert new UTXOs
            for (auto const& [key, output] : to_insert) {
                auto data = output.to_data();
                (void)db.insert(key, data, block_height);
                ++total_insertions;
            }

            // Apply the accumulated batch periodically.
            if (block_height % 5 == 0 && ! pending_deletes.empty()) {
                log_print("Applying {} deletions...\n", pending_deletes.size());
                auto const progress = db.apply_deletes(pending_deletes);
                log_print("  Erased: {}\n", progress.erased.size());
                log_print("  Absent: {}\n", progress.absent.size());
                log_print("  Unresolved: {}\n", progress.unresolved.size());

                // Only the unresolved may be sent again; the rest are settled.
                pending_deletes = progress.unresolved;
                if (progress.error) {
                    log_print("  The batch did not complete; {} entries carried over\n",
                              pending_deletes.size());
                }
            }

            // Compact database periodically
            if (block_height % 10 == 0 && block_height > 0) {
                log_print("Compacting database...\n");
                if (auto compacted = db.compact_all(); !compacted) {
                    fmt::println("Error: compaction failed, the database is inconsistent");
                    return 1;
                }
            }

            // Print progress
            if (block_height % 5 == 0) {
                log_print("=== Statistics after block {} ===\n", block_height);
                db.print_statistics();
            }
        }

        // Final processing
        log_print("Applying the final {} deletions...\n", pending_deletes.size());
        auto const final_progress = db.apply_deletes(pending_deletes);
        log_print("Final erased: {}\n", final_progress.erased.size());
        log_print("Final absent: {}\n", final_progress.absent.size());
        log_print("Final unresolved: {}\n", final_progress.unresolved.size());

        // Same handling as the periodic path above: what could not be finished is
        // kept rather than dropped, and the failure is reported rather than
        // summarised away.
        pending_deletes = final_progress.unresolved;
        if (final_progress.error) {
            log_print("The final batch did not complete; {} deletions are still owed\n",
                      pending_deletes.size());
        }

        // Final compaction
        log_print("Final compaction...\n");
        if (auto compacted = db.compact_all(); !compacted) {
            fmt::println("Error: compaction failed, the database is inconsistent");
            return 1;
        }

        // Final statistics
        log_print("\n=== FINAL STATISTICS ===\n");
        db.print_statistics();

        auto stats = db.get_statistics();
        log_print("\nSummary:\n");
        log_print("  Blocks processed: {}\n", num_blocks);
        log_print("  Total insertions: {}\n", total_insertions);
        log_print("  Total deletions: {}\n", total_deletions);
        log_print("  Final UTXO count: {}\n", stats.total_entries);
        log_print("  Cache hit rate: {}%\n", stats.cache_hit_rate * 100);

        log_print("Processing completed successfully\n");

    } catch (std::exception const& e) {
        log_print("Error: {}\n", e.what());
        close_log_file();
        return 1;
    }

    close_log_file();
    return 0;
}
