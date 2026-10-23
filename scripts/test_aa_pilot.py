#!/usr/bin/env python3
"""Negative coverage for the A/A runner, with fake harnesses that really run.

A parser test proves the parser. These drive the runner end to end against
executables that behave the way a broken harness behaves: emitting Infinity,
emitting a list, measuring something other than what was asked, exiting
nonzero, printing nothing, hanging. Each must end the run with a nonzero exit
and a named reason, never a shortened `n`.
"""
import json, os, stat, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
RUNNER = HERE / "run_aa_pilot.py"
STATS = HERE / "bench_analyze.py"   # the repository's own estimator
# A CPU this process may run on: the runner's default (2) may not be one.
CPU = min(os.sched_getaffinity(0))

GOOD = {
    "schema": "utxoz-calib-1", "metric": "find_hit", "shard_count": 1,
    "ops": 10, "warmup": 2, "population_requested": 5, "population_observed": 5,
    "version_files_all_classes": 5, "version_files_class_48": 1, "value_size": 33,
    "seed": 1, "cache": "warm", "elapsed_ns": 1000, "ns_per_op": 100.0,
    "hits": 10, "not_resolved": 0, "not_found": 0, "real_errors": 0,
    "first_error_code": 0, "warmup_hits": 2, "warmup_not_resolved": 0,
    "warmup_not_found": 0, "verified_payload": 10, "verified_height": 10,
    "valid": True, "invalid_because": "",
}


# A fake that answers honestly has to echo the seed it was given: a fake that
# always says 1 is what let the missing seed check go unnoticed.
PREAMBLE = """#!/usr/bin/env python3
import sys, json, time
_seed = 1
for _i, _a in enumerate(sys.argv):
    if _a == '--seed':
        _seed = int(sys.argv[_i + 1])
"""


def fake(dirpath, name, body):
    p = Path(dirpath) / name
    p.write_text(PREAMBLE + body)
    p.chmod(p.stat().st_mode | stat.S_IEXEC)
    return p


def run(harness, outdir, extra=(), cwd=None):
    cmd = [sys.executable, "-I", str(RUNNER), "--harness", str(harness),
           "--metric", "find_hit", "--stats-module", str(STATS),
           "--pairs", "2", "--ops", "10", "--warmup", "2", "--population", "5",
           "--timeout", "5", "--cpu", str(CPU), "--out-dir", str(outdir), *extra]
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=120, cwd=cwd)
    except subprocess.TimeoutExpired as e:
        return -1, f"the runner did not finish within 120s: {e}"
    return p.returncode, p.stdout + p.stderr


