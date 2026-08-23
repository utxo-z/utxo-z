#!/usr/bin/env python3
"""What the A/B harness has to get right, asserted rather than assumed.

The scenarios here are the ones a benchmark lane is judged on: it must not call
noise a regression, it must catch a real one in either direction, it must say
"I don't know" when it doesn't, and it must refuse to interpret a run whose
provenance is broken. They are driven from synthetic samples so each can be
stated exactly — filling a segment until a runner happens to be slow is not a
test of anything.
"""

from __future__ import annotations

import json
import random
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import statistics as st

import bench_ab
import bench_analyze as ba
import bench_quiesce as bq

BENCH = "simulated IBD (100 blocks)"
ROUNDS = 15


def samples(a_values, b_values, *, mode="ab", name=BENCH, metric="elapsed",
            identical=False, order=None, extra=None):
    """One artifact, shaped exactly as `bench_ab.py` writes it."""
    rounds = len(a_values)
    order = order or ["AB" if i % 2 == 0 else "BA" for i in range(rounds)]

    def side(values):
        out = []
        for v in values:
            entry = {m: None for m in bench_ab.METRICS}
            entry[metric] = v
            if extra:
                entry.update(extra)
            out.append(entry)
        return out

    return {
        "schema": bench_ab.SCHEMA,
        "mode": mode,
        "harness": {"seed": 7, "rounds": rounds, "order": order,
                    "executions": [], "interleaved": True},
        "sides": {
            "A": {"label": "A", "binary_sha256": "a" * 64, "commit": "aaaaaaa"},
            "B": {"label": "B", "binary_sha256": ("a" if identical else "b") * 64,
                  "commit": "bbbbbbb"},
        },
        "identical_binaries": identical,
        "environment": {},
        "benchmarks": {name: {"A": side(a_values), "B": side(b_values)}},
    }


def noisy(rng, base, spread, n=ROUNDS):
    return [base * (1.0 + rng.uniform(-spread, spread)) for _ in range(n)]


def verdict_of(report, metric="elapsed", name=BENCH):
    for f in report["findings"]:
        if f["benchmark"] == name and f["metric"] == metric:
            return f
    raise AssertionError(f"no finding for {name}/{metric}")


def analyse(sample, control=None):
    return ba.analyse(sample, control, resamples=2000, confidence=0.95, seed=1)


class NoiseFloor(unittest.TestCase):
    def test_aa_produces_no_regression(self):
        """The control negative: one build measured as both sides.

        This is PR #141 reduced to its essentials, and the previous lane's answer
        to it was "-53%" and "+102%" at the same time.
        """
        rng = random.Random(11)
        control = samples(noisy(rng, 1000, 0.10), noisy(rng, 1000, 0.10),
                          mode="aa", identical=True)
        report = analyse(control, control)
        for f in report["findings"]:
            self.assertNotEqual(f["verdict"], ba.REGRESSION,
                                f"{f['benchmark']}/{f['metric']} called noise a regression")

    def test_difference_inside_the_floor_is_indistinguishable(self):
        """A real difference smaller than the control's own resolution.

        Both sides are quiet here, so the interval is narrow and fits inside the
        floor. That is the case the word "indistinguishable" is for: the
        measurement is good enough to have found a 1% difference and did not.
        """
        rng = random.Random(12)
        control = samples(noisy(rng, 1000, 0.03), noisy(rng, 1000, 0.03),
                          mode="aa", identical=True)
        ab_run = samples(noisy(rng, 1000, 0.03), noisy(rng, 1005, 0.03))
        self.assertEqual(verdict_of(analyse(ab_run, control))["verdict"],
                         ba.INDISTINGUISHABLE)

    def test_a_run_noisier_than_its_control_is_no_evidence(self):
        """And the case it is not for.

        When the A/B interval is wider than the floor it has to clear, the honest
        answer is that this run could not tell — not that nothing changed. The
        difference between the two matters: one invites a merge, the other asks
        for another measurement.
        """
        rng = random.Random(12)
        control = samples(noisy(rng, 1000, 0.03), noisy(rng, 1000, 0.03),
                          mode="aa", identical=True)
        ab_run = samples(noisy(rng, 1000, 0.25), noisy(rng, 1010, 0.25))
        self.assertEqual(verdict_of(analyse(ab_run, control))["verdict"],
                         ba.NO_EVIDENCE)

    def test_floor_comes_from_the_control_not_from_the_run(self):
        """A change that adds variance must not widen its own tolerance.

        The A/B side here is far noisier than the control. If the floor were
        derived from the run being judged, that noise would raise the bar it has
        to clear; taken from the control, the extra spread widens the interval
        instead and the answer becomes "not enough evidence".
        """
        rng = random.Random(13)
        control = samples([1000.0] * ROUNDS, [1000.0] * ROUNDS, mode="aa", identical=True)
        ab_run = samples(noisy(rng, 1000, 0.40), noisy(rng, 1000, 0.40))
        self.assertEqual(verdict_of(analyse(ab_run, control))["verdict"], ba.NO_EVIDENCE)


