/**
 * @file logging_example.cpp
 * @brief Example: logging backend demonstration
 *
 * Build with different backends to see the difference:
 *   ./scripts/rebuild.sh Release custom   -> callback-based logging
 *   ./scripts/rebuild.sh Release spdlog   -> spdlog logging
 *   ./scripts/rebuild.sh Release none     -> no logging output
 */

#include <array>
#include <fstream>
#include <vector>

#include <fmt/core.h>

#include <utxoz/config.hpp>
#include <utxoz/database.hpp>
#include <utxoz/logging.hpp>
#include <utxoz/utils.hpp>

void do_some_operations(utxoz::db& db, uint32_t base_height) {
    for (int i = 0; i < 50; ++i) {
        std::array<uint8_t, 32> tx_hash{};
        tx_hash[0] = static_cast<uint8_t>(i);
        tx_hash[1] = static_cast<uint8_t>(base_height & 0xFF);
        auto key = utxoz::make_key(tx_hash, static_cast<uint32_t>(i));
        std::vector<uint8_t> value{1, 2, 3, 4, 5};
        [[maybe_unused]] auto inserted = db.insert(key, value, base_height);
    }
    db.print_statistics();
}

int main() {
    fmt::println("UTXO-Z Logging Example");
    fmt::println("======================\n");

#if defined(UTXOZ_LOG_CUSTOM)
    fmt::println("Backend: CUSTOM (callback)");
#elif defined(UTXOZ_LOG_SPDLOG)
    fmt::println("Backend: SPDLOG");
#elif defined(UTXOZ_LOG_NONE)
    fmt::println("Backend: NONE (disabled)");
#endif

    fmt::println("");

    try {
        utxoz::db db;
        db.configure_for_testing("./logging_example_data", true);

#ifdef UTXOZ_LOG_CUSTOM
        // =====================================================================
        // Custom backend: demonstrate callback usage
        // =====================================================================
        fmt::println("=== Part 1: No callback (silent) ===\n");

        do_some_operations(db, 100);

        fmt::println("\n(no library output above)\n");

        fmt::println("=== Part 2: With callback ===\n");

        std::ofstream log_file("logging_example.log");

        utxoz::set_log_callback([&log_file](utxoz::log_level level, std::string_view msg) {
            // Add [utxoz] prefix to differentiate from other modules in a node
            fmt::println("[utxoz] [{}] {}", utxoz::log_level_name(level), msg);
            if (log_file.is_open()) {
                log_file << "[utxoz] [" << utxoz::log_level_name(level) << "] " << msg << "\n";
            }
        });

        do_some_operations(db, 200);

        fmt::println("\n(library output shown above with [utxoz] prefix)\n");

        fmt::println("=== Part 3: Callback cleared ===\n");
        utxoz::set_log_callback(nullptr);

        do_some_operations(db, 300);

        fmt::println("\n(no library output above)\n");

#elif defined(UTXOZ_LOG_SPDLOG)
        // =====================================================================
        // Spdlog backend: messages go directly to spdlog
        // =====================================================================
        fmt::println("=== Part 1: No prefix ===\n");

        do_some_operations(db, 100);

        fmt::println("\n(spdlog output without prefix)\n");

        fmt::println("=== Part 2: With prefix ===\n");

        utxoz::set_log_prefix("utxoz");

        do_some_operations(db, 200);

        fmt::println("\n(spdlog output with [utxoz] prefix)\n");

        fmt::println("=== Part 3: Prefix cleared ===\n");

        utxoz::set_log_prefix("");

        do_some_operations(db, 300);

        fmt::println("\n(spdlog output without prefix)\n");

#elif defined(UTXOZ_LOG_NONE)
        // =====================================================================
        // None backend: all logging is compiled out
        // =====================================================================
        fmt::println("Logging is disabled, no library output:\n");

        do_some_operations(db, 100);

        fmt::println("\n(no output - logging compiled out)\n");
#endif

        db.close();

    } catch (std::exception const& e) {
        fmt::println(stderr, "Error: {}", e.what());
        return 1;
    }

    fmt::println("Done.");

    return 0;
}
