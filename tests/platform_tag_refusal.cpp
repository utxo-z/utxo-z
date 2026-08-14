// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file platform_tag_refusal.cpp
 * @brief A translation unit that must not compile.
 *
 * Built by ctest with a deliberately bad `UTXOZ_PLATFORM_TAG` and required to
 * fail. The contract it pins cannot be checked from inside a running test: a
 * refusal that happens at compile time is invisible to a suite that only runs
 * what compiled.
 *
 * What it stands for: an unrecognised platform must not be able to reach an
 * accepted identity. There is no shared fallback, so two unknown targets cannot
 * agree on a tag neither earned — and the escape hatch for a build that does know
 * what it is has a floor beneath it, so an explicit tag cannot collide with a
 * platform this header assigns itself.
 */

#include "detail/format_identity.hpp"

// Reached only if the assertions above did not fire, which is the failure.
static_assert(utxoz::detail::platform_tag != 0);
