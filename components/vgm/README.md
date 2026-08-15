# VGM component

This directory is the development home for the VGM/VGZ foobar2000 path and the current cross-chip source-state/enhancement core.

The imported upstream wrapper remains the reference playback foundation. Project-owned analysis and enhanced-rendering work must stay traceable to exact VGM command/device state and must not silently replace reference behavior.

For full engineering status, see `../../docs/vgm-frontier.md`. For the common semantic model, see `../../docs/musical-execution-model.md`. For the durable enhancement target, see `../../docs/source-native-enhanced-rendering.md`.

## Current execution path

The project-owned path is becoming:

```text
VGM command stream
        ↓
format/version semantics
        ↓
allocation-free ordered command capture
        ↓
chip-family-specific device transitions
        ↓
rebuildable device state
        ↓
bounded physical voice episodes
        ↓
conservative pitch/control observations
        ↓
common musical coordinates only where independently earned
        ↓
higher musical identity / structure only when stronger evidence supports it
```

The source trace remains canonical evidence. Replayed device state is a rebuildable view, not a replacement for ordered transition history.

Capture gaps, truncated payloads and stateful-command ambiguity fail closed rather than fabricating later semantic continuity.

The VGM specification is the authority for what the format bytes mean. Chip manuals, drivers and mature emulator implementations answer what the addressed hardware does with them.

## Cross-chip VGM floor

The current format-level core includes:

- version-aware VGM clock semantics;
- dual-chip and selected variant flags;
- VGM 1.70 distinct second-instance clocks and extra-header volume metadata;
- EOF and GD3 structural validation;
- classified data-block families;
- Yamaha register-write transport for `0x51-0x5F` and `0xA1-0xAF`;
- explicit exclusion of `0xA0`, which remains AY8910;
- generic DAC Stream Control decoding for `0x90-0x95`;
- strict reserved-field and 256-opcode-width controls;
- the complete currently defined VGM 1.72 beta Mikey delta under the project shorthand **VGM 1.72d**: Mikey clock, `0x40` register writes and Mikey PCM data blocks;
- spec-driven corpus timing and loop validation in `tools/vgm_corpus_audit.py`.

`1.72d` is a project label, not an upstream VGM version claim. It maps to the currently defined upstream 1.72 beta surface so provenance remains explicit.

These are transport/format facts, not a universal chip state model.

## Yamaha family boundaries now represented

The first cross-chip comparison deliberately keeps several branches independent.

```text
OPN
YM2203 • YM2608 • YM2610/B • YM2612/3438

OPM
YM2151/2164

OPL
YM3526 • Y8950 • YM3812 • YMF262

OPLL
YM2413-class preset/user-instrument devices
```

OPN shares key/register/FNUM mechanics internally. OPM uses key code + key fraction. OPL uses a different connection/feedback packing and two-operator baseline, with OPL3 able to pair selected channels dynamically into four-operator voices. OPLL adds preset/user-patch provenance.

A small four-operator invariant was earned across OPN and OPM, then explicitly stopped there because OPL falsifies it as universal Yamaha behavior.

The pitch layer follows the same rule: family-specific coordinates may derive the same nominal frequency without being collapsed into one native pitch encoding or a MIDI note.

## Genesis state currently represented

YM2612 coverage includes register state, operator/key masks, F-number/block semantics, algorithm/feedback, routing, AMS/FMS/LFO, channel-3 special-mode state, operator parameters, DAC enable and resolved DAC source activity.

SN76489-family state includes tone periods, attenuation, noise state/control and stereo routing where available.

The current semantic adapters deliberately distinguish:

```text
device transition
!= physical voice episode
!= musical note
!= persistent part
```

A physical channel is an execution coordinate, not a permanent musical identity.

## QSound spatial source state

QSound is the first VGM family in this project where the historical device itself provides a rich source-spatial grammar before final stereo collapse.

Independent mature implementations agree on the important topology:

```text
16 PCM voices + 3 ADPCM voices
        ↓
per-source pan state
        ↓
per-source dry L/R + wet L/R coefficients
        ↓
shared echo / FIR / delay / output stages
        ↓
final historical stereo
```

The project now preserves a QSound-specific four-way source route rather than reducing the device to one pan scalar. For the decoded pan regions:

- `0x110..0x130` is the 33-position QSound spatial table with distinct direct/dry and filtered/wet coefficients;
- `0x140..0x160` is the recovered dry-only linear-pan table;
- unknown/gap pan words remain raw and undecoded rather than inheriting libvgm's defensive clamp as source truth.

PCM channels also preserve the signed `0xBA..0xC9` per-channel contribution to the shared echo input. That value is distinct from the per-source wet routing table and is not collapsed into it.

The modified foobar VGM path now observes VGM `0xC4` writes in realtime and maintains a fail-closed sample-timed QSound source-control shadow. Source-facing pan and PCM echo-contribution writes are admitted; global echo feedback, FIR selection, output delays and output component volumes remain renderer state.

This is currently **control/evidence state, not audible QSound source substitution**. libvgm remains the historical QSound renderer and therefore the listening/reference control.

