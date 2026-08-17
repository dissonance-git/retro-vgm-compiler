# Source-aware host rendering evidence

## Status

Research synthesis for realtime foobar/VGM/SPC source transport, source-aware rendering, and later Omniphony projection.

This note records the evidence that currently constrains the implementation. It does **not** promote historical testimony, emulator labels, or academic spatial models above exact runtime observations.

## Central rule

```text
runtime/device evidence
> implementation prior art
> composer/production testimony
> perceptual rendering model
```

Each layer answers a different question.

- Runtime/device evidence says what the emulated machine actually did.
- Implementation prior art can reveal useful observation/tap points.
- Composer testimony can show that routing/effects/channel deployment were often intentional compositional decisions.
- Spatial-audio research constrains how those preserved facts may later be projected for headphone listening.

The upper layers may guide interpretation and rendering. They may not rewrite the lower layers.

## 1. SPCPresenter confirms that S-DSP state can be observed live

Pinned prior-art implementation:

- https://github.com/nununoisy/spc-presenter-rs
- bundled fork: `external/snes-apu-spcp`
- inspected files:
  - `src/dsp/voice.rs`
  - `src/dsp/dsp.rs`

SPCPresenter uses a custom `snes-apu` fork and exposes detailed per-frame S-DSP state. Its voice implementation preserves signed per-channel volume, source index, envelope state, pitch, echo enable, pitch modulation, and per-channel amplitude.

This validates the basic feasibility of a realtime semantic sidecar without requiring blind separation from the final stereo mixture.

It does **not** by itself establish our source identity model, host timeline semantics, or a scientifically valid stem decomposition.

## 2. The strongest current SPC dry-tap candidate is before voice pan/echo mixing

The inspected fork makes the relevant synthesis boundary unusually clear.

In `voice3c()` it computes `l_output` from the current voice after:

```text
BRR decode / noise source
-> interpolation
-> envelope
-> pitch-modulation-dependent pitch trajectory
-> mono synthesized voice sample (`l_output`)
```

Then `voice4()` and `voice5()` call `output()` for left and right. `output()` multiplies that mono signal by the signed native voice volume for the selected side, accumulates it into the master mix, and also accumulates it into the shared echo input when the voice's echo-send state is enabled.

Therefore the leading candidate for an isolated SPC dry lane is:

```text
after voice3c()
before voice4()/voice5()
```

This is stronger than mute/subtract extraction because it observes a causal internal signal before downstream route mixing.

### Why this boundary matters

It preserves:

- BRR/noise realization;
- interpolation behavior;
- envelope behavior;
- pitch-modulation consequences on the generated voice;
- physical-voice episode timing.

It does not bake in:

- signed L/R route gains;
- master output volume;
- shared echo return.

Those remain separate evidence/realization layers.

## 3. Echo send is per voice; echo return is shared state

An S-DSP voice can contribute to the echo input, but the resulting wet signal passes through shared delay/FIR/feedback state.

Therefore:

```text
voice echo-send enabled
!= independently attributable voice wet stem
```

The source contract may preserve exact per-voice send state while still reporting no isolated shared wet-return lane until that bus is explicitly observed.

Do not duplicate the full echo return onto every sending voice.

## 4. Physical voice, sample source, and musical part are different identities

Existing repository runtime semantics already establish that an SPC `SRCN`/sample change can occur inside one physical voice episode.

The host source model therefore uses:

```text
physical S-DSP voice slot
+ episode generation
```

for realtime source identity.

A `source_latched`/SRCN change is evidence inside that episode. It does not automatically create a new renderer source, note, instrument, or persistent musical part.

Likewise, a new accepted key-on creates a new physical-voice generation and must become a hard host-transport boundary if it occurs inside a larger decode request.

## 5. Composer testimony says native mix decisions are compositionally meaningful

These sources are historical/interpretive evidence, not machine-state authority.

### Hiroki Kikuta — Secret of Mana

Interview:

- https://vgmonline.net/hirokikikutainterview/

Kikuta describes spending much of the SNES development effort sampling timbres and adjusting the pan, volume, and effects of each of the eight sound channels, alongside manual data compression.

Consequence:

```text
native pan / volume / effect deployment
may be part of the authored arrangement and mix
```

A remastering renderer should preserve those values as evidence rather than treating them as disposable defaults.

### Barry Leitch — Top Gear

Composer interview sources describe his deliberate use of SNES echo at a musically chosen delay while balancing the echo buffer against scarce RAM.

Consequence:

```text
shared effect topology can be an intentional rhythmic/compositional choice
```

The echo network is not merely decoration to be replaced by a generic modern reverb.

### David Wise — SNES / Donkey Kong Country era

Interviews describe the eight-channel and memory constraints and the deliberate integration of environmental/texture material into the available musical resources.

Consequence:

```text
physical channel
!= stable conventional instrument role
```

Channel identity must not be promoted directly to `bass`, `melody`, `drums`, or another musical-part label.

### Yuzo Koshiro — Mega Drive / Streets of Rage

