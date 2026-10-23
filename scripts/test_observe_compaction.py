#!/usr/bin/env python3
"""Functional tests of utxoz_calib_mut --observe-compaction, on tiny images. Untimed.

The figures it prints are ignored: these check that the observation is
complete and honest, not what it observed.

  - the mark parser and validator pass their own self-test (complete, missing,
    duplicated, out of order, no completion, clocks back, incompatible text);
  - an observed compaction validates its content like an unobserved one, and
    records exactly one mark per container of the geometry, in order;
  - a compaction with nothing to merge still records the full sequence, and its
    generations do not change;
  - observation is refused outside compaction;
  - the source images are byte-for-byte unchanged.

Everything is created under a new temporary directory inside --work-parent and
removed at the end. Run:
  python3 -I scripts/test_observe_compaction.py --build build-obs1 --build build-obs8 \
      --work-parent DIR
"""
import argparse, hashlib, json, os, shutil, subprocess, sys, tempfile
from pathlib import Path

ok = failed = 0
# Every harness call is bounded. One that does not finish in time is a failed
# case and the next case still runs; the finally below still removes what it
# made. (The env override lets a test of this file time a fake out quickly.)
SUBPROCESS_TIMEOUT = float(os.environ.get("UTXOZ_TEST_SUBPROCESS_TIMEOUT", "600"))
CLASSES = 5


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
    root = Path(tempfile.mkdtemp(prefix="observe-functional-", dir=a.work_parent))
    print(f"=== --observe-compaction, funcional, en {root} ===")
    try:
        builds = [Path(b).resolve() for b in a.build]
        code, so, err = call([str(builds[0] / "benchmarks" / "utxoz_calib_mut"), "--selftest-marks"])
        check("autotest del parser y validador de marcas", code == 0, so + err)
        scratch = root / "scratch"; scratch.mkdir()
        for b in builds:
            fx = root / f"fx-{b.name}"
            code, _, err = call([str(b / "benchmarks" / "utxoz_calib_hist"), "--build-fixture", str(fx),
                                 "--population", "300000"])
            shape = json.loads((fx / "shape.json").read_text()) if code == 0 else {}
            check(f"{b.name}: fixture de 300 000", code == 0, err[-200:])
            n = shape.get("shard_count", 0)
            before = store_digest(fx) if fx.is_dir() else None

            def mut(*args):
                code, so, err = call([str(b / "benchmarks" / "utxoz_calib_mut"), "--metric", "compact",
                                      "--source", str(fx), "--scratch-root", str(scratch), *args])
                try: d = json.loads(so) if so.strip() else None
                except json.JSONDecodeError: d = None
                return code, (d if isinstance(d, dict) else None), err

            code, d, err = mut("--observe-compaction", "--sample-us", "2000", "--delete-batch", "50000")
            obs = (d or {}).get("observation") or {}
            ivs = obs.get("intervals", [])
            want = [(0, s) for s in range(n)] + [(c, 0) for c in range(1, CLASSES)]
            check(f"{b.name}: compactación observada válida", code == 0 and d and d["valid"] and d["observed"]
                  and d["final_digest"] == d["expected_digest"], err[-300:])
            check(f"{b.name}: una marca por contenedor, en orden ({len(want)} + inicio y fin)",
                  obs.get("marks_valid") is True and obs.get("marks") == len(want) + 2
                  and [(i["class"], i["shard"]) for i in ivs] == want, str(obs)[:300])
            check(f"{b.name}: intervalos no negativos y crecientes en el tiempo",
                  all(i["interval_to_next_mark_ns"] >= 0 for i in ivs)
                  and all(x["start_ns"] <= y["start_ns"] for x, y in zip(ivs, ivs[1:])))
            check(f"{b.name}: el muestreo de disco tomó muestras y el pico no es menor que el primero",
                  obs.get("disk_samples", 0) >= 2
                  and obs.get("disk_alloc_peak_bytes", 0) >= obs.get("disk_alloc_first_bytes", 1))
            check(f"{b.name}: RSS declarado (high-water mark disponible o no)",
                  isinstance(obs.get("rss_hwm_available"), bool))
            merged = d and d["generations_class48_before"] > d["generations_class48_after"]
            check(f"{b.name}: hubo fusiones", bool(merged))

            code, d, err = mut("--observe-compaction", "--sample-us", "2000", "--skip-delete-setup")
            obs = (d or {}).get("observation") or {}
            check(f"{b.name}: sin nada que fusionar, la secuencia sigue completa y válida",
                  code == 0 and d and d["valid"] and obs.get("marks_valid") is True
                  and obs.get("marks") == len(want) + 2, err[-300:])
            check(f"{b.name}: sin nada que fusionar, las generaciones no cambian",
                  d and d["generations_class48_before"] == d["generations_class48_after"]
                  and d["setup_deletions"] == 0, str(d and d.get("generations_class48_after")))

            code, _, _ = call([str(b / "benchmarks" / "utxoz_calib_mut"), "--metric", "insert_lifecycle",
                               "--source", str(fx), "--scratch-root", str(scratch), "--observe-compaction"])
            check(f"{b.name}: observar fuera de compactación se rechaza (2)", code == 2, f"exit {code}")
            check(f"{b.name}: la fuente quedó byte a byte como estaba",
                  before is not None and fx.is_dir() and store_digest(fx) == before)
    finally:
        print(f"\n  {ok} ok, {failed} fallas")
        shutil.rmtree(root, ignore_errors=True)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
