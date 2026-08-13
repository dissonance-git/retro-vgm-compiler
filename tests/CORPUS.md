# VGM Tooling test corpus

This document governs user-supplied real-music fixtures used to pressure-test VGM Tooling beyond synthetic unit cases.

## Purpose

The corpus exists to test the full path from preserved game-music objects into execution, synthesis, musical analysis, rendering, attribution safeguards, and human-facing discussion.

It complements synthetic regressions under `tests/vgm/`, `tests/spc/`, and `tests/model/`.

Real corpus files should be useful for questions such as:

```text
can the file execute correctly?
can exact source/device state be captured?
can persistent musical behavior be recovered conservatively?
can parts / gestures / sections / repetition be analyzed?
can the reference and enhanced renderers be compared?
can VGM Tooling describe what a listener hears naturally?
can every material description descend into evidence?
can metadata remain separate from attribution evidence?
```

## Preservation rule

User-supplied corpus files are immutable evidence objects.

Do not rewrite, retag, normalize, recompress, or otherwise mutate a canonical corpus object merely to make metadata cleaner.

The canonical supplied object depends on the delivery form:

```text
direct runnable files
        ↓ preserve each byte-for-byte
canonical VGM / VGZ / SPC / NSF / NSFe fixtures

archive supplied as a canonical object
        ↓ preserve archive byte-for-byte when retained
archive evidence object
        ↓ extract without rewriting media
runnable VGM / VGZ / SPC fixtures
```

A ZIP is not required merely to duplicate files that were supplied and committed directly. When direct runnable files are the canonical corpus objects, preserve those files and their hashes. When an archive itself is intended to be retained as evidence, preserve its exact bytes and hash as a separate object. Record any extraction or transformation explicitly.

## Metadata is not one evidence class

Embedded metadata can be exact **as bytes contained in the supplied artifact** while still being historically stale or non-authoritative as a real-world claim.

Therefore:

```text
embedded metadata value
!= authoritative musicological identity
```

The corpus must never turn a convenient tag into stronger evidence merely because the player exposes it.

## Artist-name rule

For these user-supplied test sets, **embedded artist metadata is not authoritative**.

In particular, do not use VGM GD3 artist fields, SPC internal artist fields, or analogous native embedded tags as the expected artist identity for attribution, catalog, or user-facing artist-name tests.

The user's curated artist naming lives in foobar2000 external tags. Helix has already ingested that state and explicitly records `external-tags.db` as canonical for the user's local foobar metadata.

Helix's current normalized route is:

```text
foobar2000 external-tags.db
        ↓
Helix canonical music source records
        ↓
Library/Sources/Music/FoobarLibraryMetadata.jsonl
        ↓
optional compact query/index projections
```

The historical capitalization above is retained because it is the source path recorded by the Helix ingestion report. VGM Tooling does not own or duplicate that database.

Therefore:

```text
embedded artist tag
= exact artifact metadata only

Helix-ingested foobar external artist tag
= preferred user-facing artist identity for this corpus
```

If the Helix/foobar external tag is unavailable to a standalone test runner, artist identity should remain `unknown` unless an independently verified documentary source supplies it.

Do **not** fall back silently to the embedded artist field.

This rule is intentionally narrow. Other embedded fields may be inspected according to their own provenance and reliability; the user correction specifically applies to artist names.

## Cross-project ownership

VGM Tooling owns:

- the immutable VGM/VGZ/SPC/NSF/NSFe fixture bytes;
- hashes and source-family inventory;
- execution/synthesis/performance analysis of those fixtures;
- corpus-specific regression expectations that belong to game-music machinery.

Helix owns:

- the user's canonical ingested foobar metadata;
- provenance back to `external-tags.db`;
- broader music-library identity and cross-project evidence;
- Sonic 3 project state and historical attribution evidence.

Do not copy the full Helix music database into this repository merely to make tests self-contained.

Prefer a small join record when needed:

```text
fixture SHA-256 / normalized local path
        ↕
Helix source-record ID / canonical tag route
```

That join is provenance, not a second metadata source of truth.

## Foobar behavior

The test corpus must preserve the distinction between source metadata and externally supplied library metadata.

A correct foobar-facing path may therefore expose:

```text
source file
  contains stale embedded artist

external foobar tag
  supplies current/curated artist

foobar display
  uses external artist

source bytes
  remain unchanged
```

