// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file log.cpp
 * @brief Logging implementation
 */

#include "detail/log.hpp"

namespace utxoz::detail {

std::ofstream* log_file_ptr = nullptr;

void set_log_file(std::ofstream* file) {
    log_file_ptr = file;
}

} // namespace utxoz::detail
