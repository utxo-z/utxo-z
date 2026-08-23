#!/usr/bin/env python3
"""Run two benchmark binaries against each other, interleaved, in one sitting.

Why this exists
---------------

The lane it replaces measured one sample of the head commit on today's runner
and compared it against the median of samples taken on *other* runners, days
earlier. PR #141 is the control that showed what that is worth: it changed only
test files, left the benchmark binary byte-identical to master, and the lane
reported changes from -53% to +102% at the same time.

Measured on one machine, the same binary run three times:

    metric          median spread   worst
    wall time            2.30%     116.33%
    cpu cycles           2.67%     122.37%
    instructions         0.016%      1.11%
    branches             0.029%      2.18%

So the instrument is not the problem. Wall time and cycles carry the machine's
mood; instruction and branch counts do not. Two things follow, and this file is
built on both: a comparison has to be paired and interleaved inside one run on
one machine, and what counts as a difference has to come from a measurement of
the noise rather than from a number somebody picked.

What a run produces
-------------------

One JSON artifact carrying every sample, the order they were taken in, the seed
that produced that order, and the identity of both binaries. Nothing is
summarised here — `analyze` does that, from the artifact, so the analysis can be
re-run and argued with without re-measuring.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import random
import shutil
import subprocess
import sys
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path

SCHEMA = "utxoz.bench.ab/1"

# The nanobench fields worth keeping, and the name each gets in the artifact.
# `median(...)` is nanobench's own summary of its eleven epochs; the per-epoch
# measurements stay in its output and are not needed here, because this harness
# takes its repetitions at the level of whole runs.
METRICS = {
    "elapsed": "median(elapsed)",
    "cycles": "median(cpucycles)",
    "instructions": "median(instructions)",
    "branches": "median(branchinstructions)",
    "branchmisses": "median(branchmisses)",
    "pagefaults": "median(pagefaults)",
    "contextswitches": "median(contextswitches)",
    # nanobench's own within-run dispersion. Kept because a benchmark that is
    # unstable inside a single run cannot be rescued by repeating the run, and
    # the report should be able to say so.
    "mdape": "medianAbsolutePercentError(elapsed)",
}


class HarnessError(RuntimeError):
    """A fact about the run that is wrong, as opposed to a slow benchmark.

    Raised for anything that would make the measurement mean something other
    than what it claims: a binary that is not there, a side that produced a
    different set of benchmarks, a run that exited non-zero. These fail the job.
    A performance verdict never does that on its own.
    """


def parse_runtime_config(spec: str | None) -> dict:
    """`KEY=VALUE,KEY=VALUE` into the environment one side runs under.

    Recorded in the artifact rather than only applied, because it is the whole
    provenance of a same-binary comparison: without it the two sides are
    indistinguishable and the run reads as an accidental A/A.
    """
    if not spec:
        return {}
    out = {}
    for item in spec.split(","):
        if "=" not in item:
            raise HarnessError(f"runtime config {item!r} is not KEY=VALUE")
        key, value = item.split("=", 1)
        out[key.strip()] = value.strip()
    return out


def sha256_of(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def draw_order(seed: int, rounds: int) -> list[str]:
    """Which side goes first in each round.

    Not always A-then-B. A machine that drifts — thermally, or because a
    neighbour started work — moves every measurement after the drift in the same
    direction, and a fixed order turns that drift into a difference between the
    sides. Alternating cancels the linear part of it; drawing the alternation
    from a seeded generator, and publishing the seed, keeps the run reproducible
    while stopping any period in the workload from lining up with the order.
    """
    rng = random.Random(seed)
    return ["AB" if rng.random() < 0.5 else "BA" for _ in range(rounds)]


def read_nanobench(path: Path) -> dict[str, dict[str, float]]:
    """One run's results, by benchmark name.

    A field that nanobench did not produce is recorded as None rather than as
    zero. The two are not the same thing: zero page faults is a measurement,
    and no instruction count — a runner that will not open a perf counter — is
    the absence of one. Downstream that becomes NO_EVIDENCE, never "no change".
    """
    with path.open() as f:
        data = json.load(f)
    out: dict[str, dict[str, float]] = {}
    for entry in data.get("results", []):
        name = entry.get("name")
        if not name:
            raise HarnessError(f"{path}: a result with no name")
        if name in out:
            raise HarnessError(f"{path}: two results named {name!r}")
        out[name] = {ours: entry.get(theirs) for ours, theirs in METRICS.items()}
    if not out:
        raise HarnessError(f"{path}: no results")
    return out


def cpu_topology(cpu: int) -> dict:
    """Where a logical CPU sits, as far as an unprivileged process can tell.

    Recorded because on this machine it is the whole question. zen5-desktop is a
    Ryzen 9 9950X3D: two core complexes, and only one of them carries the
    stacked cache — 96 MiB of L3 on CPUs 0-7 and 16-23, 32 MiB on 8-15 and
    24-31. A process the scheduler moves between them sees a different machine.
    """
    base = Path(f"/sys/devices/system/cpu/cpu{cpu}")
    def read(rel):
        try:
            return base.joinpath(rel).read_text().strip()
        except OSError:
            return None
    return {
        "cpu": cpu,
        "core_id": read("topology/core_id"),
        "package": read("topology/physical_package_id"),
        "die": read("topology/die_id"),
        "l3_size": read("cache/index3/size"),
        "l3_shared_cpu_list": read("cache/index3/shared_cpu_list"),
        "max_freq_khz": read("cpufreq/cpuinfo_max_freq"),
    }


def run_once(binary: Path, workdir: Path, pin_cpu: int | None = None,
             runtime_config: dict | None = None,
             timeout_seconds: float = 3600.0) -> dict[str, dict[str, float]]:
    """One execution, in a directory of its own.

    The binary writes `benchmark_results.json` beside itself in the working
    directory and several benchmarks create scratch databases there, so each
    execution gets a fresh directory: a leftover from the previous run is a
    warm page cache for one side and not the other.
    """
    workdir.mkdir(parents=True, exist_ok=True)
    command = [str(binary)]
    if pin_cpu is not None:
        # `taskset` rather than changing anything on the host: affinity is a
        # property of this process, it dies with it, and nothing about the
        # machine is left different afterwards.
        command = ["taskset", "-c", str(pin_cpu)] + command
    environment = dict(os.environ)
    environment.update({k: str(v) for k, v in (runtime_config or {}).items()})
    # Bounded, and every failure to launch becomes a HarnessError. Without the
    # bound a benchmark that hangs hangs the session with it, past any quiet
    # window it was waiting for; without the conversion an OSError escapes the
    # only handler `main` has and the documented fail-closed exit code is lost.
    try:
        proc = subprocess.run(
            command, cwd=workdir, capture_output=True, text=True, check=False,
            env=environment, timeout=timeout_seconds,
        )
    except subprocess.TimeoutExpired as e:
        raise HarnessError(
            f"{binary} did not finish within {timeout_seconds:g}s") from e
    except OSError as e:
        raise HarnessError(f"{binary} could not be started: {e}") from e
    if proc.returncode != 0:
        raise HarnessError(
            f"{binary} exited {proc.returncode}\n"
            f"--- stdout ---\n{proc.stdout[-2000:]}\n--- stderr ---\n{proc.stderr[-2000:]}"
        )
    results = workdir / "benchmark_results.json"
    if not results.exists():
        raise HarnessError(f"{binary} produced no {results.name}")
    return read_nanobench(results)


def comparison_kind(sides: dict, identical_binaries: bool) -> str:
    """What sort of comparison this is, so nobody has to infer it from a hash.

    Three cases, and they mean different things:

      different_builds     the ordinary A/B. Any benchmark may move, including
                           through relinking: on mac two builds differing only in
                           one benchmark's source shared 46% of their bytes.
      runtime_configured   one image, one SHA-256, different run-time settings.
                           Every benchmark outside the configured one executes
                           the same bytes, so a finding there is the instrument.
      identical            same image, same settings — an A/A control, whether it
                           was meant as one or arrived by accident.

    The middle case used to be indistinguishable from the third, and the report
    would announce "every difference below is the machine talking" over a run
    that did contain a deliberate change.
    """
    if not identical_binaries:
        return "different_builds"
    if sides["A"].get("runtime_config") != sides["B"].get("runtime_config"):
        return "runtime_configured"
    return "identical"


def comparison_identity(sides: dict) -> dict:
    """What must be the same on both sides for the comparison to mean anything.

    Separate from `host_id`, which says *where* the measurement happened. This
    says *what was compared*: two builds that differ in compiler, dependency set,
    build type or options are not a measurement of the source change, they are a
    measurement of the toolchain, and no amount of pairing fixes that.

    A difference here fails closed rather than being reported as a caveat. A
    caveat at the bottom of a report is read once; a comparison that refuses to
    run is read every time.
    """
    fields = ("toolchain", "dependencies", "build_type", "options")
    shared, differing = {}, {}
    for field in fields:
        a, b = sides["A"].get(field), sides["B"].get(field)
        if a == b:
            shared[field] = a
        else:
            differing[field] = {"A": a, "B": b}
    if differing:
        detail = "; ".join(
            f"{k}: A={v['A']!r} B={v['B']!r}" for k, v in sorted(differing.items())
        )
        raise HarnessError(
            "the two sides were not built the same way, so a difference between "
            f"them is not a difference in the source: {detail}"
        )
    return shared


def resolve_binary(label: str, path: str) -> Path:
    """An absolute path, always.

    Every execution runs in a working directory of its own — a fresh one per
    round, so no run inherits the previous one's warm page cache — which means a
    relative path names something different each time and usually nothing at
    all. Resolved once, here, where the error can still say which side it was.
    """
    resolved = Path(path).expanduser().resolve()
    if not resolved.exists():
        raise HarnessError(f"side {label}: {path} resolves to {resolved}, which does not exist")
    return resolved


def describe_side(label: str, binary: Path, commit: str | None, flags: str | None,
                  tree: str | None = None, toolchain: str | None = None,
                  dependencies: str | None = None, build_type: str | None = None,
                  options: str | None = None, runtime_config: dict | None = None) -> dict:
    if not binary.exists():
        raise HarnessError(f"side {label}: {binary} does not exist")
    if not os.access(binary, os.X_OK):
        raise HarnessError(f"side {label}: {binary} is not executable")
    return {
        "label": label,
        "binary": str(binary),
        "binary_sha256": sha256_of(binary),
        "binary_bytes": binary.stat().st_size,
        "commit": commit,
        "tree": tree,
        "runtime_config": runtime_config,
        "build_flags": flags,
        "toolchain": toolchain,
        "dependencies": dependencies,
        "build_type": build_type,
        "options": options,
    }


def _sysctl(name: str) -> str | None:
    """One sysctl value on Darwin, or None. Never raises."""
    try:
        out = subprocess.run(["sysctl", "-n", name], capture_output=True,
                             text=True, check=False, timeout=5)
        return out.stdout.strip() or None
    except (OSError, subprocess.SubprocessError):
        return None


def cpu_model() -> str | None:
    """The CPU's own name for itself, wherever this platform keeps it.

    Linux has /proc/cpuinfo, Darwin has a sysctl, and neither is assumed: an
    absent answer is None, which is a different fact from an empty string and
    stays a different fact all the way to the report.
    """
    try:
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.startswith("model name"):
                return line.split(":", 1)[1].strip()
    except OSError:
        pass
    return _sysctl("machdep.cpu.brand_string") or _sysctl("hw.model")


def total_memory_bytes() -> int | None:
    try:
        return os.sysconf("SC_PHYS_PAGES") * os.sysconf("SC_PAGE_SIZE")
    except (ValueError, OSError, AttributeError):
        pass
    value = _sysctl("hw.memsize")
    return int(value) if value and value.isdigit() else None


def harness_commit() -> str | None:
    """The commit of the scripts doing the measuring.

    Distinct from the commit the binary was built from, and they can genuinely
    differ: the first R3a sessions ran a harness at aff6fb1 against binaries
    built at ae99bad. That was legitimate — the only difference between those two
    commits is fourteen lines of `bench_session.py`, and nothing that reaches the
    binary changed — but a manifest that records one commit cannot say so.

    None when this is not a checkout, which is a fact worth recording rather than
    an error: a harness copied to a host without its history still measures.
    """
    try:
        result = subprocess.run(
            ["git", "-C", str(Path(__file__).resolve().parent), "rev-parse", "HEAD"],
            capture_output=True, text=True, timeout=10, check=False)
        return result.stdout.strip() or None
    except (OSError, subprocess.SubprocessError):
        return None


def host_identity(declared: str | None) -> dict:
    """Who this machine is, in terms that do not change between runs.

    Deliberately free of anything dynamic. Load average, observed frequency,
    free memory, the power mode of the moment — none of it belongs here: put a
    load average in the identity and the same machine becomes a different host
    on every run, which makes "compare two runs on the same host" impossible to
    express. Those live in `run_environment`, where they are evidence about the
    conditions rather than about the machine.
    """
    fields = {
        "declared": declared or platform.node(),
        "system": platform.system(),
        "release": platform.release(),
        "machine": platform.machine(),
        "cpu_model": cpu_model(),
        "cpu_count": os.cpu_count(),
        "memory_bytes": total_memory_bytes(),
    }
    stable = json.dumps(fields, sort_keys=True)
    return {**fields, "host_id": hashlib.sha256(stable.encode()).hexdigest()[:16]}


def run_environment() -> dict:
    """The conditions of this particular run, which are expected to differ.

    Recorded so that a measurement taken while the machine was compiling can be
    recognised as such afterwards — the first A/A control on zen5-desktop was taken
    at a load average of 9.53 and its noise floors were three times what a quiet
    machine gives.
    """
    try:
        load1, load5, load15 = os.getloadavg()
    except (OSError, AttributeError):
        load1 = load5 = load15 = None
    governors = []
    try:
        for path in sorted(Path("/sys/devices/system/cpu").glob("cpu*/cpufreq/scaling_governor")):
            governors.append(path.read_text().strip())
    except OSError:
        pass
    return {
        "timestamp_utc": _now(),
        "load_average": {"1m": load1, "5m": load5, "15m": load15},
        "cpu_governors": sorted(set(governors)) or None,
        "perf_event_paranoid": _read_or_none("/proc/sys/kernel/perf_event_paranoid"),
        "thermal_pressure": _sysctl("machdep.xcpm.cpu_thermal_level"),
        "is_github_hosted": bool(os.environ.get("GITHUB_ACTIONS")),
        "github_run_id": os.environ.get("GITHUB_RUN_ID"),
    }


def _read_or_none(path: str) -> str | None:
    try:
        return Path(path).read_text().strip()
    except OSError:
        return None


def _now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


def quiesce(seconds: float, label: str) -> dict:
    """Wait, and say what the machine was doing on either side of the wait.

    Compiling and measuring in the same breath is how the first zen5-desktop control
    was spoiled. The build has to be finished and its heat and its page cache
    given time to settle before the first timed iteration, and the only way to
    show that happened is to record the load before and after.
    """
    before = run_environment()["load_average"]
    if seconds > 0:
        print(f"{label}: settling for {seconds:g}s", flush=True)
        time.sleep(seconds)
    after = run_environment()["load_average"]
    return {"phase": label, "seconds": seconds, "load_before": before, "load_after": after}


def run(args: argparse.Namespace) -> int:
    binary_a = resolve_binary("A", args.binary_a)
    binary_b = resolve_binary("B", args.binary_b)
    args.binary_a, args.binary_b = str(binary_a), str(binary_b)

    a = describe_side("A", binary_a, args.commit_a, args.flags,
                      tree=args.tree_a, toolchain=args.toolchain_a,
                      dependencies=args.dependencies_a,
                      build_type=args.build_type, options=args.options,
                      runtime_config=parse_runtime_config(args.runtime_config_a))
    b = describe_side("B", binary_b, args.commit_b, args.flags,
                      tree=args.tree_b, toolchain=args.toolchain_b,
                      dependencies=args.dependencies_b,
                      build_type=args.build_type, options=args.options,
                      runtime_config=parse_runtime_config(args.runtime_config_b))

    # Before a single round is measured. Two builds that differ in toolchain or
    # dependencies are not a measurement of the source change, and finding that
    # out after an hour of rounds helps nobody.
    identity = comparison_identity({"A": a, "B": b})

    scratch = Path(args.scratch) if args.scratch else Path(tempfile.mkdtemp(prefix="bench-ab-"))
    scratch_root = scratch
    made_scratch = args.scratch is None

    order = draw_order(args.seed, args.rounds)
    phases = [quiesce(args.cooldown, "cooldown")]
    samples: dict[str, dict[str, list]] = {}
    executions: list[dict] = []
    names_seen: set[str] | None = None

    try:
        # Inside the guard, so a warmup that throws still has its scratch cleaned
        # up. Thrown away on purpose: the first execution after a build pays for a
        # cold page cache and a cold branch predictor, and whichever side happened
        # to go first would pay it alone.
        for i in range(args.warmup):
            for side in ("A", "B"):
                run_once(Path(args.binary_a if side == "A" else args.binary_b),
                         Path(scratch_root) / f"warmup{i}-{side}", args.pin_cpu,
                         (a if side == "A" else b)["runtime_config"],
                     args.execution_timeout)
        phases.append({"phase": "warmup", "rounds": args.warmup, "discarded": True})

        for round_index, first in enumerate(order):
            for position, side in enumerate(first):
                binary = Path(args.binary_a if side == "A" else args.binary_b)
                results = run_once(binary, scratch / f"r{round_index}-{side}", args.pin_cpu,
                                   (a if side == "A" else b)["runtime_config"],
                                   args.execution_timeout)

                names = set(results)
                if names_seen is None:
                    names_seen = names
                elif names != names_seen:
                    missing = sorted(names_seen - names)
                    extra = sorted(names - names_seen)
                    raise HarnessError(
                        "the two sides do not run the same benchmarks: "
                        f"missing={missing} unexpected={extra}"
                    )

                executions.append(
                    {"round": round_index, "position": position, "side": side}
                )
                for name, metrics in results.items():
                    slot = samples.setdefault(name, {"A": [], "B": []})
                    slot[side].append(metrics)
            print(f"round {round_index + 1}/{args.rounds} ({first})", flush=True)
    finally:
        if made_scratch and not args.keep_scratch:
            shutil.rmtree(scratch, ignore_errors=True)

    phases.append({"phase": "measure", "rounds": args.rounds,
                   "load_after": run_environment()["load_average"]})

    artifact = {
        "schema": SCHEMA,
        "mode": args.mode,
        "harness": {
            "seed": args.seed,
            "rounds": args.rounds,
            "warmup": args.warmup,
            "order": order,
            "executions": executions,
            "interleaved": True,
            "phases": phases,
        },
        "sides": {"A": a, "B": b},
        "identical_binaries": a["binary_sha256"] == b["binary_sha256"],
        "comparison_kind": comparison_kind({"A": a, "B": b},
                                           a["binary_sha256"] == b["binary_sha256"]),
        # Stable identity and run conditions are separate on purpose: putting a
        # load average in the identity would make the same machine a different
        # host on every run.
        "host": host_identity(args.host),
        # Provenance, split three ways because the three can differ:
        #   harness_commit       the scripts that measured
        #   binary_source_commit the tree the binary was compiled from
        #   binary_sha256        the image actually executed, per side
        "harness_commit": harness_commit(),
        "provenance": {
            "harness_commit": harness_commit(),
            "binary_source_commit": {"A": a.get("commit"), "B": b.get("commit")},
            "binary_sha256": {"A": a["binary_sha256"], "B": b["binary_sha256"]},
        },
        "comparison_identity": identity,
        # Named `environment` because that is what bench_analyze reads. The two
        # drifted apart when the function was renamed and the artifact key went
        # with it, so the analyser recorded None for every run's conditions.
        "environment": run_environment(),
        "pinned_cpu": (cpu_topology(args.pin_cpu) if args.pin_cpu is not None else None),
        "benchmarks": samples,
    }
    Path(args.output).write_text(json.dumps(artifact, indent=2))
    print(
        f"wrote {args.output}: {len(samples)} benchmarks, {args.rounds} paired rounds"
        + (", binaries are byte-identical" if artifact["identical_binaries"] else "")
    )
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary-a", required=True, help="the base side")
    parser.add_argument("--binary-b", required=True, help="the head side")
    parser.add_argument("--rounds", type=int, default=15)
    parser.add_argument("--seed", type=int, required=True,
                        help="published in the artifact; fixes the A/B order")
    parser.add_argument("--commit-a", default=None)
    parser.add_argument("--commit-b", default=None)
    parser.add_argument("--flags", default=None, help="build configuration, recorded verbatim")
    parser.add_argument("--tree-a", default=None)
    parser.add_argument("--tree-b", default=None)
    parser.add_argument("--toolchain-a", default=None)
    parser.add_argument("--toolchain-b", default=None)
    parser.add_argument("--dependencies-a", default=None)
    parser.add_argument("--dependencies-b", default=None)
    parser.add_argument("--build-type", default=None)
    parser.add_argument("--options", default=None)
    parser.add_argument("--runtime-config-a", default=None,
                        help="KEY=VALUE,... applied to side A's environment")
    parser.add_argument("--runtime-config-b", default=None,
                        help="KEY=VALUE,... applied to side B's environment")
    parser.add_argument("--mode", choices=["ab", "aa"], default="ab",
                        help="aa marks a control run whose two sides are the same build")
    parser.add_argument("--host", default=None,
                        help="the declared host name; defaults to this machine's")
    parser.add_argument("--warmup", type=int, default=2,
                        help="paired runs taken before measuring and thrown away")
    parser.add_argument("--cooldown", type=float, default=0.0,
                        help="seconds to let the machine settle after the builds")
    parser.add_argument("--execution-timeout", type=float, default=3600.0,
                        help="seconds a single benchmark execution may take")
    parser.add_argument("--pin-cpu", type=int, default=None,
                        help="run every execution on this logical CPU via taskset")
    parser.add_argument("--scratch", default=None)
    parser.add_argument("--keep-scratch", action="store_true")
    parser.add_argument("--output", default="bench_ab_samples.json")
    args = parser.parse_args(argv)

    if args.rounds < 2:
        print("error: at least two rounds are needed to pair anything", file=sys.stderr)
        return 2
    try:
        return run(args)
    except HarnessError as e:
        # Fail closed. A harness that cannot vouch for what it measured must not
        # hand back numbers that look like a verdict.
        print(f"harness error: {e}", file=sys.stderr)
        return 3


if __name__ == "__main__":
    raise SystemExit(main())
