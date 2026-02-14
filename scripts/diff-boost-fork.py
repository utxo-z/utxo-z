#!/usr/bin/env python3
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Show a diff between the original boost/unordered files and our
# forked src/flatmap/ files, paired by the FILE_MAP from fork_boost_unordered.py.
#
# Usage:
#   python3 scripts/diff-boost-fork.py                  # colored diff, all files
#   python3 scripts/diff-boost-fork.py --stat           # summary: lines changed per file
#   python3 scripts/diff-boost-fork.py --semantic       # only semantic changes (skip renames)
#   python3 scripts/diff-boost-fork.py --semantic --stat
#   python3 scripts/diff-boost-fork.py core.hpp         # diff only files matching pattern
#   python3 scripts/diff-boost-fork.py --semantic core.hpp

import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_DIR = SCRIPT_DIR.parent

# Import FILE_MAP and OPTIONAL_FILES from the fork script.
# Save/restore sys.argv to prevent it from parsing our CLI args.
_saved_argv = sys.argv
sys.argv = [sys.argv[0]]
sys.path.insert(0, str(SCRIPT_DIR))
from fork_boost_unordered import FILE_MAP, OPTIONAL_FILES, BOOST_TAG
sys.argv = _saved_argv
FOA_DIR = PROJECT_DIR / "src" / "utxoz" / "flatmap"

GREEN = "\033[32m"
RED = "\033[31m"
DIM = "\033[90m"
RESET = "\033[0m"


def apply_mechanical_renames(txt: str) -> str:
    """Apply the same mechanical renames that fork_boost_unordered.py does,
    so that diffing the result against our fork shows only semantic changes."""

    # Guards (order: most specific first)
    txt = txt.replace("BOOST_UNORDERED_DETAIL_FOA_", "UTXOZ_FLATMAP_DETAIL_FOA_")
    txt = txt.replace("BOOST_UNORDERED_DETAIL_",     "UTXOZ_FLATMAP_DETAIL_")
    txt = txt.replace("BOOST_UNORDERED_",            "UTXOZ_FLATMAP_")

    # Qualified names (most specific first)
    txt = txt.replace("boost::unordered::detail", "utxoz::flatmap::detail")
    txt = txt.replace("boost::unordered", "utxoz::flatmap")

    # Namespace declarations (preserve spacing)
    txt = re.sub(r'namespace\s+boost(\s*)\{', r'namespace utxoz\1{', txt)
    txt = re.sub(r'namespace\s+unordered(\s*)\{', r'namespace flatmap\1{', txt)

    # Closing comments
    txt = txt.replace("/* namespace boost */",      "/* namespace utxoz */")
    txt = txt.replace("/* namespace unordered */",   "/* namespace flatmap */")
    txt = txt.replace("// namespace boost",          "// namespace utxoz")
    txt = txt.replace("// namespace unordered",      "// namespace flatmap")

    # Includes
    txt = re.sub(r'#include\s*<boost/unordered/detail/foa/([^>]+)>',
                 r'#include <utxoz/flatmap/detail/\1>', txt)
    txt = re.sub(r'#include\s*<boost/unordered/detail/([^>]+)>',
                 r'#include <utxoz/flatmap/detail/\1>', txt)
    txt = re.sub(r'#include\s*<boost/unordered/([^>]+)>',
                 r'#include <utxoz/flatmap/\1>', txt)

    # Class/type rename
    txt = txt.replace("unordered_flat_map", "mmap_flat_map")

    # boost:: qualifiers added by the fork script
    txt = re.sub(r'(?<!boost::)(?<!\w)empty_value\s*<', 'boost::empty_value<', txt)
    txt = re.sub(r'(?<!boost::)(?<!\w)(empty_init)\b(?!_)', r'boost::\1', txt)

    return txt


def clone_boost(work_dir: Path, tag: str) -> Path:
    print(f"Cloning boost/unordered at {tag} ...", file=sys.stderr)
    repo_dir = work_dir / "unordered"
    subprocess.run(
        ["git", "clone", "--depth", "1", "--branch", tag,
         "https://github.com/boostorg/unordered.git", str(repo_dir)],
        check=True, capture_output=True,
    )
    return repo_dir / "include" / "boost" / "unordered"


def diff_contents(a: str, b: str, label_a: str, label_b: str, colored: bool) -> tuple[str, int, int]:
    """Diff two strings via a temp file pair. Returns (output, added, removed)."""
    with tempfile.NamedTemporaryFile(mode="w", suffix=".hpp", delete=False) as fa, \
         tempfile.NamedTemporaryFile(mode="w", suffix=".hpp", delete=False) as fb:
        fa.write(a); fa.flush()
        fb.write(b); fb.flush()
        cmd = ["diff", "-u"]
        if colored:
            cmd.append("--color=always")
        cmd += ["--label", label_a, "--label", label_b, fa.name, fb.name]
        result = subprocess.run(cmd, capture_output=True, text=True)
        os.unlink(fa.name)
        os.unlink(fb.name)

    lines = result.stdout.splitlines()[2:]  # skip --- and +++
    added = sum(1 for l in lines if l.startswith("+"))
    removed = sum(1 for l in lines if l.startswith("-"))
    return result.stdout, added, removed


def main():
    if not FOA_DIR.is_dir():
        print(f"Error: {FOA_DIR} not found. Are you on the mmap-flat-map branch?")
        sys.exit(1)

    tag = os.environ.get("BOOST_TAG", BOOST_TAG)

    args = sys.argv[1:]
    semantic = "--semantic" in args
    stat = "--stat" in args
    args = [a for a in args if a not in ("--semantic", "--stat")]
    filter_pattern = args[0] if args else None
    mode = "stat" if stat else "diff"

    all_files = {**FILE_MAP, **OPTIONAL_FILES}

    with tempfile.TemporaryDirectory() as work_dir:
        boost_src = clone_boost(Path(work_dir), tag)

        total_files = 0
        changed_files = 0
        total_added = 0
        total_removed = 0

        for boost_rel in sorted(all_files):
            our_rel = all_files[boost_rel]
            boost_file = boost_src / boost_rel
            our_file = FOA_DIR / our_rel

            if not boost_file.exists() or not our_file.exists():
                continue

            if filter_pattern and filter_pattern not in our_rel and filter_pattern not in boost_rel:
                continue

            total_files += 1
            boost_label = f"boost/{boost_rel}"
            our_label = f"src/flatmap/{our_rel}"

            boost_content = boost_file.read_text()
            our_content = our_file.read_text()

            if semantic:
                boost_content = apply_mechanical_renames(boost_content)

            output, added, removed = diff_contents(
                boost_content, our_content, boost_label, our_label,
                colored=(mode == "diff"))

            if mode == "stat":
                if added > 0 or removed > 0:
                    changed_files += 1
                    total_added += added
                    total_removed += removed
                    print(f"  {our_rel:<45}  {GREEN}+{added:<5}{RESET}  {RED}-{removed:<5}{RESET}  (boost: {boost_rel})")
                else:
                    print(f"  {our_rel:<45}  {DIM}identical{RESET}")
            else:
                if output:
                    print(output)

        if mode == "stat":
            print()
            print(f"  {total_files} files compared, {changed_files} changed")
            print(f"  Total: {GREEN}+{total_added}{RESET}  {RED}-{total_removed}{RESET}")


if __name__ == "__main__":
    main()
