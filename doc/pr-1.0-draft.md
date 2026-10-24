# UTXO-Z 1.0 — the whole change, in one description

1.0 is delivered as **four stacked pull requests**, one commit each, merged in order.
After each squash merge the next one is rebased onto the new `master`, with a backup
ref kept first. Before it is merged, it is checked to still be one commit with an
unchanged diff, as `doc/roadmap-1.0.md` §2 describes:

1. sharding and the geometry-4 format (breaking), on `master`;
2. `insert_batch`, on 1;
3. benchmarks, runners and the rung-exact testing profile, on 2;
4. documentation, on 3.

This file describes what the four add up to. The split, and why a few pieces sit where
they do, is in `doc/roadmap-1.0.md` §2.

**Status, updated when this documentation pull request was rebased onto `master`
(`a220833`):** the first three are merged - #145 (`ccf31b2`), #146 (`568198e`) and
#149 (`a220833`); this one, #150, is the fourth. 1.0 is **not released**: version, tag
and publication are not authorized, and the release gate is `doc/roadmap-1.0.md` §9.
The rest of this file was written before those merges, and its pending items describe
that moment.

Base at the time (the historical scope base, before the stack): `origin/master`
(`b144903`). Today's base is `a220833`, above. Status then: **READY-FOR-PR, not READY-RELEASE.** Implemented and validated locally (Linux x86_64, GCC 16) under the
acceptance criterion of `doc/roadmap-1.0.md` §7.0. Codex's independent review is in
progress and has not approved yet. GitHub CI (Linux, macOS, Windows, the Boost probe)
has not run. The release gate is
`doc/roadmap-1.0.md` §9.

---

**Title:** `feat!: shard the 48-byte class eight ways, fix the format at geometry 4, and insert in batches`

## What this does

- **Shards the 48-byte class into eight containers, fixed.**
  - A key's shard is `shard_of(key)`. It is specified byte by byte and pinned by
    golden vectors.
  - Every shard has its own files, map, catalogue and history, and rotates on its own.
  - Every other class keeps a single container.
- **Fixes the production format at geometry 4.**
  - Geometry 4 is geometry 3 with the 48-byte class split eight ways: 1 966 079
    buckets in 167.5 MiB per shard. The other classes are unchanged.
  - Every earlier database is refused with a specific error. There is no migration:
    rebuild from the chain.
- **Adds `full_db::insert_batch`.**
  - One prepared phase of inserts, applied by W workers with one writer per slot.
  - It returns a per-request outcome.
  - Without a latch, the result is the same as inserting one by one.
- **Carries the fixes and features merged to master since 0.10.0**, none of them
  released yet: #127, #128, #130, #132, #133, #134, #137, #138, #139, #140, #141 and
  #142.

## Breaking changes

- **On-disk format.** 1.0 writes geometry 4 and refuses everything else:

  | Database | Result |
  |---|---|
  | 0.10.0 or earlier (no recorded identity) | `migration_required` |
  | master builds between 0.10.0 and 1.0 (geometries 1–3) | `geometry_mismatch` |
  | experimental builds (`0x8000_00xx`) | `geometry_mismatch` |

  Rebuild from the chain.
- **Build option removed.** `UTXOZ_SHARD_COUNT` is gone. Setting it is a configure
  error, and defining it is a compile error in the public header.
  - The shard count is the constant `utxoz::shard_count == 8`. It is not a Conan
    option, and it does not affect `package_id`.
  - Other counts exist only through `UTXOZ_EXPERIMENTAL_SHARD_COUNT` (1, 2, 4, 16, 32).
    Such a build is marked `utxoz::experimental_build`, writes an experimental geometry
    id, and installs nothing.
