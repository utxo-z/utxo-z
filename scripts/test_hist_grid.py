#!/usr/bin/env python3
"""End-to-end tests for run_hist_grid.py, with fake harnesses that really run.

Each case builds fake fixtures (a shape.json and a census.json per side), a fake
harness that answers from its fixture's shape, a spec, and runs the real runner
and, where it matters, the real family verifier. The rejections are the point:
a side measured against the wrong expectation, two sides that asked for
different lookups, a block order that was not the declared one, a store that
moved.

Run: python3 -I scripts/test_hist_grid.py
"""
import hashlib, json, os, shutil, subprocess, sys, tempfile, threading
from pathlib import Path

HERE = Path(__file__).resolve().parent
RUNNER = HERE / "run_hist_grid.py"
VERIFIER = HERE / "recompute_aa_family.py"
STATS = HERE / "bench_analyze.py"   # the repository's own estimator

POP = 6_000_000
ok = failed = 0

# Answers from the fixture's own shape, so each side reports its own geometry.
# The probe digest depends only on what the two sides must share - unless
# DIGEST_EXTRA mixes in something side-specific.
FAKE = r'''#!/usr/bin/env python3
import hashlib, json, os, sys
args = sys.argv[1:]
g = lambda k, d=None: args[args.index(k) + 1] if k in args else d
shape = json.load(open(os.path.join(g("--fixture"), "shape.json")))
ops, warm, batch = int(g("--ops")), int(g("--warmup")), int(g("--batch"))
pct, seed, below, metric = int(g("--extra-pct")), int(g("--seed")), int(g("--probe-below")), g("--metric")
extra = ops * pct // 100; total = ops + extra
digest = hashlib.sha256(f"{metric}|{seed}|{below}|{ops}|{warm}|{batch}|{extra}DIGEST_EXTRA".encode()).hexdigest()[:16]
hit = metric == "hist_hit"
elapsed = (2000 + 100 * shape["shard_count"]) * ops
doc = {"schema": "utxoz-calib-hist-2", "metric": metric, "shard_count": shape["shard_count"],
       "ops": ops, "extra_ops": extra, "extra_pct": pct, "total_ops": total, "warmup": warm,
       "batch": batch, "resolve_calls": total // batch, "population_observed": shape["population"],
       "historical_prefix": shape["historical_prefix"], "active_entries": shape["active_entries"],
       "probe_below": below, "probe_digest": digest, "geometry_id": shape["geometry_id"],
       "generations_class_48": shape["generations_class_48"], "version_files_all_classes": 63,
       "version_files_class_48": 59, "value_size": 33, "seed": seed, "query_height": 9000000,
       "elapsed_ns": elapsed, "ns_per_op": round(elapsed / ops, 4), "resolved_by_find": 0,
       "found": total if hit else 0, "absent": 0 if hit else total, "not_found_from_find": 0,
       "real_errors": 0, "first_error_code": 0,
       "warmup_found": warm if hit else 0, "warmup_absent": 0 if hit else warm,
       "warmup_resolved_by_find": 0,
       "verified_payload": total if hit else 0, "verified_height": total if hit else 0,
       "verified_absent": 0 if hit else total, "valid": True, "invalid_because": ""}
MUTATE
print(json.dumps(doc))
'''


def make_side(root, name, shards, prefix):
    fx = root / f"fx-{name}"; fx.mkdir()
    (fx / "shape.json").write_text(json.dumps({
        "schema": "utxoz-hist-shape-2", "population": POP, "historical_prefix": prefix,
        "active_entries": POP - prefix, "generations_class_48": 59 * shards,
        "shard_count": shards, "geometry_id": 3 if shards == 1 else 0x80000010 + shards,
        "value_size": 33}))
    (fx / "census.json").write_text("{}")
    (fx / "cont_0_v00000.dat").write_bytes(b"x" * 64)
    return fx


def make_harness(root, name, mutate="pass", digest_extra=""):
    h = root / f"fake-{name}"
    h.write_text(FAKE.replace("MUTATE", mutate).replace("DIGEST_EXTRA", digest_extra))
    h.chmod(0o755)
    return h


