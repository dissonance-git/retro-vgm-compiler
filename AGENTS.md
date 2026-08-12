# AGENTS.md

## GitHub connector entrance

When entering this repository through GitHub, an LLM connector, or another remote agent surface, do not reconstruct VGM Tooling from code search or one active format alone.

Use this bounded entrance sequence:

```text
current main HEAD
→ README.md
→ AGENTS.md at the same HEAD
→ recent commits
→ current source-family / research frontier
→ smallest task-relevant model / component / test / document
→ exact target file
```

1. Resolve `dissonance-git/vgm-tooling` and record the current `main` commit before substantive work.
2. Read `README.md` for current project orientation and this `AGENTS.md` for source, identity, rendering, evidence, and project-boundary law from the same repository state.
3. Inspect recent commits before selecting a work area. This repository can advance quickly; recent commits are activity hints, not authority over exact source evidence or governing contracts.
4. Hydrate only the source family and layer relevant to the task. Do not load every emulator, format, research case, libaural, Omniphony, or Sonic 3 context when one bounded subsystem is enough.
5. Before any GitHub replacement write, re-fetch current `main` and the exact target file. If `main` changed since preflight, re-read this file and reconstruct the edit from current target content.
6. Write against the exact current blob SHA. Preserve unrelated concurrent work. Never replace a file from a cached or reconstructed older copy.
7. After publication, fetch the resulting commit, inspect its changed paths, and confirm the commit remains in current `main` history. Report publication, build/tests, CI, reference parity, and listening validation as separate evidence states.

Fast routes:

- project orientation and current priorities: `README.md`
- governing project/source law: `AGENTS.md`
- common semantic layer: `docs/musical-execution-model.md`
- VGM/VGZ frontier: `docs/vgm-frontier.md`
- upstream provenance and licenses: `docs/upstreams.md`
- shared/source models: `model/`
- source-family implementations: `components/`, `imports/`, and task-specific adapters
- executable tools and diagnostics: `tools/`
- exact regressions: `tests/`
- bounded research inputs and cases: `research/`

For active work, prefer `README.md` → recent commits → the exact source-family code/tests → the smallest relevant research case. Do not begin with a broad historical or cross-project scan unless the question requires it.

## Instruction chain

This repository is the independent implementation home for `project:vgm-tooling`, tracked by Helix.

Before substantive work, read:

1. `dissonance-git/Helix/AGENTS.md` for applicable shared operating law;
2. this file for VGM Tooling-specific law.

Peer-project instructions are not inherited globally. Read and apply libaural, Omniphony, Sonic 3, or another project's local instructions only when work actually crosses that project boundary.

Direct user instruction or correction outranks the chain.

## Scope

VGM Tooling owns executable understanding, analysis, and source-native rendering of digital game music.

`VGM` in the project name is historical shorthand for video game music tooling. The project is not limited to the `.vgm` format and is not defined by foobar2000.

The repository may own:

- format/container parsers;
- authored symbolic inputs such as MML;
- driver/sequence models;
- device/chip models;
- reference and enhanced synthesis;
- source/performance/device state;
- provenance-aware musical analysis;
- deterministic fixtures;
- consumer bridges and frontends.

The foobar2000 SPC and VGM/VGZ components are important realtime applications, not the project ontology.

## Project boundaries

Keep connected projects independent.

```text
Helix
  shared research / evidence / project continuity

VGM Tooling
  executable game-music source / driver / synthesis understanding

libaural
  general artificial hearing

Omniphony
  general headphone spatial rendering
```

Sonic 3 Music Attribution is a bounded VGM Tooling subproject/case in Helix because its technical questions depend directly on SMPS, YM2612, PSG, DAC, prototype/final and arrangement evidence.

Do not copy another project's implementation into this repository merely to make a relationship visible.

## Core execution law

Different source formats require different ingestion and execution machinery, but higher-level musical reasoning should not require a separate ontology for every codec, driver, chip or platform.

```text
source-specific representation
        ↓
source-specific parser / compiler / executor
        ↓
exact execution and synthesis state
        ↓
common musical execution model
        ↓
reasoning / rendering / libaural / forensics
```

Normalize meaning **after execution**, not the source before execution.

Do not convert everything to MIDI, PCM, stems, or another lowest-common-denominator representation as the canonical model.

Keep source-specific evidence attached so higher reasoning can descend to the exact bytes, addresses, commands, registers, samples, driver events, MML commands, or device state when needed.

