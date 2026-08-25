# Real-music corpus

`tests/corpus/` is VGM Compiler's permanent immutable pressure surface for real digital game-music objects. Synthetic tests isolate mechanisms; the corpus tests whether those mechanisms survive contact with complete historical artifacts.

## Canonical ownership

```text
fixture bytes                canonical preserved evidence
tests/corpus/manifest.json   machine-readable set inventory + provenance
<corpus-id>.sha256           exact per-file hashes
this README                   preservation / evidence / testing policy
```

Do not maintain a second prose inventory of every set, dated expansion, file count, download, or digest. Git history records when the corpus grew; the manifest and hash inventories record what exists now.

## Layout

```text
tests/corpus/
├── README.md
├── manifest.json
├── <corpus-id>.sha256
└── <corpus-id>/
    └── immutable runnable files and required sidecars
```

The runnable/source files are canonical evidence objects. Preserve them byte-for-byte.

## Preservation law

Do not retag, normalize, recompress, rename for metadata cleanup, resample, rewrite headers, or otherwise modify a canonical corpus object merely to make analysis easier.

The canonical supplied object depends on delivery:

```text
direct runnable files
→ preserve each exact file

archive intentionally retained as evidence
→ preserve archive bytes and hash
→ extract without rewriting media
→ record transformation/provenance
```

A ZIP is not required merely to duplicate direct files already preserved canonically. If an archive was only a transport used to obtain the canonical runnable files, record its provenance/hash when required and do not keep redundant transport material by default.

## Verification

Use the repository-owned verifier:

```bash
python tools/corpus_import.py --verify
```

To index a direct-file set already placed under `tests/corpus/<id>/`:

```bash
python tools/corpus_import.py --record-existing --id <id>
```

Archive import remains available when the archive itself is the intended preservation object:

```bash
python tools/corpus_import.py --archive soundtrack.zip --id <id>
```

Verification should detect byte drift, path drift, and manifest/hash disagreement without rewriting the evidence.

## Metadata is not one evidence class

An embedded metadata field can be exact **as bytes in the artifact** while being stale, editorial, incomplete, or historically non-authoritative as a real-world claim.

```text
embedded metadata value
!= authoritative musicological identity
```

For the user-supplied corpus, embedded artist fields such as VGM GD3 or SPC textual tags are not authoritative attribution labels.

Where the user's curated foobar2000 external tags have been ingested by Helix, those tags may provide preferred user-facing library identity. They remain metadata provenance, not proof of composition, arrangement, programming, patch design, or authorship.

If the external/library identity is unavailable to a standalone test, keep artist identity unknown unless an independently verified documentary source establishes it. Do not silently fall back to embedded artist text.

## Cross-project ownership

VGM Compiler owns:

- immutable game-music fixture bytes and required routing sidecars;
- hashes and source-family inventory;
- source parsing/execution/synthesis/performance analysis;
- corpus regressions that belong to game-music machinery.

Helix may own:

- canonical ingested library metadata;
- broader historical/documentary research;
- cross-project work identity and attribution evidence.

Prefer small provenance joins when cross-project evidence is needed. Do not copy another project's database into this corpus to make a test appear self-contained.

## Source-family boundaries

The corpus deliberately contains materially different source classes. Admission proves only what the corresponding validator actually checks.

### VGM / VGZ

Strong source evidence may include exact logged commands, timing/order, declared clocks, embedded data blocks, and format-level structure.

A valid VGM log does not by itself prove original driver-track identity, authored notation, or historical creator role. GD3 is artifact metadata, not execution truth.

### SPC

An SPC snapshot preserves CPU/RAM/S-DSP state at a capture point. Controlled execution may recover further runtime behavior, sample state, and voice evidence.

A valid snapshot does not automatically prove authored note names, original driver parts, persistent musical parts, or perceptual streams.

### NSF / NSFe / HES / KSS / SGC

These retain executable program/data rather than a VGM-style register timeline. Container and playlist checks can establish bounded structural facts without establishing playback correctness, runtime semantics, or attribution.

Extended M3U routing sidecars are canonical when required to select subsongs and are hashed as evidence even when they are not runnable objects themselves.

### PSF-family / GSF / USF / 2SF / NCSF

A shared xSF envelope can establish container structure, version, tags, CRC, dependencies, overlays, and byte provenance. Platform-specific effective objects remain distinct.

```text
shared envelope
!= shared runtime
!= shared driver
!= shared sequence model
!= shared voice model
```

A valid effective object does not establish a running machine or correct playback until that execution route is validated.

### All families

```text
exact source fact
!= recovered musical structure
!= listener/perceptual claim
!= historical attribution
```

Higher analyses remain derived or hypothetical as appropriate and must retain their support route.

## Artist and attribution controls

Corpus labels used to route controls must never leak into creator-blind feature extraction.

Especially for Sonic 3 and other mixed-credit material:

```text
external artist tag
!= composer proof

sound-team credit
!= track-level composer proof

shared patch / voice / MOD behavior
!= authorship proof

technical arrangement fingerprint
!= composition fingerprint

prototype realization
!= final realization
```

Attribution experiments must start from the exact work/version object and the creative role in dispute. Candidate labels enter only at the evaluation boundary required by the experiment.

## Analysis policy

The corpus is an empirical pressure surface, not a shortcut around provenance.

Good corpus tests ask whether a bounded capability survives real material, for example:

```text
fixture parses/executes under its validated source contract
loop boundary is preserved
exact device/sample state remains rebuildable
physical voice episodes remain bounded
source-relative feature reports exact/derived/hypothesis/unknown correctly
programmed control survives performance extraction
persistent-part hypotheses respect continuity gaps
phrase/cadence evidence does not become circular
external metadata stays outside creator-blind features
technical similarity does not promote itself to authorship
reference/enhanced rendering preserve declared identities
```

Avoid broad golden aesthetic judgments. A test should name the capability and evidence boundary it protects.

## Derived artifacts

Caches, sidecars, matrices, rankings, rendered audio, temporary extractions, and experiment reports are not canonical source evidence merely because they were computed from the corpus.

```text
immutable corpus source
→ repeatable tool / controlled execution
→ derived object
→ hypothesis / comparison / report
```

Track a derived artifact only when it is itself required as a frozen experimental witness, preregistration, or regression input. Disposable caches belong in ignored paths.

## Adding a corpus set

A new set should have:

- a stable corpus ID and repository path;
- exact canonical bytes;
- SHA-256 inventory;
- source-family classification;
- delivery/provenance record appropriate to the source;
- known work/cue identifiers when justified;
- device/platform information when determinable;
- explicit limitations;
- a reason the set adds discriminating coverage.

Do not invent missing metadata to make the manifest look complete.

## Governing principle

> Preserve the artifact exactly, state only what the source establishes, and use the corpus to falsify abstractions that looked safe on synthetic examples.