class ClassificationRules(unittest.TestCase):
    """The three rules, stated as arithmetic rather than as prose.

    They are policy, so they are asserted here and written down in doc/. The
    factor of two is the initial equivalence policy and nothing more universal
    than that; it is a parameter, and this is where a change to it announces
    itself.
    """

    FLOOR = 0.05

    def test_detection_needs_the_interval_to_clear_the_floor(self):
        # lo > 1 + floor  ->  regression
        self.assertEqual(ba.classify(1.0501, 1.20, self.FLOOR)[0], ba.REGRESSION)
        self.assertNotEqual(ba.classify(1.0499, 1.20, self.FLOOR)[0], ba.REGRESSION)
        # hi < 1 - floor  ->  improvement
        self.assertEqual(ba.classify(0.80, 0.9499, self.FLOOR)[0], ba.IMPROVEMENT)
        self.assertNotEqual(ba.classify(0.80, 0.9501, self.FLOOR)[0], ba.IMPROVEMENT)

    def test_equivalence_needs_the_interval_inside_twice_the_floor(self):
        # the whole interval within 1 +- 2*floor
        self.assertEqual(ba.classify(0.901, 1.099, self.FLOOR)[0], ba.INDISTINGUISHABLE)
        self.assertEqual(ba.classify(0.899, 1.099, self.FLOOR)[0], ba.NO_EVIDENCE)

    def test_the_band_between_the_two_is_no_evidence(self):
        """Neither clears the floor nor fits the margin: the run did not answer."""
        verdict, reason = ba.classify(1.02, 1.15, self.FLOOR)
        self.assertEqual(verdict, ba.NO_EVIDENCE)
        self.assertIn("did not resolve", reason)

    def test_the_equivalence_multiple_is_a_parameter_not_a_constant(self):
        wide = ba.classify(0.90, 1.10, self.FLOOR, equivalence_multiple=2.0)[0]
        narrow = ba.classify(0.90, 1.10, self.FLOOR, equivalence_multiple=1.0)[0]
        self.assertEqual(wide, ba.INDISTINGUISHABLE)
        self.assertEqual(narrow, ba.NO_EVIDENCE)

    def test_the_multiple_reaches_the_artifact(self):
        rng = random.Random(61)
        control = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02),
                          mode="aa", identical=True)
        run = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02))
        report = ba.analyse(run, control, resamples=500, confidence=0.95, seed=1,
                            equivalence_multiple=3.0)
        self.assertIn("3x", report["method"]["equivalence_margin"])


class FloorFormula(unittest.TestCase):
    """`floor = max(|aa_ci_low - 1|, |aa_ci_high - 1|)`, and where it comes from."""

    def _control(self, a, b):
        return samples(a, b, mode="aa", identical=True)

    def test_the_floor_is_the_wider_arm_not_the_average(self):
        """The A/A interval is not symmetric around 1, and the bound is the far side.

        Skewed ratios put more of the bootstrap interval on one side. Averaging
        the two arms, or halving the width, would understate the floor exactly
        when the distribution is worst behaved.
        """
        # A control with a heavy right tail: half the pairs land on 1.0 and the
        # rest run long, which is the shape a real machine produces when
        # something occasionally interrupts it. The bootstrap median can then
        # move up but has nowhere to go down, so the interval is one-sided.
        values_a = [1000.0] * ROUNDS
        values_b = [1000.0] * 8 + [1100.0, 1150.0, 1200.0, 1400.0, 1800.0, 2400.0, 3000.0]
        control = self._control(values_a, values_b)
        detail = ba.floor_from_control(control, BENCH, "elapsed",
                                       resamples=2000, confidence=0.95, seed=1)
        lo, hi, floor = detail["aa_ci_low"], detail["aa_ci_high"], detail["floor"]
        self.assertAlmostEqual(floor, max(abs(lo - 1.0), abs(hi - 1.0)), places=12)
        # asymmetric, and the floor followed the wider arm
        self.assertNotAlmostEqual(abs(lo - 1.0), abs(hi - 1.0), places=6)
        self.assertGreaterEqual(floor, (abs(lo - 1.0) + abs(hi - 1.0)) / 2.0)

    def test_the_floor_and_its_provenance_reach_the_artifact(self):
        rng = random.Random(72)
        control = self._control(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02))
        run = samples(noisy(rng, 1000, 0.02), noisy(rng, 1300, 0.02))
        found = verdict_of(ba.analyse(run, control, resamples=500,
                                      confidence=0.95, seed=1))
        detail = found["floor_detail"]
        for key in ("aa_ci_low", "aa_ci_high", "floor", "formula",
                    "policy_version", "control_pairs"):
            self.assertIn(key, detail)
        self.assertEqual(detail["formula"],
                         "max(abs(aa_ci_low - 1), abs(aa_ci_high - 1))")
        self.assertEqual(found["noise_floor"], detail["floor"])
        self.assertEqual(found["equivalence_multiple"], 2.0)
        self.assertAlmostEqual(found["equivalence_margin"], detail["floor"] * 2.0)
        self.assertEqual(found["samples"], ROUNDS)

    def test_the_floor_uses_only_the_control_pairs(self):
        """One A/B run, two controls, two verdicts.

        The earlier version of this case built the A/B run and then never used
        it: it compared two floors and asserted they differed, which says nothing
        about where a *verdict* gets its threshold from. Here the same run is
        judged twice, and only the control changes.
        """
        rng = random.Random(73)
        run = samples(noisy(rng, 1000, 0.02), noisy(rng, 1080, 0.02))

        tight = self._control([1000.0] * ROUNDS, [1000.0] * ROUNDS)
        loose = self._control(noisy(rng, 1000, 0.30), noisy(rng, 1000, 0.30))

        tight_floor = ba.floor_from_control(tight, BENCH, "elapsed", 500, 0.95, 1)
        loose_floor = ba.floor_from_control(loose, BENCH, "elapsed", 500, 0.95, 1)
        self.assertLess(tight_floor["floor"], loose_floor["floor"])
        self.assertEqual(tight_floor["control_pairs"], ROUNDS)

        # An 8% difference clears a floor measured on a quiet control and does
        # not clear one measured on a noisy control. Same numbers on the A/B
        # side, both times.
        self.assertEqual(verdict_of(analyse(run, tight))["verdict"], ba.REGRESSION)
        self.assertNotEqual(verdict_of(analyse(run, loose))["verdict"], ba.REGRESSION)


