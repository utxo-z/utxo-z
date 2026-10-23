#!/usr/bin/env python3
"""Recomputes an A/A family from raws already on disk. Measures nothing.

Two metrics reported together are a family of two, whether they were produced by
one command or by two. Running them separately does not remove the multiplicity:
with two independent 95% intervals the chance that at least one strays is about
one in ten, not one in twenty.

So the per-metric intervals are kept and shown, and a simultaneous band is added
beside them: Bonferroni at 1 - 0.05/2 = 97.5% per metric, which needs no
assumption about how the two metrics covary.

What that band is and is not: Bonferroni is a **nominal** adjustment. It widens
each interval so that, *if each interval covered at its nominal rate*, the
family statement would hold at 95%. A percentile bootstrap of a median on twenty
pairs may undercover, and then the family statement undercovers too. So the
output says "nominal", and does not claim that the family statement holds at 95%.

It fails closed. A report with failures, an incomplete pair set, a non-finite
ratio, a duplicated metric or an output path that already exists ends the run.
An earlier version accepted any document that happened to carry a `ratios` key
and wrote a band from it, including one whose own verdict said NO MEASUREMENT.
"""
import argparse, hashlib, importlib.util, json, math, sys
from pathlib import Path



LEGACY_SCHEMAS = {"utxoz-aa-pilot-1"}
CURRENT_SCHEMA = "utxoz-aa-pilot-2"


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def load_stats(path):
    p = Path(path).resolve()
    if not p.is_file():
        raise SystemExit(f"stats module {p} is not a file")
    spec = importlib.util.spec_from_file_location("aa_stats", p)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    if not hasattr(mod, "bootstrap_median_ci"):
        raise SystemExit(f"stats module {p} has no bootstrap_median_ci")
    return mod, sha256_file(p), str(p)


ESSENTIAL_CONFIG = (
    "schema", "kind", "metric", "pairs", "ops_per_sample", "warmup_per_sample",
    "population", "shard_count_expected", "order", "seed_rule", "cpu_affinity",
    "timeout_seconds_per_sample", "estimator", "harness", "harness_sha256",
    "harness_source_sha256", "runner_sha256", "host", "kernel", "started_utc",
    "config_sha256",
)


def sha256_text(t):
    return hashlib.sha256(t.encode()).hexdigest()


def load_parser(runner_path):
    """`parse_sample` from the runner, so that "a valid sample" has one meaning.

    Re-deciding it here would let a raw the runner would have rejected pass on
    the way back in.
    """
    p = Path(runner_path).resolve()
    if not p.is_file():
        raise SystemExit(f"runner {p} is not a file")
    spec = importlib.util.spec_from_file_location("aa_runner", p)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    if not hasattr(mod, "parse_sample"):
        raise SystemExit(f"runner {p} has no parse_sample")
    return mod.parse_sample, sha256_file(p), str(p), load_contract(mod)


# Optional, and offered by the runner rather than decided here.
#
# Without it this file rebuilds each raw's expectation from the fields every
# runner shares, which is all the stationary runner has. A runner whose sides
# differ - a control, where side B is owed extra work - needs more, and only
# the runner knows what: rebuilt here from the shared fields alone, a side B
# rewritten to 0% extra work re-parsed as valid. So a runner may export
#
#   expect_from_config(cfg, pair, side) -> dict   the full expectation per raw
#   verify_report(report, cfg) -> None | str      report-level facts, e.g. the
#                                                 image it was measured on
#   check_sample_record(d, rec, cfg) -> None | str  that the summary record of
#                                                 a sample says what its raw says,
#                                                 for fields report-level checks
#                                                 rely on
#
# and when it does they are used instead of, never alongside, the default. A
# runner that exports neither is verified exactly as before.
CONTRACT_HOOKS = ("expect_from_config", "verify_report", "check_sample_record")


def load_contract(mod):
    return {name: getattr(mod, name) for name in CONTRACT_HOOKS
            if callable(getattr(mod, name, None))}


