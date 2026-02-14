// Copyright (C) 2022-2023 Christian Mazakas
// Copyright (C) 2024-2025 Joaquin M Lopez Munoz
// Copyright 2026 Fernando Pelliccioni.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef UTXOZ_FLATMAP_UNORDERED_FLAT_MAP_HPP_INCLUDED
#define UTXOZ_FLATMAP_UNORDERED_FLAT_MAP_HPP_INCLUDED

#include <boost/config.hpp>
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <utxoz/flatmap/concurrent_flat_map_fwd.hpp>
#include <utxoz/flatmap/detail/flat_map_types.hpp>
#include <utxoz/flatmap/detail/table.hpp>
#include <utxoz/flatmap/detail/throw_exception.hpp>
#include <utxoz/flatmap/detail/type_traits.hpp>
#include <utxoz/flatmap/mmap_flat_map_fwd.hpp>

#include <boost/core/allocator_access.hpp>
#include <boost/container_hash/hash.hpp>

#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace utxoz {
  namespace flatmap {

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable : 4714) /* marked as __forceinline not inlined */
#endif

    template <class Key, class T, class Hash, class KeyEqual, class Allocator>
    class mmap_flat_map
    {
      template <class Key2, class T2, class Hash2, class Pred2,
        class Allocator2>
      friend class concurrent_flat_map;

      using map_types = detail::foa::flat_map_types<Key, T>;

      using table_type = detail::foa::table<map_types, Hash, KeyEqual,
        typename boost::allocator_rebind<Allocator,
          typename map_types::value_type>::type>;

      table_type table_;

      template <class K, class V, class H, class KE, class A>
      bool friend operator==(mmap_flat_map<K, V, H, KE, A> const& lhs,
        mmap_flat_map<K, V, H, KE, A> const& rhs);

      template <class K, class V, class H, class KE, class A, class Pred>
      typename mmap_flat_map<K, V, H, KE, A>::size_type friend erase_if(
        mmap_flat_map<K, V, H, KE, A>& set, Pred pred);

    public:
      using key_type = Key;
      using mapped_type = T;
      using value_type = typename map_types::value_type;
      using init_type = typename map_types::init_type;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;
      using hasher = typename utxoz::flatmap::detail::type_identity<Hash>::type;
      using key_equal = typename utxoz::flatmap::detail::type_identity<KeyEqual>::type;
      using allocator_type = typename utxoz::flatmap::detail::type_identity<Allocator>::type;
      using reference = value_type&;
      using const_reference = value_type const&;
      using pointer = typename boost::allocator_pointer<allocator_type>::type;
      using const_pointer =
        typename boost::allocator_const_pointer<allocator_type>::type;
      using iterator = typename table_type::iterator;
      using const_iterator = typename table_type::const_iterator;

#if defined(UTXOZ_FLATMAP_ENABLE_STATS)
      using stats = typename table_type::stats;
#endif

      
      // === UTXO-Z: mmap factory methods ===

      /// Create a fresh map in an mmap buffer (initializes groups + sentinel).
      static mmap_flat_map create(
        void* buffer, std::size_t groups_size_index, std::size_t groups_size_mask,
        hasher const& h = hasher(), key_equal const& pred = key_equal())
      {
        mmap_flat_map m;
        m.table_ = table_type::create(buffer, groups_size_index, groups_size_mask, h, pred);
        return m;
      }

      /// Attach to an existing map already in an mmap buffer.
      static mmap_flat_map attach(
        void* buffer,
        std::size_t groups_size_index, std::size_t groups_size_mask,
        std::size_t size, std::size_t ml,
        hasher const& h = hasher(), key_equal const& pred = key_equal())
      {
        mmap_flat_map m;
        m.table_ = table_type::attach(buffer, groups_size_index, groups_size_mask, size, ml, h, pred);
        return m;
      }

      std::size_t get_groups_size_index() const noexcept {
        return table_.get_groups_size_index();
      }
      std::size_t get_groups_size_mask() const noexcept {
        return table_.get_groups_size_mask();
      }
      std::size_t get_max_load() const noexcept {
        return table_.get_max_load();
      }
      std::size_t capacity() const noexcept {
        return table_.capacity();
      }
      static std::size_t required_buffer_size(std::size_t groups_size_mask) noexcept {
        return table_type::required_buffer_size(groups_size_mask);
      }

      // === end UTXO-Z factory methods ===

      mmap_flat_map() : mmap_flat_map(0) {}

      explicit mmap_flat_map(size_type n, hasher const& h = hasher(),
        key_equal const& pred = key_equal(),
        allocator_type const& a = allocator_type())
          : table_(n, h, pred, a)
      {
      }

      mmap_flat_map(size_type n, allocator_type const& a)
          : mmap_flat_map(n, hasher(), key_equal(), a)
      {
      }

      mmap_flat_map(size_type n, hasher const& h, allocator_type const& a)
          : mmap_flat_map(n, h, key_equal(), a)
      {
      }

      template <class InputIterator>
      mmap_flat_map(
        InputIterator f, InputIterator l, allocator_type const& a)
          : mmap_flat_map(f, l, size_type(0), hasher(), key_equal(), a)
      {
      }

      explicit mmap_flat_map(allocator_type const& a)
          : mmap_flat_map(0, a)
      {
      }

      template <class Iterator>
      mmap_flat_map(Iterator first, Iterator last, size_type n = 0,
        hasher const& h = hasher(), key_equal const& pred = key_equal(),
        allocator_type const& a = allocator_type())
          : mmap_flat_map(n, h, pred, a)
      {
        this->insert(first, last);
      }

      template <class Iterator>
      mmap_flat_map(
        Iterator first, Iterator last, size_type n, allocator_type const& a)
          : mmap_flat_map(first, last, n, hasher(), key_equal(), a)
      {
      }

      template <class Iterator>
      mmap_flat_map(Iterator first, Iterator last, size_type n,
        hasher const& h, allocator_type const& a)
          : mmap_flat_map(first, last, n, h, key_equal(), a)
      {
      }

      mmap_flat_map(mmap_flat_map const& other) : table_(other.table_)
      {
      }

      mmap_flat_map(
        mmap_flat_map const& other, allocator_type const& a)
          : table_(other.table_, a)
      {
      }

      mmap_flat_map(mmap_flat_map&& other)
        noexcept(std::is_nothrow_move_constructible<table_type>::value)
          : table_(std::move(other.table_))
      {
      }

      mmap_flat_map(mmap_flat_map&& other, allocator_type const& al)
          : table_(std::move(other.table_), al)
      {
      }

      mmap_flat_map(std::initializer_list<value_type> ilist,
        size_type n = 0, hasher const& h = hasher(),
        key_equal const& pred = key_equal(),
        allocator_type const& a = allocator_type())
          : mmap_flat_map(ilist.begin(), ilist.end(), n, h, pred, a)
      {
      }

      mmap_flat_map(
        std::initializer_list<value_type> il, allocator_type const& a)
          : mmap_flat_map(il, size_type(0), hasher(), key_equal(), a)
      {
      }

      mmap_flat_map(std::initializer_list<value_type> init, size_type n,
        allocator_type const& a)
          : mmap_flat_map(init, n, hasher(), key_equal(), a)
      {
      }

      mmap_flat_map(std::initializer_list<value_type> init, size_type n,
        hasher const& h, allocator_type const& a)
          : mmap_flat_map(init, n, h, key_equal(), a)
      {
      }

      template <bool avoid_explicit_instantiation = true>
      mmap_flat_map(
        concurrent_flat_map<Key, T, Hash, KeyEqual, Allocator>&& other)
          : table_(std::move(other.table_))
      {
      }

      ~mmap_flat_map() = default;

      mmap_flat_map& operator=(mmap_flat_map const& other)
      {
        table_ = other.table_;
        return *this;
      }

      mmap_flat_map& operator=(mmap_flat_map&& other) noexcept(
        noexcept(std::declval<table_type&>() = std::declval<table_type&&>()))
      {
        table_ = std::move(other.table_);
        return *this;
      }

      mmap_flat_map& operator=(std::initializer_list<value_type> il)
      {
        this->clear();
        this->insert(il.begin(), il.end());
        return *this;
      }

      allocator_type get_allocator() const noexcept
      {
        return table_.get_allocator();
      }

      /// Iterators
      ///

      iterator begin() noexcept { return table_.begin(); }
      const_iterator begin() const noexcept { return table_.begin(); }
      const_iterator cbegin() const noexcept { return table_.cbegin(); }

      iterator end() noexcept { return table_.end(); }
      const_iterator end() const noexcept { return table_.end(); }
      const_iterator cend() const noexcept { return table_.cend(); }

      /// Capacity
      ///

      BOOST_ATTRIBUTE_NODISCARD bool empty() const noexcept
      {
        return table_.empty();
      }

      size_type size() const noexcept { return table_.size(); }

      size_type max_size() const noexcept { return table_.max_size(); }

      /// Modifiers
      ///

      void clear() noexcept { table_.clear(); }

      template <class Ty>
      BOOST_FORCEINLINE auto insert(Ty&& value)
        -> decltype(table_.insert(std::forward<Ty>(value)))
      {
        return table_.insert(std::forward<Ty>(value));
      }

      BOOST_FORCEINLINE std::pair<iterator, bool> insert(init_type&& value)
      {
        return table_.insert(std::move(value));
      }

      template <class Ty>
      BOOST_FORCEINLINE auto insert(const_iterator, Ty&& value)
        -> decltype(table_.insert(std::forward<Ty>(value)).first)
      {
        return table_.insert(std::forward<Ty>(value)).first;
      }

      BOOST_FORCEINLINE iterator insert(const_iterator, init_type&& value)
      {
        return table_.insert(std::move(value)).first;
      }

      template <class InputIterator>
      BOOST_FORCEINLINE void insert(InputIterator first, InputIterator last)
      {
        for (auto pos = first; pos != last; ++pos) {
          table_.emplace(*pos);
        }
      }

      void insert(std::initializer_list<value_type> ilist)
      {
        this->insert(ilist.begin(), ilist.end());
      }

      template <class M>
      std::pair<iterator, bool> insert_or_assign(key_type const& key, M&& obj)
      {
        auto ibp = table_.try_emplace(key, std::forward<M>(obj));
        if (ibp.second) {
          return ibp;
        }
        ibp.first->second = std::forward<M>(obj);
        return ibp;
      }

      template <class M>
      std::pair<iterator, bool> insert_or_assign(key_type&& key, M&& obj)
      {
        auto ibp = table_.try_emplace(std::move(key), std::forward<M>(obj));
        if (ibp.second) {
          return ibp;
        }
        ibp.first->second = std::forward<M>(obj);
        return ibp;
      }

      template <class K, class M>
      typename std::enable_if<
        utxoz::flatmap::detail::are_transparent<K, hasher, key_equal>::value,
        std::pair<iterator, bool> >::type
      insert_or_assign(K&& k, M&& obj)
      {
        auto ibp = table_.try_emplace(std::forward<K>(k), std::forward<M>(obj));
        if (ibp.second) {
          return ibp;
        }
        ibp.first->second = std::forward<M>(obj);
        return ibp;
      }

      template <class M>
      iterator insert_or_assign(const_iterator, key_type const& key, M&& obj)
      {
        return this->insert_or_assign(key, std::forward<M>(obj)).first;
      }

      template <class M>
      iterator insert_or_assign(const_iterator, key_type&& key, M&& obj)
      {
        return this->insert_or_assign(std::move(key), std::forward<M>(obj))
          .first;
      }

      template <class K, class M>
      typename std::enable_if<
        utxoz::flatmap::detail::are_transparent<K, hasher, key_equal>::value,
        iterator>::type
      insert_or_assign(const_iterator, K&& k, M&& obj)
      {
        return this->insert_or_assign(std::forward<K>(k), std::forward<M>(obj))
          .first;
      }

      template <class... Args>
      BOOST_FORCEINLINE std::pair<iterator, bool> emplace(Args&&... args)
      {
        return table_.emplace(std::forward<Args>(args)...);
      }

      template <class... Args>
      BOOST_FORCEINLINE iterator emplace_hint(const_iterator, Args&&... args)
      {
        return table_.emplace(std::forward<Args>(args)...).first;
      }

      template <class... Args>
      BOOST_FORCEINLINE std::pair<iterator, bool> try_emplace(
        key_type const& key, Args&&... args)
      {
        return table_.try_emplace(key, std::forward<Args>(args)...);
      }

      template <class... Args>
      BOOST_FORCEINLINE std::pair<iterator, bool> try_emplace(
        key_type&& key, Args&&... args)
      {
        return table_.try_emplace(std::move(key), std::forward<Args>(args)...);
      }

      template <class K, class... Args>
      BOOST_FORCEINLINE typename std::enable_if<
        utxoz::flatmap::detail::transparent_non_iterable<K,
          mmap_flat_map>::value,
        std::pair<iterator, bool> >::type
      try_emplace(K&& key, Args&&... args)
      {
        return table_.try_emplace(
          std::forward<K>(key), std::forward<Args>(args)...);
      }

      template <class... Args>
      BOOST_FORCEINLINE iterator try_emplace(
        const_iterator, key_type const& key, Args&&... args)
      {
        return table_.try_emplace(key, std::forward<Args>(args)...).first;
      }

      template <class... Args>
      BOOST_FORCEINLINE iterator try_emplace(
        const_iterator, key_type&& key, Args&&... args)
      {
        return table_.try_emplace(std::move(key), std::forward<Args>(args)...)
          .first;
      }

      template <class K, class... Args>
      BOOST_FORCEINLINE typename std::enable_if<
        utxoz::flatmap::detail::transparent_non_iterable<K,
          mmap_flat_map>::value,
        iterator>::type
      try_emplace(const_iterator, K&& key, Args&&... args)
      {
        return table_
          .try_emplace(std::forward<K>(key), std::forward<Args>(args)...)
          .first;
      }

      BOOST_FORCEINLINE typename table_type::erase_return_type erase(
        iterator pos)
      {
        return table_.erase(pos);
      }

      BOOST_FORCEINLINE typename table_type::erase_return_type erase(
        const_iterator pos)
      {
        return table_.erase(pos);
      }

      iterator erase(const_iterator first, const_iterator last)
      {
        while (first != last) {
          this->erase(first++);
        }
        return iterator{detail::foa::const_iterator_cast_tag{}, last};
      }

      BOOST_FORCEINLINE size_type erase(key_type const& key)
      {
        return table_.erase(key);
      }

      template <class K>
      BOOST_FORCEINLINE typename std::enable_if<
        detail::transparent_non_iterable<K, mmap_flat_map>::value,
        size_type>::type
      erase(K const& key)
      {
        return table_.erase(key);
      }

      BOOST_FORCEINLINE init_type pull(const_iterator pos)
      {
        return table_.pull(pos);
      }

      void swap(mmap_flat_map& rhs) noexcept(
        noexcept(std::declval<table_type&>().swap(std::declval<table_type&>())))
      {
        table_.swap(rhs.table_);
      }

      template <class H2, class P2>
      void merge(
        mmap_flat_map<key_type, mapped_type, H2, P2, allocator_type>&
          source)
      {
        table_.merge(source.table_);
      }

      template <class H2, class P2>
      void merge(
        mmap_flat_map<key_type, mapped_type, H2, P2, allocator_type>&&
          source)
      {
        table_.merge(std::move(source.table_));
      }

      /// Lookup
      ///

      mapped_type& at(key_type const& key)
      {
        auto pos = table_.find(key);
        if (pos != table_.end()) {
          return pos->second;
        }
        // TODO: someday refactor this to conditionally serialize the key and
        // include it in the error message
        //
        utxoz::flatmap::detail::throw_out_of_range(
          "key was not found in mmap_flat_map");
      }

      mapped_type const& at(key_type const& key) const
      {
        auto pos = table_.find(key);
        if (pos != table_.end()) {
          return pos->second;
        }
        utxoz::flatmap::detail::throw_out_of_range(
          "key was not found in mmap_flat_map");
      }

      template <class K>
      typename std::enable_if<
        utxoz::flatmap::detail::are_transparent<K, hasher, key_equal>::value,
        mapped_type&>::type
      at(K&& key)
      {
        auto pos = table_.find(std::forward<K>(key));
        if (pos != table_.end()) {
          return pos->second;
        }
        utxoz::flatmap::detail::throw_out_of_range(
          "key was not found in mmap_flat_map");
      }

      template <class K>
      typename std::enable_if<
        utxoz::flatmap::detail::are_transparent<K, hasher, key_equal>::value,
        mapped_type const&>::type
      at(K&& key) const
      {
        auto pos = table_.find(std::forward<K>(key));
        if (pos != table_.end()) {
          return pos->second;
        }
        utxoz::flatmap::detail::throw_out_of_range(
          "key was not found in mmap_flat_map");
      }

      BOOST_FORCEINLINE mapped_type& operator[](key_type const& key)
      {
        return table_.try_emplace(key).first->second;
      }

      BOOST_FORCEINLINE mapped_type& operator[](key_type&& key)
      {
        return table_.try_emplace(std::move(key)).first->second;
      }

      template <class K>
      typename std::enable_if<
        utxoz::flatmap::detail::are_transparent<K, hasher, key_equal>::value,
        mapped_type&>::type
      operator[](K&& key)
      {
        return table_.try_emplace(std::forward<K>(key)).first->second;
      }

      BOOST_FORCEINLINE size_type count(key_type const& key) const
      {
        auto pos = table_.find(key);
        return pos != table_.end() ? 1 : 0;
      }

      template <class K>
      BOOST_FORCEINLINE typename std::enable_if<
        detail::are_transparent<K, hasher, key_equal>::value, size_type>::type
      count(K const& key) const
      {
        auto pos = table_.find(key);
        return pos != table_.end() ? 1 : 0;
      }

      BOOST_FORCEINLINE iterator find(key_type const& key)
      {
        return table_.find(key);
      }

      BOOST_FORCEINLINE const_iterator find(key_type const& key) const
      {
        return table_.find(key);
      }

      template <class K>
      BOOST_FORCEINLINE typename std::enable_if<
        utxoz::flatmap::detail::are_transparent<K, hasher, key_equal>::value,
        iterator>::type
      find(K const& key)
      {
        return table_.find(key);
      }

      template <class K>
      BOOST_FORCEINLINE typename std::enable_if<
        utxoz::flatmap::detail::are_transparent<K, hasher, key_equal>::value,
        const_iterator>::type
      find(K const& key) const
      {
        return table_.find(key);
      }

      BOOST_FORCEINLINE bool contains(key_type const& key) const
      {
        return this->find(key) != this->end();
      }

      template <class K>
      BOOST_FORCEINLINE typename std::enable_if<
        utxoz::flatmap::detail::are_transparent<K, hasher, key_equal>::value,
        bool>::type
      contains(K const& key) const
      {
        return this->find(key) != this->end();
      }

      std::pair<iterator, iterator> equal_range(key_type const& key)
      {
        auto pos = table_.find(key);
        if (pos == table_.end()) {
          return {pos, pos};
        }

        auto next = pos;
        ++next;
        return {pos, next};
      }

      std::pair<const_iterator, const_iterator> equal_range(
        key_type const& key) const
      {
        auto pos = table_.find(key);
        if (pos == table_.end()) {
          return {pos, pos};
        }

        auto next = pos;
        ++next;
        return {pos, next};
      }

      template <class K>
      typename std::enable_if<
        detail::are_transparent<K, hasher, key_equal>::value,
        std::pair<iterator, iterator> >::type
      equal_range(K const& key)
      {
        auto pos = table_.find(key);
        if (pos == table_.end()) {
          return {pos, pos};
        }

        auto next = pos;
        ++next;
        return {pos, next};
      }

      template <class K>
      typename std::enable_if<
        detail::are_transparent<K, hasher, key_equal>::value,
        std::pair<const_iterator, const_iterator> >::type
      equal_range(K const& key) const
      {
        auto pos = table_.find(key);
        if (pos == table_.end()) {
          return {pos, pos};
        }

        auto next = pos;
        ++next;
        return {pos, next};
      }

      /// Hash Policy
      ///

      size_type bucket_count() const noexcept { return table_.capacity(); }

      float load_factor() const noexcept { return table_.load_factor(); }

      float max_load_factor() const noexcept
      {
        return table_.max_load_factor();
      }

      void max_load_factor(float) {}

      size_type max_load() const noexcept { return table_.max_load(); }

      void rehash(size_type n) { table_.rehash(n); }

      void reserve(size_type n) { table_.reserve(n); }

#if defined(UTXOZ_FLATMAP_ENABLE_STATS)
      /// Stats
      ///
      stats get_stats() const { return table_.get_stats(); }

      void reset_stats() noexcept { table_.reset_stats(); }
#endif

      /// Observers
      ///

      hasher hash_function() const { return table_.hash_function(); }

      key_equal key_eq() const { return table_.key_eq(); }
    };

    template <class Key, class T, class Hash, class KeyEqual, class Allocator>
    bool operator==(
      mmap_flat_map<Key, T, Hash, KeyEqual, Allocator> const& lhs,
      mmap_flat_map<Key, T, Hash, KeyEqual, Allocator> const& rhs)
    {
      return lhs.table_ == rhs.table_;
    }

    template <class Key, class T, class Hash, class KeyEqual, class Allocator>
    bool operator!=(
      mmap_flat_map<Key, T, Hash, KeyEqual, Allocator> const& lhs,
      mmap_flat_map<Key, T, Hash, KeyEqual, Allocator> const& rhs)
    {
      return !(lhs == rhs);
    }

    template <class Key, class T, class Hash, class KeyEqual, class Allocator>
    void swap(mmap_flat_map<Key, T, Hash, KeyEqual, Allocator>& lhs,
      mmap_flat_map<Key, T, Hash, KeyEqual, Allocator>& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    template <class Key, class T, class Hash, class KeyEqual, class Allocator,
      class Pred>
    typename mmap_flat_map<Key, T, Hash, KeyEqual, Allocator>::size_type
    erase_if(
      mmap_flat_map<Key, T, Hash, KeyEqual, Allocator>& map, Pred pred)
    {
      return erase_if(map.table_, pred);
    }
#if defined(BOOST_MSVC)
#pragma warning(pop) /* C4714 */
#endif

#if UTXOZ_FLATMAP_TEMPLATE_DEDUCTION_GUIDES

    template <class InputIterator,
      class Hash =
        boost::hash<utxoz::flatmap::detail::iter_key_t<InputIterator> >,
      class Pred =
        std::equal_to<utxoz::flatmap::detail::iter_key_t<InputIterator> >,
      class Allocator = std::allocator<
        utxoz::flatmap::detail::iter_to_alloc_t<InputIterator> >,
      class = std::enable_if_t<detail::is_input_iterator_v<InputIterator> >,
      class = std::enable_if_t<detail::is_hash_v<Hash> >,
      class = std::enable_if_t<detail::is_pred_v<Pred> >,
      class = std::enable_if_t<detail::is_allocator_v<Allocator> > >
    mmap_flat_map(InputIterator, InputIterator,
      std::size_t = utxoz::flatmap::detail::foa::default_bucket_count,
      Hash = Hash(), Pred = Pred(), Allocator = Allocator())
      -> mmap_flat_map<utxoz::flatmap::detail::iter_key_t<InputIterator>,
        utxoz::flatmap::detail::iter_val_t<InputIterator>, Hash, Pred,
        Allocator>;

    template <class Key, class T,
      class Hash = boost::hash<std::remove_const_t<Key> >,
      class Pred = std::equal_to<std::remove_const_t<Key> >,
      class Allocator = std::allocator<std::pair<const Key, T> >,
      class = std::enable_if_t<detail::is_hash_v<Hash> >,
      class = std::enable_if_t<detail::is_pred_v<Pred> >,
      class = std::enable_if_t<detail::is_allocator_v<Allocator> > >
    mmap_flat_map(std::initializer_list<std::pair<Key, T> >,
      std::size_t = utxoz::flatmap::detail::foa::default_bucket_count,
      Hash = Hash(), Pred = Pred(), Allocator = Allocator())
      -> mmap_flat_map<std::remove_const_t<Key>, T, Hash, Pred,
        Allocator>;

    template <class InputIterator, class Allocator,
      class = std::enable_if_t<detail::is_input_iterator_v<InputIterator> >,
      class = std::enable_if_t<detail::is_allocator_v<Allocator> > >
    mmap_flat_map(InputIterator, InputIterator, std::size_t, Allocator)
      -> mmap_flat_map<utxoz::flatmap::detail::iter_key_t<InputIterator>,
        utxoz::flatmap::detail::iter_val_t<InputIterator>,
        boost::hash<utxoz::flatmap::detail::iter_key_t<InputIterator> >,
        std::equal_to<utxoz::flatmap::detail::iter_key_t<InputIterator> >,
        Allocator>;

    template <class InputIterator, class Allocator,
      class = std::enable_if_t<detail::is_input_iterator_v<InputIterator> >,
      class = std::enable_if_t<detail::is_allocator_v<Allocator> > >
    mmap_flat_map(InputIterator, InputIterator, Allocator)
      -> mmap_flat_map<utxoz::flatmap::detail::iter_key_t<InputIterator>,
        utxoz::flatmap::detail::iter_val_t<InputIterator>,
        boost::hash<utxoz::flatmap::detail::iter_key_t<InputIterator> >,
        std::equal_to<utxoz::flatmap::detail::iter_key_t<InputIterator> >,
        Allocator>;

    template <class InputIterator, class Hash, class Allocator,
      class = std::enable_if_t<detail::is_hash_v<Hash> >,
      class = std::enable_if_t<detail::is_input_iterator_v<InputIterator> >,
      class = std::enable_if_t<detail::is_allocator_v<Allocator> > >
    mmap_flat_map(
      InputIterator, InputIterator, std::size_t, Hash, Allocator)
      -> mmap_flat_map<utxoz::flatmap::detail::iter_key_t<InputIterator>,
        utxoz::flatmap::detail::iter_val_t<InputIterator>, Hash,
        std::equal_to<utxoz::flatmap::detail::iter_key_t<InputIterator> >,
        Allocator>;

    template <class Key, class T, class Allocator,
      class = std::enable_if_t<detail::is_allocator_v<Allocator> > >
    mmap_flat_map(std::initializer_list<std::pair<Key, T> >, std::size_t,
      Allocator) -> mmap_flat_map<std::remove_const_t<Key>, T,
      boost::hash<std::remove_const_t<Key> >,
      std::equal_to<std::remove_const_t<Key> >, Allocator>;

    template <class Key, class T, class Allocator,
      class = std::enable_if_t<detail::is_allocator_v<Allocator> > >
    mmap_flat_map(std::initializer_list<std::pair<Key, T> >, Allocator)
      -> mmap_flat_map<std::remove_const_t<Key>, T,
        boost::hash<std::remove_const_t<Key> >,
        std::equal_to<std::remove_const_t<Key> >, Allocator>;

    template <class Key, class T, class Hash, class Allocator,
      class = std::enable_if_t<detail::is_hash_v<Hash> >,
      class = std::enable_if_t<detail::is_allocator_v<Allocator> > >
    mmap_flat_map(std::initializer_list<std::pair<Key, T> >, std::size_t,
      Hash, Allocator) -> mmap_flat_map<std::remove_const_t<Key>, T,
      Hash, std::equal_to<std::remove_const_t<Key> >, Allocator>;
#endif

  } // namespace flatmap
} // namespace utxoz

#endif
