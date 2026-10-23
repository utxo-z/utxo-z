#!/usr/bin/env python3
"""Diagnostic repetitions of an observed compaction per shard count. No statistics.

Each repetition runs utxoz_calib_mut --metric compact --observe-compaction on a
private copy of a side's immutable source, exactly as run_mut_grid does, and
keeps what the observation recorded: one interval per container between the
library's own log marks, the RSS high-water mark over compact_all, and the
sampled peak of the work directory's bytes. There are no pairs and no
confidence intervals: this describes the unit of work and the peaks, it does
not compare times.

Everything that protects a campaign is reused from run_mut_grid.py, unchanged:
the external authorization (re-read before every repetition), a scratch root of
its own, removal of a work copy only when it provably is one this run made,
the space budgets, and hashes of every source before and after.

Spec:
  task_id, scratch_parent, min_free_gib (>=10), max_scratch_gib (<=4),
  repetitions (1..3), cpu, sampler_cpu, sample_us, delete_batch, timeout,
  harness_source, sides: {name: {harness, source}}
Order: repetition r runs every side, the sides rotated by r-1.
"""
import argparse, importlib.util, json, os, platform, shlex, sys, time
from pathlib import Path

HERE = Path(__file__).resolve().parent
spec_ = importlib.util.spec_from_file_location("mut_grid", HERE / "run_mut_grid.py")
grid = importlib.util.module_from_spec(spec_); spec_.loader.exec_module(grid)
auth, GIB = grid.auth, grid.GIB


def expect_for(side, rep, spec):
    return {"schema": grid.SCHEMA, "metric": "compact", "shard_count": side["shard_count"],
            "geometry_id": side["geometry_id"], "population": side["population"],
            "source": side["source"], "seed": rep, "delete_batch": spec["delete_batch"], "inserts": 0}


def observation_problem(d, side):
    """None if the observation is complete and of this geometry, else why."""
    o = d.get("observation")
    if d.get("observed") is not True or not isinstance(o, dict):
        return "the harness did not observe"
    if o.get("marks_valid") is not True:
        return f"the marks are not valid: {o.get('marks_invalid_because')}"
    want = [(0, s) for s in range(side["shard_count"])] + [(c, 0) for c in range(1, 5)]
    got = [(i.get("class"), i.get("shard")) for i in o.get("intervals", [])]
    if got != want:
        return f"the intervals are {got}, not one per container of N={side['shard_count']}"
    if not isinstance(o.get("rss_hwm_available"), bool) or o.get("disk_samples", 0) < 2:
        return "the RSS or disk observation is missing"
    return None


