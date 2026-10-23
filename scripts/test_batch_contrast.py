#!/usr/bin/env python3
"""Tests for run_batch_contrast.py. Untimed: no time any of them observes is reported.

  - the topology check, on fake /sys trees (SMT sibling, two L3s, two nodes, an
    offline CPU, a wrong count) and on this machine's real one;
  - the runner end to end with fake harnesses and grants written by this test
    for a test task only: a complete run, no grant, an expired grant, a grant
    for another task, every way a sample can be wrong, a source written during
    the run, and the spec's limits;
  - the sample and cross-sample predicates on W2/W4/W8 samples of the reference
    matrix, on any host, so a host with one CPU still checks them;
  - with --build1/--build8/--work-parent, a real smoke: the real harnesses on
    300 000-entry images, every cell of the matrix once, through the runner,
    checked for validity and for one content across all of them. Its times are
    in the runner's raw files and this test neither reads nor prints them.

Run:
  python3 -I scripts/test_batch_contrast.py [--build1 B --build8 B --work-parent DIR]
"""
import argparse, datetime, importlib.util, json, os, platform, shutil, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
RUNNER = HERE / "run_batch_contrast.py"
spec_ = importlib.util.spec_from_file_location("contrast", RUNNER)
contrast = importlib.util.module_from_spec(spec_); spec_.loader.exec_module(contrast)
POP = 600_000
SETS = {"1": [2], "2": [2, 3], "4": [2, 3, 4, 5], "8": [0, 1, 2, 3, 4, 5, 6, 7]}   # the reference layout

# The fake-harness runs do not depend on this host's layout. They use CPUs this
# process may actually run on - so taskset works anywhere - and give the runner a
# fake /sys that describes them as distinct physical cores on one L3 and one node.
# The runner's topology check runs unchanged, only against that tree. Worker
# counts the host cannot give W distinct CPUs for are left out of the matrix.
AVAILABLE = sorted(os.sched_getaffinity(0))
FIT = [w for w in (1, 2, 4, 8) if w <= len(AVAILABLE)]
FAKE_SETS = {str(w): AVAILABLE[:w] for w in FIT}
CELLS = [{"name": "N1-W1", "side": "N1", "metric": "insert_batch", "workers": 1},
         {"name": "N8-W1", "side": "N8", "metric": "insert_batch", "workers": 1},
         {"name": "N8-W2", "side": "N8", "metric": "insert_batch", "workers": 2},
         {"name": "N8-W4", "side": "N8", "metric": "insert_batch", "workers": 4},
         {"name": "N8-W8", "side": "N8", "metric": "insert_batch", "workers": 8},
         {"name": "N1-seq", "side": "N1", "metric": "insert_lifecycle", "workers": 1},
         {"name": "N8-seq", "side": "N8", "metric": "insert_lifecycle", "workers": 1}]
RUN_CELLS = [c for c in CELLS if c["workers"] in FIT]
MULTI = any(c["workers"] > 1 for c in RUN_CELLS)
ok = failed = skipped = 0

FAKE = r'''#!/usr/bin/env python3
import hashlib, json, os, sys
args = sys.argv[1:]
g = lambda k, d=None: args[args.index(k) + 1] if k in args else d
src, root, metric = g("--source"), g("--scratch-root"), g("--metric")
shape = json.load(open(os.path.join(src, "shape.json")))
n, K, B, W = shape["shard_count"], int(g("--inserts")), int(g("--batch-size")), int(g("--workers"))
token = hashlib.sha256(os.urandom(8)).hexdigest()[:16]
work = os.path.join(root, f"calibmut_{metric}_n{n}_{os.getpid()}_{token}")
os.makedirs(work)
open(os.path.join(work, ".calibmut-owner"), "w").write(f"token={token}\nsource={src}\n")
slots = [{"class": 0, "shard": s, "gained": K // n + (1 if s < K % n else 0)} for s in range(n)] + \
        [{"class": c, "shard": 0, "gained": 0} for c in range(1, 5)]
batch = metric == "insert_batch"
calls = -(-K // B) if batch else 0
used = min(W, n) if batch else 0
spread = {"affinity_cpus": sorted(os.sched_getaffinity(0)), "cpu_ns_over_timed_region": 5,
          "inserts_counted_by_statistics": K, "slots": slots,
          "workers_requested": W if batch else 0, "batch_size": B if batch else 0, "calls": calls,
          "tasks": calls * used, "tasks_on_calling_thread": calls, "workers_used_min": used,
          "workers_used_max": used, "calls_short_of_threads": 0, "inserted": K if batch else 0,
          "duplicates": 0, "failed": 0, "not_applied": 0, "calls_with_error": 0}
doc = {"schema": "utxoz-calib-mut-1", "metric": metric, "shard_count": n,
       "geometry_id": shape["geometry_id"], "population": shape["population"], "value_size": 33,
       "inserts": K, "pause_every": int(g("--pause-every")), "delete_batch": 100000, "setup_deletions": 0,
       "seed": int(g("--seed")), "source": os.path.realpath(src), "work_dir": work, "token": token,
       "copy_files": 3, "copy_bytes": 4096, "copy_ms": 1.0, "initial_digest": "0123456789abcdef",
       "elapsed_ns": 1000, "failed_ops": 0,
       "batches": calls if batch else K // int(g("--pause-every")), "batch_ns_p50": 0, "batch_ns_p99": 0,
       "batch_ns_max": 0, "rotations_class48": 3 * n, "generations_class48_before": 6,
       "generations_class48_after": 9, "version_files_before": 10, "version_files_after": 13,
       "logical_bytes_before": 1, "logical_bytes_after": 1, "entries_after": shape["population"] + K,
       "final_digest": "fedcba9876543210", "expected_digest": "fedcba9876543210", "maxrss_kb": 1,
       "observed": False, "observation": None, "spread": spread, "valid": True, "invalid_because": ""}
MUTATE
print(json.dumps(doc))
'''


