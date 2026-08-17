# Counting what is stored

There are two questions about a database and they are answered by different
things.

**What has this process done?** `get_statistics()` and `print_sizing_report()`.
Counters that start at zero when the database is opened: inserts performed,
probes answered, sizes handed in. A node restarted a minute ago reports almost
nothing here, and that is what a counter is, not a defect.

**What is in it?** `census()`. It opens every generation of every class, reads
every entry, and reports what is there. It costs a full pass over the data and it
says how long it took and how much it read.

Nothing in the first answers the second. Before this existed, the sizing report
looked like it did — it printed "current entries" and a histogram of value sizes
— and both were counters from this session presented under names that read like
storage.

## Scope

This release has one scope: `physical_stored`. Everything currently
materialised, every entry in every generation, **counted once per copy**.

That is not the logical state, and the difference is not hypothetical. An insert
consults only the active map of the class its payload size selects. It does not
look in older generations and it does not look in other classes. So one outpoint
can be stored in two places:

- inserted again after a rotation, it is in two generations of one class;
- inserted again with a payload of a different size, it is in **two classes**.

Both are inconsistent states, and compaction says so — it refuses a merge with
`duplicate_key` and calls the database locally inconsistent — but nothing
prevents them from arising and nothing reports them today. Worse, which copy
answers a lookup can depend on the LRU cache; that is issue #136. Reconciling them is a
second, more expensive walk, with its own algorithm and its own memory bound. It
is deliberately not in this scope, and `entries` here is a count of stored
entries rather than of distinct outpoints. The report says so in its own text, so
a pasted excerpt carries the caveat with it.

## Exact, modelled, residual

The byte figures are three different kinds of number, and the JSON groups them so
that adding across the groups is a decision the reader makes knowingly.

**Exact** — read from the entries, or asked of Boost.Interprocess and the
filesystem.

| field | what it is |
|---|---|
| `entry_payload_bytes` | full: the sum of `actual_size`. reference: entries × `sizeof(reference_value)` |
| `unused_payload_capacity` | the class's payload capacity minus what each entry uses. `not_applicable` in reference mode, where an entry is a fixed record |
| `object_padding_bytes` | what `sizeof` adds to a class beyond its named fields. Zero for all five classes in geometry 3 — measured every run rather than assumed |
| `segment_size_bytes` | `managed_mapped_file::get_size()` |
| `segment_free_bytes` | `get_free_memory()`: bytes the allocator never handed out |
| `logical_file_bytes` | the length the filesystem reports |

**Modelled** — computed from the certified map layout. They match the layout this
build is certified against; they are not observations of what the allocator did.

| field | model |
|---|---|
| `occupied_slot_bytes` | `entries × sizeof(value_type)` |
| `empty_slot_bytes` | `(bucket_count − entries) × sizeof(value_type)` |
| `estimated_group_metadata_bytes` | one 16-byte group descriptor per 15 slots — the same model `tools/sizing.cpp` uses, so the instrument and the census cannot describe one table two ways |

**Residual** — `unattributed_allocated_bytes` is
`(segment_size − segment_free) − occupied − empty − group_metadata`. It is a
subtraction, so every modelling error lands in it. It is **not** described as the
segment manager's own overhead: that would be a claim about bytes nobody counted.
When the modelled parts come to more than was allocated, the model is wrong, and
the report says so — `model_inconsistent` is set and the residual is withheld
rather than clamped to zero, which would read as a tight fit.

Nothing is double counted: the empty slots and the group metadata are **inside**
the map's allocation, while `segment_free_bytes` is what the allocator never
handed out. The identity

    occupied + empty + group_metadata + unattributed + free == segment_size

holds per generation and is checked by a test.

## Physical allocation

A version file is created at its full configured size and filled in gradually, so
on a filesystem with sparse files its length and its cost are different numbers —
and for choosing a geometry the second one is the one that is paid.

**The platforms do not measure the same thing**, so the method travels with the
figure as `physical_measurement_method`:

