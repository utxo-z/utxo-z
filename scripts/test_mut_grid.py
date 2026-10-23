#!/usr/bin/env python3
"""End-to-end tests for run_mut_grid.py, with fake harnesses that really run.

The fake does what the real harness does to the filesystem - creates its own
work directory inside the scratch root, writes an owner marker with its token,
writes a copy - and prints a document. Each case twists one thing and checks
that the runner, its cleanup, or the family verifier notices.

Run: python3 -I scripts/test_mut_grid.py
"""
import datetime, hashlib, json, platform, shutil, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
RUNNER = HERE / "run_mut_grid.py"
VERIFIER = HERE / "recompute_aa_family.py"
STATS = HERE.parent.parent / "wt-bench" / "scripts" / "bench_analyze.py"
POP = 600_000
ok = failed = 0

FAKE = r'''#!/usr/bin/env python3
import hashlib, json, os, sys
args = sys.argv[1:]
g = lambda k, d=None: args[args.index(k) + 1] if k in args else d
src, root, metric = g("--source"), g("--scratch-root"), g("--metric")
shape = json.load(open(os.path.join(src, "shape.json")))
seed = int(g("--seed")); inserts = int(g("--inserts"))
token = hashlib.sha256(os.urandom(8)).hexdigest()[:16]
TOKEN_OVERRIDE
work = os.path.join(root, f"calibmut_{metric}_n{shape['shard_count']}_{os.getpid()}_{token}")
os.makedirs(work, exist_ok=True)
open(os.path.join(work, ".calibmut-owner"), "w").write(f"token={token}\nsource={src}\n")
open(os.path.join(work, "cont_0_v00000.dat"), "wb").write(b"x" * 4096)
ins = metric == "insert_lifecycle"
elapsed = (1000 + 100 * shape["shard_count"]) * 1000
doc = {"schema": "utxoz-calib-mut-1", "metric": metric, "shard_count": shape["shard_count"],
       "geometry_id": shape["geometry_id"], "population": shape["population"], "value_size": 33,
       "inserts": inserts if ins else 0, "pause_every": int(g("--pause-every")),
       "delete_batch": int(g("--delete-batch")),
       "setup_deletions": 0 if ins else shape["population"] // 2, "seed": seed,
       "source": os.path.realpath(src), "work_dir": work, "token": token,
       "copy_files": 3, "copy_bytes": 4096, "copy_ms": 12.5, "initial_digest": "0123456789abcdef",
       "elapsed_ns": elapsed, "failed_ops": 0,
       "batches": inserts // int(g("--pause-every")) if ins else 0,
       "batch_ns_p50": 10, "batch_ns_p99": 20, "batch_ns_max": 30, "rotations_class48": 8,
       "generations_class48_before": 48, "generations_class48_after": 56,
       "version_files_before": 52, "version_files_after": 60,
       "logical_bytes_before": 100, "logical_bytes_after": 110,
       "entries_after": shape["population"] + inserts if ins else shape["population"] - shape["population"] // 2,
       "final_digest": "fedcba9876543210", "expected_digest": "fedcba9876543210",
       "maxrss_kb": 1000, "valid": True, "invalid_because": ""}
MUTATE
print(json.dumps(doc))
'''


def make_source(root, name, shards):
    fx = root / f"src-{name}"; fx.mkdir()
    (fx / "shape.json").write_text(json.dumps({
        "schema": "utxoz-hist-shape-2", "population": POP, "historical_prefix": 500000,
        "active_entries": 90000, "generations_class_48": 6 * shards, "shard_count": shards,
        "geometry_id": 3 if shards == 1 else 0x80000010 + shards, "value_size": 33}))
    (fx / "census.json").write_text("{}")
    (fx / "cont_0_v00000.dat").write_bytes(b"s" * 4096)
    return fx


def make_harness(root, name, mutate="pass", token_override=""):
    h = root / f"fake-{name}"
    h.write_text(FAKE.replace("MUTATE", mutate).replace("TOKEN_OVERRIDE", token_override))
    h.chmod(0o755)
    return h