def check(name, good, why=""):
    global ok, failed
    if good: print(f"  ok     {name}"); ok += 1
    else: print(f"  FALLA  {name}: {why}"); failed += 1


def skip(name, why):
    global skipped
    print(f"  SKIP   {name}: {why}"); skipped += 1


def case(name, body):
    root = Path(tempfile.mkdtemp(prefix="contrast_"))
    try: good, why = body(root)
    except Exception as e: good, why = False, f"{type(e).__name__}: {e}"
    finally: shutil.rmtree(root, ignore_errors=True)
    check(name, good, why)


# ------------------------------------------------------------------ topology

def fake_sys(root, cpus):
    """cpus: {cpu: (package, core, l3, node, online)}"""
    sc, sn = root / "cpu", root / "node"
    for c, (pkg, core, l3, node, online) in cpus.items():
        b = sc / f"cpu{c}"; (b / "topology").mkdir(parents=True)
        (b / "topology" / "physical_package_id").write_text(f"{pkg}\n")
        (b / "topology" / "core_id").write_text(f"{core}\n")
        (b / "online").write_text("1\n" if online else "0\n")
        for i, (lvl, cid) in enumerate(((1, core), (2, core), (3, l3))):
            d = b / "cache" / f"index{i}"; d.mkdir(parents=True)
            (d / "level").write_text(f"{lvl}\n"); (d / "id").write_text(f"{cid}\n")
    nodes = {}
    for c, (_, _, _, node, _) in cpus.items(): nodes.setdefault(node, []).append(c)
    for node, cs in nodes.items():
        d = sn / f"node{node}"; d.mkdir(parents=True)
        (d / "cpulist").write_text(",".join(map(str, cs)) + "\n")
    return sc, sn


def machine(over=None):
    # 16 cores, SMT siblings at +16, L3 0 for cores 0-7 and 1 for 8-15, one node.
    m = {c: (0, c % 16, 0 if c % 16 < 8 else 1, 0, True) for c in range(32)}
    m.update(over or {})
    return m


def topology_cases():
    def topo(sets, m):
        def body(root):
            sc, sn = fake_sys(root, m)
            return contrast.check_topology(sets, sc, sn)
        return body
    case("topología: cuatro conjuntos de núcleos físicos en un L3 y un nodo",
         lambda root: (lambda r: (r[0] is not None, r[1]))(topo(SETS, machine())(root)))
    def refused(name, sets, m, want):
        case(name, lambda root: (lambda r: (r[0] is None and want in (r[1] or ""), r[1]))(topo(sets, m)(root)))
    refused("topología: dos CPUs hermanas SMT se rechazan", {"2": [2, 18]}, machine(), "SMT")
    refused("topología: un conjunto que cruza L3 se rechaza", {"4": [6, 7, 8, 9]}, machine(), "L3")
    refused("topología: conjuntos en L3 distintos se rechazan", {"1": [2], "2": [8, 9]}, machine(), "L3")
    refused("topología: dos nodos NUMA se rechazan", {"2": [2, 3]}, machine({3: (0, 3, 0, 1, True)}), "NUMA")
    refused("topología: una CPU fuera de línea se rechaza", {"1": [2]}, machine({2: (0, 2, 0, 0, False)}), "offline")
    refused("topología: un conjunto con otro tamaño que W se rechaza", {"4": [2, 3, 4]}, machine(), "not 4")
    refused("topología: un conjunto fuera de orden ascendente se rechaza", {"2": [3, 2]}, machine(), "ascending")
    def no_node(root):
        sc, sn = fake_sys(root, machine())
        (sn / "node0" / "cpulist").write_text("0-1,3-31\n")    # cpu 2 on no node
        facts, why = contrast.check_topology({"1": [2]}, sc, sn)
        return facts is None and "no NUMA node" in (why or ""), why
    case("topología: una CPU sin nodo NUMA se rechaza (no None)", no_node)
    # And this machine, as it is - only where it has the reference layout.
    facts, why = contrast.check_topology(SETS)
    if facts is None:
        skip("topología real: los conjuntos de referencia pasan en esta máquina",
             f"this host does not have the reference layout ({why})")
    else:
        case("topología real: los conjuntos de referencia pasan en esta máquina",
             lambda root: (contrast.check_topology(SETS)[0] is not None, None))
    siblings = Path("/sys/devices/system/cpu/cpu2/topology/thread_siblings_list")
    if not siblings.is_file() or 18 not in contrast.parse_list(siblings.read_text()):
        skip("topología real: CPU 2 con su hermana SMT 18 se rechaza",
             "CPUs 2 and 18 are not SMT siblings on this host")
    else:
        case("topología real: CPU 2 con su hermana SMT 18 se rechaza",
             lambda root: (lambda r: (r[0] is None and "SMT" in (r[1] or ""), r[1]))(
                 contrast.check_topology({"2": [2, 18]})))


