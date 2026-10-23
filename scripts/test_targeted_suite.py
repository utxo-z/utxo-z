#!/usr/bin/env python3
"""Tests for run_targeted_suite.py, with a fake Catch2 binary. Untimed.

  - every name, whatever Catch2 characters it holds, is passed as the spec
    written out below (not recomputed from the driver's formula), with
    -w UnmatchedTestSpec;
  - a spec Catch2 says selects more than its case, or none, is a failure and
    the case is not run;
  - a case that ran no test at all is a failure even when the binary exits 0;
  - a case that only skipped stays a skip (exit 4), not a failure.

The fake does not parse specs; whether these specs select what they should in a
real Catch2 was checked against Catch2 v3.15 itself (the PR147 review evidence).

Run:
  python3 -I scripts/test_targeted_suite.py
"""
import json, os, shutil, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
DRIVER = HERE / "run_targeted_suite.py"
# name -> the spec the driver must pass, written out by hand.
SPECS = {"a plain name": '"a plain name"',
         "a name, with a comma": '"a name\\, with a comma"',
         "brackets [like a tag]": '"brackets [like a tag]"',
         "a * star": '"a * star"',
         "~ a leading tilde": '"~ a leading tilde"',
         'a "quoted" word': '"a \\"quoted\\" word"',
         "a back\\slash": '"a back\\\\slash"'}
NAMES = list(SPECS)
ok = failed = 0

# The fake lists, for a spec it was told about, the names it was told; for any
# other spec, nothing - and running a spec it does not know runs nothing.
FAKE = r'''#!/usr/bin/env python3
import json, os, sys, time
from xml.sax.saxutils import escape
cfg = json.load(open(os.environ["FAKE_CFG"]))
args = sys.argv[1:]
def xml(names):
    print("<?xml version=\"1.0\"?><MatchingTests>"
          + "".join(f"<TestCase><Name>{escape(n)}</Name></TestCase>" for n in names)
          + "</MatchingTests>")
if "--list-tests" in args:
    spec = args[3] if len(args) > 3 else None
    with open(os.environ["FAKE_LOG"] + ".list", "a") as f: f.write(json.dumps(args) + "\n")
    if (spec is None and cfg.get("hang_list")) or spec in cfg.get("hang_selects", []):
        time.sleep(5)                                    # longer than the test's 1 s limit
    xml(cfg["names"] if spec is None else cfg["selects"].get(spec, []))
    sys.exit(0)
spec = args[0]
with open(os.environ["FAKE_LOG"], "a") as f: f.write(json.dumps(args) + "\n")
mode = cfg.get("modes", {}).get(spec, cfg["mode"])
if mode == "fail":
    print("test cases: 1 | 1 failed"); sys.exit(42)
if mode == "bad4":
    print("something that is not a Catch2 tally"); sys.exit(4)  # 4 without a skip
if mode == "zero":
    print("=" * 79); print("No tests ran"); sys.exit(0)        # a false pass to catch
if spec not in cfg["selects"]:
    print("No tests ran"); sys.exit(3)
if mode == "skip":
    print("test cases: 1 | 1 skipped"); sys.exit(4)
print("All tests passed (1 assertion in 1 test case)"); sys.exit(0)
'''


def drive(mode, names=NAMES, selects=None, modes=None, args=(), hang_list=False, hang_selects=()):
    """(process, run calls, listing calls, .code text, JSON summary)."""
    root = Path(tempfile.mkdtemp(prefix="targeted-suite-test-"))
    try:
        (root / "build" / "tests").mkdir(parents=True)
        fake = root / "build" / "tests" / "utxoz_tests"
        fake.write_text(FAKE); fake.chmod(0o755)
        cfg = root / "cfg.json"
        cfg.write_text(json.dumps({"names": names, "mode": mode, "modes": modes or {},
                                   "hang_list": hang_list, "hang_selects": list(hang_selects),
                                   "selects": selects or {SPECS[n]: [n] for n in names}}))
        log = root / "argv.log"
        env = dict(os.environ, FAKE_CFG=str(cfg), FAKE_LOG=str(log),
                   UTXOZ_SUITE_OUTDIR=str(root / "out"),
                   UTXOZ_SUITE_LIST_TIMEOUT="1", UTXOZ_SUITE_SELECT_TIMEOUT="1")
        p = subprocess.run([sys.executable, "-I", str(DRIVER), "t", str(root / "build"), *args],
                           capture_output=True, text=True, env=env, cwd=root, timeout=300)
        read = lambda f: [json.loads(l) for l in f.read_text().splitlines()] if f.is_file() else []
        out = root / "out"
        code = (out / "t.code").read_text().strip() if (out / "t.code").is_file() else None
        summary = json.loads((out / "t.json").read_text()) if (out / "t.json").is_file() else None
        return p, read(log), read(Path(str(log) + ".list")), code, summary
    finally:
        shutil.rmtree(root, ignore_errors=True)


def check(name, good, why=""):
    global ok, failed
    if good: print(f"  ok     {name}"); ok += 1
    else: print(f"  FALLA  {name}: {why}"); failed += 1


