# Source-aware host rendering evidence

## Status

Research synthesis for realtime foobar/VGM/SPC source transport, source-aware rendering, and Omniphony projection.

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

## 2. The exact SPC dry-source seam is before voice pan/echo mixing

The prior-art SPCPresenter fork first identified the useful conceptual boundary. Direct inspection of upstream `libgme/game-music-emu` then located the corresponding exact seam in `gme/Spc_Dsp.cpp`.

For each physical voice, libgme computes one mono `output` after:

```text
BRR decode / noise source
-> Gaussian interpolation
-> envelope application
-> pitch-modulation consequences
-> mono synthesized voice sample (`output`)
```

Only after that does libgme compute:

```text
l = output * voice_volume_left
r = output * voice_volume_right
```

and add those routed values into the main and optional echo-send accumulators. Shared echo FIR/feedback processing occurs later.

Therefore the exact isolated SPC dry lane is:

```text
per-voice `output`
after synthesis/envelope/pitch consequences
before signed VOLL/VOLR multiplication
before main-mix accumulation
before shared echo accumulation
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

The repository now has a bounded native-source capture contract, exact 32 kHz storage/projection, and a stateful hook bridge whose ordinal survives ordinary decoder blocks and restarts only at an explicit discontinuity.

## 3. Echo send is per voice; echo return is shared state

An S-DSP voice can contribute to the echo input, but the resulting wet signal passes through shared delay/FIR/feedback state.

Therefore:

```text
voice echo-send enabled
!= independently attributable voice wet stem
```

The source contract may preserve exact per-voice send state while still reporting no isolated shared wet-return lane until that bus is explicitly observed.

Do not duplicate the full echo return onto every sending voice.

The next SPC effect-side tap should expose the shared stereo echo field as a shared-effect lane, preserving its own causal identity and signed L/R behavior.

## 4. Physical voice, sample source, and musical part are different identities

Existing repository runtime semantics establish that an SPC `SRCN`/sample change can occur inside one physical voice episode.

The host source model therefore uses:

```text
physical S-DSP voice slot
+ episode generation
```

for realtime source identity.

A `source_latched`/SRCN change is evidence inside that episode. It does not automatically create a new renderer source, note, instrument, or persistent musical part.

Likewise, a new accepted key-on creates a new physical-voice generation and must become a hard host-transport boundary if it occurs inside a larger decode request.

Persistent musical identity is a separate, higher claim. It may bridge physical-slot migration only when independent evidence earns that correspondence.

## 5. Composer testimony says native mix decisions are compositionally meaningful

These sources are historical/interpretive evidence, not machine-state authority.

### Hiroki Kikuta - Secret of Mana

Interview:

- https://vgmonline.net/hirokikikutainterview/

Kikuta describes spending much of the SNES development effort sampling timbres and adjusting the pan, volume, and effects of each of the eight sound channels, alongside manual data compression.

Consequence:

```text
native pan / volume / effect deployment
may be part of the authored arrangement and mix
```

A remastering renderer should preserve those values as evidence rather than treating them as disposable defaults.

### Barry Leitch - Top Gear

Composer interview sources describe his deliberate use of SNES echo at a musically chosen delay while balancing the echo buffer against scarce RAM.

Consequence:

```text
shared effect topology can be an intentional rhythmic/compositional choice
```

The echo network is not merely decoration to be replaced by a generic modern reverb.

### David Wise - SNES / Donkey Kong Country era

Interviews describe the eight-channel and memory constraints and the deliberate integration of environmental/texture material into the available musical resources.

Consequence:

```text
physical channel
!= stable conventional instrument role
```

Channel identity must not be promoted directly to `bass`, `melody`, `drums`, or another musical-part label.

### Yuzo Koshiro - Mega Drive / Streets of Rage

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

### Low-latency and spatially informed separation

Tom Barker, Tuomas Virtanen, Niels Henrik Pontoppidan, 2015:

- `Low-latency sound-source-separation using non-negative matrix factorisation with coupled analysis and synthesis dictionaries`
- DOI: https://doi.org/10.1109/ICASSP.2015.7177968

Paul Magron and Tuomas Virtanen, 2020:

- `Online Spectrogram Inversion for Low-Latency Audio Source Separation`
- DOI: https://doi.org/10.1109/LSP.2020.2970310

Satvik Venkatesh et al., 2024:

- `Real-Time Low-Latency Music Source Separation Using Hybrid Spectrogram-TasNet`
- DOI: https://doi.org/10.1109/ICASSP48485.2024.10448381

Yichen Yang et al., 2024:

- `Stereophonic Music Source Separation with Spatially-Informed Bridging Band-Split Network`
- DOI: https://doi.org/10.1109/ICASSP48485.2024.10446287

Sylvain Marchand and related informed-source-separation work additionally show the value of carrying side information into a later remix/respatialization stage instead of forcing all semantics to be rediscovered from the flattened mixture.

These works support the general usefulness of source-aware and informed realtime processing, but they infer or reconstruct sources under uncertainty.

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

Andreas Floros and Nicolas-Alexander Tatlas, 2011, likewise describe spatial enhancement that preserves original panning/source-location evidence while expanding the perceived sound stage.

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
route-gain arithmetic provenance
shared-effect send state
shared wet-return availability
ordered in-block evidence changes
playback discontinuity epoch
persistent-part evidence when independently earned
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

## 9. Omniphony is a downstream contract quarry, not a second compiler architecture

Current Omniphony source ABI and renderer behavior expose several rules that improve the compiler upstream.

### 9.1 Protected reference is not an object lane

Omniphony explicitly rejects `REFERENCE_MIX` from the object renderer. The historical/reference stereo mix stays beside the source-aware path as the audible authority and A/B/reconstruction control.

Compiler consequence:

```text
reference mix
!= source object
```

`spatial_audio_lane_is_object_renderable()` now makes that distinction executable before the ABI boundary.

### 9.2 Route gain needs arithmetic provenance

Omniphony ABI 0.3 has `ROUTE_GAIN_PREAPPLIED` for the case where source PCM already contains the exact native sample-accurate gain trajectory while the signed L/R gains remain useful pose/polarity evidence.

Compiler consequence:

```text
native route evidence
+ whether route gain is already in PCM
```

must travel independently.

`stereo_route_evidence::gain_preapplied` now carries that fact through normal and timed source evidence. The ABI bridge derives the Omniphony flag from the evidence at each event boundary, while retaining the older host-side override as a migration path.

This prevents a downstream renderer from applying the native gain twice.

### 9.3 Validate the whole timed evidence list before rendering

Omniphony validates ordered event structure before processing the first sample of a call.

Compiler consequence:

- transport construction is transactional;
- malformed event order or lane references fail before rendering;
- evidence is not partially committed into a block that later proves invalid.

### 9.4 Terminal events are next-block state

Omniphony accepts an event at:

```text
frame_offset == frame_count
```

as a zero-length terminal transition. No audio in the completed block belongs to that new state.

This exposed an upstream learning defect: the compiler's musical frontend previously allowed terminal source state to become the `final_evidence` paired with the just-completed PCM. That could train a newly started source episode from the previous episode's audio.

The frontend now excludes trailing terminal events from completed-block acoustic/role learning while still transporting them to Omniphony at the exact boundary.

### 9.5 Presentation continuity is not physical-slot continuity

Omniphony uses an independently earned persistent musical part when available; otherwise it uses runtime source identity. A hardware lane reused by an unrelated source does not inherit the outgoing source's pose history.

Compiler consequence:

- keep physical slot, source episode, and persistent part distinct;
- do not manufacture persistent-part identity merely to smooth the renderer;
- confidence-gate persistent-part transfer into the ABI.

### 9.6 Failed render does not advance musical interpretation

Omniphony's canonical handoff commits renderer presentation identity only after successful rendering. The compiler's `realtime_musical_omniphony_pipeline` similarly calls `complete_block(raw_block)` only after the Omniphony call succeeds.

Compiler consequence:

```text
prepare from past-only state
-> render
-> learn from raw completed evidence only on success
```

This prevents a failed/retried audio block from moving semantic memory ahead of what actually sounded.

### 9.7 Omniphony placement policy stays downstream

Useful Omniphony policies include:

- signed native route is evidence, not authored 3-D position;
- absence of a musical-role label is not positive permission for rear/up placement;
- native phase/surround evidence may outweigh a weak inferred role without becoming authored geometry;
- shared wet fields should remain shared environmental objects rather than fictional per-instrument wet stems.

These are valuable renderer constraints, but the compiler should export the evidence needed to support them rather than copying Omniphony's pose heuristics into the source model.

The ownership line is:

```text
retro-vgm-compiler
-> exact source/device facts
-> bounded musical evidence
-> source identity and timing
-> arithmetic provenance

