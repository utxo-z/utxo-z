#!/usr/bin/env python3
"""Compare current benchmark results against the median of recent commits.

Reads historical benchmark data from the gh-pages branch (data.js) and
compares the current run against the median of the last N data points.
This reduces false regression alerts caused by GHA runner variability.

Usage:
    python3 benchmark_compare.py <current_results.json> [--data-js <path>] [--window <N>] [--threshold <pct>]

Options:
    --data-js     Path to data.js from gh-pages (default: fetched via git show)
    --window      Number of recent commits to use for median (default: 10)
    --threshold   Regression threshold percentage (default: 15)
    --output      Output file for the markdown report (default: stdout)
"""

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path
from statistics import median


def fetch_data_js(data_dir: str) -> str | None:
    """Fetch data.js content from gh-pages branch."""
    try:
        result = subprocess.run(
            ["git", "show", f"gh-pages:{data_dir}/data.js"],
            capture_output=True, text=True, check=True,
        )
        return result.stdout
    except subprocess.CalledProcessError:
        return None


def parse_data_js(content: str) -> dict:
    """Parse the window.BENCHMARK_DATA = {...}; format."""
    # Strip the JS wrapper: "window.BENCHMARK_DATA = " prefix and trailing ";"
    match = re.search(r"window\.BENCHMARK_DATA\s*=\s*(\{.*\})\s*;?\s*$", content, re.DOTALL)
    if not match:
        return {}
    try:
        return json.loads(match.group(1))
    except json.JSONDecodeError as e:
        print(f"Warning: malformed data.js, ignoring history: {e}", file=sys.stderr)
        return {}


def get_historical_values(data: dict, window: int) -> dict[str, list[float]]:
    """Extract the last `window` values for each benchmark name.

    Returns a dict mapping benchmark name -> list of recent values (oldest first).
    """
    history: dict[str, list[float]] = {}
    entries = data.get("entries", {})
    if not isinstance(entries, dict):
        return history

    # github-action-benchmark uses the tool name or "Benchmark" as key
    for _suite_name, runs in entries.items():
        if not isinstance(runs, list):
            continue
        for run in runs:
            if not isinstance(run, dict):
                continue
            for bench in run.get("benches", []):
                if not isinstance(bench, dict):
                    continue
                name = bench.get("name", "")
                value = bench.get("value", 0.0)
                if isinstance(name, str) and name and isinstance(value, (int, float)):
                    history.setdefault(name, []).append(float(value))

    # Keep only the last `window` values
    return {name: values[-window:] for name, values in history.items()}


def compare(current_results: list[dict], history: dict[str, list[float]],
            threshold_pct: float) -> tuple[list[dict], list[dict], list[dict]]:
    """Compare current results against historical median.

    Returns (regressions, improvements, stable) where each is a list of dicts with:
        name, current, median, change_pct, samples
    """
    regressions = []
    improvements = []
    stable = []

    for bench in current_results:
        name = bench.get("name", "")
        current_value = bench.get("value", 0.0)

        past_values = history.get(name, [])
        if not past_values:
            # No history — nothing to compare against
            stable.append({
                "name": name,
                "current": current_value,
                "median": None,
                "change_pct": None,
                "samples": 0,
            })
            continue

        med = median(past_values)
        if med == 0:
            stable.append({
                "name": name,
                "current": current_value,
                "median": 0,
                "change_pct": None,
                "samples": len(past_values),
            })
            continue

        # For "bigger is better": negative change means regression
        change_pct = ((current_value - med) / med) * 100.0

        entry = {
            "name": name,
            "current": current_value,
            "median": med,
            "change_pct": change_pct,
            "samples": len(past_values),
        }

        if change_pct < -threshold_pct:
            regressions.append(entry)
        elif change_pct > threshold_pct:
            improvements.append(entry)
        else:
            stable.append(entry)

    return regressions, improvements, stable


def format_value(value: float) -> str:
    """Format a value with appropriate precision."""
    if value >= 1_000_000:
        return f"{value / 1_000_000:.2f}M"
    if value >= 1_000:
        return f"{value / 1_000:.1f}K"
    return f"{value:.1f}"