See `docs/musical-execution-model.md`.

## Levels of truth

Keep these distinct:

```text
authored representation
≠ compiled driver/sequence state
≠ physical device/channel state
≠ rendered audio
≠ perceptual interpretation
≠ downstream application
```

Examples:

- an MML note command may directly prove an authored symbolic event while a VGM-derived note is only reconstructed from device execution;
- YM2612 channel 2 is a physical synthesis lane, not automatically one persistent musical object;
- a GEMS logical note may move between hardware channels while remaining one musical source;
- a VGM register log may prove device state without proving the original driver track or score;
- an SPC may preserve machine state and driver data without making the driver grammar explicit;
- a MIDI note may be one logical event that expands into several synthesis partials inside a module;
- a perceptual stream may fuse several physical sources or split one physical source into several useful components;
- an arranger fingerprint is evidence, not authorship confirmation.

Every higher-level inference needs provenance and confidence appropriate to the layer that supports it.

## Identity law

Do not equate implementation coordinates with musical identity.

```text
musical event / part
        ↓ realizes through
synthesis object / voice
        ↓ occupies
physical execution slot
        ↓ produces
acoustic contribution
        ↓ may become
auditory event / stream
```

These mappings may be one-to-one, one-to-many, many-to-one, or time-varying.

Examples:

- GEMS can reallocate logical notes across FM channels;
- an exact BRR sample may move between SPC SRCN slots across songs;
- an MT-32/LA-synthesis note may use multiple partials;
- one FM voice contains multiple operators;
- several physical sources may perceptually fuse into one stream.

Persistent identity must use the strongest available combination of authored identity, driver-track identity, synthesis-object identity, exact content identity, control continuity, temporal continuity, and provenance.

## Source families

Treat source families according to what evidence they actually retain.

### Authored symbolic / programming representations

Examples: MML dialects and related source languages.

Potentially strong evidence:

- explicit notes/rests/ties;
- length and tempo commands;
- loops/macros/control flow;
- instrument/program selection;
- envelope/modulation/articulation instructions;
- chip-specific synthesis parameters;
- part/channel intent before runtime allocation.

MML is not one universal standardized language. MUCOM88, PMD, FMP, MCK/PPMCK, mml2vgm and other dialects have different semantics and targets. Preserve the dialect and compiler provenance.

### Driver / sequence representations

Examples: SMPS, GEMS, N-SPC, MDX, PMD compiled data and other known driver formats.

Potential evidence:

- logical tracks;
- performance events;
- instrument/sample/patch identity;
- modulation and articulation;
- loops/control flow;
- allocation policy into physical devices.

### Logged execution traces

Examples: VGM/VGZ.

Strong evidence:

- commands present in the log;
- command timing;
- register state;
- embedded data/ROM blocks;
- device configuration represented by the format.

Do not invent original driver-level truth when the log does not retain it. Also preserve capture quality as evidence: incomplete initialization, logging jitter, stripped commands, or transformed traces may make a technically valid VGM an imperfect record of the original execution.

### Executable snapshots / ripped machine states

Examples: SPC, NSF/NSFe, HES, KSS and PSF-family objects where code/state survives.

Potential evidence:

- CPU/program state;
- RAM;
- sound-driver code/data;
- samples/instruments;
- live device state;
- enough machine context to recover semantics by controlled execution.

### Symbolic performance formats

Example: MIDI.

Potential evidence:

- note-on/off;
- velocity;
- pitch bend;
- controllers;
- program/bank state;
- SysEx and device-specific state.

The target synthesizer/module remains part of the musical realization. MIDI alone does not prove the final instrument physics.

### Tracker/module formats

Potential evidence:

- pattern/order structure;
- explicit notes;
- instrument/sample identity;
- channel/effect commands;
- source mixing state.

## Realtime playback law

Normal playback frontends are realtime players, not offline compilers.

Do not require whole-song analysis before playback. Do not require stem export or preparation of an offline master. Do not reverse-compile a VGM into a score as a mandatory playback stage.

Use live source/register/DSP/driver state as playback proceeds.

Small causal state, streaming analysis, and bounded lookahead are permitted where justified.

The broader VGM Tooling project may contain offline or forensic analysis utilities when the research question requires them. Those are analysis tools, not hidden prerequisites for realtime playback.

