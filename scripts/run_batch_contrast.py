#!/usr/bin/env python3
"""A short multithread contrast of insert_batch: N1W1 and N8 W1/2/4/8. Exploratory.

Every sample runs utxoz_calib_mut on a private copy of its side's immutable
source, exactly as run_mut_grid does, and inserts the same K keys, payloads and
heights - one image, one rule, so every sample must end with the same content.
A cell is a side, a metric and a worker count:

  insert_batch      full_db::insert_batch, --batch-size requests per call,
                    --workers W, the library's own threads
  insert_lifecycle  insert() one by one: a diagnostic of what the batch API
                    costs at W1, not a baseline for the workers. The baseline
                    for W>1 is W1 through the same API.

Each cell runs on a declared CPU set of W physical cores. The sets are checked
against the machine's topology before anything is created: every CPU online,
one CPU per physical core (no SMT sibling), and every set on the same NUMA node
and the same L3. The harness reports the affinity it actually had, and a sample
whose affinity is not its cell's set is not a measurement.

Participation is read from what happened, never from the W requested: the
tasks each call ran and how many ran on the calling thread, and the entries
every slot gained. The harness also reports the CPU time of the process beside
its wall time; above it says threads ran at once, not which ones. Nothing here
proves that the workers overlapped.

No statistics. Three repetitions per cell are a diagnostic: the report lists
each sample's elapsed time and nothing derived from them. It does not choose N.

Every sample is judged by one function, validate_sample(), against what the
config, the cell and the repetition say it must be. The same function judges
the raw files again later: `--verify OUT_DIR` re-reads config.json, checks the
config's own hash, its spec against spec.json, the schedule against the plan
the spec implies, and the hashes of the modules and binaries that ran against
the files here; re-parses every raw output, and recomputes the verdict. It
exits 0 only for a run that is both consistent and complete. A sample is
never repaired after the fact: if its raw output does not satisfy the
contract, it is invalid.

Reused from run_mut_grid.py, unchanged: the external authorization (re-read
before every sample), a scratch root of its own, removal of a work copy only
when it provably is one this run made, the space budgets, and hashes of every
source before and after.

Spec:
  task_id, scratch_parent, min_free_gib (>=10), max_scratch_gib (<=4),
  repetitions (1..3), inserts, batch_size, pause_every, timeout, harness_source,
  sides: {name: {harness, source}},
  cpu_sets: {"W": [cpus]},
  cells: [{name, side, metric, workers}]
Order: repetition r runs every cell, the cells rotated by r-1.
Exit: 0 complete, 2 refused before anything ran, 3 incomplete or not verified.
"""
import argparse, hashlib, importlib.util, json, math, os, platform, shlex, sys, time
from pathlib import Path

HERE = Path(__file__).resolve().parent
spec_ = importlib.util.spec_from_file_location("mut_grid", HERE / "run_mut_grid.py")
grid = importlib.util.module_from_spec(spec_); spec_.loader.exec_module(grid)
auth, GIB = grid.auth, grid.GIB
SCHEMA = grid.SCHEMA
CONFIG_SCHEMA = "utxoz-batch-contrast-2"
METRICS = ("insert_batch", "insert_lifecycle")
CLASSES = 5                     # container classes; every one but class 0 has one slot
VALUE_SIZE = 33                 # the payload of the grid images and of their inserts
MODULES = ("run_batch_contrast.py", "run_mut_grid.py", "run_hist_grid.py", "run_hist_pilot.py",
           "campaign_auth.py")
SYS_CPU = Path("/sys/devices/system/cpu")
SYS_NODE = Path("/sys/devices/system/node")


# ------------------------------------------------------------------ types

def is_int(v):
    return isinstance(v, int) and not isinstance(v, bool)


def is_hex16(v):
    return isinstance(v, str) and len(v) == 16 and all(c in "0123456789abcdef" for c in v)


def parse_raw(text):
    """The harness document, or (None, why). NaN and Infinity are refused, not read."""
    if not text.strip():
        return None, "no output"
    def no_constants(c):
        raise ValueError(f"non-numeric constant {c}")
    try:
        d = json.loads(text, parse_constant=no_constants)
    except (json.JSONDecodeError, ValueError) as e:
        return None, f"output is not usable JSON ({e})"
    if not isinstance(d, dict):
        return None, "output is not an object"
    return d, None


