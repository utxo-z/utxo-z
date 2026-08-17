# How much counting a build carries

One knob, cumulative, chosen by name. The value is one of `off`, `basic` or
`lookup` — written out below rather than as `off|basic|lookup`, which is a
notation and not something to type:

    cmake -DUTXOZ_STATISTICS_LEVEL=lookup ...
    conan install . -o "utxoz/*:statistics_level=lookup" ...

Both default to `basic`, and both refuse a value they do not know.

| level | what is compiled |
|---|---|
| `off` | nothing. Every counter, every recording call and every accumulator is absent, and the reporting types return zeros |
| `basic` | what this process did: inserts, deletes, probes, resolutions, lifetimes, fragmentation. **The default** |
| `lookup` | `basic`, plus the per-class read-path telemetry: which class answered, how many generation files a lookup cost, how far back the answer was |

Cumulative, so the code asks `#if UTXOZ_STATISTICS_LEVEL >= 1` or `>= 2` and there
is one authority rather than a flag per counter set. An unknown value is refused
at configure time by both CMake and Conan — a typo must not quietly produce the
default, because whoever asked for `lookups` would then read counters that were
never compiled and find zeros.

## Why `lookup` is not the default

It costs about **eleven per cent of a `find()` that hits**, measured, on the path
a node spends most of its time in. It is meant for measurement builds run against
a real database, not for every build.

It also widens its own counters, and that is the second reason it is a level
rather than a flag: the counters are sharded across 64 cache-line-padded slots,
so a wider field set is a bigger object.

### The sharded read-path counters: a subtotal

| level | probe_stats | resolution_stats | lookup_stats ×5 | subtotal |
|---|---|---|---|---|
| `off` | 1 B | 1 B | 5 B | 7 bytes |
| `basic` | 8 KiB | 8 KiB | 5 B | **16 KiB** |
| `lookup` | 8 KiB | 8 KiB | 80 KiB | **96 KiB** |

`sizeof` on each type at each level, added up. `lookup` is **80 KiB more than
`basic`**, and all of that increment is the five per-class instances.

**This is a subtotal of three things, not the size of all statistics and not the
size of a `database_impl`.** The same object also holds `container_stats_`,
`height_range_stats_`, `deferred_stats_`, `not_found_stats_`, `lifetime_stats_`
and `fragmentation_stats_`, several of which own maps that grow with the data —
value-size histograms, per-height ranges, age distributions. A single number for
all of it would be a number about a moment, not about a build, so none is given
here. What this table answers is the question the level actually decides: how
much fixed storage the sharded counters take.

An earlier draft called the last column a total, and an earlier draft still said
112 KiB. That figure belonged to the previous design, before `sharded_counters`
was parameterised: the wide field set was imposed on `probe_stats` and
`resolution_stats` as well, so all seven instances were 16 KiB. Removing that is
what the parameterisation was for, and the numbers above are what a build
produces today.

These figures live here and the code points at this file rather than restating
them, so that they cannot drift apart again.

At `basic` the older counters keep exactly the size they had. That is what the
level buys over a boolean: a build that never enables the telemetry does not pay
for its width.

## What each level costs, measured

Same head, same workloads, seven interleaved rounds, medians in nanoseconds:

| workload | `basic` | `lookup` | telemetry costs |
|---|---|---|---|
| active hit, first class | 35.02 | 38.80 | +10.8% |
| miss, every class probed | 17.81 | 17.83 | +0.1% |
| sweep of 256 keys / 3 generations | 13 013 | 13 298 | +2.2% |
| mixed, 9 active hits to 1 historical | 526.2 | 576.2 | +9.5% |

A miss measures +0.1%, which is inside the noise of these runs rather than a
saving — the honest reading is that it costs nothing measurable here, not that it
provably costs nothing. There is no counter left on that path: what used to be
counted there is derived instead. See below.

## Counters that are not counted

Two figures that used to cost an atomic increment each are arithmetic instead.

**`probes` = `answered` + `deferred`.** Every probe ends one way or the other, so
counting the total as well was a third increment on the hot path for a number
addition already had. Removing it made `basic` itself **9% faster** on a hit than
before.

**`active_maps_probed[i]`.** `find()` visits the classes in order and stops at
the first that answers, so class `i` is asked exactly when the answer came from
`i` or later — or from nowhere:

    active_maps_probed[i] = (lookups answered by class i or after) + deferred

Counting it directly, one increment per class per lookup, measured **+160%** on a
lookup that misses everywhere. Counting only where the search stopped, one
increment per lookup, was still an atomic for a number this arithmetic already
had.

## What the output says

`statistics_level` — `"off"`, `"basic"` or `"lookup"` — and not a boolean.
"Statistics are on" was never the same claim as "these counters were collected":
at `basic` the first is true and the second is false, and a reader given only the
first would take a page of zeros for a database that answered nothing.

At `basic` the per-class list is **empty** rather than five rows of zeros, for the
same reason.
