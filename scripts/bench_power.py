#!/usr/bin/env python3
"""What this harness can and cannot see, computed from a host's own A/A rounds.

The numbers in any report about resolution — the family-wise error rate, the
smallest effect that gets found — are only worth as much as the ability to
recompute them. This is that computation, kept beside the harness rather than in
a scratch directory, taking the sample artifacts as input and recording their
hashes so a later reader can tell whether the inputs are the ones the report used.

## What it measures

**Family-wise error rate.** A session is one trial. It counts as a failure if
*any* of the twenty-one negative controls is declared, not once per benchmark
declared: the family-wise rate is the probability that a session says something
it should not, so dividing by twenty-one times the trials answers the
per-comparison question instead and flatters the correction twentyfold.

**Power.** How often a planted effect of a given size is found, with a binomial
interval — 295 of 300 is 98%, give or take a couple of points, and reporting it
as a bare percentage claims a precision three hundred trials do not carry. The
interval is Wilson's rather than the textbook `p ± z·√(p(1−p)/n)`, which at the
edges produces intervals of zero width and, for a single success in three
hundred, negative probabilities.

Where no failure is seen at all, the rule of three (`3/N`) is printed beside the
interval: it is the same answer to two decimals and can be checked without a
computer.

## What it resamples

Whole rounds, drawn from real executions of the A/A control on that host. A
round is one A execution against one B execution, and every benchmark in it
shares whatever state that pair of processes was in; drawing each benchmark
independently would break the correlation and give a band narrower than the data
deserve. The effect is applied to one benchmark, which is the shape of a real
regression.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import random
import statistics as st
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import bench_analyze as ba

SCHEMA = "utxoz.bench.power/1"


def wilson(hits: int, n: int, z: float = 1.96) -> tuple[float, float]:
    """A binomial interval that stays inside [0, 1] and never collapses to a point."""
    if n == 0:
        return (0.0, 1.0)
    p = hits / n
    denominator = 1 + z * z / n
    centre = (p + z * z / (2 * n)) / denominator
    half = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / denominator
    return (max(0.0, centre - half), min(1.0, centre + half))


def sha256_of(path: Path) -> str:
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load_ratios(path: Path) -> tuple[dict, int, list[str]]:
    """Paired ratios from an A/A artifact, through the analyser's own rules.

    Loaded and checked with `bench_analyze`, not parsed here: an artifact this
    program is willing to read but the analyser would refuse is not a basis for
    a statement about the analyser. And the ratios come from `paired_ratios`, so
    a zero denominator or a non-finite value is handled the one way rather than
    two slightly different ways.
    """
    samples = ba.load(Path(path))
    ba.check_integrity(samples, Path(path), expect_mode="aa")
    names = ba.primary_family(samples)
    rounds = samples["harness"]["rounds"]
    out: dict[str, list] = {}
    for name in names:
        ratios = ba.paired_ratios(samples["benchmarks"][name], ba.PRIMARY_METRIC)
        out[name] = ratios if ratios is not None else [None] * rounds
    # A benchmark whose pairs are all unusable would reach `st.median` as an
    # empty list. Dropped here, where it can be said out loud, rather than as a
    # StatisticsError in the middle of a thousand trials.
    usable = {n: r for n, r in out.items() if any(v is not None for v in r)}
    dropped = sorted(set(out) - set(usable))
    if dropped:
        print(f"  note: no usable {ba.PRIMARY_METRIC} pairs for {dropped}", file=sys.stderr)
    return usable, rounds, sorted(usable)


def simulate(rng, pool, rounds, names, floor, effect, target, resamples):
    """One session. Returns (was the target declared, did any control fire)."""
    draw = [rng.randrange(rounds) for _ in range(rounds)]
    run = {}
    for name in names:
        values = [pool[name][i] for i in draw if pool[name][i] is not None]
        run[name] = [v * (1 + effect) for v in values] if name == target else values
    # Drawn from the caller's generator rather than fixed at 2: with a constant
    # every simulated session resampled in the same pattern, so the trials were
    # correlated and the spread across them understated. Still reproducible,
    # because the sequence comes from the seed the caller chose.
    half = ba.family_deviation_quantile(run, rounds, resamples, 0.95,
                                        rng.randrange(1 << 30), "observed")
    target_declared = False
    control_declared = False
    for name in names:
        verdict, _ = ba.classify_family(st.median(run[name]), half, floor)
        if name == target:
            target_declared = verdict == ba.REGRESSION
        elif verdict in (ba.REGRESSION, ba.IMPROVEMENT):
            control_declared = True
    return target_declared, control_declared


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pool", required=True,
                        help="A/A samples.json the sessions are drawn from")
    parser.add_argument("--control", required=True,
                        help="a second, independent A/A samples.json for the floor")
    parser.add_argument("--target", default="find miss",
                        help="the benchmark the planted effect is applied to")
    parser.add_argument("--null-trials", type=int, default=1000)
    parser.add_argument("--power-trials", type=int, default=300)
    parser.add_argument("--effects", default="0.05,0.06,0.07,0.08,0.10,0.273")
    parser.add_argument("--resamples", type=int, default=700)
    parser.add_argument("--seed", type=int, default=4242)
    parser.add_argument("--host", default=None)
    parser.add_argument("--output", default=None)
    args = parser.parse_args(argv)

    try:
        pool, rounds, names = load_ratios(Path(args.pool))
        control, control_rounds, _ = load_ratios(Path(args.control))
    except ba.IntegrityError as e:
        print(f"harness integrity: {e}", file=sys.stderr)
        return 3
    if args.target not in names:
        print(f"error: {args.target!r} is not in the family", file=sys.stderr)
        return 2

    floor = ba.family_deviation_quantile(control, control_rounds, args.resamples,
                                         0.95, 1, "one")
    started = time.monotonic()

    rng = random.Random(args.seed)
    failures = 0
    for _ in range(args.null_trials):
        _, control_fired = simulate(rng, pool, rounds, names, floor, 0.0,
                                    args.target, args.resamples)
        failures += control_fired
    fwer_lo, fwer_hi = wilson(failures, args.null_trials)

    power = []
    for effect in [float(e) for e in args.effects.split(",")]:
        rng = random.Random(args.seed + int(effect * 10000))
        hits = 0
        for _ in range(args.power_trials):
            declared, _ = simulate(rng, pool, rounds, names, floor, effect,
                                   args.target, args.resamples)
            hits += declared
        lo, hi = wilson(hits, args.power_trials)
        power.append({"effect": effect, "hits": hits, "trials": args.power_trials,
                      "rate": hits / args.power_trials, "wilson": [lo, hi]})

    report = {
        "schema": SCHEMA,
        "host": args.host,
        "inputs": {
            "pool": {"path": str(args.pool), "sha256": sha256_of(Path(args.pool))},
            "control": {"path": str(args.control), "sha256": sha256_of(Path(args.control))},
        },
        "parameters": {
            "seed": args.seed, "resamples": args.resamples, "rounds": rounds,
            "null_trials": args.null_trials, "power_trials": args.power_trials,
            "target": args.target, "family_size": len(names),
            "primary_metric": ba.PRIMARY_METRIC,
            "policy_version": ba.POLICY_VERSION,
        },
        "family": names,
        "family_floor": floor,
        "fwer": {"sessions_with_any_finding": failures, "trials": args.null_trials,
                 "wilson": [fwer_lo, fwer_hi],
                 "rule_of_three": (3.0 / args.null_trials) if failures == 0 else None},
        "power": power,
        "seconds": round(time.monotonic() - started, 1),
    }

    label = args.host or Path(args.pool).parent.name
    print(f"=== {label} — family of {len(names)}, A/A floor ±{floor*100:.2f}% ===")
    print(f"  seed {args.seed}   resamples {args.resamples}   "
          f"null trials {args.null_trials}   power trials {args.power_trials}")
    print(f"  FWER: {failures}/{args.null_trials} sessions with >=1 finding among "
          f"{len(names)-1} negative controls")
    print(f"    Wilson 95% [{fwer_lo*100:.2f}%, {fwer_hi*100:.2f}%]" +
          (f"   rule of three <= {3.0/args.null_trials*100:.2f}%" if failures == 0 else ""))
    print(f"  {'effect':>8} {'hits/N':>12} {'power':>9} {'Wilson 95%':>20}")
    for row in power:
        lo, hi = row["wilson"]
        # One decimal throughout: 1/300 is 0.3%, not 0%, and 299/300 is 99.7%,
        # not 100%. Rounding those to whole numbers erases the only difference
        # between "never seen" and "seen once".
        print(f"  {row['effect']*100:7.1f}% {f'{row['hits']}/{row['trials']}':>12} "
              f"{row['rate']*100:8.1f}%   [{lo*100:5.1f}%, {hi*100:5.1f}%]")

    if args.output:
        Path(args.output).write_text(json.dumps(report, indent=2))
        print(f"  wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