# ------------------------------------------------------------------ runner

def source(root, name, n):
    fx = root / f"src-{name}"; fx.mkdir()
    (fx / "shape.json").write_text(json.dumps({"schema": "utxoz-hist-shape-2", "population": POP,
        "historical_prefix": 500000, "active_entries": 90000, "generations_class_48": 6 * n,
        "shard_count": n, "geometry_id": 3 if n == 1 else 0x80000010 + n, "value_size": 33}))
    (fx / "census.json").write_text("{}")
    (fx / "cont_0_v00000.dat").write_bytes(b"s" * 4096)
    return fx


def harness(root, name, mutate="pass"):
    h = root / f"fake-{name}"; h.write_text(FAKE.replace("MUTATE", mutate)); h.chmod(0o755)
    return h


def grant(root, **over):
    g = {"schema": "utxoz-campaign-authorization-1", "host": platform.node(), "task_id": "TEST-FAKE-ONLY",
         "approval_code": "fake-for-tests", "granted_by": "test_batch_contrast.py",
         "expires_utc": (datetime.datetime.now(datetime.timezone.utc)
                         + datetime.timedelta(minutes=10)).strftime("%Y-%m-%dT%H:%M:%SZ")}
    g.update(over); p = root / "grant.json"; p.write_text(json.dumps(g)); return p


WRAPPER = r'''
import importlib.util, sys
from pathlib import Path
runner, sys_cpu, sys_node = sys.argv[1:4]
spec = importlib.util.spec_from_file_location("run_batch_contrast", runner)
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
# The production check, unchanged, pointed at a fake /sys of CPUs this host has.
m.check_topology.__defaults__ = (Path(sys_cpu), Path(sys_node))
sys.argv = [runner] + sys.argv[4:]
m.main()
'''


DELETE = object()   # an override that removes the key from the spec


def fake_host(root):
    """A fake /sys describing AVAILABLE[:max(FIT)] as one core each, one L3, one node."""
    cpus = {c: (0, i, 0, 0, True) for i, c in enumerate(AVAILABLE[:max(FIT)])}
    return fake_sys(root / "fake-sys", cpus)


def run(root, n8_mutate="pass", authorization="default", sides=None, real_topology=False, **over):
    parent = root / "scratch-parent"; parent.mkdir(exist_ok=True)
    src = root / "src.cpp"; src.write_text("// fake\n")
    spec = {"task_id": "TEST-FAKE-ONLY", "scratch_parent": str(parent), "min_free_gib": 10,
            "max_scratch_gib": 4, "repetitions": 3, "inserts": 3_000_000, "batch_size": 100_000,
            "pause_every": 10_000, "timeout": 30, "harness_source": str(src),
            "cpu_sets": SETS if real_topology else FAKE_SETS,
            "cells": CELLS if real_topology else RUN_CELLS,
            "sides": sides or {"N1": {"harness": str(harness(root, "n1")), "source": str(source(root, "n1", 1))},
                               "N8": {"harness": str(harness(root, "n8", n8_mutate)),
                                      "source": str(source(root, "n8", 8))}}}
    spec.update(over)
    for k in [k for k, v in spec.items() if v is DELETE]: del spec[k]
    sp = root / "spec.json"; sp.write_text(json.dumps(spec))
    out = root / "out"
    if real_topology:
        cmd = [sys.executable, "-I", str(RUNNER)]
    else:
        wrapper = root / "fake_topology_runner.py"; wrapper.write_text(WRAPPER)
        sc, sn = fake_host(root)
        cmd = [sys.executable, "-I", str(wrapper), str(RUNNER), str(sc), str(sn)]
    cmd += ["--spec", str(sp), "--out-dir", str(out)]
    if authorization == "default": authorization = grant(root)
    if authorization is not None: cmd += ["--authorization", str(authorization)]
    return subprocess.run(cmd, capture_output=True, text=True, timeout=600), out


