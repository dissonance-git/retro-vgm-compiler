# VGM enhancement frontier

This document records the current engineering frontier. It is intentionally about what exists, not what is hoped for.

For the durable enhancement target and evidence rules, see `source-native-enhanced-rendering.md`.

## Audible status

**foobar2000 playback is still the unmodified libvgm reference render.**

The enhanced renderers below currently run as source-state/shadow infrastructure or dependency-free testable cores. None should be described as an audible improvement until a controlled substitution build has been listened to and retained.

That is deliberate. Accuracy is the control; enhanced rendering must earn the right to replace each source family separately.

## Enhancement target

The target is not generic modernization and not conversion to MIDI, SoundFont, VST instruments, or another arrangement.

It is a **counterfactual source-native realization**:

```text
same executable musical idea
+ same notes / timing / articulation
+ same patch / sample / modulation relationships
+ same structural density and arrangement
        ↓
relax only implementation ceilings that are not identity-bearing
        ↓
higher-fidelity realization
```

Historical constraints are not assumed to be unwanted. Some were burdens or storage compromises; others became part of the instrument.

Every relaxed limitation therefore needs an evidence basis and a reversible A/B.

## VGM format boundary

Cross-chip work now begins with a format layer whose job is deliberately smaller than device emulation.

The VGMRips VGM specification defines what the file-format bytes mean. Chip manuals, driver source, emulator implementations, die analysis and hardware measurements define what the addressed device does with those writes.

The current format layer protects:

- VGM version constants and version gates;
- pre-1.51 versus 1.51+ clock-word flag semantics;
- dual-chip declarations;
- selected bit-31 chip variants;
- the VGM 1.00/1.01 overloaded Yamaha clock field;
- VGM 1.70 distinct second-instance clocks and extra-header metadata;
- EOF and GD3 structure;
- data-block classification;
- Yamaha register-write transport `0x51-0x5F` and `0xA1-0xAF`;
- `0xA0` remaining AY8910;
- generic DAC Stream Control `0x90-0x95`;
- strict reserved-field checks and exhaustive fixed-width opcode controls;
- structural VGM/VGZ timing and loop validation;
- the complete currently defined VGM 1.72 beta Mikey surface under the project shorthand **VGM 1.72d**: Mikey clock, `0x40` register writes and Mikey PCM data blocks.

`1.72d` is a project label for the currently defined upstream 1.72 beta delta, not an upstream version claim.

Relevant code includes:

- `vgm_format_version.h`
- `vgm_chip_clock.h`
- `vgm_yamaha_register_write.h`
- `vgm_dac_stream_command.h`
- strict VGM conformance tests under `tests/vgm/`
- `tools/vgm_corpus_audit.py`

The boundary is:

```text
VGM bytes
→ exact format semantics
→ chip-specific state
→ performance state
→ musical interpretation
```

Do not make libvgm, one emulator, or one chip adapter the sole authority for what the file bytes mean when the format specification already answers that question.

## Live source observation

The repo carries a libvgm patch series that exposes realtime playback state without reverse-compiling a VGM into a score:

1. `0001-realtime-command-observer.patch`
   - command + reset events in normal render and seek replay
2. `0002-resolved-ym2612-dac-observer.patch`
   - actual bytes consumed by legacy `0x80..0x8F` YM2612 DAC playback
3. `0003-dac-stream-source-observer.patch`
   - source-level modern DAC stream state after libvgm resolves bank/destination/rate/start/length
4. `0004-refresh-dac-stream-pcm-bank.patch`
   - refresh stream pointers if an appended PCM block reallocates a bank
5. `0005-fix-dac-stream-millisecond-length.patch`
   - convert millisecond stream lengths using `frequency * ms / 1000`

The resolved observer remains useful, but the raw `0x90-0x95` decoder now provides an independent spec-level oracle beneath it.

The same realtime observer now feeds QSound source-control evidence from the ordinary VGM `0xC4` command stream. This does not require a new libvgm callback and does not alter reference rendering.

## Genesis source truth

`genesis_state` currently tracks two YM2612 instances and two SN76489-family instances.

YM2612 state includes:

- six channels;
- exact register cache;
- key/operator mask;
- F-number/block with Yamaha high-byte latch semantics;
- algorithm/feedback;
- authored L/R routing;
- AMS/FMS;
- LFO;
- channel-3 special mode/CSM frequencies;
- all four operator parameter sets: DT/MUL/TL/KS/AR/AM/DR/SR/SL/RR/SSG-EG;
- DAC enable and resolved DAC source activity.

