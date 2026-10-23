#!/usr/bin/env python3
"""Drives a Catch2 selection case by case, and records what actually happened.

Why this exists rather than one `ctest` or one filtered invocation:

* A single invocation hides which case is in flight, so a case that runs for an
  hour looks like a hang with no name attached. One accidental `[.slow]` case
  wrote 34 GB before anyone could say which case it was.
* `[tag]` selects hidden `[.slow]` cases too — an explicit tag filter overrides
  the `[.]` hiding — so every alternative of an OR needs `~[.slow]` and the
  listing has to be read before the run, not after.
* An empty or failed listing must not read as a pass. It is the one outcome that
  looks like success and proves nothing.
* The `compact` reporter does not report skips: a skipped case prints
  "assertions: - none -" and nothing else. Counting skips from it yields zero
  whatever happened. The console reporter says "1 skipped", so that is the one
  used here.
* A pipeline around the binary reports the last stage's status. The exit code
  this script returns is the worst exit code of the cases it ran.

Usage: run_targeted_suite.py <label> <build-dir> <selection> [--list-only]
"""
import json, os, re, subprocess, sys, threading, time
from xml.sax.saxutils import unescape as xml_unescape

HEARTBEAT_SECONDS = 15
OUTDIR = os.environ.get("UTXOZ_SUITE_OUTDIR", "/tmp/claude-1000/run")


def listing(binary, selection):
    """The cases a selection selects, with [.slow] excluded from every
    alternative of an OR rather than from the expression as a whole."""
    sel = ",".join(f"{alt}~[.slow]" for alt in selection.split(",")) if selection else ""
    cmd = [binary, "--list-tests", "--reporter", "xml"] + ([sel] if sel else [])
    p = subprocess.run(cmd, capture_output=True, text=True)
    # XML, because the console listing wraps a long name at eighty columns and
    # the continuation then reads as another case.
    names = [xml_unescape(m) for m in re.findall(r"<Name>(.*?)</Name>", p.stdout, re.S)]
    return sel, names, p.returncode


def tallies(text):
    """What Catch2 said, read from its own summary rather than inferred."""
    out = {"passed": 0, "failed": 0, "skipped": 0}
    m = re.search(r"test cases:\s+(\d+)\s*\|(.*)", text)
    if m:
        for count, word in re.findall(r"(\d+)\s+(passed|failed|skipped)", m.group(2)):
            out[word] = int(count)
    elif "All tests passed" in text:
        n = re.search(r"in (\d+) test case", text)
        out["passed"] = int(n.group(1)) if n else 1
    return out


def main():
    if len(sys.argv) < 3:
        print(__doc__); sys.exit(2)
    label, build = sys.argv[1], sys.argv[2]
    selection = sys.argv[3] if len(sys.argv) > 3 and not sys.argv[3].startswith("--") else ""
    only_list = "--list-only" in sys.argv
    binary = os.path.join(build, "tests", "utxoz_tests")
    os.makedirs(OUTDIR, exist_ok=True)
    if not os.access(binary, os.X_OK):
        print(f"{label}: NO BINARY at {binary}"); sys.exit(127)

    sel, names, rc = listing(binary, selection)
    with open(f"{OUTDIR}/{label}.list", "w") as f:
        f.write(f"# binary: {binary}\n# selection: {sel}\n# list exit: {rc}\n")
        f.write("\n".join(names) + "\n")
    print(f"{label}: selection={sel!r} listed={len(names)} list_exit={rc}")
    if rc != 0:
        print(f"{label}: LISTING FAILED — not a pass"); sys.exit(2)
    if not names:
        print(f"{label}: EMPTY SELECTION — not a pass"); sys.exit(2)
    if only_list:
        for n in names:
            print(f"    - {n}")
        sys.exit(0)

    results, worst = [], 0
    total = {"passed": 0, "failed": 0, "skipped": 0}
    for i, name in enumerate(names, 1):
        log = f"{OUTDIR}/{label}.{i:03d}.out"
        start = time.time()
        print(f"  [{i}/{len(names)}] START {time.strftime('%H:%M:%S')} {name}", flush=True)
        with open(log, "w") as lf:
            # Console reporter: the compact one does not report skips.
            proc = subprocess.Popen([binary, name.replace(",", "\\,")],
                                    stdout=lf, stderr=subprocess.STDOUT)
            stop = threading.Event()

            def beat(pid=proc.pid, t0=start, nm=name):
                while not stop.wait(HEARTBEAT_SECONDS):
                    print(f"      alive pid={pid} {time.time()-t0:.0f}s on {nm[:50]!r} "
                          f"(liveness only, not progress)", flush=True)

            t = threading.Thread(target=beat, daemon=True); t.start()
            code = proc.wait(); stop.set()
        # Binary bytes can reach a test's stdout; strict decoding would crash the
        # driver and lose the exit code it exists to record.
        counts = tallies(open(log, errors="replace").read())
        for k in total:
            total[k] += counts[k]
        results.append({"name": name, "exit": code, "log": log, **counts})
        worst = worst or code
        print(f"  [{i}/{len(names)}] END   {time.strftime('%H:%M:%S')} "
              f"{time.time()-start:.1f}s exit={code} "
              f"passed={counts['passed']} failed={counts['failed']} "
              f"skipped={counts['skipped']}", flush=True)

    failed = [r for r in results if r["exit"] != 0]
    skipped = [r for r in results if r["skipped"]]
    with open(f"{OUTDIR}/{label}.json", "w") as f:
        json.dump({"label": label, "binary": binary, "selection": sel,
                   "cases": results, "totals": total, "worst_exit": worst}, f, indent=1)
    with open(f"{OUTDIR}/{label}.code", "w") as f:
        f.write(str(worst) + "\n")
    print(f"{label}: cases={len(results)} ok={len(results)-len(failed)} "
          f"failed={len(failed)} skipped_cases={len(skipped)} "
          f"catch2_passed={total['passed']} catch2_failed={total['failed']} "
          f"catch2_skipped={total['skipped']} worst_exit={worst}")
    for r in skipped:
        print(f"    SKIPPED {r['name']}")
    for r in failed:
        print(f"    FAILED exit={r['exit']} {r['name']}  log={r['log']}")
    sys.exit(worst)


main()