class MetricDirection(unittest.TestCase):
    """Each metric declares which way is better; none of them infers it."""

    def test_every_reported_metric_declares_a_direction(self):
        for metric in ba.METRIC_ROLE:
            self.assertIn(metric, ba.METRIC_DIRECTION, metric)
            self.assertIn(ba.METRIC_DIRECTION[metric],
                          (ba.LOWER_IS_BETTER, ba.HIGHER_IS_BETTER, ba.NO_DIRECTION))

    def test_the_time_metrics_are_costs(self):
        for metric in ("elapsed", "cycles", "instructions", "branches"):
            self.assertEqual(ba.METRIC_DIRECTION[metric], ba.LOWER_IS_BETTER)

    def test_a_dispersion_has_no_direction(self):
        self.assertEqual(ba.METRIC_DIRECTION["mdape"], ba.NO_DIRECTION)

    def test_an_undeclared_metric_is_no_evidence_not_a_guess(self):
        rng = random.Random(74)
        run = samples(noisy(rng, 1000, 0.02), noisy(rng, 1300, 0.02))
        saved = dict(ba.METRIC_ROLE)
        try:
            ba.METRIC_ROLE["throughput"] = "informative"
            found = verdict_of(ba.analyse(run, run, 500, 0.95, 1), metric="throughput")
            self.assertEqual(found["verdict"], ba.NO_EVIDENCE)
            self.assertIn("no declared direction", found["reason"])
        finally:
            ba.METRIC_ROLE.clear()
            ba.METRIC_ROLE.update(saved)

    def test_the_direction_travels_with_each_finding(self):
        rng = random.Random(75)
        control = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02),
                          mode="aa", identical=True)
        run = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02))
        for f in ba.analyse(run, control, 500, 0.95, 1)["findings"]:
            self.assertIn("direction", f)


class FamilyWiseBand(unittest.TestCase):
    """One session, twenty-one benchmarks, one statement about the family.

    Each of these is a way the correction can quietly fail: a band that grows
    when one benchmark moves would hide the others; a band that ignores the
    expected one-in-twenty would keep declaring it; and a band built from
    independent draws would be too narrow for correlated data.
    """

    FAMILY = 21
    ROUNDS = 60

    # Independent per-benchmark noise, chosen so these fixtures reproduce the
    # family floor the real sessions produce: over sixty rounds it yields 3.56%,
    # against 3.49% measured on mac and 3.52% on fedora. The tests below are
    # about behaviour at that floor, so the fixture has to sit at it.
    JITTER = 0.07

    def _family(self, rng, moved=None, spread=0.02, move=0.0, jitter=None):
        """Twenty-one benchmarks over shared rounds, with a common per-round mood.

        The per-round factor is the point: it is what makes the benchmarks
        correlated, exactly as a real process's state does. The independent term
        on top is each benchmark's own noise.
        """
        jitter = self.JITTER if jitter is None else jitter
        rounds = [1.0 + rng.uniform(-spread, spread) for _ in range(self.ROUNDS)]
        out = {}
        for i in range(self.FAMILY):
            name = f"bench{i:02d}"
            effect = (1.0 + move) if name == moved else 1.0
            out[name] = [r * effect * (1.0 + rng.uniform(-jitter, jitter)) for r in rounds]
        return out

    def test_a_large_effect_in_one_does_not_widen_the_band_for_the_others(self):
        """The reason the A/B band is centred on each observed median.

        Centred on 1, a benchmark that really moved 50% would push the family
        maximum to 50% and swamp every other benchmark's interval. Centred on
        its own median, its deviation is as small as anyone else's.
        """
        rng = random.Random(101)
        quiet = self._family(rng)
        loud = self._family(random.Random(101), moved="bench07", move=0.50)
        band_quiet = ba.family_deviation_quantile(quiet, self.ROUNDS, 800, 0.95, 1, "observed")
        band_loud = ba.family_deviation_quantile(loud, self.ROUNDS, 800, 0.95, 1, "observed")
        self.assertLess(abs(band_loud - band_quiet), 0.01,
                        "a 50% effect in one benchmark widened the band for the family")

    def test_centring_on_one_would_have_widened_it(self):
        """The control for the test above: the wrong centre really does inflate."""
        loud = self._family(random.Random(101), moved="bench07", move=0.50)
        centred = ba.family_deviation_quantile(loud, self.ROUNDS, 800, 0.95, 1, "observed")
        at_one = ba.family_deviation_quantile(loud, self.ROUNDS, 800, 0.95, 1, "one")
        self.assertGreater(at_one, centred * 5)

    def test_one_small_move_in_twenty_one_is_not_declared(self):
        """mac's `find miss`: +1.85% against a family floor near 3.5%."""
        rng = random.Random(102)
        control = self._family(rng)
        floor = ba.family_deviation_quantile(control, self.ROUNDS, 800, 0.95, 1, "one")
        run = self._family(random.Random(103), moved="bench07", move=0.018)
        half = ba.family_deviation_quantile(run, self.ROUNDS, 800, 0.95, 1, "observed")
        observed = st.median(run["bench07"])
        verdict, _ = ba.classify_family(observed, half, floor)
        self.assertEqual(verdict, ba.INDISTINGUISHABLE)

    def test_two_correlated_aa_controls_produce_nothing(self):
        """Correlation is not an effect. Whole-round resampling must keep it so."""
        rng = random.Random(104)
        control = self._family(rng, spread=0.12)
        run = self._family(random.Random(105), spread=0.12)
        floor = ba.family_deviation_quantile(control, self.ROUNDS, 800, 0.95, 1, "one")
        half = ba.family_deviation_quantile(run, self.ROUNDS, 800, 0.95, 1, "observed")
        for name, ratios in run.items():
            verdict, _ = ba.classify_family(st.median(ratios), half, floor)
            self.assertEqual(verdict, ba.INDISTINGUISHABLE, f"{name} was declared")

    def test_a_real_effect_still_clears_the_family_floor(self):
        """The correction must not make the instrument blind."""
        rng = random.Random(106)
        control = self._family(rng)
        floor = ba.family_deviation_quantile(control, self.ROUNDS, 800, 0.95, 1, "one")
        run = self._family(random.Random(107), moved="bench07", move=0.27)
        half = ba.family_deviation_quantile(run, self.ROUNDS, 800, 0.95, 1, "observed")
        verdict, reason = ba.classify_family(st.median(run["bench07"]), half, floor)
        self.assertEqual(verdict, ba.REGRESSION)
        self.assertIn("simultaneous lower bound", reason)
        for name in run:
            if name == "bench07":
                continue
            v, _ = ba.classify_family(st.median(run[name]), half, floor)
            self.assertEqual(v, ba.INDISTINGUISHABLE, f"{name} declared alongside")

    def test_the_lower_bound_and_not_the_median_decides(self):
        """A median above the floor with an interval that straddles it is not a finding."""
        floor = 0.05
        self.assertEqual(ba.classify_family(1.06, 0.001, floor)[0], ba.REGRESSION)
        self.assertEqual(ba.classify_family(1.06, 0.02, floor)[0], ba.INDISTINGUISHABLE)

    def test_an_improvement_is_symmetric(self):
        floor = 0.05
        self.assertEqual(ba.classify_family(0.94, 0.001, floor)[0], ba.IMPROVEMENT)


