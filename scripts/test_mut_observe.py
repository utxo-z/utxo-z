#!/usr/bin/env python3
"""End-to-end tests for run_mut_observe.py, with fake harnesses and fake grants.

The grants are written by this test for these fakes only. Run:
  python3 -I scripts/test_mut_observe.py
"""
import datetime, json, platform, shutil, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
RUNNER = HERE / "run_mut_observe.py"
POP = 600_000
ok = failed = 0

FAKE = r'''#!/usr/bin/env python3
import hashlib, json, os, sys
args = sys.argv[1:]
g = lambda k, d=None: args[args.index(k) + 1] if k in args else d
src, root = g("--source"), g("--scratch-root")
shape = json.load(open(os.path.join(src, "shape.json")))
n = shape["shard_count"]
token = hashlib.sha256(os.urandom(8)).hexdigest()[:16]
work = os.path.join(root, f"calibmut_compact_n{n}_{os.getpid()}_{token}")
os.makedirs(work)
open(os.path.join(work, ".calibmut-owner"), "w").write(f"token={token}\nsource={src}\n")
ivs = [{"class": 0, "shard": s, "start_ns": 10 * s, "interval_to_next_mark_ns": 10, "rss_kb_at_mark": 1}
       for s in range(n)] + [{"class": c, "shard": 0, "start_ns": 100 + c, "interval_to_next_mark_ns": 1,
                              "rss_kb_at_mark": 1} for c in range(1, 5)]
obs = {"marks_valid": True, "marks_invalid_because": "", "marks": len(ivs) + 2, "other_messages": 0,
       "intervals": ivs, "rss_before_kb": 1, "rss_hwm_available": True, "rss_hwm_kb": 2,
       "disk_sample_us": int(g("--sample-us")), "disk_sampler_cpu": int(g("--sampler-cpu")),
       "disk_samples": 5, "disk_alloc_first_bytes": 1, "disk_alloc_peak_bytes": 2,
       "disk_alloc_peak_at_ns": 1, "disk_alloc_last_bytes": 1, "disk_logical_peak_bytes": 2}
doc = {"schema": "utxoz-calib-mut-1", "metric": "compact", "shard_count": n,
       "geometry_id": shape["geometry_id"], "population": shape["population"], "value_size": 33,
       "inserts": 0, "pause_every": 10000, "delete_batch": int(g("--delete-batch")),
       "setup_deletions": shape["population"] // 2, "seed": int(g("--seed")),
       "source": os.path.realpath(src), "work_dir": work, "token": token, "copy_files": 3,
       "copy_bytes": 4096, "copy_ms": 1.0, "initial_digest": "0123456789abcdef", "elapsed_ns": 1000,
       "failed_ops": 0, "batches": 0, "batch_ns_p50": 0, "batch_ns_p99": 0, "batch_ns_max": 0,
       "rotations_class48": 1, "generations_class48_before": 6, "generations_class48_after": 3,
       "version_files_before": 10, "version_files_after": 7, "logical_bytes_before": 1,
       "logical_bytes_after": 1, "entries_after": shape["population"] - shape["population"] // 2,
       "final_digest": "fedcba9876543210", "expected_digest": "fedcba9876543210", "maxrss_kb": 1,
       "observed": True, "observation": obs, "valid": True, "invalid_because": ""}
MUTATE
print(json.dumps(doc))
'''


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
         "approval_code": "fake-for-tests", "granted_by": "test_mut_observe.py",
         "expires_utc": (datetime.datetime.now(datetime.timezone.utc)
                         + datetime.timedelta(minutes=10)).strftime("%Y-%m-%dT%H:%M:%SZ")}
    g.update(over); p = root / "grant.json"; p.write_text(json.dumps(g)); return p


def run(root, b_mutate="pass", authorization="default", **over):
    parent = root / "scratch-parent"; parent.mkdir(exist_ok=True)
    src = root / "src.cpp"; src.write_text("// fake\n")
    spec = {"task_id": "TEST-FAKE-ONLY", "scratch_parent": str(parent), "min_free_gib": 10,
            "max_scratch_gib": 4, "repetitions": 2, "cpu": 0, "sampler_cpu": 1, "sample_us": 1000,
            "delete_batch": 100000, "timeout": 30, "harness_source": str(src),
            "sides": {"N1": {"harness": str(harness(root, "n1")), "source": str(source(root, "n1", 1))},
                      "N8": {"harness": str(harness(root, "n8", b_mutate)), "source": str(source(root, "n8", 8))}}}
    spec.update(over)
    sp = root / "spec.json"; sp.write_text(json.dumps(spec))
    out = root / "out"
    cmd = [sys.executable, "-I", str(RUNNER), "--spec", str(sp), "--out-dir", str(out)]
    if authorization == "default": authorization = grant(root)
    if authorization is not None: cmd += ["--authorization", str(authorization)]
    return subprocess.run(cmd, capture_output=True, text=True, timeout=600), out


