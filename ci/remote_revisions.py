#!/usr/bin/env python3

"""Which recipe revisions a remote holds for a reference, or why we cannot say.

`conan list` exits 0 whether the reference is there, absent, or the remote is
unreachable — the outcome is a field in its JSON, not a status. Reading that with
`if ! conan list` treats "the query failed" as "the package is not there", which
is how a network blip becomes a missing package and a missing package becomes a
release nobody notices is broken.

The three outcomes are separated, and each exits differently:

    0  the remote holds it; the revisions are printed, one per line
    3  the remote answered, and said it does not have it
    4  the query did not succeed — network, auth, an unknown remote, or an error
       that cannot be read as a definite answer

Only 0 is an answer. 4 is never absence: not knowing is its own outcome, and a
caller that collapses it into "missing" has invented a fact.

Absence is only ever claimed on the remote's own words. The existence query is
made separately from the revisions query for exactly that reason: asked for a
reference it does not have, the remote replies `Recipe 'utxoz/x' not found`,
which is unambiguous — while asking for revisions of the same missing reference
replies with an empty error message, which is not.

Usage:  remote_revisions.py <remote> <name/version>
"""

from __future__ import annotations

import json
import os
import subprocess
import sys

EXIT_OK = 0
EXIT_ABSENT = 3
EXIT_UNKNOWN = 4

# A query that never returns is not an answer either. Without a bound, a stalled
# Conan process holds the publish job open until the workflow's own timeout kills
# it — which reports the job as timed out rather than as "we could not establish
# whether the package is there", and loses the one distinction this program
# exists to make. Generous, because a slow remote is not a stalled one.
QUERY_TIMEOUT_SECONDS = float(os.environ.get("UTXOZ_REMOTE_QUERY_TIMEOUT", "120"))


class Unknown(Exception):
    """The query did not produce an answer either way."""


def as_object(value: object, what: str) -> dict:
    """`value` as a JSON object, or Unknown.

    json.loads accepts any valid JSON, not only the shape Conan documents, and
    the wrong shape is not a small problem here: `{"kth": "invalid"}` makes
    `"error" in reply` a substring test and `reference not in reply` another,
    so a malformed answer sails through as *absence*. Every level is checked
    before it is read.
    """
    if not isinstance(value, dict):
        raise Unknown(f"{what} is {type(value).__name__}, not an object")
    return value


def query(remote: str, pattern: str) -> dict:
    """The remote's reply, or Unknown. Never a guess."""
    try:
        completed = subprocess.run(
            ["conan", "list", pattern, "-r", remote, "--format=json"],
            capture_output=True,
            text=True,
            timeout=QUERY_TIMEOUT_SECONDS,
        )
    except subprocess.TimeoutExpired:
        raise Unknown(f"conan list did not return within {QUERY_TIMEOUT_SECONDS:g}s")
    except OSError as error:
        raise Unknown(f"could not run conan list: {error}")
    if completed.returncode != 0:
        raise Unknown(
            f"conan list exited {completed.returncode}\n{completed.stderr.strip()}"
        )
    try:
        payload = json.loads(completed.stdout)
    except json.JSONDecodeError as error:
        raise Unknown(f"output is not JSON: {error}\n{completed.stdout[:1000]}")
    payload = as_object(payload, "the reply")
    if remote not in payload:
        raise Unknown(f"the reply says nothing about remote {remote!r}")
    return as_object(payload[remote], f"the entry for remote {remote!r}")


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <remote> <name/version>", file=sys.stderr)
        return EXIT_UNKNOWN

    remote, reference = sys.argv[1], sys.argv[2]

    try:
        # 1. Does the remote have it? Only this query's error text is trusted to
        #    mean absence, because only this one says so in words.
        existence = query(remote, reference)
        if "error" in existence:
            message = str(existence["error"])
            if f"'{reference}' not found" in message:
                print(f"the remote does not have {reference}", file=sys.stderr)
                print(f"  {message}", file=sys.stderr)
                return EXIT_ABSENT
            raise Unknown(message)
        if reference not in existence:
            # Not absence. The remote said nothing about this reference, and
            # silence is not a denial — only its own "not found" is. Inferring
            # absence from a reply about something else is the mistake this
            # program exists to avoid, one level up.
            listed = ", ".join(sorted(existence)) or "(nothing)"
            raise Unknown(f"the reply did not mention {reference}; it listed: {listed}")

        # 2. Which revisions. By here the reference exists, so an error is a
        #    failure to read rather than an absence.
        listing = query(remote, f"{reference}#*")
        if "error" in listing:
            raise Unknown(str(listing["error"]))
        entry = as_object(listing.get(reference, {}), f"the entry for {reference}")
        revisions = as_object(entry.get("revisions", {}), f"the revisions of {reference}")
    except Unknown as error:
        print(f"could not determine whether {remote} has {reference}", file=sys.stderr)
        for line in str(error).splitlines():
            print(f"  {line}", file=sys.stderr)
        return EXIT_UNKNOWN

    if not revisions:
        # The reference exists and has no revision: nothing to install.
        print(f"{reference} is on the remote with no recipe revision", file=sys.stderr)
        return EXIT_ABSENT

    for revision in revisions:
        print(revision)
    return EXIT_OK


if __name__ == "__main__":
    raise SystemExit(main())