- **API.** `insert_batch` and its types are new; `insert()` is unchanged. The
  `feat!` changes already on master (#128, #132, #134) are part of this release.

## `insert_batch`, the contract in short

The full contract is in `include/utxoz/database.hpp`. The summary for integrators is
`doc/insert-batch-kth-handoff.md`.

- **One mutation to the caller.** The call is synchronous and returns when no task is
  running.
- **The executor must be synchronous.** It has to join every thread it handed `task`
  to before returning, also when it throws.
  - A *copy* of `task` called late runs nothing: that is a safety net, not support for
    asynchronous executors.
  - Tasks nobody ran are run on the calling thread.
- **Not atomic.** Each request reports `inserted`, `duplicate`, `failed` or
  `not_applied`, and only `not_applied` may be resent as is.
  - An integrity latch stops the other workers before their next request.
  - Requests already in flight finish and report how they ended.
- **Memory.** Everything the batch needs for its bookkeeping is allocated before the
  first request; a failure there throws `std::bad_alloc` with nothing changed. After
  that it does not throw, and counting a stored entry allocates nothing.
- **No sync.** It does not sync; a later `sync()` covers every slot it rotated.
- **Slots.** The same (key, class) always reaches the same slot. There is no global
  uniqueness check and no BIP30.
- **Workers.** W is configurable and limited to the slots the batch uses; W=8 at N=8
  is valid.
- **Unchanged:** the global file cache, `resolve_mutex_`, historical deletes run
  serially, and compaction runs one shard at a time. The external threading contract
  is the same.

## Performance (measured, with limits)

All measurements are on one machine, on synthetic 6 M-entry images, with the
rung-exact *testing* profile. This is not a node sync, and nothing here claims one.

- **Single thread, N=8 against N=1.**
  - Historical reads: +51 % (`hist_hit`) and +69 % (`hist_miss`).
  - Insertion: +11.9 %.
  - Compaction: +135 %.
  - All of these are nominal regressions (TAREAs 06 and 07).
- **`insert_batch` at N=8** (TAREA 10, 3 repetitions, exploratory): median 380.560 ms
  at W1 and 121.503 ms at W8, about 3.13×. N=1 at W1: 264.253 ms.
  - This shows that the workers participated, not that they overlapped.
  - Threads are created per call, the batch size is fixed at 100 k, and everything ran
    in one L3.
- **No equivalence or non-regression claim is made.** The ±2 % thresholds of the
  design were not met and were not used to choose N; eight is a product decision.

## Tests

- **Before the closing change:** the full suite on `build-b9-8` - the code of `7f21dfc`
  with the experimental geometry id `0x80000012`, not a production build - gave 412
  cases: 393 passed, 0 failed and 19 skipped (`doc/roadmap-1.0.md` §7.1). The 19 skips
  were 15 geometry-3 fixtures (12 compatibility, 3 generator), 2 for the refusal of
  geometries 1 and 2, and 2 rung-exact (profile not compiled). The production run is
  the one after the closing change, below.
- **`insert_batch`:** `[batch]` and the allocation binary pass under ASan+UBSan and
  TSan, and seeded mutations of the executor gate and the preallocation are all
  caught.
- **After the closing change**, locally (`doc/roadmap-1.0.md` §7.2, with commands
  and logs):

  A run is accepted at exit 0, or at exit 4 only when every case behind the 4 is a
  pure skip (consistent tally, every skip listed and justified), with zero assertion
  failures, timeouts and infrastructure errors. The real exit codes are reported, not
  rounded to 0.

  | Build | Selection | Result |
  |---|---|---|
  | production, statistics `basic` | full suite, 414 cases | exit 4 (skips only): 395 passed, 0 failed; 19 skipped (17 need statistics `lookup`, 2 rung-exact profile) |
  | ASan+UBSan, statistics `lookup` | risk tags, 218 cases, plus the allocation binary | exit 4 (skips only): 216 passed, 0 failed, 2 rung-exact skips, no sanitizer report; allocation binary exit 0. The 17 statistics cases ran and passed here |
  | TSan | `[batch]`, `[concurrency]`, `[ownership]`, `[executor]`, plus the allocation binary | exit 0: 39/39, no report; a seeded use-after-free is caught (positive control). This local targeted run is the TSan gate; there is no TSan CI job |
  | experimental N=1 | targeted tags | exit 4 (skips only): 131 passed, 0 failed, 22 justified skips; installs 0 files |

  Other checks:
  - The compatibility suite reads the geometry-4 set (44/44, no skips) and refuses
    the geometry-3 set untouched.
  - `conan create` plus `test_package`: the consumer sees 8 shards and geometry 4
    with no defines, and runs `insert_batch` with 8 workers.
  - `utxoz_census` and `utxoz_verify_unique` report geometry 4 and 8 shards on a
    geometry-4 database, and refuse a geometry-3 one with `geometry_mismatch`.

## Not in this PR

- KTH integration. It comes after, with its own end-to-end validation, and this PR
  does not change KTH.
- Cache pinning, the in-memory active generation, parallel historical deletes or
  resolves, and any new external concurrency.

## Checklist

- [x] Closing change committed (`9bdeb73`, `f8c1020`)
- [x] Geometry-4 compatibility fixtures promoted (authorized), reproducible in logical digest
- [x] Production suite: 0 failures, every skip listed and justified (local)
- [x] Experimental N=1 build: compiles, installs nothing, targeted suite green (local)
- [x] ASan+UBSan and TSan targeted runs (local)
- [x] `conan create` and `test_package` (local cache only)
- [x] `utxoz_census` and `utxoz_verify_unique` on a geometry-4 database
- [x] README: on-disk format (geometries 1–4) and `insert_batch`
- [x] Diff free of builds, evidence and scratch
- [ ] Codex independent review approved (in progress: 58 independent compat/format/generator/batch tests and the CMake negatives reported green; no approval yet)
- [ ] GitHub CI green on Linux, macOS, Windows, sanitizers and statistics jobs
- [ ] Push, PR, version and tag authorized (not yet)
- [ ] Codex review approved
- [ ] Release version and history shape decided by Fernando