def check_config(cfg):
    """A config that is complete and whose hash is the hash of itself."""
    for f in ESSENTIAL_CONFIG:
        if f not in cfg:
            return f"config is missing {f}"
    pairs = cfg["pairs"]
    if isinstance(pairs, bool) or not isinstance(pairs, int):
        return f"config pairs is {type(pairs).__name__}, not an integer"
    if not 1 <= pairs <= 20:
        return f"config pairs is {pairs}, outside 1..20"
    # The runner hashes the config before adding the hash to it.
    probe = {k: v for k, v in cfg.items() if k != "config_sha256"}
    recomputed = sha256_text(json.dumps(probe, sort_keys=True))
    if recomputed != cfg["config_sha256"]:
        return ("config_sha256 does not match the config it is in "
                f"({cfg['config_sha256'][:12]}… vs {recomputed[:12]}…): "
                "the config was edited after it was written")
    return None


def check_samples(r, cfg):
    """Exactly the sample records the config implies, each of them a success.

    Returns (ns_per_op_by_pair, why). A summary with no samples, or with a
    sample that failed, cannot support a confidence band: the band would be
    computed from numbers nothing produced.
    """
    pairs = cfg["pairs"]
    samples = r.get("samples")
    if not isinstance(samples, list):
        return None, "samples is not a list"
    if len(samples) != 2 * pairs:
        return None, f"samples has {len(samples)} records, not the {2 * pairs} for {pairs} pairs"

    by_pair = {}
    for i, rec in enumerate(samples):
        if not isinstance(rec, dict):
            return None, f"sample {i} is not an object"
        pr, side = rec.get("pair"), rec.get("side")
        if isinstance(pr, bool) or not isinstance(pr, int) or not 1 <= pr <= pairs:
            return None, f"sample {i} has pair {pr!r}"
        if side not in ("A", "B"):
            return None, f"sample {i} has side {side!r}"
        if (pr, side) in by_pair:
            return None, f"pair {pr} side {side} appears twice"
        if rec.get("exit") != 0:
            return None, f"pair {pr}{side} exited {rec.get('exit')!r}"
        if rec.get("timed_out") is not False:
            return None, f"pair {pr}{side} has timed_out {rec.get('timed_out')!r}"
        ns = rec.get("ns_per_op")
        if isinstance(ns, bool) or not isinstance(ns, (int, float)):
            return None, f"pair {pr}{side} ns_per_op is {type(ns).__name__}"
        if not math.isfinite(ns) or ns <= 0:
            return None, f"pair {pr}{side} ns_per_op is {ns!r}"
        by_pair[(pr, side)] = rec

    for pr in range(1, pairs + 1):
        for side in ("A", "B"):
            if (pr, side) not in by_pair:
                return None, f"pair {pr} side {side} is missing"
    return by_pair, None


def check_raws(by_pair, cfg, report_path, parse_sample, contract=None):
    """The raws behind the samples, hashed and re-parsed.

    Without these the document is a summary of itself. Their absence is not an
    error — an archived report may have lost them — but it changes what can be
    said, so it is reported rather than ignored.
    """
    raw_dir = Path(report_path).parent / "raw"
    if not raw_dir.is_dir():
        return "summary-only", f"no raw directory beside {report_path}"

    metric = cfg["metric"]
    for (pr, side), rec in sorted(by_pair.items()):
        f = raw_dir / f"{metric}-pair{pr:02d}-{side}.json"
        if not f.is_file():
            return "summary-only", f"{f.name} is not there"
        text = f.read_text()
        if sha256_text(text) != rec.get("raw_sha256"):
            return None, (f"{f.name} does not hash to what the report recorded: "
                          "the raw or the report was changed afterwards")
        if contract and "expect_from_config" in contract:
            try:
                expect = contract["expect_from_config"](cfg, pr, side)
            except (KeyError, TypeError, ValueError) as e:
                return None, (f"the config cannot say what {f.name} should hold "
                              f"({type(e).__name__}: {e})")
        else:
            expect = {"metric": metric, "ops": cfg["ops_per_sample"],
                      "warmup": cfg["warmup_per_sample"],
                      "population_observed": cfg["population"],
                      "shard_count": cfg["shard_count_expected"], "seed": pr}
        d, why = parse_sample(text, expect, f.name)
        if d is None:
            return None, f"{f.name} is not a usable sample: {why}"
        if abs(d["ns_per_op"] - rec["ns_per_op"]) > 1e-9:
            return None, (f"{f.name} says {d['ns_per_op']} and the report "
                          f"records {rec['ns_per_op']}")
        # verify_report reads the summary records; this is what ties them to the
        # raws. Without it a summary can claim two sides asked the same thing
        # while the raws say otherwise.
        if contract and "check_sample_record" in contract:
            try:
                why = contract["check_sample_record"](d, rec, cfg)
            except (KeyError, TypeError, ValueError) as e:
                why = f"the record check could not run ({type(e).__name__}: {e})"
            if why:
                return None, f"{f.name}: {why}"
    return "verified", None


