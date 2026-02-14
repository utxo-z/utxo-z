/* Copyright 2022 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See https://www.boost.org/libs/unordered for library home page.
 */

#ifndef UTXOZ_FLATMAP_DETAIL_NARROW_CAST_HPP
#define UTXOZ_FLATMAP_DETAIL_NARROW_CAST_HPP

#include <utxoz/flatmap/detail/static_assert.hpp>

#include <boost/config.hpp>
#include <type_traits>

namespace utxoz{
namespace flatmap{
namespace detail{

template<typename To,typename From>
constexpr To narrow_cast(From x) noexcept
{
  UTXOZ_FLATMAP_STATIC_ASSERT(std::is_integral<From>::value);
  UTXOZ_FLATMAP_STATIC_ASSERT(std::is_integral<To>::value);
  UTXOZ_FLATMAP_STATIC_ASSERT(sizeof(From)>=sizeof(To));

  return static_cast<To>(
    x

#if defined(__MSVC_RUNTIME_CHECKS)
    /* Avoids VS's "Run-Time Check Failure #1 - A cast to a smaller data type
     * has caused a loss of data."
     */
    &static_cast<typename std::make_unsigned<To>::type>(~static_cast<To>(0))
#endif
  );
}

} /* namespace detail */
} /* namespace flatmap */
} /* namespace utxoz */

#endif
