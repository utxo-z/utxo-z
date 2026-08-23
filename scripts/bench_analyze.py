#!/usr/bin/env python3
"""Turn paired samples into a verdict, and say how the verdict was reached.

Two separate questions, deliberately not mixed
----------------------------------------------

**Is the harness sound?** Did both sides run, run the same benchmarks, produce
the number of samples they claim, and identify the binaries they measured? A
"no" here is an error: the run tells us nothing, and it fails the job.

**What does the evidence say?** Improvement, regression, indistinguishable from
noise, or not enough evidence to tell. A performance verdict is a finding, not
an error, and whether it gates is a policy set outside this file.

How a difference is decided
---------------------------

Ratios are paired: for each round, `B / A` from that round's two executions.
Pairing is what makes the interleaving worth doing — a machine that slows down
between round 3 and round 4 slows both sides of round 4, and the ratio is
untouched by it.

The interval around the median of those ratios is a percentile bootstrap. Not a
t-interval: the ratios are skewed and heavy-tailed — one benchmark moved 116%
between runs of the *same* binary — and a method that assumes symmetry would
report confidence it has not earned.

The width that counts as "no difference" is not chosen here. It comes from an
A/A control: the same build measured as both sides, on the same runner, in the
same job. That is deliberate, and it is the reason the floor is not derived from
the A/B run being judged — a change that makes a benchmark *more variable* would
otherwise widen its own tolerance until it could not be caught.
"""

from __future__ import annotations

import argparse
import json
import math
import random
import sys
from pathlib import Path
from statistics import median

SCHEMA_IN = "utxoz.bench.ab/1"
SCHEMA_OUT = "utxoz.bench.verdict/1"

IMPROVEMENT = "IMPROVEMENT"
REGRESSION = "REGRESSION"
INDISTINGUISHABLE = "INDISTINGUISHABLE"
NO_EVIDENCE = "NO_EVIDENCE"

POLICY_VERSION = "utxoz.bench.policy/1"

# ---------------------------------------------------------------------------
# The statistical family, stated once and in one place.
#
# A family-wise correction is only meaningful against a declared family. Ours is
# the twenty-one gateable benchmarks judged on wall time, and nothing else:
#
#   * `elapsed` is the only primary metric. It is the one that answers "is this
#     slower", and it is the one the floor and the simultaneous band are built
#     from.
#   * cycles, instructions, branches, faults and the rest are diagnostic. They
#     say *what* changed and are reported, but they issue no verdict — a family
#     of twenty-one benchmarks times seven metrics is a hundred and forty-seven
#     intervals, and correcting one metric while leaving six uncorrected would
#     put the expected false finding straight back in.
#   * the six unstable inserts are outside the family entirely. They accumulate
#     entries across adaptive epochs and cross capacity and rotations, so they
#     measure a non-stationary mixture of insert and store lifecycle. They are
#     reported separately and are not gateable pending a benchmark redesign;
#     letting them into the maximum would inflate the band for everyone else.
# ---------------------------------------------------------------------------
PRIMARY_METRIC = "elapsed"

# The planted effect, predeclared and judged on its own.
#
# It is a positive control, not a subject: its job is to carry a difference the
# harness is supposed to find, so "did it move" is asked of it directly rather
# than as one of a family. Keeping it out of the family also keeps the two
# questions from contaminating each other — the family exists to bound how often
# an *unchanged* benchmark is declared, and a benchmark that was deliberately
# changed has no business setting that bound.
#
# It was silently inside the family until this was written down: with the
# benchmark added, `primary_family` returned twenty-two.
SYNTHETIC_CONTROL = "synthetic control: scaled lookups"

NOT_GATEABLE = frozenset({
    "insert 123B",
    "insert P2PKH (43B)",
    "insert 89B",
    "insert P2SH (41B)",
    "insert-heavy workload (1K inserts, 100 finds)",
    "bulk insert 10K (P2PKH)",
})