class ComparisonIdentity(unittest.TestCase):
    """Two builds that differ in how they were built are not a comparison."""

    def _sides(self, **b_overrides):
        base = {"toolchain": "gcc 16.1.0", "dependencies": "boost/1.91.0",
                "build_type": "Release", "options": "statistics_level=basic"}
        return {"A": dict(base), "B": {**base, **b_overrides}}

    def test_matching_builds_are_accepted(self):
        shared = bench_ab.comparison_identity(self._sides())
        self.assertEqual(shared["toolchain"], "gcc 16.1.0")

    def test_a_different_toolchain_fails_closed(self):
        with self.assertRaisesRegex(bench_ab.HarnessError, "toolchain"):
            bench_ab.comparison_identity(self._sides(toolchain="clang 21"))

    def test_different_dependencies_fail_closed(self):
        with self.assertRaisesRegex(bench_ab.HarnessError, "dependencies"):
            bench_ab.comparison_identity(self._sides(dependencies="boost/1.88.0"))

    def test_a_different_build_type_fails_closed(self):
        with self.assertRaisesRegex(bench_ab.HarnessError, "build_type"):
            bench_ab.comparison_identity(self._sides(build_type="Debug"))

    def test_different_options_fail_closed(self):
        with self.assertRaisesRegex(bench_ab.HarnessError, "options"):
            bench_ab.comparison_identity(self._sides(options="statistics_level=off"))


