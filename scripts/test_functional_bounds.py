#!/usr/bin/env python3
"""The functional suites' bounds, with fake harnesses. Untimed.

test_calib_mut.py and test_observe_compaction.py drive real binaries on real
images. Here both run against a fake build whose utxoz_calib_mut never answers,
with a 1 s limit per call, and must:

  - count each call that timed out as a failed case and go on to the next one
    (every call is made), exit 1, and leave --work-parent empty;
  - give the single-CPU case a CPU this process may run on, not a fixed one.

No image is built: the fake utxoz_calib_hist writes only a shape.json.

Run:
  python3 -I scripts/test_functional_bounds.py
"""
import json, os, shutil, subprocess, sys, tempfile, time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ok = failed = 0

FAKE_HIST = r'''#!/usr/bin/env python3
import json, os, sys
d = sys.argv[sys.argv.index("--build-fixture") + 1]
os.makedirs(d)
json.dump({"shard_count": 1, "population": 300000}, open(os.path.join(d, "shape.json"), "w"))
'''
FAKE_MUT = r'''#!/usr/bin/env python3
import json, os, sys, time
with open(os.environ["FAKE_LOG"], "a") as f:
    f.write(json.dumps({"argv": sys.argv[1:], "cpus": sorted(os.sched_getaffinity(0))}) + "\n")
time.sleep(30)                                   # never answers within the 1 s limit
'''


def check(name, good, why=""):
    global ok, failed
    if good: print(f"  ok     {name}"); ok += 1
    else: print(f"  FALLA  {name}: {why}"); failed += 1


def drive(script, want_calls):
    root = Path(tempfile.mkdtemp(prefix="functional-bounds-"))
    try:
        b = root / "fake-build"; (b / "benchmarks").mkdir(parents=True)
        for name, body in (("utxoz_calib_hist", FAKE_HIST), ("utxoz_calib_mut", FAKE_MUT)):
            f = b / "benchmarks" / name; f.write_text(body); f.chmod(0o755)
        parent = root / "work-parent"; parent.mkdir()
        log = root / "calls.log"
        env = dict(os.environ, FAKE_LOG=str(log), UTXOZ_TEST_SUBPROCESS_TIMEOUT="1")
        t0 = time.monotonic()
        p = subprocess.run([sys.executable, "-I", str(HERE / script), "--build", str(b),
                            "--work-parent", str(parent)], capture_output=True, text=True, env=env, timeout=300)
        secs = time.monotonic() - t0
        calls = [json.loads(l) for l in log.read_text().splitlines()] if log.is_file() else []
        return p, calls, secs, sorted(os.listdir(parent))
    finally:
        shutil.rmtree(root, ignore_errors=True)


