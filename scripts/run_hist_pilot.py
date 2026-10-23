#!/usr/bin/env python3
"""The historical pilot: A/A spread, and a positive control, where history is real.

A sibling of run_aa_pilot.py rather than an edit of it. That file and its harness
are what the accepted A/A confirmation measured; leaving them byte-for-byte lets
their hashes keep standing for that result while this one measures a different
operating point.

Two run kinds, and keeping them apart is the point:

* **A/A** — both sides are the same command. Whatever interval comes out is the
  spread of the instrument on this workload. It measures nothing about sharding.
* **control (A/B)** — side A is the baseline and side B does `--extra-pct` more
  *real lookups of the identical distribution*, inside the timing and outside the
  denominator. It is a check that the instrument can see a known amount of extra
  work at all. It is not busy-wait, not a sleep, and no time is corrected
  anywhere.

  **+N% work is not +N% time, and nothing here claims it is.** The extra lookups
  cost whatever they cost; what is known is how much extra *work* was done, which
  the harness counts and this runner checks. One detection is also not a power
  estimate, and the report says so.

Everything else is deliberately the method already accepted: the config is
written before the first measurement, every sample keeps its exit code, full
stderr, command and raw hash, the statistics module is named and hashed, the
output directory must be new, each sample has a timeout, and `n` is fixed.

The report schema is the one the family verifier already checks
(`utxoz-aa-pilot-2`): the report *structure* is identical, and which harness and
runner produced it is in the config by hash. `parse_sample` below is exported for
that verifier to re-parse the raws with.
"""
import argparse, hashlib, importlib.util, json, math, os, platform, shlex
import subprocess, sys, threading, time
from pathlib import Path

HEARTBEAT_SECONDS = 15
SCHEMA = "utxoz-calib-hist-1"
# The second version of the harness document: the historical prefix measured by
# a full scan rather than a binary search, the probe span declared, and a digest
# of the probe sequence so two sides can be shown to have asked the same thing.
SCHEMA_V2 = "utxoz-calib-hist-2"
SCHEMAS = (SCHEMA, SCHEMA_V2)
METRICS = ("hist_hit", "hist_miss")


