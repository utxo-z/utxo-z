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
  this script returns is the worst outcome of the cases it ran: any failure
  outranks a skip, and a skip outranks a pass. A case is a skip only when
  Catch2 exits 4 and says it skipped and failed nothing; the suite then exits
  4, never 0. Among failures, the first one's code is kept (1 if it was 4).

Usage: run_targeted_suite.py <label> <build-dir> <selection> [--list-only]
"""
import json, os, re, subprocess, sys, threading, time
from xml.sax.saxutils import unescape as xml_unescape

HEARTBEAT_SECONDS = 15
OUTDIR = os.environ.get("UTXOZ_SUITE_OUTDIR", "/tmp/claude-1000/run")
# Listing is quick; one that does not answer is a failure, not a hang. (The env
# overrides let the tests time a fake out in a second.)
LIST_TIMEOUT = float(os.environ.get("UTXOZ_SUITE_LIST_TIMEOUT", "300"))
SELECT_TIMEOUT = float(os.environ.get("UTXOZ_SUITE_SELECT_TIMEOUT", "120"))


def alternatives(selection):
    """The OR alternatives of a Catch2 test spec: split at the commas Catch2
    splits at, not at one escaped with a backslash or inside quotes."""
    alts, cur, quoted, i = [], "", False, 0
    while i < len(selection):
        ch = selection[i]
        if ch == "\\" and i + 1 < len(selection):
            cur += selection[i:i + 2]; i += 2; continue
        if ch == '"':
            quoted = not quoted
        if ch == "," and not quoted:
            alts.append(cur); cur = ""
        else:
            cur += ch
        i += 1
    return alts + [cur]


def listing(binary, selection):
    """The cases a selection selects, with [.slow] excluded from every
    alternative of an OR rather than from the expression as a whole. The
    exclusion goes first: after an unquoted name, Catch2 would read `~` as part
    of the name and `[.slow]` as a required tag."""
    sel = ",".join(f"~[.slow]{alt}" for alt in alternatives(selection)) if selection else ""
    cmd = [binary, "--list-tests", "--reporter", "xml"] + ([sel] if sel else [])
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=LIST_TIMEOUT)
    except subprocess.TimeoutExpired:
        return sel, [], f"timed out after {LIST_TIMEOUT:g}s"
    # XML, because the console listing wraps a long name at eighty columns and
    # the continuation then reads as another case.
    names = [xml_unescape(m) for m in re.findall(r"<Name>(.*?)</Name>", p.stdout, re.S)]
    return sel, names, p.returncode


def catch2_literal(name):
    """A test name as a Catch2 (v3) test spec for that name.

    Quoted, so `[`, `]` and a leading `~` are plain characters; inside the quotes
    `,` still separates and `"` still closes, so they and `\\` are escaped. Two
    things no spelling avoids: Catch2 drops the escapes before it looks for
    wildcards, so a leading or trailing `*` still is one, and it compares names
    case-insensitively with the ends trimmed. That is why selects() confirms
    the spec with Catch2 itself before the case runs.
    """
    return '"' + "".join("\\" + ch if ch in '\\,"' else ch for ch in name) + '"'


def selects(binary, spec):
    """The names Catch2 itself says spec selects; None if it did not say in time."""
    try:
        p = subprocess.run([binary, "--list-tests", "--reporter", "xml", spec],
                           capture_output=True, text=True, timeout=SELECT_TIMEOUT)
    except subprocess.TimeoutExpired:
        return None
    return [xml_unescape(m) for m in re.findall(r"<Name>(.*?)</Name>", p.stdout, re.S)]


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


def outcome(code, counts):
    """passed, skipped or failed. A skip is Catch2's exit 4 with skips and no
    failure in its own tally; any other nonzero exit is a failure."""
    if code == 0:
        return "passed"
    if code == 4 and counts["skipped"] > 0 and counts["failed"] == 0:
        return "skipped"
    return "failed"


RANK = {"passed": 0, "skipped": 1, "failed": 2}


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

    results, worst, worst_rank = [], 0, 0
    total = {"passed": 0, "failed": 0, "skipped": 0}
    for i, name in enumerate(names, 1):
        log = f"{OUTDIR}/{label}.{i:03d}.out"
        start = time.time()
        print(f"  [{i}/{len(names)}] START {time.strftime('%H:%M:%S')} {name}", flush=True)
        spec = catch2_literal(name)
        chosen = selects(binary, spec)
        if chosen != [name]:
            # Not run: the spec would run other cases, or none, under this name.
            print(f"  [{i}/{len(names)}] SPEC {spec!r} selects {chosen!r}, not only this case: "
                  f"counted as a failure", flush=True)
            results.append({"name": name, "exit": 3, "outcome": "failed", "log": None,
                            "selects": chosen, "passed": 0, "failed": 0, "skipped": 0})
            if RANK["failed"] > worst_rank: worst, worst_rank = 3, RANK["failed"]
            continue
        with open(log, "w") as lf:
            # Console reporter: the compact one does not report skips.
            # -w UnmatchedTestSpec: a spec that matches nothing exits 3 rather than
            # Catch2's default 2. (v3 has no "NoTests" warning; it rejects the
            # option, which would fail every case.)
            proc = subprocess.Popen([binary, spec, "-w", "UnmatchedTestSpec"],
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
        # Fail closed: a case that ran no test at all is not a pass, whatever
        # the exit code says - this Catch2 exits nonzero for an unmatched spec,
        # but nothing here should rest on that. A skipped case did run.
        if code == 0 and counts["passed"] + counts["failed"] + counts["skipped"] == 0:
            code = 3
            print(f"  [{i}/{len(names)}] NO TEST RAN for {name!r}: counted as a failure", flush=True)
        for k in total:
            total[k] += counts[k]
        kind = outcome(code, counts)
        results.append({"name": name, "exit": code, "outcome": kind, "log": log, **counts})
        if RANK[kind] > worst_rank:
            # A failure must not exit with the code a skip uses.
            worst, worst_rank = (1 if kind == "failed" and code == 4 else code), RANK[kind]
        print(f"  [{i}/{len(names)}] END   {time.strftime('%H:%M:%S')} "
              f"{time.time()-start:.1f}s exit={code} "
              f"passed={counts['passed']} failed={counts['failed']} "
              f"skipped={counts['skipped']}", flush=True)

    failed = [r for r in results if r["outcome"] == "failed"]
    skipped = [r for r in results if r["outcome"] == "skipped"]
    with open(f"{OUTDIR}/{label}.json", "w") as f:
        json.dump({"label": label, "binary": binary, "selection": sel,
                   "cases": results, "totals": total, "worst_exit": worst}, f, indent=1)
    with open(f"{OUTDIR}/{label}.code", "w") as f:
        f.write(str(worst) + "\n")
    print(f"{label}: cases={len(results)} ok={len(results)-len(failed)-len(skipped)} "
          f"failed={len(failed)} skipped_cases={len(skipped)} "
          f"catch2_passed={total['passed']} catch2_failed={total['failed']} "
          f"catch2_skipped={total['skipped']} worst_exit={worst}")
    for r in skipped:
        print(f"    SKIPPED {r['name']}")
    for r in failed:
        print(f"    FAILED exit={r['exit']} {r['name']}  log={r['log']}")
    sys.exit(worst)


main()