# Every benchmark the family is expected to contain, by name. A list rather than
# a rule, so that adding or renaming a benchmark fails a test instead of quietly
# changing what "family-wise" was corrected over.
EXPECTED_FAMILY = (
    "apply_deletes (100 entries)",
    "apply_deletes 1K",
    "apply_deletes hit (1 entry)",
    "apply_deletes miss (1 entry)",
    "batch find 1K hits",
    "bulk insert 10K (chain mix)",
    "close+reopen 100K (P2PKH)",
    "close+reopen 10K (123B)",
    "close+reopen 10K (P2PKH)",
    "close+reopen 1K (P2PKH)",
    "close+reopen 50K (123B)",
    "close+reopen 50K (P2PKH)",
    "find hit (chain mix)",
    "find hit (latest version)",
    "find miss",
    "read-heavy workload (5K finds on 1K entries)",
    "simulated IBD (100 blocks)",
    "telemetry: active hit, first class",
    "telemetry: miss, every class probed",
    "telemetry: mixed 9 active hits to 1 historical",
    "telemetry: sweep of 256 keys over three generations",
)


def primary_family(samples: dict) -> list[str]:
    """The benchmarks a session issues family-wise verdicts about.

    The declared members that are present, and nothing else. It used to be a
    filter — everything except the excluded — which meant a benchmark added to
    the suite joined the family at run time without anyone deciding it had. The
    family is what a family-wise correction is corrected over, so its membership
    is a decision rather than a leftover.

    A benchmark belonging to none of the three lists is not silently dropped
    either: `undeclared_benchmarks` names it and the report says so.
    """
    present = set(samples.get("benchmarks") or {})
    return sorted(n for n in EXPECTED_FAMILY if n in present)


def undeclared_benchmarks(samples: dict) -> list[str]:
    """Benchmarks the policy has never heard of.

    Neither in the family, nor among the six that are not gateable, nor the
    synthetic control. Reported rather than assumed into or out of the family,
    because both silences are wrong: joining changes what the correction covers,
    and dropping loses a benchmark nobody decided to lose.
    """
    known = set(EXPECTED_FAMILY) | set(NOT_GATEABLE) | {SYNTHETIC_CONTROL}
    return sorted(set(samples.get("benchmarks") or {}) - known)

# Which way is better, declared per metric and never inferred.
#
# Every metric this harness currently reads is a cost per operation — seconds,
# cycles, instructions, faults — so `head/base > 1` means worse. That is a fact
# about these particular metrics, not a rule about ratios: a throughput figure
# in ops/sec would invert it, and `mdape` is a dispersion, not a cost, so it has
# no direction at all. A metric added without an entry here has no declared
# semantics and is refused rather than assumed.
LOWER_IS_BETTER = "lower_is_better"
HIGHER_IS_BETTER = "higher_is_better"
NO_DIRECTION = "no_direction"

METRIC_DIRECTION = {
    "elapsed": LOWER_IS_BETTER,
    "cycles": LOWER_IS_BETTER,
    "instructions": LOWER_IS_BETTER,
    "branches": LOWER_IS_BETTER,
    "branchmisses": LOWER_IS_BETTER,
    "pagefaults": LOWER_IS_BETTER,
    "contextswitches": LOWER_IS_BETTER,
    "mdape": NO_DIRECTION,
}

# What each metric is allowed to mean, from the calibration policy:
#
#   gate         may fail a job, once gating is switched on
#   informative  reported, never fails
#   structural   reported as evidence about *what changed*, not about speed
#
# Instructions and branches are all but deterministic — 0.016% and 0.029%
# median spread across runs of one binary — which makes them excellent at
# answering "did the work change?" and poor at answering "is it slower?": they
# cannot see a locality regression that adds cache misses without adding
# instructions. So they are structural, and never a performance gate on their
# own.
METRIC_ROLE = {
    "elapsed": "gate",
    "instructions": "structural",
    "branches": "structural",
    "cycles": "informative",
    "branchmisses": "informative",
    "pagefaults": "informative",
    "contextswitches": "informative",
}


class IntegrityError(RuntimeError):
    """The run cannot be interpreted. Distinct from a bad result."""


def load(path: Path) -> dict:
    try:
        data = json.loads(Path(path).read_text())
    except (OSError, json.JSONDecodeError) as e:
        raise IntegrityError(f"{path}: {e}") from e
    if data.get("schema") != SCHEMA_IN:
        raise IntegrityError(f"{path}: expected schema {SCHEMA_IN}, got {data.get('schema')!r}")
    return data


