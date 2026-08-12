# AGENTS.md

## Helix relationship

This repository is the independent implementation home for `project:vgm-tooling`, which is tracked by Helix for project continuity, research, evidence, relationships, and re-entry.

Before substantive work, read the current Helix operating law at `dissonance-git/Helix/AGENTS.md` and apply the parts relevant to evidence, provenance, correction, validation, re-entry, and concurrent repository safety. This file owns VGM Tooling's project-specific implementation laws and may specialize repository workflow where the project genuinely differs. Direct user instruction or correction outranks both.

Do not copy VGM Tooling's implementation into Helix merely to make the connection visible, and do not copy Helix machinery into this repository merely to inherit its design. Helix preserves the exact route to this project; this repository remains canonical for its code, tests, builds, local implementation history, and releases.

## Scope

This repository is the implementation home for **VGM Tooling**: executable understanding, analysis, and source-native rendering of digital game music.

`VGM` in the project name is historical shorthand for video game music tooling. The project is not limited to the `.vgm` format and is not defined by foobar2000.

The repository may own:

- format/container parsers
- driver/sequence models
- device/chip models
- reference and enhanced synthesis
- source/performance/device state
- provenance-aware analysis
- deterministic fixtures
- consumer bridges and frontends

The current foobar2000 SPC and VGM/VGZ components remain important realtime applications, not the project ontology.

## Project boundary

This repository owns executable game-music machinery and local implementation history.

Helix owns broader project orientation, research control, exact evidence, cross-project relations, negative results, and re-entry state.

libaural owns general artificial hearing.

Omniphony owns general headphone spatial rendering.

Do not solve boundaries by copying every related project into this repository.

```text
Helix
  ↓ research / evidence / project state
VGM Tooling
  ↓ source truth / executable synthesis
├─ foobar2000 frontends
├─ libaural experiments
├─ attribution/forensic subprojects
└─ Omniphony input
```

## Levels of truth

Keep these distinct:

```text
format/container
≠ driver/performance state
≠ physical device/channel state
≠ rendered audio
≠ perceptual interpretation
≠ downstream application
```

Examples:

- YM2612 channel 2 is a physical synthesis lane, not automatically one persistent musical object.
- A GEMS logical note may move between hardware channels while remaining one musical source.
- A VGM register log may prove device state without proving the original driver track or score.
- A perceptual stream may fuse several physical sources or split one physical source into several useful components.
- An arranger fingerprint is evidence, not authorship confirmation.

Every higher-level inference needs provenance and confidence appropriate to the layer that supports it.

## Realtime playback law

Normal playback frontends are realtime players, not offline compilers.

Do not require whole-song analysis before playback. Do not require stem export or preparation of an offline master. Do not reverse-compile a VGM into a score as a mandatory playback stage.

Use live source/register/DSP/driver state as playback proceeds.

Small causal state, streaming analysis, and bounded lookahead are permitted where justified.

The broader VGM Tooling project may still contain offline or forensic analysis utilities when the research question requires them. Those are analysis tools, not hidden prerequisites for realtime playback.

## Accuracy and enhancement

The accurate renderer is the reference and foundation.

It is not the quality ceiling.

Enhanced mode is allowed to exceed historical hardware limitations in bandwidth, interpolation, synthesis precision, sample reconstruction, dynamics, mixing, spatial presentation, and effects realization when the result remains traceable to the encoded music.

Preserve:

- notes
- timing
- groove
- phrasing
- instrument identity
- authored modulation/automation
- musical hierarchy
- deliberate effects
- meaningful hardware coloration that became part of the instrument

Enhance where justified:

- source reconstruction
- bandwidth
- interpolation
- transient fidelity
- low-frequency body
- synthesis quality
- masking/separation
- mixing precision
- source extent
- environmental rendering
- stereo presentation

Do not use “sounds more modern” as sufficient evidence.

## Driver knowledge matters

Prefer the highest trustworthy source layer available.

A register log is valuable, but driver semantics can provide stronger musical identity:

```text
sequence event
→ persistent driver track
→ instrument/sample/patch
→ hardware allocation
→ register/device state
→ acoustic render
```

SMPS, GEMS, N-SPC and other known drivers may therefore have dedicated adapters/models.

Do not invent driver-level truth when the source only proves register-level state.

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

- improve a PCM voice before the device mixer rather than EQing the whole mix
- reconstruct a DAC transient at its trigger rather than running a generic transient shaper over stereo
- render an FM patch from live operator/register state rather than applying a generic exciter afterward
- preserve dry/effect distinctions when the source exposes them
- preserve persistent driver identity when hardware-channel allocation changes

Generic bus EQ/compression/widening is a fallback, not the design center.

## VGM/VGZ priorities

VGM/VGZ is the current realtime format design center.

