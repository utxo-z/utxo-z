// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file system_entropy.hpp
 * @brief Bytes from the operating system's cryptographic generator.
 * @internal
 *
 * `std::random_device` is not this. The standard permits it to be a
 * deterministic engine, and implementations have shipped exactly that — so a
 * merge identifier drawn from it could repeat, and repeating is the one thing
 * it must not do. What is needed is the platform's own CSPRNG, named directly.
 *
 * The clock, the process id, and counters over them are not alternatives. Two
 * containers started together share a second and, under a container runtime,
 * can share a pid — which is precisely the situation the identifier exists to
 * make safe. There is no fallback here: if the system will not produce
 * entropy, that is an error and it is returned as one, before anything has been
 * written.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include <utxoz/types.hpp>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
// arc4random_buf is the native interface here, is backed by the kernel
// generator, and is documented as unable to fail. getentropy exists on recent
// macOS but is not declared by the headers this includes.
#include <cstdlib>
#elif defined(__linux__) || defined(__EMSCRIPTEN__)
#include <cerrno>
#include <cstdio>
#include <sys/random.h>
#include <unistd.h>
#else
#include <cstdio>
#endif

namespace utxoz::detail {

/**
 * @brief Fills `out` with `length` bytes from the system's cryptographic RNG.
 *
 * Each platform uses the interface that is native to it and verifiable there,
 * rather than one interface guessed at across all of them:
 *
 * - Windows: `BCryptGenRandom` with the system-preferred provider.
 * - macOS and the BSDs: `arc4random_buf`, which cannot fail.
 * - Linux and Emscripten: `getentropy` — Emscripten backs it with the
 *   platform's crypto API — falling back to `/dev/urandom`, read to
 *   completion, if the call is unavailable at runtime.
 * - Any other POSIX: `/dev/urandom`, read to completion.
 *
 * @return empty on success, `error_code::entropy_unavailable` otherwise. There
 *         is deliberately no fallback to anything derived from time or identity.
 */
/**
 * @brief How much of a request to issue next.
 *
 * The platform calls take a bounded count — `BCryptGenRandom` takes a `ULONG`
 * and `getentropy` refuses more than 256 bytes — while this takes a `size_t`.
 * Casting a larger request down would ask for the wrong number of bytes and
 * report success, so requests are split instead. Factored out because the only
 * way to test the splitting otherwise is to ask for four gigabytes of entropy.
 */
[[nodiscard]]
inline constexpr size_t entropy_chunk(size_t remaining, size_t limit) noexcept {
    return remaining < limit ? remaining : limit;
}

[[nodiscard]]
inline result<> system_entropy(void* out, size_t length) {
    if (length == 0) return {};

#if defined(_WIN32)
    auto* bytes = static_cast<unsigned char*>(out);
    size_t offset = 0;
    while (offset < length) {
        size_t const chunk = entropy_chunk(length - offset,
                                           size_t((std::numeric_limits<ULONG>::max)()));
        auto const status = ::BCryptGenRandom(nullptr, static_cast<PUCHAR>(bytes + offset),
                                              ULONG(chunk), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        if (status != 0) return std::unexpected(error_code::entropy_unavailable);
        offset += chunk;
    }
    return {};

#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    ::arc4random_buf(out, length);
    return {};

#else
    auto* bytes = static_cast<unsigned char*>(out);
    size_t offset = 0;

#if defined(__linux__) || defined(__EMSCRIPTEN__)
    // getentropy caps at 256 bytes per call and either fills the buffer or
    // fails; there is no partial success to handle.
    while (offset < length) {
        size_t const chunk = entropy_chunk(length - offset, 256);
        if (::getentropy(bytes + offset, chunk) != 0) {
            if (errno == ENOSYS) break;   // absent at runtime; fall through
            return std::unexpected(error_code::entropy_unavailable);
        }
        offset += chunk;
    }
    if (offset == length) return {};
#endif

    // Read to completion: a short read is not entropy, it is part of it.
    std::FILE* source = std::fopen("/dev/urandom", "rb");
    if (source == nullptr) return std::unexpected(error_code::entropy_unavailable);

    size_t const read = std::fread(bytes + offset, 1, length - offset, source);
    std::fclose(source);

    return read == length - offset ? result<>{}
                                   : std::unexpected(error_code::entropy_unavailable);
#endif
}

} // namespace utxoz::detail