| method | what it returns |
|---|---|
| `posix_st_blocks` | `st_blocks × 512`: blocks the filesystem allocated. A sparse file reports less than its length, which is the intent. `st_blocks` is in 512-byte units by definition, whatever the filesystem's own block size |
| `windows_compressed_size` | `GetCompressedFileSize`, which returns the size on disk **only** where the volume supports compression or sparse files and the file is one of those — otherwise it returns the logical size. A Windows figure equal to the file size is therefore not evidence that nothing is sparse |
| `none` | not asked for |

`physical_allocated_bytes` is storage the filesystem assigned to the file. It
includes filesystem metadata and every page ever touched. It is **not** "the
bytes of the live entries".

## An entry that cannot be true

The config and the stamp certify **identity and layout**. Neither certifies that
each entry is internally consistent, and the census is where that shows.

An entry whose recorded payload length exceeds what its class can hold is refused
with `entry_corrupt`, and so is a map claiming more entries than it has buckets,
and so is any count that cannot be multiplied without overflowing. A historical
generation is also stamp-checked exactly as the ordinary open checks it: a mapped
segment is not yet a segment of *this* database, and a file renamed into a name
that is not its own would otherwise be counted as the generation it is pretending
to be. The census
fails; there is no partial report.

Taking the minimum instead — which is what the first version did — would turn an
impossible length into a plausible one: the entry would be reported as full and
nothing would be said. A silent normalisation of corruption inside the one tool
whose purpose is to be believed is the worst of the available behaviours.

The diagnostic names the class, the generation and the offending value. It does
not name the key or the payload: a message that ends up in an issue does not
carry the chain.

Arithmetic on figures read from a file is checked for the same reason. Unchecked
multiplication does not produce an obviously wrong number, it produces a
plausibly **small** one, which is exactly the failure a census must not have.

## Where the directory came from

`scope` says **what was counted** and is an enumerated value a consumer switches
on. It is always one of the defined values and never carries a caveat.

Provenance is a separate field, because it is a fact about the world that no
program here can check:

```json
"source": {
  "declared_external_snapshot": true,
  "consistency": "not_verified",
  "note": "declared by the caller; this program cannot verify how a copy was taken"
}
```

| consistency | meaning |
|---|---|
| `live_database_exclusive` | read from the database itself, under the exclusive claim, with the caller responsible for there being no concurrent mutation |
| `not_verified` | the caller declared this a copy. Whether it is a coherent moment of the original depends on how it was taken |

`declared_external_snapshot` is a **declaration**, never an observation: nothing
here can tell a copy from an original.

## Missing is not zero

Every figure that might not exist is carried as a status plus a number, and the
JSON writes the number as `null` unless it was measured:

| status | meaning |
|---|---|
| `measured` | the number is the answer |
| `not_applicable` | the question does not arise here — reference entries have no payload capacity to leave unused |
| `unavailable` | it applies, and this platform or this call could not answer it. `detail` says why |

A sum that includes a part nobody could measure is `unavailable` too. A total
that quietly skipped the missing part would be a smaller number wearing the name
of a complete one.

## Running it

    census_options options;              // defaults: blocks measured, detail included
    auto report = db.census(options);
    if (report) std::cout << to_json(*report);

or from outside the process:

    utxoz_census <directory> [options]

| option | what it does |
|---|---|
| `--json` | machine-readable output (the default) |
| `--text` | the same report for a person |
| `--mode=full` / `--mode=reference` | which storage mode the database is in; `full` by default |
| `--no-physical-blocks` | skip the per-file `stat` that asks the filesystem how many blocks it actually gave the file. The figure then reads `not_applicable` rather than zero |
| `--no-generation-detail` | per-class totals only; the per-generation list is omitted and every total is unchanged |
| `--snapshot` | declare that this directory is a copy taken from elsewhere |

It is built by `UTXOZ_BUILD_TOOLS` (on by default) and **not** by
`UTXOZ_BUILD_TESTS`: an operator who builds without tests still needs to be able
to ask what is in their database. It is installed with the library.

### Packaging

The Conan recipe exports `tools/` and carries a `with_tools` option:

