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
# A harness that does not answer within this is a failed case, never a hang.
# (The env override lets a test of this file time a fake out in a second.)
CALL_TIMEOUT = float(os.environ.get("UTXOZ_TEST_CLI_TIMEOUT", "60"))
TIMED_OUT = None      # run()'s exit for a call that timed out
last_timed_out = None # set by run(); check() fails the case it belongs to


def check(name, good, why=""):
    global ok, failed, last_timed_out
    # A timed-out call fails its case whatever the condition says: "exit != 2"
    # must not read a timeout as an answer.
    if last_timed_out:
        good, why = False, last_timed_out
    last_timed_out = None
    if good: print(f"  ok     {name}"); ok += 1
    else: print(f"  FALLA  {name}: {why}"); failed += 1


def run(cmd, prepare=None):
    """(exit, stderr, seconds, entries left in the working directory). A call
    that times out returns exit TIMED_OUT and marks the next check() failed;
    the working directory is removed either way."""
    global last_timed_out
    cwd = Path(tempfile.mkdtemp(prefix="harness-cli-"))
    try:
        if prepare: prepare(cwd)
        before = set(os.listdir(cwd))
        t0 = time.monotonic()
        try:
            p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=CALL_TIMEOUT)
        except subprocess.TimeoutExpired as e:
            err = e.stderr.decode(errors="replace") if isinstance(e.stderr, bytes) else (e.stderr or "")
            left = sorted(set(os.listdir(cwd)) - before)
            last_timed_out = f"timed out after {CALL_TIMEOUT:g}s, left {left}, stderr {err.strip()[-160:]!r}"
            return TIMED_OUT, f"timed out after {CALL_TIMEOUT:g}s; {err}", time.monotonic() - t0, left
        last_timed_out = None
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

    stationary = str(b / "utxoz_calib")
    code, _, secs, left = run([stationary, "--selftest-write-file"])
    check("stationary: a --out write to a full device or a missing directory is reported (self-test)",
          code == 0 and not left and secs < QUICK_SECONDS, f"exit {code}")
    refused("stationary: --out in a directory that does not exist",
            [stationary, "--metric", "find_hit", "--out", "/nonexistent-utxoz-dir/out.json"],
            "its directory does not exist")

    mut = str(b / "utxoz_calib_mut")
    code, err_out, secs, left = run([mut, "--selftest-owner-marker"])
    check("mut: an owner marker that cannot be written fully is reported, and only an empty "
          "unmarked directory is removed (self-test)",
          code == 0 and not left and secs < QUICK_SECONDS, f"exit {code}")
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

    # Review 5638467033: --out in a missing directory is refused before any image
    # is opened or built; a bare file name (the current directory) is accepted.
    refused("hist: --out in a missing directory, before the fixture is opened",
            [hist, "--metric", "hist_hit", "--fixture", "no-such-fixture", "--out", "/nonexistent-utxoz-dir/r.json"],
            "its directory does not exist")
    code, err, secs, left = run([hist, "--build-fixture", "new-fx", "--population", "10",
                                 "--out", "/nonexistent-utxoz-dir/r.json"])
    check("hist: --out in a missing directory, before any image is built (new-fx not created)",
          code == 2 and "its directory does not exist" in err and not left and secs < QUICK_SECONDS,
          f"exit {code}, left {left}, {err[-160:]}")
    code, err, secs, left = run([hist, "--metric", "hist_hit", "--fixture", "no-such-fixture", "--out", "r.json"])
    check("hist: control, --out r.json (current directory) passes; the missing fixture is what fails",
          code != 0 and "its directory does not exist" not in err and not left, f"exit {code}, {err[-160:]}")
    # --batch-size is insert_batch's: 0 is refused there and ignored by insert_lifecycle
    # (no division by it - see mark_capacity's static_asserts).
    refused("mut: insert_batch --batch-size 0", [mut, "--metric", "insert_batch", "--batch-size", "0",
                                                 "--source", "src", "--scratch-root", "."], "must be positive")
    code, err, secs, left = run([mut, "--metric", "insert_lifecycle", "--batch-size", "0",
                                 "--source", "no-such-source", "--scratch-root", "."])
    check("mut: insert_lifecycle ignores --batch-size 0 (gets past every argument check)",
          code is not None and code != 2 and "batch-size" not in err and code >= 0 and not left, f"exit {code}, {err[-160:]}")

    # Numeric options in all three harnesses: the whole value, digits only, in
    # range, or a usage error before any work. Zero is a value, not an error.
    numeric = {"hist": [hist, "--metric", "hist_hit", "--fixture", "fx"],
               "stationary": [stationary, "--metric", "find_hit"],
               "mut": [mut, "--metric", "compact", "--source", "src", "--scratch-root", "."]}
    option = {"hist": "--ops", "stationary": "--ops", "mut": "--inserts"}
    for h, base in numeric.items():
        opt = option[h]
        refused(f"{h}: {opt} with no value", base + [opt], "needs a value")
        for value, why in (("-1", "a negative number"), ("40000x", "trailing text"), ("abc", "text"),
                           ("", "nothing"), ("99999999999999999999999", "an overflow"),
                           ("1099511627777", "above 2^40"), ("+5", "a sign")):
            refused(f"{h}: {opt} {value!r} ({why})", base + [opt, value], "must be a whole number")
        refused(f"{h}: --seed above 64 bits", base + ["--seed", "18446744073709551616"], "must be a whole number")
        refused(f"{h}: --seed -1", base + ["--seed", "-1"], "must be a whole number")
    refused("hist: --value-size above 1 MiB", numeric["hist"] + ["--value-size", "1048577"], "must be a whole number")
    # Controls: 0 parses, and only the harness's own rules then speak.
    code, err, secs, left = run(numeric["stationary"] + ["--ops", "0", "--warmup", "0", "--seed", "0"])
    check("stationary: control, --ops 0 --warmup 0 --seed 0 parse; --ops 0 is refused by its own rule",
          code == 2 and "must be positive" in err and "whole number" not in err and not left, err[-160:])
    code, err, secs, left = run(numeric["mut"][:1] + ["--metric", "nope", "--inserts", "0", "--seed", "0",
                                                       "--pause-every", "0"])
    check("mut: control, zeros parse; the metric is what is refused",
          code == 2 and "--metric must be" in err and "whole number" not in err and not left, err[-160:])
    code, err, secs, left = run(numeric["hist"] + ["--warmup", "0", "--seed", "0", "--extra-pct", "0"],
                                shape_fixture(10_000, 5_000, a.shards))
    check("hist: control, --warmup 0 --seed 0 --extra-pct 0 parse and get past the parser",
          code != 2 and "whole number" not in err and secs < QUICK_SECONDS, f"exit {code} {err[-160:]}")

    print(f"\n  {ok} ok, {failed} fallas")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
