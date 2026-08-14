# How large a segment is, and how many buckets it starts with

> **This is an incompatible change.** A database written before it is refused with
> `geometry_mismatch` and has to be rebuilt from the chain. There is no migrator
> and there will not be one. Nothing about an earlier experimental database will
> open.

One decision, in one place. A file size without the capacity it was chosen for is
a number nobody can check; a capacity without the file size it needs is a rehash
waiting for the entries to arrive. `src/detail/capacity_policy.hpp` holds both.

## What `geometry_id` identifies

From 3 onwards, the **whole storage geometry**: the size classes, which payload
goes to which of them, the capacity a new segment's map is built with, and the
size of the segment. They are one decision — a capacity without the file size it
needs is a rehash waiting for the entries — so they are identified together.

Neither the config nor the stamp records a capacity, so before this there was
nothing a reader could check: a binary with a new policy would have opened an old
database and operated it with thresholds meant for a different file. The geometry
id carries it now.

`map_layout_epoch` and `hash_epoch` do not move: the map's layout is unchanged and
no key changes bucket.

## The invariant

**A generation's bucket count never changes.** When a container fills up the store
makes a new generation and compaction deals with the cost later — that is the
whole design. A map that grew instead is a defect even where there was room for
it: the file it wrote is not the file the policy describes, and the growth copied
every entry to get there.

Two defences, in order:

1. **The guard rotates first.** `can_insert_safely()` compares the map's size
   against `133/160` of *its own* bucket count. `unordered_flat_map` grows at
   `0.875`; the store rotates at `0.875 × 0.95 = 133/160` exactly, five per cent
   below.
2. **The file cannot hold the growth.** A segment sized to its map has no room for
   the step above, so even a bypassed guard fails closed rather than quietly
   rewriting the file.

`bad_alloc` is not part of either. It is what would happen if both failed, and it
is not expected in normal operation.

### And it is watched at run time

`rehashes_observed` counts generations whose bucket count moved. It is compiled in
**every** build, not behind the statistics switch, because a rehash is not a
statistic — an operator running without statistics is exactly who needs to know it
did not happen. It costs one comparison against a value already in cache.

The check runs after the insert, so it cannot become an error the caller acts on:
a retry would write the entry twice. The entry is there; what is wrong is that the
map grew. It is logged and counted, and a build with assertions stops, because
continuing to write into a store that has begun rehashing under itself buries the
evidence.

## The two profiles

They answer different questions and share no formula.

|  | production | testing |
|---|---|---|
| fixed | the capacity | the file size, at 10 MiB |
| derived | the file size | the capacity that fits |
| margin | 5%, rounded up to a whole MiB | none |

The margin belongs to the first. Applying it to the second would drop container 0
from 122879 buckets to 61439 — its floor at 10 MiB leaves 0.27% of slack — and
every compatibility fixture would change for a reason that has nothing to do with
the format.

## Container 0

Measured by `tools/utxoz_sizing` on all three platforms, built the way production
builds: segment, stamp, `map_object_name`, real allocator.

|  | before | now |
|---|---|---|
| bucket_count | 7 864 319 | **15 728 639** |
| entries before rotating | 6 537 215 | **13 074 431** |
| file | 2048 MiB | **1340 MiB** |
| a growth fits in the file | **yes** | no |

```text
floor    Linux   1 337 987 188   ← the largest of the three, and the base
         macOS   1 337 987 140
         Windows 1 337 987 132
× 1.05           1 404 886 548
rounded up       1 405 091 840 = 1340 MiB
```

The three differ by a fixed 48 and 56 bytes — `sizeof(segment_manager)`, which is
200 on glibc, 152 on Apple and 144 on Windows — constant across all twelve
measurements and absorbed entirely by the rounding. **One size for all three**;
that is a statement about the rule, not about the files. A database written on
Linux is still refused on macOS by `platform_abi_id`, as it should be.

## The other five

They keep exactly the file size and capacity they had. The policy records them
rather than discovering them, and marks them `certified: false` — the size is what
it always was, and the measured recommendation may be larger.

| class | file now | buckets | rotates at | measured recommendation | a growth fits today |
|---|---|---|---|---|---|
| 96 | 500 MiB | 3 932 159 | 3 268 607 | 524 MiB | no |
| 128 | 1024 MiB | 3 932 159 | 3 268 607 | 650 MiB | no |
| 256 | 50 MiB | 122 879 | 102 143 | 37 MiB | no |
| 10240 | 10 MiB | 959 | 797 | 10 MiB | no |
| reference | 4096 MiB | 7 864 319 | 6 537 215 | 387 MiB | **yes** |

Two of these are worth knowing about:

**Class 96** has a map of 499 MiB inside a 500 MiB file. It cannot grow, so it is
not unsafe, but it has about a megabyte of margin where the measurement recommends
524. It is the one entry whose current size is *below* its own recommendation.

**Reference** has a map of 368 MiB inside a 4 GiB file, and a growth **does** fit
there — the same live defect container 0 had, measured the same way. It is left
alone here deliberately: it is a decision of its own.

## What is still open

The table above is measurement, not a decision. Choosing the rest needs data this
project does not have yet: a live histogram of output sizes, survival, which tier
answers `find()`, latency, and what compaction costs. Until then a class keeps
what it has, written down.