GYM, DRO, and S98 are legacy compatibility formats. Keep them working when practical, but do not spend research effort on bespoke enhancement unless explicitly requested.

Enhancement should dispatch by active device/source family:

- FM
- PSG / wavetable
- PCM / ADPCM / DAC
- authored spatial DSP such as QSound

Do not create one universal “VGM enhancer” that ignores chip structure.

## QSound

Native QSound playback must preserve authored QSound behavior.

Separately, QSound may serve as a controlled reference for generalized source-domain stereo rendering and for libaural auditory-scene experiments.

Potentially reusable ideas include:

- per-source positioning
- source-dependent pan behavior
- spectral localization cues
- controlled interchannel phase behavior
- source extent
- direct/environment separation
- per-source effect sends
- spatial processing before final summation

Do not run unrelated sources through a literal QSound emulator merely to make them wider.

## Sonic 3 subproject boundary

Sonic 3 Music Attribution is a Helix subproject/case under VGM Tooling.

VGM Tooling may provide:

- SMPS track/command understanding
- stable logical-track identity
- YM2612 patch/operator state
- PSG behavior
- DAC sample identity and timing
- prototype/final technical comparison
- technical realization fingerprints

It must not silently upgrade technical similarity into composer/arranger confirmation. The Sonic 3 evidence hierarchy remains independent and stricter than implementation resemblance.

## Omniphony boundary

Omniphony owns general headphone spatial rendering.

This repository owns source-native game-audio rendering.

Do not move chip-specific implementation details into Omniphony.

Primary contract:

- enhanced stereo PCM

Possible later optional side information:

- source multiplicity
- directness
- source extent
- stable motion
- environmental energy
- confidence

Do not send arbitrary chip channel numbers as spatial coordinates. Source state may grant presentation permission; it does not reveal authored 3D truth.

## libaural boundary

libaural may use driver/device/source state as ground truth for artificial-hearing research.

The ideal experiment structure is:

```text
known executable source state
→ controlled reference render
→ libaural observes only audio
→ compare inferred auditory organization with the answer key
```

Large learned models, source-separation systems, or expensive research pipelines do not belong in normal realtime playback unless reduced to a small validated mechanism.

## Shared-core rule

Do not prematurely force source families into one abstraction.

A mechanism becomes shared only when at least two source families genuinely need the same abstraction and sharing does not erase useful source-specific information.

Good candidates may include:

- exact event timing
- persistent source IDs
- provenance/confidence structures
- high-quality resampling
- source-aware headroom/mixing primitives
- diagnostics and A/B capture

BRR reconstruction is SNES-specific. FM operator rendering is FM-specific. QSound register handling is QSound-specific. GEMS allocation behavior is driver-specific.

## Testing

Every audible change requires a reference-vs-enhanced comparison.

Prefer deterministic fixtures and measurable invariants where possible:

- exact timing preserved
- loop points preserved
- note/event sequence unchanged
- source identity survives legal hardware reallocation
- channel/device activity preserved
- no accidental clipping
- no unstable gain pumping
- no new discontinuities at loops
- no stereo collapse
- no transient smearing from source-aware spatial processing

For analysis/semantic layers, include negative controls and confidence boundaries. A parser that cannot prove a higher layer must say so rather than inventing it.

Listening tests remain decisive for perceptual quality, but measurements should catch regressions before listening.

Keep known winning listening baselines and make rollback easy.

## Research discipline

Before a substantive new mechanism, inspect relevant literature and mature open-source implementations.

Important recurring sources include chip/device emulators, driver disassemblies, VGMRips documentation/forum archaeology, HCS64, VGMTrans, sequence/conversion tooling, patents where relevant, and auditory-science/MIR literature.

Record what is borrowed conceptually, what is implemented, what is only a hypothesis, and what licensing prevents direct reuse.

Use established terminology where possible. Do not create product-y names for ordinary DSP or reverse-engineering concepts.

## Provenance and licensing

Preserve upstream licenses and attribution alongside imported source.

Do not silently relicense imported code.

Keep upstream source provenance documented in `docs/UPSTREAMS.md`.

Where licensing prevents direct reuse, treat the implementation as a research/reference source and implement independently only when legally appropriate.

## Historical lineage

The earlier private `dissonance-git/vgmspc` repository is project ancestry.

Preserve its Git history rather than copying only its final working tree. Current architecture may reject old role heuristics or spatial experiments while retaining the commits as historical evidence.

See `docs/HISTORY.md`.

## Repository workflow

Work on `main` unless explicitly instructed otherwise.

Keep commits small enough that an audible, semantic, or architectural change can be reverted independently.

Do not rewrite working upstream code merely to make the tree aesthetically uniform.

For playback paths: establish reference behavior, expose source truth, then enhance.

For semantic/driver paths: establish exact source evidence, preserve uncertainty, then infer only what the evidence supports.