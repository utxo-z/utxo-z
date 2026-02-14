
// Copyright (C) 2022 Christian Mazakas
// Copyright (C) 2024 Braden Ganetsky
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef UTXOZ_FLATMAP_FLAT_MAP_FWD_HPP_INCLUDED
#define UTXOZ_FLATMAP_FLAT_MAP_FWD_HPP_INCLUDED

#include <boost/config.hpp>
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <boost/container_hash/hash_fwd.hpp>
#include <functional>
#include <memory>

#ifndef BOOST_NO_CXX17_HDR_MEMORY_RESOURCE
#include <memory_resource>
#endif

namespace utxoz {
  namespace flatmap {
    template <class Key, class T, class Hash = boost::hash<Key>,
      class KeyEqual = std::equal_to<Key>,
      class Allocator = std::allocator<std::pair<const Key, T> > >
    class mmap_flat_map;

    template <class Key, class T, class Hash, class KeyEqual, class Allocator>
    bool operator==(
      mmap_flat_map<Key, T, Hash, KeyEqual, Allocator> const& lhs,
      mmap_flat_map<Key, T, Hash, KeyEqual, Allocator> const& rhs);

    template <class Key, class T, class Hash, class KeyEqual, class Allocator>
    bool operator!=(
      mmap_flat_map<Key, T, Hash, KeyEqual, Allocator> const& lhs,
      mmap_flat_map<Key, T, Hash, KeyEqual, Allocator> const& rhs);

    template <class Key, class T, class Hash, class KeyEqual, class Allocator>
    void swap(mmap_flat_map<Key, T, Hash, KeyEqual, Allocator>& lhs,
      mmap_flat_map<Key, T, Hash, KeyEqual, Allocator>& rhs)
      noexcept(noexcept(lhs.swap(rhs)));

#ifndef BOOST_NO_CXX17_HDR_MEMORY_RESOURCE
    namespace pmr {
      template <class Key, class T, class Hash = boost::hash<Key>,
        class KeyEqual = std::equal_to<Key> >
      using mmap_flat_map =
        utxoz::flatmap::mmap_flat_map<Key, T, Hash, KeyEqual,
          std::pmr::polymorphic_allocator<std::pair<const Key, T> > >;
    } // namespace pmr
#endif
  } // namespace flatmap

  using utxoz::flatmap::mmap_flat_map;
} // namespace utxoz

#endif