def rep(out): return json.loads((out / "report.json").read_text())


def runner_cases():
    def good(root):
        p, out = run(root)
        r = rep(out)
        return (p.returncode == 0 and r["verdict"] == "complete" and len(r["records"]) == 3 * len(RUN_CELLS)
                and not r["left_in_scratch"] and all(x["cleanup"] == "removed" for x in r["records"])
                and "elapsed=" not in p.stdout, p.stdout[-400:])
    case("una corrida correcta: 7 celdas x 3, completa, sin copias sobrantes, progreso sin tiempos", good)
    def order(root):
        p, out = run(root)
        sch = json.loads((out / "schedule.json").read_text())
        names = [c["name"] for c in RUN_CELLS]
        return ([s["order"][0] for s in sch] == names[:3], str(sch)[:200])
    case("las celdas rotan por repetición", order)
    def verify(root, tamper=None, want=None):
        p, out = run(root)
        if tamper: tamper(out)
        v = subprocess.run([sys.executable, "-I", str(RUNNER), "--verify", str(out)],
                           capture_output=True, text=True, timeout=600)
        if want is None:
            return v.returncode == 0 and "verified" in v.stdout, v.stdout[-300:]
        return v.returncode == 3 and want in v.stdout, v.stdout[-300:]
    case("--verify re-juzga una corrida intacta desde sus archivos (exit 0)", verify)
    def raw_edit(out):
        f = out / "raw" / "N8-W1-rep2.json"; d = json.loads(f.read_text()); d["elapsed_ns"] += 1
        f.write_text(json.dumps(d))
    case("--verify: un crudo editado después no verifica", lambda root: verify(root, raw_edit, "not the one recorded"))
    def config_edit(out):
        f = out / "config.json"; c = json.loads(f.read_text()); c["spec"]["inserts"] += 1
        f.write_text(json.dumps(c))
    case("--verify: un config editado no coincide con su propio hash",
         lambda root: verify(root, config_edit, "its own hash"))
    def report_edit(out):
        f = out / "report.json"; r = json.loads(f.read_text()); r["records"][3]["elapsed_ns"] = 7
        f.write_text(json.dumps(r))
    case("--verify: un valor del reporte que no sale del crudo no verifica",
         lambda root: verify(root, report_edit, "recomputes"))
    def config_module(out):
        f = out / "config.json"; c = json.loads(f.read_text()); c["modules"]["campaign_auth.py"] = "0" * 64
        import hashlib
        body = {k: v for k, v in c.items() if k != "config_sha256"}
        c["config_sha256"] = hashlib.sha256(json.dumps(body, sort_keys=True).encode()).hexdigest()
        f.write_text(json.dumps(c))
    case("--verify: un módulo que no es el que corrió no verifica",
         lambda root: verify(root, config_module, "campaign_auth.py here is not"))
    # Review 2: what a complete verdict can no longer be made of.
    def edit(out, schedule=None, report=None, config=None):
        import hashlib
        if schedule:
            f = out / "schedule.json"; f.write_text(json.dumps(schedule(json.loads(f.read_text()))))
        if report:
            f = out / "report.json"; r = json.loads(f.read_text()); report(r); f.write_text(json.dumps(r))
        if config:
            f = out / "config.json"; c = json.loads(f.read_text()); config(c)
            body = {k: v for k, v in c.items() if k != "config_sha256"}
            c["config_sha256"] = hashlib.sha256(json.dumps(body, sort_keys=True).encode()).hexdigest()
            f.write_text(json.dumps(c))
            f = out / "report.json"; r = json.loads(f.read_text()); r["config_sha256"] = c["config_sha256"]
            f.write_text(json.dumps(r))
    def emptied(r):
        r["records"] = []; r["per_cell"] = {}; r["sources_after"] = {}; r["cross_sample_problems"] = []
    case("--verify: schedule, registros, por-celda y fuentes vacíos no dan 'complete'",
         lambda root: verify(root, lambda out: edit(out, schedule=lambda s: [], report=emptied), "canonical plan"))
    def truncated(r):
        r["records"] = [x for x in r["records"] if x["repetition"] < 3]
        r["per_cell"] = {k: {**v, "elapsed_ns": v["elapsed_ns"][:2], "cpu_ns": v["cpu_ns"][:2]}
                         for k, v in r["per_cell"].items()}
    case("--verify: schedule y registros truncados a dos repeticiones no dan 'complete'",
         lambda root: verify(root, lambda out: edit(out, schedule=lambda s: s[:2], report=truncated),
                             "canonical plan"))
    case("--verify: registros truncados con el schedule intacto no dan 'complete'",
         lambda root: verify(root, lambda out: edit(out, report=truncated), "does not recompute"))
    def duplicated(r):
        r["records"].append(dict(r["records"][0]))
    case("--verify: un registro duplicado no verifica",
         lambda root: verify(root, lambda out: edit(out, report=duplicated), "recorded twice"))
    case("--verify: falta un lado en sources_after",
         lambda root: verify(root, lambda out: edit(out, report=lambda r: r["sources_after"].pop("N8")),
                             "sources_after names"))
    def wrong_hash(r):
        r["sources_after"]["N1"]["store_sha256_after"] = "0" * 64
    case("--verify: fuente con unchanged=true y hash distinto",
         lambda root: verify(root, lambda out: edit(out, report=wrong_hash), "unchanged is True but"))
    case("--verify: el spec del config no es spec.json (con hash propio recalculado)",
         lambda root: verify(root, lambda out: edit(out, config=lambda c: c["spec"].update(inserts=1)),
                             "not spec.json"))
    def failed_honest(root):
        p, out = run(root, n8_mutate='doc["seed"] += 1 if W == 1 and metric == "insert_batch" else 0')
        v = subprocess.run([sys.executable, "-I", str(RUNNER), "--verify", str(out)],
                           capture_output=True, text=True, timeout=600)
        return (p.returncode == 3 and v.returncode == 3 and "INCOMPLETE: not certified" in v.stdout
                and "NOT VERIFIED" not in v.stdout, v.stdout[-300:])
    case("--verify: una muestra fallida bien informada es consistente pero no se certifica (exit 3)",
         failed_honest)
    def nothing(root, **kw):
        p, out = run(root, **kw)
        return (p.returncode == 2 and not out.exists()
                and not any((root / "scratch-parent").iterdir()), p.stdout[-200:])
    case("sin autorización no se crea nada", lambda root: nothing(root, authorization=None))
    case("autorización vencida: no se crea nada",
         lambda root: nothing(root, authorization=grant(root, expires_utc="2020-01-01T00:00:00Z")))
    case("autorización de otra tarea (TAREA07): no se crea nada",
         lambda root: nothing(root, authorization=grant(root, task_id="TAREA07")))
    case("topología rechazada antes de pedir autorización: no se crea nada",
         lambda root: nothing(root, cpu_sets={**FAKE_SETS, "1": [AVAILABLE[0], AVAILABLE[0]]}))
    # Review 5637941911: a set out of ascending order is refused before anything
    # exists, not found out after the run. Only where a two-CPU set fits.
    if "2" in FAKE_SETS:
        case("conjunto de CPUs fuera de orden: rechazado antes de crear nada",
             lambda root: (lambda pr: (pr[0].returncode == 2 and "ascending" in pr[0].stdout
                                       and not pr[1].exists() and not any((root / "scratch-parent").iterdir()),
                                       pr[0].stdout[-200:]))(
                 run(root, cpu_sets={**FAKE_SETS, "2": FAKE_SETS["2"][::-1]})))
    else:
        skip("conjunto de CPUs fuera de orden: rechazado antes de crear nada",
             "no two-CPU set fits the CPUs this process may run on; see the topology case")
    def bad(mutate, want):
        def body(root):
            p, out = run(root, n8_mutate=mutate)
            r = rep(out)
            badr = [x for x in r["records"] if not x["valid"]]
            return (p.returncode == 3 and badr and want in (badr[0]["unusable_because"] or "")
                    and all(x["work_dir"] is None or Path(x["work_dir"]).exists() for x in badr), str(badr[:1])[:300])
        return body
    case("afinidad distinta de la de la celda", bad('spread["affinity_cpus"] = [0]', "not the cell's"))
    case("una llamada sin todos sus hilos", bad('spread["calls_short_of_threads"] = 1', "calls_short_of_threads"))
    case("menos workers que los posibles", bad('spread["workers_used_max"] = 1 if W > 1 else 0',
                                               "workers_used_max"))
    case("estadísticas que no cuentan todo", bad('spread["inserts_counted_by_statistics"] -= 1', "statistics"))
    case("un slot que no ganó lo suyo", bad('spread["slots"][0]["gained"] -= 1', "slots gained"))
    case("contenido final distinto del de la regla", bad('doc["final_digest"] = "0" * 16', "rule predicts"))
    case("el harness lo declara inválido (exit 3)",
         bad('doc["valid"] = False; doc["invalid_because"] = "x"; print(json.dumps(doc)); sys.exit(3)', "exit 3"))
    case("otra cantidad de llamadas", bad('spread["calls"] += 1', "calls"))
    # One per family of the contract check_sample closes.
    for name, mutate, want in (
            ("semilla que no es la repetición", 'doc["seed"] += 1', "seed is"),
            ("pause_every distinto", 'doc["pause_every"] += 1', "pause_every is"),
            ("value_size distinto de 33", 'doc["value_size"] = 34', "value_size is"),
            ("marcas de lote distintas", 'doc["batches"] += 1', "batches is"),
            ("un duplicado en las llamadas", 'spread["duplicates"] = 1 if batch else 0', "duplicates"),
            ("un pedido fallido", 'spread["failed"] = 1 if batch else 0', "failed"),
            ("un pedido no aplicado", 'spread["not_applied"] = 1 if batch else 0', "not_applied"),
            ("una llamada con error", 'spread["calls_with_error"] = 1 if batch else 0', "calls_with_error"),
            ("tareas que no son llamadas x workers", 'spread["tasks"] += 1 if batch else 0', "spread.tasks"),
            ("tareas en el llamador que no son una por llamada",
             'spread["tasks_on_calling_thread"] += 1 if batch else 0', "tasks_on_calling_thread"),
            ("contadores de lote en insert() uno por uno", 'spread["calls"] += 0 if batch else 1', "spread.calls"),
            ("un slot repetido", 'spread["slots"][1] = dict(spread["slots"][0])', "the slots are"),
            ("un slot de menos", 'spread["slots"].pop()', "slots, not"),
            ("una ganancia booleana",
             'spread["slots"][-1]["gained"] = True; spread["slots"][0]["gained"] -= 1', "non-negative integers"),
            ("una ganancia negativa compensada",
             'spread["slots"][0]["gained"] += 1; spread["slots"][-1]["gained"] = -1', "non-negative integers"),
            ("ganancia en otra clase, compensada",
             'spread["slots"][0]["gained"] -= 5; spread["slots"][-1]["gained"] = 5', "class 4 gained"),
            ("tiempo de CPU negativo", 'spread["cpu_ns_over_timed_region"] = -1', "cpu_ns"),
            ("tiempo de CPU no entero", 'spread["cpu_ns_over_timed_region"] = 1.5', "cpu_ns"),
            ("tiempo de CPU NaN", 'spread["cpu_ns_over_timed_region"] = float("nan")', "no usable JSON"),
            ("tiempo transcurrido booleano", 'doc["elapsed_ns"] = True', "elapsed_ns"),
            ("tiempo transcurrido cero", 'doc["elapsed_ns"] = 0', "elapsed_ns"),
            ("digests ausentes e iguales (None == None)",
             'doc["final_digest"] = doc["expected_digest"] = None', "16 hex digits"),
            ("digest vacío", 'doc["initial_digest"] = ""', "16 hex digits")):
        case(f"muestra inválida: {name}", bad(mutate, want))
    # Only a W>1 cell can use fewer workers than it may and still use some; the
    # predicate itself is covered on any host by predicate_cases().
    if MULTI:
        case("muestra inválida: workers mínimos menores que los posibles",
             bad('spread["workers_used_min"] -= 1 if batch and W > 1 else 0', "workers_used_min"))
    else:
        skip("muestra inválida: workers mínimos menores que los posibles",
             "no W>1 cell fits the CPUs this process may run on; see 'predicado:' cases")
    # Review 5637775062: a document of the wrong shape is an invalid sample in
    # a written report, from the run and from --verify - never a traceback.
    def shaped(mutate, want):
        def body(root):
            p, out = run(root, n8_mutate=mutate)
            r = rep(out)
            badr = [x for x in r["records"] if not x["valid"]]
            v = subprocess.run([sys.executable, "-I", str(RUNNER), "--verify", str(out)],
                               capture_output=True, text=True, timeout=600)
            return (p.returncode == 3 and "Traceback" not in p.stderr and badr
                    and all(want in (x["unusable_because"] or "") for x in badr)
                    and all(x["work_dir"] is None or isinstance(x["work_dir"], str) for x in r["records"])
                    and v.returncode == 3 and "Traceback" not in v.stderr and "NOT VERIFIED" not in v.stdout,
                    f"exit {p.returncode}/{v.returncode} {p.stderr[-200:]} {v.stderr[-200:]} {str(badr[:1])[:200]}")
        return body
    for name, mutate, want in (
            ("una lista", 'print("[]"); sys.exit(0)', "no usable JSON"),
            ("null", 'print("null"); sys.exit(0)', "no usable JSON"),
            ("un número", 'print("5"); sys.exit(0)', "no usable JSON"),
            ("slots null", 'spread["slots"] = None', "slots, not"),
            ("slots numérico", 'spread["slots"] = 5', "slots, not"),
            ("work_dir numérico", 'doc["work_dir"] = 5', "work_dir is missing"),
            ("token lista", 'doc["token"] = [1]', "token is [1]")):
        case(f"documento con otra forma: {name} es inválido, sin traceback", shaped(mutate, want))
    def typed(root):
        e = {"cpus": [0]}
        rec = contrast.record_of({"work_dir": 5, "token": [1], "spread": {"slots": None}}, {**e, "repetition": 1},
                                 {"name": "x", "side": "N8", "metric": "insert_batch", "workers": 1},
                                 "{}", 3, False, 30)
        return (rec["work_dir"] is None and rec["token"] is None and rec["slots_gained"] == []
                and not rec["valid"], str(rec)[:300])
    case("record_of: un work_dir o token que no es texto nunca llega a la limpieza", typed)
    def cross(mutate, want):
        def body(root):
            p, out = run(root, n8_mutate=mutate)
            r = rep(out)
            return (p.returncode == 3 and any(want in c for c in r["cross_sample_problems"]),
                    str(r["cross_sample_problems"])[:300])
        return body
    case("dos celdas con contenido final distinto entre sí (ambos válidos)",
         cross('doc["final_digest"] = doc["expected_digest"] = "1" * 16', "final_digest"))
    # insert_batch and insert() cells of one side differ even with W1 alone, so
    # this runs on any host; with W>1 cells the worker counts differ too.
    case("rotaciones distintas entre celdas del mismo N",
         cross('doc["rotations_class48"] += W if batch else 0', "rotated"))
    def mutated(root):
        p, out = run(root, n8_mutate='open(os.path.join(src, "x.dat"), "w").write("x")')
        r = rep(out)
        return p.returncode == 3 and not r["sources_after"]["N8"]["unchanged"], str(r["sources_after"])[:200]
    case("una fuente escrita invalida la corrida", mutated)
    for name, over in (("cuatro repeticiones", {"repetitions": 4}),
                       ("scratch de más de 4 GiB", {"max_scratch_gib": 5}),
                       ("menos de 10 GiB libres exigidos", {"min_free_gib": 9}),
                       ("insert() uno por uno con W2", {"cells": RUN_CELLS + [{"name": "x", "side": "N8",
                                                         "metric": "insert_lifecycle", "workers": 2}]}),
                       ("una celda sin conjunto de CPUs", {"cells": RUN_CELLS + [{"name": "x", "side": "N8",
                                                            "metric": "insert_batch", "workers": 3}]}),
                       ("workers booleano", {"cells": RUN_CELLS + [{"name": "x", "side": "N8",
                                              "metric": "insert_batch", "workers": True}]}),
                       ("workers cero", {"cells": RUN_CELLS + [{"name": "x", "side": "N8",
                                         "metric": "insert_batch", "workers": 0}]}),
                       ("timeout infinito", {"timeout": float("inf")}),
                       ("scratch_parent ausente", {"scratch_parent": DELETE}),
                       ("scratch_parent nulo", {"scratch_parent": None}),
                       ("scratch_parent numérico", {"scratch_parent": 123}),
                       ("scratch_parent vacío", {"scratch_parent": ""}),
                       ("scratch_parent inexistente", {"scratch_parent": "/nonexistent/utxoz-cr147"}),
                       ("repeticiones booleanas", {"repetitions": True})):
        case(f"spec: {name} se rechaza",
             lambda root, over=over: (lambda pr: (pr[0].returncode == 2 and not pr[1].exists(),
                                                  pr[0].stdout[-200:]))(run(root, **over)))