Relevant interviews:

- https://shmuplations.com/yuzokoshiro/
- https://shmuplations.com/sormusic/
- https://vgmonline.net/yuzokoshirointerview/

Koshiro describes writing/altering sound drivers, seeking greater control and precision over chip features than MIDI exposed, and deliberately exploiting the peculiarities of the Mega Drive synthesizer.

Consequence:

```text
chip-level synthesis/routing behavior can be intentional musical material
```

For VGM, register-level and driver-level evidence should remain primary rather than reconstructing a generic MIDI-like performance and discarding device behavior.

## 6. Academic source-separation research supports informed realtime models, but our emulator boundary is stronger

### Soundprism

Zhiyao Duan and Bryan Pardo, 2011:

- `Soundprism: An Online System for Score-Informed Source Separation of Music Audio`
- DOI: https://doi.org/10.1109/JSTSP.2011.2159701

Soundprism demonstrates online source separation guided by structured musical information and framewise score alignment.

### Low-latency separation

Tom Barker, Tuomas Virtanen, Niels Henrik Pontoppidan, 2015:

- `Low-latency sound-source-separation using non-negative matrix factorisation with coupled analysis and synthesis dictionaries`
- DOI: https://doi.org/10.1109/ICASSP.2015.7177968

Paul Magron and Tuomas Virtanen, 2020:

- `Online Spectrogram Inversion for Low-Latency Audio Source Separation`
- DOI: https://doi.org/10.1109/LSP.2020.2970310

Satvik Venkatesh et al., 2024:

- `Real-Time Low-Latency Music Source Separation Using Hybrid Spectrogram-TasNet`
- DOI: https://doi.org/10.1109/ICASSP48485.2024.10448381

These works show that low-latency source separation is possible, but they infer sources from acoustic mixtures under uncertainty.

Game Music Interpreter often has a more informative boundary:

```text
instrumented emulator internal source state
```

When an exact causal signal exists there, prefer observing it to estimating the same signal from the final stereo mix.

Mixture-based source separation remains useful as a fallback, validator, or cross-check where internal source observability is unavailable.

## 7. Spatial-audio research says wet/dry and binaural relationships matter perceptually

Taeboo Choe and Jung-Woo Choi, 2023:

- `Impact of direct-to-reverberation ratio and interaural cross-correlation on externalization in binaural audio rendering`
- DOI: https://doi.org/10.1121/10.0023095

Song Li, Roman Schlieper, and Jürgen Peissig, 2019:

- `The Role of Reverberation and Magnitude Spectra of Direct Parts in Contralateral and Ipsilateral Ear Signals on Perceived Externalization`
- DOI: https://doi.org/10.3390/app9030460

These results reinforce a renderer boundary already implied by the device evidence:

```text
preserve native source routing + shared-effect relationships
-> then perform binaural/externalization projection
```

Do not flatten away wet/dry or L/R evidence and hope to recreate a perceptually equivalent scene later from source labels alone.

## 8. Host-transport consequences

The realtime host path should preserve all of the following independently:

```text
reference playback sample timeline
physical source episode identity
source PCM availability
native signed routing state
shared-effect send state
shared wet-return availability
ordered in-block evidence changes
playback discontinuity epoch
```

Required failure behavior:

- missing dry PCM remains unavailable, never fabricated as observed zero;
- capture overflow becomes a visible continuity failure;
- trace-index gaps require reset/recovery rather than silent stitching;
- accepted key-on changes generation and creates a hard source-identity boundary;
- seek/flush/track change discards buffered semantic tail and starts a new playback epoch;
- an event exactly at a decode-window end belongs to the next window's state;
- composer testimony never overrides a contradictory runtime observation;
- inferred renderer geometry never becomes authored 3-D provenance.

## 9. Current implementation mapping

Shared host transport:

- `model/spatial_source_host_assembler.h`
- `model/spatial_source_host_session.h`

SPC runtime projection:

- `components/spc/spc_runtime_capture.h`
- `components/spc/spc_runtime_spatial_adapter.h`

Regression surface:

- `tests/model/spatial_source_host_assembler_test.cpp`
- `tests/model/spatial_source_host_session_test.cpp`
- `tests/spc/spc_runtime_spatial_adapter_test.cpp`
- `cmake/host_transport_tests.cmake`

The SPC spatial adapter intentionally remains **evidence-only** for audio today. The next source-rendering frontier is a validated `voice3c -> pre-pan` mono tap plus a separately observed shared echo return.

## Stop conditions

Stop rather than overclaim if:

- SRCN is treated as permanent voice or musical-part identity;
- a key-on generation change is hidden inside a same-identity evidence event;
- signed S-DSP L/R values are collapsed to absolute pan balance;
- the shared echo return is copied into each sending voice;
- missing source PCM is represented as observed silence;
- mixture-based AI separation replaces an exact emulator signal that is already observable;
- composer recollection is used to overwrite contradictory runtime evidence;
- a renderer-inferred position is relabeled as authored geometry.

Correction outranks coherence.