class FamilyDefinition(unittest.TestCase):
    """Who is in the family, and who is not."""

    def test_only_elapsed_is_primary(self):
        self.assertEqual(ba.PRIMARY_METRIC, "elapsed")
        self.assertEqual(ba.METRIC_ROLE[ba.PRIMARY_METRIC], "gate")
        for metric, role in ba.METRIC_ROLE.items():
            if metric != ba.PRIMARY_METRIC:
                self.assertIn(role, ("structural", "informative"),
                              f"{metric} would issue verdicts alongside elapsed")

    def test_the_family_is_exactly_the_declared_names(self):
        """Names and cardinality, pinned.

        A benchmark added to the suite must not join the family — or leave it —
        without this failing. The synthetic control did exactly that: it appeared
        in the binary and `primary_family` went from twenty-one to twenty-two
        with nothing to say so.
        """
        everyone = list(ba.EXPECTED_FAMILY) + list(ba.NOT_GATEABLE) + [ba.SYNTHETIC_CONTROL]
        family = ba.primary_family({"benchmarks": {n: {} for n in everyone}})
        self.assertEqual(len(family), 21)
        self.assertEqual(tuple(family), tuple(sorted(ba.EXPECTED_FAMILY)))
        self.assertNotIn(ba.SYNTHETIC_CONTROL, family)
        self.assertEqual(len(everyone), 28)

    def test_a_new_benchmark_does_not_join_silently(self):
        """An unknown benchmark changes nothing, and is named.

        The earlier version asserted the family *differed* from the declared
        list, which passes precisely when the unknown benchmark has joined — the
        opposite of what the name claims. It could never have caught the thing it
        was written for.
        """
        everyone = list(ba.EXPECTED_FAMILY) + ["brand new benchmark"]
        sample = {"benchmarks": {n: {} for n in everyone}}
        family = ba.primary_family(sample)
        self.assertEqual(tuple(family), tuple(sorted(ba.EXPECTED_FAMILY)),
                         "an unknown benchmark joined the family")
        self.assertEqual(ba.undeclared_benchmarks(sample), ["brand new benchmark"])

    def test_a_declared_benchmark_is_not_undeclared(self):
        sample = {"benchmarks": {n: {} for n in
                                 list(ba.EXPECTED_FAMILY) + list(ba.NOT_GATEABLE)
                                 + [ba.SYNTHETIC_CONTROL]}}
        self.assertEqual(ba.undeclared_benchmarks(sample), [])

    def test_the_synthetic_control_is_judged_on_its_own(self):
        self.assertNotIn(ba.SYNTHETIC_CONTROL, ba.EXPECTED_FAMILY)
        self.assertNotIn(ba.SYNTHETIC_CONTROL, ba.NOT_GATEABLE)

    def test_the_six_unstable_inserts_are_outside_the_family(self):
        # Full names: the whitelist matches exactly, and a truncated name is a
        # different benchmark. The earlier fixture said "simulated IBD" and
        # passed because the family was then a filter that accepted anything not
        # explicitly excluded.
        sample = {"benchmarks": {n: {} for n in
                                 list(ba.NOT_GATEABLE)
                                 + ["find miss", "simulated IBD (100 blocks)"]}}
        family = ba.primary_family(sample)
        self.assertEqual(sorted(family), ["find miss", "simulated IBD (100 blocks)"])
        self.assertEqual(len(ba.NOT_GATEABLE), 6)
        for n in ba.NOT_GATEABLE:
            self.assertNotIn(n, family)

    def test_an_excluded_benchmark_cannot_inflate_the_band(self):
        """Letting one in would widen the maximum for every benchmark that stayed."""
        rng = random.Random(201)
        rounds = 60
        family = {f"bench{i:02d}": [1.0 + rng.uniform(-0.02, 0.02) for _ in range(rounds)]
                  for i in range(5)}
        with_outlier = dict(family)
        with_outlier["insert 123B"] = [1.0 + rng.uniform(-3.0, 3.0) for _ in range(rounds)]
        clean = ba.family_deviation_quantile(family, rounds, 600, 0.95, 1, "one")
        dirty = ba.family_deviation_quantile(
            {k: v for k, v in with_outlier.items() if k not in ba.NOT_GATEABLE},
            rounds, 600, 0.95, 1, "one")
        self.assertAlmostEqual(clean, dirty, places=9,
                               msg="an excluded benchmark reached the family maximum")

    def test_a_diagnostic_metric_does_not_widen_the_primary_band(self):
        """Instructions may move 40%; the wall-time verdict must not notice.

        Driven through `analyse`, not through two calls to the band function with
        the same arguments — an earlier version did exactly that and asserted they
        were equal, which is true of any function and proves nothing about which
        metric the band is built from.
        """
        rng = random.Random(202)
        quiet = [1.0 + rng.uniform(-0.01, 0.01) for _ in range(ROUNDS)]

        def side(elapsed_values, instruction_values):
            return [{**{m: None for m in bench_ab.METRICS},
                     "elapsed": e, "instructions": i}
                    for e, i in zip(elapsed_values, instruction_values)]

        def build(instruction_factor):
            # A fresh generator with a fixed seed, so the elapsed values are
            # byte-for-byte the same in both builds and the only thing that
            # differs between them is the instruction count. Sharing the outer
            # generator made the two runs differ in elapsed too, which is exactly
            # the confound this test exists to rule out.
            local = random.Random(9090)
            benchmarks = {}
            for name in ba.EXPECTED_FAMILY[:6]:
                benchmarks[name] = {
                    "A": side(quiet, [1000.0] * ROUNDS),
                    "B": side([v * (1.0 + local.uniform(-0.002, 0.002)) for v in quiet],
                              [1000.0 * instruction_factor] * ROUNDS),
                }
            return {
                "schema": bench_ab.SCHEMA, "mode": "ab",
                "harness": {"seed": 1, "rounds": ROUNDS,
                            "order": ["AB" if i % 2 else "BA" for i in range(ROUNDS)],
                            "executions": [], "interleaved": True},
                "sides": {"A": {"binary_sha256": "a"*64, "runtime_config": {}},
                          "B": {"binary_sha256": "b"*64, "runtime_config": {}}},
                "identical_binaries": False, "environment": {},
                "benchmarks": benchmarks,
            }

        calm = ba.analyse(build(1.0), build(1.0), 400, 0.95, 1)
        loud = ba.analyse(build(1.40), build(1.0), 400, 0.95, 1)

        # Instructions moved 40% in the second run and are reported as such...
        moved = [f for f in loud["findings"]
                 if f["metric"] == "instructions" and f.get("median")]
        self.assertTrue(moved, "the 40% instruction move was not measured at all")
        self.assertAlmostEqual(max(f["median"] for f in moved), 1.40, places=2)

        # ...and the primary band is identical, because it is built from elapsed.
        self.assertIsNotNone(calm["family"])
        self.assertAlmostEqual(calm["family"]["half_width"],
                               loud["family"]["half_width"], places=9)
        for name, decided in loud["family"]["verdicts"].items():
            self.assertEqual(decided["verdict"], ba.INDISTINGUISHABLE, name)


