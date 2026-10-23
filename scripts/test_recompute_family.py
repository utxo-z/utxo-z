#!/usr/bin/env python3
"""Negative coverage for the family recomputation.

The cases that matter most are not malformed JSON — they are documents that look
like measurements. A summary carrying only a schema, a metric and a ratios list
once passed as a current-schema result; so did one whose config had been edited
after it was hashed. Those are the cases below.
"""
import copy, hashlib, json, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
TOOL = HERE / "recompute_aa_family.py"
RUNNER = HERE / "run_aa_pilot.py"
STATS = HERE / "bench_analyze.py"   # the repository's own estimator


def sha(t):
    return hashlib.sha256(t.encode()).hexdigest()


def raw_doc(metric, pair, ops=10, warmup=2, pop=5, elapsed=10000):
    return {"schema": "utxoz-calib-1", "metric": metric, "shard_count": 1,
            "ops": ops, "warmup": warmup, "population_requested": pop,
            "population_observed": pop, "version_files_all_classes": 5,
            "version_files_class_48": 1, "value_size": 33, "seed": pair,
            "cache": "warm", "elapsed_ns": elapsed,
            "ns_per_op": round(elapsed / ops, 4),
            "hits": ops if metric == "find_hit" else 0,
            "not_resolved": 0 if metric == "find_hit" else ops,
            "not_found": 0, "real_errors": 0, "first_error_code": 0,
            "warmup_hits": warmup if metric == "find_hit" else 0,
            "warmup_not_resolved": 0 if metric == "find_hit" else warmup,
            "warmup_not_found": 0,
            "verified_payload": ops if metric == "find_hit" else 0,
            "verified_height": ops if metric == "find_hit" else 0,
            "valid": True, "invalid_because": ""}


def build_run(root, metric, pairs=3, elapsed=None):
    """A complete run directory: config, raws, and a report that agrees."""
    out = Path(root) / f"run-{metric}"
    raws = out / "raw"
    raws.mkdir(parents=True)
    cfg = {"schema": "utxoz-aa-pilot-2", "kind": "A/A", "metric": metric,
           "pairs": pairs, "ops_per_sample": 10, "warmup_per_sample": 2,
           "population": 5, "shard_count_expected": 1, "cache": "warm",
           "order": "AB/BA", "seed_rule": "pair index", "cpu_affinity": 2,
           "timeout_seconds_per_sample": 60.0,
           "estimator": {"function": "bootstrap_median_ci"},
           "harness": "/fake/harness", "harness_sha256": "0" * 64,
           "harness_source_sha256": "1" * 64, "runner_sha256": "2" * 64,
           "host": "test", "kernel": "test", "started_utc": "2026-01-01T00:00:00Z"}
    cfg["config_sha256"] = sha(json.dumps(cfg, sort_keys=True))

    samples, sides = [], {"A": [], "B": []}
    for pr in range(1, pairs + 1):
        for side in ("A", "B"):
            e = (elapsed or 10000) + (7 if side == "B" else 0)
            doc = raw_doc(metric, pr, elapsed=e)
            text = json.dumps(doc)
            (raws / f"{metric}-pair{pr:02d}-{side}.json").write_text(text)
            samples.append({"pair": pr, "side": side, "exit": 0, "timed_out": False,
                            "command": "fake", "raw_sha256": sha(text), "stderr": "",
                            "seconds": 0.1, "ns_per_op": doc["ns_per_op"],
                            "unusable_because": None})
            sides[side].append(doc["ns_per_op"])
    ratios = [b / a for a, b in zip(sides["A"], sides["B"])]
    report = {"config": cfg, "samples": samples, "failures": [],
              "ns_per_op": sides, "ratios": ratios}
    (out / "report.json").write_text(json.dumps(report, indent=1))
    return out / "report.json"


def mutate(path, dest, fn):
    r = json.loads(Path(path).read_text())
    fn(r)
    d = Path(dest)
    d.parent.mkdir(parents=True, exist_ok=True)
    # The raws travel with it, so only what fn changed differs.
    src_raw = Path(path).parent / "raw"
    if src_raw.is_dir():
        tgt = d.parent / "raw"
        tgt.mkdir(exist_ok=True)
        for f in src_raw.iterdir():
            (tgt / f.name).write_text(f.read_text())
    d.write_text(json.dumps(r, indent=1))
    return d


def run(out, paths, family="find_hit,find_miss"):
    cmd = [sys.executable, "-I", str(TOOL), "--stats-module", str(STATS),
           "--runner", str(RUNNER), "--out", str(out),
           "--expect-family", family, *map(str, paths)]
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
    except subprocess.TimeoutExpired as e:
        return -1, f"the tool did not finish within 120s: {e}"
    return p.returncode, p.stdout + p.stderr


