#!/usr/bin/env python3
"""End-to-end tests for run_hist_pilot.py, with fake harnesses that really run.

Each case writes a small executable that prints a document, runs the real runner
against it, and checks the exit code and what the report says. The point is the
rejections: a runner that accepts a document describing an operation that did not
happen produces a figure nobody can audit, and that has happened here before.

Run: python3 -I scripts/test_hist_pilot.py
"""
import json, os, shutil, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
RUNNER = HERE / "run_hist_pilot.py"
STATS = HERE / "bench_analyze.py"   # the repository's own estimator

OPS, WARMUP, BATCH, POP = 8_000, 4_000, 4_000, 6_000_000
ACTIVE_FROM, GENS = 5_924_294, 59

ok = failed = 0


def document(side_extra_pct, metric="hist_hit", **over):
    """A document a correct harness would print for one sample."""
    ops = over.pop("ops", OPS)
    extra = ops * side_extra_pct // 100
    total = ops + extra
    elapsed = 2400 * ops
    d = {
        "schema": "utxoz-calib-hist-1", "metric": metric, "shard_count": 1,
        "ops": ops, "extra_ops": extra, "extra_pct": side_extra_pct,
        "total_ops": total, "warmup": WARMUP, "batch": BATCH,
        "resolve_calls": total // BATCH, "population_observed": POP,
        "active_from": ACTIVE_FROM, "history_entries": ACTIVE_FROM,
        "generations_class_48": GENS, "version_files_all_classes": 63,
        "version_files_class_48": GENS, "value_size": 33, "seed": 1,
        "query_height": 9_000_000, "elapsed_ns": elapsed,
        "ns_per_op": round(elapsed / ops, 4),
        "resolved_by_find": 0,
        "found": total if metric == "hist_hit" else 0,
        "absent": 0 if metric == "hist_hit" else total,
        "not_found_from_find": 0, "real_errors": 0, "first_error_code": 0,
        "warmup_found": WARMUP if metric == "hist_hit" else 0,
        "warmup_absent": 0 if metric == "hist_hit" else WARMUP,
        "warmup_resolved_by_find": 0,
        "verified_payload": total if metric == "hist_hit" else 0,
        "verified_height": total if metric == "hist_hit" else 0,
        "verified_absent": 0 if metric == "hist_hit" else total,
        "valid": True, "invalid_because": "",
    }
    d.update(over)
    return d


def write_harness(path, body):
    path.write_text("#!/usr/bin/env python3\nimport json,sys\n" + body)
    path.chmod(0o755)


def fixture_at(d, extra_file=None):
    d.mkdir(parents=True)
    (d / "shape.json").write_text(json.dumps(
        {"schema": "utxoz-hist-shape-1", "population": POP,
         "active_from": ACTIVE_FROM, "generations_class_48": GENS,
         "value_size": 33}))
    (d / "cont_0_v00000.dat").write_bytes(b"x" * 64)
    if extra_file:
        (d / extra_file).write_bytes(b"y" * 8)
    return d


# The generic fake: echoes back the seed and extra-pct it was given, so a runner
# that says it checks them is actually tested. `MUTATE` is the per-case twist.
GENERIC = """
args = sys.argv[1:]
g = lambda k, d=None: args[args.index(k) + 1] if k in args else d
doc = DOCUMENT
doc["seed"] = int(g("--seed", 1))
doc["metric"] = g("--metric", "hist_hit")
pct = int(g("--extra-pct", 0))
# Read back what the runner actually asked for, so a runner that passes one
# thing and expects another is caught rather than papered over.
ops = int(g("--ops", doc["ops"]))
doc["ops"] = ops
doc["warmup"] = int(g("--warmup", doc["warmup"]))
doc["batch"] = int(g("--batch", doc["batch"]))
doc["warmup_found"] = doc["warmup"] if doc["metric"] == "hist_hit" else 0
doc["warmup_absent"] = 0 if doc["metric"] == "hist_hit" else doc["warmup"]
doc["extra_pct"] = pct
doc["extra_ops"] = ops * pct // 100
doc["total_ops"] = ops + doc["extra_ops"]
doc["resolve_calls"] = doc["total_ops"] // doc["batch"]
if doc["metric"] == "hist_hit":
    doc["found"] = doc["total_ops"]
    doc["verified_payload"] = doc["verified_height"] = doc["total_ops"]
else:
    doc["absent"] = doc["verified_absent"] = doc["total_ops"]
    doc["found"] = doc["verified_payload"] = doc["verified_height"] = 0
    doc["warmup_found"], doc["warmup_absent"] = 0, doc["warmup"]
doc["elapsed_ns"] = 2400 * ops
doc["ns_per_op"] = round(doc["elapsed_ns"] / ops, 4)
MUTATE
print(json.dumps(doc))
sys.exit(EXITCODE)
"""


