#!/usr/bin/env python3
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Retry the no-build install while the uploaded binary becomes visible.

The recipe can arrive from another platform before this platform's binary.
Keep the exact reference, profile, lock and --build=never on every attempt.
A monotonic deadline bounds both the commands and the gaps between them.
"""

import math
import os
import subprocess
import sys
import time


def positive_setting(name, default):
    value = float(os.environ.get(name, default))
    if not math.isfinite(value) or value <= 0:
        raise ValueError(f"{name} must be finite and greater than zero")
    return value


def main():
    try:
        timeout = positive_setting("UTXOZ_BINARY_TIMEOUT", "300")
        gap = positive_setting("UTXOZ_BINARY_POLL_INITIAL", "2")
        maximum = positive_setting("UTXOZ_BINARY_POLL_MAX", "30")
        if maximum < gap:
            raise ValueError("UTXOZ_BINARY_POLL_MAX is below UTXOZ_BINARY_POLL_INITIAL")
    except ValueError as error:
        print(error, file=sys.stderr)
        return 1

    if not sys.argv[1:]:
        print("usage: await_binary.py <conan install command...>", file=sys.stderr)
        return 1

    deadline = time.monotonic() + timeout
    attempt = 0
    print(f"Waiting up to {timeout:g}s for the uploaded binary", flush=True)
    while (remaining := deadline - time.monotonic()) > 0:
        attempt += 1
        try:
            result = subprocess.run(sys.argv[1:], timeout=remaining, check=False)
        except subprocess.TimeoutExpired:
            print("Binary retrieval exceeded its deadline", file=sys.stderr)
            return 1
        if result.returncode == 0:
            return 0
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        print(f"Binary install attempt {attempt} failed; retrying within the publication window",
              flush=True)
        time.sleep(min(gap, remaining))
        gap = min(gap * 2, maximum)
    print("Uploaded binary was not retrieved within the publication window", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
