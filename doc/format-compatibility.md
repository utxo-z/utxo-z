# Keeping a database readable

The store keeps its hash tables inside memory-mapped files, so the internal
layout of `boost::unordered_flat_map` is part of the file format. Boost promises
nothing about that layout, and Boost.Interprocess resolves a named object by name
alone — it does not check the type — so a build whose idea of the layout differed
from the one that wrote the files would reinterpret them rather than refuse them.

Four numbers stand in the way, and this is how they move.

| | changes when |
|---|---|
| `geometry_id` | our container sizing changes |
| `map_layout_epoch` | the map implementation we are certified against changes what it persists |
| `hash_epoch` | the effective hash changes, so keys move |
| `platform_abi_id` | derived from the target; never edited by hand — it covers the data ABI *and* which interprocess mutex the build compiles |

`BOOST_VERSION` is recorded in every config and **the barrier never consults it**.
Several Boost releases can share one `map_layout_epoch`, and treating a version
bump as an incompatibility would invalidate every database for nothing.

That is a statement about the barrier, not about the field. The writer comparison
below does read it — to normalise it away, together with the checksum that covers
it — precisely because it is the one difference expected between two builds that
are otherwise the same.

## The evidence

`tests/fixtures/epoch1-lp64/` holds databases written once, under epoch 1, on a
64-bit little-endian target with Boost 1.91. `manifest.json` records what each was
written under, the SHA-256 of every file, a logical digest of the whole database
and one per segment, and the entry count of each generation.

`tests/test_format_compatibility.cpp` opens copies of them and checks both halves:
that they open at all — config and stamps accepted before anything reaches a map —
and that every entry comes back, canonically ordered and digested. Neither half is
sufficient. `open()` succeeding says nothing about the values, and a matching
entry count stays green while every one of them is wrong.

### What a green run does not prove

A fixture written under epoch 1 proves that the **reader** still reads epoch 1. It
does not prove that the **writer** still produces the same bytes for every state a
database can reach, because a fixture contains only the states it contains. That
is why the per-segment digests exist, why the generator is a separate tool whose
candidates a person compares, and why the informative Boost probe reports what it
found rather than a colour.

### The files are byte-reproducible, and were not

Two runs of the same binary, given the same inputs and the same database
identity, now produce identical files — all eighteen of them. That is what makes
physical equality usable as a criterion at all, and it had to be earned.

It used to be false, and the reason was one line. A `utxo_value` is stored inline
in the mapping and stored whole: the container's size class, not the size of the
output in it. `set_data()` wrote `actual_size` bytes and left the rest alone, so
everything past the payload was whatever that storage last held — for a value
built on the stack, the caller's stack — and it went to disk. Up to 6 KiB per
entry in the largest container. Nothing reads it back, which is why nothing
failed.

It surfaced as non-determinism: two runs differed in 171990 bytes, in runs of six
at a stride of eight. That shape is what identified it — pointers, whose upper
bytes move with ASLR, left behind in stack slots. A first reading blamed process
addresses in a rotated generation's segment header; that was wrong, and the same
defect explains those eleven bytes too. There are no absolute pointers in the
persisted image: Boost.Interprocess stores `offset_ptr`, and the ASLR correlation
came from residue, not from structure.

`set_data()` now defines everything past the payload, in two regions: the unused
tail of `data`, and the padding `sizeof` adds where a size class is not a multiple
of the alignment. No class in the current geometry is such a size — that is what
geometry 2 fixed — but the guarantee belongs to the object rather than to whichever
classes are listed, so the branch stays and a non-geometry size is tested against
it. `tests/test_value_initialisation.cpp` pins both, through the object
representation rather than through the members, and checks what actually reaches
the file rather than only what the object holds.

## Changing something

### Update a golden without moving any epoch

When the fixtures are regenerated for a reason that is not a format change — a new
fixture shape, more coverage — the new files are compared against the old by a
person, not by a test. Regenerating always produces different bytes anyway: every
database draws a fresh `database_id`.

Never update a checksum or a digest to make CI green. A digest that changed is a
statement that the bytes changed, and the only acceptable response is to find out
why.

### Add a fixture

Whenever a state the existing fixtures do not contain becomes worth certifying: a
new storage mode, a new container, a generation shape nothing else exercises, or a
second ABI. Fixtures are added, not replaced — an old one keeps proving what it
proved.

    cmake --build <build> --target utxoz_make_format_fixtures
    <build>/utxoz_make_format_fixtures /tmp/candidates

The generator refuses to write into a directory that already holds a manifest.
Promotion is a person copying files after reading the diff.

### Bump `geometry_id`

When `container_sizes` changes, a container is added or removed, or a value that
used to live in one container now lives in another. A `static_assert` in
`format_identity.hpp` fails the build if the geometry moves without this number,
so forgetting is not one of the options.

Existing databases are refused with `geometry_mismatch` and have to be rebuilt.
There is no migrator and there deliberately will not be one: a value's container
is decided by its size, so migrating means reading every entry and writing it
somewhere else — which is rebuilding, with the added risk of a tool that thinks it
is doing something safer.

**Geometry 2** is the second class becoming 96 rather than 94. The object always
occupied 96, because 94 rounds up to the alignment, so the two bytes it gained
were padding no caller could reach: `sizeof(utxo_value<94>)` and
`sizeof(utxo_value<96>)` are both 96, and the stored pair is 132 either way.
Naming the class 96 turns those bytes into payload — capacity goes from 89 to 91 —
at no cost in bytes per slot, entries per file or anything else the capacity
profile measures.