def case(name, mutate="pass", exitcode=0, want_exit=3, pairs=2, kind="aa",
         extra_pct=5, metric="hist_hit", runner_args=(), pre=None, raw=None,
         check=None, out_absent=False):
    global ok, failed
    tmp = Path(tempfile.mkdtemp(prefix="histtest_"))
    try:
        fx = fixture_at(tmp / "fx")
        h = tmp / "fake"
        if raw is not None:
            write_harness(h, raw)
        else:
            write_harness(h, GENERIC
                          .replace("DOCUMENT", repr(document(0, metric)))
                          .replace("MUTATE", mutate)
                          .replace("EXITCODE", str(exitcode)))
        src = tmp / "src.cpp"; src.write_text("// fake source\n")
        outd = tmp / "out"
        if pre:
            pre(outd, fx)
        cmd = [sys.executable, "-I", str(RUNNER),
               "--harness", str(h), "--harness-source", str(src),
               "--metric", metric, "--fixture", str(fx),
               "--stats-module", str(STATS), "--kind", kind,
               "--pairs", str(pairs), "--ops", str(OPS), "--warmup", str(WARMUP),
               "--batch", str(BATCH), "--extra-pct", str(extra_pct),
               "--population", str(POP), "--out-dir", str(outd),
               "--timeout", "30", *runner_args]
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
        detail = ""
        good = p.returncode == want_exit
        if good and out_absent and outd.exists():
            good, detail = False, "the refused run left its output directory behind"
        if good and check:
            rep = outd / "report.json"
            good, detail = check(json.loads(rep.read_text())) if rep.is_file() \
                           else (False, "no report.json")
        if good:
            print(f"  ok     {name}")
            ok += 1
        else:
            print(f"  FALLA  {name}: exit {p.returncode}, esperaba {want_exit}. {detail}")
            print("         " + (p.stdout.strip().splitlines() or [""])[-1][:160])
            failed += 1
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


# --------------------------------------------------------------- the verifier
#
# The runner checks each side when it measures. These check that the family
# verifier does too when it re-reads, using the runner's own contract: each
# tamper below is internally consistent - the raw's arithmetic adds up and its
# hash in the report is redone - so only a check of what the side was OWED can
# catch it.

VERIFIER = HERE / "recompute_aa_family.py"


def produce_run(tmp, kind, metric="hist_hit"):
    fx = fixture_at(tmp / "fx")
    h = tmp / "fake"
    write_harness(h, GENERIC.replace("DOCUMENT", repr(document(0, metric)))
                            .replace("MUTATE", "pass").replace("EXITCODE", "0"))
    src = tmp / "src.cpp"; src.write_text("// fake source\n")
    outd = tmp / "out"
    p = subprocess.run(
        [sys.executable, "-I", str(RUNNER), "--harness", str(h),
         "--harness-source", str(src), "--metric", metric, "--fixture", str(fx),
         "--stats-module", str(STATS), "--kind", kind, "--pairs", "2",
         "--ops", "80000", "--warmup", str(WARMUP), "--batch", str(BATCH),
         "--extra-pct", "5", "--population", str(POP), "--out-dir", str(outd),
         "--timeout", "30"], capture_output=True, text=True, timeout=600)
    if p.returncode != 0:
        raise RuntimeError(f"could not produce a run: {p.stdout[-300:]}")
    return outd


def sha(text):
    import hashlib
    return hashlib.sha256(text.encode()).hexdigest()