PSG state includes tone periods, attenuation, noise state/control and stereo mask.

No instrument name, musical role, importance, width, height or other semantic inference is stored as source truth.

## QSound spatial frontier

QSound is a particularly valuable VGM control family because the historical system already keeps source voices separate through an authored spatial-routing stage before the final stereo renderer.

Two mature implementation paths were compared before adding the GMI model:

- MAME's current QSound LLE executes the DL-1425 DSP16A program and documents the host-visible command words;
- the superctr QSound HLE used by MAME/libvgm exposes the decoded source voices, pan tables, shared echo, filters and delays recovered from the DSP program.

They agree on the source-facing command structure that matters here.

### Source topology

The decoded QSound engine contains:

```text
16 PCM source voices
+ 3 ADPCM source voices
        ↓
19 source outputs
        ↓
per-source pan word
        ↓
per-source dry L/R + wet L/R coefficients
        ↓
shared echo / FIR / delay / output stages
        ↓
final stereo
```

For PCM voices, a second source-facing control contributes the source to the shared echo input.

This creates two distinct concepts that must not be collapsed:

```text
QSound wet pan-table contribution
!=
PCM per-channel shared-echo contribution
```

### Source evidence versus renderer state

Current authority boundary:

| QSound state | GMI treatment |
|---|---|
| physical PCM/ADPCM voice | source/execution identity |
| raw pan word | preserved source control |
| decoded dry L/R coefficients | device-authored stereo route evidence |
| decoded wet L/R coefficients | QSound-specific source-routing evidence |
| PCM `0xBA..0xC9` echo contribution | signed source-to-shared-echo evidence |
| global `0x93` echo feedback | historical renderer state |
| global `0xD9` echo delay | historical renderer state |
| FIR table selection | historical renderer state |
| dry/wet output delays | historical renderer state |
| dry/wet output volumes | historical renderer state |
| final QSound stereo | protected reference render |

No row in this table is an authored 3-D coordinate.

### Pan regions currently decoded

The recovered program gives two bounded regions that both MAME HLE and libvgm expose coherently:

```text
0x110..0x130
33-position QSound spatial table
with distinct dry and wet coefficients

0x140..0x160
33-position dry-only linear-pan table
```

A characteristic control case is useful because it proves the wet path is not a generic reverb-send scalar:

```text
raw pan 0x110
  dry: L=1, R=0
  wet: L=0, R=1

raw pan 0x120
  dry: L=1, R=1
  wet: L=0.625, R=0.625

raw pan 0x130
  dry: L=0, R=1
  wet: L=1, R=0
```

The gaps and unknown raw pan words are preserved but **not decoded**. The HLE's defensive index clamp is not promoted into source truth because the MAME LLE executes the real DSP ROM rather than that defensive approximation.

### Live VGM shadow now implemented

VGM command `0xC4` carries:

```text
[data MSB] [data LSB] [QSound address]
```

The modified foobar path now captures source-facing C4 writes at exact output-sample offsets using the same allocation-free/fail-closed pattern as the Genesis source observers.

Admitted live controls are limited to:

- PCM pan `0x80..0x8F`;
- ADPCM pan `0x90..0x92`;
- PCM shared-echo contribution `0xBA..0xC9`.

The adjacency is tested explicitly: `0x93` is global echo feedback, not a twentieth pan control.

The shadow resets to the recovered DL-1425 initialization state: all 19 pan words at center `0x120`, PCM echo contributions cleared.

Overflow, malformed commands and out-of-order block timing fail closed.

### Current audible boundary

QSound source **control evidence** is now live. QSound source **audio** is not yet substituted.

libvgm remains the audible historical QSound renderer.

The next audio tap is the decoded QSound `voice_output[19]` point after source synthesis/voice volume but before the four dry/wet pan coefficients are applied. That is the correct causal source boundary for Omniphony.

The intended experiment is therefore:

```text
same VGM QSound command stream
        ├─ historical libvgm/QSound renderer → stereo control
        └─ 19 causal source voices
              + authored QSound source controls
              + separately preserved environmental path
                    ↓
                 Omniphony
                    ↓
           modern full-sphere binaural
```