def base_spec(root, sides, cells, **over):
    src = root / "src.cpp"; src.write_text("// fake\n")
    spec = {"pairs": 4, "ops": 80_000, "warmup": 4_000, "batch": 4_000, "value_size": 33,
            "probe_below": 5_900_000, "cpu": 0, "timeout": 30, "resamples": 200,
            "confidence": 0.95, "bootstrap_seed": 5, "stats_module": str(STATS),
            "harness_source": str(src), "sides": sides, "cells": cells}
    spec.update(over)
    return spec


def standard(root, b_mutate="pass", b_digest=""):
    sides = {
        "N1": {"harness": str(make_harness(root, "n1")),
               "fixture": str(make_side(root, "n1", 1, 5_924_294))},
        "N8": {"harness": str(make_harness(root, "n8", b_mutate, b_digest)),
               "fixture": str(make_side(root, "n8", 8, 5_914_791))},
    }
    cells = [
        {"name": "aa-hit", "kind": "aa", "metric": "hist_hit", "A": "N1", "B": "N1"},
        {"name": "control-hit", "kind": "control", "metric": "hist_hit", "A": "N1", "B": "N1",
         "extra_pct_b": 5},
        {"name": "N8-hit", "kind": "ab", "metric": "hist_hit", "A": "N1", "B": "N8"},
        {"name": "N8-miss", "kind": "ab", "metric": "hist_miss", "A": "N1", "B": "N8"},
    ]
    return sides, cells


def run(spec, root):
    sp = root / "spec.json"; sp.write_text(json.dumps(spec))
    out = root / "out"
    p = subprocess.run([sys.executable, "-I", str(RUNNER), "--spec", str(sp), "--out-dir", str(out)],
                       capture_output=True, text=True, timeout=900)
    return p, out


def verify(out, cells, family_name="fam"):
    target = out.parent / f"{family_name}.json"
    p = subprocess.run([sys.executable, "-I", str(VERIFIER), "--stats-module", str(STATS),
                        "--runner", str(RUNNER), "--out", str(target),
                        "--expect-family", ",".join(c.split("|")[0] for c in cells),
                        *[str(out / c.split("|")[0] / "report.json") for c in cells]],
                       capture_output=True, text=True, timeout=900)
    return p


def case(name, body):
    global ok, failed
    root = Path(tempfile.mkdtemp(prefix="histgrid_"))
    try:
        good, why = body(root)
    except Exception as e:  # a case that raises is a case that failed
        good, why = False, f"{type(e).__name__}: {e}"
    finally:
        shutil.rmtree(root, ignore_errors=True)
    if good:
        print(f"  ok     {name}"); ok += 1
    else:
        print(f"  FALLA  {name}: {why}"); failed += 1


def rewrite(out, cell, change):
    rp = out / cell / "report.json"; r = json.loads(rp.read_text()); change(r)
    rp.write_text(json.dumps(r, indent=1))


