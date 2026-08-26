# Upstream and reference evidence registry

This research owner records **why external sources matter and what claim classes they can constrain**. It is not the operational dependency lockfile and must not duplicate exact build pins already owned by import manifests, workflows, patch guards, or build tooling.

Durable semantic law lives in [`../../docs/architecture.md`](../../docs/architecture.md). Immutable imported package identities live in [`../../imports/MANIFEST.md`](../../imports/MANIFEST.md). Exact source revisions required by a build belong with the build/patch owner that verifies them.

A reference is not automatically a runtime dependency:

```text
primary specification / source / measurement
        ↓
source-family implementation evidence
        ↓
independent implementation controls
        ↓
project-owned semantic model
```

## Evidence classes

- **Format authority:** primary specifications such as the VGMRips VGM specification constrain byte, header, command, timing, loop, data-block, and version semantics.
- **Hardware authority:** official manuals, application notes, schematics, development material, and measured hardware constrain device behavior and exposed mechanisms.
- **Implementation controls:** libvgm, Nuked cores, ymfm, SPCPlay/SNESAPU, MAME, QSound implementations, and similar projects provide independent executable comparison surfaces.
- **Container/platform controls:** psflib, GSF/USF/2SF/NCSF players and tools constrain xSF envelope/dependency behavior while preserving platform-specific execution boundaries.
- **Driver/source controls:** SMPSPlay, GEMSPlay, VGMTrans, Hoot, preservation tooling, and source ledgers constrain driver identity, sequence/instrument/sample data, and executable behavior.
- **Execution-to-music controls:** SPC/VGM-to-MIDI and related recovery tools expose recoverable musical projections without making MIDI the project ontology.
- **Representation observatories:** see [`music-representation-systems.md`](music-representation-systems.md) and [`openmusic-libraries.md`](openmusic-libraries.md) for upper-layer pressure tests.
- **Literature:** papers contribute established distinctions and falsifiable experiment ideas, not imported ontology.

## Use law

For every external source:

1. Record the question it helps answer.
2. Prefer primary evidence when available.
3. Keep conceptual evidence separate from imported implementation.
4. Inspect licensing before reuse and preserve required attribution.
5. Keep source-specific mechanisms source-specific.
6. Put exact mutable pins in the operational owner that verifies them, not here.
7. Promote a cross-system lesson only after independent evidence supports the same relation.

```text
shared frontend != shared semantic depth
register resemblance != shared musical law
reference implementation != format specification
conceptual usefulness != permission to copy
```

The purpose of this registry is evidence routing and re-entry, not a second dependency database or architecture document.
