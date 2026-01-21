// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file logging.hpp
 * @brief Public logging API
 *
 * The logging backend is selected at build time:
 * - UTXOZ_LOG_CUSTOM: User-provided callback (default)
 * - UTXOZ_LOG_SPDLOG: spdlog library
 * - UTXOZ_LOG_NONE: No-op (logging disabled)
 *
 * When built with log=custom, users can provide a custom logging callback:
 * @code
 * #include <utxoz/logging.hpp>
 * #ifdef UTXOZ_LOG_CUSTOM
 * utxoz::set_log_callback([](utxoz::log_level level, std::string_view msg) {
 *     fmt::println("[{}] {}", utxoz::log_level_name(level), msg);
 * });
 * #endif
 * @endcode
 */

#pragma once

#include <utxoz/config.hpp>

#ifdef UTXOZ_LOG_SPDLOG

#include <string>
#include <string_view>

namespace utxoz {

/**
 * @brief Set the log prefix for spdlog messages
 *
 * When set, all log messages will be prefixed with [prefix].
 * Example: set_log_prefix("utxoz") will produce "[utxoz] message"
 *
 * @param prefix The prefix string, or empty to disable prefix
 */
void set_log_prefix(std::string prefix);

/**
 * @brief Get the current log prefix
 *
 * @return The current prefix, or empty string if none is set
 */
std::string const& get_log_prefix();

} // namespace utxoz

#endif // UTXOZ_LOG_SPDLOG

#ifdef UTXOZ_LOG_CUSTOM

#include <functional>
#include <string_view>

namespace utxoz {

/**
 * @brief Log level enumeration
 */
enum class log_level { trace, debug, info, warn, error, critical };

/**
 * @brief Get the name of a log level
 */
constexpr char const* log_level_name(log_level level) {
    switch (level) {
        case log_level::trace:    return "trace";
        case log_level::debug:    return "debug";
        case log_level::info:     return "info";
        case log_level::warn:     return "warn";
        case log_level::error:    return "error";
        case log_level::critical: return "critical";
    }
    return "unknown";
}

/**
 * @brief Log callback function type
 *
 * The callback receives the log level and message.
 */
using log_callback_t = std::function<void(log_level, std::string_view)>;

/**
 * @brief Set the logging callback
 *
 * @param callback The callback function, or nullptr to disable logging
 *
 * @note Thread-safety: Setting the callback while logging is occurring
 *       from other threads may result in undefined behavior. Set the
 *       callback before starting database operations.
 */
void set_log_callback(log_callback_t callback);

/**
 * @brief Get the current logging callback
 *
 * @return The current callback, or nullptr if none is set
 */
log_callback_t get_log_callback();

} // namespace utxoz

#endif // UTXOZ_LOG_CUSTOM