| | what the package contains |
|---|---|
| `with_tools=True` (default) | the library, `include/utxoz/census.hpp` and the rest of the API, **and** `bin/utxoz_census` |
| `with_tools=False` | the library and the whole API, including `census.hpp` — and no binary. `tools/census.cpp` is not compiled at all |

Turning it off is not only about size. **The installed binary is built for the
package's target, not for the host.** Cross-compiling a consumer means the tool
would be produced for the target architecture and would not run on the machine
doing the build — Conan does not make it a build-time tool by putting it in
`bin/`. A consumer who wants the library for another target and has no use for a
binary they cannot execute turns it off:

    conan create . -o '&:with_tools=False'

`census()` itself is unaffected either way: it is library API, `census.hpp` ships
in both, and a consumer can census from their own process without the tool.

The tool opens the directory through the library — `open_for_inspection()`, which
validates the config and the stamps and takes the directory claim — and then asks
it to count. **It does not read the format itself**: a tool with its own idea of
what a segment looks like is a second implementation of the format, and the day
they disagree the untested one wins.

### Exclusivity

`census()` is `const`, and that is a statement about the object rather than about
safety. It requires:

- the database open with the **exclusive** directory claim;
- **no concurrent mutation** — it reads the active maps in place and opens every
  historical generation, and an insert running underneath it is a data race.

It opens with `open_for_inspection()`, not `open()`, because `open()` *creates* a
database where there is none: a mistyped path would leave a new empty database
behind and then be reported as holding nothing — a lie and a side effect in one.

The check belongs in the library and not in the tool. A caller outside can only
ask before opening, and between their answer and the open the config can go;
open-or-create then creates a database over the answer they were given.
`open_for_inspection()` asks **under the directory claim**, so there is no such window,
and a test arms a seam that removes the config at exactly that moment to pin it.
It also means the tool knows no file names: it reads the format through the
library or not at all.

Absence comes back as `database_not_found`. A database that *is* there and is
wrong comes back as what is wrong with it — a corrupt config is
`config_file_corrupt`, never absence.

### A class with no generations

"Creates nothing" is meant literally, and it is why this is a different door
rather than a flag on the usual one.

`open()` also creates the active container of a **class that has no
generations** — a state compaction produces by draining one completely, and an
ordinary one for a working database. For a store about to receive inserts,
giving it version zero back is right. For an instrument it is not: the file is
ten megabytes, and the census would then report a generation that this very call
had made. The measurement would be of the instrument.

So under inspection the class stays absent and is censused as **zero generations
and zero entries**. The consequence is the contract: an inspection supports
`census()` and `close()` and refuses everything else with `inspection_only` —
there is no map for the rest to work on, and making one is the thing being
avoided.

A config with **no** generations in any class is still `database_not_found`: that
is not a database with empty classes, it is a config with nothing behind it.

Nothing is created, with one documented exception: taking the claim creates
`.utxoz.lock` if it is not there, and that file is permanent by design (see
`database_lock`, where replacing it is explained as worse than keeping it). A
lock file is not a database and is never read as one, so a refused open leaves a
directory that `open_for_inspection()` still refuses.

The claim is exclusive, so a running node holds it and the tool refuses rather
than reading underneath it. Stop the node, or work on a copy. If you work on a
copy, pass `--snapshot`: whether that copy is a consistent moment of the original
depends on how it was taken, which nothing here can see, and the flag records
that in the report instead of leaving it to be remembered.

Note also that `open_for_inspection()` may write. It does not create a database, which
is the promise it makes; it does take the lock file, and it does settle an
interrupted merge in a database that is there. It is not a read-only inspection
and is not offered as one.

## Determinism

The same database censused twice produces the same JSON, byte for byte, with one
exception stated rather than left to be discovered: `duration_ms`, which measures
the machine and not the database.

## What the output never contains

Keys, payloads, txids, or anything else from the chain. The report is counts,
sizes and identities, and it is meant to be pasted into an issue. A test asserts
it: a database is filled with recognisable markers in both the keys and the
values, and neither presentation may contain them.
