# Measuring whether a change made anything slower

## The problem this replaces

The lane that came before measured one sample of a pull request's head on
whatever GitHub runner it landed on, and compared it against the median of
samples taken on *other* runners, days earlier.

PR #141 is the control that settled what that is worth. It changed only test
files. The benchmark binary was byte-identical to master. The lane reported:

| Benchmark | Change |
|---|---|
| `apply_deletes hit (1 entry)` | −53.0% |
| `telemetry: miss, every class probed` | −45.1% |
| `close+reopen 10K (P2PKH)` | +101.8% |
| `insert P2PKH (43B)` | +25.8% |

Thirteen movements between −53% and +102%, simultaneously, from a binary that
had not changed. A gate that fires on an unchanged binary does not protect
anything; it teaches reviewers to ignore it.

## What the instrument is

Both sides are measured **in one sitting on one machine**:

- **Interleaved.** Rounds alternate A-first and B-first, drawn from a published
  seed. Drift that is shared between a round's two executions — a machine warming,
  or something else starting — moves both sides together and largely cancels in
  the ratio. It is a reduction of common-mode drift, not an immunity: drift that
  lands on one execution and not the other still shows up, which is part of why
  the threshold is measured on an A/A control rather than assumed.
- **Paired.** The statistic is `head/base` within each round, never one side's
  aggregate against the other's.
- **Measured, not chosen.** The threshold comes from an A/A control taken in the
  same window: one build measured as both sides, where the true answer is known
  to be 1. It is deliberately not derived from the run being judged, so a change
  that adds variance cannot widen the bar it has to clear.
- **Family-wise.** Twenty-one intervals at 95% produce about one finding per
  session whether or not anything changed. The correction is a simultaneous band
  over the declared family, resampling whole rounds so the correlation between
  benchmarks survives.

## The family

**Twenty-one benchmarks, judged on `elapsed` only.** They are listed by name in
`bench_analyze.EXPECTED_FAMILY`, and a test fails if the list and the suite drift
apart — a benchmark must not join or leave the family unnoticed.

**One synthetic control, judged on its own.** `synthetic control: scaled lookups`
carries a difference the harness is supposed to find. It is a positive control,
not a subject: a benchmark deliberately changed has no business setting the bound
on how often an *unchanged* one is declared.

**Six benchmarks are not gateable.** `insert 123B`, `insert P2PKH (43B)`,
`insert 89B`, `insert P2SH (41B)`, `insert-heavy workload` and
`bulk insert 10K (P2PKH)` accumulate entries across adaptive epochs and cross
capacity thresholds and rotations, so they measure a non-stationary mixture of
insert and store lifecycle rather than insert. They are reported separately and
excluded from the family; letting them into the maximum would inflate the band
for everyone else. **Pending a benchmark redesign** — tracked as R3a/R3b debt.

**Other metrics are diagnostic.** Cycles, instructions, branches, faults and
context switches are recorded and reported, and issue no verdict. The correction
therefore covers exactly the twenty-one `elapsed` intervals that can decide
something — which is the whole point of declaring the family: a correction is
only meaningful over the set of comparisons that could produce a finding, and
letting the diagnostic metrics issue verdicts too would multiply that set
sevenfold without correcting for it.

## The rules, exactly

    floor        = max(|aa_ci_low − 1|, |aa_ci_high − 1|)      per benchmark
    family floor = quantile_0.95 of max|median_AA(n) − 1| over the family
    half width   = quantile_0.95 of max|median_boot(n) − median_obs(n)| over the family

    REGRESSION       iff  median_obs − half_width > 1 + family floor
    IMPROVEMENT      iff  median_obs + half_width < 1 − family floor
    INDISTINGUISHABLE iff the interval fits inside 1 ± (multiple × floor)
    NO_EVIDENCE      otherwise, and for any metric that is missing, zero or
                     not comparable

The A/B band is centred on each benchmark's **observed** median, not on 1. Centred
on 1, a benchmark that really moved 50% would push the family maximum to 50% and
swamp everyone else's interval.

`equivalence_multiple` is 2.0 and is a parameter, not a constant: it appears in
the artifact and a test fails if it is buried.

## What it can see

