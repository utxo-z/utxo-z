#!/usr/bin/env python3
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Which run publishes, and which runs validate — evaluated, not described.

The release authority (#104) is a property of two files together: the workflow,
whose triggers and job conditions decide what runs, and ci/release_authority.sh,
which decides what those conditions read. A model of either kept here would be a
third copy that drifts. So this reads the real workflow, runs the real script in
a real git repository, and evaluates each job's `if:` for every case that
matters:

  same-repository release and hotfix pushes, their pull requests (up to date and
  stale), ordinary pull requests, pull requests from forks whose branch is named
  release/* or hotfix/*, a version tag, master, main and workflow_dispatch.

The expression evaluator understands the handful of constructs these conditions
use and refuses anything else, so a condition rewritten in a form it cannot read
fails here instead of being skipped.

The old policy is the negative control. Every check runs again against the
workflow and the script with the old behaviour put back — a tag that publishes, a
release pull request that runs the matrix, a fork trusted for its branch name —
and each must be caught. A check that passes on what it was written to reject is
not checking anything.

The gate the release pull request runs, ci/await_push_validation.sh, is tested
here too, against a `gh` that answers from files and a clock that only moves
when the script sleeps.

Standard library only. Run from anywhere:  python3 ci/test_release_authority.py
"""

from __future__ import annotations

import json
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
WORKFLOW = ROOT / ".github" / "workflows" / "ci.yml"
AUTHORITY = ROOT / "ci" / "release_authority.sh"
GATE = ROOT / "ci" / "await_push_validation.sh"

REPO = "example/utxo-z"
FORK = "someone/utxo-z"

failures = 0


def ok(name: str) -> None:
    print(f"ok    {name}")


def bad(name: str, detail: str = "") -> None:
    global failures
    failures += 1
    print(f"FAIL  {name}" + (f": {detail}" if detail else ""))


# =============================================================================
# Reading the workflow
# =============================================================================
#
# Not a YAML parser: the parts of ci.yml this needs, read by their indentation,
# which the file keeps strictly (two spaces per level). Anything it expects and
# does not find is an error, not an absence.


class WorkflowError(Exception):
    pass


def _block(lines: list[str], start: int, indent: int) -> tuple[list[str], int]:
    """The lines after `start` that are indented deeper than `indent`."""
    out = []
    i = start + 1
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        if stripped and not stripped.startswith("#"):
            if len(line) - len(line.lstrip(" ")) <= indent:
                break
        out.append(line)
        i += 1
    return out, i


def _scalar(text: str) -> str:
    text = text.strip()
    if len(text) >= 2 and text[0] == text[-1] and text[0] in "'\"":
        return text[1:-1]
    return text


def _flow_list(text: str) -> list[str]:
    text = text.strip()
    if text.startswith("[") and text.endswith("]"):
        return [_scalar(p) for p in text[1:-1].split(",") if p.strip()]
    return [_scalar(text)]


def parse_triggers(text: str) -> dict:
    lines = text.splitlines()
    for i, line in enumerate(lines):
        if line.rstrip() == "on:":
            body, _ = _block(lines, i, 0)
            break
    else:
        raise WorkflowError("no top-level `on:`")
    triggers: dict = {}
    current = None
    for line in body:
        if not line.strip() or line.strip().startswith("#"):
            continue
        indent = len(line) - len(line.lstrip(" "))
        key, _, value = line.strip().partition(":")
        if indent == 2:
            current = key
            triggers[current] = {}
            continue
        if indent == 4 and current is not None:
            if value.strip() and value.strip() != "{}":
                triggers[current][key] = _flow_list(value)
            else:
                triggers[current][key] = []
        elif indent == 6 and current is not None and line.strip().startswith("- "):
            last = list(triggers[current])[-1]
            triggers[current][last].append(_scalar(line.strip()[2:]))
    return triggers


def parse_jobs(text: str) -> dict[str, dict]:
    lines = text.splitlines()
    for i, line in enumerate(lines):
        if line.rstrip() == "jobs:":
            body, _ = _block(lines, i, 0)
            break
    else:
        raise WorkflowError("no top-level `jobs:`")
    jobs: dict[str, dict] = {}
    i = 0
    while i < len(body):
        m = re.match(r"^  ([A-Za-z0-9_-]+):\s*$", body[i])
        if not m:
            i += 1
            continue
        name = m.group(1)
        job_lines, nxt = _block(body, i, 2)
        jobs[name] = _parse_job(job_lines)
        jobs[name]["text"] = "\n".join(job_lines)
        i = nxt
    return jobs


def _parse_job(lines: list[str]) -> dict:
    job: dict = {"needs": [], "if": None}
    i = 0
    while i < len(lines):
        line = lines[i]
        m = re.match(r"^    ([A-Za-z0-9_-]+):(.*)$", line)
        if not m:
            i += 1
            continue
        key, value = m.group(1), m.group(2).strip()
        if key == "needs":
            job["needs"] = _flow_list(value)
        elif key == "if":
            if value in (">-", ">", "|", "|-"):
                block, nxt = _block(lines, i, 4)
                joiner = " " if value.startswith(">") else "\n"
                job["if"] = joiner.join(b.strip() for b in block if b.strip())
                i = nxt
                continue
            job["if"] = _scalar(value)
        i += 1
    return job


# =============================================================================
# Evaluating a condition
# =============================================================================
#
# The subset of the GitHub Actions expression language these conditions use:
# context paths, single-quoted strings, == != && || ! and parentheses, and
# startsWith(). String comparison is case-insensitive, as it is on GitHub. A
# condition may be written with or without ${{ }}.


class ExpressionError(Exception):
    pass


TOKEN = re.compile(r"\s*(?:(?P<str>'(?:[^']|'')*')|(?P<op>==|!=|&&|\|\||!|\(|\)|,)"
                   r"|(?P<name>[A-Za-z_][A-Za-z0-9_.-]*))")


def _tokens(expr: str) -> list[tuple[str, str]]:
    expr = expr.strip()
    if expr.startswith("${{") and expr.endswith("}}"):
        expr = expr[3:-2]
    out, pos = [], 0
    while pos < len(expr):
        if expr[pos:].strip() == "":
            break
        m = TOKEN.match(expr, pos)
        if not m or m.end() == pos:
            raise ExpressionError(f"cannot read {expr[pos:]!r}")
        kind = m.lastgroup
        out.append((kind, m.group(kind)))
        pos = m.end()
    return out


def evaluate(expr: str, context: dict[str, str]):
    tokens = _tokens(expr)
    pos = 0

    def peek():
        return tokens[pos] if pos < len(tokens) else (None, None)

    def take(value=None):
        nonlocal pos
        kind, val = peek()
        if value is not None and val != value:
            raise ExpressionError(f"expected {value!r}, found {val!r} in {expr!r}")
        pos += 1
        return kind, val

    def truthy(v) -> bool:
        return bool(v) and v != "false" if isinstance(v, str) else bool(v)

    def primary():
        kind, val = peek()
        if val == "(":
            take("(")
            v = disjunction()
            take(")")
            return v
        if val == "!":
            take("!")
            return not truthy(primary())
        if kind == "str":
            take()
            return val[1:-1].replace("''", "'")
        if kind == "name":
            take()
            if peek()[1] == "(":
                if val != "startsWith":
                    raise ExpressionError(f"function {val}() is not understood here")
                take("(")
                a = disjunction()
                take(",")
                b = disjunction()
                take(")")
                return str(a).lower().startswith(str(b).lower())
            if val in ("true", "false"):
                return val == "true"
            if val not in context:
                raise ExpressionError(f"{val} is not a context this test knows")
            return context[val]
        raise ExpressionError(f"unexpected {val!r} in {expr!r}")

    def comparison():
        left = primary()
        while peek()[1] in ("==", "!="):
            op = take()[1]
            right = primary()
            equal = str(left).lower() == str(right).lower()
            left = equal if op == "==" else not equal
        return left

    def conjunction():
        left = comparison()
        while peek()[1] == "&&":
            take()
            right = comparison()
            left = right if truthy(left) else left
        return left

    def disjunction():
        left = conjunction()
        while peek()[1] == "||":
            take()
            right = conjunction()
            left = left if truthy(left) else right
        return left

    value = disjunction()
    if pos != len(tokens):
        raise ExpressionError(f"trailing input in {expr!r}")
    return truthy(value)


# =============================================================================
# Triggers
# =============================================================================


def glob_matches(pattern: str, value: str) -> bool:
    regex = ""
    i = 0
    while i < len(pattern):
        if pattern.startswith("**", i):
            regex += ".*"
            i += 2
        elif pattern[i] == "*":
            regex += "[^/]*"
            i += 1
        else:
            regex += re.escape(pattern[i])
            i += 1
    return re.fullmatch(regex, value) is not None


def triggered(triggers: dict, case: dict) -> bool:
    event = case["event"]
    if event not in triggers:
        return False
    spec = triggers[event]
    if event == "push":
        ref = case["ref"]
        if ref.startswith("refs/tags/"):
            return any(glob_matches(p, ref[len("refs/tags/"):]) for p in spec.get("tags", []))
        if ref.startswith("refs/heads/"):
            if "branches" not in spec:
                return "tags" not in spec
            return any(glob_matches(p, ref[len("refs/heads/"):]) for p in spec["branches"])
        return False
    if event == "pull_request":
        if "branches" not in spec:
            return True
        return any(glob_matches(p, case["base"]) for p in spec["branches"])
    return True


# =============================================================================
# The cases
# =============================================================================


def git(repo: Path, *args: str) -> str:
    return subprocess.run(["git", *args], cwd=repo, check=True, capture_output=True,
                          text=True).stdout.strip()


def make_history(where: Path) -> dict[str, str]:
    """base, a head that contains it, and a head that does not."""
    git(where, "init", "-q")
    git(where, "config", "user.email", "test@example.invalid")
    git(where, "config", "user.name", "test")
    git(where, "config", "commit.gpgsign", "false")
    git(where, "commit", "-q", "--allow-empty", "-m", "root")
    root = git(where, "rev-parse", "HEAD")
    git(where, "commit", "-q", "--allow-empty", "-m", "base")
    base = git(where, "rev-parse", "HEAD")
    git(where, "commit", "-q", "--allow-empty", "-m", "release")
    current = git(where, "rev-parse", "HEAD")
    git(where, "checkout", "-q", "-b", "stale", root)
    git(where, "commit", "-q", "--allow-empty", "-m", "release on an old base")
    stale = git(where, "rev-parse", "HEAD")
    return {"base": base, "current": current, "stale": stale}


def cases(shas: dict[str, str]) -> list[dict]:
    missing = "f" * 40

    def push(name, ref, publishes):
        return {"name": name, "event": "push", "ref": ref, "expect": "full",
                "publishes": publishes}

    def pr(name, head_ref, head_repo, head, expect):
        return {"name": name, "event": "pull_request", "ref": "refs/pull/7/merge",
                "base": "master", "head_ref": head_ref, "head_repo": head_repo,
                "base_sha": shas["base"], "head_sha": head, "expect": expect,
                "publishes": False}

    return [
        push("same-repository release push", "refs/heads/release/1.2.3", True),
        push("same-repository hotfix push", "refs/heads/hotfix/1.2.4", True),
        pr("release pull request, head contains base", "release/1.2.3", REPO,
           shas["current"], "by-push"),
        pr("hotfix pull request, head contains base", "hotfix/1.2.4", REPO,
           shas["current"], "by-push"),
        pr("release pull request, head does not contain base", "release/1.2.3", REPO,
           shas["stale"], "full"),
        pr("release pull request, head commit not in the clone", "release/1.2.3", REPO,
           missing, "full"),
        pr("ordinary pull request", "feature/faster-lookups", REPO, shas["current"], "full"),
        pr("fork pull request named release/*", "release/1.2.3", FORK, shas["current"], "full"),
        pr("fork pull request named hotfix/*", "hotfix/1.2.4", FORK, shas["current"], "full"),
        pr("pull request from a deleted fork", "release/1.2.3", "", shas["current"], "full"),
        {"name": "version tag", "event": "push", "ref": "refs/tags/v1.2.3",
         "expect": "nothing", "publishes": False},
        push("master push", "refs/heads/master", False),
        push("main push", "refs/heads/main", False),
        {"name": "workflow_dispatch on a release branch", "event": "workflow_dispatch",
         "ref": "refs/heads/release/1.2.3", "expect": "full", "publishes": False},
    ]


def run_authority(script: Path, repo: Path, case: dict) -> dict[str, str] | None:
    env = {
        "PATH": os.environ["PATH"],
        "EVENT_NAME": case["event"],
        "GIT_REF": case["ref"],
        "REPOSITORY": REPO,
        "PR_HEAD_REPO": case.get("head_repo", ""),
        "PR_HEAD_REF": case.get("head_ref", ""),
        "PR_BASE_SHA": case.get("base_sha", ""),
        "PR_HEAD_SHA": case.get("head_sha", ""),
    }
    done = subprocess.run(["bash", str(script)], cwd=repo, env=env, capture_output=True,
                          text=True)
    if done.returncode != 0:
        return None
    outputs = {}
    for line in done.stdout.splitlines():
        key, _, value = line.partition("=")
        outputs[key] = value
    return outputs


def context_for(case: dict, outputs: dict[str, str]) -> dict[str, str]:
    return {
        "github.event_name": case["event"],
        "github.ref": case["ref"],
        "github.head_ref": case.get("head_ref", ""),
        "github.repository": REPO,
        "github.event.pull_request.head.repo.full_name": case.get("head_repo", ""),
        "needs.setup.outputs.validation": outputs.get("validation", ""),
        "needs.setup.outputs.publish": outputs.get("publish", ""),
        "needs.setup.outputs.build-version": "1.2.3",
    }


def jobs_that_run(jobs: dict[str, dict], context: dict[str, str]) -> set[str]:
    ran: set[str] = set()
    pending = dict(jobs)
    while pending:
        progressed = False
        for name, job in list(pending.items()):
            if any(n in pending for n in job["needs"]):
                continue
            del pending[name]
            progressed = True
            if job["if"] and re.search(r"\b(always|failure|cancelled|success)\s*\(", job["if"]):
                raise ExpressionError(f"{name}: status functions change skip semantics; "
                                      "teach this test about them before using one")
            if not all(n in ran for n in job["needs"]):
                continue    # a needed job was skipped, so this one is too
            if job["if"] is None or evaluate(job["if"], context):
                ran.add(name)
        if not progressed:
            raise WorkflowError(f"needs form a cycle or name a missing job: {sorted(pending)}")
    return ran


def policy_violations(workflow_text: str, script: Path, repo: Path,
                      shas: dict[str, str]) -> list[str]:
    """Everything wrong with this workflow and script, as the release authority sees it."""
    found: list[str] = []
    try:
        triggers = parse_triggers(workflow_text)
        jobs = parse_jobs(workflow_text)
    except WorkflowError as e:
        return [f"the workflow cannot be read: {e}"]

    for required in ("setup", "publish", "build", "publish-dry-run"):
        if required not in jobs:
            found.append(f"no `{required}` job")
    if found:
        return found

    validation_jobs = {n for n in jobs if n not in ("setup", "publish", "release-validation")}

    if "tags" in triggers.get("push", {}):
        found.append("a tag push triggers the workflow; the tag is a record")

    for case in cases(shas):
        name = case["name"]
        outputs = run_authority(script, repo, case)
        if outputs is None:
            found.append(f"{name}: release_authority.sh failed")
            continue
        if not triggered(triggers, case):
            ran: set[str] = set()
        else:
            try:
                ran = jobs_that_run(jobs, context_for(case, outputs))
            except (ExpressionError, WorkflowError) as e:
                found.append(f"{name}: {e}")
                continue

        if case["expect"] == "nothing":
            if ran:
                found.append(f"{name}: runs {sorted(ran)}; it should run nothing")
            continue

        published = "publish" in ran
        if published != case["publishes"]:
            found.append(f"{name}: publish {'runs' if published else 'does not run'}")

        if case["expect"] == "by-push":
            extra = ran - {"setup", "release-validation"}
            if extra:
                found.append(f"{name}: the matrix runs again: {sorted(extra)}")
            if "release-validation" not in ran:
                found.append(f"{name}: nothing validates it — the matrix is skipped and "
                             "release-validation does not run")
        else:
            missing = validation_jobs - ran
            if missing:
                found.append(f"{name}: validation skipped: {sorted(missing)}")
            if "release-validation" in ran:
                found.append(f"{name}: release-validation runs where the matrix is the validation")

    # No output of setup may make anything but a push publish.
    forced = {"event": "pull_request", "ref": "refs/pull/7/merge", "base": "master",
              "head_ref": "release/1.2.3", "head_repo": REPO}
    try:
        if evaluate(jobs["publish"]["if"] or "true",
                    context_for(forced, {"validation": "full", "publish": "true"})):
            found.append("publish would run on a pull request if setup said so")
    except ExpressionError as e:
        found.append(f"publish: {e}")

    # The wiring between setup and the script.
    setup = jobs["setup"]["text"]
    for needle in ("validation: ${{ steps.authority.outputs.validation }}",
                   "publish: ${{ steps.authority.outputs.publish }}",
                   "id: authority",
                   "./ci/release_authority.sh >> \"$GITHUB_OUTPUT\"",
                   "EVENT_NAME: ${{ github.event_name }}",
                   "GIT_REF: ${{ github.ref }}",
                   "REPOSITORY: ${{ github.repository }}",
                   "PR_HEAD_REPO: ${{ github.event.pull_request.head.repo.full_name }}",
                   "PR_HEAD_REF: ${{ github.head_ref }}",
                   "PR_BASE_SHA: ${{ github.event.pull_request.base.sha }}",
                   "PR_HEAD_SHA: ${{ github.event.pull_request.head.sha }}",
                   "python3 ci/test_release_authority.py"):
        if needle not in setup:
            found.append(f"setup does not contain {needle!r}")
    if "release-validation" in jobs:
        gate = jobs["release-validation"]["text"]
        for needle in ("actions: read", "timeout-minutes:", "./ci/await_push_validation.sh",
                       "HEAD_SHA: ${{ github.event.pull_request.head.sha }}",
                       "HEAD_REF: ${{ github.head_ref }}",
                       "GH_TOKEN: ${{ github.token }}"):
            if needle not in gate:
                found.append(f"release-validation does not contain {needle!r}")
    return found


# =============================================================================
# The gate
# =============================================================================

SHA = "0123456789abcdef0123456789abcdef01234567"
OTHER = "fedcba9876543210fedcba9876543210fedcba98"


def run(sha, branch, repo, event, status, conclusion, run_id):
    return {"id": run_id, "head_sha": sha, "head_branch": branch, "event": event,
            "status": status, "conclusion": conclusion,
            "head_repository": {"full_name": repo} if repo else None,
            "html_url": f"https://example.invalid/runs/{run_id}"}


def gate_case(work: Path, name: str, replies: list, expected: int, *,
              sha: str = SHA, timeout: int = 100, clock_moves: bool = True,
              max_calls: int | None = None) -> None:
    """Run the gate against `replies`, one per question (the last repeats).

    A reply is a list of runs, or None for an API failure.
    """
    case = work / re.sub(r"[^a-z0-9]+", "-", name.lower())
    bindir = case / "bin"
    bindir.mkdir(parents=True)
    for i, reply in enumerate(replies, start=1):
        (case / f"reply.{i}").write_text("FAIL" if reply is None
                                         else json.dumps({"workflow_runs": reply}))
    (case / "calls").write_text("0")
    (case / "clock").write_text("1000")

    fake_gh = bindir / "gh"
    fake_gh.write_text(f"""#!/usr/bin/env bash
n=$(( $(cat "{case}/calls") + 1 )); echo "$n" > "{case}/calls"
printf '%s\\n' "$*" >> "{case}/args"
f="{case}/reply.$n"; [[ -f "$f" ]] || f="{case}/reply.{len(replies)}"
if [[ "$(cat "$f")" == FAIL ]]; then echo "HTTP 502" >&2; exit 1; fi
cat "$f"
""")
    clock = bindir / "fake-clock"
    clock.write_text(f'#!/usr/bin/env bash\ncat "{case}/clock"\n')
    sleeper = bindir / "fake-sleep"
    step = "$1" if clock_moves else "0"
    sleeper.write_text(f'#!/usr/bin/env bash\necho $(( $(cat "{case}/clock") + {step} )) '
                       f'> "{case}/clock"\n')
    for f in (fake_gh, clock, sleeper):
        f.chmod(f.stat().st_mode | stat.S_IEXEC)

    env = {"PATH": f"{bindir}:{os.environ['PATH']}", "REPOSITORY": REPO, "HEAD_SHA": sha,
           "HEAD_REF": "release/1.2.3", "UTXOZ_GATE_TIMEOUT": str(timeout),
           "UTXOZ_GATE_POLL": "10", "UTXOZ_GATE_CLOCK": str(clock),
           "UTXOZ_GATE_SLEEP": str(sleeper)}
    try:
        done = subprocess.run(["bash", str(GATE)], env=env, capture_output=True, text=True,
                              timeout=60)
    except subprocess.TimeoutExpired:
        bad(f"gate: {name}", "did not finish: the wait is not bounded")
        return
    calls = int((case / "calls").read_text())
    if done.returncode != expected:
        bad(f"gate: {name}", f"exit {done.returncode}, expected {expected}\n"
                             f"{done.stdout}{done.stderr}")
    elif max_calls is not None and calls > max_calls:
        bad(f"gate: {name}", f"asked {calls} times, expected at most {max_calls}")
    else:
        ok(f"gate: {name} (exit {done.returncode}, {calls} questions)")
    args = (case / "args").read_text() if (case / "args").exists() else ""
    if calls and (f"head_sha={sha}" not in args or "event=push" not in args):
        bad(f"gate: {name}", f"the query does not name the commit and the event: {args!r}")


def test_gate(work: Path) -> None:
    mine = lambda status, conclusion, run_id=10: run(SHA, "release/1.2.3", REPO, "push",
                                                      status, conclusion, run_id)
    gate_case(work, "push run already succeeded", [[mine("completed", "success")]], 0)
    gate_case(work, "push run succeeds after running",
              [[mine("queued", None)], [mine("in_progress", None)],
               [mine("completed", "success")]], 0)
    gate_case(work, "push run failed", [[mine("completed", "failure")]], 1)
    gate_case(work, "push run cancelled", [[mine("completed", "cancelled")]], 1)
    gate_case(work, "push run appears late and succeeds",
              [[], [], [mine("completed", "success")]], 0)
    gate_case(work, "no push run ever appears", [[]], 3)
    gate_case(work, "still running at the deadline", [[mine("in_progress", None)]], 4)
    gate_case(work, "the API never answers", [None], 4)
    gate_case(work, "the API fails, then answers", [None, [mine("completed", "success")]], 0)
    gate_case(work, "runs of other commits, events, branches and forks do not count",
              [[run(OTHER, "release/1.2.3", REPO, "push", "completed", "success", 1),
                run(SHA, "release/1.2.3", REPO, "pull_request", "completed", "success", 2),
                run(SHA, "release/9.9.9", REPO, "push", "completed", "success", 3),
                run(SHA, "release/1.2.3", FORK, "push", "completed", "success", 4),
                run(SHA, "release/1.2.3", None, "push", "completed", "success", 5)]], 3)
    gate_case(work, "the newest run decides: older success, newer failure",
              [[mine("completed", "success", 10), mine("completed", "failure", 11)]], 1)
    gate_case(work, "the newest run decides: older failure, newer success",
              [[mine("completed", "failure", 10), mine("completed", "success", 11)]], 0)
    gate_case(work, "a commit id that is not one is refused before asking",
              [[mine("completed", "success")]], 4, sha="main; rm -rf /", max_calls=0)
    gate_case(work, "a clock that stops still ends the wait", [[]], 3, clock_moves=False,
              max_calls=100 // 10 + 2)


# =============================================================================
# Negative controls
# =============================================================================


def mutate(text: str, old: str, new: str, what: str) -> str:
    if old not in text:
        raise SystemExit(f"negative control '{what}' no longer applies: {old!r} is not in "
                         "the workflow. Update the control rather than drop it.")
    return text.replace(old, new)


OLD_PUBLISH_IF = """    if: >-
      github.event_name == 'push' && (
        startsWith(github.ref, 'refs/tags/v') ||
        startsWith(github.ref, 'refs/heads/release/') ||
        startsWith(github.ref, 'refs/heads/hotfix/')
      )"""

NEW_PUBLISH_IF = "    if: github.event_name == 'push' && needs.setup.outputs.publish == 'true'"
FULL_IF = "    if: needs.setup.outputs.validation == 'full'\n"
TAG_TRIGGER = ("    branches: [main, master, 'release/**', 'hotfix/**']\n  pull_request:",
               "    branches: [main, master, 'release/**', 'hotfix/**']\n    tags:\n"
               "      - v*\n  pull_request:")


def negative_controls(workflow: str, repo: Path, shas: dict[str, str], work: Path) -> None:
    controls: list[tuple[str, str, Path]] = []

    tagged = mutate(workflow, *TAG_TRIGGER, "tag trigger")
    controls.append(("a tag push triggers the workflow again", tagged, AUTHORITY))
    controls.append(("the tag publishes again (the old publish condition)",
                     mutate(tagged, NEW_PUBLISH_IF, OLD_PUBLISH_IF, "old publish if"),
                     AUTHORITY))
    build_header = ("  build:\n    needs: [setup]\n"
                    "    # Every validation job carries this condition, and only this one: "
                    "`full`\n    # everywhere except a release pull request whose push run "
                    "is the validation.\n" + FULL_IF)
    controls.append(("the release pull request runs the matrix again (build unconditional)",
                     mutate(workflow, build_header, "  build:\n    needs: [setup]\n",
                            "build if"),
                     AUTHORITY))
    controls.append(("every validation job unconditional (the old workflow's shape)",
                     mutate(workflow, FULL_IF, "", "validation if"), AUTHORITY))
    gate_start = workflow.index("  release-validation:\n")
    gate_end = workflow.index("  build:\n", gate_start)
    controls.append(("the gate is removed, so a skipped matrix is validated by nothing",
                     workflow[:gate_start] + workflow[gate_end:], AUTHORITY))
    controls.append(("publish trusts setup alone, without the event check",
                     mutate(workflow, NEW_PUBLISH_IF,
                            "    if: needs.setup.outputs.publish == 'true'", "publish event"),
                     AUTHORITY))

    script = AUTHORITY.read_text()
    trusting = work / "authority-trusts-forks.sh"
    trusting.write_text(mutate(script, '[[ "${head_repo}" != "${REPO}" ]]', "false",
                               "same-repository check"))
    controls.append(("a fork is trusted for its branch name", workflow, trusting))
    blind = work / "authority-ignores-ancestry.sh"
    blind.write_text(mutate(script,
                            'git merge-base --is-ancestor "${PR_BASE_SHA}" "${PR_HEAD_SHA}" 2>/dev/null',
                            "true", "ancestry check"))
    controls.append(("a stale release head is deferred to its push run", workflow, blind))

    for name, text, authority in controls:
        caught = policy_violations(text, authority, repo, shas)
        if caught:
            ok(f"negative control caught: {name} ({caught[0]})")
        else:
            bad(f"negative control NOT caught: {name}")


# =============================================================================


def main() -> int:
    workflow_path = WORKFLOW
    if len(sys.argv) == 3 and sys.argv[1] == "--workflow":
        # Only for looking at another revision by hand; CI runs it with no argument.
        workflow_path = Path(sys.argv[2])
    elif len(sys.argv) != 1:
        print("usage: test_release_authority.py [--workflow PATH]", file=sys.stderr)
        return 2

    if shutil.which("git") is None:
        print("FAIL  git is required", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory() as tmp:
        work = Path(tmp)
        repo = work / "history"
        repo.mkdir()
        shas = make_history(repo)

        # The script on its own: what it prints, and that it refuses missing input.
        for case in cases(shas):
            if case["expect"] == "nothing":
                continue
            out = run_authority(AUTHORITY, repo, case)
            want = {"validation": case["expect"], "publish": str(case["publishes"]).lower()}
            if out == want:
                ok(f"authority: {case['name']} -> {want['validation']}, publish={want['publish']}")
            else:
                bad(f"authority: {case['name']}", f"got {out}, expected {want}")
        tag = run_authority(AUTHORITY, repo, {"event": "push", "ref": "refs/tags/v1.2.3"})
        if tag == {"validation": "full", "publish": "false"}:
            ok("authority: a tag that did arrive would publish nothing")
        else:
            bad("authority: tag", f"got {tag}")
        missing = subprocess.run(["bash", str(AUTHORITY)], cwd=repo, capture_output=True,
                                 text=True, env={"PATH": os.environ["PATH"]})
        if missing.returncode != 0 and missing.stdout == "":
            ok("authority: missing input is an error, with nothing on stdout")
        else:
            bad("authority: missing input", f"exit {missing.returncode}, {missing.stdout!r}")

        # The workflow and the script together.
        workflow = workflow_path.read_text()
        found = policy_violations(workflow, AUTHORITY, repo, shas)
        if found:
            for f in found:
                bad("policy", f)
        else:
            ok(f"policy: {workflow_path.name} has the release authority, in every case")

        if workflow_path == WORKFLOW:
            negative_controls(workflow, repo, shas, work)
            test_gate(work)

    print(f"\n{failures} failure(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