Tests of metadata precedence should validate the visible/effective result without modifying the fixture bytes.

## Corpus inventory requirements

Each committed corpus set should have a manifest recording at least:

- stable corpus ID and canonical repository path;
- delivery form (`direct-files`, archive import, or another explicitly recorded route);
- original supplied filename or archive name when applicable;
- SHA-256 of every canonical runnable file;
- deterministic whole-set digest and/or Git tree identity when available;
- source family (`VGM`, `VGZ`, `SPC`, `NSF`, or `NSFe`);
- game/work identifier when known;
- track/cue identifier when known;
- chip/device family when determinable from the source;
- optional normalized local/source path used to join Helix metadata;
- optional Helix source-record ID when resolved;
- whether a foobar external artist tag exists;
- authoritative artist value only when supplied through the Helix/foobar route or independently verified;
- known provenance/capture limitations;
- any reason a fixture is particularly useful for a regression.

When the canonical object is a retained archive, record the archive hash too. Do not invent an archive requirement for a corpus whose canonical objects are the directly supplied runnable files.

Do not store a guessed artist merely to make the inventory look complete.

## Sonic 3 attribution control

The Sonic 3 corpus is especially useful because it should **not** have one trivially trusted artist answer.

Helix's active `project:sonic-3-music-attribution` distinguishes:

```text
track / exact version
→ composer
→ arranger / implementation author
→ source company or team
→ evidence
→ confidence / unresolved conflict
```

The project has explicit prototype/final/replacement distinctions and unresolved composer/arranger conflicts. It also contains technical fingerprint hypotheses involving voice-bank use, MOD settings, FM/PSG initialization, patch reuse, SFX-heavy construction, and version divergence.

This gives VGM Tooling a strong negative control:

```text
external artist tag
!= composer proof

sound-team tag
!= track-level composer proof

arrangement / implementation fingerprint
!= composition fingerprint

shared patch / voice / MOD behavior
!= authorship proof

prototype work identity
!= final-version realization identity
```

A Sonic 3 fixture may legitimately have several simultaneous metadata/evidence statements, for example:

```text
external foobar artist display        curated library metadata
embedded GD3 artist                   stale artifact metadata
known exact version                   artifact/work evidence
arranger candidate                    technical/historical hypothesis
composer candidate                    separate attribution hypothesis
sound team                            contextual metadata
```

These must remain separate.

The strongest current project rule is:

> Start from the exact track/version object and the authorial layer in dispute. Technical resemblance may strengthen an arrangement/implementation hypothesis without silently becoming composer attribution.

## Analysis rules

Corpus files should pressure-test real behavior without becoming golden answers for unsupported higher-level claims.

For VGM/VGZ:

- exact logged commands and timing are source evidence;
- GD3 is artifact metadata, not execution truth;
- original driver/score/track identity remains unknown unless independently retained;
- artist attribution must not be inferred from stale GD3.

For SPC:

- snapshot CPU/RAM/S-DSP state is source evidence;
- runtime continuation may recover further executable behavior;
- internal textual tags are metadata evidence only;
- artist attribution must not be inferred from stale embedded artist fields.

For NSF/NSFe:

- the preserved object contains executable program/data rather than a VGM-style register timeline;
- cheap admission checks may establish container identity, chunk/header structure, and declared subsong count;
- valid container structure does not establish correct playback;
- embedded text fields are artifact metadata and do not independently establish attribution.

For both:

- musical structure remains derived/hypothesis as appropriate;
- listener/critic/producer/engineer descriptions are discourse projections over support bundles;
- source bytes remain available beneath every higher claim;
- library metadata, musicological attribution, and technical realization evidence remain independently scoped.

## Validation style

Prefer corpus tests that state what capability they exercise rather than hard-coding broad aesthetic judgments.

Useful examples:

```text
fixture executes without semantic gap

loop boundary is preserved exactly

YM2612 / PSG / S-DSP state remains rebuildable

physical voice episodes remain bounded

known control change survives performance extraction

source-relative feature correctly reports present / unknown / unavailable

natural section description changes when the underlying support bundle changes

external artist metadata overrides embedded artist metadata in the foobar-facing path

technical arrangement fingerprint does not promote composer attribution

prototype and final versions remain distinct work/realization objects
```

The corpus is an empirical pressure surface, not a shortcut around provenance.
