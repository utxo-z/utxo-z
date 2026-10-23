#!/usr/bin/env python3
"""The A/A pilot: how much spread this host shows when nothing changed.

Both sides are the *same* binary. Whatever interval comes out describes the
instrument on this workload at this operating point. It measures nothing about
sharding and cannot: there is no candidate.

Design rules, each of them a reaction to a way this went wrong before:

* **Fail closed.** Every field of every sample is checked against what was
  asked for — metric, ops, warmup, population, seed, shard count — and against
  itself: finite, the right type, a real bool. `json.loads` accepts `Infinity`
  and `NaN` happily, and a ratio built from one of those is not a number
  anybody should see. Anything unexpected ends the sample, and an ended sample
  ends the run rather than quietly shrinking `n`.
* **The config is written before the first measurement, not after.** A config
  persisted at the end describes what happened to survive.
* **Every sample keeps its exit code, its full stderr, its command and the hash
  of its raw output.** A summary that drops those cannot be audited.
* **The statistics module is named, validated and hashed.** Importing whatever
  sits at a hard-coded path makes the estimator invisible to the record.
* **Nothing existing is overwritten.** The output directory must be new.
* **Each sample has a timeout**, and a timeout is evidence, not a blank.
* **`n` is fixed.** Growing it until an interval looks good is how a floor stops
  being one.
"""
import argparse, hashlib, importlib.util, json, math, os, platform, shlex
import subprocess, sys, threading, time
from pathlib import Path

HEARTBEAT_SECONDS = 15
SCHEMA = "utxoz-calib-1"