def case(name, body):
    global ok, failed
    root = Path(tempfile.mkdtemp(prefix="mutobs_"))
    try: good, why = body(root)
    except Exception as e: good, why = False, f"{type(e).__name__}: {e}"
    finally: shutil.rmtree(root, ignore_errors=True)
    if good: print(f"  ok     {name}"); ok += 1
    else: print(f"  FALLA  {name}: {why}"); failed += 1


def rep(out): return json.loads((out / "report.json").read_text())


def main():
    print("=== run_mut_observe.py ===")
    def good(root):
        p, out = run(root)
        r = rep(out)
        return (p.returncode == 0 and r["verdict"] == "complete" and len(r["records"]) == 4
                and not r["left_in_scratch"] and all(x["cleanup"] == "removed" for x in r["records"]),
                p.stdout[-300:])
    case("una observación correcta: completa, sin copias sobrantes", good)
    case("sin autorización no se crea nada",
         lambda root: (lambda pr: (pr[0].returncode == 2 and not pr[1].exists(), pr[0].stdout[-200:]))(
             run(root, authorization=None)))
    case("autorización vencida",
         lambda root: (lambda pr: (pr[0].returncode == 2, pr[0].stdout[-200:]))(
             run(root, authorization=grant(root, expires_utc="2020-01-01T00:00:00Z"))))
    def bad(mutate, want):
        def body(root):
            p, out = run(root, b_mutate=mutate)
            r = rep(out)
            bad = [x for x in r["records"] if not x["valid"]]
            return (p.returncode == 3 and bad and want in (bad[0]["unusable_because"] or "")
                    and all(Path(x["work_dir"]).exists() for x in bad), str(bad[:1])[:300])
        return body
    case("marcas inválidas: la repetición falla y su copia se conserva",
         bad('obs["marks_valid"] = False; obs["marks_invalid_because"] = "x"', "marks are not valid"))
    case("intervalos de otra geometría", bad('obs["intervals"] = obs["intervals"][1:]', "one per container"))
    case("sin muestras de disco", bad('obs["disk_samples"] = 0', "disk observation"))
    case("no observó", bad('doc["observed"] = False', "did not observe"))
    case("contenido final distinto", bad('doc["final_digest"] = "0" * 16', "rule predicts"))
    def mutated(root):
        p, out = run(root, b_mutate='open(os.path.join(src, "x.dat"), "w").write("x")')
        r = rep(out)
        return p.returncode == 3 and not r["sources_after"]["N8"]["unchanged"], str(r["sources_after"])[:200]
    case("una fuente escrita invalida la corrida", mutated)
    for name, over in (("cuatro repeticiones", {"repetitions": 4}),
                       ("el muestreador en la CPU medida", {"sampler_cpu": 0}),
                       ("scratch de más de 4 GiB", {"max_scratch_gib": 5}),
                       ("scratch_parent numérico", {"scratch_parent": 123}),
                       ("scratch_parent vacío", {"scratch_parent": ""})):
        case(name, lambda root, over=over: (lambda pr: (pr[0].returncode == 2, pr[0].stdout[-200:]))(run(root, **over)))
    # Review 5637775062: harness_source checked before anything exists; output
    # of another shape is an invalid repetition, not a traceback.
    for name, value in (("harness_source vacío", ""), ("harness_source inexistente", "/nonexistent/utxoz-cr147-r2")):
        case(f"{name} se rechaza sin crear nada",
             lambda root, value=value: (lambda pr: (pr[0].returncode == 2 and "harness_source" in pr[0].stdout
                                                    and not pr[1].exists()
                                                    and not any((root / "scratch-parent").iterdir()),
                                                    pr[0].stdout[-200:]))(run(root, harness_source=value)))
    def shaped(mutate):
        def body(root):
            p, out = run(root, b_mutate=mutate)
            r = rep(out)
            bad = [x for x in r["records"] if not x["valid"]]
            return (p.returncode == 3 and "Traceback" not in p.stderr and bad
                    and all(x["work_dir"] is None or isinstance(x["work_dir"], str) for x in r["records"])
                    and all(x["token"] is None or isinstance(x["token"], str) for x in r["records"]),
                    f"exit {p.returncode}: {p.stdout[-200:]} {p.stderr[-300:]}")
        return body
    for name, mutate in (("una lista", 'print("[]"); sys.exit(0)'), ("null", 'print("null"); sys.exit(0)'),
                         ("un número", 'print("5"); sys.exit(0)'), ("work_dir numérico", 'doc["work_dir"] = 5'),
                         ("token lista", 'doc["token"] = [1]')):
        case(f"salida con otra forma ({name}): inválida, con reporte y sin traceback", shaped(mutate))
    print(f"\n  {ok} ok, {failed} fallas")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