It is still a format change, because it moves where a value lives: a 90- or
91-byte output used to go to container 2 and now fits in container 1. A database
written under geometry 1 is refused with `geometry_mismatch` and has to be rebuilt
from the chain.

### Bump `map_layout_epoch`

Only on evidence, and never automatically because Boost published a release. The
evidence is the fixtures: a build against the new implementation that cannot open
them, or opens them and returns something else, is a build that needs a new epoch.

The informative probe (`ci/check_boost_latest.sh`) produces exactly that evidence
for a Boost the project does not pin. Its verdicts:

| verdict | what it means |
|---|---|
| `NO_EVIDENCE` | there were no fixtures to test against, or the suite ran fewer cases than it claims to cover — nothing was proven |
| `DOES_NOT_BUILD` | nothing is known about the format yet |
| `BUILDS_ONLY` | it compiled and could not be run — **not** compatibility |
| `REFUSED_BY_IDENTITY` | the barrier declined; expected if an epoch was moved |
| `LOGICALLY_COMPATIBLE` | the fixtures opened and every entry matched |
| `LOGICAL_MISMATCH` | they opened and the contents are wrong |

A `LOGICALLY_COMPATIBLE` verdict is what justifies certifying a new Boost under
the *same* epoch. Record which Boost was certified; the probe pins its source by
version and checksum, or by commit, so a run always names the bytes it tested.

The probe reports a **second, separate** classification for the writer, because
"it reads what we wrote" and "it writes what we write" fail independently and one
word covering both always overstates one of them:

| classification | what it means |
|---|---|
| `PHYSICALLY_IDENTICAL` | every file matches what the pinned Boost writes from the same inputs |
| `PHYSICALLY_DIFFERENT` | something does not, and each one is named |
| `NO_EVIDENCE` | the baseline could not be built or run, or the pinned build did not reproduce its own output — nothing is claimed |

The comparison enumerates **both** trees, so a file only the candidate writes is a
difference rather than something never visited, and it normalises the config's
`boost_version` — diagnostic, never read back — together with the checksum that
covers it, rather than skipping the config. The checksum has to go with the field:
a config recording a different Boost differs twice, and normalising only the field
leaves the checksum looking like a finding. Nothing is lost, because the checksum
is a function of the bytes before it and those are compared directly. It also
compares the pinned build against a
second run of itself first: if that does not agree, equality is not the criterion
on that machine and no difference found afterwards would mean anything.
`ci/check_boost_latest.sh --self-test` runs the comparison over six trees whose
answers are known.

### Bump `hash_epoch`

When the effective hash changes — ours, Boost's mixing, or the avalanching trait
that decides whether the mixing applies at all. `format_identity.hpp` asserts the
trait, and `test_format_barrier.cpp` pins the effective hash as literals per ABI,
so a change cannot land quietly.

Every database written under the old epoch has to be rebuilt: nothing about the
files is wrong, but every key is somewhere the new build would not look.

### Refuse a platform

`platform_abi_id` is derived, not chosen, so a platform is never refused by
editing it. It folds together two things: the **data ABI** — endianness and the
widths of `size_t`, a pointer and an `offset_ptr` — and the shape of the
segment's own machinery, which is persisted right beside the data.

The second was added on evidence. A managed segment keeps its allocator's mutex,
and the segment manager that owns it, **inside the mapped file**, and Boost
compiles a different mutex per platform: POSIX process-shared mutexes where they
work, the Windows API where that is the choice, a spinlock emulation otherwise —
macOS takes the third road because Boost marks Apple's process-shared mutexes as
broken, in `detail/workaround.hpp`, with the reason written out.

Linux x86_64 and macOS arm64 agree on all four numbers of the data ABI. So the
barrier accepted the fixtures on macOS, the segment mapped cleanly, and the first
named-object lookup never returned: a lock taken on another platform's bytes. No
diagnostic, no failure, no end — eight cases held until a timeout killed them.

The family alone would not have been enough, and is not what is compared. What
goes into the fold is measurable: the size and alignment of the mutex and of the
segment manager, plus the platform and C runtime. Linux/glibc reports a 40-byte
mutex; a spinlock build reports four. Two POSIX targets are not taken for each
other merely because their mutexes happen to agree.

**Cross-platform compatibility exists only where it has been certified.** A build
that lands on the same branch as another is not thereby compatible with it: the
identity is what decides, and a fixture proves what it proves on the platform it
was written for. There are fixtures for exactly one identity today, and a build
whose identity differs is required to refuse them rather than to open them.

Note what this means for `lp64_le_abi`: it names the *data* ABI, and the hash
vectors are pinned against that. What a key hashes to does not depend on which
mutex sits beside it.

`platform_abi_encoding` exists so that changing the ingredient list is itself a
change of identity: an id computed from a different set of numbers cannot collide
with an old one into looking compatible.

There are no wasm32 fixtures. There is therefore **no evidence of a physical
round trip on wasm32**, and the compatibility suite says so rather than passing
quietly: on an ABI the fixtures were not written on, it checks that they are
refused. Adding wasm32 fixtures is what would change that.

### Require a rebuild

Any of the four epochs moving requires one. There is no in-place migration and
there is deliberately no migrator: rewriting a config over data whose layout was
never recorded asserts the one thing that cannot be checked.

Databases from before the format recorded its own identity — anything written by
v0.10.0 or earlier — are refused with `migration_required` and nothing about them
is read or altered.
