// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file sync_barrier.hpp
 * @brief A whole durability barrier in one call, for tests with nothing to
 *        run between its halves.
 */

#pragma once

#include <expected>
#include <utility>

#include <utxoz/database.hpp>
#include <utxoz/types.hpp>

namespace utxoz::testing {

/// begin_sync() followed at once by finish() on what it returned. The error is
/// whichever half's it was: a first half that failed is reported as it failed,
/// and the second is then not attempted.
[[nodiscard]]
inline result<> sync_barrier(db_base& db) {
    auto pending = db.begin_sync();
    if ( ! pending) return std::unexpected(pending.error());
    return std::move(*pending).finish();
}

} // namespace utxoz::testing