def sha256_file(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def sha256_text(t):
    return hashlib.sha256(t.encode()).hexdigest()


# The directory claim. Every open rewrites it with the holder's pid and the time,
# which is the file doing its job, so it is not part of the store's identity.
#
# A first version of this hashed the whole directory and the guard below
# correctly vetoed two complete runs, both of whose forty samples were valid: the
# lock had changed, as it must. Naming the exclusion is the honest fix — hashing
# the directory and quietly skipping dotfiles would hide which file was allowed
# to move. The store itself is checked byte for byte, and separately measured: a
# sample rewrites none of it.
LOCK_FILE = ".utxoz.lock"


def fixture_digest(d):
    """One hash over the stored bytes, so the record names what was measured.

    Every sample of the campaign reads this same image. If the store ever changed
    mid-campaign the samples would not be comparable, and a manifest of names and
    sizes would not notice a rewrite that kept the sizes.

    Returns the store's hash, its file count, and the lock's hash separately —
    the lock is expected to move and is recorded rather than ignored.
    """
    files = sorted(p for p in Path(d).iterdir() if p.is_file())
    h = hashlib.sha256()
    n = 0
    lock = None
    for p in files:
        if p.name == LOCK_FILE:
            lock = sha256_file(p)
            continue
        h.update(p.name.encode())
        h.update(b"\0")
        h.update(sha256_file(p).encode())
        h.update(b"\n")
        n += 1
    return h.hexdigest(), n, lock


def load_stats_module(path):
    p = Path(path).resolve()
    if not p.is_file():
        raise SystemExit(f"--stats-module {p} is not a file")
    spec = importlib.util.spec_from_file_location("hist_stats", p)
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

    `expect` is what the caller asked for. The family verifier rebuilds a subset
    of it from the config (metric, ops, warmup, population_observed, shard_count,
    seed), so everything beyond those is checked against the document itself
    rather than against an expectation that may not have been passed in.
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
    if d.get("schema") not in SCHEMAS:
        return None, f"unexpected schema {d.get('schema')!r}"
    v2 = d["schema"] == SCHEMA_V2
    if d.get("metric") not in METRICS:
        return None, f"unexpected metric {d.get('metric')!r}"

    boundary = (("historical_prefix", "active_entries", "probe_below", "geometry_id")
                if v2 else ("active_from", "history_entries"))
    counters = ("ops", "extra_ops", "extra_pct", "total_ops", "warmup", "batch",
                "resolve_calls", "population_observed") + boundary + (
                "generations_class_48", "value_size", "seed",
                "shard_count", "elapsed_ns", "resolved_by_find", "found", "absent",
                "not_found_from_find", "real_errors", "warmup_found",
                "warmup_absent", "warmup_resolved_by_find", "verified_payload",
                "verified_height", "verified_absent",
                "version_files_all_classes", "version_files_class_48")
    for f in counters:
        if f not in d:
            return None, f"missing field {f}"
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
    if d["ops"] <= 0:
        return None, f"ops is {d['ops']}, so there is nothing to divide by"

    # The control's bookkeeping, checked rather than trusted: the extra work has
    # to be exactly the declared percentage of the base, and the total has to be
    # the two added up. A document whose own arithmetic disagrees is not evidence
    # about how much extra work was done.
    if d["extra_ops"] * 100 != d["ops"] * d["extra_pct"]:
        return None, (f"extra_ops {d['extra_ops']} is not {d['extra_pct']}% of "
                      f"ops {d['ops']}")
    if d["total_ops"] != d["ops"] + d["extra_ops"]:
        return None, (f"total_ops {d['total_ops']} is not ops {d['ops']} + "
                      f"extra_ops {d['extra_ops']}")
    if d["batch"] <= 0 or d["total_ops"] % d["batch"] != 0:
        return None, (f"total_ops {d['total_ops']} is not a whole number of "
                      f"batches of {d['batch']}")
    if d["resolve_calls"] != d["total_ops"] // d["batch"]:
        return None, (f"{d['resolve_calls']} resolve calls for {d['total_ops']} "
                      f"lookups in batches of {d['batch']}")

    # Every lookup ends in exactly one of four places. If they do not add up, the
    # document describes something other than the run it claims to.
    outcomes = (d["resolved_by_find"] + d["found"] + d["absent"]
                + d["not_found_from_find"] + d["real_errors"])
    if outcomes != d["total_ops"]:
        return None, (f"outcomes do not partition the lookups: "
                      f"{d['resolved_by_find']}+{d['found']}+{d['absent']}+"
                      f"{d['not_found_from_find']}+{d['real_errors']} = {outcomes}, "
                      f"not {d['total_ops']}")
    # Nothing may have been answered by the active containers: the whole point of
    # this operating point is that the answer is not there.
    if d["resolved_by_find"] != 0:
        return None, (f"{d['resolved_by_find']} probes were answered by the active "
                      f"versions, so they were not historical lookups")
    if d["not_found_from_find"] != 0:
        return None, f"{d['not_found_from_find']} find() calls returned not_found"
    warm = d["warmup_found"] + d["warmup_absent"] + d["warmup_resolved_by_find"]
    if warm != d["warmup"]:
        return None, (f"warmup outcomes come to {warm}, not the {d['warmup']} "
                      f"lookups it says it made")
    for f in ("verified_payload", "verified_height", "verified_absent"):
        if d[f] > d["total_ops"]:
            return None, f"{f} is {d[f]}, more than the {d['total_ops']} lookups"

    # The history has to be real, and large enough that the probes are not all
    # answered out of one container.
    if d["generations_class_48"] < 2:
        return None, (f"{d['generations_class_48']} generations of the sharded "
                      f"class: there is no history to look through")
    if v2:
        prefix = d["historical_prefix"]
        if prefix == 0 or prefix >= d["population_observed"]:
            return None, (f"historical_prefix {prefix} of "
                          f"{d['population_observed']} leaves no history below it")
        if not 1 <= d["probe_below"] <= prefix:
            return None, (f"probe_below {d['probe_below']} is outside 1..{prefix}: "
                          f"some probes could be answered by the active versions")
        digest = d.get("probe_digest")
        if not (isinstance(digest, str) and len(digest) == 16
                and all(c in "0123456789abcdef" for c in digest)):
            return None, f"probe_digest is {digest!r}, not 16 hex digits"
    elif d["active_from"] == 0 or d["active_from"] >= d["population_observed"]:
        return None, (f"active_from {d['active_from']} of "
                      f"{d['population_observed']} leaves no history below it")

    # ns_per_op is elapsed over the BASE ops: the control's extra work is real and
    # timed, and deliberately not in the denominator.
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

    # And the outcomes have to agree with the metric. A miss is PROVEN absent by
    # resolve() or it is not a miss: `not_resolved` from find() only says the
    # active versions could not answer, which is a different statement.
    if d["metric"] == "hist_hit":
        if d["found"] != d["total_ops"]:
            return None, f"hist_hit resolved {d['found']} of {d['total_ops']}"
        if d["absent"] != 0:
            return None, f"hist_hit had {d['absent']} stored entries called absent"
        if (d["verified_payload"] != d["total_ops"]
                or d["verified_height"] != d["total_ops"]):
            return None, ("the harness did not verify every hit's payload and height: "
                          f"{d['verified_payload']}/{d['verified_height']} of "
                          f"{d['total_ops']}")
    else:
        if d["found"] != 0:
            return None, f"hist_miss found {d['found']} keys that were never inserted"
        if d["absent"] != d["total_ops"]:
            return None, (f"hist_miss proved {d['absent']} of {d['total_ops']} "
                          f"absences; the rest were never established")
        if d["verified_absent"] != d["total_ops"]:
            return None, (f"the harness confirmed {d['verified_absent']} of "
                          f"{d['total_ops']} absences")
    return d, None



# ---------------------------------------------------------------------------
# The contract the family verifier asks for, when a runner offers it.
#
# The verifier rebuilds what each raw should say from the config. On its own it
# knows only the fields every runner shares (metric, ops, warmup, population,
# shard count, seed), so a control whose side B was rewritten to do 0% extra
# work - consistently, raw hash and all - re-parsed as a valid sample: the
# document's own arithmetic still added up. The runner checked the side when it
# measured; the verifier did not when it re-read.
#
# These two functions close that. They are the runner's rules, defined once:
# main() uses the same ones to say what it asks the harness for, so what is
# verified is what was asked, not a second description of it.

def side_extra_pct(cfg, side):
    """Side A is always the baseline; side B carries the control's extra work."""
    return cfg["extra_pct_side_b"] if side == "B" else 0


def expect_from_config(cfg, pair, side):
    """Everything one raw has to report, for this pair and this side."""
    return {"metric": cfg["metric"], "ops": cfg["ops_per_sample"],
            "warmup": cfg["warmup_per_sample"], "batch": cfg["batch"],
            "extra_pct": side_extra_pct(cfg, side),
            "population_observed": cfg["population"],
            "shard_count": cfg["shard_count_expected"], "seed": pair}


def check_sample_record(d, rec, cfg):
    """The summary record of a sample has to say what its raw says."""
    if rec.get("extra_pct") != d["extra_pct"]:
        return (f"the record says extra_pct {rec.get('extra_pct')!r} and the raw "
                f"{d['extra_pct']}")
    return None


def verify_report(report, cfg):
    """What a report has to hold beyond its samples. None, or the reason.

    The run kind has to agree with the extra work it declares, every sample
    record has to carry the extra work its side was owed, and the image has to
    be the one the config names, unchanged from first sample to last.
    """
    kind = cfg.get("run_kind")
    extra = cfg.get("extra_pct_side_b")
    if isinstance(extra, bool) or not isinstance(extra, int):
        return f"extra_pct_side_b is {extra!r}, not an integer"
    if kind == "aa":
        if extra != 0:
            return f"an A/A run declares {extra}% extra work on side B"
    elif kind == "control":
        if not 0 < extra <= 100:
            return f"a control run declares {extra}% extra work on side B"
    else:
        return f"run_kind is {kind!r}"
    batch = cfg.get("batch")
    if isinstance(batch, bool) or not isinstance(batch, int) or batch <= 0:
        return f"batch is {batch!r}"

    for rec in report.get("samples", []):
        want = side_extra_pct(cfg, rec.get("side"))
        if rec.get("extra_pct") != want:
            return (f"pair {rec.get('pair')}{rec.get('side')} records extra_pct "
                    f"{rec.get('extra_pct')!r}, not the {want} its side was owed")

    image = report.get("image")
    if not isinstance(image, dict):
        return "the report has no image record"
    if image.get("unchanged") is not True:
        return "the image record says the store changed during the run"
    store = cfg.get("fixture_store_sha256")
    files = cfg.get("fixture_store_files")
    if not store or image.get("store_sha256_before") != store \
            or image.get("store_sha256_after") != store:
        return ("the store hashes before and after are not both the one the "
                "config names")
    if image.get("files_before") != files or image.get("files_after") != files:
        return "the store file counts before and after are not the config's"
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--harness", required=True)
    ap.add_argument("--harness-source", required=True,
                    help="the .cpp the harness was built from; hashed into the record")
    ap.add_argument("--metric", required=True, choices=list(METRICS))
    ap.add_argument("--fixture", required=True,
                    help="an image built by --build-fixture; read-only, shared, hashed")
    ap.add_argument("--stats-module", required=True,
                    help="the file providing bootstrap_median_ci; hashed into the record")
    ap.add_argument("--kind", required=True, choices=["aa", "control"],
                    help="aa: both sides identical. control: side B does --extra-pct more work")
    ap.add_argument("--pairs", type=int, default=20)
    ap.add_argument("--ops", type=int, default=160_000)
    ap.add_argument("--warmup", type=int, default=8_000)
    ap.add_argument("--batch", type=int, default=4_000)
    ap.add_argument("--extra-pct", type=int, default=5,
                    help="control only: extra real lookups on side B")
    ap.add_argument("--population", type=int, required=True,
                    help="what the fixture must report; checked per sample")
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
    if a.batch < 1: problems.append("--batch must be positive")
    if a.ops % a.batch: problems.append("--ops must be a multiple of --batch")
    if a.warmup % a.batch: problems.append("--warmup must be a multiple of --batch")
    if a.population < 1: problems.append("--population must be positive")
    if a.resamples < 100: problems.append("--resamples must be at least 100")
    if not 0.5 < a.confidence < 1.0: problems.append("--confidence must be in (0.5, 1)")
    if not math.isfinite(a.timeout) or a.timeout <= 0:
        problems.append("--timeout must be a finite positive number")
    # The control's extra work must be a whole number of batches, so both arms
    # issue whole resolve() calls and the only difference is how many.
    extra_pct = a.extra_pct if a.kind == "control" else 0
    if not 0 <= extra_pct <= 100:
        problems.append("--extra-pct must be between 0 and 100")
    if a.kind == "control":
        if extra_pct == 0:
            problems.append("a control with --extra-pct 0 is an A/A run; say --kind aa")
        elif (a.ops // a.batch) * extra_pct % 100:
            problems.append(f"--extra-pct {extra_pct} of {a.ops // a.batch} batches is "
                            "not a whole number of batches")
    if problems:
        for p in problems: print(f"  {p}")
        sys.exit(2)

    out = Path(a.out_dir)
    if out.exists():
        print(f"  {out} already exists; this run will not overwrite or extend it")
        sys.exit(2)

    fixture = Path(a.fixture).resolve()
    if not (fixture / "shape.json").is_file():
        print(f"  {fixture} has no shape.json; build it with --build-fixture")
        sys.exit(2)

    # Every input checked before the output directory exists: a refusal must not
    # leave one behind for the retry to trip over.
    stats, stats_hash, stats_path = load_stats_module(a.stats_module)
    harness = Path(a.harness).resolve()
    if not os.access(harness, os.X_OK):
        print(f"  {harness} is not executable"); sys.exit(2)
    source = Path(a.harness_source).resolve()
    if not source.is_file():
        print(f"  {source} is not a file"); sys.exit(2)

    out.mkdir(parents=True)
    raws = out / "raw"; raws.mkdir()

    print("  hashing the fixture before measuring...", flush=True)
    fx_hash, fx_files, fx_lock = fixture_digest(fixture)

    config = {
        "schema": "utxoz-aa-pilot-2",
        "kind": ("A/A - both sides are the same command"
                 if a.kind == "aa" else
                 f"control A/B - side B does {extra_pct}% more real lookups of the "
                 "same distribution, inside the timing and outside the denominator"),
        "run_kind": a.kind,
        "metric": a.metric,
        "pairs": a.pairs,
        "ops_per_sample": a.ops,
        "warmup_per_sample": a.warmup,
        "batch": a.batch,
        "extra_pct_side_b": extra_pct,
        "population": a.population,
        "shard_count_expected": a.shard_count,
        "fixture": str(fixture),
        "fixture_store_sha256": fx_hash,
        "fixture_store_files": fx_files,
        "fixture_lock_sha256_before": fx_lock,
        "fixture_lock_note": (
            f"{LOCK_FILE} carries the holder's pid and the time of the claim, so "
            "every open rewrites it by design. It is excluded from the store hash "
            "and recorded here instead. The stored bytes are checked in full."),
        "cache": ("warm per process; the warmup walks the same generations. The "
                  "library's file cache holds one file (its default), so every "
                  "batch re-maps the history: that is the code as it stands."),
        "denominator": ("elapsed / base ops. Control work is real, timed and "
                        "outside the denominator; +N% work is NOT +N% time."),
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
        "harness_sha256": sha256_file(harness),
        "harness_source_sha256": sha256_file(source),
        "runner_sha256": sha256_file(__file__),
        "host": platform.node(),
        "kernel": platform.release(),
        "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    }
    config["config_sha256"] = sha256_text(json.dumps(config, sort_keys=True))
    # Written before the first measurement, so the record cannot be shaped by it.
    (out / "config.json").write_text(json.dumps(config, indent=1))
    print(f"  fixture_store_sha256={fx_hash} ({fx_files} files, lock excluded)")
    print(f"  config_sha256={config['config_sha256']}")
    print(f"  config written to {out/'config.json'} before measuring")
    sys.stdout.flush()

    # Side A is always the baseline; side B carries the extra work in a control
    # run and is identical to A in an A/A run. The *order* alternates, not the
    # roles: a ratio of B over A has to mean the same thing in every pair.
    # The same rules the verifier will apply to these raws later.
    def extra_for(side):
        return side_extra_pct(config, side)

    def expect_for(pair, side):
        return expect_from_config(config, pair, side)

    sides, samples, failures = {"A": [], "B": []}, [], []
    for r in range(1, a.pairs + 1):
        for side in (("A", "B") if r % 2 == 1 else ("B", "A")):
            cmd = ["taskset", "-c", str(a.cpu), str(harness),
                   "--metric", a.metric, "--fixture", str(fixture),
                   "--ops", str(a.ops), "--warmup", str(a.warmup),
                   "--batch", str(a.batch), "--extra-pct", str(extra_for(side)),
                   "--seed", str(r)]
            t0 = time.time()
            print(f"  [pair {r}/{a.pairs} {side}] START {time.strftime('%H:%M:%S')}",
                  flush=True)
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
                     else parse_sample(so, expect_for(r, side), stem)
            # A valid document from a process that then failed is not a
            # measurement: the same rule as run_hist_grid.py.
            if d is not None and code != 0:
                d, why = None, f"exit {code}"
            rec = {"pair": r, "side": side, "exit": code, "timed_out": timed_out,
                   "extra_pct": extra_for(side),
                   "command": shlex.join(cmd),
                   "raw_sha256": sha256_text(so), "stderr": se,
                   "seconds": time.time() - t0,
                   "ns_per_op": (d["ns_per_op"] if d else None),
                   "unusable_because": why}
            samples.append(rec)
            print(f"  [pair {r}/{a.pairs} {side}] END   {time.strftime('%H:%M:%S')} "
                  f"{rec['seconds']:.1f}s exit={code} "
                  f"{'ns/op=%.3f' % d['ns_per_op'] if d else 'UNUSABLE: ' + why}",
                  flush=True)
            if d is None or code != 0:
                failures.append(rec)
            else:
                sides[side].append(d["ns_per_op"])

    # The image every sample read has to be the image the config names. A sample
    # that silently rewrote it would make the earlier and later samples
    # measurements of two different stores.
    after_hash, after_files, after_lock = fixture_digest(fixture)
    image = {"store_sha256_before": fx_hash, "store_sha256_after": after_hash,
             "files_before": fx_files, "files_after": after_files,
             "lock_sha256_before": fx_lock, "lock_sha256_after": after_lock,
             "lock_moved": after_lock != fx_lock,
             "unchanged": after_hash == fx_hash and after_files == fx_files}

    report = {"config": config, "samples": samples, "failures": failures,
              "ns_per_op": sides, "image": image}

    if not image["unchanged"]:
        report["verdict"] = ("NO MEASUREMENT - the stored bytes changed during the "
                             "run, so the samples are not of one store")
    elif failures or len(sides["A"]) != a.pairs or len(sides["B"]) != a.pairs:
        report["verdict"] = "NO MEASUREMENT - at least one sample was not usable"
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
            "language": (
                "A/A: observed precision of this pilot, on this metric, at this "
                "operating point. Control: evidence that this much extra real work "
                "is visible here; +N% work is not +N% time, and one detection is "
                "not a power estimate. Neither is a floor for future A/B "
                "comparisons or a statement about false positives."),
        })

    (out / "report.json").write_text(json.dumps(report, indent=1))
    print(f"  written: {out/'report.json'}")
    print(f"  store after: {after_hash} unchanged={image['unchanged']} "
          f"lock_moved={image['lock_moved']}")
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
