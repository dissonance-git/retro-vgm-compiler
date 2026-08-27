# Real-music corpus

`tests/corpus/` is VGM Compiler's immutable real-music pressure surface. Synthetic tests isolate mechanisms; the corpus asks whether those mechanisms survive complete historical artifacts.

## Canonical ownership

```text
fixture bytes
  immutable preserved evidence

tests/corpus/manifest.json
  machine-readable set inventory + provenance

<corpus-id>.sha256
  exact per-file identity

this README
  preservation / evidence / testing policy
```

Do not maintain a second prose inventory of sets, dates, counts, downloads, or hashes. Git records when the corpus changed; the manifest and hash inventories record what exists now.

## Preservation law

Canonical corpus objects are byte evidence.

Do not retag, normalize, recompress, resample, rewrite headers, rename for metadata cleanup, or otherwise mutate an admitted fixture merely to simplify analysis.

Delivery determines what is canonical:

```text
direct runnable files
→ preserve exact files

archive intentionally retained as evidence
→ preserve archive bytes + hash
→ extract without rewriting media
→ retain transformation/provenance

archive used only as transport
→ do not keep a redundant copy by default
```

Use the repository-owned verifier:

```bash
python tools/corpus_import.py --verify
```

To admit existing direct files:

```bash
python tools/corpus_import.py --record-existing --id <id>
```

Archive import remains available when the archive itself is the intended preservation object.

## Evidence boundary

Exact artifact bytes do not make every embedded claim authoritative.

```text
embedded metadata value
!= authoritative historical identity

valid source/container
!= correct execution
!= recovered musical structure
!= listener/perceptual claim
!= historical attribution
```

VGM GD3, SPC text fields, xSF tags, and similar metadata may be exact as bytes while remaining editorial, stale, incomplete, or non-authoritative as musicological claims.

Where user-curated external library metadata is available through Helix, it may provide preferred user-facing identity. That remains metadata provenance rather than proof of composition, arrangement, programming, sound design, or authorship.

If an independent identity source is unavailable, leave the claim unknown rather than silently promoting embedded tags.

## Source-family law

The corpus intentionally contains materially different source families. Admission proves only what the corresponding parser/executor/test actually checks.

Examples:

```text
VGM/VGZ
  logged commands / clocks / timing / data blocks may be exact

SPC
  saved CPU/RAM/S-DSP state may be exact
  later runtime evidence requires controlled execution

NSF/HES/KSS/SGC and similar executable containers
  program/data structure may be exact
  runtime semantics require execution proof

PSF/GSF/USF/2SF/NCSF
  shared xSF envelope facts may be exact
  platform-specific effective objects and runtimes remain distinct
```

A shared container does not imply a shared driver, sequence model, voice model, or runtime.

Detailed source-family semantics belong to their component/research/test owners, not this corpus policy.

## Attribution controls

Creator or artist labels must not leak into creator-blind feature extraction.

Preserve distinctions such as:

```text
external artist tag
!= composer proof

sound-team credit
!= track-level composer proof

patch / driver / implementation similarity
!= composition fingerprint

prototype realization
!= final realization
```

Attribution experiments begin from the exact work/version object and the creative role under test. Candidate labels enter only at the explicit evaluation boundary.

## Cross-project ownership

VGM Compiler owns:

- immutable game-music fixture bytes and required sidecars;
- hashes and source-family inventory;
- parsing/execution/synthesis/performance regressions tied to game-music machinery.

Helix may own broader library metadata, documentary research, and cross-project identity/attribution evidence.

Join across projects through explicit provenance. Do not copy another project's database into this corpus merely to make a test self-contained.

## Derived artifacts

A result computed from a canonical fixture is not automatically canonical evidence.

```text
immutable source
→ repeatable tool / controlled execution
→ derived object
→ hypothesis / comparison / report
```

Caches, matrices, rankings, rendered audio, temporary extractions, and analysis sidecars stay ignored/disposable unless an experiment explicitly promotes the exact bytes as a frozen witness, preregistration, or regression input.

## Good corpus tests

A corpus test should name the capability and evidence boundary it protects.

Useful questions include whether:

- the fixture parses or executes under its admitted source contract;
- loop/state boundaries remain valid;
- source/device/sample state remains rebuildable;
- physical voice episodes stay bounded;
- missing evidence remains unknown rather than fabricated;
- persistent-part or phrase hypotheses respect continuity gaps;
- creator-blind extraction remains label-blind;
- source replacement preserves declared identities;
- a shared abstraction survives heterogeneous material.

Avoid broad aesthetic golden tests unless the exact perceptual obligation and listening protocol are themselves the test.

## Adding a set

A new corpus set requires:

- a stable corpus ID/path;
- exact canonical bytes;
- SHA-256 inventory;
- source-family classification;
- delivery/provenance record;
- justified work/cue identifiers when available;
- explicit limitations;
- a reason the set adds discriminating coverage.

Do not invent missing metadata to make the manifest look complete.

## Governing principle

> **Preserve the artifact exactly, state only what the source establishes, and use real music to falsify abstractions that looked safe in isolation.**
