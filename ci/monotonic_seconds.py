#!/usr/bin/env python3

"""Whole seconds from a clock that cannot go backwards.

`date +%s` is the civil clock, and the civil clock steps. NTP correcting a
runner that has drifted moves it by seconds or minutes, in either direction, and
a deadline computed by subtracting two of its readings is not a deadline: a
backwards step lengthens the wait by however far it stepped, and a forwards step
ends it early. Neither failure announces itself — the wait simply is not the
length it printed when it started.

`time.monotonic()` is CLOCK_MONOTONIC on Linux and macOS and GetTickCount64 on
Windows. All three count from boot rather than from an epoch, so readings taken
by separate invocations of this program are still comparable to each other,
which is what makes it usable as a clock for a shell script. Only differences
between readings mean anything; the absolute value does not.

Usage:  monotonic_seconds.py
"""

from __future__ import annotations

import time

if __name__ == "__main__":
    # Truncated, not rounded: rounding up can make two readings differ by a
    # second that did not pass, and every consumer here compares whole seconds.
    print(int(time.monotonic()))
