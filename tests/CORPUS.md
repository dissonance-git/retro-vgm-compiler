# VGM Tooling test corpus

This document governs user-supplied real-music fixtures used to pressure-test VGM Tooling beyond synthetic unit cases.

## Purpose

The corpus exists to test the full path from preserved game-music objects into execution, synthesis, musical analysis, rendering, and human-facing discussion.

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
```

## Preservation rule

User-supplied corpus files are immutable evidence objects.

Do not rewrite, retag, normalize, recompress, or otherwise mutate the source file merely to make metadata cleaner.

When corpus archives are supplied, preserve the exact archive plus hashes before extracting runnable fixtures. Record any extraction/transformation separately.

```text
supplied bytes
        ↓ preserve exactly
immutable corpus/import object
        ↓ extract without rewriting media
runnable VGM / VGZ / SPC fixtures
        ↓
analysis / execution / rendering tests
```

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

The user maintains authoritative artist naming through external foobar tags, which suppress the outdated embedded artist value without rewriting the original game-music file.

Therefore:

```text
embedded artist tag
= exact artifact metadata only

external curated artist tag
= preferred user-facing artist identity for this corpus
```

If an external tag is unavailable to a standalone test runner, artist identity should remain `unknown` unless a separate curated corpus manifest or independently verified documentary source supplies it.

Do **not** fall back silently to the embedded artist field.

This rule is intentionally narrow. Other embedded fields may be inspected according to their own provenance and reliability; the user correction specifically applies to artist names.

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

- original supplied filename/archive name;
- SHA-256 of the preserved supplied object;
- extracted fixture path;
- SHA-256 of each runnable file;
- source family (`VGM/VGZ` or `SPC` initially);
- game/work identifier when known;
- track/cue identifier when known;
- chip/device family when determinable from the source;
- whether an external artist tag exists;
- authoritative artist value only when supplied by the external tag set or independently verified;
- known provenance/capture limitations;
- any reason a fixture is particularly useful for a regression.

Do not store a guessed artist merely to make the inventory look complete.

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

For both:

- musical structure remains derived/hypothesis as appropriate;
- listener/critic/producer/engineer descriptions are discourse projections over support bundles;
- source bytes remain available beneath every higher claim.

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
```

The corpus is an empirical pressure surface, not a shortcut around provenance.
