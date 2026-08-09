#!/usr/bin/env python3

"""Exactly one recipe revision from a `conan list --format=json` payload, on stdin.

Separated from the shell so the query's status and the parse are two distinct
failures. `conan list | python3` reports python's status, so a failed query would
arrive here as empty input and leave the caller unable to tell "the query broke"
from "the package has no revision".

Exits 0 and prints the revision only when there is exactly one. None and several
are both refused: taking the first of several would pin the publish check to a
revision chosen by iteration order.

Usage:  conan list "<ref>#latest" --format=json | one_revision.py <ref>
"""

from __future__ import annotations

import json
import sys


def as_object(value: object, what: str) -> dict:
    """`value` as a JSON object, or a diagnostic and exit 2.

    json.loads accepts any valid JSON, not only the shape Conan documents, and
    the wrong shape fails in ways that look like answers: a list under
    "Local Cache" raises AttributeError, and `{"revisions": "a"}` yields the
    single "revision" `a`. Every level is checked before it is read.
    """
    if not isinstance(value, dict):
        print(f"{what} is {type(value).__name__}, not an object", file=sys.stderr)
        raise SystemExit(2)
    return value


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <name/version>", file=sys.stderr)
        return 2
    reference = sys.argv[1]

    raw = sys.stdin.read()
    if not raw.strip():
        print("conan list produced no output to parse", file=sys.stderr)
        return 2
    try:
        payload = json.loads(raw)
    except json.JSONDecodeError as error:
        print(f"conan list output is not JSON: {error}", file=sys.stderr)
        return 2

    payload = as_object(payload, "the conan list output")

    # The local cache is the only source here; a remote key would mean the caller
    # asked the wrong question.
    if "Local Cache" not in payload:
        print(f"no Local Cache in the listing; it named: {', '.join(payload) or '(nothing)'}",
              file=sys.stderr)
        return 2
    cache = as_object(payload["Local Cache"], "the Local Cache entry")
    if "error" in cache:
        print(f"conan list reported an error: {cache['error']}", file=sys.stderr)
        return 2

    if reference not in cache:
        print(f"the cache does not list {reference}", file=sys.stderr)
        return 2
    entry = as_object(cache[reference], f"the entry for {reference}")

    revisions = list(as_object(entry.get("revisions", {}), f"the revisions of {reference}"))
    if len(revisions) == 1:
        print(revisions[0])
        return 0
    if not revisions:
        print(f"{reference} is in the cache with no recipe revision", file=sys.stderr)
        return 2
    print(f"{reference} has {len(revisions)} recipe revisions: {', '.join(revisions)}",
          file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
