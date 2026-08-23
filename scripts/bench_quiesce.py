#!/usr/bin/env python3
"""Is this machine quiet enough to time something on, and how do we know?

Why not a load average
----------------------

The first version of this waited for `load1 < 2`. That is a single absolute
number standing in for a question that is relative to the machine: on mac's 12
cores a load of 2 is a sixth of the box, on zen5-desktop's 32 it is a sixteenth, and
on fedora's 128 it is nothing at all. Worse, the load average is an exponentially
weighted average with a one-minute time constant, so it stays high for minutes
after the CPU has gone idle — a build that finished thirty seconds ago still
reads as busy, and a machine that is genuinely busy in bursts can read as calm.

So there is no single boolean here. Each signal is measured where the platform
offers it, reported with a status when it does not, and the decision says which
signals it was made from. A window that cannot be shown to be quiet is not
waited on forever: it is refused, and the measurement taken in it is marked.
"""

from __future__ import annotations

import os
import platform
import re
import subprocess
import time
from pathlib import Path

AVAILABLE = "available"
UNAVAILABLE = "unavailable"

# Names that mean this machine is building something — including this harness's
# own builds, which are the ones most likely to still be running.
BUSY_PROCESS = re.compile(
    r"\b(cc1plus|cc1|clang-\d+|ld\.lld|ld\.gold|collect2|cmake|ninja|make|"
    r"conan|utxoz_benchmarks|utxoz_tests)\b"
)


def _signal(value, status=AVAILABLE, **extra):
    return {"value": value, "status": status, **extra}


def _unavailable(reason: str):
    return {"value": None, "status": UNAVAILABLE, "reason": reason}


def normalised_load() -> dict:
    """Load average over logical cores, which is the comparable form.

    Still slow to fall, so it is never the only signal — but a normalised load
    well above 1 does mean more runnable work than the machine has cores, on any
    of the three hosts.
    """
    cores = os.cpu_count() or 1
    try:
        load1, load5, load15 = os.getloadavg()
    except (OSError, AttributeError) as e:
        return _unavailable(f"no load average on this platform ({e})")
    return _signal(load1 / cores, load1=load1, load5=load5, load15=load15, cores=cores)


def cpu_utilisation(sample_seconds: float = 0.5) -> dict:
    """The fraction of the machine that is busy *now*, not over the last minute.

    This is the signal the load average cannot give: it responds immediately, so
    it is what distinguishes "a build finished a moment ago" from "a build is
    still running".
    """
    if platform.system() == "Linux":
        try:
            first = _proc_stat_total()
            time.sleep(sample_seconds)
            second = _proc_stat_total()
        except OSError as e:
            return _unavailable(f"/proc/stat unreadable ({e})")
        busy = (second["busy"] - first["busy"])
        total = (second["total"] - first["total"])
        if total <= 0:
            return _unavailable("no jiffies elapsed between samples")
        return _signal(busy / total, method="/proc/stat delta", sample_seconds=sample_seconds)

    if platform.system() == "Darwin":
        # `top`, not `iostat`. An earlier version parsed iostat's last three
        # numeric columns as `us sy id` and took the last one as idle. Those
        # columns are the one-, five- and fifteen-minute load averages: the
        # parser read a load average of 2.47 as "2.47% idle" and reported a
        # machine that was 94% idle as 97.5% busy, which stopped a session from
        # ever starting. `top` names the field.
        try:
            out = subprocess.run(["top", "-l", "2", "-n", "0", "-s", "1"],
                                 capture_output=True, text=True, timeout=20, check=False)
        except (OSError, subprocess.SubprocessError) as e:
            return _unavailable(f"top unavailable ({e})")
        idle = None
        for line in out.stdout.splitlines():
            if not line.startswith("CPU usage:"):
                continue
            match = re.search(r"([0-9.]+)%\s+idle", line)
            if match:
                idle = float(match.group(1))   # the second sample wins
        if idle is None:
            return _unavailable("top printed no CPU usage line")
        return _signal(max(0.0, 1.0 - idle / 100.0), method="top -l 2 idle")

    return _unavailable(f"no CPU utilisation source for {platform.system()}")


def _is_number(text: str) -> bool:
    try:
        float(text)
        return True
    except ValueError:
        return False


def _proc_stat_total() -> dict:
    fields = Path("/proc/stat").read_text().split("\n", 1)[0].split()[1:]
    values = [int(v) for v in fields]
    idle = values[3] + (values[4] if len(values) > 4 else 0)
    total = sum(values)
    return {"busy": total - idle, "total": total}


