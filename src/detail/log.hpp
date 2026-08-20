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

namespace detail {

/**
 * @brief Every emission goes through here, and nothing gets out.
 *
 * A log is never worth a decision. Two things behind these functions can throw:
 * formatting allocates, and the emission ends in code this library does not own
 * — a spdlog sink, or a callback the caller supplied and this library cannot
 * constrain. Either one propagating turns the operation being described into a
 * different outcome than the one it reached.
 *
 * That is not hypothetical. `new_version()` logs, and `rotate_for()` wraps it in
 * `catch (std::exception const&)` because a rotation that cannot make its file
 * throws. With a callback that refused, the log at the end of a *successful*
 * rotation was caught by that handler and reported as `file_open_failed`: a
 * generation that existed on disk, announced as one that could not be made. The
 * insert then refused instead of retrying, and the instance latched itself out
 * of the size class for good — all because a log line failed.
 *
 * So the contract is that a call into `utxoz::log` completes. Arguments are
 * still evaluated by the caller, so a caller that builds an allocating argument
 * — `outpoint_to_string`, a preformatted `fmt::format` — has to guard that
 * itself; `detail::diagnose` in insert_transition.hpp is that guard.
 */
template <typename Emit>
void guarded(Emit&& emit) noexcept {
    try {
        emit();
    } catch (...) {
        // Deliberately silent. The only way to report that reporting failed is
        // to report, which is what just failed.
    }
}

} // namespace detail


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
void trace(fmt::format_string<Args...> format_str, Args&&... args) noexcept {
    detail::guarded([&] {
        spdlog::trace("{}", detail::format_with_prefix(
            fmt::format(format_str, std::forward<Args>(args)...)));
    });
}

template<typename... Args>
void debug(fmt::format_string<Args...> format_str, Args&&... args) noexcept {
    detail::guarded([&] {
        spdlog::debug("{}", detail::format_with_prefix(
            fmt::format(format_str, std::forward<Args>(args)...)));
    });
}

template<typename... Args>
void info(fmt::format_string<Args...> format_str, Args&&... args) noexcept {
    detail::guarded([&] {
        spdlog::info("{}", detail::format_with_prefix(
            fmt::format(format_str, std::forward<Args>(args)...)));
    });
}

template<typename... Args>
void warn(fmt::format_string<Args...> format_str, Args&&... args) noexcept {
    detail::guarded([&] {
        spdlog::warn("{}", detail::format_with_prefix(
            fmt::format(format_str, std::forward<Args>(args)...)));
    });
}

template<typename... Args>
void error(fmt::format_string<Args...> format_str, Args&&... args) noexcept {
    detail::guarded([&] {
        spdlog::error("{}", detail::format_with_prefix(
            fmt::format(format_str, std::forward<Args>(args)...)));
    });
}

template<typename... Args>
void critical(fmt::format_string<Args...> format_str, Args&&... args) noexcept {
    detail::guarded([&] {
        spdlog::critical("{}", detail::format_with_prefix(
            fmt::format(format_str, std::forward<Args>(args)...)));
    });
}

// =============================================================================
// Backend: Custom callback
// =============================================================================
#elif defined(UTXOZ_LOG_CUSTOM)

enum class level { trace, debug, info, warn, error, critical };

extern void dispatch_log_message(level lvl, std::string_view msg);

template<typename... Args>
void trace(fmt::format_string<Args...> format_str, Args&&... args) noexcept {
    detail::guarded([&] {
        dispatch_log_message(level::trace,
                             fmt::format(format_str, std::forward<Args>(args)...));
    });
}

template<typename... Args>
void debug(fmt::format_string<Args...> format_str, Args&&... args) noexcept {
    detail::guarded([&] {
        dispatch_log_message(level::debug,
                             fmt::format(format_str, std::forward<Args>(args)...));
    });
}

template<typename... Args>
void info(fmt::format_string<Args...> format_str, Args&&... args) noexcept {
    detail::guarded([&] {
        dispatch_log_message(level::info,
                             fmt::format(format_str, std::forward<Args>(args)...));
    });
}

template<typename... Args>
void warn(fmt::format_string<Args...> format_str, Args&&... args) noexcept {
    detail::guarded([&] {
        dispatch_log_message(level::warn,
                             fmt::format(format_str, std::forward<Args>(args)...));
    });
}

template<typename... Args>
void error(fmt::format_string<Args...> format_str, Args&&... args) noexcept {
    detail::guarded([&] {
        dispatch_log_message(level::error,
                             fmt::format(format_str, std::forward<Args>(args)...));
    });
}

template<typename... Args>
void critical(fmt::format_string<Args...> format_str, Args&&... args) noexcept {
    detail::guarded([&] {
        dispatch_log_message(level::critical,
                             fmt::format(format_str, std::forward<Args>(args)...));
    });
}

// =============================================================================
// Backend: None (no-op)
// =============================================================================
#elif defined(UTXOZ_LOG_NONE)

template<typename... Args>
void trace([[maybe_unused]] fmt::format_string<Args...> format_str,
           [[maybe_unused]] Args&&... args) noexcept {}

template<typename... Args>
void debug([[maybe_unused]] fmt::format_string<Args...> format_str,
           [[maybe_unused]] Args&&... args) noexcept {}

template<typename... Args>
void info([[maybe_unused]] fmt::format_string<Args...> format_str,
          [[maybe_unused]] Args&&... args) noexcept {}

template<typename... Args>
void warn([[maybe_unused]] fmt::format_string<Args...> format_str,
          [[maybe_unused]] Args&&... args) noexcept {}

template<typename... Args>
void error([[maybe_unused]] fmt::format_string<Args...> format_str,
           [[maybe_unused]] Args&&... args) noexcept {}

template<typename... Args>
void critical([[maybe_unused]] fmt::format_string<Args...> format_str,
              [[maybe_unused]] Args&&... args) noexcept {}

#else
#error "No logging backend defined. Check config.hpp generation."
#endif

} // namespace utxoz::log