def check_integrity(samples: dict, path: Path, expect_mode: str | None = None) -> list[str]:
    """Everything that must be true before a number here means anything.

    Returns the notes worth printing. Raises when the run is not interpretable.
    """
    notes: list[str] = []
    harness = samples.get("harness") or {}
    sides = samples.get("sides") or {}

    for key in ("A", "B"):
        side = sides.get(key)
        if not side:
            raise IntegrityError(f"{path}: side {key} is missing")
        if not side.get("binary_sha256"):
            raise IntegrityError(f"{path}: side {key} has no binary hash")

    rounds = harness.get("rounds")
    if not isinstance(rounds, int) or rounds < 2:
        raise IntegrityError(f"{path}: {rounds!r} rounds is not a comparison")
    if not harness.get("interleaved"):
        raise IntegrityError(f"{path}: the run was not interleaved")

    order = harness.get("order") or []
    if len(order) != rounds:
        raise IntegrityError(f"{path}: {len(order)} orders for {rounds} rounds")
    if len(set(order)) < 2 and rounds > 3:
        raise IntegrityError(
            f"{path}: every round ran {order[0]} first, so temporal drift is not cancelled"
        )

    benchmarks = samples.get("benchmarks") or {}
    if not benchmarks:
        raise IntegrityError(f"{path}: no benchmarks")
    for name, sides_data in benchmarks.items():
        for key in ("A", "B"):
            got = len(sides_data.get(key) or [])
            if got != rounds:
                raise IntegrityError(
                    f"{path}: {name!r} side {key} has {got} samples for {rounds} rounds"
                )

    if expect_mode and samples.get("mode") != expect_mode:
        raise IntegrityError(
            f"{path}: expected a {expect_mode!r} run, artifact says {samples.get('mode')!r}"
        )

    kind = samples.get("comparison_kind")
    if kind == "runtime_configured":
        cfg_a = (sides["A"] or {}).get("runtime_config")
        cfg_b = (sides["B"] or {}).get("runtime_config")
        notes.append(
            "One image, one SHA-256, two runtime configurations "
            f"(A={cfg_a}, B={cfg_b}). Every benchmark outside the configured one "
            "executes the same bytes, so a finding in those is the instrument "
            "rather than the linker — which is what a two-build comparison could "
            "never establish."
        )
    elif samples.get("identical_binaries"):
        notes.append(
            "The two sides are byte-identical builds under the same configuration, "
            "so every difference below is the machine talking. This is what PR #141 "
            "was, and what the previous lane reported as changes between -53% and "
            "+102%."
        )
    if samples.get("mode") == "aa":
        if not samples.get("identical_binaries"):
            raise IntegrityError(f"{path}: an A/A control whose two binaries differ")
        if kind == "runtime_configured":
            raise IntegrityError(
                f"{path}: an A/A control whose sides carry different runtime "
                "configuration is not a control"
            )

    return notes


def paired_ratios(sides_data: dict, metric: str) -> list[float] | None:
    """`B/A` per round, or None when the metric cannot be compared.

    A missing field, a zero denominator or a non-finite value ends the metric
    here. Substituting a default would turn "this runner would not open a perf
    counter" into "nothing changed", which is the specific lie this whole
    exercise exists to stop telling.
    """
    a_samples = sides_data.get("A") or []
    b_samples = sides_data.get("B") or []
    if len(a_samples) != len(b_samples) or not a_samples:
        return None
    ratios = []
    for a, b in zip(a_samples, b_samples):
        av, bv = a.get(metric), b.get(metric)
        if av is None or bv is None:
            return None
        try:
            av, bv = float(av), float(bv)
        except (TypeError, ValueError):
            return None
        if not math.isfinite(av) or not math.isfinite(bv) or av <= 0:
            return None
        ratios.append(bv / av)
    return ratios


def bootstrap_median_ci(ratios: list[float], resamples: int, confidence: float,
                        seed: int) -> tuple[float, float, float]:
    """Percentile bootstrap interval for the median ratio.

    Resampling the paired ratios keeps the pairing intact — each draw is a whole
    round's A-and-B, not an A from one round against a B from another.
    """
    rng = random.Random(seed)
    n = len(ratios)
    medians = []
    for _ in range(resamples):
        medians.append(median(rng.choice(ratios) for _ in range(n)))
    medians.sort()
    tail = (1.0 - confidence) / 2.0
    lo = medians[max(0, int(math.floor(tail * resamples)))]
    hi = medians[min(resamples - 1, int(math.ceil((1.0 - tail) * resamples)) - 1)]
    return median(ratios), lo, hi