class ComparisonKind(unittest.TestCase):
    """One image with two settings is not an accidental A/A, and must not read as one."""

    def _sides(self, same_binary, cfg_a, cfg_b):
        return {"A": {"binary_sha256": "a"*64, "runtime_config": cfg_a},
                "B": {"binary_sha256": ("a" if same_binary else "b")*64,
                      "runtime_config": cfg_b}}

    def test_two_builds_are_different_builds(self):
        self.assertEqual(
            bench_ab.comparison_kind(self._sides(False, {}, {}), False),
            "different_builds")

    def test_one_image_two_settings_is_runtime_configured(self):
        sides = self._sides(True, {"UTXOZ_BENCH_SYNTHETIC_LOOKUPS": "5"},
                                  {"UTXOZ_BENCH_SYNTHETIC_LOOKUPS": "20"})
        self.assertEqual(bench_ab.comparison_kind(sides, True), "runtime_configured")

    def test_one_image_same_settings_is_a_control(self):
        self.assertEqual(bench_ab.comparison_kind(self._sides(True, {}, {}), True),
                         "identical")

    def test_a_configured_run_is_not_announced_as_all_noise(self):
        """The banner that says every difference is the machine must not appear here."""
        sample = samples([1.0]*ROUNDS, [1.0]*ROUNDS, identical=True)
        sample["comparison_kind"] = "runtime_configured"
        sample["sides"]["A"]["runtime_config"] = {"UTXOZ_BENCH_SYNTHETIC_LOOKUPS": "5"}
        sample["sides"]["B"]["runtime_config"] = {"UTXOZ_BENCH_SYNTHETIC_LOOKUPS": "20"}
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "s.json"
            path.write_text(json.dumps(sample))
            notes = ba.check_integrity(ba.load(path), path)
        self.assertFalse(any("machine talking" in n for n in notes), notes)
        self.assertTrue(any("runtime configuration" in n for n in notes), notes)

    def test_an_aa_control_may_not_carry_different_settings(self):
        sample = samples([1.0]*ROUNDS, [1.0]*ROUNDS, mode="aa", identical=True)
        sample["comparison_kind"] = "runtime_configured"
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "s.json"
            path.write_text(json.dumps(sample))
            with self.assertRaisesRegex(ba.IntegrityError, "runtime configuration"):
                ba.check_integrity(ba.load(path), path, expect_mode="aa")


class RuntimeConfigParsing(unittest.TestCase):
    def test_pairs_are_parsed(self):
        self.assertEqual(bench_ab.parse_runtime_config("A=1,B=two"),
                         {"A": "1", "B": "two"})

    def test_empty_is_empty(self):
        self.assertEqual(bench_ab.parse_runtime_config(None), {})

    def test_a_malformed_entry_fails_closed(self):
        with self.assertRaisesRegex(bench_ab.HarnessError, "KEY=VALUE"):
            bench_ab.parse_runtime_config("A=1,oops")


class Quiescence(unittest.TestCase):
    """Whether the machine was quiet is evidence, not a boolean nobody can check."""

    def test_signals_carry_a_status_and_never_a_bare_zero(self):
        state = bq.assess(0.30, 0.25, 2, None, 60.0)
        for name, signal in state["signals"].items():
            self.assertIn(signal["status"], (bq.AVAILABLE, bq.UNAVAILABLE), name)
            if signal["status"] == bq.UNAVAILABLE:
                self.assertIsNone(signal["value"], name)
                self.assertIn("reason", signal, name)

    def test_the_decision_names_the_signals_it_used(self):
        state = bq.assess(0.30, 0.25, 2, None, 60.0)
        self.assertIsInstance(state["decided_from"], list)
        self.assertIsInstance(state["unavailable"], list)
        self.assertTrue(state["reason"])
        # a signal cannot both carry the decision and be missing
        self.assertFalse(set(state["decided_from"]) & set(state["unavailable"]))

    def test_the_limits_are_recorded_with_the_decision(self):
        state = bq.assess(0.11, 0.22, 3, 90.0, 60.0)
        self.assertEqual(state["limits"]["max_normalised_load"], 0.11)
        self.assertEqual(state["limits"]["max_utilisation"], 0.22)
        self.assertEqual(state["limits"]["max_run_queue"], 3)

    def test_a_recent_build_is_not_a_quiet_machine(self):
        state = bq.assess(99.0, 99.0, 999, seconds_since_build=5.0,
                          min_seconds_since_build=60.0)
        self.assertFalse(state["quiet"])
        self.assertIn("seconds_since_build", state["failed"])

    def test_load_is_judged_per_core_not_in_absolute_terms(self):
        load = bq.normalised_load()
        if load["status"] != bq.AVAILABLE:
            self.skipTest("no load average here")
        self.assertGreaterEqual(load["cores"], 1)
        self.assertAlmostEqual(load["value"], load["load1"] / load["cores"], places=9)


