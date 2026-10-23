#!/usr/bin/env python3
"""Arguments the harnesses must refuse before doing any work. Untimed.

Each case runs a built harness in a new, empty working directory, bounded by a
timeout, and requires exit 2, the reason on stderr, a quick answer, and the
directory still empty afterwards - no fixture, no copy, no database. A control
per guard shows that the value just inside the limit is not refused by it.

Run:
  python3 -I scripts/test_harness_cli.py --build BUILD_DIR --shards N

N is the build's shard count (8, or its UTXOZ_EXPERIMENTAL_SHARD_COUNT): a
historical fixture of another count is refused before the guard under test. It
is checked against the build's CMakeCache.txt.
"""
import argparse, json, os, shutil, subprocess, sys, tempfile, time
from pathlib import Path

ok = failed = 0
QUICK_SECONDS = 10


def check(name, good, why=""):
    global ok, failed
    if good: print(f"  ok     {name}"); ok += 1
    else: print(f"  FALLA  {name}: {why}"); failed += 1


def run(cmd, prepare=None):
    """(exit, stderr, seconds, entries left in the working directory)."""
    cwd = Path(tempfile.mkdtemp(prefix="harness-cli-"))
    try:
        if prepare: prepare(cwd)
        before = set(os.listdir(cwd))
        t0 = time.monotonic()
        p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=60)
        left = sorted(set(os.listdir(cwd)) - before)
        return p.returncode, p.stderr, time.monotonic() - t0, left
    finally:
        shutil.rmtree(cwd, ignore_errors=True)


def refused(name, cmd, reason, prepare=None):
    code, err, secs, left = run(cmd, prepare)
    check(name, code == 2 and reason in err and secs < QUICK_SECONDS and not left,
          f"exit {code}, {secs:.1f}s, left {left}, stderr {err.strip()[-160:]!r}")


def shape_fixture(population, prefix, shard_count):
    def make(cwd):
        fx = cwd / "fx"; fx.mkdir()
        (fx / "shape.json").write_text(json.dumps({
            "schema": "utxoz-hist-shape-2", "population": population,
            "historical_prefix": prefix, "active_entries": population - prefix,
            "generations_class_48": 2, "shard_count": shard_count,
            "geometry_id": 4, "value_size": 33}, indent=2))
    return make


def build_shards(build):
    """The shard count build's CMakeCache.txt names: its experimental count, or 8."""
    for line in (build / "CMakeCache.txt").read_text().splitlines():
        if line.startswith("UTXOZ_EXPERIMENTAL_SHARD_COUNT:"):
            return int(line.split("=", 1)[1] or 8)
    return 8


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", required=True)
    ap.add_argument("--shards", type=int, required=True, help="the build's shard count")
    a = ap.parse_args()
    if a.shards != build_shards(Path(a.build).resolve()):
        print(f"--shards {a.shards} is not the build's shard count, {build_shards(Path(a.build).resolve())}")
        return 2
    b = Path(a.build).resolve() / "benchmarks"
    print("=== harness arguments refused before any work ===")

    large = str(b / "utxoz_benchmarks_large")
    for value, why in (("-1", "a negative number"), ("40000x", "trailing text"), ("", "nothing"),
                       ("0", "zero"), ("99999999999999999999999", "an overflow"),
                       ("50000001", "above the production run")):
        refused(f"large: --max-entries={value} ({why})", [large, "--smoke", f"--max-entries={value}"],
                "--max-entries must be a whole number")
    refused("large: --max-entries without --smoke", [large, "--max-entries=40000"],
            "--max-entries only applies with --smoke")
    code, out_err, secs, left = run([large, "--help"])
    check("large: control, --help answers 0 and does nothing", code == 0 and not left, f"exit {code}")

    hist = str(b / "utxoz_calib_hist")
    # Read from shape.json alone: the batch is checked before the image is opened.
    from_shape = lambda pop, prefix: shape_fixture(pop, prefix, a.shards)
    common = ["--fixture", "fx", "--ops", "4000", "--warmup", "4000", "--batch", "4000"]
    refused("hist: --batch above --probe-below (hist_hit)",
            [hist, "--metric", "hist_hit", *common, "--probe-below", "100"],
            "exceeds the 100 distinct keys", from_shape(10_000, 5_000))
    refused("hist: --batch above the population (hist_miss)",
            [hist, "--metric", "hist_miss", *common],
            "exceeds the 1000 distinct keys", from_shape(1_000, 500))
    code, err, secs, left = run([hist, "--metric", "hist_hit", *common], from_shape(10_000, 5_000))
    check("hist: control, a batch inside the span gets past the guard (and fails on the fake image)",
          code != 2 and "distinct keys" not in err and secs < QUICK_SECONDS,
          f"exit {code}, stderr {err.strip()[-160:]!r}")

    code, err, secs, left = run([hist, "--selftest-write-file"])
    check("hist: a write to a full device or a missing directory is reported (self-test)",
          code == 0 and not left and secs < QUICK_SECONDS, f"exit {code}, stderr {err.strip()[-160:]!r}")

    code, out, secs, left = run([hist, "--selftest-draw"])
    check("hist: the warmup draws its own stream, not the timed run's first batches (self-test)",
          code == 0 and not left and secs < QUICK_SECONDS, f"exit {code}")

    mut = str(b / "utxoz_calib_mut")
    base = [mut, "--metric", "compact", "--observe-compaction", "--source", "src", "--scratch-root", "."]
    for value, why in (("-1", "a negative number"), ("abc", "text"), ("99999", "above CPU_SETSIZE"),
                       ("", "nothing")):
        refused(f"mut: --sampler-cpu {value!r} ({why})", base + ["--sampler-cpu", value],
                "--sampler-cpu must be a CPU number")
    cpus = sorted(os.sched_getaffinity(0))
    if len(cpus) >= 2:
        refused("mut: --sampler-cpu outside the CPUs this process may run on",
                ["taskset", "-c", str(cpus[0]), *base, "--sampler-cpu", str(cpus[1])],
                "is not one of the CPUs this process may run on")
        code, err, secs, left = run(["taskset", "-c", str(cpus[0]), *base, "--sampler-cpu", str(cpus[0])])
        check("mut: control, an allowed --sampler-cpu gets past the guard (and fails on the missing source)",
              code != 0 and "sampler-cpu" not in err and not left, f"exit {code}, stderr {err.strip()[-160:]!r}")
    else:
        print("  SKIP   mut: affinity cases need two CPUs")

    print(f"\n  {ok} ok, {failed} fallas")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
