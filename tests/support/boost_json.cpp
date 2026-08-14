// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file boost_json.cpp
 * @brief The one translation unit that compiles Boost.JSON.
 *
 * `boost/json/src.hpp` is the library's implementation, not a header: including
 * it twice gives two definitions of everything in it, and the link fails on
 * symbols nobody wrote. It lives here alone so that any number of tests can use
 * `<boost/json.hpp>` without arranging among themselves which one carries it.
 */

#include <boost/json/src.hpp>