def sha256_file(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


# --- what ran: the harness binary and its source, before and after a run ---
# (The same three helpers as run_hist_pilot.py's; copied, so that this runner's
# own hash still covers everything it runs.)
#
# A run hashes these before its first sample and again after its last. A file
# that changed, vanished or could not be read in between means some samples may
# come from another binary or source: the report is no measurement. The record
# is checked from the hashes it holds, never from a summary flag, and a report
# without it (one written before it existed) cannot be certified.

def hash_or_none(path):
    """sha256 of path, or None if it is gone or cannot be read."""
    try:
        return sha256_file(path)
    except OSError:
        return None


def after_hashes(before):
    """{label: (path, sha256 before the run)} -> the record, hashing each again now."""
    return {label: {"path": str(path), "sha256_before": sha, "sha256_after": hash_or_none(path)}
            for label, (path, sha) in before.items()}


def provenance_problem(record, expected=None):
    """None if every file in record hashed the same after the run as before it,
    else why. With expected = {label: sha256 the config names}, the record must
    name exactly those files with exactly those before-hashes."""
    if not isinstance(record, dict) or not record:
        return ("the report has no after-run hashes of the harness binaries and source; "
                "a report from before that record existed cannot be certified")
    if expected is not None:
        if set(record) != set(expected):
            return f"the after-run record names {sorted(record)}, not {sorted(expected)}"
        for label, sha in expected.items():
            if not isinstance(record[label], dict) or record[label].get("sha256_before") != sha:
                return f"the recorded hash of {label} before the run is not the one the config names"
    for label, e in record.items():
        if not isinstance(e, dict) or not e.get("sha256_before"):
            return f"{label} has no hash from before the run"
        if e.get("sha256_after") is None:
            return f"{label} was gone or unreadable after the run"
        if e["sha256_after"] != e["sha256_before"]:
            return f"{label} changed during the run"
    return None


def sha256_text(t):
    return hashlib.sha256(t.encode()).hexdigest()


def load_stats_module(path):
    """The estimator, named explicitly so the record can say which one ran."""
    p = Path(path).resolve()
    if not p.is_file():
        raise SystemExit(f"--stats-module {p} is not a file")
    spec = importlib.util.spec_from_file_location("aa_stats", p)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    for fn in ("bootstrap_median_ci",):
        if not hasattr(mod, fn):
            raise SystemExit(f"--stats-module {p} has no {fn}")
    return mod, sha256_file(p), str(p)


def _finite_number(v):
    return isinstance(v, (int, float)) and not isinstance(v, bool) and math.isfinite(v)


def parse_sample(text, expect, path_hint=""):
    """One harness document, or the reason it is not a measurement.

    `expect` is what the runner asked for. A document that measured something
    else is not a sample of this experiment, however well-formed it is.
    """
    if not text.strip():
        return None, f"no output from {path_hint or 'the harness'}"
    try:
        d = json.loads(text, parse_constant=lambda c: (_ for _ in ()).throw(
            ValueError(f"non-numeric constant {c!r} in JSON")))
    except (json.JSONDecodeError, ValueError) as e:
        return None, f"output is not usable JSON ({e})"
    if not isinstance(d, dict):
        return None, f"output is {type(d).__name__}, not an object"
    if d.get("schema") != SCHEMA:
        return None, f"unexpected schema {d.get('schema')!r}"

    counters = ("ops", "warmup", "hits", "not_resolved", "not_found",
                "real_errors", "population_observed", "seed", "shard_count",
                "verified_payload", "verified_height", "warmup_hits",
                "warmup_not_resolved", "warmup_not_found", "elapsed_ns")
    for f in counters:
        if f not in d:
            return None, f"missing field {f}"
        # Counters are counts: an integer, not negative, not a float that happens
        # to look like one. A negative count passed every earlier check.
        if isinstance(d[f], bool) or not isinstance(d[f], int):
            return None, f"field {f} is {type(d[f]).__name__}, not an integer: {d[f]!r}"
        if d[f] < 0:
            return None, f"field {f} is negative: {d[f]}"
    if "ns_per_op" not in d:
        return None, "missing field ns_per_op"
    if not _finite_number(d["ns_per_op"]):
        return None, f"field ns_per_op is not a finite number: {d['ns_per_op']!r}"
    if not isinstance(d.get("valid"), bool):
        return None, f"field valid is {type(d.get('valid')).__name__}, not a bool"
    if not d["valid"]:
        return None, f"the harness reported the operation invalid: {d.get('invalid_because')}"
    if d["ns_per_op"] <= 0 or d["elapsed_ns"] <= 0:
        return None, f"ns_per_op {d['ns_per_op']} / elapsed_ns {d['elapsed_ns']} not positive"
    if d["real_errors"] != 0:
        return None, f"{d['real_errors']} lookups failed with a real error"

    # The four outcomes partition the lookups. If they do not add up, the
    # document describes something other than the run it claims to.
    outcomes = d["hits"] + d["not_resolved"] + d["not_found"] + d["real_errors"]
    if outcomes != d["ops"]:
        return None, (f"outcomes do not partition the lookups: "
                      f"{d['hits']}+{d['not_resolved']}+{d['not_found']}+"
                      f"{d['real_errors']} = {outcomes}, not {d['ops']}")
    warm = d["warmup_hits"] + d["warmup_not_resolved"] + d["warmup_not_found"]
    if warm != d["warmup"]:
        return None, (f"warmup outcomes come to {warm}, not the {d['warmup']} "
                      f"lookups it says it made")
    for f in ("verified_payload", "verified_height"):
        if d[f] > d["ops"]:
            return None, f"{f} is {d[f]}, more than the {d['ops']} lookups"

    # ns_per_op has to be elapsed/ops. A figure that disagrees with its own
    # inputs is not a measurement of them.
    #
    # The harness prints four decimals, so the only discrepancy to forgive is
    # that rounding: half of 1e-4, plus room for the float arithmetic on both
    # sides. A *relative* tolerance was wrong — at 1000 ns/op it let a figure be
    # off by 0.5, which is four thousand times the rounding it was meant to
    # cover.
    if d["ops"] <= 0:
        return None, f"ops is {d['ops']}, so there is nothing to divide by"
    derived = d["elapsed_ns"] / d["ops"]
    tolerance = 5e-5 + 8.0 * sys.float_info.epsilon * max(1.0, abs(derived))
    if abs(derived - d["ns_per_op"]) > tolerance:
        return None, (f"ns_per_op {d['ns_per_op']} does not match "
                      f"elapsed_ns/ops = {derived:.6f} (tolerance {tolerance:.2e})")

    # What was measured has to be what was asked for.
    for field, want in expect.items():
        got = d.get(field)
        if got != want:
            return None, f"{field} is {got!r}, not the {want!r} requested"

    # And the outcomes have to agree with the metric.
    if d["metric"] == "find_hit":
        if d["hits"] != d["ops"]:
            return None, f"find_hit resolved {d['hits']} of {d['ops']}"
        if d["verified_payload"] != d["ops"] or d["verified_height"] != d["ops"]:
            return None, ("the harness did not verify every hit's payload and height: "
                          f"{d['verified_payload']}/{d['verified_height']} of {d['ops']}")
    else:
        if d["hits"] != 0:
            return None, f"find_miss resolved {d['hits']} lookups"
        if d["not_resolved"] + d["not_found"] != d["ops"]:
            return None, ("unanswerable lookups do not add up: "
                          f"{d['not_resolved']} + {d['not_found']} != {d['ops']}")
    return d, None


def verify_report(report, cfg):
    """For recompute_aa_family: the binary and source that ran are the ones the
    config names, unchanged from the first sample to the last - from the
    recorded hashes. None, or the reason."""
    return provenance_problem(report.get("provenance_after"),
                              {"harness": cfg.get("harness_sha256"),
                               "harness_source": cfg.get("harness_source_sha256")})


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--harness", required=True)
    ap.add_argument("--metric", required=True, choices=["find_hit", "find_miss"])
    ap.add_argument("--stats-module", required=True,
                    help="the file providing bootstrap_median_ci; hashed into the record")
    ap.add_argument("--pairs", type=int, default=20)
    ap.add_argument("--ops", type=int, default=10_000_000)
    ap.add_argument("--warmup", type=int, default=500_000)
    ap.add_argument("--population", type=int, default=90_000)
    ap.add_argument("--cpu", type=int, default=2)
    ap.add_argument("--resamples", type=int, default=3000)
    ap.add_argument("--confidence", type=float, default=0.95)
    ap.add_argument("--bootstrap-seed", type=int, default=5)
    ap.add_argument("--timeout", type=float, default=300.0)
    ap.add_argument("--shard-count", type=int, default=1)
    ap.add_argument("--out-dir", required=True,
                    help="must not exist; this run creates it and writes only there")
    a = ap.parse_args()

    # Ranges, before anything runs.
    problems = []
    if not 1 <= a.pairs <= 20:
        problems.append("--pairs must be between 1 and 20; the cap is deliberate")
    if a.ops < 1: problems.append("--ops must be positive")
    if a.warmup < 0: problems.append("--warmup cannot be negative")
    if a.population < 1: problems.append("--population must be positive")
    if a.resamples < 100: problems.append("--resamples must be at least 100")
    if not 0.5 < a.confidence < 1.0: problems.append("--confidence must be in (0.5, 1)")
    if not math.isfinite(a.timeout) or a.timeout <= 0:
        problems.append("--timeout must be a finite positive number")
    if problems:
        for p in problems: print(f"  {p}")
        sys.exit(2)

    out = Path(a.out_dir)
    if out.exists():
        print(f"  {out} already exists; this run will not overwrite or extend it")
        sys.exit(2)
    # Every input checked before the output directory exists: a refusal must not
    # leave one behind for the retry to trip over.
    stats, stats_hash, stats_path = load_stats_module(a.stats_module)
    harness = Path(a.harness).resolve()
    if not os.access(harness, os.X_OK):
        print(f"  {harness} is not executable"); sys.exit(2)
    # Found from this script, not from the caller's working directory, and
    # hashed now: a runner copied away from benchmarks/ stops here.
    harness_source = Path(__file__).resolve().parent.parent / "benchmarks" / "calib_stationary.cpp"
    if not harness_source.is_file():
        print(f"  harness source {harness_source} is not a file"); sys.exit(2)
    harness_hash, harness_source_hash = sha256_file(harness), sha256_file(harness_source)
    out.mkdir(parents=True)
    raws = out / "raw"; raws.mkdir()

    config = {
        "schema": "utxoz-aa-pilot-2",
        "kind": "A/A — both sides are the same binary",
        "metric": a.metric,
        "pairs": a.pairs,
        "ops_per_sample": a.ops,
        "warmup_per_sample": a.warmup,
        "population": a.population,
        "shard_count_expected": a.shard_count,
        "cache": "warm; the harness warms the same distribution it then measures",
        "order": "AB on odd pairs, BA on even pairs, fixed",
        "seed_rule": "both sides of pair r use seed r",
        "cpu_affinity": a.cpu,
        "timeout_seconds_per_sample": a.timeout,
        "estimator": {
            "module": stats_path,
            "module_sha256": stats_hash,
            "function": "bootstrap_median_ci",
            "description": "percentile bootstrap of the median of paired ratios",
            "resamples": a.resamples,
            "confidence": a.confidence,
            "seed": a.bootstrap_seed,
        },
        "harness": str(harness),
        "harness_sha256": harness_hash,
        "harness_source_sha256": harness_source_hash,
        "runner_sha256": sha256_file(__file__),
        "host": platform.node(),
        "kernel": platform.release(),
        "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    }
    config["config_sha256"] = sha256_text(json.dumps(config, sort_keys=True))
    # Written before the first measurement, so the record cannot be shaped by it.
    (out / "config.json").write_text(json.dumps(config, indent=1))
    print(f"  config_sha256={config['config_sha256']}")
    print(f"  config written to {out/'config.json'} before measuring")
    sys.stdout.flush()

    # The seed is per pair, so the expectation has to be rebuilt per pair. An
    # earlier version listed the seed as checked and never put it in `expect`,
    # which meant a harness reporting seed 1 for every pair passed.
    def expect_for(pair):
        return {"metric": a.metric, "ops": a.ops, "warmup": a.warmup,
                "population_observed": a.population,
                "shard_count": a.shard_count, "seed": pair}

    sides, samples, failures = {"A": [], "B": []}, [], []
    for r in range(1, a.pairs + 1):
        if failures:
            break               # an unusable sample ends the run
        for side in (("A", "B") if r % 2 == 1 else ("B", "A")):
            cmd = ["taskset", "-c", str(a.cpu), str(harness),
                   "--metric", a.metric, "--ops", str(a.ops),
                   "--warmup", str(a.warmup), "--population", str(a.population),
                   "--seed", str(r), "--work", str(raws)]
            t0 = time.time()
            print(f"  [pair {r}/{a.pairs} {side}] START {time.strftime('%H:%M:%S')}", flush=True)
            stop = threading.Event()
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE, text=True)

            def beat(pid=proc.pid, t=t0):
                while not stop.wait(HEARTBEAT_SECONDS):
                    print(f"      alive pid={pid} {time.time()-t:.0f}s "
                          f"(liveness only, not progress)", flush=True)

            th = threading.Thread(target=beat, daemon=True); th.start()
            timed_out = False
            try:
                so, se = proc.communicate(timeout=a.timeout)
            except subprocess.TimeoutExpired:
                proc.kill()
                so, se = proc.communicate()
                timed_out = True
            stop.set()
            code = proc.returncode
            stem = f"{a.metric}-pair{r:02d}-{side}"
            (raws / f"{stem}.json").write_text(so)
            (raws / f"{stem}.stderr").write_text(se)
            (raws / f"{stem}.cmd").write_text(shlex.join(cmd) + "\n")

            d, why = (None, f"timed out after {a.timeout}s") if timed_out \
                     else parse_sample(so, expect_for(r), stem)
            if d is not None and code != 0:
                d, why = None, f"exit {code}"
            rec = {"pair": r, "side": side, "exit": code, "timed_out": timed_out,
                   "command": shlex.join(cmd),
                   "raw_sha256": sha256_text(so), "stderr": se,
                   "seconds": time.time() - t0,
                   "ns_per_op": (d["ns_per_op"] if d else None),
                   "unusable_because": why}
            samples.append(rec)
            print(f"  [pair {r}/{a.pairs} {side}] END   {time.strftime('%H:%M:%S')} "
                  f"{rec['seconds']:.1f}s exit={code} "
                  f"{'ns/op=%.3f' % d['ns_per_op'] if d else 'UNUSABLE: ' + why}", flush=True)
            if d is None:
                failures.append(rec)
                break
            else:
                sides[side].append(d["ns_per_op"])

    provenance = after_hashes({"harness": (harness, harness_hash),
                               "harness_source": (harness_source, harness_source_hash)})
    report = {"config": config, "samples": samples, "failures": failures,
              "ns_per_op": sides, "provenance_after": provenance}

    if provenance_problem(provenance):
        report["verdict"] = f"NO MEASUREMENT — {provenance_problem(provenance)}"
    elif failures or len(sides["A"]) != a.pairs or len(sides["B"]) != a.pairs:
        report["verdict"] = "NO MEASUREMENT — at least one sample was not usable"
    else:
        ratios = [b / x for x, b in zip(sides["A"], sides["B"])]
        med, lo, hi = stats.bootstrap_median_ci(ratios, a.resamples, a.confidence,
                                                a.bootstrap_seed)
        report.update({
            "ratios": ratios,
            "median_ratio": med,
            "ci": {"low": lo, "high": hi},
            "median_pct": (med - 1.0) * 100.0,
            "ci_pct": {"low": (lo - 1.0) * 100.0, "high": (hi - 1.0) * 100.0},
            "half_width_pct": (hi - lo) * 50.0,
            "upper_bound_pct": (hi - 1.0) * 100.0,
            "language": ("observed precision of this pilot, on this metric, at this "
                         "operating point. Not a floor for future A/B comparisons, "
                         "not a statement about false positives, and without a "
                         "positive control not a statement about power."),
        })

    (out / "report.json").write_text(json.dumps(report, indent=1))
    print(f"  written: {out/'report.json'}")
    if "verdict" in report:
        print(f"  {report['verdict']}")
        for f in failures[:5]:
            print(f"    pair {f['pair']}{f['side']} exit={f['exit']} {f['unusable_because']}")
        sys.exit(3)
    print(f"  median {report['median_pct']:+.3f}%  "
          f"CI{int(a.confidence*100)} [{report['ci_pct']['low']:+.3f}%, "
          f"{report['ci_pct']['high']:+.3f}%]  "
          f"half-width {report['half_width_pct']:.3f}%")
    sys.exit(0)


if __name__ == "__main__":
    main()