def setup(root, b_mutate="pass", b_token="", **over):
    sides = {"N1": {"harness": str(make_harness(root, "n1")), "source": str(make_source(root, "n1", 1))},
             "N8": {"harness": str(make_harness(root, "n8", b_mutate, b_token)),
                    "source": str(make_source(root, "n8", 8))}}
    cells = [{"name": "aa-ins", "kind": "aa", "metric": "insert_lifecycle", "A": "N1", "B": "N1"},
             {"name": "N8-ins", "kind": "ab", "metric": "insert_lifecycle", "A": "N1", "B": "N8"},
             {"name": "N8-cmp", "kind": "ab", "metric": "compact", "A": "N1", "B": "N8"}]
    parent = root / "scratch-parent"; parent.mkdir()
    src = root / "src.cpp"; src.write_text("// fake\n")
    spec = {"pairs": 4, "inserts": 100000, "pause_every": 10000, "delete_batch": 100000,
            "cpu": 0, "timeout": 30, "resamples": 200, "confidence": 0.95, "bootstrap_seed": 5,
            "min_free_gib": 10, "max_scratch_gib": 4, "stats_module": str(STATS),
            "harness_source": str(src), "scratch_parent": str(parent), "sides": sides, "cells": cells,
            "task_id": TASK}
    spec.update(over)
    return spec


TASK = "TEST-FAKE-ONLY"


def grant(root, **over):
    """A grant for these fakes only, written by the test into its own temp dir.

    A real campaign's grant comes from the coordinator; nothing in this file or
    the runner writes one for a real task.
    """
    g = {"schema": "utxoz-campaign-authorization-1", "host": platform.node(),
         "task_id": TASK, "approval_code": "fake-for-tests", "granted_by": "test_mut_grid.py",
         "expires_utc": (datetime.datetime.now(datetime.timezone.utc)
                         + datetime.timedelta(minutes=10)).strftime("%Y-%m-%dT%H:%M:%SZ")}
    g.update(over)
    path = root / "grant.json"
    path.write_text(json.dumps({k: v for k, v in g.items() if v is not None}))
    return path


def run(spec, root, authorization="default"):
    sp = root / "spec.json"; sp.write_text(json.dumps(spec))
    out = root / "out"
    cmd = [sys.executable, "-I", str(RUNNER), "--spec", str(sp), "--out-dir", str(out)]
    if authorization == "default":
        authorization = grant(root)
    if authorization is not None:
        cmd += ["--authorization", str(authorization)]
    p = subprocess.run(cmd, capture_output=True, text=True)
    return p, out


def verify(out, cells, root):
    p = subprocess.run([sys.executable, "-I", str(VERIFIER), "--stats-module", str(STATS),
                        "--runner", str(RUNNER), "--out", str(root / "fam.json"),
                        "--expect-family", ",".join(cells),
                        *[str(out / c / "report.json") for c in cells]],
                       capture_output=True, text=True)
    return p


def case(name, body):
    global ok, failed
    root = Path(tempfile.mkdtemp(prefix="mutgrid_"))
    try:
        good, why = body(root)
    except Exception as e:
        good, why = False, f"{type(e).__name__}: {e}"
    finally:
        shutil.rmtree(root, ignore_errors=True)
    if good: print(f"  ok     {name}"); ok += 1
    else: print(f"  FALLA  {name}: {why}"); failed += 1


def report(out, cell):
    return json.loads((out / cell / "report.json").read_text())


