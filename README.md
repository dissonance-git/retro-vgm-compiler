# foobar2000-game-audio

Realtime, source-native enhanced playback for game-music formats in foobar2000.

This repository develops two separate foobar2000 input components in one shared research and implementation home:

- **SPC**: SNES SPC700 / SNESAPU playback.
- **VGM/VGZ**: chip-log playback built around libvgm.

The components remain separate DLLs. They share enhancement, mixing, spatial, resampling, diagnostics, and validation infrastructure where the underlying mechanism is genuinely common.

## Objective

The accurate renderer is the **reference and foundation, not the ceiling**.

The default enhanced path aims for the highest-quality plausible **realtime realization of the music encoded by the source**, rather than merely reproducing the limitations of the original playback hardware.

The project may remove or reduce limitations imposed by sample storage, interpolation, DAC precision, bandwidth, channel mixing, aliasing, output filtering, effect hardware, and other historical constraints when doing so makes the encoded musical intent clearer or more expressive.

It must preserve the musical decisions that define the work:

- notes and timing
- rhythm and groove
- phrasing
- instrument identity
- authored modulation and automation
- musical hierarchy
- deliberate effects and coloration
- intentional spatial relationships

A hardware artifact that materially defines an instrument is part of the instrument and may be retained. A hardware limitation is not automatically artistic intent.

## Realtime only

This is a playback engine, not a compiler or offline remastering tool.

```text
source file
   ↓
live decoder / register / DSP state
   ↓
chip- or system-specific enhanced renderer
   ↓
source-aware realtime mix
   ↓
foobar2000 PCM
   ↓
optional Omniphony processing
```

No enhancement stage may require reverse-compiling a song, exporting stems, scanning the whole track before playback, or preparing an offline master. Small causal state and bounded lookahead are allowed where they are justified for realtime audio quality.

## Why source-native enhancement

SPC and VGM contain far more information than the final stereo waveform.

Depending on the source, the player may know live information such as:

- independent voices or chip channels
- pitch and modulation
- envelopes and key state
- FM operators, algorithms, feedback, and register automation
- PCM/ADPCM sample memory and playback state
- authored pan
- echo/effect routing
- DAC streams
- loop state
- device and clock identity

The project should exploit that information **before it is collapsed into stereo** whenever possible.

A generic post-EQ, bus compressor, stereo widener, or AI remaster after emulation is not the design center.

## Components

### SPC

The SPC path uses SNESAPU as its editable rendering foundation.

The first obligation is to reconcile the editable SNESAPU source with the newest known SPCPlay/SNESAPU behavior before adding enhancement work. The supplied `spcplay-2.21.3.9130` build is a behavioral/version reference, not the foobar component source.

After parity is established, the enhancement frontier includes:

- BRR/sample reconstruction
- improved interpolation and high-rate rendering
- loop/sustain reconstruction
- transient and low-frequency restoration
- per-voice de-masking
- source-aware mixing
- dry/echo separation
- higher-quality realization of authored SNES echo intent
- source-aware spatial presentation

The original eight-voice and DSP structure should remain available to the realtime enhancement layer rather than being discarded at the stereo boundary.

### VGM/VGZ

VGM/VGZ is the design center of the VGM component.

Legacy GYM, DRO, and S98 support may remain for compatibility, but they are not enhancement priorities.

Enhancement is selected by **active chip/source type**, not by one generic VGM effect:

- FM: high-quality synthesis from live register state, preserving patch identity
- PSG/wavetable: clean band-limited or high-resolution realization of authored oscillators/waves
- PCM/ADPCM/DAC: sample-aware reconstruction, interpolation, transient/body restoration, and high-precision mixing
- authored spatial processors such as QSound: preserve their intended behavior while using them as research references for stronger generalized source-domain spatial rendering

## QSound

QSound is important in two ways.

1. Native QSound VGM playback must preserve the authored QSound DSP behavior.
2. Its deeper architecture can inform a **generalized per-source stereo presentation layer** for other VGM chips.

The goal is not to run every VGM through a literal QSound emulator. The goal is to study and generalize useful ideas such as per-source spatial positioning, source-dependent pan laws, spectral/phase localization cues, direct/environment separation, and effect sends before final summation.

That generalized layer can create a much stronger stereo substrate for Omniphony than ordinary console stereo.

## Omniphony relationship

This repository owns source-native game-audio reconstruction and rendering.

Omniphony remains a general headphone spatial renderer and must not absorb SNES, YM2612, BRR, QSound-register, or other chip-specific implementation logic.

The normal contract is enhanced PCM. A later optional bridge may expose compact perceptual evidence such as source multiplicity, directness, source extent, stable motion, and environmental energy. Omniphony decides the 3D presentation.

```text
game-audio source
      ↓
source-native enhanced renderer
      ↓
source-aware stereo master
      ↓
Omniphony
      ↓
full-sphere headphone presentation
```

## libaural relationship

libaural may use the same chip/source state as unusually strong ground truth for artificial-hearing research. Large learned or research systems do not belong in the realtime playback path unless a small validated mechanism is distilled from them.

## Repository shape

```text
components/
  spc/
  vgm/
core/
  realtime/
  enhancement/
  mixing/
  spatial/
  resampling/
  diagnostics/
systems/
  snes/
  fm/
  psg/
  pcm/
  qsound/
tests/
docs/
```

The tree will grow from the imported upstream component sources rather than forcing them into this shape immediately. Refactor only after baseline builds and behavior are captured.

## Initial milestones

1. Import the two existing foobar component source trees with provenance and licenses intact.
2. Establish reproducible baseline builds.
3. Bring the editable SNESAPU source forward to the supplied `2.21.3.9130` behavior and verify parity.
4. Update libvgm and the VGM component baseline without changing audible behavior.
5. Expose stable per-source/per-channel realtime state at the renderer boundary.
6. Add chip-specific enhanced renderers one mechanism at a time with reference-vs-enhanced A/B tests.
7. Add a shared source-aware realtime mixer only after repeated mechanisms justify sharing.
8. Generalize useful QSound spatial principles without imposing QSound coloration on unrelated systems.
9. Feed the resulting enhanced stereo into Omniphony, then evaluate a compact optional source-evidence bridge.

## Validation law

Every audible enhancement must be compared against the accurate/reference render.

A change survives when it makes the encoded music more intelligible, powerful, spacious, coherent, or natural without erasing musical identity. If an enhancement sounds impressive but changes the composition, groove, characteristic instrument identity, or intended effect behavior, it fails.

The long-term target is simple:

> Every supported soundtrack should aim to sound like the highest-quality realization its original musical data can support.