def load_spec(path):
    spec = json.loads(Path(path).read_text())
    problems = []
    for name, lo, hi in (("repetitions", 1, 3), ("cpu", 0, None), ("sampler_cpu", 0, None),
                         ("sample_us", 100, None), ("delete_batch", 1, None),
                         ("min_free_gib", 10, None), ("max_scratch_gib", 1, 4)):
        v = spec.get(name)
        if isinstance(v, bool) or not isinstance(v, int) or v < lo or (hi is not None and v > hi):
            problems.append(f"{name} is {v!r}")
    if spec.get("cpu") == spec.get("sampler_cpu"):
        problems.append("the sampler must not share the measured CPU")
    t = spec.get("timeout")
    if not isinstance(t, (int, float)) or isinstance(t, bool) or not t > 0:
        problems.append(f"timeout is {t!r}")
    if not isinstance(spec.get("task_id"), str) or not spec["task_id"].strip():
        problems.append("task_id is missing")
    if not isinstance(spec.get("sides"), dict) or not spec["sides"]:
        problems.append("no sides")
    parent = spec.get("scratch_parent")
    if not isinstance(parent, str) or not parent.strip() or not Path(parent).is_dir():
        problems.append(f"scratch_parent {parent!r} is not an existing directory")
    src = spec.get("harness_source")
    if not isinstance(src, str) or not src.strip() or not Path(src).is_file():
        problems.append(f"harness_source {src!r} is not an existing file")
    return spec, problems


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--spec", required=True)
    ap.add_argument("--out-dir", required=True, help="must not exist")
    ap.add_argument("--authorization", default=None)
    a = ap.parse_args()
    spec, problems = load_spec(a.spec)
    if problems:
        for p in problems: print(f"  {p}")
        sys.exit(2)
    out = Path(a.out_dir)
    if out.exists():
        print(f"  {out} already exists"); sys.exit(2)
    grant, why = auth.check(a.authorization, spec["task_id"])
    if grant is None:
        print(f"  NOT AUTHORIZED: {why}")
        print("  nothing was created, copied or measured")
        sys.exit(2)

    names = sorted(spec["sides"])
    sides = {n: grid.describe_side(n, spec["sides"][n]) for n in names}
    out.mkdir(parents=True); (out / "raw").mkdir()
    scratch = Path(spec["scratch_parent"]) / f"obsrun_{os.getpid()}_{os.urandom(6).hex()}"
    scratch.mkdir()
    spec_text = Path(a.spec).read_text()
    config = {"schema": "utxoz-mut-observe-1", "spec": spec, "spec_sha256": grid.sha256_text(spec_text),
              "sides": sides, "scratch_root": str(scratch),
              "runner_sha256": grid.sha256_text(grid.sha256_file(__file__)
                                                + grid.sha256_file(HERE / "run_mut_grid.py")),
              "harness_source_sha256": grid.sha256_file(spec["harness_source"]),
              "authorization": {k: grant[k] for k in ("sha256", "path", "task_id", "host",
                                                       "expires_utc", "approval_code", "granted_by")},
              "host": platform.node(), "kernel": platform.release(),
              "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())}
    (out / "config.json").write_text(json.dumps(config, indent=1))
    plan = [(r, names[(r - 1) % len(names):] + names[:(r - 1) % len(names)]) for r in range(1, spec["repetitions"] + 1)]
    (out / "schedule.json").write_text(json.dumps([{"repetition": r, "order": o} for r, o in plan], indent=1))
    print(f"  authorized until {grant['expires_utc']}; {len(names)} sides x {spec['repetitions']} repetitions", flush=True)

    records, stop = [], None
    for r, order in plan:
        if stop: break
        for n in order:
            again, why = auth.check(a.authorization, spec["task_id"])
            if again is None or again["sha256"] != grant["sha256"]:
                stop = f"authorization no longer valid: {why or 'the grant file changed'}"; break
            s = sides[n]; need = 2 * s["source_bytes"]
            if grid.free_bytes(scratch) - need < spec["min_free_gib"] * GIB:
                stop = f"free space would fall below {spec['min_free_gib']} GiB"; break
            if grid.du_bytes(scratch) + need > spec["max_scratch_gib"] * GIB:
                stop = f"the scratch root would exceed {spec['max_scratch_gib']} GiB"; break
            # The process starts on `cpu`; the harness moves only its sampler thread
            # to `sampler_cpu`, so the compaction does not share a CPU with it.
            cmd = ["taskset", "-c", str(spec["cpu"]), s["harness"], "--metric", "compact",
                   "--source", s["source"], "--scratch-root", str(scratch), "--delete-batch", str(spec["delete_batch"]),
                   "--observe-compaction", "--sample-us", str(spec["sample_us"]),
                   "--sampler-cpu", str(spec["sampler_cpu"]), "--seed", str(r)]
            so, se, code, timed_out, secs = grid.run_sample(cmd, spec["timeout"], f"rep {r} {n}")
            stem = f"{n}-rep{r}"
            (out / "raw" / f"{stem}.json").write_text(so)
            (out / "raw" / f"{stem}.stderr").write_text(se)
            (out / "raw" / f"{stem}.cmd").write_text(shlex.join(cmd) + "\n")
            d, why = (None, "timed out") if timed_out else grid.parse_sample(so, expect_for(s, r, spec), stem)
            if d is not None and code != 0: d, why = None, f"exit {code}"
            if d is not None:
                why = observation_problem(d, s)
                if why: d = None
            _, work, token = grid.partial_fields(so)
            rec = {"repetition": r, "side": n, "exit": code, "seconds": secs, "raw_sha256": grid.sha256_text(so),
                   "work_dir": work, "token": token, "valid": d is not None, "unusable_because": why}
            if work:
                keep = d is None and grid.du_bytes(scratch) + need <= spec["max_scratch_gib"] * GIB
                rec["cleanup"] = "kept: the repetition failed" if keep else grid.safe_remove(work, scratch, token)
            records.append(rec)
            print(f"  rep {r} {n}: {'ok' if d else 'UNUSABLE: ' + str(why)} cleanup={rec.get('cleanup')}", flush=True)

    after = {}
    for n, s in sides.items():
        h, f, _ = grid.fixture_digest(Path(s["source"]))
        after[n] = {"unchanged": h == s["source_store_sha256"] and f == s["source_store_files"],
                    "store_sha256_after": h}
    report = {"config_sha256": grid.sha256_file(out / "config.json"), "records": records,
              "sources_after": after, "stopped": stop,
              "left_in_scratch": sorted(p.name for p in scratch.iterdir())}
    ok = not stop and all(x["valid"] for x in records) and all(v["unchanged"] for v in after.values()) \
        and len(records) == len(names) * spec["repetitions"]
    report["verdict"] = "complete" if ok else "INCOMPLETE - see records, stopped and sources_after"
    (out / "report.json").write_text(json.dumps(report, indent=1))
    print(f"  {report['verdict']}")
    sys.exit(0 if ok else 3)


if __name__ == "__main__":
    main()