## Accuracy and enhancement

The accurate renderer is the reference and foundation.

It is not the quality ceiling.

Enhanced mode is allowed to exceed historical hardware limitations in bandwidth, interpolation, synthesis precision, sample reconstruction, dynamics, mixing, spatial presentation, and effects realization when the result remains traceable to the encoded music.

Preserve:

- notes;
- timing;
- groove;
- phrasing;
- instrument identity;
- authored modulation/automation;
- musical hierarchy;
- deliberate effects;
- meaningful hardware coloration that became part of the instrument.

Enhance where justified:

- source reconstruction;
- bandwidth;
- interpolation;
- transient fidelity;
- low-frequency body;
- synthesis quality;
- masking/separation;
- mixing precision;
- source extent;
- environmental rendering;
- stereo presentation.

Do not use “sounds more modern” as sufficient evidence.

## Driver and compiler knowledge matters

Prefer the highest trustworthy source layer available.

```text
MML / authored symbolic event
→ compiled sequence / driver event
→ persistent logical track
→ instrument/sample/patch
→ hardware allocation
→ register/device state
→ acoustic render
```

Not every source preserves the whole route. Each adapter must say which links are exact, derived, or uncertain.

A useful validation pattern is round-trip or paired-direction testing where possible:

```text
authored symbolic source
→ known compiler / driver
→ device execution
→ recovered common musical model
```

The recovered model should preserve the intended musical operation without pretending that all device-specific details are universal.

## Baseline before enhancement

Do not begin audible enhancement work until the relevant source path has a reproducible reference baseline.

For SPC:

1. Preserve/import the editable SNESAPU source.
2. Treat the supplied `spcplay-2.21.3.9130` files as the newest behavioral/version reference.
3. Port the editable source forward until intended behavior matches that reference as closely as can be verified.
4. Only then begin enhancement work.

For VGM:

1. Preserve/import the supplied `foo_input_vgm` source and its MPL-2.0 license.
2. Establish a clean baseline build against the intended libvgm revision.
3. Update dependencies deliberately and verify reference playback before audible replacement.

## Source-domain first

When source-level state exists, prefer acting there instead of repairing the final stereo bus.

Examples:

- improve a PCM voice before the device mixer rather than EQing the whole mix;
- reconstruct a DAC transient at its trigger rather than running a generic transient shaper over stereo;
- render an FM patch from live operator/register state rather than applying a generic exciter afterward;
- preserve dry/effect distinctions when the source exposes them;
- preserve persistent driver identity when hardware-channel allocation changes.

Generic bus EQ/compression/widening is a fallback, not the design center.

## VGM/VGZ priorities

VGM/VGZ is the current realtime trace-format design center.

GYM, DRO, and S98 are legacy compatibility formats. Keep them working when practical, but do not spend research effort on bespoke enhancement unless explicitly requested.

Enhancement should dispatch by active device/source family:

- FM;
- PSG / wavetable;
- PCM / ADPCM / DAC;
- authored spatial DSP such as QSound.

Do not create one universal “VGM enhancer” that ignores chip structure.

## QSound

Native QSound playback must preserve authored QSound behavior.

Separately, QSound may serve as a controlled reference for generalized source-domain stereo rendering and for libaural auditory-scene experiments.

Potentially reusable ideas include:

- per-source positioning;
- source-dependent pan behavior;
- spectral localization cues;
- controlled interchannel phase behavior;
- source extent;
- direct/environment separation;
- per-source effect sends;
- spatial processing before final summation.

Do not run unrelated sources through a literal QSound emulator merely to make them wider.

## Sonic 3 subproject boundary

Sonic 3 Music Attribution is a Helix subproject/case under VGM Tooling.

VGM Tooling may provide:

- SMPS track/command understanding;
- stable logical-track identity;
- YM2612 patch/operator state;
- PSG behavior;
- DAC sample identity and timing;
- prototype/final technical comparison;
- technical realization fingerprints.

It must not silently upgrade technical similarity into composer/arranger confirmation. The Sonic 3 evidence hierarchy remains independent and stricter than implementation resemblance.

## Omniphony boundary

Omniphony owns general headphone spatial rendering.

This repository owns source-native game-audio rendering.

Do not move chip-specific implementation details into Omniphony.

Primary contract:

- enhanced stereo PCM.

Possible later optional side information:

- source multiplicity;
- directness;
- source extent;
- stable motion;
- environmental energy;
- confidence.