def run_queue() -> dict:
    """Runnable tasks other than the one asking. Linux only.

    `procs_running` counts the observer: the process reading /proc/stat is
    itself runnable at that instant, so the counter never reads below 1 and a
    limit of 1 silently means "nothing else at all". That is not what it looks
    like it means, and it does not survive contact with a real machine — the
    first session to use it sat in the quiescence loop for thirty-three minutes
    on an idle box, because the counter flickered between 1 and 2 and three
    consecutive samples of exactly 1 never came up.

    Subtracting the observer makes the number say what the limit assumes: how
    much else is competing. Darwin has no equivalent that can be read without
    elevated privileges.
    """
    try:
        for line in Path("/proc/stat").read_text().splitlines():
            if line.startswith("procs_running"):
                raw = int(line.split()[1])
                return _signal(max(0, raw - 1), raw=raw, excludes_observer=True)
    except OSError as e:
        return _unavailable(f"/proc/stat unreadable ({e})")
    return _unavailable(f"no run queue counter on {platform.system()}")


def competing_processes() -> dict:
    """Builds and benchmarks that are still running, by name.

    The one signal that can say *what* is keeping the machine busy, which is
    what makes an aborted window actionable instead of merely disappointing.
    """
    # PID and name, because the exclusion has to be by PID. An earlier version
    # asked `ps` for names only and then did `hits.pop(str(os.getpid()))` on a
    # dictionary keyed by name — which could never match, so the observer counted
    # itself whenever it happened to be named like a build tool.
    try:
        out = subprocess.run(["ps", "-eo", "pid=,comm="], capture_output=True,
                             text=True, timeout=15, check=False)
    except (OSError, subprocess.SubprocessError) as e:
        return _unavailable(f"ps unavailable ({e})")
    hits: dict[str, int] = {}
    mine = {os.getpid(), os.getppid()}
    for line in out.stdout.splitlines():
        parts = line.split(None, 1)
        if len(parts) != 2:
            continue
        try:
            pid = int(parts[0])
        except ValueError:
            continue
        if pid in mine:
            continue
        name = parts[1].strip().rsplit("/", 1)[-1]
        if BUSY_PROCESS.search(name):
            hits[name] = hits.get(name, 0) + 1
    return _signal(sum(hits.values()), processes=hits or None)