def main():
    if not STATS.is_file():
        print(f"stats module not found at {STATS}"); return 2
    print("=== run_mut_grid.py ===")

    def good(root):
        p, out = run(setup(root), root)
        if p.returncode != 0: return False, f"runner exit {p.returncode}: {p.stdout[-300:]}"
        hk = json.loads((out / "housekeeping.json").read_text())
        if hk["left_in_scratch"]: return False, f"scratch not clean: {hk['left_in_scratch']}"
        if any(e["cleanup"] != "removed" for e in hk["entries"]): return False, "a copy was not removed"
        v = verify(out, ["N8-ins", "N8-cmp"], root)
        if v.returncode != 0: return False, f"verifier exit {v.returncode}: {v.stdout[-300:]}"
        fam = json.loads((root / "fam.json").read_text())
        if fam["sample_parser"]["runner_contract"] != ["check_sample_record", "expect_from_config", "verify_report"]:
            return False, "contract not used"
        return True, ""
    case("una grilla correcta mide, limpia sus copias y verifica", good)

    def source_mutated(root):
        spec = setup(root, b_mutate='open(os.path.join(src, "intruso.dat"), "w").write("x")')
        p, out = run(spec, root)
        r = report(out, "N8-ins")
        return p.returncode == 3 and "source changed" in r.get("verdict", ""), p.stdout[-200:]
    case("un lado que escribe en su fuente invalida las celdas que la usan", source_mutated)

    def inherits(root):
        spec = setup(root, b_token='token = "aaaaaaaaaaaaaaaa"')
        p, out = run(spec, root)
        if p.returncode != 0: return False, f"runner exit {p.returncode}"
        v = verify(out, ["N8-ins"], root)
        return v.returncode == 3 and "reused a work directory" in v.stdout, v.stdout[-200:]
    case("un lado que reusa su directorio de trabajo: herencia rechazada", inherits)

    def other_initial(root):
        spec = setup(root, b_mutate='doc["initial_digest"] = "1111111111111111"')
        p, out = run(spec, root)
        v = verify(out, ["N8-ins"], root)
        return v.returncode == 3 and "different content" in v.stdout, v.stdout[-200:]
    case("un lado que arranca de otro contenido", other_initial)

    def digest_mismatch(root):
        spec = setup(root, b_mutate='doc["final_digest"] = "0000000000000000"')
        p, out = run(spec, root)
        r = report(out, "N8-ins")
        hk = json.loads((out / "housekeeping.json").read_text())
        kept = [e for e in hk["entries"] if e["cleanup"].startswith("kept")]
        return (p.returncode == 3 and "verdict" in r and kept and all(Path(e["work_dir"]).exists() for e in kept),
                p.stdout[-200:])
    case("contenido final distinto del predicho: inválida y la copia fallida se conserva", digest_mismatch)

    def failed_ops(root):
        spec = setup(root, b_mutate='doc["failed_ops"] = 1')
        p, out = run(spec, root)
        return p.returncode == 3 and "verdict" in report(out, "N8-ins"), p.stdout[-200:]
    case("una operación que no se completó", failed_ops)

    def wrong_entries(root):
        spec = setup(root, b_mutate='doc["entries_after"] += 1')
        p, out = run(spec, root)
        return p.returncode == 3, p.stdout[-200:]
    case("conteo final de entradas incorrecto", wrong_entries)

    def wrong_geometry(root):
        spec = setup(root, b_mutate='doc["geometry_id"] = 3')
        p, out = run(spec, root)
        return p.returncode == 3, p.stdout[-200:]
    case("el lado B informa otra geometría", wrong_geometry)

    def outside_scratch(root):
        outside = root / "ajeno"; outside.mkdir()
        spec = setup(root, b_mutate=f'''
import shutil
alt = os.path.join({str(outside)!r}, os.path.basename(work)); shutil.copytree(work, alt); doc["work_dir"] = alt''')
        p, out = run(spec, root)
        hk = json.loads((out / "housekeeping.json").read_text())
        foreign = [e for e in hk["entries"] if str(outside) in e["work_dir"]]
        return (foreign and all("not directly inside" in e["cleanup"] for e in foreign)
                and all(Path(e["work_dir"]).exists() for e in foreign), str(foreign)[:200])
    case("un directorio fuera del scratch de la corrida no se borra", outside_scratch)

    def wrong_marker(root):
        spec = setup(root, b_mutate='open(os.path.join(work, ".calibmut-owner"), "w").write("token=otro\\n")')
        p, out = run(spec, root)
        hk = json.loads((out / "housekeeping.json").read_text())
        bad = [e for e in hk["entries"] if "another token" in e["cleanup"]]
        return bad and all(Path(e["work_dir"]).exists() for e in bad), str(hk["entries"][:2])[:200]
    case("una marca de propiedad con otro token: no se borra", wrong_marker)

    def no_space(root):
        p, out = run(setup(root, min_free_gib=10**6), root)
        hk = json.loads((out / "housekeeping.json").read_text())
        return p.returncode == 3 and "free space" in (hk["stopped"] or ""), p.stdout[-200:]
    case("sin espacio libre suficiente la corrida se detiene antes de copiar", no_space)

    def pairs_11(root):
        p, _ = run(setup(root, pairs=11), root)
        return p.returncode == 2, p.stdout[-200:]
    case("pares por encima de 10", pairs_11)

    def scratch_budget(root):
        p, _ = run(setup(root, max_scratch_gib=5), root)
        return p.returncode == 2, p.stdout[-200:]
    case("presupuesto de scratch por encima de 4 GiB", scratch_budget)

    def raw_vs_record(root):
        p, out = run(setup(root), root)
        if p.returncode != 0: return False, "runner"
        raw = out / "N8-ins" / "raw" / "insert_lifecycle-pair01-B.json"
        d = json.loads(raw.read_text()); d["elapsed_ns"] += 7
        text = json.dumps(d) + "\n"; raw.write_text(text)
        rp = out / "N8-ins" / "report.json"; r = json.loads(rp.read_text())
        for s in r["samples"]:
            if s["pair"] == 1 and s["side"] == "B": s["raw_sha256"] = hashlib.sha256(text.encode()).hexdigest()
        rp.write_text(json.dumps(r, indent=1))
        v = verify(out, ["N8-ins"], root)
        return v.returncode == 3, v.stdout[-200:]
    case("verificador: un crudo con otro tiempo que su registro, hash rehecho", raw_vs_record)

    # --- the authorization guard ---------------------------------------------
    def refused(make_grant, want):
        def body(root):
            spec = setup(root)
            p, out = run(spec, root, authorization=make_grant(root))
            nothing = not out.exists() and not any(Path(spec["scratch_parent"]).iterdir())
            return (p.returncode == 2 and want in p.stdout and nothing,
                    "exit %d, out exists=%s: %s" % (p.returncode, out.exists(), p.stdout[-200:]))
        return body

    def unreadable(r):
        (r / "g.json").write_text("{nope")
        return r / "g.json"

    def in_days(n):
        return (datetime.datetime.now(datetime.timezone.utc)
                + datetime.timedelta(days=n)).strftime("%Y-%m-%dT%H:%M:%SZ")

    case("sin autorización no se crea ni se copia nada", refused(lambda r: None, "no authorization file"))
    case("autorización inexistente", refused(lambda r: r / "no-such.json", "does not exist"))
    case("autorización ilegible", refused(unreadable, "not readable JSON"))
    case("autorización vencida", refused(lambda r: grant(r, expires_utc="2020-01-01T00:00:00Z"), "expired"))
    case("autorización para otro host", refused(lambda r: grant(r, host="otra-maquina"), "for host"))
    case("autorización para otra tarea", refused(lambda r: grant(r, task_id="TAREA07"), "for task"))
    case("autorización sin código de aprobación", refused(lambda r: grant(r, approval_code=""), "approval_code"))
    case("autorización con fecha sin Z", refused(lambda r: grant(r, expires_utc="2099-01-01T00:00:00"),
                                                 "not YYYY"))
    case("autorización de más de un día", refused(lambda r: grant(r, expires_utc=in_days(3)),
                                                  "longer than a day"))

    def revoked_mid_run(root):
        # The fake harness for B deletes the grant after its first sample: the
        # coordinator revoking the window. The next sample must not start.
        g = grant(root)
        mutate = "p = " + repr(str(g)) + "\nif os.path.exists(p): os.remove(p)"
        spec = setup(root, b_mutate=mutate)
        p, out = run(spec, root, authorization=g)
        hk = json.loads((out / "housekeeping.json").read_text())
        ran = len(hk["entries"])
        return (p.returncode == 3 and "authorization no longer valid" in (hk["stopped"] or "")
                and ran < 2 * 4 * 3, "exit %d ran=%d stopped=%s" % (p.returncode, ran, hk["stopped"]))
    case("autorización revocada a mitad de campaña: se detiene antes de la siguiente muestra",
         revoked_mid_run)

    def grant_recorded(root):
        g = grant(root)
        p, out = run(setup(root), root, authorization=g)
        cfg = json.loads((out / "N8-ins" / "config.json").read_text())
        a = cfg.get("authorization", {})
        return (p.returncode == 0 and a.get("sha256") == hashlib.sha256(g.read_bytes()).hexdigest()
                and a.get("task_id") == TASK, str(a)[:200])
    case("la autorización queda registrada por hash en cada config", grant_recorded)

    print(f"\n  {ok} ok, {failed} fallas")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