def main():
    bad = 0
    with tempfile.TemporaryDirectory(prefix="aa-runner-tests-") as tmp:
        variants = [
            ("good (echoes --seed)",
             f"d={GOOD!r}; d['seed']=_seed; print(json.dumps(d))", 0),
            ("infinity", "print('{\"schema\":\"utxoz-calib-1\",\"ns_per_op\":Infinity}')", 3),
            ("a list", "print('[1,2,3]')", 3),
            ("empty output", "pass", 3),
            ("nonzero exit", f"d={GOOD!r}; d['seed']=_seed; print(json.dumps(d)); sys.exit(3)", 3),
            ("wrong metric",
             f"d={GOOD!r}; d['seed']=_seed; d['metric']='find_miss'; print(json.dumps(d))", 3),
            ("wrong ops", f"d={GOOD!r}; d['seed']=_seed; d['ops']=99; print(json.dumps(d))", 3),
            ("wrong population",
             f"d={GOOD!r}; d['seed']=_seed; d['population_observed']=999; print(json.dumps(d))", 3),
            ("valid is a number",
             f"d={GOOD!r}; d['seed']=_seed; d['valid']=1; print(json.dumps(d))", 3),
            ("harness says invalid",
             f"d={GOOD!r}; d['seed']=_seed; d['valid']=False; d['invalid_because']='x'; print(json.dumps(d))", 3),
            ("real errors counted",
             f"d={GOOD!r}; d['seed']=_seed; d['real_errors']=4; print(json.dumps(d))", 3),
            ("unverified hits",
             f"d={GOOD!r}; d['seed']=_seed; d['verified_payload']=3; print(json.dumps(d))", 3),
            ("hangs", "time.sleep(60)", 3),
            # Round 2 of review found each of these passing.
            ("always reports seed 1",
             f"d={GOOD!r}; d['seed']=1; print(json.dumps(d))", 3),
            ("a negative counter",
             f"d={GOOD!r}; d['seed']=_seed; d['not_resolved']=-99; "
             f"d['hits']=10; print(json.dumps(d))", 3),
            ("a float counter",
             f"d={GOOD!r}; d['seed']=_seed; d['hits']=10.5; print(json.dumps(d))", 3),
            ("outcomes do not add up",
             f"d={GOOD!r}; d['seed']=_seed; d['hits']=10; d['not_resolved']=3; "
             f"print(json.dumps(d))", 3),
            ("warmup does not add up",
             f"d={GOOD!r}; d['seed']=_seed; d['warmup_hits']=99; print(json.dumps(d))", 3),
            ("ns_per_op disagrees with elapsed/ops",
             f"d={GOOD!r}; d['seed']=_seed; d['ns_per_op']=42.0; print(json.dumps(d))", 3),
            ("verified more than it looked up",
             f"d={GOOD!r}; d['seed']=_seed; d['verified_payload']=99; print(json.dumps(d))", 3),
        ]
        for i, (name, body, want) in enumerate(variants):
            h = fake(tmp, f"fake{i}.py", body)
            code, log = run(h, Path(tmp) / f"out{i}")
            ok = code == want
            bad += 0 if ok else 1
            print(f"  {name:<24} exit={code} want={want} {'ok' if ok else 'FAILED'}")
            if not ok:
                print("    " + log.strip().replace("\n", "\n    ")[:600])

        # Range and safety checks.
        h = fake(tmp, "fakeg.py", f"d={GOOD!r}; d['seed']=_seed; print(json.dumps(d))")
        for name, extra, want in [
            ("pairs above the cap", ("--pairs", "21"), 2),
            ("confidence out of range", ("--confidence", "1.5"), 2),
            ("timeout is NaN", ("--timeout", "nan"), 2),
            ("resamples too low", ("--resamples", "10"), 2),
        ]:
            code, _ = run(h, Path(tmp) / name.replace(" ", "_"), extra)
            ok = code == want
            bad += 0 if ok else 1
            print(f"  {name:<24} exit={code} want={want} {'ok' if ok else 'FAILED'}")

        existing = Path(tmp) / "already"
        existing.mkdir()
        code, _ = run(h, existing)
        ok = code == 2
        bad += 0 if ok else 1
        print(f"  {'refuses an existing dir':<24} exit={code} want=2 {'ok' if ok else 'FAILED'}")

        code, _ = run(h, Path(tmp) / "nostats", ("--stats-module", "/nonexistent.py"))
        ok = code != 0 and not (Path(tmp) / "nostats").exists()
        bad += 0 if ok else 1
        print(f"  {'rejects a missing module':<24} exit={code} want!=0, no out dir {'ok' if ok else 'FAILED'}")
        # Refused inputs leave no output directory behind for the retry to trip over.
        noexec = Path(tmp) / "noexec.py"; noexec.write_text("print('{}')\n"); noexec.chmod(0o644)
        code, _ = run(noexec, Path(tmp) / "noexec_out")
        ok = code == 2 and not (Path(tmp) / "noexec_out").exists()
        bad += 0 if ok else 1
        print(f"  {'non-executable harness':<24} exit={code} want=2, no out dir {'ok' if ok else 'FAILED'}")
        # From a working directory that is not the repository: the source hash is
        # of the repository's harness source, found from the runner itself.
        import hashlib
        elsewhere = Path(tmp) / "elsewhere"; elsewhere.mkdir()
        code, log = run(h, Path(tmp) / "from_elsewhere", cwd=elsewhere)
        want_hash = hashlib.sha256((HERE.parent / "benchmarks" / "calib_stationary.cpp").read_bytes()).hexdigest()
        cfg = Path(tmp) / "from_elsewhere" / "config.json"
        got = json.loads(cfg.read_text()).get("harness_source_sha256") if cfg.is_file() else None
        rep_f = Path(tmp) / "from_elsewhere" / "report.json"
        cmds = [s["command"] for s in json.loads(rep_f.read_text())["samples"]] if rep_f.is_file() else []
        ok = code == 0 and got == want_hash and cmds and all(c.startswith(f"taskset -c {CPU} ") for c in cmds)
        bad += 0 if ok else 1
        print(f"  {'run from another cwd':<24} exit={code} source hash {'matches' if got == want_hash else got} "
              f"cpu {CPU} in every command {bool(cmds) and all(c.startswith(f'taskset -c {CPU} ') for c in cmds)} "
              f"{'ok' if ok else 'FAILED'}")
        if not ok:
            print("    " + log.strip().replace("\n", "\n    ")[:600])

        # The first unusable sample ends the run: the harness is not called again.
        # Each fake counts its calls; --pairs 2 would otherwise make four.
        for name, body, want_calls, want_why in [
            ("every sample bad: one call", f"d={GOOD!r}; d['seed']=99; print(json.dumps(d))", 1, "seed"),
            ("second sample bad: two calls",
             f"d={GOOD!r}; d['seed']=_seed; d['ops']=10 if _n == 1 else 99; print(json.dumps(d))", 2, "not 99"),
            ("valid document, exit 3: one call",
             f"d={GOOD!r}; d['seed']=_seed; print(json.dumps(d)); sys.exit(3)", 1, "exit 3"),
        ]:
            calls = Path(tmp) / f"calls-{name[:5]}"
            counting = (f"open({str(calls)!r}, 'a').write('x')\n"
                        f"_n = len(open({str(calls)!r}).read())\n" + body)
            h2 = fake(tmp, f"count-{name[:5]}.py", counting)
            outd = Path(tmp) / f"count-out-{name[:5]}"
            code, log = run(h2, outd)
            n = len(calls.read_text()) if calls.is_file() else 0
            rep = json.loads((outd / "report.json").read_text()) if (outd / "report.json").is_file() else {}
            fails = rep.get("failures", [])
            ok = (code == 3 and n == want_calls and len(rep.get("samples", [])) == want_calls
                  and len(fails) == 1 and want_why in (fails[0].get("unusable_because") or "")
                  and fails[0].get("ns_per_op") is None and "NO MEASUREMENT" in rep.get("verdict", ""))
            bad += 0 if ok else 1
            print(f"  {name:<24} exit={code} calls={n} want={want_calls} {'ok' if ok else 'FAILED'}")
            if not ok:
                print("    " + log.strip().replace("\n", "\n    ")[:600])

    print(f"  {'FAILURES: %d' % bad if bad else 'all runner tests passed'}")
    return 1 if bad else 0


sys.exit(main())
