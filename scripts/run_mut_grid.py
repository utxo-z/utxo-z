#!/usr/bin/env python3
"""Paired insertion and compaction comparisons, each sample on its own copy.

The read grid (run_hist_grid.py) lets every sample share an image. A mutation
cannot, so here each sample's harness copies its side's immutable source into a
new directory, verifies the copy against the rule the source was built with, and
only then measures. This runner adds what that needs and nothing else:

* **A scratch root of its own.** Created new under the declared parent - never an
  inherited directory - and the only place samples write.
* **Cleanup that can only remove what this run made.** A work directory is
  removed only after its raw, stderr, command and sizes are saved, and only if
  it is inside this run's scratch root, is named as the harness names its work
  directories, and carries an owner marker with the token the harness reported.
  A failed sample's copy is kept while the budget allows it, and the report says
  which were kept.
* **Space budgets, checked before every sample.** At least `min_free_gib` free on
  the filesystem after the sample's copy, and at most `max_scratch_gib` in the
  scratch root.
* **An external grant, or nothing runs.** A timed campaign shares the machine;
  whether it may run is the coordinator's decision, written down as an
  authorization file (campaign_auth.py). Checked before anything is created and
  again before every sample - re-read each time, so revoking it, or its window
  closing, stops the campaign. Its hash is in every config.
* **No inheritance.** Every sample's work directory and token are unique, and its
  initial content digest - computed by the harness from the copy - has to equal
  the one every other sample reported: the same rule, the same image.

Schedule, order, seeds, estimator and report format are the read grid's: rounds
with cells rotated, blocks AB,BA,BA,AB, one report per cell that the family
verifier checks through this module's contract. The ratio is B/A of the
sample's total elapsed time; `ns_per_op` in the records holds that total, so the
verifier's field means "the sample's measured time" here.
"""
import argparse, hashlib, importlib.util, json, math, os, platform, shlex, shutil
import subprocess, sys, time
from pathlib import Path

HERE = Path(__file__).resolve().parent


def _load(name, file):
    spec = importlib.util.spec_from_file_location(name, HERE / file)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


grid = _load("hist_grid", "run_hist_grid.py")
auth = _load("campaign_auth", "campaign_auth.py")
first_side, cell_order, run_sample = grid.first_side, grid.cell_order, grid.run_sample
sha256_file, sha256_text, fixture_digest = grid.sha256_file, grid.sha256_text, grid.fixture_digest

SCHEMA = "utxoz-calib-mut-1"
METRICS = ("insert_lifecycle", "compact")
KINDS = ("aa", "ab")
GIB = 1 << 30
OWNER_FILE = ".calibmut-owner"


def _finite(v):
    return isinstance(v, (int, float)) and not isinstance(v, bool) and math.isfinite(v)