def read_report(path, parse_sample, contract=None):
    """A report, or the reason it cannot contribute to a family.

    Fails closed. An earlier version accepted a document carrying only
    `config{schema,metric,pairs}` and a `ratios` list, which is to say it would
    certify a fabricated summary as a current-schema measurement.
    """
    p = Path(path)
    if not p.is_file():
        return None, f"{path} is not a file"
    try:
        r = json.loads(p.read_text(), parse_constant=lambda c: (_ for _ in ()).throw(
            ValueError(f"non-numeric constant {c!r}")))
    except (json.JSONDecodeError, ValueError) as e:
        return None, f"{path} is not usable JSON ({e})"
    if not isinstance(r, dict):
        return None, f"{path} is {type(r).__name__}, not an object"

    cfg = r.get("config")
    if not isinstance(cfg, dict):
        return None, f"{path} has no config object"
    schema = cfg.get("schema")
    if schema in LEGACY_SCHEMAS:
        # Kept readable, never promoted. A legacy report has no samples to check
        # and no config hash to verify, so its ratios are taken as given and the
        # whole family is labelled exploratory because of it.
        ratios = r.get("ratios")
        why = check_ratios(ratios, cfg, path)
        if why:
            return None, why
        return {"report": r, "config": cfg, "ratios": list(ratios),
                "verification": "legacy-exploratory",
                "verification_note": f"{schema}: no samples and no config hash to check",
                "sha256": sha256_file(p), "path": str(p)}, None
    if schema != CURRENT_SCHEMA:
        return None, f"{path} has unknown config schema {schema!r}"

    if r.get("verdict"):
        return None, f"{path} carries a verdict, so it is not a measurement: {r['verdict']}"
    failures = r.get("failures")
    if not isinstance(failures, list):
        return None, f"{path} has failures as {type(failures).__name__}, not a list"
    if failures:
        return None, f"{path} records {len(failures)} unusable samples"

    why = check_config(cfg)
    if why:
        return None, f"{path}: {why}"

    by_pair, why = check_samples(r, cfg)
    if why:
        return None, f"{path}: {why}"

    if contract and "verify_report" in contract:
        try:
            why = contract["verify_report"](r, cfg)
        except (KeyError, TypeError, ValueError) as e:
            why = f"the runner's report check could not run ({type(e).__name__}: {e})"
        if why:
            return None, f"{path}: {why}"

    # Recomputed from the samples, never taken from the document. A `ratios`
    # list is an output, and trusting an output is how a fabricated one gets in.
    ratios = [by_pair[(pr, "B")]["ns_per_op"] / by_pair[(pr, "A")]["ns_per_op"]
              for pr in range(1, cfg["pairs"] + 1)]
    received = r.get("ratios")
    if isinstance(received, list) and len(received) == len(ratios):
        for i, (a_, b_) in enumerate(zip(received, ratios)):
            if not isinstance(a_, (int, float)) or isinstance(a_, bool) \
               or not math.isfinite(a_) or abs(a_ - b_) > 1e-9:
                return None, (f"{path}: ratio {i} in the document is {a_!r} and the "
                              f"samples give {b_!r}")

    level, note = check_raws(by_pair, cfg, p, parse_sample, contract)
    if level is None:
        return None, f"{path}: {note}"

    return {"report": r, "config": cfg, "ratios": ratios,
            "verification": "verified-current" if level == "verified" else "summary-only",
            "verification_note": note or "raws hashed and re-parsed",
            "sha256": sha256_file(p), "path": str(p)}, None