def main():
    bad = 0
    with tempfile.TemporaryDirectory(prefix="recompute-tests-") as d:
        hit = build_run(d, "find_hit")
        miss = build_run(d, "find_miss")

        def case(name, paths, want, family="find_hit,find_miss"):
            nonlocal bad
            code, log = run(Path(d) / f"out-{abs(hash(name))}.json", paths, family)
            ok = code == want
            bad += 0 if ok else 1
            print(f"  {name:<44} exit={code} want={want} {'ok' if ok else 'FAILED'}")
            if not ok:
                print("    " + log.strip().replace("\n", "\n    ")[:400])
            return log

        out = Path(d) / "verified.json"
        code, _ = run(out, [hit, miss])
        body = json.loads(out.read_text()) if code == 0 else {}
        ok = (code == 0
              and body.get("widths_compatible_with_2pct_margin") is True
              and "re-parsed" in body.get("status", "")
              and all(i["verification"] == "verified-current" for i in body["inputs"]))
        bad += 0 if ok else 1
        print(f"  {'a complete, verified family':<44} exit={code} "
              f"margin={body.get('widths_compatible_with_2pct_margin')} "
              f"{'ok' if ok else 'FAILED'}")

        # The exact hole round 3 found: a summary with no samples.
        case("current-minimal: schema+metric+pairs+ratios",
             [mutate(hit, Path(d) / "min/report.json",
                     lambda r: r.update({"samples": None}) or
                               [r.pop(k) for k in ("ns_per_op",) if k in r]), miss], 3)
        case("samples missing entirely",
             [mutate(hit, Path(d) / "nosamp/report.json",
                     lambda r: r.pop("samples")), miss], 3)
        case("one sample short",
             [mutate(hit, Path(d) / "short/report.json",
                     lambda r: r["samples"].pop()), miss], 3)
        case("a sample that exited nonzero",
             [mutate(hit, Path(d) / "exit/report.json",
                     lambda r: r["samples"][0].update({"exit": 3})), miss], 3)
        case("a sample that timed out",
             [mutate(hit, Path(d) / "to/report.json",
                     lambda r: r["samples"][0].update({"timed_out": True})), miss], 3)
        case("a duplicated pair/side",
             [mutate(hit, Path(d) / "dup/report.json",
                     lambda r: r["samples"].__setitem__(
                         1, copy.deepcopy(r["samples"][0]))), miss], 3)
        case("ratios that disagree with the samples",
             [mutate(hit, Path(d) / "rat/report.json",
                     lambda r: r.update({"ratios": [9.9] * len(r["ratios"])})), miss], 3)
        case("a config edited after it was hashed",
             [mutate(hit, Path(d) / "tamper/report.json",
                     lambda r: r["config"].update({"population": 999999})), miss], 3)
        case("pairs as a string",
             [mutate(hit, Path(d) / "pstr/report.json",
                     lambda r: r["config"].update({"pairs": "3"})), miss], 3)
        case("pairs missing from the config",
             [mutate(hit, Path(d) / "pmiss/report.json",
                     lambda r: r["config"].pop("pairs")), miss], 3)
        case("failures as a non-empty list",
             [mutate(hit, Path(d) / "fail/report.json",
                     lambda r: r.update({"failures": [{"pair": 1}]})), miss], 3)
        case("failures as a non-list",
             [mutate(hit, Path(d) / "fdict/report.json",
                     lambda r: r.update({"failures": {}})), miss], 3)
        case("a verdict of no measurement",
             [mutate(hit, Path(d) / "verd/report.json",
                     lambda r: r.update({"verdict": "NO MEASUREMENT"})), miss], 3)
        case("the same metric twice", [hit, hit], 2, "find_hit,find_hit")
        case("a metric the family did not declare", [hit, miss], 3, "find_hit,resolve")

        # A raw whose bytes no longer match the hash the report recorded.
        tampered = mutate(hit, Path(d) / "rawtamper/report.json", lambda r: None)
        (Path(d) / "rawtamper/raw/find_hit-pair01-A.json").write_text(
            json.dumps(raw_doc("find_hit", 1, elapsed=99999)))
        case("a raw that no longer hashes to the report", [tampered, miss], 3)

        # Raws absent: readable, but not verified, and no margin verdict.
        noraw = mutate(hit, Path(d) / "noraw/report.json", lambda r: None)
        for f in (Path(d) / "noraw/raw").iterdir():
            f.unlink()
        (Path(d) / "noraw/raw").rmdir()
        out = Path(d) / "summary-only.json"
        code, log = run(out, [noraw, miss])
        body = json.loads(out.read_text()) if code == 0 else {}
        ok = code == 0 and body.get("widths_compatible_with_2pct_margin") is None \
            and "SUMMARY-ONLY" in body.get("status", "")
        bad += 0 if ok else 1
        print(f"  {'raws absent: readable, margin NOT assessed':<44} exit={code} "
              f"{'ok' if ok else 'FAILED'}")

        # Legacy stays exploratory and also gets no margin verdict.
        leg = Path(d) / "legacy.json"
        leg.write_text(json.dumps({
            "config": {"schema": "utxoz-aa-pilot-1", "metric": "find_hit", "pairs": 3,
                       "ops_per_sample": 10, "population": 5},
            "failures": [], "ratios": [1.0, 1.01, 0.99]}))
        out = Path(d) / "legacy-out.json"
        code, _ = run(out, [leg, miss])
        body = json.loads(out.read_text()) if code == 0 else {}
        ok = code == 0 and "EXPLORATORY" in body.get("status", "") \
            and body.get("widths_compatible_with_2pct_margin") is None
        bad += 0 if ok else 1
        print(f"  {'legacy stays exploratory, no margin verdict':<44} exit={code} "
              f"{'ok' if ok else 'FAILED'}")

        # Still refuses to overwrite.
        once = Path(d) / "once.json"
        c1, _ = run(once, [hit, miss])
        c2, _ = run(once, [hit, miss])
        ok = c1 == 0 and c2 == 2
        bad += 0 if ok else 1
        print(f"  {'refuses to overwrite':<44} first={c1} second={c2} "
              f"{'ok' if ok else 'FAILED'}")

    print(f"  {'FAILURES: %d' % bad if bad else 'all recompute tests passed'}")
    return 1 if bad else 0


sys.exit(main())