def main():
    print("=== run_targeted_suite.py ===")
    p, calls, _, _, _ = drive("pass")
    check("names with Catch2 characters arrive as the written-out specs: exit 0",
          p.returncode == 0 and [c[0] for c in calls] == [SPECS[n] for n in NAMES], p.stdout[-400:])
    check("every case asks Catch2 to refuse a spec that matches nothing",
          calls and all(c[1:] == ["-w", "UnmatchedTestSpec"] for c in calls), str(calls[:1]))
    over = {SPECS[n]: [n] for n in NAMES}
    over[SPECS["a * star"]] = ["a * star", "a * starfish"]
    over[SPECS["a plain name"]] = []
    p, calls, _, _, _ = drive("pass", selects=over)
    check("a spec that selects more than its case, or none, fails and is not run",
          p.returncode == 3 and "failed=2" in p.stdout and "selects ['a * star', 'a * starfish']" in p.stdout
          and [c[0] for c in calls] == [SPECS[n] for n in NAMES if n not in ("a * star", "a plain name")],
          p.stdout[-500:])
    p, *_ = drive("zero")
    check("a case that ran no test is a failure even at exit 0",
          p.returncode != 0 and "NO TEST RAN" in p.stdout and f"failed={len(NAMES)}" in p.stdout,
          p.stdout[-400:])
    p, *_ = drive("skip", NAMES[:1])
    check("a case that only skipped stays a skip (exit 4)",
          p.returncode == 4 and "skipped_cases=1" in p.stdout and "NO TEST RAN" not in p.stdout,
          p.stdout[-300:])
    # Review 5637775062: one outcome per case, and the suite's exit is the worst
    # of them - a failure over a skip, a skip over a pass - in the exit, the
    # .code file and the JSON alike.
    four = NAMES[:4]
    def mixed(modes_by_name, want, failed_n, skipped_n):
        modes = {SPECS[n]: m for n, m in modes_by_name.items()}
        p, _, _, code, summary = drive("pass", four, modes=modes)
        outcomes = [c.get("outcome") for c in summary["cases"]] if summary else None
        good = (p.returncode == want and code == str(want) and summary and summary["worst_exit"] == want
                and f"failed={failed_n} " in p.stdout and f"skipped_cases={skipped_n} " in p.stdout
                and outcomes == [{"pass": "passed", "skip": "skipped"}.get(modes_by_name.get(n, "pass"), "failed")
                                 for n in four])
        return good, f"exit {p.returncode} code {code} outcomes {outcomes} {p.stdout[-300:]}"
    for name, modes, want, f_n, s_n in (
            ("todo pasa: exit 0", {}, 0, 0, 0),
            ("un skip y el resto pasa: exit 4, sin fallas", {four[1]: "skip"}, 4, 0, 1),
            ("skip, luego falla: la falla manda (42)", {four[0]: "skip", four[2]: "fail"}, 42, 1, 1),
            ("falla, luego skip: la falla manda (42)", {four[0]: "fail", four[3]: "skip"}, 42, 1, 1),
            ("exit 4 sin skips es una falla, y sale 1, no 4", {four[1]: "bad4", four[2]: "skip"}, 1, 1, 1),
            ("ninguno corrió en uno, skip en otro: la falla manda (3)",
             {four[0]: "skip", four[1]: "zero"}, 3, 1, 1)):
        check(f"agregado: {name}", *mixed(modes, want, f_n, s_n))

    # Review 5637941911: a listing that does not answer in time is a failure,
    # not a traceback - the whole listing ends the run as LISTING FAILED, one
    # case's listing fails that case and the others still run and are written.
    p, *_ = drive("pass", hang_list=True)
    check("un listado que no responde a tiempo: LISTING FAILED (exit 2), sin traceback",
          p.returncode == 2 and "LISTING FAILED" in p.stdout and "timed out" in p.stdout
          and "Traceback" not in p.stderr, p.stdout[-300:] + p.stderr[-300:])
    p, calls, _, code, summary = drive("pass", four, hang_selects=[SPECS[four[1]]])
    outcomes = [c.get("outcome") for c in summary["cases"]] if summary else None
    check("la selección de un caso que no responde: ese caso falla, los demás corren y se escriben",
          p.returncode == 3 and code == "3" and "Traceback" not in p.stderr
          and outcomes == ["passed", "failed", "passed", "passed"]
          and summary["cases"][1].get("selects") is None
          and [c[0] for c in calls] == [SPECS[n] for n in four if n != four[1]],
          f"exit {p.returncode} code {code} outcomes {outcomes} {p.stdout[-300:]} {p.stderr[-300:]}")

    # Selections: split where Catch2 splits, ~[.slow] on every alternative.
    sel = '[tag],"a name\\, with a comma",a name\\, with a comma,"x,y"'
    p, _, lists, _, _ = drive("pass", selects={}, args=(sel, "--list-only"))
    got = lists[-1][3] if lists and len(lists[-1]) > 3 else None
    want_sel = ('~[.slow][tag],~[.slow]"a name\\, with a comma",'
                '~[.slow]a name\\, with a comma,~[.slow]"x,y"')
    check("una selección se parte en las comas de Catch2, ~[.slow] en cada alternativa",
          got == want_sel, f"{got!r} != {want_sel!r}")
    print(f"\n  {ok} ok, {failed} fallas")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
