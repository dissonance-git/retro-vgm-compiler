# VGM Compiler

VGM Compiler is a provenance-aware compiler and musical reasoning system for digital game music.

Its primary goal is **holistic musical understanding**. Decoding formats, executing drivers, reconstructing synthesis, tracing provenance, rendering audio, and attribution are supporting capabilities that make that understanding more accurate and defensible.

## Model

```text
native source / executable state / audio evidence
        ↓
source-specific semantics and execution
        ↓
performed gestures + persistent musical parts
        ↓
melody • bass • rhythm • harmony • timbre • articulation
        ↓
phrase • motif • cadence • counterpoint • form • arrangement
        ↓
whole-work + soundtrack model
        ↓
creator grammar / attribution / explanation / transformation
```

The compiler never treats a convenient projection as source truth. MIDI, notation, stems, PCM, chord labels, prose, and attribution are outputs or hypotheses unless the source itself establishes them.

Read [`docs/architecture.md`](docs/architecture.md) for the durable semantic/evidence contract and [`docs/vgm-compiler-roadmap.md`](docs/vgm-compiler-roadmap.md) for the active frontier.

## Current frontier

The current semantic work is phrase-scale arbitration around locally ambiguous cadential behavior, especially Ionian `V → VI` cases where local closure evidence can coexist with larger-scale continuation and later authentic resolution.

The implementation deliberately preserves competing interpretations until independent phrase-role and longer-range evidence separates them.

## Repository map

| Path | Owner |
| --- | --- |
| `model/` | source-independent musical and evidence semantics |
| `components/` | source-family, device, decoder, and rendering machinery |
| `tests/` | regressions and immutable real-music corpus |
| `research/` | bounded investigations and named integration testbeds |
| `docs/` | current architecture, roadmap, rendering, and provenance contracts |
| `tools/` | reusable repository-facing commands |
| `imports/` | preserved imported/upstream evidence that must remain byte-stable |
| `patches/` | maintained patch stacks for external source trees |

Start at the smallest owner. For a task concept, prefer `python tools/repository_catalog.py --focus cadence` or another focused term before broad repository search. Use the unfiltered catalog only when repository shape itself matters. Do not build a second prose inventory.

Useful local maps:

- [`model/README.md`](model/README.md)
- [`components/README.md`](components/README.md)
- [`research/README.md`](research/README.md)
- [`tests/corpus/README.md`](tests/corpus/README.md)
- [`tools/README.md`](tools/README.md)

## Build and test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`tools/run_core_tests.py` and `tests/run_full_core_suite.py` provide repository-owned validation routes. The canonical private foobar2000 VGM/SPC package path is `.github/workflows/private-foobar-build.yml` plus `tools/build_private_foobar_components.ps1`.

A compile is not a delivery. A blocked CI runner is not a pass. Report build, tests, corpus evidence, package/runtime verification, and listening validation as separate evidence states.

## Durable laws

- Preserve source-native semantics before sharing abstractions.
- `physical slot != voice episode != persistent musical part != auditory stream`.
- `exact != derived != hypothesis`; provenance and capture quality are separate coordinates.
- Unknown is not false and unavailable is not absent.
- A higher musical claim may summarize lower evidence but may not erase its uncertainty.
- A cadence label may not provide the phrase closure used to prove that cadence.
- Composition, arrangement/programming, driver/toolchain, patch/sample design, and final realization attribution remain distinct.
- Similarity is evidence, not authorship proof.
- Reference rendering remains available beneath every enhancement.
- Generated files and caches are disposable; canonical evidence and contracts are tracked.
- Repository structure and tooling should maximize useful reasoning per context token without sacrificing provenance, uncertainty, or validation power.
- Historical state belongs in Git history unless it remains operationally necessary.
- Corrections outrank narrative consistency.

## Project boundaries

VGM Compiler owns game-music source, execution, analysis, rendering, and source-native playback bridges. Helix owns broader research continuity and cross-project evidence. libaural owns general artificial hearing. Omniphony owns general spatial presentation.

Cross-project evidence may be referenced without copying another project's ontology or database into this repository.