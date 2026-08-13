# Hardware reference evidence

This directory is a compact, test-oriented projection of large primary hardware and development documents.

The source PDFs themselves are **not** committed here by default. Large scans remain archival evidence outside the implementation repository. This directory preserves enough information to find the source again, identify what was extracted, and use stable documented facts without repeatedly re-reading an entire manual.

The rule is:

```text
primary document
        ↓
source catalog + page-level provenance
        ↓
small reusable documented facts
        ↓
code/tests/research may consume those facts

when the compact record is insufficient or sources disagree
        ↓
re-open the primary document
```

Do not turn these files into replacement manuals.

## Contents

- `schema.json`: schema for source records and extracted claims.
- `sources.json`: catalog of primary documents currently used by the project.
- `devices/*.json`: small device-specific claim sets.
- `platforms/*.json`: platform/bus/access claims that should not be collapsed into one chip's semantics.
- `../../tools/validate_hardware_refs.py`: checks source IDs, page ranges, evidence classes, checksums, and claim-ID uniqueness without requiring a third-party schema library.

## What belongs here

Extract facts that are stable and repeatedly useful for implementation or testing, such as:

- device/family identity;
- channel and operator topology;
- register-bank organization;
- documented key-on or pitch encoding;
- documented clock relationships and frequency formulas;
- LFO, DAC/PCM, timer, routing, and envelope semantics;
- platform bus/address and chip-access rules;
- documented compatibility relationships;
- documented variant distinctions.

Do **not** exhaustively transcribe:

- explanatory prose;
- every electrical characteristic;
- every timing diagram;
- every register description;
- editorial commentary;
- inferred musical meaning.

Those remain in the primary source until a task makes them relevant.

## Evidence law

Every extracted claim keeps:

- a source ID;
- PDF page number;
- evidence class;
- whether the claim is literal/documented or an interpretation;
- optional context needed to avoid over-generalizing the statement.

A manual-derived fact is not automatically complete hardware truth. Compare it with emulator/die-analysis implementations, surviving driver source, real VGM execution, and measurements where relevant.

```text
official documentation
        +
emulator / die-analysis behavior
        +
real driver / VGM execution
        ↓
convergent evidence where they agree
```

Contradictions are retained as research signals rather than silently reconciled.

## Storage policy

Large PDF scans should normally remain outside Git. `sources.json` records `repository_binary: false` for that reason.

If a particular scan later becomes difficult to obtain or its exact version is required for reproducibility, preserve that artifact in an appropriate archival store and record its checksum here. Do not make the implementation repository the default document archive.
