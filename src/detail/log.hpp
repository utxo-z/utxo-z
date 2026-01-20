// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file log.hpp
 * @brief Internal logging system
 * @internal
 */

#pragma once

#include <fstream>

#include <fmt/core.h>
#include <fmt/format.h>

namespace utxoz::detail {

/**
 * @brief Internal logging function
 * @internal
 */
template<typename... Args>
void log_print(fmt::format_string<Args...> format_str, Args&&... args) {
    auto msg = fmt::format(format_str, std::forward<Args>(args)...);
    fmt::print("{}", msg);

    // Also log to file if available
    extern std::ofstream* log_file_ptr;
    if (log_file_ptr && log_file_ptr->is_open()) {
        *log_file_ptr << msg;
        log_file_ptr->flush();
    }
}

/**
 * @brief Set log file for output
 * @internal
 */
void set_log_file(std::ofstream* file);

} // namespace utxoz::detail

// Convenience macro for internal logging
#define UTXO_LOG(...) ::utxoz::detail::log_print(__VA_ARGS__)
