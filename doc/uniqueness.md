# Is any outpoint stored more than once?

A correct UTXO set holds each outpoint **at most once, across every class and
every generation**. Nothing in the write path enforces that, so whether a given
database satisfies it is a question that has to be asked of the files.

`verify_unique_outpoints()` asks it. It is a separate operation and not a mode of
`census()`: a census describes what is there, and this decides whether what is
there is allowed to be.

## A verdict, not an error

A database with duplicates is not a database this can fail to read. The walk
finishes, every figure it reports is exact, and the answer is `unique == false`.

There is deliberately no `error_code` for a duplicate, and `entry_corrupt` is not
reused for one. Returning an error would conflate two different things — *the
verification could not be completed* and *the verification completed and the
answer is no* — and only the first is about the instrument.

The tool encodes the distinction in its exit status, which is why it is a
separate binary from `utxoz_census`:

| status | meaning |
|---|---|
| `0` | no outpoint is stored more than once |
| `2` | some outpoint is stored more than once |
| `1` | the verification could not be completed |

A script that treats every non-zero status alike would take a database it could
not open for a database it had condemned.

## It does not choose a winner

For a key stored more than once with copies that disagree there is no "the value
of this key": the store has no precedence policy across classes, and in history
which copy answers a lookup can depend on the file cache (issue #136). Naming a
winner would be inventing that policy inside an instrument.

So this reports **how many copies exist and where they are**, and stops. Repair
is a rebuild from the blocks into a new directory, verified and then swapped in —
not a choice made here.

## Outpoints are withheld unless asked for

An outpoint identifies a transaction output on a public chain. By default a
finding says how many copies a key has and where each one is, and not which key
it is, so that a report can be pasted into an issue without a second thought.

`include_outpoints` puts them in, for whoever is going to look at the entries.
There is no substitute form: a truncated hash would be neither private — it is
reversible against the chain — nor useful, since it cannot be fed back to
`find()`.

The document states which it is (`"outpoints_included": false`) rather than
leaving it to be inferred from an absence, because a report with no findings and
a report with the outpoints withheld would otherwise look the same.

## The sample is bounded

`keys_with_multiple_copies` is the count. The findings are a sample of at most
`max_findings` keys, each locating at most `max_locations_per_finding` copies, so
that a thoroughly broken database produces a report of bounded size rather than
one proportional to the breakage. What was left out is reported
(`findings_omitted` for the keys, `locations_omitted` for the copies of one key),
so a truncated sample never reads as a complete one.

| option | default |
|---|---|
| `include_outpoints` | `false` |
| `max_findings` | `16` |
| `max_locations_per_finding` | `16` |

The sample is collected in two flat vectors metered by the same allocator as the
records, each reserved exactly once so that nothing ever grows and no two blocks
are ever alive at the same moment. A sample the budget cannot hold is refused
before the work. It is converted into the public `duplicate_finding`s after the
walk, when the records are no longer held.

**The returned report is outside the budget, and that is stated rather than
implied.** `findings` are public types with the global allocator, and `reserve`
promises *at least* what it was asked for, so claiming the report is inside a
metered ceiling would be claiming that `reserve(n)` allocates exactly n — which
the standard does not promise and which no test can establish for every
implementation. What is bounded instead is the *shape*: at most `max_findings`
findings of at most `max_locations_per_finding` locations each, reported as
`report_bytes_estimate`, and never growing with the database.

Doing it the obvious way was wrong and is worth stating: filling the public
vectors directly and declaring their final size to the budget accounts for
neither geometric growth, nor a `capacity()` larger than what was asked for, nor
the moment during a reallocation when the old block and the new one are both
alive. With `max_findings` at 17 a plain vector ends up holding 32 having briefly
held 16 and 32 together, against 17 accounted for.

## What it costs

Counting distinct keys exactly needs every copy of a key to be considered
together, and the set of all keys is a set nobody sized. Nothing is written —
there is no temporary directory, no spill file and no residue on any failure
path, because nothing is created. Instead the data is walked once per partition
group:

1. a planning pass counts entries per 12-bit prefix — counters only;
2. prefixes are grouped so each group's records fit the budget, from the counts
   just observed rather than an estimate;
3. each group is walked, collected, sorted by key, grouped;
4. a prefix that cannot fit is refused, with the budget it would have needed.

The prefix width is fixed and there is no subdivision: every copy of one key
hashes alike, so a key with more copies than fit can never be split.

**The cost is re-reading, and it is reported rather than implied.** Measured on a
500 005-entry database (500 000 distinct outpoints, five of them stored twice),
1.3 GB on disk, warm page cache:

| budget | passes | entries examined | time | peak the walk claims |
|---|---|---|---|---|
| 256 MiB (default) | 1 planning + 1 data | 1 000 010 | 244 ms | 56 132 401 |
| 16 MB | 1 planning + 3 data | 2 000 020 | 303 ms | 15 996 496 |
| 4 MB | 1 planning + 13 data | 7 000 070 | 806 ms | 3 999 416 |

All three give the same verdict and the same counts, and all three end inside the
budget they were given. The cost of a small budget is linear in the number of
partitions, and the report carries `duration_per_pass_ms` so that where the time
went is visible rather than inferred from the total. With a cold page cache each pass is the
database's worth of I/O, which is why the pass count is in the report.

### The budget is enforced, not described

An earlier version of this computed what it expected to hold and reported it.
That is a description: a container allocated beside the records would not have
appeared in the figure, so the report could have said the ceiling was honoured
while the process held considerably more.

Now every allocation the walk makes goes through a meter, and it is the allocator
that asks. A request past the ceiling is refused **before the system is asked for
the memory** — which on a real database is the difference between a diagnostic
and a `bad_alloc`. `estimated_peak` is the most the meter ever had outstanding,
observed rather than predicted. A release that does not match an acquisition
leaves the meter unbalanced, and an unbalanced meter authorises nothing further:
once what is held is unknown, handing out more against that figure is the overrun
this exists to prevent. The books are also checked **after** every metered
container has been destroyed, because a mismatch in the last deallocation of all
has no acquisition following it to be refused — and a meter that lost track did
not honour a ceiling, so there is no verdict to give on that basis.

What the ceiling covers, as a list rather than a claim, and pinned by a test:

| inside the budget | |
|---|---|
| sort allowance | 1 048 576 — the only figure left, because a sort's own stack has no allocator to route through |
| planning counters | metered: 4096 × 8 |
| the group list | metered, reserved once to its exact maximum of one group per prefix (96 KiB) |
| the collected sample | metered, reserved once and never grown |
| the records | metered, per pass |
| headroom | a tenth of the budget, **held back**: the meter's ceiling is the budget without it, so the walk cannot reach it |

| outside the budget | |
|---|---|
| the memory-mapped segments | the operating system's pages, mapped and reclaimed on its terms |
| the returned `uniqueness_report` | its `findings`, their locations, and `duration_per_pass_ms` — public types with the global allocator, estimated as `report_bytes_estimate` |

Everything in the first table but the sort allowance is a real allocation the
meter saw. The counters and the group list used to be plain vectors covered by a
hand-computed allowance of 192 KiB; the worst case they can actually reach — a
reallocation holding two blocks at once — came to **196 608 bytes against an
allowance of 196 608**. It held, by nothing but one allocator's growth factor, and
the `static_assert` guarding it compared final sizes rather than transients.
Metering them removes the arithmetic instead of correcting it.

`report_bytes_estimate` is an estimate computed from the requested element counts,
**not an upper bound**: `reserve` promises at least what it was asked for, so
excess capacity and transients belong to the standard library. What is bounded is
the shape — at most `max_findings` findings of at most `max_locations_per_finding`
locations each — which is why the figure does not grow with the database.

The duplicate group handed to the caller is a view over storage the meter already
holds, and it can only be built from such storage — a copy would have to be
metered too, which is what stops the old bug returning as somebody's convenience.

This is not a sandbox: code that calls the global allocator directly is outside
any in-process budget. What changed is which one the code invites.

It does not govern the **mapped segments**. Resident memory for the three runs
above was 1.38 GB, 1.37 GB and 1.35 GB: dominated by the database's own pages,
which the operating system maps and reclaims and which no budget here can bound.
Read `estimated_peak` as what this walk asked the allocator for, and not as what
`ps` will show.

## The 32-bit guard, and where it is checked

The counts are `uint64_t` and `reserve` takes a `size_t`, so on a 32-bit target any
count of 2^32 or more narrows on the way in — 2^32 itself included, since the
largest a 32-bit `size_t` holds is 2^32 − 1. The guard against that is a predicate
taking the width as a parameter — `sample_fits_addressable` — rather than reading
`sizeof(size_t)` directly, because the only platform where it can fire is the one
where no test runs: the wasm32 job builds the library and runs no `ctest`, and this
suite could not run there anyway.

Production calls a wrapper that takes **no width at all**, so an ordinary call
site cannot pass the wrong one. The parameterised primitive stays for the tests,
which drive it from Linux at 16, 32 and 64 bits, and both are pinned by
`static_assert` — including, under `#if SIZE_MAX < UINT64_MAX`, assertions about
the wrapper itself, which only a 32-bit build evaluates.

## The contract

The same as `census()`: the exclusive directory claim and no concurrent mutation.
`const` is a statement about the object, not about safety.
`open_for_inspection()` satisfies the contract by construction, and is what the
tool uses.