def rewrite_raws(outd, side, change):
    """Edit every raw of one side, keeping it consistent and re-hashing it."""
    rep_path = outd / "report.json"
    rep = json.loads(rep_path.read_text())
    for rec in rep["samples"]:
        if rec["side"] != side:
            continue
        f = outd / "raw" / f"{rep['config']['metric']}-pair{rec['pair']:02d}-{side}.json"
        d = json.loads(f.read_text())
        change(d)
        text = json.dumps(d) + "\n"
        f.write_text(text)
        rec["raw_sha256"] = sha(text)
    rep_path.write_text(json.dumps(rep, indent=1))


def edit_report(outd, change):
    rep_path = outd / "report.json"
    rep = json.loads(rep_path.read_text())
    change(rep)
    rep_path.write_text(json.dumps(rep, indent=1))


def verify(outd, metric="hist_hit"):
    out = outd.parent / "family.json"
    p = subprocess.run(
        [sys.executable, "-I", str(VERIFIER), "--stats-module", str(STATS),
         "--runner", str(RUNNER), "--out", str(out), "--expect-family", metric,
         str(outd / "report.json")], capture_output=True, text=True, timeout=600)
    return p.returncode, p.stdout


def b_without_extra(d):
    ops = d["ops"]
    d["extra_pct"] = 0
    d["extra_ops"] = 0
    d["total_ops"] = ops
    d["resolve_calls"] = ops // d["batch"]
    d["found"] = d["verified_payload"] = d["verified_height"] = ops


def other_batch(d):
    d["batch"] = 2000
    d["resolve_calls"] = d["total_ops"] // 2000


def verifier_case(name, kind, tamper, want_exit, want_text=""):
    global ok, failed
    tmp = Path(tempfile.mkdtemp(prefix="histverify_"))
    try:
        try:
            outd = produce_run(tmp, kind)
            tamper(outd)
            code, out = verify(outd)
        except Exception as e:  # a case that raises or times out is a case that failed
            code, out = None, f"{type(e).__name__}: {e}"
        good = code == want_exit and want_text in out
        if good:
            print(f"  ok     {name}")
            ok += 1
        else:
            print(f"  FALLA  {name}: exit {code}, esperaba {want_exit}")
            print("         " + " | ".join(out.strip().splitlines()[-2:])[:200])
            failed += 1
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def recompute_config_hash(cfg):
    probe = {k: v for k, v in cfg.items() if k != "config_sha256"}
    cfg["config_sha256"] = sha(json.dumps(probe, sort_keys=True))


def verifier_cases():
    print("=== recompute_aa_family.py con el contrato del runner ===")
    nothing = lambda outd: None
    verifier_case("un control intacto verifica", "control", nothing, 0)
    verifier_case("un A/A intacto verifica", "aa", nothing, 0)

    verifier_case("control: B reescrito a 0% extra en crudos y registros, hash rehecho",
                  "control",
                  lambda o: (rewrite_raws(o, "B", b_without_extra),
                             edit_report(o, lambda r: [s.update(extra_pct=0)
                                                       for s in r["samples"]])),
                  3, "extra_pct")
    verifier_case("control: B reescrito a 0% sólo en los crudos, hash rehecho",
                  "control", lambda o: rewrite_raws(o, "B", b_without_extra),
                  3, "extra_pct")
    verifier_case("batch distinto en todos los crudos, consistente y rehasheado",
                  "control",
                  lambda o: (rewrite_raws(o, "A", other_batch),
                             rewrite_raws(o, "B", other_batch)),
                  3, "batch")
    verifier_case("image.unchanged en false, sin verdict", "control",
                  lambda o: edit_report(o, lambda r: r["image"].update(unchanged=False)),
                  3, "store changed")
    verifier_case("hash del store después distinto aunque diga unchanged", "control",
                  lambda o: edit_report(o, lambda r: r["image"].update(
                      store_sha256_after="0" * 64)),
                  3, "store hashes")
    verifier_case("un archivo más en el store después", "aa",
                  lambda o: edit_report(o, lambda r: r["image"].update(
                      files_after=r["image"]["files_after"] + 1)),
                  3, "file counts")
    verifier_case("sin registro de imagen", "aa",
                  lambda o: edit_report(o, lambda r: r.pop("image")),
                  3, "image record")

    def aa_claims_control(o):
        def ch(r):
            r["config"]["extra_pct_side_b"] = 5
            recompute_config_hash(r["config"])
        edit_report(o, ch)
    verifier_case("A/A que declara 5% en B con el hash del config rehecho", "aa",
                  aa_claims_control, 3, "A/A run declares")


