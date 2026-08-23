#!/usr/bin/env python3
"""One measurement session: build, settle, warm up, measure A/A, measure A/B.

Why a session and not two commands
----------------------------------

The A/A control exists to say how far this benchmark's answer wanders when
nothing has changed, and that number is a property of a machine *at a moment* —
its load, its clocks, its page cache, whatever else is running on it. A floor
measured this morning does not describe this afternoon, and a floor measured
before a build does not describe the machine after one.

So the two runs belong to the same window, taken back to back, with nothing
compiled between them. This script owns that ordering:

    builds (all four, finished)
        ↓
    cooldown until the machine can be shown to be quiet
        ↓
    warmup, paired and discarded
        ↓
    A/A interleaved   ── the floor
        ↓                (no builds, no other harness work in between)
    A/B interleaved   ── the comparison the floor is for

The four binaries are built by whoever calls this, before it starts. That is
deliberate: a script that builds and measures in one breath is how the first
zen5-desktop control was spoiled, taken at a load average of 9.53 while twelve
compilers were running.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import bench_quiesce as bq

SCHEMA = "utxoz.bench.session/1"


def _stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def measure(label: str, binary_a: Path, binary_b: Path, out_dir: Path,
            args: argparse.Namespace, mode: str, commit_a: str | None,
            commit_b: str | None) -> Path:
    """One interleaved run, via the runner, with its raw output kept.

    Shelled out rather than called in-process so that the artifact is produced
    by exactly the command a person would type to reproduce it, and so that a
    crash in one run cannot take the session's own bookkeeping with it.
    """
    out_dir.mkdir(parents=True, exist_ok=True)
    samples = out_dir / "samples.json"
    cmd = [
        sys.executable, str(Path(__file__).parent / "bench_ab.py"),
        "--binary-a", str(binary_a), "--binary-b", str(binary_b),
        "--rounds", str(args.rounds), "--seed", str(args.seed),
        "--warmup", str(args.warmup), "--mode", mode,
        "--scratch", str(out_dir / "scratch"), "--keep-scratch",
        "--output", str(samples), "--host", args.host,
        "--build-type", args.build_type, "--options", args.options,
        "--toolchain-a", args.toolchain, "--toolchain-b", args.toolchain,
        "--dependencies-a", args.dependencies, "--dependencies-b", args.dependencies,
    ]
    if args.pin_cpu is not None:
        cmd += ["--pin-cpu", str(args.pin_cpu)]
    # Passed straight through: the runner is what applies them to each side's
    # environment and records them as the run's provenance.
    if args.runtime_config_a:
        cmd += ["--runtime-config-a", args.runtime_config_a]
    if args.runtime_config_b:
        cmd += ["--runtime-config-b", args.runtime_config_b]
    if commit_a:
        cmd += ["--commit-a", commit_a]
    if commit_b:
        cmd += ["--commit-b", commit_b]
    print(f"\n=== {label} ===", flush=True)
    proc = subprocess.run(cmd, check=False)
    if proc.returncode != 0:
        raise SystemExit(f"{label} failed with exit {proc.returncode}")
    return samples


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True, help="the declared host name")
    parser.add_argument("--aa-a", required=True, help="A/A side one (built already)")
    parser.add_argument("--aa-b", required=True, help="A/A side two, same tree, separate build")
    parser.add_argument("--base", required=True, help="A/B base binary")
    parser.add_argument("--head", required=True, help="A/B head binary")
    parser.add_argument("--commit-base", default=None)
    parser.add_argument("--commit-head", default=None)
    parser.add_argument("--rounds", type=int, default=15)
    parser.add_argument("--warmup", type=int, default=2)
    parser.add_argument("--seed", type=int, required=True)
    parser.add_argument("--toolchain", required=True)
    parser.add_argument("--dependencies", required=True)
    parser.add_argument("--build-type", default="Release")
    parser.add_argument("--options", default="")
    parser.add_argument("--pin-cpu", type=int, default=None,
                        help="pin every execution to this logical CPU")
    parser.add_argument("--runtime-config-a", default=None,
                        help="KEY=VALUE,... for side A; one image, two settings")
    parser.add_argument("--runtime-config-b", default=None)
    parser.add_argument("--label", default="session",
                        help="what this run is: 'session' for official, "
                             "'calibration-affinity' for a diagnostic")
    parser.add_argument("--out", default="bench-runs")
    parser.add_argument("--build-finished-at", type=float, default=None,
                        help="unix time the last build ended, for the cooldown check")
    # Quiescence limits. Defaults are zen5-desktop's starting point and are recorded
    # in the artifact; nothing here claims they suit mac or fedora.
    parser.add_argument("--max-utilisation", type=float,
                        default=bq.ZEN5_DESKTOP_LIMITS["max_utilisation"])
    parser.add_argument("--max-normalised-load", type=float,
                        default=bq.ZEN5_DESKTOP_LIMITS["max_normalised_load"])
    parser.add_argument("--max-run-queue", type=int,
                        default=bq.ZEN5_DESKTOP_LIMITS["max_run_queue"])
    parser.add_argument("--min-seconds-since-build", type=float,
                        default=bq.ZEN5_DESKTOP_LIMITS["min_seconds_since_build"])
    parser.add_argument("--consecutive", type=int, default=bq.CONSECUTIVE_SAMPLES)
    parser.add_argument("--quiesce-timeout", type=float, default=3600.0)
    parser.add_argument("--poll", type=float, default=20.0)
    args = parser.parse_args(argv)

    session_dir = Path(args.out) / args.host / f"{_stamp()}-{args.label}"
    session_dir.mkdir(parents=True, exist_ok=True)
    started = time.time()

    print(f"session: {session_dir}")
    print("waiting for a quiet window (this does not kill anything and does not "
          "wait forever)", flush=True)

    since_build = (started - args.build_finished_at) if args.build_finished_at else None
    quiescence = bq.wait_for_quiescence(
        timeout_seconds=args.quiesce_timeout,
        poll_seconds=args.poll,
        consecutive=args.consecutive,
        max_normalised_load=args.max_normalised_load,
        max_utilisation=args.max_utilisation,
        max_run_queue=args.max_run_queue,
        build_finished_at=args.build_finished_at,
        seconds_since_build=since_build,
        min_seconds_since_build=args.min_seconds_since_build,
    )

    # A diagnostic is never "official", however clean it comes out: it exists to
    # test the instrument, not to judge a change.
    clean = "official" if args.label == "session" else args.label
    status = clean if quiescence["quiet"] else "contaminated"
    if not quiescence["quiet"]:
        # Measured anyway, and labelled. The samples are still worth keeping —
        # they are what this machine does under this load — but nothing here may
        # be read as a floor or a verdict.
        print(f"\nNOT QUIET: {quiescence['reason']}", file=sys.stderr)
        print("continuing, and marking the session contaminated", file=sys.stderr)

    # The control takes side A's configuration on both sides: a control whose
    # sides differ in anything is not a control, and the analyser refuses one.
    aa_args = argparse.Namespace(**vars(args))
    aa_args.runtime_config_b = args.runtime_config_a
    aa = measure("A/A control", Path(args.aa_a), Path(args.aa_b),
                 session_dir / "aa", aa_args, "aa", args.commit_base, args.commit_base)
    # Nothing between the two but the runs themselves.
    ab = measure("A/B comparison", Path(args.base), Path(args.head),
                 session_dir / "ab", args, "ab", args.commit_base, args.commit_head)

    after = bq.assess(args.max_normalised_load, args.max_utilisation,
                      args.max_run_queue, since_build, args.min_seconds_since_build)
    if status == "official" and not after["quiet"]:
        status = "contaminated"
        print(f"\nthe machine stopped being quiet during the session: {after['reason']}",
              file=sys.stderr)

    verdict = session_dir / "verdict.json"
    report = session_dir / "report.md"
    rc = subprocess.run([
        sys.executable, str(Path(__file__).parent / "bench_analyze.py"), str(ab),
        "--control", str(aa), "--output", str(verdict), "--report", str(report),
    ], check=False)

    manifest = {
        "schema": SCHEMA,
        "host": args.host,
        "label": args.label,
        "status": status,
        "pinned_cpu": args.pin_cpu,
        "started_utc": datetime.fromtimestamp(started, timezone.utc).isoformat(timespec="seconds"),
        "finished_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "duration_seconds": round(time.time() - started, 1),
        "phases": ["builds (before this script)", "cooldown", "warmup (discarded)",
                   "aa interleaved", "ab interleaved"],
        "builds_between_runs": False,
        "quiescence": {"before": quiescence, "after": after},
        "artifacts": {
            "aa_samples": str(aa), "ab_samples": str(ab),
            "verdict": str(verdict) if verdict.exists() else None,
            "report": str(report) if report.exists() else None,
            "raw": [str(p) for p in sorted(session_dir.rglob("benchmark_results.json"))],
        },
        "analyzer_exit": rc.returncode,
    }
    (session_dir / "manifest.json").write_text(json.dumps(manifest, indent=2, default=str))

    print(f"\nsession {status}: {session_dir}")
    print(f"raw json preserved: {len(manifest['artifacts']['raw'])} files")

    # The analyser's own exit code carries meaning the session must not swallow:
    # 3 is a harness integrity failure — a run that cannot be read at all — and 1
    # is a gated regression. Deriving success from the quiescence status alone
    # reported a clean session over an artifact the analyser had refused.
    if rc.returncode != 0:
        print(f"the analyser exited {rc.returncode}; the session is not usable as a "
              f"result", file=sys.stderr)
        return rc.returncode
    return 0 if status == clean else 4


if __name__ == "__main__":
    raise SystemExit(main())
