# VGM Compiler

Executable understanding, musical analysis, source-native rendering, perceptual organization, and human-readable reasoning for digital game music.

## Objective

VGM Compiler follows the transformations that actually produce and are heard as game music instead of flattening every source into MIDI, stems, final PCM, or a detached analytical summary.

**The primary objective is holistic musical understanding.** Exact decoding, execution tracing, synthesis reconstruction, source isolation, driver archaeology, formal analysis, and provenance are supporting machinery. They matter because they improve, constrain, explain, or validate the musical model.

The current architecture and roadmap live in [`docs/vgm-compiler-roadmap.md`](docs/vgm-compiler-roadmap.md).

```text
native encoded truth
        ↓
authoring language / source project / sequence semantics
        ↓
compiler / assembler / conversion tool
        ↓
driver program / logical tracks / scheduling / arbitration
        ↓
device / synthesis / sample state
        ↓
performed gestures and persistent musical parts
        ↓
melody • bass • rhythm • harmony • timbre • articulation
        ↓
phrase • motif • cadence • counterpoint • form • arrangement
        ↓
integrated cross-representation song model
        ↓
recurring rules across independent works
        ↓
cross-soundtrack creator grammar
        ↓
blind attribution stress tests
        ↓
holistic soundtrack understanding and human explanation
```

The project is vertically end-to-end. The lowest useful fact may be a bit field, byte, address, pointer rule, register write, sample, driver state transition, or machine cycle. The highest useful result may be an integrated explanation of what a cue is doing musically, how it relates to the soundtrack, and which creator-specific behaviors recur in other projects.

## Current implementation

The repository already contains substantial working machinery across the stack.

### Source and execution

- VGM/VGZ command streams and Genesis device-state reconstruction;
- SPC snapshot, RAM, sample, runtime, and voice evidence;
- PSF-family and other executable-rip object reconstruction;
- native driver and sequence research, including SMPS-family dialect/revision boundaries;
- exact source/runtime provenance and source-aware capture;
- reference and enhanced rendering paths;
- installable private foobar2000 VGM/SPC decoder delivery and verification.

### Musical semantics

- persistent musical-part identity separated from physical channels;
- performed pitch trajectories and articulation evidence;
- motif recurrence and transformation;
- phrase-boundary hypotheses and cross-part phrase consensus;
- phrase regions and relationships;
- tonal-center, key-class, chord-degree, and harmonic verticality hypotheses;
- harmonic transitions and harmonic rhythm;
- persistent-part-aware voice leading;
- bass/harmony interaction and source-backed harmonic-role evidence;
- cadential-arrival evidence;
- Ionian cadence morphology candidates with provenance-bound degrees;
- persistent-melody PAC/IAC refinement;
- non-circular formal-closure evidence;
- morphology/formal-closure binding;
- V→VI continuation and deferred-authentic-resolution candidates;
- V→VI deceptive-close candidates grounded by independent phrase completion;
- section, counterpoint, imitation, arrangement, creator-grammar, and role-aware attribution structures.

### Verification and transformation

- dependency-free C++17 semantic regressions;
- CMake/CTest registration for the current cadence evidence family;
- real-corpus controls and immutable corpus manifests;
- negative and matched-decoy controls;
- source/representation differential tests;
- formal/static-analysis helpers including graph, constraint, and solver-oriented machinery;
- loss-declared symbolic projections and semantic round-trip experiments.

## Current semantic frontier

The compiler now represents two competing interpretations of an Ionian V→VI arrival without pretending the chord sequence settles the answer.

```text
V → VI
+ cross-part continuation through VI
+ later independently grounded V → I closure
→ deferred authentic resolution candidate
```

```text
V → VI
+ independently grounded non-cadence-derived phrase completion at VI
→ deceptive cadence candidate
```

Both are intentionally **candidates**. Neither layer is allowed to establish the final cadence class or Roman-numeral discourse by itself.

The next bridge is phrase-syntax arbitration:

> **When local harmonic morphology and formal evidence support different interpretations, what independent longer-range evidence distinguishes close, continuation, return, prolongation, and deferred resolution?**

That requires phrase role, continuation/return structure, subsequent harmonic behavior, and style-sensitive grouping evidence without turning chord sequences into a vocabulary lookup. The evidence firewall remains strict: a cadence label may not supply the phrase closure used to prove itself.

## Evidence boundaries

The project keeps identities and analytical levels separate.

```text
physical slot != voice episode != persistent musical part != auditory stream
```

```text
register frequency
!= nominal frequency
!= programmed pitch
!= transposed pitch
!= performed pitch
!= heard pitch
!= notation spelling
```

```text
simultaneous pitches
!= chord spelling
!= harmonic function
!= cadence
!= form
!= creator grammar
!= authorship
```

Likewise:

```text
same chip state != same upstream intention
same opcode != same meaning across driver / scope / revision
same driver family != same revision behavior
same physical channel != same logical musical part
transformed runtime != authoring source
reconstructed source candidate != exact authored source
```

Higher analysis may summarize lower evidence, but it may not erase uncertainty or provenance underneath it.

## Two generalization axes

Composer-level and soundtrack-level understanding must generalize in two directions.

### Same work across representations

```text
MIDI / MML / tracker / native sequence / source ASM
↕
driver program / compiled sequence / runtime state
↕
VGM / SPC / PSF-family execution
↕
patches / samples / synthesis state
↕
rendered audio / auditory organization
↕
external transcription / documentary evidence
```