def family_deviation_quantile(ratios_by_benchmark: dict[str, list[float]],
                             rounds: int, resamples: int, confidence: float,
                             seed: int, centre: str) -> float:
    """How far the worst of a whole family of benchmarks strays, in one number.

    Twenty-one intervals at 95% are not a 95% statement about a session: with
    nothing changed, about one benchmark in twenty falls outside its own
    interval, so one finding per session is the expected count rather than a
    result. On mac that expected finding was `find miss` at +1.85%, and looking
    at it alone could not tell it from a small real effect.

    Rounds are resampled **whole**. A round is one A execution against one B
    execution and every benchmark in it shares whatever state that pair of
    processes was in; drawing each benchmark's ratios independently would break
    that correlation and give a band narrower than the family deserves.

    `centre` selects which of the two questions is being asked:

      "one"      how far medians stray from 1 when nothing changed. Used on the
                 A/A control, where the true ratio is known to be 1, and the
                 answer is the family floor.
      "observed" how far a bootstrap median strays from *this run's* median.
                 Used on the A/B run, where the true ratio is unknown, and the
                 answer is the half-width of the simultaneous interval. Centring
                 on the observed median is what keeps a large real effect in one
                 benchmark from inflating the band for the other twenty — the
                 deviation measured is from that benchmark's own centre, not
                 from 1.
    """
    rng = random.Random(seed)
    names = [n for n, rs in ratios_by_benchmark.items() if any(r is not None for r in rs)]
    observed = {n: median([r for r in ratios_by_benchmark[n] if r is not None])
                for n in names}
    maxima = []
    for _ in range(resamples):
        draw = [rng.randrange(rounds) for _ in range(rounds)]
        worst = 0.0
        for n in names:
            vals = [ratios_by_benchmark[n][i] for i in draw
                    if i < len(ratios_by_benchmark[n]) and ratios_by_benchmark[n][i] is not None]
            if not vals:
                continue
            reference = 1.0 if centre == "one" else observed[n]
            worst = max(worst, abs(median(vals) - reference))
        maxima.append(worst)
    maxima.sort()
    index = min(len(maxima) - 1, int(math.ceil(confidence * len(maxima))) - 1)
    return maxima[max(0, index)]


def classify_family(observed_median: float, half_width: float,
                    family_floor: float,
                    equivalence_multiple: float = 2.0) -> tuple[str, str]:
    """The session-level verdict for one benchmark.

    A regression needs the *simultaneous* lower bound of the effect to clear the
    family floor measured on the A/A control — not the per-benchmark bound, and
    not the per-benchmark floor. Both loosenings were what let one expected
    finding in twenty-one read as a result.
    """
    lo = observed_median - half_width
    hi = observed_median + half_width
    if lo > 1.0 + family_floor:
        return REGRESSION, (f"the simultaneous lower bound {(lo-1)*100:+.2f}% clears the "
                            f"family floor of {family_floor*100:.2f}%")
    if hi < 1.0 - family_floor:
        return IMPROVEMENT, (f"the simultaneous upper bound {(hi-1)*100:+.2f}% clears the "
                             f"family floor of {family_floor*100:.2f}%")
    margin = family_floor * equivalence_multiple
    if lo >= 1.0 - margin and hi <= 1.0 + margin:
        return INDISTINGUISHABLE, (
            f"the simultaneous interval [{(lo-1)*100:+.2f}%, {(hi-1)*100:+.2f}%] "
            f"fits inside {margin:.2%}, {equivalence_multiple:g}x the "
            f"{family_floor:.2%} family floor")
    return NO_EVIDENCE, (
        f"the simultaneous interval [{(lo-1)*100:+.2f}%, {(hi-1)*100:+.2f}%] "
        f"neither clears the {family_floor:.2%} family floor nor fits inside the "
        f"{margin:.2%} equivalence margin; this session did not resolve it")