def main():
    print("=== bounds of the functional suites ===")
    cpu = min(os.sched_getaffinity(0))
    p, calls, secs, left = drive("test_calib_mut.py", 12)
    check("test_calib_mut: every call that times out is a failed case, and every case still runs",
          p.returncode == 1 and len(calls) == 12 and "Traceback" not in p.stderr
          and "las fuentes quedaron byte a byte" in p.stdout and secs < 120,
          f"exit {p.returncode}, {len(calls)} calls, {secs:.0f}s {p.stdout[-300:]} {p.stderr[-300:]}")
    check("test_calib_mut: --work-parent is left empty", left == [], str(left))
    tasked = [c for c in calls if "--workers" in c["argv"] and "--inserts" not in c["argv"]
              and c["argv"][c["argv"].index("--workers") + 1] == "2"]
    check("test_calib_mut: the single-CPU case runs on a CPU this process may use",
          len(tasked) == 1 and tasked[0]["cpus"] == [cpu], f"{tasked} want [{cpu}]")
    p, calls, secs, left = drive("test_observe_compaction.py", 4)
    check("test_observe_compaction: every call that times out is a failed case, and every case still runs",
          p.returncode == 1 and len(calls) == 4 and "Traceback" not in p.stderr
          and "la fuente quedó byte a byte" in p.stdout and secs < 120,
          f"exit {p.returncode}, {len(calls)} calls, {secs:.0f}s {p.stdout[-300:]} {p.stderr[-300:]}")
    check("test_observe_compaction: --work-parent is left empty", left == [], str(left))
    # test_harness_cli: a harness that does not answer is a failed case and the
    # next case still runs - with a 1 s limit and a fake that sleeps, not 60 s.
    probe = r'''
import importlib.util, json, sys, tempfile, os, time
spec = importlib.util.spec_from_file_location("thc", sys.argv[1])
t = importlib.util.module_from_spec(spec); spec.loader.exec_module(t)   # main() is guarded
hang = [sys.executable, "-c", "import time; time.sleep(30)"]
before = len([d for d in os.listdir(tempfile.gettempdir()) if d.startswith("harness-cli-")])
t0 = time.monotonic()
t.refused("hang refused", hang, "anything")                               # must fail
code, err, secs, left = t.run(hang)
t.check("hang control", code != 2 and "whole number" not in err, "")     # must fail too
t.refused("quick after", [sys.executable, "-c", "import sys; print('no', file=sys.stderr); sys.exit(2)"], "no")
after = len([d for d in os.listdir(tempfile.gettempdir()) if d.startswith("harness-cli-")])
print(json.dumps({"ok": t.ok, "failed": t.failed, "seconds": time.monotonic() - t0, "left": after - before,
                  "code": code}))
'''
    env = dict(os.environ, UTXOZ_TEST_CLI_TIMEOUT="1")
    p = subprocess.run([sys.executable, "-I", "-c", probe, str(HERE / "test_harness_cli.py")],
                       capture_output=True, text=True, env=env, timeout=120)
    try:
        res = json.loads(p.stdout.strip().splitlines()[-1])
    except (ValueError, IndexError):
        res = {}
    check("test_harness_cli: a timeout fails its case (also a 'code != 2' control) and the next case runs",
          p.returncode == 0 and res.get("failed") == 2 and res.get("ok") == 1 and res.get("code") is None
          and res.get("left") == 0 and res.get("seconds", 99) < 10,
          f"exit {p.returncode}, {res} {p.stdout[-300:]} {p.stderr[-300:]}")
    # test_hist_pilot: the mode-0 case is skipped, and counted, where mode 0 does
    # not stop a read (root); the cases after it still run. Root is simulated by
    # patching os.geteuid inside a subprocess - no privilege is used.
    probe = r'''
import importlib.util, io, json, os, sys, contextlib
spec = importlib.util.spec_from_file_location("thp", sys.argv[1])
t = importlib.util.module_from_spec(spec); spec.loader.exec_module(t)   # main() is guarded
real_geteuid = os.geteuid
buf = io.StringIO()
with contextlib.redirect_stdout(buf):
    os.geteuid = lambda: 0
    t.unreadable_store_case()                              # as "root": skipped
    os.geteuid = real_geteuid
    t.case("a later case still runs", want_exit=0)
    as_root = {"ok": t.ok, "failed": t.failed, "skipped": t.skipped}
    readable = t.mode0_is_readable()                       # the real answer on this host
    t.unreadable_store_case()                              # unpatched: runs, or skips if really root
print(json.dumps({"as_root": as_root, "readable_here": readable,
                  "final": {"ok": t.ok, "failed": t.failed, "skipped": t.skipped},
                  "skip_line": "SKIP   un archivo del store ilegible" in buf.getvalue()}))
'''
    p = subprocess.run([sys.executable, "-I", "-c", probe, str(HERE / "test_hist_pilot.py")],
                       capture_output=True, text=True, timeout=300)
    try:
        res = json.loads(p.stdout.strip().splitlines()[-1])
    except (ValueError, IndexError):
        res = {}
    want_final = ({"ok": 1, "failed": 0, "skipped": 2} if res.get("readable_here")
                  else {"ok": 2, "failed": 0, "skipped": 1})
    check("test_hist_pilot: as root the mode-0 case is a counted skip, later cases run; "
          "on this host it runs or skips as it should",
          p.returncode == 0 and res.get("as_root") == {"ok": 1, "failed": 0, "skipped": 1}
          and res.get("skip_line") is True and res.get("final") == want_final,
          f"exit {p.returncode}, {res} {p.stderr[-300:]}")
    print(f"\n  {ok} ok, {failed} fallas")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