# ------------------------------------------------------------------ topology

def cpu_facts(cpu, sys_cpu=SYS_CPU, sys_node=SYS_NODE):
    """What the kernel says about one CPU, or raises if it cannot say."""
    base = sys_cpu / f"cpu{cpu}"
    online = base / "online"
    if online.exists() and online.read_text().strip() != "1":
        raise ValueError(f"cpu {cpu} is offline")
    topo = base / "topology"
    core = (int((topo / "physical_package_id").read_text()), int((topo / "core_id").read_text()))
    l3 = None
    for idx in sorted((base / "cache").glob("index*")):
        if (idx / "level").read_text().strip() == "3":
            l3 = (idx / "id").read_text().strip()
    if l3 is None:
        raise ValueError(f"cpu {cpu} reports no L3")
    node = None
    for n in sorted(sys_node.glob("node[0-9]*")):
        if cpu in parse_list((n / "cpulist").read_text()):
            node = n.name
    if node is None:
        raise ValueError(f"cpu {cpu} is on no NUMA node the kernel lists")
    return {"cpu": cpu, "core": list(core), "l3": l3, "node": node}


def parse_list(text):
    out = set()
    for part in text.strip().split(","):
        if not part: continue
        lo, _, hi = part.partition("-")
        out.update(range(int(lo), int(hi or lo) + 1))
    return out


def check_topology(cpu_sets, sys_cpu=SYS_CPU, sys_node=SYS_NODE):
    """(facts, None) if every set is W physical cores on one L3 and one node, else (None, why)."""
    facts, l3s, nodes = {}, set(), set()
    for w, cpus in cpu_sets.items():
        if not isinstance(cpus, list) or not all(is_int(c) and c >= 0 for c in cpus) \
                or not w.isdigit() or len(cpus) != int(w) or len(set(cpus)) != len(cpus):
            return None, f"cpu set {w} is {cpus!r}, not {w} distinct CPUs"
        if cpus != sorted(cpus):
            # The harness reports its mask in ascending order and the sample is
            # compared with this list as written: refuse it now, not after the run.
            return None, f"cpu set {w} is {cpus!r}, not in ascending order"
        try:
            fs = [cpu_facts(c, sys_cpu, sys_node) for c in cpus]
        except (OSError, ValueError) as e:
            return None, f"cpu set {w}: {e}"
        cores = [tuple(f["core"]) for f in fs]
        if len(set(cores)) != len(cores):
            return None, f"cpu set {w} puts two CPUs on one physical core (SMT): {cpus}"
        l3s.update(f["l3"] for f in fs); nodes.update(f["node"] for f in fs)
        facts[w] = fs
    if len(l3s) != 1:
        return None, f"the cpu sets span L3 domains {sorted(l3s)}; keep them on one"
    if len(nodes) != 1:
        return None, f"the cpu sets span NUMA nodes {sorted(nodes)}; keep them on one"
    return facts, None


# ------------------------------------------------------------------ one sample