def format_report(regressions: list[dict], improvements: list[dict],
                  stable: list[dict], window: int, threshold_pct: float) -> str:
    """Generate a markdown report."""
    lines = []
    lines.append(f"## Benchmark comparison (median of last {window} commits, ±{threshold_pct:.0f}% threshold)\n")

    new_benchmarks = [s for s in stable if s["samples"] == 0]
    compared = [s for s in stable if s["samples"] > 0]
    total_compared = len(regressions) + len(improvements) + len(compared)

    if not regressions and not improvements and not new_benchmarks:
        lines.append(f"All {total_compared} benchmarks are within the expected range.\n")
        return "\n".join(lines)

    if not regressions and not improvements and not total_compared:
        lines.append("No historical data to compare against yet.\n")
        # Fall through to show new benchmarks

    if regressions:
        lines.append(f"### ⚠️ Possible regressions ({len(regressions)})\n")
        lines.append("| Benchmark | Current | Median | Change |")
        lines.append("|-----------|---------|--------|--------|")
        for r in sorted(regressions, key=lambda x: x["change_pct"]):
            lines.append(
                f"| {r['name']} | {format_value(r['current'])} | "
                f"{format_value(r['median'])} | {r['change_pct']:+.1f}% "
                f"({r['samples']} samples) |"
            )
        lines.append("")

    if improvements:
        lines.append(f"### ✅ Improvements ({len(improvements)})\n")
        lines.append("| Benchmark | Current | Median | Change |")
        lines.append("|-----------|---------|--------|--------|")
        for r in sorted(improvements, key=lambda x: x["change_pct"], reverse=True):
            lines.append(
                f"| {r['name']} | {format_value(r['current'])} | "
                f"{format_value(r['median'])} | {r['change_pct']:+.1f}% "
                f"({r['samples']} samples) |"
            )
        lines.append("")

    if new_benchmarks:
        lines.append(f"### 🆕 New benchmarks ({len(new_benchmarks)}) — no historical data\n")
        for s in new_benchmarks:
            lines.append(f"- **{s['name']}**: {format_value(s['current'])}")
        lines.append("")

    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description="Compare benchmarks against historical median")
    parser.add_argument("current_results", help="Path to current benchmark results JSON")
    parser.add_argument("--data-js", help="Path to data.js file (default: fetch from gh-pages)")
    parser.add_argument("--data-dir", default="dev/bench", help="Data dir path in gh-pages (default: dev/bench)")
    parser.add_argument("--window", type=int, default=10, help="Number of recent commits for median (default: 10)")
    parser.add_argument("--threshold", type=float, default=15.0, help="Regression threshold %% (default: 15)")
    parser.add_argument("--output", help="Output file (default: stdout)")
    args = parser.parse_args()

    if args.window < 1:
        parser.error("--window must be >= 1")
    if args.threshold < 0:
        parser.error("--threshold must be >= 0")

    # Load current results
    current_path = Path(args.current_results)
    if not current_path.exists():
        print(f"Error: {current_path} not found", file=sys.stderr)
        sys.exit(1)

    with open(current_path, encoding="utf-8") as f:
        current_results = json.load(f)

    if not isinstance(current_results, list):
        print(f"Error: {current_path} must contain a JSON array", file=sys.stderr)
        sys.exit(1)
    current_results = [
        b for b in current_results
        if isinstance(b, dict) and isinstance(b.get("name"), str) and b["name"]
        and isinstance(b.get("value"), (int, float))
    ]

    # Load historical data
    if args.data_js:
        with open(args.data_js, encoding="utf-8") as f:
            data_js_content = f.read()
    else:
        data_js_content = fetch_data_js(args.data_dir)

    if data_js_content:
        data = parse_data_js(data_js_content)
        history = get_historical_values(data, args.window)
    else:
        print("No historical data found on gh-pages. Skipping comparison.", file=sys.stderr)
        history = {}

    # Compare
    regressions, improvements, stable = compare(current_results, history, args.threshold)

    # Generate report
    report = format_report(regressions, improvements, stable, args.window, args.threshold)

    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(report)
        print(f"Report written to {args.output}")
    else:
        print(report)

    # Exit with non-zero if regressions found (for CI awareness, not failure)
    # We use exit code 0 always — the report itself communicates the results
    sys.exit(0)


if __name__ == "__main__":
    main()