# ------------------------------------------------------------------ predicates
#
# The runner's own validate_sample and cross_problems, called on samples of the
# reference matrix (W1-W8 on SETS) whatever this host has: the fake harness
# writes the document, the test states the CPUs it ran on. No taskset, no /sys.

def predicate_cases():
    inserts, batch_size, pause = 3_000_000, 100_000, 10_000
    def sample(root, cell, mutate="pass", rep=1):
        n = 8 if cell["side"] == "N8" else 1
        src = source(root, f"{cell['name']}-{rep}", n); scratch = root / f"scratch-{cell['name']}-{rep}"
        scratch.mkdir()
        p = subprocess.run([sys.executable, "-I", str(harness(root, f"{cell['name']}-{rep}", mutate)),
                            "--source", str(src), "--scratch-root", str(scratch), "--metric", cell["metric"],
                            "--inserts", str(inserts), "--batch-size", str(batch_size),
                            "--workers", str(cell["workers"]), "--pause-every", str(pause), "--seed", str(rep)],
                           capture_output=True, text=True, timeout=60)
        d = json.loads(p.stdout)
        d["spread"]["affinity_cpus"] = SETS[str(cell["workers"])]      # the mocked topology
        config = {"spec": {"cells": CELLS, "cpu_sets": SETS, "inserts": inserts, "batch_size": batch_size,
                           "pause_every": pause},
                  "sides": {cell["side"]: {"shard_count": n, "geometry_id": 3 if n == 1 else 0x80000010 + n,
                                           "population": POP, "value_size": 33,
                                           "source": os.path.realpath(src)}}}
        return d, contrast.expected_for(config, cell["name"], rep)
    multi = [c for c in CELLS if c["workers"] > 1]
    def judged(mutate, want):
        def body(root):
            got = [contrast.validate_sample(*sample(root, c, mutate)) for c in multi]
            return all(w is not None and want in w for w in got), str(got)
        return body
    def intact(root):
        got = [contrast.validate_sample(*sample(root, c)) for c in multi]
        return got == [None] * len(multi), str(got)
    case("predicado: muestras W2/W4/W8 intactas son válidas (control)", intact)
    case("predicado: workers mínimos menores que los posibles (W2/W4/W8)",
         judged('spread["workers_used_min"] -= 1', "spread.workers_used_min"))
    case("predicado: workers máximos menores que los posibles (W2/W4/W8)",
         judged('spread["workers_used_max"] = 1', "spread.workers_used_max"))
    def rotations(root, mutate):
        recs = []
        for c in [c for c in CELLS if c["side"] == "N8"]:
            d, e = sample(root, c, mutate)
            recs.append(contrast.record_of(d, e, c, json.dumps(d), 0, False, 30))
        return [x for x in recs if not x["valid"]], contrast.cross_problems(recs, ["N8"])
    def rot_differ(root):
        bad, probs = rotations(root, 'doc["rotations_class48"] += W')
        return not bad and any("side N8's samples rotated" in p for p in probs), f"{bad[:1]} {probs}"
    case("predicado: rotaciones distintas entre workers del mismo N", rot_differ)
    def rot_same(root):
        bad, probs = rotations(root, "pass")
        return not bad and probs == [], f"{bad[:1]} {probs}"
    case("predicado: rotaciones iguales en todas las celdas de N8 no son un problema (control)", rot_same)