def floor_from_control(control: dict, name: str, metric: str, resamples: int,
                       confidence: float, seed: int) -> dict | None:
    """How far this benchmark's answer can stray when nothing has changed.

    The A/A run measured the same build as both sides, so its paired median
    should be exactly 1. The floor is how far the *interval around that median*
    reaches instead — the resolution of the very statistic the A/B run is judged
    by, measured on a run where the true answer is known.

    An earlier version used a high quantile of `|r - 1|` over the individual
    ratios. With fifteen pairs that is the worst single pair, and it produced
    floors between 22% and 93% on real data: a benchmark whose paired median
    landed within 0.2% of 1 was assigned a 22% tolerance because one of its
    fifteen pairs had swung. Comparing a median's interval against a single
    observation's extreme is not comparing like with like.

    The extreme is still returned alongside, because "this benchmark's pairs
    swing by 90%" is worth printing even when it does not set the threshold.
    """
    if not control:
        return None
    sides_data = (control.get("benchmarks") or {}).get(name)
    if not sides_data:
        return None
    ratios = paired_ratios(sides_data, metric)
    if not ratios:
        return None
    _, lo, hi = bootstrap_median_ci(ratios, resamples, confidence, seed)
    # The formula, written once here and published with every finding:
    #
    #     floor = max(|aa_ci_low - 1|, |aa_ci_high - 1|)
    #
    # The maximum of the two arms, not their average and not half the width. The
    # A/A interval is not symmetric around 1 — the ratios are skewed, so the
    # bootstrap puts more of the interval on one side — and taking the wider arm
    # is what makes the floor a bound rather than a typical case.
    return {
        "aa_ci_low": lo,
        "aa_ci_high": hi,
        "floor": max(abs(lo - 1.0), abs(hi - 1.0)),
        "formula": "max(abs(aa_ci_low - 1), abs(aa_ci_high - 1))",
        "policy_version": POLICY_VERSION,
        "control_pairs": len(ratios),
        "control_worst_pair": max(abs(r - 1.0) for r in ratios),
    }


def classify(lo: float, hi: float, noise_floor: float | None,
             equivalence_multiple: float = 2.0,
             direction: str | None = LOWER_IS_BETTER) -> tuple[str, str]:
    """The verdict, and why it was reached.

    Detection and equivalence are two different questions and they get two
    different thresholds, which is why there are two numbers here rather than
    one.

    *Is there an effect?* The interval has to clear the floor — the resolution
    the A/A control measured for this benchmark. Nothing narrower than what the
    instrument can see is called a difference.

    *Is there no effect?* The interval has to fit inside a margin, and the margin
    cannot be the floor itself. The floor is the half-width of the control's own
    interval, and the A/B interval is built from a sample of the same size on the
    same machine, so the two are about equally wide: asking one to fit inside the
    other is asking a coin to land on its edge. An earlier version did exactly
    that and reported NO_EVIDENCE for a quiet 0.5% difference it had in fact
    resolved. The margin is the floor times `equivalence_multiple`, so an
    interval no worse than the control's has room to fit.

    Between the two thresholds sits a band that is neither: an interval that
    does not clear the floor and does not fit inside the margin has not answered
    the question, and says so.
    """
    # Only costs are supported. Every metric read today is one — seconds, cycles,
    # instructions, faults — so `ratio > 1` means worse. A throughput metric would
    # invert that, and the failure would be silent in the worst direction:
    # a regression reported as an improvement. Refused rather than assumed.
    if direction == HIGHER_IS_BETTER:
        return NO_EVIDENCE, ("this rule reads a ratio above 1 as worse, which is "
                             "wrong for a higher-is-better metric; it is refused "
                             "rather than inverted silently")
    if direction is not None and direction != LOWER_IS_BETTER:
        return NO_EVIDENCE, f"no verdict rule for direction {direction!r}"
    if noise_floor is None:
        return NO_EVIDENCE, "no A/A control for this benchmark and metric"
    detect_lo, detect_hi = 1.0 - noise_floor, 1.0 + noise_floor
    margin = noise_floor * equivalence_multiple
    equiv_lo, equiv_hi = 1.0 - margin, 1.0 + margin
    if lo > detect_hi:
        return REGRESSION, f"the whole interval is above the {noise_floor:.2%} noise floor"
    if hi < detect_lo:
        return IMPROVEMENT, f"the whole interval is below the {noise_floor:.2%} noise floor"
    if lo >= equiv_lo and hi <= equiv_hi:
        return INDISTINGUISHABLE, (
            f"the interval fits inside {margin:.2%}, "
            f"{equivalence_multiple:g}x the measured {noise_floor:.2%} floor"
        )
    return NO_EVIDENCE, (
        f"the interval [{lo:.4f}, {hi:.4f}] neither clears the {noise_floor:.2%} "
        f"floor nor fits inside the {margin:.2%} equivalence margin; this run did "
        "not resolve the question"
    )


