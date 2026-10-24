#!/usr/bin/env python3
"""Functional tests of the real mutation harness, on tiny fixtures. Untimed.

Builds a 300 000-entry image per shard count with utxoz_calib_hist, then runs
utxoz_calib_mut against it: valid runs must validate, the self-tests must fail
with exit 3, misuse must fail with exit 2, and the source image must be
byte-for-byte what it was after every run. Nothing here is a measurement; the
elapsed times the harness prints are ignored.

Everything is created under a new temporary directory inside --work-parent and
removed at the end, after the outcome is printed. Nothing outside it is written.

Run: python3 -I scripts/test_calib_mut.py --build build-grid1 --build build-grid8 \
         --work-parent DIR
"""
import argparse, hashlib, json, os, shutil, subprocess, sys, tempfile
from pathlib import Path

ok = failed = 0
# Every harness call is bounded. One that does not finish in time is a failed
# case and the next case still runs; the finally below still removes what it
# made. (The env override lets a test of this file time a fake out quickly.)
SUBPROCESS_TIMEOUT = float(os.environ.get("UTXOZ_TEST_SUBPROCESS_TIMEOUT", "600"))
# One CPU this process may run on, for the case that allows the harness a single CPU.
CPU = min(os.sched_getaffinity(0))


def store_digest(d):
    h = hashlib.sha256()
    for p in sorted(Path(d).iterdir()):
        if p.is_file() and p.name != ".utxoz.lock":
            h.update(p.name.encode()); h.update(hashlib.sha256(p.read_bytes()).digest())
    return h.hexdigest()


def call(cmd):
    """(exit, stdout, stderr); exit None when the call timed out."""
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=SUBPROCESS_TIMEOUT)
    except subprocess.TimeoutExpired:
        return None, "", f"timed out after {SUBPROCESS_TIMEOUT:g}s"
    return r.returncode, r.stdout, r.stderr


