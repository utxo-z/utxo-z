// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file log.cpp
 * @brief Logging implementation
 */

#include <utxoz/config.hpp>

#ifdef UTXOZ_LOG_SPDLOG

#include <utxoz/logging.hpp>

namespace utxoz {

namespace {
std::string g_log_prefix;
} // anonymous namespace

void set_log_prefix(std::string prefix) {
    g_log_prefix = std::move(prefix);
}

std::string const& get_log_prefix() {
    return g_log_prefix;
}

} // namespace utxoz

#endif // UTXOZ_LOG_SPDLOG

#ifdef UTXOZ_LOG_CUSTOM

#include <mutex>

#include <utxoz/logging.hpp>

#include "detail/log.hpp"

namespace utxoz {

namespace {
log_callback_t g_log_callback = nullptr;

/// Keeps the callback from being entered by two threads at once. Until batched
/// inserts, nothing the threading contract permits could log from two threads
/// together; a batch's workers can (a rotation logs), and the guarantee callers
/// had is kept rather than handed to them. Recursive, so a callback that logs
/// on its own thread does not wait for itself. Registration is unchanged: set
/// the callback before operations start.
std::recursive_mutex g_dispatch_mutex;
} // anonymous namespace

void set_log_callback(log_callback_t callback) {
    g_log_callback = std::move(callback);
}

log_callback_t get_log_callback() {
    return g_log_callback;
}

namespace log {

void dispatch_log_message(level lvl, std::string_view msg) {
    if (g_log_callback) {
        std::scoped_lock const lock(g_dispatch_mutex);
        // Convert internal level to public log_level
        g_log_callback(static_cast<log_level>(lvl), msg);
    }
}

} // namespace log

} // namespace utxoz

#endif // UTXOZ_LOG_CUSTOM
