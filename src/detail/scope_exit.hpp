// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file scope_exit.hpp
 * @brief Runs an action when the scope ends, however it ends.
 * @internal
 */

#pragma once

#include <exception>
#include <utility>

namespace utxoz::detail {

/**
 * @brief Runs an action when the scope ends, however it ends.
 *
 * For restoring an invariant that a function suspends while it works, where
 * every exit has to put it back — including the exit nobody writes, taken by a
 * throw. Compaction is the case that motivated it: it closes the active
 * container before it starts, and an exception on the way through would
 * otherwise leave the container null and turn the next operation into a null
 * dereference.
 *
 * The action must not throw. It may run while another exception is propagating,
 * where a second one would terminate the process, so anything escaping is
 * swallowed here.
 */
template <typename F>
class scope_exit {
public:
    explicit scope_exit(F f) : f_(std::move(f)) {}

    scope_exit(scope_exit const&) = delete;
    scope_exit& operator=(scope_exit const&) = delete;

    ~scope_exit() {
        try {
            f_();
        } catch (...) {
            // Nothing useful to do here, and rethrowing during unwinding ends
            // the process. Callers that need to know must not rely on this.
        }
    }

private:
    F f_;
};

} // namespace utxoz::detail