def check(name, cond, detail=""):
    global ok, failed
    if cond: print(f"  ok     {name}"); ok += 1
    else: print(f"  FALLA  {name}: {detail}"); failed += 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", action="append", required=True)
    ap.add_argument("--work-parent", required=True)
    a = ap.parse_args()
    root = Path(tempfile.mkdtemp(prefix="calibmut-functional-", dir=a.work_parent))
    print(f"=== utxoz_calib_mut, funcional, en {root} ===")
    initial = set()
    try:
        fixtures = {}
        for b in a.build:
            b = Path(b).resolve()
            fx = root / f"fx-{b.name}"
            code, _, err = call([str(b / "benchmarks" / "utxoz_calib_hist"), "--build-fixture", str(fx),
                                 "--population", "300000"])
            check(f"{b.name}: fixture de 300 000", code == 0, err[-200:])
            fixtures[b] = fx
        before = {b: store_digest(fx) if fx.is_dir() else None for b, fx in fixtures.items()}
        scratch = root / "scratch"; scratch.mkdir()

        def mut(b, *args):
            code, so, err = call([str(b / "benchmarks" / "utxoz_calib_mut"), "--scratch-root", str(scratch), *args])
            try: d = json.loads(so) if so.strip() else None
            except json.JSONDecodeError: d = None
            return code, (d if isinstance(d, dict) else None), err

        builds = list(fixtures)
        for b in builds:
            fx = fixtures[b]
            code, d, err = mut(b, "--metric", "insert_lifecycle", "--source", str(fx), "--inserts", "20000",
                               "--pause-every", "1000")
            check(f"{b.name}: inserción válida", code == 0 and d and d["valid"]
                  and d["entries_after"] == 320000 and d["final_digest"] == d["expected_digest"]
                  and d["batches"] == 20, err[-200:])
            if d:
                initial.add(d["initial_digest"])
                marker = Path(d["work_dir"]) / ".calibmut-owner"
                check(f"{b.name}: la copia está en el scratch, con marca y token",
                      Path(d["work_dir"]).parent == scratch and marker.is_file()
                      and f"token={d['token']}\n" in marker.read_text())
            one_by_one = d["final_digest"] if d else None
            # insert_batch: the same keys, payloads and heights; the same content.
            for w in ("1", "2"):
                code, d, err = mut(b, "--metric", "insert_batch", "--source", str(fx), "--inserts", "20000",
                                   "--batch-size", "1000", "--workers", w)
                sp = (d or {}).get("spread") or {}
                used = sum(1 for x in sp.get("slots", []) if x["gained"])
                check(f"{b.name}: insert_batch W{w} válida, mismo contenido que uno por uno",
                      code == 0 and d and d["valid"] and d["entries_after"] == 320000
                      and d["final_digest"] == one_by_one and sp.get("calls") == 20
                      and sp.get("inserts_counted_by_statistics") == 20000
                      and sp.get("workers_used_max") == min(int(w), used)
                      and sp.get("calls_short_of_threads") == 0, err[-200:])
                if d: initial.add(d["initial_digest"])
            code, _, _ = mut(b, "--metric", "insert_batch", "--source", str(fx), "--inserts", "5000",
                             "--selftest-insert-existing")
            check(f"{b.name}: insert_batch con una clave existente falla (3)", code == 3, f"exit {code}")
            code, _, _ = mut(b, "--metric", "insert_batch", "--source", str(fx), "--inserts", "5000",
                             "--selftest-expect-wrong")
            check(f"{b.name}: insert_batch con contenido final distinto (3)", code == 3, f"exit {code}")
            code, _, _ = mut(b, "--metric", "insert_batch", "--source", str(fx), "--workers", "0")
            check(f"{b.name}: insert_batch con 0 workers (2)", code == 2, f"exit {code}")
            code, _, _ = call(["taskset", "-c", str(CPU), str(b / "benchmarks" / "utxoz_calib_mut"),
                               "--scratch-root", str(scratch), "--metric", "insert_batch", "--source", str(fx),
                               "--workers", "2"])
            check(f"{b.name}: insert_batch con más workers que CPUs permitidas (2)", code == 2, f"exit {code}")
            code, d, err = mut(b, "--metric", "compact", "--source", str(fx), "--delete-batch", "50000")
            check(f"{b.name}: compactación válida", code == 0 and d and d["valid"]
                  and d["entries_after"] == 150000 and d["setup_deletions"] == 150000
                  and d["generations_class48_after"] <= d["generations_class48_before"], err[-200:])
            if d: initial.add(d["initial_digest"])
            code, _, _ = mut(b, "--metric", "insert_lifecycle", "--source", str(fx), "--inserts", "5000",
                             "--selftest-insert-existing")
            check(f"{b.name}: insertar una clave existente falla (3)", code == 3, f"exit {code}")
            code, _, _ = mut(b, "--metric", "insert_lifecycle", "--source", str(fx), "--inserts", "5000",
                             "--selftest-expect-wrong")
            check(f"{b.name}: contenido final distinto, inserción (3)", code == 3, f"exit {code}")
            code, _, _ = mut(b, "--metric", "compact", "--source", str(fx), "--selftest-expect-wrong")
            check(f"{b.name}: contenido final distinto, compactación (3)", code == 3, f"exit {code}")
            code, _, _ = mut(b, "--metric", "nope", "--source", str(fx))
            check(f"{b.name}: métrica inválida (2)", code == 2, f"exit {code}")
        if len(builds) > 1:
            code, _, _ = mut(builds[1], "--metric", "compact", "--source", str(fixtures[builds[0]]))
            check("una fuente de otro N se rechaza (2)", code == 2, f"exit {code}")
        dirs = [p for p in scratch.iterdir()]
        check("cada corrida tuvo su propio directorio", len({p.name for p in dirs}) == len(dirs)
              and len(dirs) >= 4 * len(builds))
        check("todas las copias arrancaron del mismo contenido, en todo N", len(initial) == 1, str(initial))
        after = {b: store_digest(fx) if fx.is_dir() else None for b, fx in fixtures.items()}
        check("las fuentes quedaron byte a byte como estaban",
              before == after and None not in before.values())
    finally:
        print(f"\n  {ok} ok, {failed} fallas")
        shutil.rmtree(root, ignore_errors=True)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