# ------------------------------------------------------------------ real smoke

def real_smoke(build1, build8, parent):
    root = Path(tempfile.mkdtemp(prefix="contrast-smoke-", dir=parent))
    try:
        sides = {}
        for name, b in (("N1", build1), ("N8", build8)):
            fx = root / f"fx-{name}"
            r = subprocess.run([str(Path(b) / "benchmarks" / "utxoz_calib_hist"), "--build-fixture", str(fx),
                                "--population", "300000"], capture_output=True, text=True)
            check(f"smoke real: imagen de 300 000 para {name}", r.returncode == 0, r.stderr[-200:])
            sides[name] = {"harness": str(Path(b) / "benchmarks" / "utxoz_calib_mut"), "source": str(fx)}
        before = {n: contrast.grid.fixture_digest(Path(s["source"]))[0] for n, s in sides.items()}
        p, out = run(root, sides=sides, repetitions=1, inserts=150_000, batch_size=10_000, timeout=300,
                     real_topology=True)
        r = rep(out)
        check("smoke real: las 7 celdas válidas, completa (exit 0)", p.returncode == 0 and r["verdict"] == "complete",
              p.stdout[-500:])
        problems, complete = contrast.verify_run(out)
        check("smoke real: --verify re-juzga la corrida desde sus archivos: completa, sin problemas",
              not problems and complete, str(problems)[:300])
        finals = {x["final_digest"] for x in r["records"]}
        check("smoke real: un único contenido final en todas las celdas", len(finals) == 1, str(finals))
        n8 = {x["cell"]: x for x in r["records"] if x["side"] == "N8"}
        check("smoke real: en N8, workers usados = W (1, 2, 4, 8) y una tarea por llamada en el llamador",
              [n8[f"N8-W{w}"]["workers_used_max"] for w in (1, 2, 4, 8)] == [1, 2, 4, 8]
              and all(n8[f"N8-W{w}"]["tasks_on_calling_thread"] == 15 for w in (1, 2, 4, 8)),
              str({k: (v["workers_used_max"], v["tasks_on_calling_thread"]) for k, v in n8.items()}))
        n1 = [x for x in r["records"] if x["side"] == "N1" and x["metric"] == "insert_batch"]
        check("smoke real: en N1 hay un solo slot con inserciones, y W1 usa uno",
              all(sum(1 for g in x["slots_gained"] if g) == 1 and x["workers_used_max"] == 1 for x in n1))
        check("smoke real: las 8 particiones de N8 ganaron entradas",
              all(sum(1 for g in x["slots_gained"] if g) == 8 for x in n8.values()))
        check("smoke real: sin copias sobrantes y fuentes intactas",
              not r["left_in_scratch"] and all(v["unchanged"] for v in r["sources_after"].values())
              and all(contrast.grid.fixture_digest(Path(s["source"]))[0] == before[n] for n, s in sides.items()))
    finally:
        shutil.rmtree(root, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build1"); ap.add_argument("--build8"); ap.add_argument("--work-parent")
    a = ap.parse_args()
    print("=== run_batch_contrast.py ===")
    topology_cases()
    runner_cases()
    predicate_cases()
    if a.build1 and a.build8 and a.work_parent:
        real_smoke(a.build1, a.build8, a.work_parent)
    else:
        print("  (smoke real no pedido: faltan --build1/--build8/--work-parent)")
    print(f"\n  {ok} ok, {failed} fallas, {skipped} skipped")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
