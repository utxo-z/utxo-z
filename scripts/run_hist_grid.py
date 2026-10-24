#!/usr/bin/env python3
"""A grid of paired comparisons, each side its own binary and its own fixture.

run_hist_pilot.py compares a binary with itself on one image. Comparing shard
counts needs more: side A is the N=1 baseline on the N=1 image, side B is an N>1
binary on the image that binary built, and the two have to be shown to have
asked for exactly the same lookups. This runner does that, for several
comparisons at once, interleaved so that no comparison owns a stretch of time.

Everything is declared in one spec file, hashed into every config, before the
first measurement:

* **sides** — named (binary, fixture) pairs. Each is hashed: the binary, the
  stored bytes of the fixture (lock excluded, see run_hist_pilot.LOCK_FILE), its
  shape.json and census.json. The fixture's shape gives its shard count,
  geometry and historical prefix, and each sample is checked against them.
* **cells** — one per comparison and metric: which side is A, which is B, and
  whether B does extra control work. Kinds: `aa` (same side twice), `control`
  (same side, B does `extra_pct_b`% more real lookups) and `ab` (N=1 against an
  N>1 side).
* **the probe span** — `probe_below`, the smallest historical prefix of every
  side used. Both sides of every pair draw from [0, probe_below) with the same
  seed, so they ask for the same keys, and no key is answered by an active
  version on either side. Each harness document carries a digest of its probe
  sequence and the two digests of a pair must be equal.

Order, fixed and recorded:

* rounds r = 1..pairs; in each round every cell runs its pair r, cells rotated
  by r-1 so no cell always goes first;
* inside a pair, blocks AB, BA, BA, AB by r mod 4 — so each seed parity sees
  both orders, which the first pilot's AB-odd/BA-even could not;
* both sides of pair r use seed r.

Times are what the harness measured, with no correction of any kind. Setup and
warmup are outside the harness's elapsed; nothing here claims anything about
which pages are resident.

Each cell's output is the report the family verifier already checks, and this
module exports the contract it asks for (expect_from_config, verify_report), so
recompute_aa_family.py --runner run_hist_grid.py verifies a grid per side.
"""
import argparse, hashlib, importlib.util, json, math, os, platform, shlex
import subprocess, sys, threading, time
from pathlib import Path

HERE = Path(__file__).resolve().parent