Measured on each host's own A/A rounds, reproducible with `scripts/bench_power.py`:

| | mac | fedora |
|---|---|---|
| family floor | ±3.39% | ±3.50% |
| sessions declaring a control, out of 1000 simulated | **0** | **0** |
| 95% upper bound on that rate | 0.38% | 0.38% |
| power at 5% | 5.7% | 0.3% |
| power at 6% | 63.3% | 20.0% |
| power at 7% | 98.3% | 74.3% |
| power at 8% | 99.7% | 98.0% |

**The smallest effect found at least nine times in ten is 7% on mac and 8% on
fedora.** A 5% effect is invisible at sixty pairs. That is a limit worth knowing
before the instrument is used to choose a geometry: if a candidate differs by
less, sixty pairs will not see it.

## Results

Three hosts, harness `aff6fb1`, binaries from `ae99bad` (the two commits differ
only in fourteen lines of `bench_session.py`; nothing that reaches the binary
changed). One image per host, shared by both sides, so every benchmark outside
the synthetic control executes literally the same bytes.

| | zen5-desktop | fedora | mac |
|---|---|---|---|
| CPU | Ryzen 9 9950X3D, 16c/32t | Threadripper PRO 5995WX, 64c/128t | Apple arm64, 12c |
| A/A 5/5 — controls declared | 0 / 21 | 0 / 21 | 0 / 21 |
| A/B 5/20 — controls declared | 0 / 21 | 0 / 21 | 0 / 21 |
| synthetic control | **+27.1%** | **+27.0%** | **+25.9%** |
| independent clean A/A | 2 | 2 | 2 |

The planted effect is +27.3% by construction. **None of the observed families
produced a finding.** The FWER bound above comes from simulation, not from these
six sessions.

## zen5-desktop's two modes, still unexplained

Every execution of the benchmark binary on zen5-desktop comes out in one of two states
and stays in it: all twenty-seven benchmarks run about 24% slower together,
executing the same instructions at the same 4.29 GHz and taking more cycles.

Ruled out, each by measurement:

| Cause | How |
|---|---|
| CPU frequency / boost | `cycles/elapsed` identical between modes (1.000×) |
| ASLR | `setarch --addr-no-randomize`: spread 30.45% vs 30.30% |
| CCD migration | pinned to one CPU: 26 of 27 still bimodal |
| Transparent huge pages | mappings are `KernelPageSize=MMUPageSize=4 kB`, `FilePmdMapped=0`, `THPeligible=0` |
| The benchmark itself | instruction counts identical between modes (1.000×) |
| Branch prediction | branch misses *fall* in the slow mode (0.639×) |

Process-level `perf` counters cannot narrow it further: nanobench adapts its
iteration count to a time budget, so a slow run executes fewer instructions in
total and the aggregates are not comparable.

**The cause is unknown, and the instrument does not need it.** With sixty pairs
the paired family rule found the +27.3% on zen5-desktop without declaring a single
negative control. Cancelling the mode did not require explaining it.

## Running a session

Builds first, always, and finished before anything is timed:

    scripts/bench_session.py --host <name> --label calibration-aa \
      --aa-a bin/side_a --aa-b bin/side_b --base bin/side_a --head bin/side_b \
      --runtime-config-a UTXOZ_BENCH_SYNTHETIC_LOOKUPS=5 \
      --runtime-config-b UTXOZ_BENCH_SYNTHETIC_LOOKUPS=5 \
      --rounds 60 --seed <n> --toolchain ... --dependencies ...

The session waits for a quiet window on its own — normalised load, instantaneous
CPU use, run queue, competing processes by name, clocks, and time since the build,
each recorded with a status where the platform cannot provide it. It never waits
forever: a window that does not arrive marks the session `contaminated` and says
why. Nothing changes the host; affinity is per-process via `taskset` and dies
with it.

## GitHub Actions

The lane validates the harness: unit tests, a smoke build, one short interleaved
session end to end, and a check that a malformed run is refused. That session does
measure — it runs the benchmarks and analyses them — but the lane **issues no
performance verdict and gates no regression**, because a GitHub-hosted runner
cannot tell one from its own variance. Its sessions are labelled `ci-smoke`, so
nothing they produce can be mistaken for a measurement of the change.