def check_ratios(ratios, cfg, path):
    if not isinstance(ratios, list) or not ratios:
        return f"{path} has no ratios list"
    for i, v in enumerate(ratios):
        if isinstance(v, bool) or not isinstance(v, (int, float)):
            return f"{path} ratio {i} is {type(v).__name__}"
        if not math.isfinite(v) or v <= 0:
            return f"{path} ratio {i} is not a finite positive number: {v!r}"
    declared = cfg.get("pairs")
    if isinstance(declared, int) and not isinstance(declared, bool) \
       and declared != len(ratios):
        return f"{path} declares {declared} pairs and carries {len(ratios)} ratios"
    if not 1 <= len(ratios) <= 20:
        return f"{path} has {len(ratios)} ratios, outside 1..20"
    if not cfg.get("metric"):
        return f"{path} does not say which metric it measured"
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--stats-module", required=True)
    ap.add_argument("--runner", required=True,
                    help="run_aa_pilot.py, for its parse_sample; hashed into the record")
    ap.add_argument("--out", required=True, help="must not exist")
    ap.add_argument("--expect-family", required=True,
                    help="comma-separated metric names this family consists of")
    ap.add_argument("--alpha", type=float, default=0.05)
    ap.add_argument("--resamples", type=int, default=3000)
    ap.add_argument("--seed", type=int, default=5)
    ap.add_argument("reports", nargs="+")
    a = ap.parse_args()

    if Path(a.out).exists():
        print(f"  {a.out} already exists; refusing to overwrite it")
        return 2
    if not 0.0 < a.alpha < 0.5:
        print("  --alpha must be in (0, 0.5)"); return 2
    if a.resamples < 100:
        print("  --resamples must be at least 100"); return 2

    expected = [m.strip() for m in a.expect_family.split(",") if m.strip()]
    if len(expected) != len(set(expected)):
        print("  --expect-family lists a metric twice"); return 2

    stats, stats_hash, stats_path = load_stats(a.stats_module)
    parse_sample, runner_hash, runner_path, contract = load_parser(a.runner)

    loaded, problems = [], []
    for path in a.reports:
        d, why = read_report(path, parse_sample, contract)
        (problems if d is None else loaded).append(why if d is None else d)
    if problems:
        for w in problems:
            print(f"  REJECTED {w}")
        print("  no family was computed")
        return 3

    # A family member is named by its cell when the report has one, and by its
    # metric otherwise. A grid compares several shard counts on the same two
    # metrics, so there the metric alone does not tell its members apart; every
    # earlier report has no `cell` and is named exactly as before.
    metrics = [d["config"].get("cell") or d["config"]["metric"] for d in loaded]
    if len(metrics) != len(set(metrics)):
        print(f"  the same member appears twice: {metrics}"); return 3
    if sorted(metrics) != sorted(expected):
        print(f"  the family was declared as {expected} and the reports are {metrics}")
        return 3

    k = len(loaded)
    per_metric_conf = 1.0 - a.alpha
    simultaneous_conf = 1.0 - a.alpha / k

    family = {
        "family_size": k,
        "declared_family": expected,
        "alpha": a.alpha,
        "per_metric_confidence": per_metric_conf,
        "nominal_simultaneous_confidence_per_metric": simultaneous_conf,
        "correction": (f"Bonferroni, nominal: {per_metric_conf:.3f} per metric becomes "
                       f"{simultaneous_conf:.4f} per metric. This is a nominal "
                       f"adjustment — if each interval covers at its nominal rate, "
                       f"the family statement holds at {1.0 - a.alpha:.2f}. A "
                       f"percentile bootstrap of a median on this few pairs may "
                       f"undercover, and then so does the family statement."),
        "estimator": {"module": stats_path, "module_sha256": stats_hash,
                      "function": "bootstrap_median_ci",
                      "resamples": a.resamples, "seed": a.seed},
        "sample_parser": {"module": runner_path, "module_sha256": runner_hash,
                          "function": "parse_sample",
                          "runner_contract": sorted(contract)},
        "inputs": [{"path": d["path"], "sha256": d["sha256"],
                    "config_schema": d["config"].get("schema"),
                    "verification": d["verification"],
                    "verification_note": d["verification_note"]} for d in loaded],
        "metrics": [],
    }
    levels = {d["verification"] for d in loaded}
    all_verified = levels == {"verified-current"}
    if "legacy-exploratory" in levels:
        family["status"] = ("EXPLORATORY — at least one input uses a legacy config "
                            "schema, with no samples and no config hash to check; "
                            "these numbers do not become stronger by being recomputed")
    elif "summary-only" in levels:
        family["status"] = ("SUMMARY-ONLY, NOT VALIDATED — at least one input's raws "
                            "could not be found, so its samples were not re-parsed")
    else:
        family["status"] = ("recomputed from current-schema reports whose configs hash "
                            "to themselves and whose raws were re-parsed")

    for d in loaded:
        ratios, cfg = d["ratios"], d["config"]
        med, lo, hi = stats.bootstrap_median_ci(ratios, a.resamples, per_metric_conf, a.seed)
        _, slo, shi = stats.bootstrap_median_ci(ratios, a.resamples, simultaneous_conf, a.seed)
        family["metrics"].append({
            "source": d["path"], "metric": cfg.get("metric"),
            "member": cfg.get("cell") or cfg.get("metric"), "pairs": len(ratios),
            "ops_per_sample": cfg.get("ops_per_sample"),
            "population": cfg.get("population"),
            "median_pct": (med - 1.0) * 100.0,
            "per_metric_ci_pct": {"low": (lo - 1.0) * 100.0, "high": (hi - 1.0) * 100.0},
            "nominal_simultaneous_ci_pct": {"low": (slo - 1.0) * 100.0,
                                            "high": (shi - 1.0) * 100.0},
            "nominal_simultaneous_upper_pct": (shi - 1.0) * 100.0,
        })

    worst = max(m["nominal_simultaneous_upper_pct"] for m in family["metrics"])
    family["worst_nominal_simultaneous_upper_pct"] = worst
    # Only a fully verified family gets a verdict about the margin. Anything else
    # has an interval and no standing to draw a conclusion from it.
    if all_verified:
        family["widths_compatible_with_2pct_margin"] = bool(worst <= 2.0)
    else:
        family["widths_compatible_with_2pct_margin"] = None
        family["margin_not_assessed_because"] = family["status"]
    family["language"] = (
        "Observed precision of these runs, on these metrics, at this operating "
        "point. Compatibility with a 2% margin is a statement about the width of "
        "these intervals under a nominal adjustment. It is not a floor for future "
        "A/B comparisons, not a guarantee about false positives, and — with no "
        "positive control run — not a statement about power.")
    Path(a.out).write_text(json.dumps(family, indent=1))

    print(f"  family of {k} ({', '.join(metrics)}), Bonferroni nominal "
          f"{simultaneous_conf:.4f} per metric")
    print(f"  status: {family['status'][:70]}")
    for m in family["metrics"]:
        print(f"    {m['member']:<14} n={m['pairs']:<3} median {m['median_pct']:+.3f}%  "
              f"per-metric [{m['per_metric_ci_pct']['low']:+.3f}, "
              f"{m['per_metric_ci_pct']['high']:+.3f}]  "
              f"nominal simultaneous [{m['nominal_simultaneous_ci_pct']['low']:+.3f}, "
              f"{m['nominal_simultaneous_ci_pct']['high']:+.3f}]")
    verdict = family["widths_compatible_with_2pct_margin"]
    print(f"  worst nominal simultaneous upper bound {worst:+.3f}%  "
          f"widths compatible with a 2% margin: "
          f"{verdict if verdict is not None else 'NOT ASSESSED'}")
    print(f"  written: {a.out}")
    return 0


sys.exit(main())