QSound is a historical authored-spatial base and calibration system, not a ceiling. The original two-speaker transfer function remains the scientific control while the modern renderer is free to improve externalization and three-dimensional presentation without rewriting the source controls as fictional coordinates.

### GitHub and literature controls

Implementation authorities used for this boundary include:

- MAME `src/devices/sound/qsound.cpp`: current DL-1425 LLE and host command semantics;
- MAME `src/devices/sound/qsoundhle.cpp`: HLE recovered from the DSP program;
- libvgm `emu/cores/qsound_ctr.c`: independent product-side HLE integration used by the current VGM playback foundation.

The Arcade Projects CPS QSound amplifier reverse-engineering thread is useful secondary evidence for a separate point: the cabinet amplifier/filter stage is downstream of the QSound spatial processing, so adding another QSound-like spatial effect after the final CPS stereo output would be double processing. It is a research lead, not authority for DSP register semantics.

Peer-reviewed spatial-audio work supports the architectural separation rather than QSound-specific register claims:

- Landschoot & Jot, 2023, *Binaural externalization processing method for object-based audio rendering*, JASA, DOI `10.1121/10.0018389`: object-aware externalization can be treated separately from source identity;
- Coleman et al., 2017, *Object-Based Reverberation for Spatial Audio*, JAES, DOI `10.17743/JAES.2016.0059`: direct objects and environmental/reverberant contribution can remain separately renderable;
- Sunder et al., 2015, *Natural Sound Rendering for Headphones*, IEEE Signal Processing Magazine, DOI `10.1109/MSP.2014.2372062`: headphone rendering must balance spatial and timbral fidelity.

These papers support the rendering split. They do not prove historical QSound intent for a particular song or pan word.

## Yamaha cross-chip state frontier

The YM2612 vertical slice is now being pressure-tested against related Yamaha families rather than being promoted directly into a universal model.

### OPN

Current bounded family coverage distinguishes:

- YM2203;
- YM2608;
- YM2610;
- YM2610B;
- YM2612;
- YM3438.

Shared OPN register helpers now cover key/channel mapping, operator ordering, frequency latch/commit semantics, channel-3 special frequency registers and algorithm/feedback packing.

`genesis_state.cpp` consumes those helpers. The old Genesis regressions remain the acceptance test for the refactor.

### OPM

YM2151/OPM preserves its own 8-channel register map and key-code + key-fraction pitch representation.

OPM and OPN independently earned only a very small four-operator shared surface:

```text
algorithm bits 0..2
feedback  bits 3..5
register slot order 1,3,2,4
```

That surface is intentionally narrower than "Yamaha FM."

### OPL

YM3526, Y8950, YM3812 and YMF262 are explicitly separate from the OPN/OPM packing.

OPL uses a one-bit connection selector plus feedback in bits 1..3. YMF262 also supports 18 channels, four output buses and six dynamic four-operator channel pairs.

The regression deliberately proves that the same register byte can have different algorithm/connection semantics under OPL and OPN/OPM.

### OPLL

YM2413/OPLL adds preset/user-instrument provenance:

```text
instrument 0 → user patch registers
instrument >0 → preset instrument data
```

The command stream can prove patch selection without necessarily containing the full preset definition. Patch identity therefore needs both selection evidence and the relevant chip/variant instrument-data context.

## Nominal pitch convergence

Pitch normalization is being earned above the device encodings rather than imposed below them.

Current source coordinates include:

```text
YM2612 / OPN: block + 11-bit FNUM
YM2151 / OPM: key code + key fraction
OPL:          block + 10-bit FNUM
OPLL:         block + 9-bit FNUM
SN76489:      tone period
SMPS:         compiled/transposed chromatic coordinate where source evidence exists
```

A new OPL/OPLL regression demonstrates that YM3812, YMF262 and YM2413 can encode the same approximately `439.990595 Hz` nominal channel basis through different native pitch codes and clock divisors.

That common coordinate is **not** automatically:

- a note spelling;
- a MIDI note;
- a performed acoustic fundamental;
- a heard pitch;
- a chord tone.

Those remain higher inference layers.

## Enhanced source engines implemented

### SN76489-family PSG

`sn76489_enhanced`

