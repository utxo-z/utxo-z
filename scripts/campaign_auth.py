#!/usr/bin/env python3
"""Whether a timed campaign may run here, now: an explicit, external grant.

A timed campaign shares the machine with other work, and a quiet `pgrep` or a
low load average says nothing about whether that work has agreed to give the
machine up. This module does not decide that. It reads a grant written by the
coordinator - never by the runner, never by the session that wants to measure -
and refuses unless every field says this run, on this host, for this task, now.

The grant is a JSON file:

    {
      "schema": "utxoz-campaign-authorization-1",
      "host": "<platform.node() of the machine>",
      "task_id": "<the spec's task_id>",
      "expires_utc": "YYYY-MM-DDTHH:MM:SSZ",
      "approval_code": "<the coordinator's code for this window>",
      "granted_by": "<who granted it>"
    }

Fail closed, every way: a missing file, unreadable JSON, a missing or empty
field, another host, another task, an expiry that is past, unparseable, or more
than a day away. The runner checks it before creating anything and again before
every sample, so a window that closes mid-campaign stops the campaign.

Nothing here can tell a genuine grant from one somebody wrote for themselves.
That is the point of keeping it external and of recording its hash in every
config: the record says which grant a campaign ran under, and the coordinator
knows which grants they gave.
"""
import datetime as _dt
import hashlib
import json
import platform
from pathlib import Path

SCHEMA = "utxoz-campaign-authorization-1"
FIELDS = ("schema", "host", "task_id", "expires_utc", "approval_code", "granted_by")
MAX_WINDOW = _dt.timedelta(days=1)


def _parse_utc(text):
    if not isinstance(text, str) or not text.endswith("Z"):
        return None
    try:
        return _dt.datetime.strptime(text, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=_dt.timezone.utc)
    except ValueError:
        return None


def check(path, task_id, now=None):
    """(grant, None) if the campaign may run, or (None, the reason it may not)."""
    if not path:
        return None, "no authorization file was given; a timed campaign needs one"
    p = Path(path)
    if not p.is_file():
        return None, f"authorization {p} does not exist"
    try:
        text = p.read_text()
        grant = json.loads(text)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as e:
        return None, f"authorization {p} is not readable JSON ({e})"
    if not isinstance(grant, dict):
        return None, "the authorization is not an object"
    for f in FIELDS:
        if not isinstance(grant.get(f), str) or not grant[f].strip():
            return None, f"the authorization has no {f}"
    if grant["schema"] != SCHEMA:
        return None, f"the authorization schema is {grant['schema']!r}"
    if grant["host"] != platform.node():
        return None, f"the authorization is for host {grant['host']!r}, not {platform.node()!r}"
    if grant["task_id"] != task_id:
        return None, f"the authorization is for task {grant['task_id']!r}, not {task_id!r}"
    expires = _parse_utc(grant["expires_utc"])
    if expires is None:
        return None, f"expires_utc {grant['expires_utc']!r} is not YYYY-MM-DDTHH:MM:SSZ"
    now = now or _dt.datetime.now(_dt.timezone.utc)
    if now >= expires:
        return None, f"the authorization expired at {grant['expires_utc']}"
    if expires - now > MAX_WINDOW:
        return None, "the authorization's window is longer than a day; refuse rather than guess"
    grant = dict(grant)
    grant["sha256"] = hashlib.sha256(text.encode()).hexdigest()
    grant["path"] = str(p.resolve())
    return grant, None


def still_valid(grant, now=None):
    """None while the grant's window is open, else the reason it is not."""
    now = now or _dt.datetime.now(_dt.timezone.utc)
    if now >= _parse_utc(grant["expires_utc"]):
        return f"the authorization expired at {grant['expires_utc']}"
    return None