Each representation is a sensor. They should cross-check one another where a real correspondence exists, but they are not interchangeable.

### Same creator across different soundtracks

A creator model built from one soundtrack can accidentally learn the soundtrack's driver, patch/sample bank, arranger, platform, production period, cue functions, or related themes.

Prefer controls such as:

```text
leave-one-work-family-out
leave-one-soundtrack-out
leave-one-platform-out
leave-one-arranger-out
leave-one-career-period-out
```

See [`docs/composer-level-understanding.md`](docs/composer-level-understanding.md), [`docs/holistic-musical-understanding.md`](docs/holistic-musical-understanding.md), and [`research/music/composer-grammar-attribution.md`](research/music/composer-grammar-attribution.md).

## Source families

Current work spans materially different representations and architectures, including:

- VGM/VGZ command streams;
- SPC snapshots and S-DSP state;
- NSF and related executable-rip formats;
- PSF1, GSF, USF, 2SF, and NCSF object families;
- native music drivers and sequence formats;
- MIDI, MML, tracker source, source ASM, and other symbolic representations;
- ROM-derived samples, patches, sequences, executables, and control data;
- rendered audio and documentary evidence.

These remain source-specific until materially different systems earn a common abstraction.

## Source-native enhanced rendering

The accurate/reference renderer is the scientific control, not the quality ceiling.

Enhanced rendering may relax an implementation ceiling only when the same musical identity survives: notes, timing relationships, groove, part relationships, patch/sample identity, articulation, modulation, automation, deliberate effects, and structural density.

See [`docs/source-native-enhanced-rendering.md`](docs/source-native-enhanced-rendering.md).

## Source-aware immersive playback

When a source family exposes real causal voices or channels, VGM Compiler can hand those objects to Omniphony before historical stereo collapse. Source provenance stays here; immersive presentation stays in Omniphony.

See [`docs/omniphony-realtime-spatial-path.md`](docs/omniphony-realtime-spatial-path.md).

## Private foobar2000 delivery

VGM Compiler owns the reproducible build and publication path for the private VGM and SPC decoder components.

```text
current main + pinned upstream inputs
→ native decoder builds
→ .fb2k-component packaging
→ package / PE / ABI verification
→ packaged runtime verification
→ combined bundle verification
→ GitHub Actions artifact upload
→ rolling verified delivery release
```

A successful compile is not a delivery. Publish-ready means the declared end-to-end workflow completed successfully.

## Repository map

Use this path before broad search:

```text
current main
→ README.md
→ AGENTS.md
→ docs/vgm-compiler-roadmap.md
→ smallest canonical owner
→ recent commits for that surface
→ exact target code, test, or document
```

| Need | Route |
| --- | --- |
| project identity and map | `README.md` |
| operating law | `AGENTS.md` |
| current frontier | `docs/vgm-compiler-roadmap.md` |
| source-family machinery | `components/README.md` |
| shared musical semantics | `model/README.md` |
| corpus | `tests/corpus/README.md` and `tests/corpus/manifest.json` |
| research programs | `research/README.md` |
| utilities | `tools/README.md` |
| current documentation | `docs/README.md` |
| historical prose | `docs/history/` |

Repository shelves:

```text
model/          shared musical/evidence semantics that earned sharing
components/     source-family and device-specific machinery
tests/          regressions + immutable real corpus
research/       bounded investigations and named testbeds
docs/           current architecture / reasoning contracts
tools/          reusable executable operations
imports/        preserved upstream/import provenance
patches/        maintained patch material
```

For exact mechanical inventory:

```text
python tools/repository_catalog.py
```

Generated inventories are projections, not authority.

## Testing

Core regressions are registered through CMake and can also be exercised with `tools/run_core_tests.py`.

```text
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Important mechanisms should also be challenged by real corpus controls, negative controls, independent implementations, matched decoys, revision differentials, cross-soundtrack holdouts, and confound interventions.

## Working rules

1. Holistic musical understanding is the primary objective.
2. Blind attribution is a capstone stress test, not a shortcut objective.
3. Preserve source-native semantics before normalizing meaning.
4. Keep physical channels, voice episodes, persistent parts, auditory streams, and musical roles distinct.
5. Keep composition, arrangement/programming, driver/toolchain, patch/sample, and final realization attribution distinct.
6. Preserve driver family, dialect/revision, semantic scope, timing domain, and capability state when they affect interpretation.
7. Never infer command semantics from opcode bytes without the required dialect/scope context.
8. Do not jump from low-level pitch directly to harmony, creator grammar, or authorship.
9. Do not let a cadence label circularly establish the phrase closure needed to prove that cadence.
10. Group related versions and derivative cues so the system cannot win by recognizing the work.
11. Use same-creator cross-soundtrack and cross-platform controls to separate creator invariants from project artifacts.
12. Keep exact, derived, inferred, perceptual, and external claims distinct.
13. Unknown is not negative evidence.
14. Corrections outrank narrative coherence.
15. Accuracy/reference behavior remains available beneath every enhancement.
16. Traceability supports understanding but does not substitute for it.

> **Understand each musical work across its representations, then understand creators across different soundtracks deeply enough that attribution can emerge as a consequence of the music rather than the production environment.**