def parse_sample(text, expect, path_hint=""):
    """One harness document, or the reason it is not a measurement."""
    if not text.strip():
        return None, f"no output from {path_hint or 'the harness'}"
    try:
        d = json.loads(text, parse_constant=lambda c: (_ for _ in ()).throw(
            ValueError(f"non-numeric constant {c!r}")))
    except (json.JSONDecodeError, ValueError) as e:
        return None, f"output is not usable JSON ({e})"
    if not isinstance(d, dict):
        return None, f"output is {type(d).__name__}, not an object"
    if d.get("schema") != SCHEMA:
        return None, f"unexpected schema {d.get('schema')!r}"
    if d.get("metric") not in METRICS:
        return None, f"unexpected metric {d.get('metric')!r}"
    counters = ("shard_count", "geometry_id", "population", "value_size", "inserts",
                "pause_every", "delete_batch", "setup_deletions", "seed", "copy_files",
                "copy_bytes", "elapsed_ns", "failed_ops", "batches", "batch_ns_p50",
                "batch_ns_p99", "batch_ns_max", "rotations_class48",
                "generations_class48_before", "generations_class48_after",
                "version_files_before", "version_files_after", "logical_bytes_before",
                "logical_bytes_after", "entries_after", "maxrss_kb")
    for f in counters:
        if f not in d:
            return None, f"missing field {f}"
        if isinstance(d[f], bool) or not isinstance(d[f], int) or d[f] < 0:
            return None, f"field {f} is {d[f]!r}, not a non-negative integer"
    for f in ("initial_digest", "final_digest", "expected_digest", "token"):
        v = d.get(f)
        if not (isinstance(v, str) and len(v) == 16 and all(c in "0123456789abcdef" for c in v)):
            return None, f"field {f} is {v!r}, not 16 hex digits"
    for f in ("work_dir", "source"):
        if not isinstance(d.get(f), str) or not d[f]:
            return None, f"field {f} is missing"
    if not _finite(d.get("copy_ms")):
        return None, "copy_ms is not a finite number"
    if not isinstance(d.get("valid"), bool):
        return None, "field valid is not a bool"
    if not d["valid"]:
        return None, f"the harness reported the operation invalid: {d.get('invalid_because')}"
    if d["elapsed_ns"] <= 0:
        return None, "elapsed_ns is not positive"
    if d["failed_ops"] != 0:
        return None, f"{d['failed_ops']} operations did not complete"
    if d["final_digest"] != d["expected_digest"]:
        return None, "the final content is not what the rule predicts"
    if d["metric"] == "insert_lifecycle":
        if d["inserts"] <= 0 or d["entries_after"] != d["population"] + d["inserts"]:
            return None, (f"{d['entries_after']} entries after {d['inserts']} inserts into "
                          f"{d['population']}")
        if d["batches"] != d["inserts"] // d["pause_every"]:
            return None, f"{d['batches']} batch marks for {d['inserts']} inserts"
    else:
        if d["setup_deletions"] != d["population"] // 2 or \
                d["entries_after"] != d["population"] - d["population"] // 2:
            return None, (f"{d['setup_deletions']} deletions and {d['entries_after']} entries "
                          f"after, from {d['population']}")
        if d["inserts"] != 0 or d["batches"] != 0:
            return None, "a compaction sample reports inserts or batches"
    # The ratio is of the sample's total time; the verifier calls the field ns_per_op.
    d["ns_per_op"] = float(d["elapsed_ns"])
    for field, want in expect.items():
        if d.get(field) != want:
            return None, f"{field} is {d.get(field)!r}, not the {want!r} requested"
    return d, None


def expect_from_config(cfg, pair, side):
    s = cfg["sides"][side]
    e = {"schema": SCHEMA, "metric": cfg["metric"], "shard_count": s["shard_count"],
         "geometry_id": s["geometry_id"], "population": s["population"],
         "source": s["source"], "seed": pair, "delete_batch": cfg["delete_batch"],
         "pause_every": cfg["pause_every"]}
    e["inserts"] = cfg["inserts"] if cfg["metric"] == "insert_lifecycle" else 0
    return e


def check_sample_record(d, rec, cfg):
    for f in ("work_dir", "token", "initial_digest", "final_digest", "elapsed_ns"):
        if rec.get(f) != d[f]:
            return f"the record says {f} {rec.get(f)!r} and the raw {d[f]!r}"
    return None


def verify_report(report, cfg):
    kind = cfg.get("run_kind")
    sides = cfg.get("sides")
    if kind not in KINDS:
        return f"run_kind is {kind!r}"
    if not isinstance(sides, dict) or set(sides) != {"A", "B"}:
        return "the config does not describe exactly two sides"
    a, b = sides["A"], sides["B"]
    for name, s in (("A", a), ("B", b)):
        for f in ("harness_sha256", "source_store_sha256", "source_store_files",
                  "shard_count", "geometry_id", "population", "source"):
            if f not in s:
                return f"side {name} has no {f}"
    if a["population"] != b["population"]:
        return "the two sides start from different populations"
    if kind == "aa":
        if a["harness_sha256"] != b["harness_sha256"] or a["source_store_sha256"] != b["source_store_sha256"]:
            return "an A/A cell must run the same binary on the same source"
    elif a["shard_count"] != 1 or b["shard_count"] == 1:
        return "a comparison cell is N=1 against N>1"

    dirs, tokens, initial = set(), set(), set()
    for rec in report.get("samples", []):
        if rec.get("ran_first") != first_side(rec.get("pair", 0)):
            return f"pair {rec.get('pair')} did not run in the declared block order"
        if rec.get("work_dir") in dirs or rec.get("token") in tokens:
            return f"pair {rec.get('pair')}{rec.get('side')} reused a work directory or token"
        dirs.add(rec.get("work_dir")); tokens.add(rec.get("token"))
        initial.add(rec.get("initial_digest"))
    # Every copy, on either side, held the same content before it was measured.
    if len(initial) != 1:
        return f"the samples started from different content: {sorted(initial)}"

    image = report.get("image")
    if not isinstance(image, dict):
        return "the report has no image record"
    for name, s in (("A", a), ("B", b)):
        im = image.get(name)
        if not isinstance(im, dict) or im.get("unchanged") is not True:
            return f"side {name}'s source changed during the run"
        if im.get("store_sha256_before") != s["source_store_sha256"] \
                or im.get("store_sha256_after") != s["source_store_sha256"]:
            return f"side {name}'s source hashes are not the one the config names"
    return None