Omniphony
-> presentation policy
-> spatial continuity
-> room/binaural realization
-> perceptual quality judgment
```

## 10. Current implementation mapping

Shared source and host transport:

- `model/spatial_source.h`
- `model/spatial_source_host_assembler.h`
- `model/spatial_source_host_session.h`
- `model/omniphony_source_transport.h`
- `model/omniphony_realtime_client.h`
- `model/realtime_musical_spatial_frontend.h`
- `model/realtime_musical_omniphony_pipeline.h`

SPC runtime/source path:

- `components/spc/spc_runtime_capture.h`
- `components/spc/spc_runtime_spatial_adapter.h`
- `components/spc/spc_native_source_capture.h`
- `components/spc/spc_native_exact_source_storage.h`
- `components/spc/spc_runtime_host_pipeline.h`
- `components/spc/snes_spc_native_source_hook_bridge.h`

Regression surface includes:

- `tests/model/spatial_source_test.cpp`
- `tests/model/spatial_source_host_assembler_test.cpp`
- `tests/model/spatial_source_host_session_test.cpp`
- `tests/vgm/omniphony_source_transport_test.cpp`
- `tests/vgm/omniphony_realtime_client_test.cpp`
- `tests/vgm/realtime_musical_spatial_frontend_test.cpp`
- `tests/vgm/realtime_musical_omniphony_pipeline_test.cpp`
- `tests/spc/spc_runtime_spatial_adapter_test.cpp`
- `tests/spc/spc_runtime_host_pipeline_test.cpp`
- `tests/spc/spc_native_source_capture_test.cpp`
- `tests/spc/spc_native_exact_source_storage_test.cpp`
- `tests/spc/snes_spc_native_source_hook_bridge_test.cpp`
- `cmake/host_transport_tests.cmake`

The SPC path is no longer evidence-only in principle. It now has an exact dry-source path at the native 32 kHz S-DSP rate. That path is still not yet the normal foobar/libgme execution path because the actual upstream DSP call site has not been patched/bound in the consumer shell.

## 11. Current frontier

The next implementation frontier is now:

1. instrument the actual libgme `Spc_Dsp::run()` per-voice `output` seam and feed `snes_spc_native_source_hook_bridge` during real decoder execution;
2. expose the shared S-DSP echo return as its own causal shared-effect lane without copying it onto sending voices;
3. bind the protected foobar SPC decoder lifecycle to `spc_runtime_host_pipeline`, including seek/flush/reset epochs;
4. carry the resulting source block through the existing Omniphony ABI 0.3 client while keeping the reference stereo mix outside the object renderer;
5. validate sample/chunk invariance and exact native 32 kHz timing against protected reference playback;
6. only then address non-32-kHz host output, where libgme's resampling phase/history must be represented honestly rather than pretending native source samples are already host-rate samples;
7. apply the same compiler/renderer ownership rules to VGM and later xSF families as each gains exact causal source observability.

## Stop conditions

Stop rather than overclaim if:

- SRCN is treated as permanent voice or musical-part identity;
- a key-on generation change is hidden inside a same-identity evidence event;
- signed S-DSP L/R values are collapsed to absolute pan balance in the source model;
- route gain is applied twice because arithmetic provenance was dropped;
- the shared echo return is copied into each sending voice;
- the protected reference mix is passed as a spatial object lane;
- missing source PCM is represented as observed silence;
- terminal next-block evidence is learned from the previous block's PCM;
- mixture-based AI separation replaces an exact emulator signal that is already observable;
- composer recollection is used to overwrite contradictory runtime evidence;
- persistent-part identity is invented to make renderer continuity smoother;
- a renderer-inferred position is relabeled as authored geometry;
- Omniphony presentation heuristics migrate upstream and become source truth.

Correction outranks coherence.