Do not send arbitrary chip channel numbers as spatial coordinates. Source state may grant presentation permission; it does not reveal authored 3D truth.

## libaural boundary

libaural is a peer project for general artificial hearing.

VGM Tooling can provide unusually strong answer keys:

```text
known executable source / performance state
→ controlled reference render
→ libaural observes only audio
→ compare inferred auditory organization with the answer key
```

This can test concurrent grouping, sequential grouping, source continuity, pitch crossings, onset synchrony, harmonicity, common modulation, timbre similarity, masking, authored spatial routing, echo/reverb and source count.

Physical-source truth is not identical to perceptual truth. The experiment exists to measure that relationship.

Large learned models, source-separation systems, or expensive research pipelines do not belong in normal realtime playback unless reduced to a small validated mechanism.

## Shared-core rule

Do not prematurely force source families into one abstraction.

A mechanism becomes shared only when multiple source families genuinely need the same abstraction and sharing does not erase useful source-specific information.

Good candidates include:

- exact event timing;
- persistent source IDs;
- provenance/confidence structures;
- high-quality resampling;
- source-aware headroom/mixing primitives;
- diagnostics and A/B capture;
- common musical event/trajectory objects.

BRR reconstruction is SNES-specific. FM operator rendering is FM-specific. QSound register handling is QSound-specific. GEMS allocation behavior is driver-specific. MML dialect grammar is compiler-specific.

## Testing

Every audible change requires a reference-vs-enhanced comparison.

Prefer deterministic fixtures and measurable invariants where possible:

- exact timing preserved;
- loop points preserved;
- note/event sequence unchanged;
- source identity survives legal hardware reallocation;
- channel/device activity preserved;
- no accidental clipping;
- no unstable gain pumping;
- no new discontinuities at loops;
- no stereo collapse;
- no transient smearing from source-aware spatial processing.

For semantic layers, include cross-representation controls where possible:

- MML → compiler/driver → device trace;
- driver sequence → device trace;
- VGM/SPC execution → recovered musical events;
- MIDI → known module → internal partials/audio.

A parser that cannot prove a higher layer must say so rather than inventing it.

Listening tests remain decisive for perceptual quality, but measurements should catch regressions before listening.

Keep known winning listening baselines and make rollback easy.

## Research discipline

Before a substantive new mechanism, inspect relevant literature and mature open-source implementations.

Important recurring sources include:

- chip/device emulators;
- MAME whole-machine/device implementations;
- driver disassemblies and native players;
- MML compilers and source languages;
- VGM Tools / libvgm / VGMTrans;
- Hoot and its driver corpus;
- VGMRips documentation/forum archaeology;
- HCS64;
- sequence/conversion tooling;
- patents where relevant;
- symbolic-music representation research;
- score-informed source separation;
- differentiable/parametric synthesis research;
- auditory-scene-analysis and music-cognition literature.

Record what is borrowed conceptually, what is implemented, what is only a hypothesis, and what licensing prevents direct reuse.

Use established terminology where possible. Do not create product-y names for ordinary DSP, MIR, compiler, emulator, or reverse-engineering concepts.

## Provenance and licensing

Preserve upstream licenses and attribution alongside imported source.

Do not silently relicense imported code.

Keep upstream source provenance documented in `docs/upstreams.md`.

Where licensing prevents direct reuse, treat the implementation as a research/reference source and implement independently only when legally appropriate.

## Documentation naming

Use lowercase kebab-case for documentation filenames.

`README.md` and `AGENTS.md` are the only intentional uppercase documentation names.

Examples:

- `docs/musical-execution-model.md`
- `docs/vgm-frontier.md`
- `docs/research-sources.md`

## Historical lineage

The earlier private `dissonance-git/vgmspc` repository is project ancestry.

Preserve its Git history rather than copying only its final working tree. Current architecture may reject old role heuristics or spatial experiments while retaining the commits as historical evidence.

See `docs/history.md`.

## Repository workflow

Work on `main` unless explicitly instructed otherwise.

Keep commits small enough that an audible, semantic, or architectural change can be reverted independently.

Do not rewrite working upstream code merely to make the tree aesthetically uniform.

For playback paths: establish reference behavior, expose source truth, then enhance.

For semantic/driver/compiler paths: establish exact source evidence, preserve uncertainty, then infer only what the evidence supports.