def main():
    if not STATS.is_file():
        print(f"stats module not found at {STATS}"); return 2
    print("=== run_hist_pilot.py ===")

    # --- what must be accepted -------------------------------------------
    case("una corrida A/A correcta pasa", want_exit=0,
         check=lambda r: (len(r["ratios"]) == 2 and r["image"]["unchanged"],
                          "ratios/image"))
    case("una corrida de control correcta pasa", kind="control", want_exit=0,
         runner_args=("--ops", "80000"),
         check=lambda r: (r["config"]["extra_pct_side_b"] == 5
                          and all(s["extra_pct"] == (5 if s["side"] == "B" else 0)
                                  for s in r["samples"]),
                          "el lado B debe llevar el trabajo extra"))
    case("hist_miss correcto pasa", metric="hist_miss", want_exit=0)

    # --- the control's own bookkeeping ------------------------------------
    # The mutation runs inside the fake harness, where only doc exists: a name
    # it does not define would fail the sample with a NameError and pass here.
    case("extra_ops que no es el porcentaje declarado",
         mutate='doc["extra_ops"] += doc["batch"]',
         check=lambda r: (bool(r["failures"]) and all(
             "extra_ops" in (f["unusable_because"] or "") and "% of ops" in (f["unusable_because"] or "")
             for f in r["failures"]), str([f["unusable_because"] for f in r["failures"]][:2])))
    case("total_ops que no es la suma", mutate='doc["total_ops"] += 1')
    case("resolve_calls que no cuadra con los lotes",
         mutate='doc["resolve_calls"] += 1')
    case("total_ops que no es un numero entero de lotes",
         mutate='doc["total_ops"] += 1; doc["extra_ops"] += 1')
    case("el lado B no hace el trabajo extra pedido", kind="control",
         runner_args=("--ops", "80000"),
         mutate='doc["extra_pct"] = 0; doc["extra_ops"] = 0; '
                'doc["total_ops"] = ops; doc["resolve_calls"] = ops // doc["batch"]; '
                'doc["found"] = doc["verified_payload"] = doc["verified_height"] = ops')

    # --- the historical contract ------------------------------------------
    case("una sonda contestada por las versiones activas",
         mutate='doc["resolved_by_find"] = 1; doc["found"] -= 1')
    case("find devolvio not_found",
         mutate='doc["not_found_from_find"] = 1; doc["found"] -= 1')
    case("los desenlaces no parten las consultas", mutate='doc["found"] -= 1')
    case("un miss que resolve no probo ausente", metric="hist_miss",
         mutate='doc["absent"] -= 1; doc["not_found_from_find"] = 1')
    case("un miss que encontro una clave jamas insertada", metric="hist_miss",
         mutate='doc["found"] = doc["total_ops"]; doc["absent"] = 0')
    case("un miss con menos ausencias confirmadas", metric="hist_miss",
         mutate='doc["verified_absent"] -= 1')
    case("un hit sin verificar todos los payloads",
         mutate='doc["verified_payload"] -= 1')
    case("un hit sin verificar todas las alturas",
         mutate='doc["verified_height"] -= 1')
    case("un hit que llamo ausente a una entrada guardada",
         mutate='doc["absent"] = 1; doc["found"] -= 1')
    case("sin historia: una sola generacion",
         mutate='doc["generations_class_48"] = 1')
    case("sin historia: la activa empieza en cero",
         mutate='doc["active_from"] = 0')
    case("la frontera activa esta fuera de la poblacion",
         mutate='doc["active_from"] = doc["population_observed"]')

    # --- the generic failures the accepted runner also rejects -------------
    case("ns_per_op que no es elapsed/ops", mutate='doc["ns_per_op"] += 0.5')
    case("warmup que no suma", mutate='doc["warmup_found"] -= 1')
    case("verificado por encima de las operaciones",
         mutate='doc["verified_payload"] = doc["total_ops"] + 1')
    case("un contador negativo", mutate='doc["found"] = -1')
    case("un contador flotante", mutate='doc["resolve_calls"] = 2.0')
    case("un contador booleano", mutate='doc["real_errors"] = False')
    case("errores reales distintos de cero", mutate='doc["real_errors"] = 1')
    case("valid en false", mutate='doc["valid"] = False; '
                                  'doc["invalid_because"] = "a proposito"')
    case("valid que no es un bool", mutate='doc["valid"] = "true"')
    case("elapsed_ns en cero", mutate='doc["elapsed_ns"] = 0')
    case("esquema inesperado", mutate='doc["schema"] = "otra-cosa"')
    case("metrica inesperada", mutate='doc["metric"] = "find_hit"')
    case("exit distinto de cero con documento valido", exitcode=1,
         check=lambda r: (bool(r["failures"]) and all(
                              f["unusable_because"] == "exit 1" and f["ns_per_op"] is None
                              for f in r["failures"]),
                          "cada falla tiene que decir 'exit 1' y no llevar medida"))
    # Refused inputs leave no output directory behind for the retry to trip over.
    case("un modulo de estadistica que no existe no deja --out-dir", want_exit=1,
         runner_args=("--stats-module", "/nonexistent/stats.py"), out_absent=True)
    case("una fuente del harness que no existe no deja --out-dir", want_exit=2,
         runner_args=("--harness-source", "/nonexistent/source.cpp"), out_absent=True)
    case("siempre informa la semilla 1", mutate='doc["seed"] = 1', pairs=2,
         runner_args=())
    case("Infinity pasa json.loads pero no el runner",
         raw='print(\'{"schema":"utxoz-calib-hist-1","ns_per_op":Infinity}\')\n'
             'import sys; sys.exit(0)\n')
    case("una lista en vez de un objeto",
         raw='print("[1,2,3]")\nimport sys; sys.exit(0)\n')
    case("sin salida", raw='import sys; sys.exit(0)\n')

    # --- the image, and the runner's own ranges ---------------------------
    # The lock carries a pid and a time, so the library rewrites it on every
    # open. A guard that calls that a changed store vetoes every honest run -
    # which is exactly what a first version of this runner did.
    case("que el lock se mueva NO invalida la corrida",
         mutate='import os; '
                'open(os.path.join(g("--fixture"), ".utxoz.lock"), "w")'
                '.write("%d 123" % os.getpid())',
         want_exit=0,
         check=lambda r: (r["image"]["unchanged"] and r["image"]["lock_moved"],
                          "el store debia seguir intacto y el lock marcado movido"))
    case("un byte distinto en el store invalida la corrida",
         mutate='import os; '
                'open(os.path.join(g("--fixture"), "cont_0_v00000.dat"), "wb")'
                '.write(b"z" * 64)',
         want_exit=3,
         check=lambda r: (not r["image"]["unchanged"]
                          and "stored bytes changed" in r["verdict"],
                          "debia detectar el cambio del store"))
    case("un archivo nuevo en el store invalida la corrida",
         mutate='import os; '
                'open(os.path.join(g("--fixture"), "intruso.dat"), "w").write("x")',
         want_exit=3,
         check=lambda r: (not r["image"]["unchanged"]
                          and r["image"]["files_after"] > r["image"]["files_before"],
                          "debia contar un archivo mas"))
    case("directorio de salida ya existente", want_exit=2,
         pre=lambda outd, fx: outd.mkdir(parents=True))
    case("pares por encima del tope", pairs=21, want_exit=2)
    case("control con extra-pct 0", kind="control", extra_pct=0, want_exit=2)
    case("extra-pct que no da lotes enteros", kind="control", extra_pct=7,
         runner_args=("--ops", "80000"), want_exit=2)
    case("ops que no es multiplo de batch", want_exit=2,
         runner_args=("--ops", "4001"))
    case("timeout NaN", want_exit=2, runner_args=("--timeout", "nan"))
    case("confianza fuera de rango", want_exit=2,
         runner_args=("--confidence", "1.5"))
    case("remuestreos insuficientes", want_exit=2,
         runner_args=("--resamples", "10"))

    verifier_cases()

    print(f"\n  {ok} ok, {failed} fallas")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
