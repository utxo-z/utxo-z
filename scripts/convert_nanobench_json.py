#!/usr/bin/env python3
"""Convert nanobench JSON output to github-action-benchmark format.

Reads nanobench JSON from benchmark_results.json and writes
the converted format to benchmark_results_gh.json.

The output format is 'customBiggerIsBetter' with ops/sec as the unit,
which is what benchmark-action/github-action-benchmark@v1 expects.
"""

import json
import sys
from pathlib import Path


def convert(input_path: str, output_path: str) -> None:
    with open(input_path, "r") as f:
        data = json.load(f)

    results = []
    for entry in data.get("results", []):
        name = entry.get("name", "unknown")
        # nanobench reports median elapsed time in seconds
        median_s = entry.get("median(elapsed)", 0.0)
        if median_s > 0:
            ops_per_sec = 1.0 / median_s
        else:
            ops_per_sec = 0.0

        results.append({
            "name": name,
            "unit": "ops/sec",
            "value": round(ops_per_sec, 2),
        })

    with open(output_path, "w") as f:
        json.dump(results, f, indent=2)

    print(f"Converted {len(results)} benchmarks: {input_path} -> {output_path}")


def main() -> None:
    input_path = sys.argv[1] if len(sys.argv) > 1 else "benchmark_results.json"
    output_path = sys.argv[2] if len(sys.argv) > 2 else "benchmark_results_gh.json"

    if not Path(input_path).exists():
        print(f"Error: {input_path} not found", file=sys.stderr)
        sys.exit(1)

    convert(input_path, output_path)


if __name__ == "__main__":
    main()
