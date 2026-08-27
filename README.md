# VGM Compiler

VGM Compiler is a provenance-preserving compiler and musical reasoning system for digital game music.

It recovers source-native execution and synthesis evidence, lifts that evidence into persistent musical structure, and keeps a route from higher interpretation back to the support that earned it.

```text
native source / executable state / audio evidence
→ source-specific execution and synthesis
→ performed gestures + persistent musical parts
→ melody / bass / rhythm / harmony / timbre / articulation
→ phrase / motif / cadence / counterpoint / form / arrangement
→ whole-work + soundtrack relations
→ creator grammar / attribution / explanation / transformation
```

MIDI, notation, stems, PCM, chord labels, prose, reconstructed source, and attribution are projections or claims. None becomes source truth merely because it is convenient.

## Repository landmarks

> **More capability, fewer conceptual machines.**
>
> **One concept, one writable owner. Store authority. Derive views. Preserve evidence. Promote slowly.**

| Surface | Canonical responsibility |
| --- | --- |
| `README.md` | repository identity, architecture entry, owner routing |
| `AGENTS.md` | repository/change/evidence/concurrency/publication law |
| `docs/architecture.md` | shared semantic, provenance, evidence, and abstraction law |
| `docs/musical-understanding.md` | north-star definition of musical understanding |
| `docs/vgm-compiler-roadmap.md` | unresolved/active project frontier |
| focused `docs/*.md` | specialized durable contracts |
| `research/` | bounded evidence, experiments, preregistrations, controls, observatories |
| `model/` | source-independent musical/evidence semantics that earned sharing |
| `components/` | source-family, device, driver, decoder, execution, rendering machinery |
| `tests/` | executable contracts and immutable real-music controls |
| `tools/` | reusable operations and derived projections |
| `imports/` | immutable external inputs required by current work |
| `patches/` | maintained transformations of external source trees |
| `.github/workflows/` | executable CI, private build, and validation routes |
| `.agents/` | agent and connector procedure, never musical/research truth |
| Git history | chronology and retired repository state |

A current fact should not be mirrored across status documents, architecture prose, research notes, and code. Keep the canonical owner and derive or delete the rest. Git preserves superseded repository narrative.

## Enter by the smallest owner

For a local checkout, prefer a task-sized mechanical projection:

```bash
python tools/repository_catalog.py --focus <concept>
```

The catalog derives relations from tracked state. It is not a second repository database.

Useful namespace owners:

- [`model/README.md`](model/README.md)
- [`components/README.md`](components/README.md)
- [`tests/README.md`](tests/README.md)
- [`research/README.md`](research/README.md)
- [`tools/README.md`](tools/README.md)

Reasoning agents entering through GitHub use the derived bootstrap emitted by [`tools/github_agent_bootstrap.py`](tools/github_agent_bootstrap.py) plus the live [`.agents/skills/`](.agents/skills/) inventory. No committed connector-state file owns first-connection truth; the bootstrap is rebuilt from the current Git snapshot and current skill bytes.

## Build and verification

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`tools/run_core_tests.py` and `tests/run_full_core_suite.py` provide broader repository-owned validation. Private foobar2000 VGM/SPC delivery is owned by `.github/workflows/private-foobar-build.yml` and `tools/build_private_foobar_components.ps1`.

Keep evidence states distinct:

```text
source/container correctness
≠ code compiles
≠ tests pass
≠ corpus control passes
≠ package/runtime delivery works
≠ physical listening validates the result
```

## Project boundary

VGM Compiler owns executable game-music source semantics, execution/reconstruction, musical analysis, source-native rendering, and playback bridges.

Helix owns broader cross-project research continuity. libaural owns general artificial hearing. Omniphony owns general spatial presentation. Cross-project evidence may be linked without copying another project's ontology, workspace state, or databases into this repository.