def analyse(samples: dict, control: dict | None, resamples: int, confidence: float,
            seed: int, equivalence_multiple: float = 2.0) -> dict:
    findings = []
    for name, sides_data in sorted((samples.get("benchmarks") or {}).items()):
        for metric, role in METRIC_ROLE.items():
            ratios = paired_ratios(sides_data, metric)
            direction = METRIC_DIRECTION.get(metric)
            entry = {
                "benchmark": name,
                "metric": metric,
                "role": role,
                "direction": direction,
                "lower_is_better": direction == LOWER_IS_BETTER,
            }
            if direction is None:
                entry.update({
                    "verdict": NO_EVIDENCE,
                    "reason": f"{metric!r} has no declared direction, so a ratio "
                              "above 1 cannot be read as better or worse",
                    "ratios": None,
                })
                findings.append(entry)
                continue
            if ratios is None:
                entry.update({
                    "verdict": NO_EVIDENCE,
                    "reason": "the metric is missing, zero or not comparable on at least "
                              "one side (a runner that will not open a perf counter "
                              "lands here, and must not be read as no change)",
                    "ratios": None,
                })
                findings.append(entry)
                continue

            ratio, lo, hi = bootstrap_median_ci(ratios, resamples, confidence, seed)
            measured = floor_from_control(control, name, metric, resamples,
                                          confidence, seed) if control else None
            noise_floor = measured["floor"] if measured else None
            verdict, reason = classify(lo, hi, noise_floor, equivalence_multiple,
                                       direction)
            entry.update({
                "ratios": [round(r, 6) for r in ratios],
                "samples": len(ratios),
                "min": min(ratios),
                "median": ratio,
                "max": max(ratios),
                "ci_low": lo,
                "ci_high": hi,
                "noise_floor": noise_floor,
                "floor_detail": measured,
                "equivalence_multiple": equivalence_multiple,
                "equivalence_margin": (noise_floor * equivalence_multiple
                                       if noise_floor is not None else None),
                "verdict": verdict,
                "reason": reason,
            })
            findings.append(entry)
    # The family pass. Until this existed the analyser emitted twenty-one
    # independent intervals and called each one a verdict, which is the rule this
    # whole exercise established is wrong: at 95% about one of twenty-one fires
    # whether or not anything changed. The per-metric findings above stay — they
    # are the diagnostic detail — but for the primary metric the verdict now comes
    # from the simultaneous band.
    family = primary_family(samples)
    family_report = None
    if family and control:
        ab_ratios = {n: paired_ratios(samples["benchmarks"][n], PRIMARY_METRIC)
                     for n in family}
        aa_ratios = {n: paired_ratios((control.get("benchmarks") or {}).get(n, {}),
                                      PRIMARY_METRIC)
                     for n in family}
        usable = {n: r for n, r in ab_ratios.items() if r}
        control_usable = {n: r for n, r in aa_ratios.items() if r}
        rounds = (samples.get("harness") or {}).get("rounds") or 0
        control_rounds = (control.get("harness") or {}).get("rounds") or 0
        if usable and control_usable and rounds and control_rounds:
            family_floor = family_deviation_quantile(
                control_usable, control_rounds, resamples, confidence, seed, "one")
            half_width = family_deviation_quantile(
                usable, rounds, resamples, confidence, seed + 1, "observed")
            verdicts = {}
            for name, ratios in usable.items():
                observed = median(ratios)
                verdict, reason = classify_family(observed, half_width, family_floor,
                                                  equivalence_multiple)
                verdicts[name] = {"median": observed, "verdict": verdict,
                                  "reason": reason,
                                  "simultaneous_low": observed - half_width,
                                  "simultaneous_high": observed + half_width}
            family_report = {
                "members": sorted(usable),
                "undeclared": undeclared_benchmarks(samples),
                "excluded_not_gateable": sorted(NOT_GATEABLE),
                "excluded_synthetic_control": SYNTHETIC_CONTROL,
                "metric": PRIMARY_METRIC,
                "family_floor": family_floor,
                "half_width": half_width,
                "equivalence_multiple": equivalence_multiple,
                "verdicts": verdicts,
            }
            # The family verdict is the one the findings carry for the primary
            # metric, so nothing downstream can read the uncorrected one by
            # accident.
            for finding in findings:
                if finding["metric"] == PRIMARY_METRIC and finding["benchmark"] in verdicts:
                    decided = verdicts[finding["benchmark"]]
                    finding["per_benchmark_verdict"] = finding["verdict"]
                    finding["verdict"] = decided["verdict"]
                    finding["reason"] = decided["reason"]
                    finding["simultaneous_low"] = decided["simultaneous_low"]
                    finding["simultaneous_high"] = decided["simultaneous_high"]
                    finding["family_floor"] = family_floor

    # The synthetic control is predeclared and judged on its own: it carries a
    # difference on purpose, so it has no business setting the bound on how often
    # an unchanged benchmark is declared, and it never gates.
    synthetic = next((f for f in findings
                      if f["benchmark"] == SYNTHETIC_CONTROL
                      and f["metric"] == PRIMARY_METRIC), None)
    if synthetic is not None:
        synthetic["role"] = "positive_control"

    return {
        "schema": SCHEMA_OUT,
        "family": family_report,
        "method": {
            "policy_version": POLICY_VERSION,
            "pairing": "head/base within each interleaved round",
            "floor_formula": "max(abs(aa_ci_low - 1), abs(aa_ci_high - 1))",
            "detection_rule": "regression iff ci_low > 1 + floor; "
                              "improvement iff ci_high < 1 - floor",
            "equivalence_rule": "indistinguishable iff the whole interval lies "
                                "within 1 +- equivalence_multiple * floor",
            "interval": f"percentile bootstrap of the paired median, {confidence:.0%}",
            "resamples": resamples,
            "analysis_seed": seed,
            "noise_floor": "half-width of the A/A control's own bootstrap interval "
                           "for the same statistic",
            "equivalence_margin": f"{equivalence_multiple:g}x the measured floor",
            "run_seed": (samples.get("harness") or {}).get("seed"),
            "order": (samples.get("harness") or {}).get("order"),
        },
        "sides": samples.get("sides"),
        "identical_binaries": samples.get("identical_binaries"),
        "environment": samples.get("environment"),
        "control_present": control is not None,
        "findings": findings,
    }