- four isolated mono stems: three tone + noise;
- floating-point 2 dB attenuation ladder;
- oversampled PolyBLEP square reconstruction;
- source-faithful noise LFSR;
- sample-accurate timed writes inside a foobar block;
- fast no-output state advance for seeks/shadow playback;
- real device clock divider, feedback taps, LFSR width, Sega zero-period behavior and output polarity;
- explicit fallback for unvalidated NCR/T6W28 variants.

An objective regression test compares off-harmonic alias energy against a naive high-pitch square wave.

### Classic YM2612 DAC

`ym2612_dac_enhanced`

- isolated floating-point PCM stem;
- exact direct `$2A/$2B` state;
- exact resolved legacy VGM DAC bytes;
- `0x80` as the source zero point;
- interpolation only between PCM points that actually exist;
- hard authored DAC enable/disable boundaries;
- null-output shadow advancement.

### Modern VGM source-bank streams

The VGM stream transport is now treated as chip-neutral at the format layer.

The existing YM2612 sink remains:

`ym2612_pcm_stream`

- consumes the original libvgm PCM bank directly;
- source write frequency is authoritative;
- step/base interleaving preserved;
- start/length semantics preserved;
- reverse and loop preserved;
- windowed-sinc/Lanczos reconstruction over the original source bytes.

The important architectural change is that future PCM/ADPCM-capable chip adapters should reuse the VGM stream transport semantics rather than each inventing a private `0x90-0x95` parser.

All 256 VGM stream IDs remain logically separate in the wrapper shadow state.

## YM2612 FM frontier

The FM boundary now preserves **absolute VGM source ticks**, not output-frame offsets.

`ym2612_block_capture` stores exact ordered register writes in the VGM source clock. `ym2612_fm_clock` provides an overflow-safe rational mapping from VGM ticks to the first native FM sample at or after that tick.

`ym2612_fm_backend` receives the whole ordered timed-write block and owns:

```text
absolute VGM source ticks
        ↓
native YM2612 FM clock / synthesis
        ↓
high-quality rate conversion
        ↓
six phase-coherent output-rate FM stems
```

The caller must not quantize writes to consumer/output frames first.

The backend contract also exposes fixed algorithmic output latency so the FM path can later be aligned with PSG/DAC before source mixing. Null-output advancement must traverse the same timing/state path.

The intended first FM backend remains a mature Yamaha synthesis engine with channel output exposed **before final stereo summation**. `ymfm` remains the leading architecture under investigation because one coherent FM engine can be clocked once while channel outputs are observed independently. Nuked-OPN2 remains an important independent accuracy reference.

Do not instantiate six unrelated YM2612 emulators merely to obtain six stems. Shared LFO/global state and phase must remain coherent.

The first FM milestone remains:

> same patch + same automation + mature synthesis semantics + six isolated source stems

After reference parity is established, the next question is:

> **Which hardware ceilings can be relaxed while the patch still sounds unmistakably like the same programmed FM instrument?**

Candidate experiments remain one-at-a-time and reversible:

- higher internal numerical precision;
- improved reconstruction/output bandwidth;
- reduced avoidable aliasing/imaging where it is not identity-bearing;
- higher-quality PCM/DAC realization;
- higher-precision summation and headroom;
- removal of output-stage defects only when they are not part of patch identity.

SMPS/GEMS/source-side controls should verify that enhanced rendering preserves authored driver/patch behavior rather than merely matching a register trace superficially.

## Real-corpus structural control

`tools/vgm_corpus_audit.py` now provides a spec-driven admission test for VGM/VGZ files before chip-specific analysis.

The immutable Sonic 3 & Knuckles corpus currently gives this result:

```text
58 / 58 structurally valid
58 / 58 computed total wait samples == header Total # samples
57 looped files
57 / 57 loop offsets on command boundaries
57 / 57 computed loop samples == header Loop # samples
57 × VGM 1.50
 1 × VGM 1.10
```

The only non-looped file is `30 - Staff Roll (S&K).vgz`.

All 58 declare SN76489 at `3,579,545 Hz` and YM2612 at `7,670,453 Hz`.

This is now the known-good control for admitting additional VGMRips families. External OPN/OPNA/OPM/OPL/OPLL packs are research candidates but are not yet permanent byte-verified fixtures.

See `../research/vgm-cross-chip-controls.md`.

QSound should now receive its own small orthogonal corpus rather than being tested only with synthetic table values. Useful controls include static hard-left/center/hard-right pan, time-varying pan, nonzero PCM echo contribution, quiet sources and tracks that exercise the dry-only linear range if real material uses it. Store derived evidence and hashes, not copyrighted game data.