class RealDifferences(unittest.TestCase):
    def test_synthetic_degradation_is_detected(self):
        rng = random.Random(21)
        control = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02),
                          mode="aa", identical=True)
        ab_run = samples(noisy(rng, 1000, 0.02), noisy(rng, 1300, 0.02))
        found = verdict_of(analyse(ab_run, control))
        self.assertEqual(found["verdict"], ba.REGRESSION)
        self.assertGreater(found["median"], 1.2)

    def test_synthetic_improvement_is_detected(self):
        rng = random.Random(22)
        control = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02),
                          mode="aa", identical=True)
        ab_run = samples(noisy(rng, 1000, 0.02), noisy(rng, 700, 0.02))
        found = verdict_of(analyse(ab_run, control))
        self.assertEqual(found["verdict"], ba.IMPROVEMENT)
        self.assertLess(found["median"], 0.8)

    def test_direction_is_not_inverted(self):
        """Slower is worse. A sign error in a gate is silent the wrong way."""
        rng = random.Random(23)
        control = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02),
                          mode="aa", identical=True)
        slower = verdict_of(analyse(samples(noisy(rng, 1000, 0.02), noisy(rng, 1500, 0.02)), control))
        faster = verdict_of(analyse(samples(noisy(rng, 1000, 0.02), noisy(rng, 500, 0.02)), control))
        self.assertEqual(slower["verdict"], ba.REGRESSION)
        self.assertEqual(faster["verdict"], ba.IMPROVEMENT)


class MissingEvidence(unittest.TestCase):
    def test_absent_hardware_counter_is_no_evidence(self):
        """A runner that will not open a perf counter must not read as no change."""
        rng = random.Random(31)
        control = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02),
                          mode="aa", identical=True)
        ab_run = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02))
        found = verdict_of(analyse(ab_run, control), metric="instructions")
        self.assertEqual(found["verdict"], ba.NO_EVIDENCE)
        self.assertIsNone(found["ratios"])

    def test_zero_denominator_is_no_evidence(self):
        rng = random.Random(32)
        ab_run = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02))
        ab_run["benchmarks"][BENCH]["A"][3]["elapsed"] = 0.0
        self.assertEqual(verdict_of(analyse(ab_run, ab_run))["verdict"], ba.NO_EVIDENCE)

    def test_no_control_is_no_evidence_not_no_change(self):
        rng = random.Random(33)
        ab_run = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02))
        found = verdict_of(analyse(ab_run, None))
        self.assertEqual(found["verdict"], ba.NO_EVIDENCE)
        self.assertNotEqual(found["verdict"], ba.INDISTINGUISHABLE)

    def test_a_benchmark_absent_from_the_control_is_no_evidence(self):
        rng = random.Random(34)
        control = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02),
                          mode="aa", identical=True, name="something else")
        ab_run = samples(noisy(rng, 1000, 0.02), noisy(rng, 1300, 0.02))
        self.assertEqual(verdict_of(analyse(ab_run, control))["verdict"], ba.NO_EVIDENCE)


class Integrity(unittest.TestCase):
    """Provenance failures are errors, not findings. They fail closed."""

    def _check(self, sample, **kw):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "s.json"
            p.write_text(json.dumps(sample))
            return ba.check_integrity(ba.load(p), p, **kw)

    def test_uninterleaved_run_is_refused(self):
        sample = samples([1.0] * ROUNDS, [1.0] * ROUNDS)
        sample["harness"]["interleaved"] = False
        with self.assertRaises(ba.IntegrityError):
            self._check(sample)

    def test_a_single_order_for_every_round_is_refused(self):
        sample = samples([1.0] * ROUNDS, [1.0] * ROUNDS, order=["AB"] * ROUNDS)
        with self.assertRaisesRegex(ba.IntegrityError, "drift"):
            self._check(sample)

    def test_a_short_side_is_refused(self):
        sample = samples([1.0] * ROUNDS, [1.0] * ROUNDS)
        sample["benchmarks"][BENCH]["B"].pop()
        with self.assertRaisesRegex(ba.IntegrityError, "samples for"):
            self._check(sample)

    def test_a_missing_binary_hash_is_refused(self):
        sample = samples([1.0] * ROUNDS, [1.0] * ROUNDS)
        sample["sides"]["B"].pop("binary_sha256")
        with self.assertRaisesRegex(ba.IntegrityError, "binary hash"):
            self._check(sample)

    def test_an_aa_control_with_two_different_binaries_is_refused(self):
        sample = samples([1.0] * ROUNDS, [1.0] * ROUNDS, mode="aa", identical=False)
        with self.assertRaisesRegex(ba.IntegrityError, "two binaries differ"):
            self._check(sample)

    def test_a_control_that_is_not_a_control_is_refused(self):
        sample = samples([1.0] * ROUNDS, [1.0] * ROUNDS, mode="ab")
        with self.assertRaisesRegex(ba.IntegrityError, "expected a 'aa' run"):
            self._check(sample, expect_mode="aa")

    def test_a_foreign_schema_is_refused(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "s.json"
            p.write_text(json.dumps({"schema": "something/else"}))
            with self.assertRaisesRegex(ba.IntegrityError, "schema"):
                ba.load(p)

    def test_identical_binaries_are_reported_not_refused(self):
        """#141's shape: a real PR whose binary did not change.

        Not an error — it is the most informative run there is — but it must be
        said out loud, because every number under it is noise by construction.
        """
        sample = samples([1.0] * ROUNDS, [1.0] * ROUNDS, identical=True)
        notes = self._check(sample)
        self.assertTrue(any("byte-identical" in n for n in notes))


class Ordering(unittest.TestCase):
    def test_the_order_is_seeded_and_reproducible(self):
        self.assertEqual(bench_ab.draw_order(4242, 15), bench_ab.draw_order(4242, 15))
        self.assertNotEqual(bench_ab.draw_order(1, 15), bench_ab.draw_order(2, 15))

    def test_both_orders_occur(self):
        order = bench_ab.draw_order(20260820, 15)
        self.assertIn("AB", order)
        self.assertIn("BA", order)

    def test_every_round_runs_both_sides_once(self):
        for o in bench_ab.draw_order(20260820, 15):
            self.assertEqual(sorted(o), ["A", "B"])


class Artifact(unittest.TestCase):
    def test_the_verdict_is_parseable_and_carries_the_method(self):
        rng = random.Random(41)
        control = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02),
                          mode="aa", identical=True)
        ab_run = samples(noisy(rng, 1000, 0.02), noisy(rng, 1300, 0.02))
        report = analyse(ab_run, control)
        round_tripped = json.loads(json.dumps(report))
        self.assertEqual(round_tripped["schema"], ba.SCHEMA_OUT)
        method = round_tripped["method"]
        for key in ("pairing", "interval", "resamples", "analysis_seed",
                    "noise_floor", "run_seed", "order"):
            self.assertIn(key, method)
        found = verdict_of(round_tripped)
        for key in ("ratios", "min", "median", "max", "ci_low", "ci_high",
                    "noise_floor", "verdict", "reason"):
            self.assertIn(key, found)
        self.assertEqual(len(found["ratios"]), ROUNDS)

    def test_the_report_renders_every_required_field(self):
        rng = random.Random(42)
        control = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02),
                          mode="aa", identical=True)
        ab_run = samples(noisy(rng, 1000, 0.02), noisy(rng, 1300, 0.02))
        text = ba.render(analyse(ab_run, control), [], gate=False, history=None)
        for needle in ("seed", "resamples", "percentile bootstrap", "noise floor",
                       "REGRESSION", "Interleaved"):
            self.assertIn(needle, text)