def _fmt_pct(ratio: float) -> str:
    return f"{(ratio - 1.0) * 100:+.2f}%"


def render(report: dict, notes: list[str], gate: bool, history: str | None) -> str:
    method = report["method"]
    sides = report.get("sides") or {}
    lines: list[str] = ["## Controlled A/B benchmark comparison", ""]

    for note in notes:
        lines += ["> [!IMPORTANT]", f"> {note}", ""]

    a, b = sides.get("A", {}), sides.get("B", {})
    lines += [
        "| | A (base) | B (head) |",
        "|---|---|---|",
        f"| commit | `{a.get('commit') or '?'}` | `{b.get('commit') or '?'}` |",
        f"| binary sha256 | `{(a.get('binary_sha256') or '')[:16]}` | `{(b.get('binary_sha256') or '')[:16]}` |",
        "",
        f"Interleaved, run seed `{method['run_seed']}`, order "
        f"`{' '.join(method['order'] or [])}`. Interval: {method['interval']}, "
        f"{method['resamples']} resamples, analysis seed `{method['analysis_seed']}`. "
        f"Noise floor: {method['noise_floor']}.",
        "",
    ]
    if not report.get("control_present"):
        lines += [
            "> [!WARNING]",
            "> No A/A control was supplied, so no benchmark has a measured noise "
            "floor and every verdict below is NO_EVIDENCE. That is the correct "
            "answer, not a failure to compute one.",
            "",
        ]

    by_role: dict[str, list[dict]] = {}
    for f in report["findings"]:
        by_role.setdefault(f["role"], []).append(f)

    titles = {
        "gate": "Timing — the gating metric",
        "structural": "Structural — did the work itself change?",
        "informative": "Informative",
    }
    for role in ("gate", "structural", "informative"):
        rows = by_role.get(role) or []
        if not rows:
            continue
        interesting = [r for r in rows if r["verdict"] in (REGRESSION, IMPROVEMENT)]
        shown = interesting if role == "informative" else rows
        lines += [f"### {titles[role]}", ""]
        if not shown:
            lines += ["Nothing outside the noise floor.", ""]
            continue
        lines += [
            "| benchmark | metric | min | median | max | interval | floor | verdict |",
            "|---|---|---:|---:|---:|---|---:|---|",
        ]
        for r in sorted(shown, key=lambda x: (x["verdict"], x["benchmark"])):
            if r.get("ratios") is None:
                lines.append(
                    f"| {r['benchmark']} | {r['metric']} | — | — | — | — | — | {r['verdict']} |"
                )
                continue
            # For the primary metric the verdict came from the simultaneous
            # interval against the family floor, so those are the numbers the row
            # has to carry. Printing a per-benchmark interval beside a family
            # verdict shows a reader figures the decision was not made from,
            # which is how a correct verdict comes to look like a mistake.
            if r.get("simultaneous_low") is not None:
                low, high = r["simultaneous_low"], r["simultaneous_high"]
                floor_value = r.get("family_floor")
            else:
                low, high = r["ci_low"], r["ci_high"]
                floor_value = r.get("noise_floor")
            floor = f"±{floor_value*100:.2f}%" if floor_value is not None else "—"
            lines.append(
                f"| {r['benchmark']} | {r['metric']} | {_fmt_pct(r['min'])} | "
                f"{_fmt_pct(r['median'])} | {_fmt_pct(r['max'])} | "
                f"[{_fmt_pct(low)}, {_fmt_pct(high)}] | {floor} | "
                f"{r['verdict']} |"
            )
        lines.append("")

    counts: dict[str, int] = {}
    for f in report["findings"]:
        counts[f["verdict"]] = counts.get(f["verdict"], 0) + 1
    lines += [
        "### Summary",
        "",
        ", ".join(f"{v}: {n}" for v, n in sorted(counts.items())) or "nothing measured",
        "",
    ]
    if not gate:
        lines += [
            "> [!NOTE]",
            "> Performance verdicts are informative in this run: the timing gate is "
            "off while the A/A control is being calibrated on GitHub-hosted runners. "
            "Harness integrity failures do fail the job.",
            "",
        ]
    if history:
        lines += ["### Historical context (not part of the verdict)", "", history, ""]
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("samples", help="artifact from `bench_ab.py`")
    parser.add_argument("--control", default=None,
                        help="A/A artifact measured on the same runner")
    parser.add_argument("--resamples", type=int, default=10000)
    parser.add_argument("--confidence", type=float, default=0.95)
    parser.add_argument("--seed", type=int, default=1,
                        help="seed for the bootstrap, so the interval is reproducible")
    parser.add_argument("--equivalence-multiple", type=float, default=2.0,
                        help="how many measured floors wide the no-difference margin is")
    parser.add_argument("--report", default=None, help="markdown output path")
    parser.add_argument("--output", default=None, help="structured verdict path")
    parser.add_argument("--history", default=None,
                        help="markdown from the historical lane, included as context only")
    parser.add_argument("--gate", action="store_true",
                        help="exit non-zero on a REGRESSION in a gating metric")
    args = parser.parse_args(argv)

    try:
        samples = load(Path(args.samples))
        notes = check_integrity(samples, Path(args.samples))
        control = None
        if args.control:
            control = load(Path(args.control))
            check_integrity(control, Path(args.control), expect_mode="aa")
    except IntegrityError as e:
        # Fail closed, and loudly. This is not a slow benchmark; it is a run
        # that cannot be read.
        print(f"harness integrity: {e}", file=sys.stderr)
        return 3

    report = analyse(samples, control, args.resamples, args.confidence, args.seed,
                     args.equivalence_multiple)
    history = Path(args.history).read_text() if args.history and Path(args.history).exists() else None
    markdown = render(report, notes, args.gate, history)

    if args.output:
        Path(args.output).write_text(json.dumps(report, indent=2))
    if args.report:
        Path(args.report).write_text(markdown)
    else:
        print(markdown)

    if args.gate:
        # Only the primary metric, only the declared family: the six
        # non-stationary inserts are outside it, and the synthetic control is a
        # positive control whose whole purpose is to move.
        bad = [f for f in report["findings"]
               if f["metric"] == PRIMARY_METRIC
               and f["benchmark"] not in NOT_GATEABLE
               and f["benchmark"] != SYNTHETIC_CONTROL
               and f["verdict"] == REGRESSION]
        if bad:
            for f in bad:
                print(f"regression: {f['benchmark']} {f['metric']} {_fmt_pct(f['median'])}",
                      file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