def expected_for(config, cell_name, rep):
    """What a sample of this cell, in this repetition, must report."""
    spec = config["spec"]
    cell = next(c for c in spec["cells"] if c["name"] == cell_name)
    side = config["sides"][cell["side"]]
    batch = cell["metric"] == "insert_batch"
    return {"cell": cell_name, "repetition": rep, "metric": cell["metric"], "workers": cell["workers"],
            "cpus": spec["cpu_sets"][str(cell["workers"])], "shard_count": side["shard_count"],
            "geometry_id": side["geometry_id"], "population": side["population"],
            "value_size": side["value_size"], "source": side["source"], "inserts": spec["inserts"],
            "pause_every": spec["pause_every"], "seed": rep,
            "batch_size": spec["batch_size"] if batch else 0,
            "calls": -(-spec["inserts"] // spec["batch_size"]) if batch else 0,
            "batches": -(-spec["inserts"] // spec["batch_size"]) if batch
                       else spec["inserts"] // spec["pause_every"]}


def validate_sample(d, e):
    """None if the harness document d is a valid sample with expectation e, else why."""
    if d.get("schema") != SCHEMA:
        return f"schema {d.get('schema')!r}"
    for f in ("metric", "shard_count", "geometry_id", "population", "value_size", "source", "inserts",
              "pause_every", "seed", "batches"):
        if d.get(f) != e[f] or isinstance(d.get(f), bool):
            return f"{f} is {d.get(f)!r}, not {e[f]!r}"
    if d.get("valid") is not True:
        return f"the harness reported the operation invalid: {d.get('invalid_because')!r}"
    for f in ("initial_digest", "final_digest", "expected_digest", "token"):
        if not is_hex16(d.get(f)):
            return f"{f} is {d.get(f)!r}, not 16 hex digits"
    if d["final_digest"] != d["expected_digest"]:
        return "the final content is not what the rule predicts"
    if not isinstance(d.get("work_dir"), str) or not d["work_dir"]:
        return "work_dir is missing"
    if not is_int(d.get("elapsed_ns")) or d["elapsed_ns"] <= 0:
        return f"elapsed_ns is {d.get('elapsed_ns')!r}, not a positive integer"
    for f in ("failed_ops", "setup_deletions"):
        if d.get(f) != 0 or not is_int(d.get(f)):
            return f"{f} is {d.get(f)!r}, not 0"
    if not is_int(d.get("entries_after")) or d["entries_after"] != e["population"] + e["inserts"]:
        return f"entries_after is {d.get('entries_after')!r}"
    if not is_int(d.get("rotations_class48")) or d["rotations_class48"] < 0:
        return f"rotations_class48 is {d.get('rotations_class48')!r}"

    sp = d.get("spread")
    if not isinstance(sp, dict):
        return "no spread record"
    if sp.get("affinity_cpus") != e["cpus"] or not all(is_int(c) for c in sp["affinity_cpus"]):
        return f"the harness ran on CPUs {sp.get('affinity_cpus')!r}, not the cell's {e['cpus']}"
    if not is_int(sp.get("cpu_ns_over_timed_region")) or sp["cpu_ns_over_timed_region"] < 0:
        return f"cpu_ns_over_timed_region is {sp.get('cpu_ns_over_timed_region')!r}"
    if sp.get("inserts_counted_by_statistics") != e["inserts"] \
            or not is_int(sp.get("inserts_counted_by_statistics")):
        return "the statistics do not count every insert"

    slots = sp.get("slots")
    if not isinstance(slots, list) or len(slots) != e["shard_count"] + CLASSES - 1:
        return f"{len(slots) if isinstance(slots, list) else slots!r} slots, not {e['shard_count'] + CLASSES - 1}"
    ids = set()
    for x in slots:
        if not isinstance(x, dict) or not all(is_int(x.get(f)) and x[f] >= 0 for f in ("class", "shard", "gained")):
            return f"slot {x!r} is not three non-negative integers"
        ids.add((x["class"], x["shard"]))
        if x["class"] != 0 and x["gained"] != 0:
            return f"class {x['class']} gained {x['gained']} entries; the inserts are all class 0"
    want_ids = {(0, s) for s in range(e["shard_count"])} | {(c, 0) for c in range(1, CLASSES)}
    if ids != want_ids:
        return f"the slots are {sorted(ids)}, not one per (class, shard) of N={e['shard_count']}"
    if sum(x["gained"] for x in slots) != e["inserts"]:
        return f"the slots gained {sum(x['gained'] for x in slots)} entries, not {e['inserts']}"
    used = sum(1 for x in slots if x["gained"])

    counters = ("workers_requested", "batch_size", "calls", "tasks", "tasks_on_calling_thread",
                "workers_used_min", "workers_used_max", "calls_short_of_threads", "inserted",
                "duplicates", "failed", "not_applied", "calls_with_error")
    for f in counters:
        if not is_int(sp.get(f)) or sp[f] < 0:
            return f"spread.{f} is {sp.get(f)!r}, not a non-negative integer"
    if e["metric"] == "insert_batch":
        # Every call carries batch_size requests spread over every used slot, so
        # every call can use min(W, used) workers - and with the library's own
        # threads it runs exactly one task on the calling thread.
        w = min(e["workers"], used)
        want = {"workers_requested": e["workers"], "batch_size": e["batch_size"], "calls": e["calls"],
                "tasks": e["calls"] * w, "tasks_on_calling_thread": e["calls"],
                "workers_used_min": w, "workers_used_max": w, "calls_short_of_threads": 0,
                "inserted": e["inserts"], "duplicates": 0, "failed": 0, "not_applied": 0,
                "calls_with_error": 0}
    else:
        want = {f: 0 for f in counters}
    for f, v in want.items():
        if sp[f] != v:
            return f"spread.{f} is {sp[f]}, not {v}"
    return None


def cross_problems(records, sides):
    """One image, one rule: one initial and one final content, and per side one rotation count."""
    out = []
    valid = [x for x in records if x["valid"]]
    for f in ("initial_digest", "final_digest"):
        vals = {x[f] for x in valid}
        if len(vals) > 1:
            out.append(f"the samples disagree on {f}: {sorted(map(str, vals))}")
    for n in sides:
        rots = {x["rotations_class48"] for x in valid if x["side"] == n}
        if len(rots) > 1:
            out.append(f"side {n}'s samples rotated {sorted(rots)} times")
    return out


def record_of(d, e, cell, so, code, timed_out, timeout):
    """The record of one sample, judged from its raw output only."""
    if timed_out: why = f"timed out after {timeout}s"
    elif d is None: why = "no usable JSON"
    elif code != 0: why = f"exit {code}: {d.get('invalid_because')!r}"
    else: why = validate_sample(d, e)
    d = d or {}
    sp = d.get("spread") if isinstance(d.get("spread"), dict) else {}
    # Whatever the document holds, the record is built and the run goes on:
    # the wrong shape makes the sample invalid above, never a traceback here,
    # and a work_dir or token that is not a string is never handed to cleanup.
    slots = sp.get("slots") if isinstance(sp.get("slots"), list) else []
    work = d.get("work_dir") if isinstance(d.get("work_dir"), str) and d.get("work_dir") else None
    token = d.get("token") if isinstance(d.get("token"), str) else None
    return {"repetition": e["repetition"], "cell": cell["name"], "side": cell["side"],
            "metric": cell["metric"], "workers": cell["workers"], "cpus": e["cpus"], "exit": code,
            "timed_out": timed_out, "raw_sha256": grid.sha256_text(so),
            "elapsed_ns": d.get("elapsed_ns"), "cpu_ns": sp.get("cpu_ns_over_timed_region"),
            "initial_digest": d.get("initial_digest"), "final_digest": d.get("final_digest"),
            "rotations_class48": d.get("rotations_class48"),
            "workers_used_max": sp.get("workers_used_max"), "tasks": sp.get("tasks"),
            "tasks_on_calling_thread": sp.get("tasks_on_calling_thread"),
            "slots_gained": [x.get("gained") for x in slots if isinstance(x, dict)],
            "work_dir": work, "token": token,
            "valid": why is None, "unusable_because": why}


def per_cell(records, cells):
    return {c["name"]: {"side": c["side"], "metric": c["metric"], "workers": c["workers"],
                        "elapsed_ns": [x["elapsed_ns"] for x in records if x["cell"] == c["name"] and x["valid"]],
                        "cpu_ns": [x["cpu_ns"] for x in records if x["cell"] == c["name"] and x["valid"]]}
            for c in cells}


def plan_of(spec):
    """The canonical schedule: repetition r runs every cell, the cells rotated by r-1."""
    cells = [c["name"] for c in spec["cells"]]
    return [{"repetition": r, "order": cells[(r - 1) % len(cells):] + cells[:(r - 1) % len(cells)]}
            for r in range(1, spec["repetitions"] + 1)]


def config_identity(config):
    body = {k: v for k, v in config.items() if k != "config_sha256"}
    return hashlib.sha256(json.dumps(body, sort_keys=True).encode()).hexdigest()


def module_hashes():
    return {m: grid.sha256_file(HERE / m) for m in MODULES}


# ------------------------------------------------------------------ verification

def verify_run(out_dir):
    """Re-judges a run from its files. Returns (problems, complete).

    What it compares is the preserved evidence: the config against its own hash
    and against spec.json, each raw output against the hash recorded for it and
    against the contract, the report against what the raws and the config say.
    Of the files on disk it rehashes only the runner's own modules; the harness
    binaries and source are judged from the hashes the run recorded before its
    first sample and after its last. It does not rehash
    the data sources (gigabytes): their after-hashes are the run's own record,
    checked against the before-hashes in the config.

    A run is certified only if there are no problems AND it is complete: every
    (repetition, cell) of the canonical plan exactly once, all valid, no
    cross-sample problem, not stopped, and every declared source unchanged.
    """
    out = Path(out_dir)
    problems = []
    config = json.loads((out / "config.json").read_text())
    report = json.loads((out / "report.json").read_text())
    if config.get("schema") != CONFIG_SCHEMA:
        return [f"config schema {config.get('schema')!r}"], False
    if config_identity(config) != config.get("config_sha256"):
        problems.append("config.json does not match its own hash")
    if report.get("config_sha256") != config.get("config_sha256"):
        problems.append("the report names another config")
    for m, h in module_hashes().items():
        if config.get("modules", {}).get(m) != h:
            problems.append(f"{m} here is not the {m} that ran")
    spec = config["spec"]
    # The harness binaries and source are judged from the run's own record of
    # them (provenance_after, below), not from whatever the files are now.
    spec_text = (out / "spec.json").read_text()
    if grid.sha256_text(spec_text) != config.get("spec_sha256"):
        problems.append("spec.json is not the spec that ran")
    if json.loads(spec_text) != spec:
        problems.append("the config's spec is not spec.json")
    sides = config.get("sides", {})
    if set(sides) != {c["side"] for c in spec["cells"]}:
        problems.append(f"the config describes sides {sorted(sides)}, not the cells' sides")

    # The plan, derived from the spec; the schedule on disk has to be exactly it.
    plan = plan_of(spec)
    if json.loads((out / "schedule.json").read_text()) != plan:
        problems.append("schedule.json is not the canonical plan of the spec")
    planned = [(s["repetition"], name) for s in plan for name in s["order"]]

    cells = {c["name"]: c for c in spec["cells"]}
    records = []
    for rec in report.get("records", []):
        if (rec.get("repetition"), rec.get("cell")) not in planned:
            problems.append(f"a record for {rec.get('cell')!r} rep {rec.get('repetition')!r} is not in the plan")
            continue
        stem = f"{rec['cell']}-rep{rec['repetition']}"
        try:
            so = (out / "raw" / f"{stem}.json").read_text()
        except OSError as e:
            problems.append(f"{stem}: the raw output is missing or unreadable ({e.strerror})")
            continue
        if grid.sha256_text(so) != rec.get("raw_sha256"):
            problems.append(f"{stem}: the raw output is not the one recorded")
        d, _ = parse_raw(so)
        e = expected_for(config, rec["cell"], rec["repetition"])
        again = record_of(d, e, cells[rec["cell"]], so, rec["exit"], rec["timed_out"], spec["timeout"])
        for f in ("valid", "elapsed_ns", "cpu_ns", "initial_digest", "final_digest", "rotations_class48"):
            if again[f] != rec.get(f):
                problems.append(f"{stem}: {f} recomputes to {again[f]!r}, the report says {rec.get(f)!r}")
        records.append(again)
    pairs = [(x["repetition"], x["cell"]) for x in records]
    if len(set(pairs)) != len(pairs):
        problems.append("a (repetition, cell) is recorded twice")
    if pairs != planned[:len(pairs)]:
        problems.append("the records are not in the order of the plan")
    if report.get("cross_sample_problems") != cross_problems(records, sides):
        problems.append("the cross-sample problems do not recompute")
    if report.get("per_cell") != per_cell(records, spec["cells"]):
        problems.append("the per-cell values do not recompute")

    after = report.get("sources_after")
    sources_ok = isinstance(after, dict) and set(after) == set(sides)
    if not sources_ok:
        problems.append(f"sources_after names {sorted(after) if isinstance(after, dict) else after!r}, "
                        f"not the sides {sorted(sides)}")
    else:
        for n, a in after.items():
            if not isinstance(a, dict):
                problems.append(f"side {n}: sources_after is {a!r}"); sources_ok = False; continue
            same = a.get("store_sha256_after") == sides[n].get("source_store_sha256")
            if a.get("unchanged") is not same:
                problems.append(f"side {n}: unchanged is {a.get('unchanged')!r} but the hashes say {same}")
            sources_ok = sources_ok and same

    # What ran, from the record alone: it has to name exactly the config's
    # binaries and source with the config's hashes (else it is not this run's
    # record - or predates it - and nothing is certified); a consistent record
    # of a file that changed or vanished during the run makes the run incomplete.
    prov = report.get("provenance_after")
    expected = {**{f"harness:{n}": sd.get("harness_sha256") for n, sd in sides.items()},
                "harness_source": config.get("harness_source_sha256")}
    if not isinstance(prov, dict) or set(prov) != set(expected) \
            or any(not isinstance(prov[k], dict) or prov[k].get("sha256_before") != v for k, v in expected.items()):
        problems.append(grid.provenance_problem(prov, expected))
        provenance_ok = False
    else:
        provenance_ok = grid.provenance_problem(prov) is None

    complete = (report.get("stopped") is None and pairs == planned and all(x["valid"] for x in records)
                and not cross_problems(records, sides) and sources_ok and provenance_ok)
    if (report.get("verdict") == "complete") != complete:
        problems.append(f"the verdict {report.get('verdict')!r} does not recompute")
    return problems, complete


# ------------------------------------------------------------------ the run

def load_spec(path):
    """(spec, the exact text it was parsed from, problems): the text recorded as
    spec.json is the text that was validated, not a second read of the file."""
    text = Path(path).read_text()
    spec = json.loads(text)
    problems = []
    for name, lo, hi in (("repetitions", 1, 3), ("inserts", 1, None), ("batch_size", 1, None),
                         ("pause_every", 1, None), ("min_free_gib", 10, None), ("max_scratch_gib", 1, 4)):
        v = spec.get(name)
        if not is_int(v) or v < lo or (hi is not None and v > hi):
            problems.append(f"{name} is {v!r}")
    # Every insert_batch call must be full: validate_sample requires the same
    # workers in every call, and a short last call can reach fewer slots.
    # Checked only once both are positive integers (refused above otherwise).
    ins, bs = spec.get("inserts"), spec.get("batch_size")
    if is_int(ins) and is_int(bs) and ins >= 1 and bs >= 1 and ins % bs:
        problems.append(f"inserts {ins} is not a multiple of batch_size {bs}: the last call would be short")
    t = spec.get("timeout")
    if not isinstance(t, (int, float)) or isinstance(t, bool) or not math.isfinite(t) or not t > 0:
        problems.append(f"timeout is {t!r}")
    if not isinstance(spec.get("task_id"), str) or not spec["task_id"].strip():
        problems.append("task_id is missing")
    if not isinstance(spec.get("harness_source"), str) or not Path(spec["harness_source"]).is_file():
        problems.append("harness_source is not a file")
    sides, sets, cells = spec.get("sides"), spec.get("cpu_sets"), spec.get("cells")
    if not isinstance(sides, dict) or not sides: problems.append("no sides")
    if not isinstance(sets, dict) or not sets: problems.append("no cpu_sets")
    if not isinstance(cells, list) or not cells: problems.append("no cells")
    else:
        names = [c.get("name") if isinstance(c, dict) else None for c in cells]
        if len(set(names)) != len(names) or not all(isinstance(n, str) and n for n in names):
            problems.append("cell names are missing or repeated")
        for c in cells:
            if not isinstance(c, dict): problems.append("a cell is not an object"); continue
            w = c.get("workers")
            if not is_int(w) or w < 1: problems.append(f"cell {c.get('name')}: workers {w!r}"); continue
            if c.get("metric") not in METRICS: problems.append(f"cell {c.get('name')}: metric")
            if not isinstance(sides, dict) or c.get("side") not in sides:
                problems.append(f"cell {c.get('name')}: side not declared")
            if not isinstance(sets, dict) or str(w) not in sets:
                problems.append(f"cell {c.get('name')}: no cpu set for {w} workers")
            if c.get("metric") == "insert_lifecycle" and w != 1:
                problems.append(f"cell {c.get('name')}: insert() one by one has one worker")
    parent = spec.get("scratch_parent")
    if not isinstance(parent, str) or not parent.strip() or not Path(parent).is_dir():
        problems.append(f"scratch_parent {parent!r} is not an existing directory")
    return spec, text, problems


def run(a):
    spec, spec_text, problems = load_spec(a.spec)
    if problems:
        for p in problems: print(f"  {p}")
        return 2
    out = Path(a.out_dir)
    if out.exists():
        print(f"  {out} already exists"); return 2
    topology, why = check_topology(spec["cpu_sets"])
    if topology is None:
        print(f"  TOPOLOGY REFUSED: {why}"); return 2
    grant, why = auth.check(a.authorization, spec["task_id"])
    if grant is None:
        print(f"  NOT AUTHORIZED: {why}")
        print("  nothing was created, copied or measured")
        return 2

    names = sorted({c["side"] for c in spec["cells"]})
    sides = {}
    for n in names:
        sides[n] = grid.describe_side(n, spec["sides"][n])
        sides[n]["value_size"] = json.loads((Path(sides[n]["source"]) / "shape.json").read_text())["value_size"]
        if sides[n]["value_size"] != VALUE_SIZE:
            print(f"  side {n}: value_size {sides[n]['value_size']}, not {VALUE_SIZE}"); return 2
    biggest = max(s["source_bytes"] for s in sides.values())
    if biggest * 2 > spec["max_scratch_gib"] * GIB:
        print(f"  a copy of {biggest/GIB:.2f} GiB, grown, does not fit the scratch budget"); return 2
    source = Path(spec["harness_source"]).resolve()
    source_sha = grid.sha256_file(source)       # before anything exists; again after the run
    out.mkdir(parents=True); (out / "raw").mkdir()
    scratch = Path(spec["scratch_parent"]) / f"batchrun_{os.getpid()}_{os.urandom(6).hex()}"
    scratch.mkdir()
    (out / "spec.json").write_text(spec_text)
    config = {"schema": CONFIG_SCHEMA, "spec": spec, "spec_sha256": grid.sha256_text(spec_text),
              "sides": sides, "topology": topology, "scratch_root": str(scratch),
              "modules": module_hashes(),
              "harness_source_sha256": source_sha,
              "authorization": {k: grant[k] for k in ("sha256", "path", "task_id", "host",
                                                       "expires_utc", "approval_code", "granted_by")},
              "measured": "the harness's elapsed_ns: every insert or insert_batch call, plus a light "
                          "diagnostic inside it - one clock read per call (per pause_every inserts for "
                          "insert()) and a copy of each call's counters into a reserved vector",
              "excluded": "the copy and its verification, building the requests, validation, census and "
                          "statistics reads. No sample calls sync(): the durable boundary as a whole is not "
                          "measured. Rotations inside the inserts are measured, metadata writes and the "
                          "writeback they start included",
              "host": platform.node(), "kernel": platform.release(),
              "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())}
    config["config_sha256"] = config_identity(config)
    (out / "config.json").write_text(json.dumps(config, indent=1))
    cells = spec["cells"]
    by_name = {c["name"]: c for c in cells}
    schedule = plan_of(spec)
    (out / "schedule.json").write_text(json.dumps(schedule, indent=1))
    plan = [(s["repetition"], [by_name[n] for n in s["order"]]) for s in schedule]
    total = len(cells) * spec["repetitions"]
    print(f"  authorized until {grant['expires_utc']}; {len(cells)} cells x {spec['repetitions']} "
          f"repetitions = {total} samples; scratch {scratch}", flush=True)

    records, stop, position = [], None, 0
    for r, order in plan:
        if stop: break
        for c in order:
            again, why = auth.check(a.authorization, spec["task_id"])
            if again is None or again["sha256"] != grant["sha256"]:
                stop = f"authorization no longer valid: {why or 'the grant file changed'}"; break
            s = sides[c["side"]]; need = 2 * s["source_bytes"]
            if grid.free_bytes(scratch) - need < spec["min_free_gib"] * GIB:
                stop = f"free space would fall below {spec['min_free_gib']} GiB"; break
            if grid.du_bytes(scratch) + need > spec["max_scratch_gib"] * GIB:
                stop = f"the scratch root would exceed {spec['max_scratch_gib']} GiB"; break
            position += 1
            e = expected_for(config, c["name"], r)
            cmd = ["taskset", "-c", ",".join(map(str, e["cpus"])), s["harness"], "--metric", c["metric"],
                   "--source", s["source"], "--scratch-root", str(scratch),
                   "--inserts", str(spec["inserts"]), "--pause-every", str(spec["pause_every"]),
                   "--batch-size", str(spec["batch_size"]), "--workers", str(c["workers"]),
                   "--seed", str(r)]
            so, se, code, timed_out, secs = grid.run_sample(
                cmd, spec["timeout"], f"[{position}/{total}] rep {r} {c['name']}")
            stem = f"{c['name']}-rep{r}"
            (out / "raw" / f"{stem}.json").write_text(so)
            (out / "raw" / f"{stem}.stderr").write_text(se)
            (out / "raw" / f"{stem}.cmd").write_text(shlex.join(cmd) + "\n")
            d, _ = parse_raw(so)
            rec = record_of(d, e, c, so, code, timed_out, spec["timeout"])
            rec["position"] = position
            if rec["work_dir"]:
                keep = not rec["valid"] and grid.du_bytes(scratch) + need <= spec["max_scratch_gib"] * GIB
                rec["cleanup"] = "kept: the sample failed" if keep \
                    else grid.safe_remove(rec["work_dir"], scratch, rec["token"])
            records.append(rec)
            # Progress without times: what ran, whether it is valid, what was cleaned.
            print(f"  [{position}/{total}] END exit={code} "
                  f"{'valid' if rec['valid'] else 'UNUSABLE: ' + str(rec['unusable_because'])} "
                  f"cleanup={rec.get('cleanup')}", flush=True)

    cross = cross_problems(records, sides)
    after = {}
    for n, s in sides.items():
        h, f, _ = grid.fixture_digest(Path(s["source"]))
        after[n] = {"unchanged": h == s["source_store_sha256"] and f == s["source_store_files"],
                    "store_sha256_after": h}
    provenance = grid.after_hashes({**{f"harness:{n}": (s["harness"], s["harness_sha256"]) for n, s in sides.items()},
                                    "harness_source": (source, source_sha)})
    changed = grid.provenance_problem(provenance)
    report = {"config_sha256": config["config_sha256"], "records": records,
              "per_cell": per_cell(records, cells), "cross_sample_problems": cross,
              "sources_after": after, "stopped": stop, "provenance_after": provenance,
              "left_in_scratch": sorted(p.name for p in scratch.iterdir()),
              "note": "exploratory: the raw elapsed values of each sample, no estimator, no choice of N"}
    ok = not stop and not cross and len(records) == total and all(x["valid"] for x in records) \
        and all(v["unchanged"] for v in after.values()) and changed is None
    report["verdict"] = "complete" if ok else ("INCOMPLETE - " + (
        changed or "see records, cross_sample_problems, stopped and sources_after"))
    (out / "report.json").write_text(json.dumps(report, indent=1))
    print(f"  {report['verdict']}")
    problems, complete = verify_run(out)
    for p in problems: print(f"  NOT VERIFIED: {p}")
    return 0 if ok and complete and not problems else 3


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--spec")
    ap.add_argument("--out-dir", help="must not exist")
    ap.add_argument("--authorization", default=None)
    ap.add_argument("--verify", metavar="OUT_DIR", help="re-judge a finished run from its files and exit")
    a = ap.parse_args()
    if a.verify:
        # Whatever is missing or malformed in the run's files is a problem the
        # verifier names, not a traceback: the answer is NOT VERIFIED either way.
        try:
            problems, complete = verify_run(a.verify)
        except (OSError, ValueError, KeyError, TypeError, AttributeError, ArithmeticError) as e:
            problems, complete = [f"the run's files could not be read: {type(e).__name__}: {e}"], False
        for p in problems: print(f"  {p}")
        if problems: print(f"  NOT VERIFIED: {len(problems)} problems")
        elif not complete: print("  consistent, but INCOMPLETE: not certified")
        else: print("  verified: complete")
        sys.exit(0 if not problems and complete else 3)
    if not a.spec or not a.out_dir:
        ap.error("--spec and --out-dir are required to run")
    sys.exit(run(a))


if __name__ == "__main__":
    main()
