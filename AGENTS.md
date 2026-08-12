# AGENTS.md

## Scope

This repository develops realtime foobar2000 playback components for game-music source formats, beginning with SPC and VGM/VGZ.

The two input components remain separate build products. Shared code is earned by repeated mechanisms, not forced by architecture diagrams.

## Non-negotiable playback law

This is a **realtime player**, not an offline compiler, stem extractor, reverse-compiler, or pre-rendering pipeline.

Do not require whole-song analysis before playback. Do not export intermediate stems as part of normal operation. Do not convert VGM/SPC into an internal score and then play the score later. Use the live source/register/DSP state directly as playback proceeds.

Small causal state, streaming analysis, and bounded lookahead are permitted when they are justified by audible benefit and realtime constraints.

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

## Baseline before enhancement

Do not begin audible enhancement work until the relevant source path has a reproducible baseline.

For SPC:

1. Preserve/import the editable SNESAPU source.
2. Treat the supplied `spcplay-2.21.3.9130` files as the newest behavioral/version reference.
3. Port the editable source forward until its intended behavior matches that reference as closely as can be verified.
4. Only then begin enhancement work.

For VGM:

1. Preserve/import the supplied `foo_input_vgm` source and its MPL-2.0 license.
2. Establish a clean baseline build against the intended libvgm revision.
3. Update dependencies deliberately and verify reference playback before enhancement work.

## Source-domain first

When source-level state exists, prefer acting there instead of repairing the final stereo bus.

Examples:

- improve a PCM voice before the device mixer rather than EQing the whole mix
- reconstruct a DAC transient at its trigger rather than running a generic transient shaper over stereo
- render an FM patch from its live operator/register state rather than applying a generic exciter afterward
- treat authored echo/effect energy separately when the source exposes the distinction

Generic bus EQ/compression/widening is a fallback, not the design center.

## VGM priorities

VGM/VGZ is the active design center.

GYM, DRO, and S98 are legacy compatibility formats. Keep them working when practical, but do not spend research effort on bespoke enhancement for them unless explicitly requested.

Enhancement should dispatch primarily by active device/source family:

- FM
- PSG / wavetable
- PCM / ADPCM / DAC
- authored spatial DSP such as QSound

Do not create one universal “VGM enhancer” that ignores chip structure.

## QSound

Native QSound playback must preserve authored QSound behavior.

Separately, QSound may serve as a reference for generalized source-domain stereo rendering. Generalize mechanisms, not branding or coloration.

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

libaural may use chip/source state as research ground truth.

Large learned models, source-separation systems, or expensive research pipelines do not belong in normal realtime playback unless they have been reduced to a small validated mechanism.

## Shared-core rule

Do not prematurely move code into `core/` or a shared library.

A mechanism becomes shared only when at least two source families genuinely need the same abstraction and sharing does not erase useful source-specific information.

Good candidates may eventually include:

- realtime state smoothing
- high-quality resampling
- source-aware headroom/mixing primitives
- masking metrics
- bounded transient/body controls
- generalized spatial extent primitives
- diagnostics and A/B capture

BRR reconstruction is SNES-specific. FM operator rendering is FM-specific. QSound register handling is QSound-specific.

## Testing

Every audible change requires a reference-vs-enhanced comparison.

Prefer deterministic fixtures and measurable invariants where possible:

- exact timing preserved
- loop points preserved
- note/event sequence unchanged
- channel activity preserved
- no accidental clipping
- no unstable gain pumping
- no new discontinuities at loops
- no stereo collapse
- no transient smearing from source-aware spatial processing

Listening tests remain decisive for perceptual quality, but measurements should catch regressions before listening.

Keep known winning listening baselines and make rollback easy.

## Research discipline

Before a substantive new audible mechanism, inspect relevant literature and mature open-source implementations.

Record what is being borrowed conceptually, what is being implemented, and what is merely a hypothesis.

Use established terminology where possible. Do not create product-y names for ordinary DSP concepts.

## Provenance and licensing

Preserve upstream licenses and attribution alongside imported source.

Do not silently relicense imported code.

Keep upstream source provenance documented in `docs/UPSTREAMS.md`.

Where licensing prevents direct reuse, treat the implementation as a research/reference source and implement independently only when legally appropriate.

## Repository workflow

Work on `main` unless explicitly instructed otherwise.

Keep commits small enough that an audible or architectural change can be reverted independently.

Do not rewrite working upstream code merely to make the tree aesthetically uniform.

First make it build. Then establish behavioral parity. Then expose source state. Then enhance.