The next QSound audio milestone is to expose the 19 synthesized source outputs before the QSound dry/wet pan tables, preserve the shared environmental path separately, and feed those causal lanes to Omniphony. QSound is a historical spatial baseline and calibration family, not a ceiling on the modern binaural presentation.

No QSound pan value is promoted to authored 3-D coordinates. Device-authored routing constrains presentation; Omniphony owns the final headphone scene.

## Enhanced source engines

Project-owned source-domain work currently includes:

- `sn76489_enhanced` for isolated high-quality PSG tone/noise stems;
- `ym2612_dac_enhanced` for classic YM2612 DAC playback;
- `ym2612_pcm_stream` for the current YM2612 sink of VGM source-bank streams;
- exact YM2612 register timeline capture and an isolated six-channel FM backend contract;
- explicit authored stereo routing and high-precision source summation primitives;
- QSound 19-source spatial-control evidence with four-way dry/wet routing and PCM echo contribution preserved before source-audio tapping.

The VGM `0x90-0x95` stream transport itself is now decoded generically so future chip-specific PCM/ADPCM sinks do not each invent private format semantics.

These cores are not evidence that foobar playback has already switched to an audible enhanced renderer. Reference playback, source-state infrastructure, testable enhancement cores and retained listening wins are separate evidence states.

## Current FM frontier

The next major audible Genesis milestone remains a mature six-channel YM2612 renderer with:

- exact patch/operator semantics;
- writes preserved in absolute VGM source ticks;
- native FM clock scheduling inside the backend;
- isolated channel output before final stereo summation;
- high-quality phase-coherent output-rate conversion;
- explicit algorithmic latency;
- reference-comparable behavior;
- reversible substitution behind an experimental path.

The caller must not quantize VGM write ticks to output frames before the backend sees them.

Do not use a simplistic approximation merely to make sound sooner, and do not instantiate six unrelated FM emulators just to obtain six stems. Global/LFO/phase state belongs to one coherent engine.

Only after a mature source-faithful FM path is validated should selected hardware limitations be relaxed experimentally.

The relaxed path is **not** a generic modern-synth replacement. Its target is the best plausible descendant of the same programmed FM instrument:

```text
same algorithm / operator relationships
same patch parameters
same envelopes / modulation / feedback
same notes / timing / articulation
        ↓
selected non-identity-bearing hardware ceilings relaxed
        ↓
higher-fidelity FM realization
```

Candidate ceilings include numerical precision, reconstruction bandwidth, avoidable aliasing/imaging, DAC realization and final summation precision. Each must be tested separately.

A chip-specific artifact that materially defines the patch remains part of the instrument until evidence and listening show a safe higher-quality equivalent.

## Real-corpus control

The immutable Sonic 3 & Knuckles corpus remains the first byte-verified VGM control.

The generic VGM corpus audit currently validates:

```text
58 / 58 structural parses
58 / 58 total-sample declarations
57 / 57 declared loop boundaries and loop durations
```

Additional YM2203/YM2608/YM2151/YM2413/YM3812/YMF262 sets should be admitted through the same structural path before they are allowed to pressure-test chip-specific state or higher musical inference.

QSound now needs its own small real-control set. High-value tracks should exercise the normal spatial pan table, the linear-pan region where it is actually used, moving pan, PCM echo contribution and quiet/centered controls. Derived evidence may be stored; copyrighted game data must not be added to the repository.

More files are useful only when they add independent information.

## Historical constraint evidence

The project distinguishes:

```text
unwanted production burden
!= storage/quality compromise
!= adapted limitation
!= deliberately adopted artifact
```

Creator testimony shows all four cases existed.

Examples include composers describing manual sound-data entry and low-level programming as burdens, material being shortened or stripped to fit memory, later recordings moving closer to original intentions, and other composers deliberately exploiting chip/sample artifacts because they became musically useful.

This evidence controls enhancement permission; it does not become a platform-wide rule.

See `../../research/historical-constraint-friction-counterfactual-rendering.md`.

## Musical-analysis relationship

The VGM path now contributes more than chip telemetry. It can provide exact lower evidence for higher questions about programmed expression, persistent musical identity, texture, parts, harmony and whole-song structure.

But a register trace usually cannot prove the original driver track or composer-facing score. Higher claims remain source-relative and provenance-bearing.

The intended analysis path is therefore:

```text
exact VGM execution
        ↓
family-specific device truth
        ↓
conservative common performance coordinates
        ↓
persistent-part / harmonic / structural hypotheses
        ↓
synchronized listening-level account
```

not:

```text
VGM -> MIDI -> canonical song model
```

## Future preservation tooling

The spec-driven structural audit makes future state-aware assistance with loop validation and VGM set preparation plausible.

The current implementation validates declared loops; it does not discover or rewrite them.

A mature loop analysis should eventually compare executable state, not just waveform similarity, so hidden chip/stream/modulation state cannot silently diverge across a musically convincing repeat.

## Realtime law

Normal foobar2000 playback remains realtime. Do not require offline song compilation, stem export or whole-song analysis before audio begins.

Offline/forensic/song-level analysis may operate over captured evidence separately.

## Legacy handlers

The imported wrapper also contains GYM, DRO and S98 compatibility paths. Keep them working where practical, but VGM/VGZ remains the active trace-format design center unless explicitly redirected.
