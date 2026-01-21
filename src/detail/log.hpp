// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file log.hpp
 * @brief Internal logging system with configurable backends
 * @internal
 */

#pragma once

#include <string_view>

#include <utxoz/config.hpp>

#include <fmt/core.h>
#include <fmt/format.h>

#if defined(UTXOZ_LOG_SPDLOG)
#include <spdlog/spdlog.h>
#include <utxoz/logging.hpp>
#endif

namespace utxoz::log {

// =============================================================================
// Backend: spdlog
// =============================================================================
#if defined(UTXOZ_LOG_SPDLOG)

namespace detail {
inline std::string format_with_prefix(std::string_view msg) {
    auto const& prefix = get_log_prefix();
    if (prefix.empty()) {
        return std::string(msg);
    }
    return fmt::format("[{}] {}", prefix, msg);
}
} // namespace detail

template<typename... Args>
void trace(fmt::format_string<Args...> format_str, Args&&... args) {
    spdlog::trace("{}", detail::format_with_prefix(fmt::format(format_str, std::forward<Args>(args)...)));
}

template<typename... Args>
void debug(fmt::format_string<Args...> format_str, Args&&... args) {
    spdlog::debug("{}", detail::format_with_prefix(fmt::format(format_str, std::forward<Args>(args)...)));
}

template<typename... Args>
void info(fmt::format_string<Args...> format_str, Args&&... args) {
    spdlog::info("{}", detail::format_with_prefix(fmt::format(format_str, std::forward<Args>(args)...)));
}

template<typename... Args>
void warn(fmt::format_string<Args...> format_str, Args&&... args) {
    spdlog::warn("{}", detail::format_with_prefix(fmt::format(format_str, std::forward<Args>(args)...)));
}

template<typename... Args>
void error(fmt::format_string<Args...> format_str, Args&&... args) {
    spdlog::error("{}", detail::format_with_prefix(fmt::format(format_str, std::forward<Args>(args)...)));
}

template<typename... Args>
void critical(fmt::format_string<Args...> format_str, Args&&... args) {
    spdlog::critical("{}", detail::format_with_prefix(fmt::format(format_str, std::forward<Args>(args)...)));
}

// =============================================================================
// Backend: Custom callback
// =============================================================================
#elif defined(UTXOZ_LOG_CUSTOM)

enum class level { trace, debug, info, warn, error, critical };

extern void dispatch_log_message(level lvl, std::string_view msg);

template<typename... Args>
void trace(fmt::format_string<Args...> format_str, Args&&... args) {
    dispatch_log_message(level::trace, fmt::format(format_str, std::forward<Args>(args)...));
}

template<typename... Args>
void debug(fmt::format_string<Args...> format_str, Args&&... args) {
    dispatch_log_message(level::debug, fmt::format(format_str, std::forward<Args>(args)...));
}

template<typename... Args>
void info(fmt::format_string<Args...> format_str, Args&&... args) {
    dispatch_log_message(level::info, fmt::format(format_str, std::forward<Args>(args)...));
}

template<typename... Args>
void warn(fmt::format_string<Args...> format_str, Args&&... args) {
    dispatch_log_message(level::warn, fmt::format(format_str, std::forward<Args>(args)...));
}

template<typename... Args>
void error(fmt::format_string<Args...> format_str, Args&&... args) {
    dispatch_log_message(level::error, fmt::format(format_str, std::forward<Args>(args)...));
}

template<typename... Args>
void critical(fmt::format_string<Args...> format_str, Args&&... args) {
    dispatch_log_message(level::critical, fmt::format(format_str, std::forward<Args>(args)...));
}

// =============================================================================
// Backend: None (no-op)
// =============================================================================
#elif defined(UTXOZ_LOG_NONE)

template<typename... Args>
void trace([[maybe_unused]] fmt::format_string<Args...> format_str,
           [[maybe_unused]] Args&&... args) {}

template<typename... Args>
void debug([[maybe_unused]] fmt::format_string<Args...> format_str,
           [[maybe_unused]] Args&&... args) {}

template<typename... Args>
void info([[maybe_unused]] fmt::format_string<Args...> format_str,
          [[maybe_unused]] Args&&... args) {}

template<typename... Args>
void warn([[maybe_unused]] fmt::format_string<Args...> format_str,
          [[maybe_unused]] Args&&... args) {}

template<typename... Args>
void error([[maybe_unused]] fmt::format_string<Args...> format_str,
           [[maybe_unused]] Args&&... args) {}

template<typename... Args>
void critical([[maybe_unused]] fmt::format_string<Args...> format_str,
              [[maybe_unused]] Args&&... args) {}

#else
#error "No logging backend defined. Check config.hpp generation."
#endif

} // namespace utxoz::log