def _load_pilot():
    spec = importlib.util.spec_from_file_location("hist_pilot", HERE / "run_hist_pilot.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


pilot = _load_pilot()
# The one meaning of "a valid sample", shared with the single-image runner.
parse_sample = pilot.parse_sample
sha256_file = pilot.sha256_file
sha256_text = pilot.sha256_text
fixture_digest = pilot.fixture_digest

HEARTBEAT_SECONDS = 15
KINDS = ("aa", "control", "ab")
METRICS = ("hist_hit", "hist_miss")
# Written directly inside --out-dir by the runner itself; no cell may be named so.
RESERVED_NAMES = ("spec.json", "schedule.json")


def first_side(pair):
    """Blocks AB, BA, BA, AB: pair 1 AB, 2 BA, 3 BA, 4 AB, and again."""
    return "A" if pair % 4 in (1, 0) else "B"


def cell_order(cells, pair):
    """Every cell runs pair r in round r, rotated by r-1."""
    k = (pair - 1) % len(cells)
    return cells[k:] + cells[:k]


# ---------------------------------------------------------------------------
# The contract the family verifier uses when it re-reads a cell.

def expect_from_config(cfg, pair, side):
    s = cfg["sides"][side]
    return {"schema": pilot.SCHEMA_V2, "metric": cfg["metric"],
            "ops": cfg["ops_per_sample"], "warmup": cfg["warmup_per_sample"],
            "batch": cfg["batch"], "extra_pct": s["extra_pct"],
            "population_observed": cfg["population"],
            "shard_count": s["shard_count"], "geometry_id": s["geometry_id"],
            "historical_prefix": s["historical_prefix"],
            "probe_below": cfg["probe_below"], "seed": pair,
            "active_entries": s["active_entries"],
            "generations_class_48": s["generations_class_48"]}


def check_sample_record(d, rec, cfg):
    """The summary record of a sample has to say what its raw says.

    verify_report compares the two sides' probe digests in the records; this is
    what makes those records the raws' and not a summary's own claim.
    """
    for f in ("probe_digest", "extra_pct"):
        if rec.get(f) != d[f]:
            return f"the record says {f} {rec.get(f)!r} and the raw {d[f]!r}"
    return None


def side_shape_problem(name, s, population, probe_below):
    """None, or why side name's shape cannot give a usable cell. The preflight
    and verify_report share it, so a side the verifier would refuse is refused
    before any sample is taken."""
    for f in ("shard_count", "historical_prefix", "active_entries", "generations_class_48", "population"):
        if isinstance(s.get(f), bool) or not isinstance(s.get(f), int):
            return f"side {name}'s {f} is {s.get(f)!r}"
    # The side's shape has to be self-consistent: a history, below a prefix,
    # with the active entries above it.
    if not 0 < s["historical_prefix"] < s["population"]:
        return f"side {name}'s historical prefix {s['historical_prefix']} is not a history"
    if not 0 < s["active_entries"] <= s["population"] - s["historical_prefix"]:
        return (f"side {name} declares {s['active_entries']} active entries, which "
                f"cannot all sit at or above its prefix {s['historical_prefix']}")
    if s["generations_class_48"] < 2 * s["shard_count"]:
        return (f"side {name} has {s['generations_class_48']} generations for "
                f"{s['shard_count']} shards: not every shard has a history")
    if s["population"] != population:
        return f"side {name} holds {s['population']} entries, not {population}"
    if not 1 <= probe_below <= s["historical_prefix"]:
        return (f"probe_below {probe_below} exceeds side {name}'s historical "
                f"prefix {s['historical_prefix']}")
    return None


hash_or_none, after_hashes, provenance_problem = (
    pilot.hash_or_none, pilot.after_hashes, pilot.provenance_problem)


def verify_report(report, cfg):
    """Report-level facts beyond the samples. None, or the reason."""
    kind = cfg.get("run_kind")
    sides = cfg.get("sides")
    if kind not in KINDS:
        return f"run_kind is {kind!r}"
    if not isinstance(sides, dict) or set(sides) != {"A", "B"}:
        return "the config does not describe exactly two sides"
    a, b = sides["A"], sides["B"]
    for name, s in (("A", a), ("B", b)):
        for f in ("harness_sha256", "fixture_store_sha256", "fixture_store_files",
                  "shard_count", "geometry_id", "historical_prefix", "extra_pct",
                  "population", "active_entries", "generations_class_48"):
            if f not in s:
                return f"side {name} has no {f}"
        why = side_shape_problem(name, s, cfg["population"], cfg["probe_below"])
        if why:
            return why
    same = (a["harness_sha256"] == b["harness_sha256"]
            and a["fixture_store_sha256"] == b["fixture_store_sha256"])
    if a["extra_pct"] != 0:
        return f"side A declares {a['extra_pct']}% extra work; A is always the baseline"
    if kind == "aa":
        if not same or b["extra_pct"] != 0:
            return "an A/A cell must run the same binary on the same image, no extra work"
    elif kind == "control":
        if not same or not 0 < b["extra_pct"] <= 100:
            return "a control cell must run the same binary and image, B with extra work"
    else:
        if b["extra_pct"] != 0:
            return "a comparison cell must not carry control work"
        if a["shard_count"] != 1 or b["shard_count"] == 1:
            return (f"a comparison cell is N=1 against N>1, not "
                    f"N={a['shard_count']} against N={b['shard_count']}")

    by_pair = {}
    for rec in report.get("samples", []):
        side = rec.get("side")
        if side not in sides:
            return f"a sample has side {side!r}"
        if rec.get("extra_pct") != sides[side]["extra_pct"]:
            return (f"pair {rec.get('pair')}{side} records extra_pct "
                    f"{rec.get('extra_pct')!r}, not the {sides[side]['extra_pct']} its side was owed")
        if rec.get("ran_first") != first_side(rec.get("pair", 0)):
            return f"pair {rec.get('pair')} did not run in the declared block order"
        by_pair.setdefault(rec.get("pair"), {})[side] = rec.get("probe_digest")
    # Same work on both sides means the same probe sequence, digest for digest. A
    # control's B adds its extra batches to that sequence, so there the two
    # digests differ by construction and are not compared.
    for pr, d in by_pair.items():
        if d.get("A") is None or d.get("B") is None:
            return f"pair {pr}: a side has no probe digest"
        if kind != "control" and d["A"] != d["B"]:
            return f"pair {pr}: the two sides did not ask for the same lookups ({d})"

    image = report.get("image")
    if not isinstance(image, dict):
        return "the report has no image record"
    for name, s in (("A", a), ("B", b)):
        im = image.get(name)
        if not isinstance(im, dict) or im.get("unchanged") is not True:
            return f"side {name}'s store changed during the run"
        if im.get("store_sha256_before") != s["fixture_store_sha256"] \
                or im.get("store_sha256_after") != s["fixture_store_sha256"]:
            return f"side {name}'s store hashes are not the one the config names"
        if im.get("files_before") != s["fixture_store_files"] \
                or im.get("files_after") != s["fixture_store_files"]:
            return f"side {name}'s store file counts are not the config's"
    return provenance_problem(report.get("provenance_after"),
                              {"harness:A": a["harness_sha256"], "harness:B": b["harness_sha256"],
                               "harness_source": cfg.get("harness_source_sha256")})


# ---------------------------------------------------------------------------

def load_spec(path):
    """(spec, the exact text it was parsed from, problems): the text recorded as
    spec.json is the text that was validated, not a second read of the file."""
    text = Path(path).read_text()
    spec = json.loads(text, parse_constant=lambda c: (_ for _ in ()).throw(
        ValueError(f"non-numeric constant {c!r}")))
    problems = []
    def need_int(name, lo, hi=None):
        v = spec.get(name)
        if isinstance(v, bool) or not isinstance(v, int) or v < lo or (hi is not None and v > hi):
            problems.append(f"{name} is {v!r}")
    need_int("pairs", 1, 20)
    need_int("ops", 1); need_int("warmup", 0); need_int("batch", 1)
    need_int("value_size", 1); need_int("probe_below", 1); need_int("cpu", 0)
    need_int("resamples", 100); need_int("bootstrap_seed", 0)
    if not problems:
        if spec["ops"] % spec["batch"] or spec["warmup"] % spec["batch"]:
            problems.append("ops and warmup must be multiples of batch")
    t = spec.get("timeout")
    if isinstance(t, bool) or not isinstance(t, (int, float)) or not math.isfinite(t) or t <= 0:
        problems.append(f"timeout is {t!r}")
    c = spec.get("confidence")
    if isinstance(c, bool) or not isinstance(c, (int, float)) or not 0.5 < c < 1.0:
        problems.append(f"confidence is {c!r}")
    sides = spec.get("sides")
    cells = spec.get("cells")
    if not isinstance(sides, dict) or not sides:
        problems.append("no sides")
    if not isinstance(cells, list) or not cells:
        problems.append("no cells")
    else:
        names = [x.get("name") if isinstance(x, dict) else None for x in cells]
        # Each name becomes a directory directly inside --out-dir: one plain
        # component, not a file the runner writes there.
        for n in names:
            if not isinstance(n, str) or n in ("", ".", "..") or "/" in n or "\\" in n or "\0" in n \
                    or n in RESERVED_NAMES:
                problems.append(f"cell name {n!r} is not a plain directory name of its own")
        if len(set(map(str, names))) != len(names):
            problems.append("two cells share a name")
        for x in cells:
            if x.get("kind") not in KINDS: problems.append(f"cell {x.get('name')}: kind {x.get('kind')!r}")
            if x.get("metric") not in METRICS: problems.append(f"cell {x.get('name')}: metric {x.get('metric')!r}")
            for s in ("A", "B"):
                if not isinstance(sides, dict) or x.get(s) not in sides:
                    problems.append(f"cell {x.get('name')}: side {s} {x.get(s)!r} is not declared")
            e = x.get("extra_pct_b", 0)
            if isinstance(e, bool) or not isinstance(e, int) or not 0 <= e <= 100:
                problems.append(f"cell {x.get('name')}: extra_pct_b {e!r}")
            elif e and (spec.get("ops", 0) // max(spec.get("batch", 1), 1)) * e % 100:
                problems.append(f"cell {x.get('name')}: {e}% is not a whole number of batches")
            if x.get("kind") == "control" and not e:
                problems.append(f"cell {x.get('name')}: a control needs extra_pct_b > 0")
            if x.get("kind") in ("aa", "ab") and e:
                problems.append(f"cell {x.get('name')}: only a control carries extra work")
            if x.get("kind") in ("aa", "control") and x.get("A") != x.get("B"):
                problems.append(f"cell {x.get('name')}: {x.get('kind')} runs one side twice")
            if x.get("kind") == "ab" and x.get("A") == x.get("B"):
                problems.append(f"cell {x.get('name')}: a comparison needs two sides")
    return spec, text, problems


def describe_side(name, d):
    harness = Path(d["harness"]).resolve()
    fixture = Path(d["fixture"]).resolve()
    if not os.access(harness, os.X_OK):
        raise SystemExit(f"side {name}: {harness} is not executable")
    shape_path = fixture / "shape.json"
    census_path = fixture / "census.json"
    if not shape_path.is_file() or not census_path.is_file():
        raise SystemExit(f"side {name}: {fixture} has no shape.json/census.json")
    shape = json.loads(shape_path.read_text())
    if shape.get("schema") != "utxoz-hist-shape-2":
        raise SystemExit(f"side {name}: shape schema {shape.get('schema')!r}")
    print(f"  hashing side {name}'s fixture...", flush=True)
    store, files, lock = fixture_digest(fixture)
    return {"name": name, "harness": str(harness), "harness_sha256": sha256_file(harness),
            "fixture": str(fixture), "fixture_store_sha256": store,
            "fixture_store_files": files, "fixture_lock_sha256_before": lock,
            "shape_sha256": sha256_file(shape_path), "census_sha256": sha256_file(census_path),
            "shard_count": shape["shard_count"], "geometry_id": shape["geometry_id"],
            "historical_prefix": shape["historical_prefix"],
            "active_entries": shape["active_entries"],
            "generations_class_48": shape["generations_class_48"],
            "population": shape["population"]}


def run_sample(cmd, timeout, label):
    t0 = time.time()
    print(f"  {label} START {time.strftime('%H:%M:%S')}", flush=True)
    stop = threading.Event()
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    def beat(pid=proc.pid):
        while not stop.wait(HEARTBEAT_SECONDS):
            print(f"      alive pid={pid} {time.time()-t0:.0f}s (liveness only)", flush=True)

    threading.Thread(target=beat, daemon=True).start()
    timed_out = False
    try:
        so, se = proc.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        proc.kill(); so, se = proc.communicate(); timed_out = True
    stop.set()
    return so, se, proc.returncode, timed_out, time.time() - t0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--spec", required=True)
    ap.add_argument("--out-dir", required=True, help="must not exist")
    a = ap.parse_args()

    spec, spec_text, problems = load_spec(a.spec)
    if problems:
        for p in problems: print(f"  {p}")
        sys.exit(2)
    out = Path(a.out_dir)
    if out.exists():
        print(f"  {out} already exists; this run will not overwrite or extend it"); sys.exit(2)

    stats_path = Path(spec["stats_module"]).resolve()
    sspec = importlib.util.spec_from_file_location("grid_stats", stats_path)
    stats = importlib.util.module_from_spec(sspec); sspec.loader.exec_module(stats)
    if not hasattr(stats, "bootstrap_median_ci"):
        print(f"  {stats_path} has no bootstrap_median_ci"); sys.exit(2)
    source = Path(spec["harness_source"]).resolve()
    if not source.is_file():
        print(f"  {source} is not a file"); sys.exit(2)

    used = sorted({x[s] for x in spec["cells"] for s in ("A", "B")})
    sides = {n: describe_side(n, spec["sides"][n]) for n in used}
    population = {s["population"] for s in sides.values()}
    if len(population) != 1:
        print(f"  the sides hold different populations: {population}"); sys.exit(2)
    population = population.pop()
    for x in spec["cells"]:
        if x["kind"] == "ab" and (sides[x["A"]]["shard_count"] != 1
                                  or sides[x["B"]]["shard_count"] == 1):
            print(f"  cell {x['name']}: A must be the N=1 baseline and B an N>1 side"); sys.exit(2)
    for s in sides.values():
        why = side_shape_problem(s["name"], s, population, spec["probe_below"])
        if why:
            print(f"  {why}"); sys.exit(2)

    # Hashed before anything exists, and again after the last sample.
    source_sha = sha256_file(source)
    out.mkdir(parents=True)
    (out / "spec.json").write_text(spec_text)
    runner_sha = sha256_text(sha256_file(__file__) + sha256_file(HERE / "run_hist_pilot.py"))
    started = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())

    cells, configs = spec["cells"], {}
    for x in cells:
        cdir = out / x["name"]; (cdir / "raw").mkdir(parents=True)
        cs = {}
        for side in ("A", "B"):
            s = dict(sides[x[side]])
            s["extra_pct"] = x.get("extra_pct_b", 0) if side == "B" else 0
            cs[side] = s
        cfg = {
            "schema": "utxoz-aa-pilot-2",
            "kind": {"aa": "A/A - the same side twice",
                     "control": f"control - B does {x.get('extra_pct_b', 0)}% more real "
                                "lookups, timed, outside the denominator",
                     "ab": f"A = {x['A']}, B = {x['B']}: N={cs['A']['shard_count']} "
                           f"against N={cs['B']['shard_count']}"}[x["kind"]],
            "run_kind": x["kind"], "cell": x["name"], "metric": x["metric"],
            "pairs": spec["pairs"], "ops_per_sample": spec["ops"],
            "warmup_per_sample": spec["warmup"], "batch": spec["batch"],
            "value_size": spec["value_size"], "probe_below": spec["probe_below"],
            "population": population,
            "shard_count_expected": {k: v["shard_count"] for k, v in cs.items()},
            "sides": cs,
            "order": ("rounds r=1..pairs, cells rotated by r-1 within a round; inside a "
                      "pair blocks AB,BA,BA,AB by r mod 4"),
            "seed_rule": "both sides of pair r use seed r",
            "cache": ("warm per process: the warmup draws its own reproducible stream, "
                      "independent of the measured one (not disjoint from it). No claim is "
                      "made about page-cache residency."),
            "denominator": "elapsed / base ops, as measured; no time is corrected",
            "cpu_affinity": spec["cpu"], "timeout_seconds_per_sample": spec["timeout"],
            "estimator": {"module": str(stats_path), "module_sha256": sha256_file(stats_path),
                          "function": "bootstrap_median_ci",
                          "description": "percentile bootstrap of the median of paired ratios B/A",
                          "resamples": spec["resamples"], "confidence": spec["confidence"],
                          "seed": spec["bootstrap_seed"]},
            "harness": f"A: {cs['A']['harness']} | B: {cs['B']['harness']}",
            "harness_sha256": sha256_text(cs["A"]["harness_sha256"] + cs["B"]["harness_sha256"]),
            "harness_source_sha256": source_sha,
            "runner_sha256": runner_sha, "spec_sha256": sha256_text(spec_text),
            "host": platform.node(), "kernel": platform.release(), "started_utc": started,
        }
        cfg["config_sha256"] = sha256_text(json.dumps(cfg, sort_keys=True))
        (cdir / "config.json").write_text(json.dumps(cfg, indent=1))
        configs[x["name"]] = cfg

    plan = [{"round": r, "cell": x["name"], "first": first_side(r)}
            for r in range(1, spec["pairs"] + 1) for x in cell_order(cells, r)]
    (out / "schedule.json").write_text(json.dumps(plan, indent=1))
    print(f"  spec_sha256={sha256_text(spec_text)}  runner_sha256={runner_sha}")
    print(f"  {len(cells)} cells, {spec['pairs']} pairs each, {2*len(plan)} samples; "
          f"configs and schedule written before measuring", flush=True)

    results = {x["name"]: {"samples": [], "failures": [], "A": {}, "B": {}} for x in cells}
    position = 0
    for step in plan:
        x = next(c for c in cells if c["name"] == step["cell"])
        cfg, r, res = configs[x["name"]], step["round"], results[x["name"]]
        order = ("A", "B") if step["first"] == "A" else ("B", "A")
        docs = {}
        for side in order:
            position += 1
            s = cfg["sides"][side]
            cmd = ["taskset", "-c", str(spec["cpu"]), s["harness"],
                   "--metric", cfg["metric"], "--fixture", s["fixture"],
                   "--ops", str(spec["ops"]), "--warmup", str(spec["warmup"]),
                   "--batch", str(spec["batch"]), "--extra-pct", str(s["extra_pct"]),
                   "--probe-below", str(spec["probe_below"]),
                   "--value-size", str(spec["value_size"]), "--seed", str(r)]
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
            rec = {"pair": r, "side": side, "exit": code, "timed_out": timed_out,
                   "extra_pct": s["extra_pct"], "ran_first": step["first"],
                   "position": position, "command": shlex.join(cmd),
                   "raw_sha256": sha256_text(so), "stderr": se, "seconds": secs,
                   "ns_per_op": d["ns_per_op"] if d else None,
                   "probe_digest": d["probe_digest"] if d else None,
                   "unusable_because": why}
            res["samples"].append(rec); docs[side] = rec
            print(f"  [{position}/{2*len(plan)}] END {secs:.1f}s exit={code} "
                  f"{'ns/op=%.3f' % d['ns_per_op'] if d else 'UNUSABLE: ' + why}", flush=True)
        # The pair is a comparison only if both sides asked for the same lookups.
        # A control's B adds extra batches on purpose, so its digest differs.
        if x["kind"] != "control" and all(docs[s]["unusable_because"] is None for s in "AB") \
                and docs["A"]["probe_digest"] != docs["B"]["probe_digest"]:
            for s in "AB":
                docs[s]["unusable_because"] = "the two sides' probe digests differ"
        for s in "AB":
            if docs[s]["unusable_because"] is None:
                res[s][r] = docs[s]["ns_per_op"]
            else:
                res["failures"].append(docs[s])

    after = {}
    for n, s in sides.items():
        h, f, lock = fixture_digest(Path(s["fixture"]))
        after[n] = {"store_sha256_before": s["fixture_store_sha256"], "store_sha256_after": h,
                    "files_before": s["fixture_store_files"], "files_after": f,
                    "lock_sha256_before": s["fixture_lock_sha256_before"],
                    "lock_sha256_after": lock,
                    "unchanged": h == s["fixture_store_sha256"] and f == s["fixture_store_files"]}
    # The binaries and the source that ran, hashed again: a rebuild during the run
    # means some samples came from another binary. None if gone or unreadable.
    binaries = {n: after_hashes({"h": (s["harness"], s["harness_sha256"])})["h"] for n, s in sides.items()}
    source_after = after_hashes({"h": (source, source_sha)})["h"]

    worst = 0
    for x in cells:
        cfg, res = configs[x["name"]], results[x["name"]]
        image = {side: after[x[side]] for side in ("A", "B")}
        provenance = {"harness:A": binaries[x["A"]], "harness:B": binaries[x["B"]],
                      "harness_source": source_after}
        A = [res["A"].get(r) for r in range(1, spec["pairs"] + 1)]
        B = [res["B"].get(r) for r in range(1, spec["pairs"] + 1)]
        report = {"config": cfg, "samples": res["samples"], "failures": res["failures"],
                  "ns_per_op": {"A": [v for v in A if v is not None],
                                "B": [v for v in B if v is not None]},
                  "image": image, "provenance_after": provenance}
        if not all(im["unchanged"] for im in image.values()):
            report["verdict"] = "NO MEASUREMENT - a store changed during the run"
        elif provenance_problem(provenance) is not None:
            report["verdict"] = f"NO MEASUREMENT - {provenance_problem(provenance)}"
        elif res["failures"] or None in A or None in B:
            report["verdict"] = "NO MEASUREMENT - at least one sample was not usable"
        else:
            ratios = [b / v for v, b in zip(A, B)]
            med, lo, hi = stats.bootstrap_median_ci(ratios, spec["resamples"],
                                                    spec["confidence"], spec["bootstrap_seed"])
            report.update({"ratios": ratios, "median_ratio": med, "ci": {"low": lo, "high": hi},
                           "median_pct": (med - 1) * 100,
                           "ci_pct": {"low": (lo - 1) * 100, "high": (hi - 1) * 100},
                           "half_width_pct": (hi - lo) * 50, "upper_bound_pct": (hi - 1) * 100})
        (out / x["name"] / "report.json").write_text(json.dumps(report, indent=1))
        if "verdict" in report:
            worst = 3
            print(f"  {x['name']}: {report['verdict']}")
            for f in res["failures"][:3]:
                print(f"    pair {f['pair']}{f['side']} exit={f['exit']} {f['unusable_because']}")
        else:
            print(f"  {x['name']}: median {report['median_pct']:+.3f}%  CI"
                  f"{int(spec['confidence']*100)} [{report['ci_pct']['low']:+.3f}%, "
                  f"{report['ci_pct']['high']:+.3f}%]")
    sys.exit(worst)


if __name__ == "__main__":
    main()