# ---------------------------------------------------------------------------

def du_bytes(path):
    total = 0
    for root, _dirs, files in os.walk(path):
        for f in files:
            try:
                total += os.lstat(os.path.join(root, f)).st_blocks * 512
            except OSError:
                pass
    return total


def free_bytes(path):
    st = os.statvfs(path)
    return st.f_bavail * st.f_frsize


def safe_remove(work, scratch, token):
    """Removes a work directory only if it provably is one this run made."""
    try:
        w = Path(work).resolve(strict=True)
    except (OSError, RuntimeError):
        return "not there"
    if w.parent != scratch.resolve():
        return f"{w} is not directly inside this run's scratch root; kept"
    if not w.name.startswith("calibmut_") or not w.is_dir() or w.is_symlink():
        return f"{w} is not a harness work directory; kept"
    marker = w / OWNER_FILE
    try:
        text = marker.read_text()
    except OSError:
        return f"{w} has no owner marker; kept"
    if f"token={token}\n" not in text:
        return f"{w}'s owner marker names another token; kept"
    shutil.rmtree(w)
    return "removed"


def load_spec(path):
    spec = json.loads(Path(path).read_text())
    problems = []
    def need_int(name, lo, hi=None):
        v = spec.get(name)
        if isinstance(v, bool) or not isinstance(v, int) or v < lo or (hi is not None and v > hi):
            problems.append(f"{name} is {v!r}")
    need_int("pairs", 1, 10)
    need_int("inserts", 1); need_int("pause_every", 1); need_int("delete_batch", 1)
    need_int("cpu", 0); need_int("resamples", 100); need_int("bootstrap_seed", 0)
    need_int("min_free_gib", 10); need_int("max_scratch_gib", 1, 4)
    for f in ("timeout",):
        v = spec.get(f)
        if not _finite(v) or v <= 0: problems.append(f"{f} is {v!r}")
    c = spec.get("confidence")
    if not _finite(c) or not 0.5 < c < 1: problems.append(f"confidence is {c!r}")
    sides, cells = spec.get("sides"), spec.get("cells")
    if not isinstance(sides, dict) or not sides: problems.append("no sides")
    if not isinstance(cells, list) or not cells: problems.append("no cells")
    else:
        names = [x.get("name") for x in cells]
        if len(set(names)) != len(names): problems.append("two cells share a name")
        for x in cells:
            if x.get("kind") not in KINDS: problems.append(f"cell {x.get('name')}: kind")
            if x.get("metric") not in METRICS: problems.append(f"cell {x.get('name')}: metric")
            for s in ("A", "B"):
                if not isinstance(sides, dict) or x.get(s) not in sides:
                    problems.append(f"cell {x.get('name')}: side {s} not declared")
            if x.get("kind") == "aa" and x.get("A") != x.get("B"):
                problems.append(f"cell {x.get('name')}: aa runs one side twice")
            if x.get("kind") == "ab" and x.get("A") == x.get("B"):
                problems.append(f"cell {x.get('name')}: ab needs two sides")
    if not isinstance(spec.get("task_id"), str) or not spec["task_id"].strip():
        problems.append("task_id is missing: a campaign names the task it was granted for")
    parent = Path(spec.get("scratch_parent", ""))
    if not parent.is_dir(): problems.append(f"scratch_parent {parent} is not a directory")
    return spec, problems