class GateBehaviour(unittest.TestCase):
    """Whether a finding fails the job is policy, and policy is a flag."""

    def _run(self, ab_run, control, *extra):
        with tempfile.TemporaryDirectory() as d:
            s, c = Path(d) / "ab.json", Path(d) / "aa.json"
            s.write_text(json.dumps(ab_run))
            c.write_text(json.dumps(control))
            return ba.main([str(s), "--control", str(c), "--resamples", "500",
                            "--report", str(Path(d) / "r.md"),
                            "--output", str(Path(d) / "v.json"), *extra])

    def setUp(self):
        rng = random.Random(51)
        self.control = samples(noisy(rng, 1000, 0.02), noisy(rng, 1000, 0.02),
                               mode="aa", identical=True)
        self.regressed = samples(noisy(rng, 1000, 0.02), noisy(rng, 1400, 0.02))

    def test_a_regression_does_not_fail_the_job_by_default(self):
        self.assertEqual(self._run(self.regressed, self.control), 0)

    def test_a_regression_fails_the_job_when_gating_is_on(self):
        self.assertEqual(self._run(self.regressed, self.control, "--gate"), 1)

    def test_an_integrity_failure_fails_even_without_gating(self):
        broken = dict(self.regressed)
        broken["harness"] = dict(broken["harness"], interleaved=False)
        self.assertEqual(self._run(broken, self.control), 3)

    def test_structural_metrics_never_gate(self):
        """Instructions are deterministic, and still not a performance gate.

        They cannot see a change that adds cache misses without adding work, so
        a regression in them is evidence about *what* changed, not proof that it
        got slower.
        """
        rng = random.Random(52)
        control = samples(noisy(rng, 1000, 0.001), noisy(rng, 1000, 0.001),
                          mode="aa", identical=True, metric="instructions")
        run = samples(noisy(rng, 1000, 0.001), noisy(rng, 1500, 0.001),
                      metric="instructions")
        report = analyse(run, control)
        found = verdict_of(report, metric="instructions")
        self.assertEqual(found["verdict"], ba.REGRESSION)
        self.assertEqual(found["role"], "structural")
        self.assertEqual(self._run(run, control, "--gate"), 0)


class EndToEnd(unittest.TestCase):
    def test_the_runner_refuses_a_binary_that_is_not_there(self):
        rc = subprocess.run(
            [sys.executable, str(Path(__file__).parent / "bench_ab.py"),
             "--binary-a", "/nonexistent/a", "--binary-b", "/nonexistent/b",
             "--seed", "1", "--rounds", "2"],
            capture_output=True, text=True,
        )
        self.assertEqual(rc.returncode, 3)
        self.assertIn("does not exist", rc.stderr)

    def test_one_round_is_refused(self):
        # `sys.executable` rather than a path like /bin/true: this harness runs on
        # Linux, macOS and NixOS, where that path does not exist at all.
        rc = subprocess.run(
            [sys.executable, str(Path(__file__).parent / "bench_ab.py"),
             "--binary-a", sys.executable, "--binary-b", sys.executable,
             "--seed", "1", "--rounds", "1"],
            capture_output=True, text=True,
        )
        self.assertEqual(rc.returncode, 2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