def main():
    if not STATS.is_file():
        print(f"stats module not found at {STATS}"); return 2
    print("=== run_hist_grid.py ===")

    def good_grid(root):
        sides, cells = standard(root)
        p, out = run(base_spec(root, sides, cells), root)
        if p.returncode != 0: return False, f"runner exit {p.returncode}: {p.stdout[-300:]}"
        plan = json.loads((out / "schedule.json").read_text())
        firsts = [s["first"] for s in plan if s["cell"] == "N8-hit"]
        if firsts != ["A", "B", "B", "A"]: return False, f"block order {firsts}"
        rounds = [s["cell"] for s in plan if s["round"] == 2]
        if rounds[0] != "control-hit": return False, f"round 2 not rotated: {rounds}"
        v = verify(out, ["N8-hit|hist_hit", "N8-miss|hist_miss"])
        if v.returncode != 0: return False, f"verifier exit {v.returncode}: {v.stdout[-300:]}"
        fam = json.loads((root / "fam.json").read_text())
        if {i["verification"] for i in fam["inputs"]} != {"verified-current"}:
            return False, "not verified-current"
        if fam["sample_parser"]["runner_contract"] != ["check_sample_record", "expect_from_config",
                                                       "verify_report"]:
            return False, "contract not used"
        return True, ""
    case("una grilla correcta corre, intercala, ordena en bloques y verifica", good_grid)

    def b_wrong_shards(root):
        sides, cells = standard(root, b_mutate='doc["shard_count"] = 4')
        p, out = run(base_spec(root, sides, cells), root)
        r = json.loads((out / "N8-hit" / "report.json").read_text())
        return (p.returncode == 3 and "verdict" in r
                and any("shard_count" in (f["unusable_because"] or "") for f in r["failures"]),
                p.stdout[-300:])
    case("el lado B informa otro N que el de su fixture", b_wrong_shards)

    def b_wrong_geometry(root):
        sides, cells = standard(root, b_mutate='doc["geometry_id"] = 3')
        p, out = run(base_spec(root, sides, cells), root)
        r = json.loads((out / "N8-hit" / "report.json").read_text())
        return (p.returncode == 3 and any("geometry_id" in (f["unusable_because"] or "")
                                          for f in r["failures"]), p.stdout[-300:])
    case("el lado B informa otra geometría", b_wrong_geometry)

    def digest_differs(root):
        sides, cells = standard(root, b_digest='|side-specific')
        p, out = run(base_spec(root, sides, cells), root)
        r = json.loads((out / "N8-hit" / "report.json").read_text())
        aa = json.loads((out / "aa-hit" / "report.json").read_text())
        return (p.returncode == 3 and "verdict" in r and "verdict" not in aa
                and any("digests differ" in (f["unusable_because"] or "") for f in r["failures"]),
                p.stdout[-300:])
    case("los dos lados no pidieron las mismas claves", digest_differs)

    def b_probe_below_changed(root):
        sides, cells = standard(root, b_mutate='doc["probe_below"] = 5_000_000')
        p, out = run(base_spec(root, sides, cells), root)
        return p.returncode == 3, p.stdout[-200:]
    case("el lado B dibuja de otro span", b_probe_below_changed)

    def below_too_high(root):
        sides, cells = standard(root)
        p, _ = run(base_spec(root, sides, cells, probe_below=5_920_000), root)
        return p.returncode == 2 and "exceeds" in p.stdout, p.stdout[-200:]
    case("probe_below por encima del prefijo de un lado", below_too_high)

    def control_without_extra(root):
        sides, cells = standard(root)
        cells[1]["extra_pct_b"] = 0
        p, _ = run(base_spec(root, sides, cells), root)
        return p.returncode == 2, p.stdout[-200:]
    case("un control sin trabajo extra", control_without_extra)

    def ab_with_extra(root):
        sides, cells = standard(root)
        cells[2]["extra_pct_b"] = 5
        p, _ = run(base_spec(root, sides, cells), root)
        return p.returncode == 2, p.stdout[-200:]
    case("una comparación con trabajo extra", ab_with_extra)

    def ab_reversed(root):
        sides, cells = standard(root)
        cells[2]["A"], cells[2]["B"] = "N8", "N1"
        p, _ = run(base_spec(root, sides, cells), root)
        return p.returncode == 2 and "baseline" in p.stdout, p.stdout[-200:]
    case("una comparación cuyo A no es N=1", ab_reversed)

    def pairs_21(root):
        sides, cells = standard(root)
        p, _ = run(base_spec(root, sides, cells, pairs=21), root)
        return p.returncode == 2, p.stdout[-200:]
    case("pares por encima del tope", pairs_21)

    def existing_out(root):
        sides, cells = standard(root)
        (root / "out").mkdir()
        p, _ = run(base_spec(root, sides, cells), root)
        return p.returncode == 2, p.stdout[-200:]
    case("directorio de salida ya existente", existing_out)

    # --- the verifier, per side ------------------------------------------------
    def tampered(change, want):
        def body(root):
            sides, cells = standard(root)
            p, out = run(base_spec(root, sides, cells), root)
            if p.returncode != 0: return False, f"runner exit {p.returncode}"
            rewrite(out, "N8-hit", change)
            v = verify(out, ["N8-hit|hist_hit"])
            return v.returncode == 3 and want in v.stdout, v.stdout[-300:]
        return body

    def raw_digest_only(root):
        # Only the raw changes: its digest, nothing else, and its hash in the
        # report is redone so the raw still "belongs". The summary record keeps
        # saying both sides asked the same thing.
        sides, cells = standard(root)
        p, out = run(base_spec(root, sides, cells), root)
        if p.returncode != 0: return False, f"runner exit {p.returncode}"
        raw = out / "N8-hit" / "raw" / "hist_hit-pair01-B.json"
        d = json.loads(raw.read_text()); d["probe_digest"] = "f" * 16
        text = json.dumps(d) + "\n"; raw.write_text(text)
        def fix_hash(r):
            for s in r["samples"]:
                if s["pair"] == 1 and s["side"] == "B":
                    s["raw_sha256"] = hashlib.sha256(text.encode()).hexdigest()
        rewrite(out, "N8-hit", fix_hash)
        v = verify(out, ["N8-hit|hist_hit"])
        return (v.returncode == 3 and "the record says probe_digest" in v.stdout,
                v.stdout[-300:])
    case("verificador: el crudo B dice otro digest que su registro (sólo el crudo)",
         raw_digest_only)

    def side_shape_inconsistent(r):
        r["config"]["sides"]["B"]["active_entries"] = POP
        probe = {k: v for k, v in r["config"].items() if k != "config_sha256"}
        r["config"]["config_sha256"] = hashlib.sha256(
            json.dumps(probe, sort_keys=True).encode()).hexdigest()
    case("verificador: un lado con activas imposibles para su prefijo, hash rehecho",
         tampered(side_shape_inconsistent, "active entries"))

    def other_digest(r):
        for s in r["samples"]:
            if s["side"] == "B" and s["pair"] == 1: s["probe_digest"] = "0" * 16
    case("verificador: un registro B con otro digest",
         tampered(other_digest, "did not ask for the same lookups"))

    def other_order(r):
        for s in r["samples"]:
            if s["pair"] == 2: s["ran_first"] = "A"
    case("verificador: un par que no corrió en el orden declarado",
         tampered(other_order, "declared block order"))

    def b_store_moved(r):
        r["image"]["B"]["store_sha256_after"] = "0" * 64
    case("verificador: el store del lado B cambió", tampered(b_store_moved, "store hashes"))

    def b_unchanged_false(r):
        r["image"]["B"]["unchanged"] = False
    case("verificador: imagen B marcada cambiada", tampered(b_unchanged_false, "store changed"))

    def ab_claims_extra(r):
        r["config"]["sides"]["B"]["extra_pct"] = 5
        probe = {k: v for k, v in r["config"].items() if k != "config_sha256"}
        r["config"]["config_sha256"] = hashlib.sha256(
            json.dumps(probe, sort_keys=True).encode()).hexdigest()
    case("verificador: comparación que declara trabajo extra, hash del config rehecho",
         tampered(ab_claims_extra, "must not carry control work"))

    # --- review 5638263526 ---------------------------------------------------
    # A cell name is one plain directory name of its own, refused before --out-dir exists.
    def bad_name(name):
        def body(root):
            sides, cells = standard(root)
            if name is KeyError: del cells[0]["name"]
            else: cells[0]["name"] = name
            spec = base_spec(root, sides, cells)
            before = sorted(os.listdir(root))
            p, out = run(spec, root)
            new = sorted(set(os.listdir(root)) - set(before) - {"spec.json"})
            return (p.returncode == 2 and "plain directory name" in p.stdout and not out.exists()
                    and new == [] and "Traceback" not in p.stderr, f"exit {p.returncode}, new {new} {p.stdout[-200:]}")
        return body
    for label, name in (("ausente", KeyError), ("vacío", ""), ("'.'", "."), ("'..'", ".."),
                        ("'../escape'", "../escape"), ("'a/b'", "a/b"), ("'spec.json'", "spec.json"),
                        ("'schedule.json'", "schedule.json"), ("numérico", 5)):
        case(f"nombre de celda {label}: rechazado sin crear ni escribir nada", bad_name(name))
    # A side whose shape the verifier would refuse is refused before any sample.
    def bad_shape(field, value, want):
        def body(root):
            sides, cells = standard(root)
            shape_path = Path(sides["N8"]["fixture"]) / "shape.json"
            shape = json.loads(shape_path.read_text()); shape[field] = value
            shape_path.write_text(json.dumps(shape))
            p, out = run(base_spec(root, sides, cells), root)
            return (p.returncode == 2 and want in p.stdout and not out.exists()
                    and "Traceback" not in p.stderr, f"exit {p.returncode}: {p.stdout[-200:]}")
        return body
    case("N=8 con 6 generaciones: rechazado antes de medir",
         bad_shape("generations_class_48", 6, "not every shard has a history"))
    case("activas que no caben sobre el prefijo: rechazado antes de medir",
         bad_shape("active_entries", POP, "cannot all sit at or above"))
    case("un prefijo que no es una historia: rechazado antes de medir",
         bad_shape("historical_prefix", POP, "is not a history"))
    case("un campo de forma que no es entero: rechazado antes de medir, sin traceback",
         bad_shape("shard_count", "8", "shard_count is '8'"))
    # The spec is read once: given through a pipe that can be read only once, the
    # run completes and records exactly the bytes it validated.
    def spec_once(root):
        sides, cells = standard(root)
        text = json.dumps(base_spec(root, sides, cells), indent=2) + "\n"
        fifo = root / "spec.fifo"; os.mkfifo(fifo)
        def feed():
            with open(fifo, "w") as f: f.write(text)
        threading.Thread(target=feed, daemon=True).start()
        out = root / "out"
        try:
            p = subprocess.run([sys.executable, "-I", str(RUNNER), "--spec", str(fifo), "--out-dir", str(out)],
                               capture_output=True, text=True, timeout=120)
        except subprocess.TimeoutExpired:
            return False, "the runner read the spec a second time and blocked on the pipe"
        if p.returncode != 0: return False, f"exit {p.returncode}: {p.stdout[-200:]}"
        cfg = json.loads((out / "aa-hit" / "config.json").read_text())
        return ((out / "spec.json").read_text() == text
                and cfg.get("spec_sha256") == hashlib.sha256(text.encode()).hexdigest(), "spec bytes differ")
    case("el spec se lee una vez: spec.json y spec_sha256 son los bytes validados", spec_once)

    # --- review 5638467033: the harness binaries and source that ran ---------
    def changed_during(mutate_for, want):
        def body(root):
            sides, cells = standard(root, b_mutate=mutate_for(root))
            p, out = run(base_spec(root, sides, cells), root)
            r = json.loads((out / "N8-hit" / "report.json").read_text())
            prov = r.get("provenance_after", {})
            return (p.returncode == 3 and "Traceback" not in p.stderr and want in r.get("verdict", "")
                    and set(prov) == {"harness:A", "harness:B", "harness_source"}
                    and all({"path", "sha256_before", "sha256_after"} <= set(v) for v in prov.values()),
                    f"exit {p.returncode}, verdict {r.get('verdict')!r} {p.stderr[-200:]}")
        return body
    case("el binario B cambia durante la corrida: NO MEASUREMENT, hashes antes/después registrados",
         changed_during(lambda root: 'open(sys.argv[0], "a").write("# rebuilt\\n")', "harness:B changed during the run"))
    case("el binario B desaparece durante la corrida: NO MEASUREMENT, sin traceback",
         changed_during(lambda root: 'os.remove(sys.argv[0])', "harness:B was gone or unreadable"))
    case("la fuente del harness cambia durante la corrida: NO MEASUREMENT",
         changed_during(lambda root: f'open({str(root / "src.cpp")!r}, "a").write("// edited\\n")',
                        "harness_source changed during the run"))
    def prov_after(r):
        r["provenance_after"]["harness:B"]["sha256_after"] = "0" * 64
    case("verificador: un hash de binario posterior alterado no verifica",
         tampered(prov_after, "harness:B changed during the run"))
    def prov_before(r):
        r["provenance_after"]["harness:B"]["sha256_before"] = "0" * 64
        r["provenance_after"]["harness:B"]["sha256_after"] = "0" * 64
    case("verificador: antes y después alterados juntos no coinciden con el config",
         tampered(prov_before, "recorded hash of harness:B before the run is not the one the config names"))
    def prov_source(r):
        r["provenance_after"]["harness_source"]["sha256_after"] = "0" * 64
    case("verificador: un hash de fuente posterior alterado no verifica",
         tampered(prov_source, "harness_source changed during the run"))
    def prov_missing(r):
        del r["provenance_after"]
    case("verificador: un reporte sin hashes posteriores (anterior a este registro) no se certifica",
         tampered(prov_missing, "cannot be certified"))
    def prov_field_missing(r):
        del r["provenance_after"]["harness:A"]["sha256_after"]
    case("verificador: un campo posterior faltante no verifica",
         tampered(prov_field_missing, "harness:A was gone or unreadable"))
    def prov_label_missing(r):
        del r["provenance_after"]["harness_source"]
    case("verificador: un archivo que falta en el registro no verifica",
         tampered(prov_label_missing, "the after-run record names"))

    print(f"\n  {ok} ok, {failed} fallas")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