def describe_side(name, d):
    harness = Path(d["harness"]).resolve()
    source = Path(d["source"]).resolve()
    if not os.access(harness, os.X_OK):
        raise SystemExit(f"side {name}: {harness} is not executable")
    shape = json.loads((source / "shape.json").read_text())
    print(f"  hashing side {name}'s source...", flush=True)
    store, files, _lock = fixture_digest(source)
    return {"name": name, "harness": str(harness), "harness_sha256": sha256_file(harness),
            "source": str(source), "source_store_sha256": store, "source_store_files": files,
            "source_bytes": du_bytes(source), "shape_sha256": sha256_file(source / "shape.json"),
            "census_sha256": sha256_file(source / "census.json"),
            "shard_count": shape["shard_count"], "geometry_id": shape["geometry_id"],
            "population": shape["population"]}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--spec", required=True)
    ap.add_argument("--out-dir", required=True, help="must not exist")
    ap.add_argument("--authorization", default=None,
                    help="the coordinator's grant for this task and host; see campaign_auth.py")
    a = ap.parse_args()
    spec, problems = load_spec(a.spec)
    if problems:
        for p in problems: print(f"  {p}")
        sys.exit(2)
    out = Path(a.out_dir)
    if out.exists():
        print(f"  {out} already exists"); sys.exit(2)
    # Before anything is created: no grant, no campaign.
    grant, why = auth.check(a.authorization, spec["task_id"])
    if grant is None:
        print(f"  NOT AUTHORIZED: {why}")
        print("  nothing was created, copied or measured")
        sys.exit(2)
    print(f"  authorized: task {grant['task_id']} on {grant['host']} until {grant['expires_utc']} "
          f"by {grant['granted_by']} (grant sha256 {grant['sha256'][:16]})", flush=True)

    stats_path = Path(spec["stats_module"]).resolve()
    sp = importlib.util.spec_from_file_location("mut_stats", stats_path)
    stats = importlib.util.module_from_spec(sp); sp.loader.exec_module(stats)
    source_cpp = Path(spec["harness_source"]).resolve()

    used = sorted({x[s] for x in spec["cells"] for s in ("A", "B")})
    sides = {n: describe_side(n, spec["sides"][n]) for n in used}
    for x in spec["cells"]:
        if x["kind"] == "ab" and (sides[x["A"]]["shard_count"] != 1 or sides[x["B"]]["shard_count"] == 1):
            print(f"  cell {x['name']}: A must be the N=1 baseline and B an N>1 side"); sys.exit(2)
    biggest = max(s["source_bytes"] for s in sides.values())
    if biggest * 2 > spec["max_scratch_gib"] * GIB:
        print(f"  a copy of {biggest/GIB:.2f} GiB, grown, does not fit the scratch budget"); sys.exit(2)

    out.mkdir(parents=True)
    # A scratch root this run creates, under the declared parent, and the only
    # directory any sample writes in.
    scratch = Path(spec["scratch_parent"]) / f"mutrun_{os.getpid()}_{hashlib.sha256(os.urandom(16)).hexdigest()[:12]}"
    scratch.mkdir()
    spec_text = Path(a.spec).read_text()
    (out / "spec.json").write_text(spec_text)
    runner_sha = sha256_text(sha256_file(__file__) + sha256_file(HERE / "run_hist_grid.py")
                             + sha256_file(HERE / "run_hist_pilot.py"))
    started = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
    population = sides[used[0]]["population"]

    cells, configs = spec["cells"], {}
    for x in cells:
        cdir = out / x["name"]; (cdir / "raw").mkdir(parents=True)
        cs = {s: dict(sides[x[s]]) for s in ("A", "B")}
        cfg = {
            "schema": "utxoz-aa-pilot-2",
            "kind": f"{x['kind']}: A={x['A']} B={x['B']}, {x['metric']}",
            "run_kind": x["kind"], "cell": x["name"], "metric": x["metric"],
            "pairs": spec["pairs"], "ops_per_sample": spec["inserts"] if x["metric"] == "insert_lifecycle" else 1,
            "warmup_per_sample": 0, "inserts": spec["inserts"], "pause_every": spec["pause_every"],
            "delete_batch": spec["delete_batch"], "population": population,
            "shard_count_expected": {k: v["shard_count"] for k, v in cs.items()},
            "sides": cs, "scratch_root": str(scratch),
            "order": "rounds r=1..pairs, cells rotated by r-1; blocks AB,BA,BA,AB by r mod 4",
            "seed_rule": "seed r is recorded; the workload is deterministic",
            "restoration": ("every sample copies its side's source with plain read/write, "
                            "fsyncs, and verifies the copy's content digest against the rule "
                            "before timing; the same method on every side"),
            "measured": "total elapsed of the timed region, as measured; no correction",
            "cpu_affinity": spec["cpu"], "timeout_seconds_per_sample": spec["timeout"],
            "estimator": {"module": str(stats_path), "module_sha256": sha256_file(stats_path),
                          "function": "bootstrap_median_ci", "resamples": spec["resamples"],
                          "confidence": spec["confidence"], "seed": spec["bootstrap_seed"]},
            "harness": f"A: {cs['A']['harness']} | B: {cs['B']['harness']}",
            "harness_sha256": sha256_text(cs["A"]["harness_sha256"] + cs["B"]["harness_sha256"]),
            "harness_source_sha256": sha256_file(source_cpp),
            "runner_sha256": runner_sha, "spec_sha256": sha256_text(spec_text),
            "host": platform.node(), "kernel": platform.release(), "started_utc": started,
            "task_id": spec["task_id"],
            "authorization": {k: grant[k] for k in ("sha256", "path", "task_id", "host",
                                                     "expires_utc", "approval_code", "granted_by")},
        }
        cfg["config_sha256"] = sha256_text(json.dumps(cfg, sort_keys=True))
        (cdir / "config.json").write_text(json.dumps(cfg, indent=1))
        configs[x["name"]] = cfg

    plan = [{"round": r, "cell": x["name"], "first": first_side(r)}
            for r in range(1, spec["pairs"] + 1) for x in cell_order(cells, r)]
    (out / "schedule.json").write_text(json.dumps(plan, indent=1))
    print(f"  spec_sha256={sha256_text(spec_text)} runner_sha256={runner_sha}")
    print(f"  scratch root {scratch}; {len(cells)} cells, {2*len(plan)} samples; "
          f"configs and schedule written before measuring", flush=True)

    results = {x["name"]: {"samples": [], "failures": [], "A": {}, "B": {}} for x in cells}
    housekeeping, position, stop_reason = [], 0, None
    for step in plan:
        if stop_reason: break
        x = next(c for c in cells if c["name"] == step["cell"])
        cfg, r, res = configs[x["name"]], step["round"], results[x["name"]]
        docs = {}
        for side in (("A", "B") if step["first"] == "A" else ("B", "A")):
            s = cfg["sides"][side]
            # Re-read every time: a grant revoked, replaced or past its window stops
            # the campaign before the next sample, not after it.
            again, why = auth.check(a.authorization, spec["task_id"])
            if again is None or again["sha256"] != grant["sha256"]:
                stop_reason = f"authorization no longer valid: {why or 'the grant file changed'}"
                break
            need = 2 * s["source_bytes"]
            if free_bytes(scratch) - need < spec["min_free_gib"] * GIB:
                stop_reason = f"free space would fall below {spec['min_free_gib']} GiB"; break
            if du_bytes(scratch) + need > spec["max_scratch_gib"] * GIB:
                stop_reason = f"the scratch root would exceed {spec['max_scratch_gib']} GiB"; break
            position += 1
            cmd = ["taskset", "-c", str(spec["cpu"]), s["harness"], "--metric", cfg["metric"],
                   "--source", s["source"], "--scratch-root", str(scratch),
                   "--inserts", str(spec["inserts"]), "--pause-every", str(spec["pause_every"]),
                   "--delete-batch", str(spec["delete_batch"]), "--seed", str(r)]
            so, se, code, timed_out, secs = run_sample(
                cmd, spec["timeout"], f"[{position}/{2*len(plan)}] {x['name']} pair {r} {side}")
            stem = f"{cfg['metric']}-pair{r:02d}-{side}"
            raw = out / x["name"] / "raw"
            (raw / f"{stem}.json").write_text(so)
            (raw / f"{stem}.stderr").write_text(se)
            (raw / f"{stem}.cmd").write_text(shlex.join(cmd) + "\n")
            d, why = (None, f"timed out after {spec['timeout']}s") if timed_out \
                else parse_sample(so, expect_from_config(cfg, r, side), stem)
            if d is not None and code != 0:
                d, why = None, f"exit {code}"
            try:
                partial = json.loads(so) if so.strip() else {}
            except json.JSONDecodeError:
                partial = {}
            work, token = partial.get("work_dir"), partial.get("token")
            rec = {"pair": r, "side": side, "exit": code, "timed_out": timed_out,
                   "ran_first": step["first"], "position": position, "command": shlex.join(cmd),
                   "raw_sha256": sha256_text(so), "stderr": se, "seconds": secs,
                   "ns_per_op": d["ns_per_op"] if d else None,
                   "elapsed_ns": d["elapsed_ns"] if d else None,
                   "work_dir": work, "token": token,
                   "initial_digest": partial.get("initial_digest"),
                   "final_digest": partial.get("final_digest"),
                   "work_bytes": du_bytes(work) if work and Path(work).is_dir() else None,
                   "unusable_because": why}
            # Saved, then cleaned: a valid sample's copy has nothing left to say;
            # a failed one is kept while the budget allows.
            if work:
                keep_failed = d is None and du_bytes(scratch) + need <= spec["max_scratch_gib"] * GIB \
                    and free_bytes(scratch) - need >= spec["min_free_gib"] * GIB
                rec["cleanup"] = "kept: the sample failed" if keep_failed \
                    else safe_remove(work, scratch, token)
                housekeeping.append({"position": position, "work_dir": work, "cleanup": rec["cleanup"]})
            res["samples"].append(rec); docs[side] = rec
            print(f"  [{position}] END {secs:.1f}s exit={code} "
                  f"{'elapsed=%.1f ms' % (d['elapsed_ns']/1e6) if d else 'UNUSABLE: ' + str(why)} "
                  f"cleanup={rec.get('cleanup')}", flush=True)
        for s in docs:
            if docs[s]["unusable_because"] is None: res[s][r] = docs[s]["ns_per_op"]
            else: res["failures"].append(docs[s])

    (out / "housekeeping.json").write_text(json.dumps(
        {"scratch_root": str(scratch), "entries": housekeeping, "stopped": stop_reason,
         "left_in_scratch": sorted(p.name for p in scratch.iterdir())}, indent=1))

    after = {}
    for n, s in sides.items():
        h, f, _ = fixture_digest(Path(s["source"]))
        after[n] = {"store_sha256_before": s["source_store_sha256"], "store_sha256_after": h,
                    "files_before": s["source_store_files"], "files_after": f,
                    "unchanged": h == s["source_store_sha256"] and f == s["source_store_files"]}
    worst = 3 if stop_reason else 0
    if stop_reason: print(f"  STOPPED: {stop_reason}")
    for x in cells:
        cfg, res = configs[x["name"]], results[x["name"]]
        image = {side: after[x[side]] for side in ("A", "B")}
        A = [res["A"].get(r) for r in range(1, spec["pairs"] + 1)]
        B = [res["B"].get(r) for r in range(1, spec["pairs"] + 1)]
        report = {"config": cfg, "samples": res["samples"], "failures": res["failures"],
                  "ns_per_op": {"A": [v for v in A if v is not None], "B": [v for v in B if v is not None]},
                  "image": image}
        if not all(im["unchanged"] for im in image.values()):
            report["verdict"] = "NO MEASUREMENT - a source changed during the run"
        elif res["failures"] or None in A or None in B:
            report["verdict"] = "NO MEASUREMENT - at least one sample was not usable or not run"
        else:
            ratios = [b / v for v, b in zip(A, B)]
            med, lo, hi = stats.bootstrap_median_ci(ratios, spec["resamples"], spec["confidence"],
                                                    spec["bootstrap_seed"])
            report.update({"ratios": ratios, "median_ratio": med, "ci": {"low": lo, "high": hi},
                           "median_pct": (med - 1) * 100,
                           "ci_pct": {"low": (lo - 1) * 100, "high": (hi - 1) * 100}})
        (out / x["name"] / "report.json").write_text(json.dumps(report, indent=1))
        if "verdict" in report:
            worst = 3; print(f"  {x['name']}: {report['verdict']}")
        else:
            print(f"  {x['name']}: median {report['median_pct']:+.2f}%  CI95 "
                  f"[{report['ci_pct']['low']:+.2f}%, {report['ci_pct']['high']:+.2f}%]")
    sys.exit(worst)


if __name__ == "__main__":
    main()
