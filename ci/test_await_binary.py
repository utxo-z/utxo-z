#!/usr/bin/env python3
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Controls for the publication window, without waiting for a real deadline."""

import contextlib
import io
import os
import subprocess
import unittest
from unittest.mock import patch

import await_binary


class PublicationWindowTests(unittest.TestCase):
    def run_window(self, outcomes):
        clock = [0.0]
        budgets = []
        outcomes = iter(outcomes)

        def run(command, *, timeout, check):
            self.assertEqual(command, ["conan", "install", "--build=never"])
            budgets.append(timeout)
            outcome = next(outcomes)
            if outcome == "timeout":
                clock[0] += timeout
                raise subprocess.TimeoutExpired(command, timeout)
            clock[0] += 1
            return subprocess.CompletedProcess(command, outcome)

        def sleep(seconds):
            clock[0] += seconds

        with patch.dict(os.environ, {"UTXOZ_BINARY_TIMEOUT": "10",
                                     "UTXOZ_BINARY_POLL_INITIAL": "2",
                                     "UTXOZ_BINARY_POLL_MAX": "4"}), \
             patch("sys.argv", ["await_binary.py", "conan", "install", "--build=never"]), \
             patch.object(await_binary.time, "monotonic", side_effect=lambda: clock[0]), \
             patch.object(await_binary.time, "sleep", side_effect=sleep), \
             patch.object(await_binary.subprocess, "run", side_effect=run), \
             contextlib.redirect_stdout(io.StringIO()), \
             contextlib.redirect_stderr(io.StringIO()):
            result = await_binary.main()
        return result, clock[0], budgets

    def test_binary_arrives_after_recipe(self):
        result, elapsed, budgets = self.run_window([1, 1, 0])
        self.assertEqual(result, 0)
        self.assertEqual(elapsed, 9)
        self.assertEqual(budgets, [10, 7, 2])

    def test_missing_binary_stops_at_deadline(self):
        result, elapsed, budgets = self.run_window([1, 1, 1])
        self.assertEqual(result, 1)
        self.assertEqual(elapsed, 10)
        self.assertEqual(budgets, [10, 7, 2])

    def test_stalled_install_is_bounded_by_remaining_window(self):
        result, elapsed, budgets = self.run_window([1, "timeout"])
        self.assertEqual(result, 1)
        self.assertEqual(elapsed, 10)
        self.assertEqual(budgets, [10, 7])

    def test_invalid_deadline_is_rejected_without_running_conan(self):
        for value in ["0", "-1", "nan", "inf", "invalid"]:
            with self.subTest(value=value), \
                 patch.dict(os.environ, {"UTXOZ_BINARY_TIMEOUT": value}), \
                 patch.object(await_binary.subprocess, "run") as run, \
                 contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(await_binary.main(), 1)
                run.assert_not_called()


if __name__ == "__main__":
    unittest.main()
