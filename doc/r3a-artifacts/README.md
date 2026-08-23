# R3a measurement artifacts

The raw output of the sessions is 1,736 nanobench JSON files across three hosts
— too much to carry in a repository, and unnecessary: what a reader needs is to
be able to tell whether the files they have are the files the report was written
from.

Each `<host>.json` here lists, per session: its label and status, whether the
machine was quiet before and after, how long it took, the SHA-256 of the
manifest and of each `samples.json`, and a digest over every raw file — the
SHA-256 of the concatenated per-file hashes, in path order. Provenance is
recorded per phase: comparison kind, harness commit, the commit each side's
binary was built from, the binary's own SHA-256, the runtime configuration, the
round count and the seed.

To check a directory against this index:

    python3 - <<'PY'
    import hashlib, glob, json
    index = json.load(open("doc/r3a-artifacts/zen5-desktop.json"))
    for session in index["sessions"]:
        problems = []

        # Every named file, by its own hash. Checking only the raw digest would
        # pass a session whose manifest or samples had been edited.
        for name, recorded in session["files"].items():
            path = f"{session['directory']}/{name}"
            try:
                actual = hashlib.sha256(open(path, "rb").read()).hexdigest()
            except OSError:
                problems.append(f"{name}: missing")
                continue
            if actual != recorded["sha256"]:
                problems.append(f"{name}: hash differs")

        # And the raw files, as one digest over their hashes in path order.
        agg = hashlib.sha256()
        raw = sorted(glob.glob(session["directory"] + "/**/benchmark_results.json",
                               recursive=True))
        for f in raw:
            agg.update(hashlib.sha256(open(f, "rb").read()).hexdigest().encode())
        if len(raw) != session["raw_count"]:
            problems.append(f"raw: {len(raw)} files, expected {session['raw_count']}")
        elif agg.hexdigest() != session["raw_digest"]:
            problems.append("raw: digest differs")

        print(f"{session['label']}: " + ("ok" if not problems else "; ".join(problems)))
    PY

The full raw files live on the hosts, under `bench-runs/<host>/` in each
benchmark worktree.