## Mixing and spatial boundary

`source_stem_mixer` performs only explicit linear source routing with double-precision accumulation.

It does **not** compress, limit, normalize, widen, infer source roles, or invent spatial positions.

`ym2612_authored_route()` preserves the chip's original L/R enable decisions for the first enhanced listening baseline.

For QSound, the source model preserves the richer dry/wet routing grammar but still does not create a 3-D position. The modern headphone scene belongs downstream in Omniphony.

The durable source-aware spatial boundary is therefore:

```text
causal VGM source audio
+ device-authored routing/effect evidence
+ protected historical reference mix
        ↓
Omniphony presentation inference
        ↓
full-sphere binaural renderer
```

Synthesis quality and spatial reinterpretation should not be changed in the same first listening experiment. QSound is useful precisely because the same command stream can drive the original historical renderer and the modern source-aware renderer as an A/B control.

## Primary documentation and historical evidence

Mature emulator/reverse-engineering sources remain critical, but they are no longer the only low-level observatory.

Official Sega/Yamaha documentation hosted by preservation sites such as Sega Retro is useful as a primary-source referee for documented chip/register/bus/clock behavior, programmer expectations and hardware-revision distinctions. The underlying manual or schematic is the primary source; a wiki summary remains secondary unless corroborated.

This is separate from historical intent evidence, which answers a different question: whether a technical limitation was unwanted, accepted, or deliberately used artistically.

Useful intent evidence includes:

- creator statements about workflow burden or compromised sound quality;
- songs/notes/sections explicitly cut for memory;
- same-team CD or higher-quality versions;
- surviving pre-compression samples or synth patches;
- source-code/driver evidence showing the transformation into the shipped asset;
- creator statements that a specific artifact was deliberately valued.

See `../research/historical-constraint-friction-counterfactual-rendering.md`.

## Future VGM-set assistance

The structural validator also exposes a future preservation-tooling direction: state-aware assistance with loop validation and set preparation.

This is **not implemented as automatic loop discovery**.

The important design law is that a loop should eventually be judged by executable state as well as by sound. A waveform can appear to repeat while hidden chip/stream/modulation state diverges, and exact state recurrence can be stronger evidence than waveform similarity alone.

The current audit validates declared timing and loops only. It does not propose or rewrite them.

## Validation

`tools/run_core_tests.py`

- discovers every `tests/vgm/*_test.cpp`;
- compiles against the complete dependency-free enhancement core;
- uses C++17, optimization and warnings-as-errors;
- runs every produced executable.

The manual `core-tests` workflow now also owns strict standalone QSound spatial-source and control-capture tests. GitHub-hosted Actions remain unavailable while the account runner is rejected before execution by the platform billing/spending-limit state. Absence of CI is not a pass or a failure.

The QSound regressions currently protect:

- 16 PCM + 3 ADPCM source topology;
- exact decoded hard-left/center/hard-right dry/wet table behavior;
- the dry-only linear pan region;
- raw preservation of unknown pan words;
- PCM-only signed shared-echo contribution;
- exclusion of global renderer registers from source evidence;
- exact VGM C4 payload parsing and sample offsets;
- bounded realtime capture and fail-closed overflow;
- no authored 3-D position claim.

## Next sequence

Do not enable every enhancement or chip family at once.

1. keep the VGM 1.72d format/spec tests as the common transport floor;
2. admit a small orthogonal real QSound corpus and compare the live C4 shadow against reference QSound behavior;
3. expose the QSound `voice_output[19]` causal audio point without altering the historical renderer;
4. preserve the QSound shared/environmental return separately and prove reference recomposition where the renderer is linear enough for that claim;
5. feed the earned QSound lanes and source evidence to Omniphony while retaining historical QSound stereo as the A/B control;
6. continue the YM2612 six-stem mature FM backend and clock/resampling work;
7. admit small orthogonal real corpora for YM2203, YM2608, YM2151, YM2413, YM3812 and YMF262;
8. build stable time-bearing pitch/part trajectories above device-specific state;
9. establish reference render parity before relaxing any synthesis ceiling;
10. retain only enhancements that preserve musical and instrument identity.

The target is not one giant Yamaha emulator and not a cleaner VGM player.

It is **many exact machines feeding one increasingly well-earned understanding of the music**.
