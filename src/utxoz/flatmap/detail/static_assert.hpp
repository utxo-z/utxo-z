// Copyright 2023 Christian Mazakas
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef UTXOZ_FLATMAP_DETAIL_STATIC_ASSERT_HPP
#define UTXOZ_FLATMAP_DETAIL_STATIC_ASSERT_HPP

#include <boost/config.hpp>
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#define UTXOZ_FLATMAP_STATIC_ASSERT(...)                                     \
  static_assert(__VA_ARGS__, #__VA_ARGS__)

#endif // UTXOZ_FLATMAP_DETAIL_STATIC_ASSERT_HPP
