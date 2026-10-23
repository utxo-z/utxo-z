// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstddef>

namespace utxoz::detail {

/// Makes sure the next `extra` insertions of new keys into a Boost flat map
/// allocate nothing. May allocate - and throw - itself; changes no element.
///
/// A flat map stores its elements in its bucket array and only allocates to
/// grow it, which it does when an insertion would take the size past
/// `max_load()`. So room for `extra` new keys is `max_load() - size() >= extra`.
///
/// Not `reserve(size() + extra)` on its own: Boost's reserve sets the capacity
/// to what the figure asks for, so a figure below the current capacity shrinks
/// the map - an allocation of its own, on every call. This only ever grows it,
/// and only when the room is not already there. It loops because max_load is
/// derived through a float, and the room is checked rather than assumed.
template <typename Map>
void make_room(Map& map, std::size_t extra) {
    std::size_t want = map.size() + extra;
    while (map.max_load() - map.size() < extra) {
        map.reserve(want);
        want += want / 2 + 1;
    }
}

} // namespace utxoz::detail
