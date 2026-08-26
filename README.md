# VGM Compiler

VGM Compiler is a provenance-preserving compiler and musical reasoning system for digital game music. It recovers coherent musical structure while retaining a route back to the source, execution, synthesis, and evidence behind every material claim.

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

Convenient outputs are projections, not source truth. MIDI, notation, stems, PCM, chord labels, prose, and attribution remain source evidence only when their provenance actually establishes that role.

## Canonical authority

Each project-level question has one active owner.

| Question | Canonical owner |
| --- | --- |
| What is this repository and where does work live? | `README.md` |
| How should an agent change it? | `AGENTS.md` |
| What semantic and evidence laws must remain true? | `docs/architecture.md` |
| What does musical understanding mean here? | `docs/musical-understanding.md` |
| What is implemented, active, and next? | `docs/vgm-compiler-roadmap.md` |
| What does a specialized subsystem promise? | the corresponding specialized document under `docs/` |
| What evidence or experiment supports a claim? | the narrowest owner under `research/` |

Do not mirror current status into durable contracts. Do not copy durable laws into status documents. Git history owns superseded repository state.

## Repository owners

| Path | Canonical responsibility |
| --- | --- |
| `model/` | source-independent musical and evidence semantics |
| `components/` | source-family, device, decoder, execution, and rendering machinery |
| `tests/` | executable contracts and immutable real-music corpus |
| `research/` | bounded investigations, evidence, controls, and named testbeds |
| `docs/` | durable architecture, musical target, roadmap, and specialized contracts |
| `tools/` | reusable operations and on-demand projections |
| `imports/` | immutable external inputs required by current work |
| `patches/` | maintained transformations of external source trees |

Start at the smallest owner. For a task concept, prefer:

```bash
python tools/repository_catalog.py --focus <concept>
```

The catalog derives local relations from tracked files and prints a task-sized projection. Widen only when the task genuinely requires repository-wide shape.

Local owner maps:

- [`model/README.md`](model/README.md)
- [`components/README.md`](components/README.md)
- [`tests/README.md`](tests/README.md)
- [`tests/corpus/README.md`](tests/corpus/README.md)
- [`research/README.md`](research/README.md)
- [`tools/README.md`](tools/README.md)

## Build and verification

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`tools/run_core_tests.py` and `tests/run_full_core_suite.py` are repository-owned validation routes. Private foobar2000 VGM/SPC delivery is owned by `.github/workflows/private-foobar-build.yml` and `tools/build_private_foobar_components.ps1`.

A compile is not a delivery, and an unexecuted or blocked check is not a pass. Report build, tests, corpus evidence, package/runtime verification, and listening validation as separate evidence states.

## Project boundary

VGM Compiler owns game-music source, execution, analysis, rendering, and source-native playback bridges. Helix owns broader research continuity and cross-project evidence. libaural owns general artificial hearing. Omniphony owns general spatial presentation.

Cross-project evidence may be linked without copying another project's ontology or database into this repository.