def frequency_and_power() -> dict:
    """Whatever the platform will say about clocks and power, without sudo.

    Observed and recorded, never changed: a governor left on `powersave` is a
    fact about the measurement, and switching it would be a change to Fernando's
    machine that nobody asked for.
    """
    system = platform.system()
    if system == "Linux":
        governors: dict[str, int] = {}
        freqs = []
        try:
            for path in sorted(Path("/sys/devices/system/cpu").glob("cpu*/cpufreq/scaling_governor")):
                g = path.read_text().strip()
                governors[g] = governors.get(g, 0) + 1
            for path in sorted(Path("/sys/devices/system/cpu").glob("cpu*/cpufreq/scaling_cur_freq")):
                freqs.append(int(path.read_text().strip()))
        except (OSError, ValueError):
            pass
        if not governors and not freqs:
            return _unavailable("no cpufreq interface")
        return _signal(governors or None,
                       mean_khz=(sum(freqs) // len(freqs)) if freqs else None)
    if system == "Darwin":
        try:
            out = subprocess.run(["pmset", "-g", "therm"], capture_output=True,
                                 text=True, timeout=10, check=False)
            text = out.stdout.strip()
        except (OSError, subprocess.SubprocessError) as e:
            return _unavailable(f"pmset unavailable ({e})")
        return _signal(text or None, method="pmset -g therm")
    return _unavailable(f"no frequency source for {system}")


def assess(max_normalised_load: float, max_utilisation: float,
           max_run_queue: int, seconds_since_build: float | None,
           min_seconds_since_build: float) -> dict:
    """Every signal, and a decision that names the ones it rested on.

    A signal the platform cannot provide does not block the window and does not
    silently count as a pass: it is listed as unavailable, and the decision says
    which signals actually carried it. That matters most on mac, where the run
    queue simply does not exist.
    """
    signals = {
        "normalised_load": normalised_load(),
        "cpu_utilisation": cpu_utilisation(),
        "run_queue": run_queue(),
        "competing_processes": competing_processes(),
        "frequency_and_power": frequency_and_power(),
        "seconds_since_build": (_signal(seconds_since_build)
                                if seconds_since_build is not None
                                else _unavailable("no build was run by this session")),
    }

    checks: list[tuple[str, bool, str]] = []

    load = signals["normalised_load"]
    if load["status"] == AVAILABLE:
        ok = load["value"] <= max_normalised_load
        checks.append(("normalised_load", ok,
                       f"{load['value']:.2f} per core vs limit {max_normalised_load:.2f}"))

    util = signals["cpu_utilisation"]
    if util["status"] == AVAILABLE:
        ok = util["value"] <= max_utilisation
        checks.append(("cpu_utilisation", ok,
                       f"{util['value']*100:.1f}% busy vs limit {max_utilisation*100:.0f}%"))

    queue = signals["run_queue"]
    if queue["status"] == AVAILABLE:
        ok = queue["value"] <= max_run_queue
        checks.append(("run_queue", ok, f"{queue['value']} runnable vs limit {max_run_queue}"))

    procs = signals["competing_processes"]
    if procs["status"] == AVAILABLE:
        ok = procs["value"] == 0
        detail = ", ".join(f"{k}×{v}" for k, v in (procs.get("processes") or {}).items())
        checks.append(("competing_processes", ok, detail or "none running"))

    since = signals["seconds_since_build"]
    if since["status"] == AVAILABLE:
        ok = since["value"] >= min_seconds_since_build
        checks.append(("seconds_since_build", ok,
                       f"{since['value']:.0f}s since the last build vs minimum "
                       f"{min_seconds_since_build:.0f}s"))

    failed = [name for name, ok, _ in checks if not ok]
    quiet = not failed and bool(checks)
    if not checks:
        reason = "no quiescence signal was available on this platform"
    elif quiet:
        reason = "; ".join(f"{name}: {detail}" for name, _, detail in checks)
    else:
        reason = "; ".join(f"{name}: {detail}" for name, ok, detail in checks if not ok)

    return {
        "quiet": quiet,
        "decided_from": [name for name, _, _ in checks],
        "unavailable": [k for k, v in signals.items() if v["status"] != AVAILABLE],
        "failed": failed,
        "reason": reason,
        "limits": {
            "max_normalised_load": max_normalised_load,
            "max_utilisation": max_utilisation,
            "max_run_queue": max_run_queue,
            "min_seconds_since_build": min_seconds_since_build,
        },
        "signals": signals,
    }


# A starting point for zen5-desktop, and only for zen5-desktop: 32 cores, a `powersave`
# governor, and a machine Fernando also works on. mac has 12 cores and no run
# queue to read; fedora has 128 and a load of 1 there is a rounding error. The
# numbers are arguments, they travel in the artifact, and nothing here claims
# they are right for another host.
ZEN5_DESKTOP_LIMITS = {
    "max_utilisation": 0.10,
    "max_normalised_load": 0.10,
    "max_run_queue": 1,
    "min_seconds_since_build": 60.0,
}
CONSECUTIVE_SAMPLES = 3


def wait_for_quiescence(timeout_seconds: float, poll_seconds: float = 10.0,
                        consecutive: int = CONSECUTIVE_SAMPLES,
                        build_finished_at: float | None = None, **limits) -> dict:
    """Poll until the machine has been quiet for several samples running, or give up.

    Several samples, not one. Every signal here is a snapshot, and a build that
    is between two translation units looks idle for a moment; a single quiet
    reading is as likely to be a gap as a finish. Requiring a run of them costs
    a few seconds and removes the whole class of "it looked idle just then".

    Never waits forever. A window that never arrives comes back with `quiet`
    false, and the caller marks the session `contaminated` and says why — more
    useful than a harness that hangs, and more honest than one that measures
    anyway and does not mention it.
    """
    deadline = time.monotonic() + timeout_seconds
    attempts: list[dict] = []
    streak = 0
    while True:
        # Recomputed every pass, not once. An earlier version measured the age of
        # the build when the session started and then reused that number, so a
        # session launched fourteen seconds after a build waited for the sixty it
        # would never reach: the value it was comparing could not grow.
        if build_finished_at is not None:
            limits["seconds_since_build"] = time.time() - build_finished_at
        state = assess(**limits)
        streak = streak + 1 if state["quiet"] else 0
        attempts.append({
            "at": len(attempts),
            "quiet": state["quiet"],
            "streak": streak,
            "reason": state["reason"],
        })
        settled = streak >= consecutive
        expired = time.monotonic() >= deadline
        if settled or expired:
            state["quiet"] = settled
            state["attempts"] = attempts
            state["consecutive_required"] = consecutive
            state["consecutive_achieved"] = streak
            state["waited_seconds"] = round(
                timeout_seconds - max(0.0, deadline - time.monotonic()), 1)
            state["timed_out"] = expired and not settled
            if not settled:
                state["reason"] = (
                    f"only {streak} consecutive quiet samples of {consecutive} required; "
                    f"last: {state['reason']}"
                )
            return state
        if state["quiet"]:
            print(f"quiet {streak}/{consecutive}", flush=True)
        else:
            print(f"not quiet yet: {state['reason']}", flush=True)
        time.sleep(poll_seconds)


if __name__ == "__main__":
    import json
    print(json.dumps(assess(ZEN5_DESKTOP_LIMITS["max_normalised_load"],
                            ZEN5_DESKTOP_LIMITS["max_utilisation"],
                            ZEN5_DESKTOP_LIMITS["max_run_queue"], None,
                            ZEN5_DESKTOP_LIMITS["min_seconds_since_build"]),
                     indent=2, default=str))
