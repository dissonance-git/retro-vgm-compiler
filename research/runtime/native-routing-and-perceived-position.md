# Native routing and perceived position

## Status

Cross-device research input for stereo/spatial semantics, acoustic realization, panning claims, and later headphone/spatial enhancement.

## Central correction

A device routing coordinate is not automatically a perceptual position coordinate.

Across unrelated game-audio systems, the exact native state ranges from hard output-enable bits to signed gain vectors, four-bus routing with phase inversion, and table-driven filter/delay spatialization.

Therefore:

```text
native routing state
!= scalar pan
!= perceived lateral position
!= source azimuth
```

The lower layer should preserve the device's actual output-routing coordinates. `left`, `right`, `center`, `wide`, `behind`, or another spatial description is a later acoustic/perceptual inference.

## 1. YM2612 / OPN2: routing bits, not continuous pan

Pinned observatory:

- `nukeykt/Nuked-OPN2`
- commit `335747d78cb0abbc3b55b004e62dad9763140115`
- `ym3438.c`

Register group `0xB4` contains two independent one-bit output enables:

```text
bit 7 -> left
bit 6 -> right
```

Nuked OPN2 stores them as `pan_l[channel]` and `pan_r[channel]`, but the exact hardware coordinate is a pair of route-enable bits.

Possible states are therefore:

```text
00 neither
10 left only
01 right only
11 both
```

Calling this a continuous `pan` value would invent interpolation that the register does not contain.

A higher renderer may derive a conventional stereo label such as `left`, `right`, or `both`, but the exact evidence remains the two bits.

## 2. Super NES S-DSP: signed L/R gain vector

Pinned observatory:

- `ares-emulator/ares`
- commit `b80f67d38312648d197762121c3a27b02c0887db`
- `ares/sfc/dsp/dsp.hpp`
- `ares/sfc/dsp/memory.cpp`
- `ares/sfc/dsp/voice.cpp`

Each physical voice owns two **signed 8-bit** volume coordinates:

```text
VxVOLL
VxVOLR
```

and the device multiplies the mono voice signal independently by the left and right values before accumulation.

Therefore these are not merely magnitudes. Negative values invert polarity on that route.

For example, the states:

```text
(+64, +64)
(-64, +64)
```

have equal absolute L/R gain magnitudes but are physically different stereo signals because the first route of the second state is phase inverted.

Thus:

```text
left/right level balance
!= complete S-DSP routing identity
```

A scalar pan coordinate cannot represent every valid native state losslessly.

The same signed-coordinate warning applies separately to main and echo output volumes, which are also signed in the pinned implementation.

## 3. Namco C352: four output buses plus phase state

Pinned observatory:

- `mamedev/mame`
- commit `1e1b6ce78a1645805bb5eef4049e3f1d3f926194`
- `src/devices/sound/c352.h`
- `src/devices/sound/c352.cpp`

The C352 has **four digital outputs**, not one stereo pan scalar.

Per voice it keeps:

```text
front left gain
front right gain
rear left gain
rear right gain
```

with ramped current-volume state for all four routes.

The flag surface additionally contains explicit phase-inversion behavior for front/rear routes.

MAME's mixer emits four separate output channels and applies the route-specific sign before gain accumulation.

Therefore a C352 voice's exact spatial/output state is closer to:

```text
4-component gain trajectory
+ route-specific phase state
```

than to `pan`.

Even if a later playback environment folds those four buses down to stereo, that fold-down is a transformation with its own provenance and must not be back-projected as the chip's authored routing.

## 4. Capcom QSound: pan index participates in a spatial DSP

Pinned observatory:

- `mamedev/mame`
- commit `1e1b6ce78a1645805bb5eef4049e3f1d3f926194`
- `src/devices/sound/qsoundhle.cpp`

MAME's QSound HLE is based on disassembled DSP code.

Each PCM/ADPCM voice has a `m_voice_pan` coordinate, but the runtime does not simply turn that coordinate into one left gain and one right gain.

For each output channel, the code forms a pan-table index and separately reads:

```text
PAN_TABLE_DRY
PAN_TABLE_WET
```

coefficients for that voice. Those contributions then participate in QSound's larger filter, echo, wet/dry delay, and output pipeline.

Thus QSound's nominal `pan` register/index is an input to a **table-driven spatial realization**, not by itself a perceptual azimuth or even a complete pair of final gains.

This is a useful negative control against renaming every platform-specific routing coordinate to a normalized `[-1,+1]` pan value.

## 5. Psychoacoustic boundary

Spatial-hearing literature independently supports keeping the upper perceptual claim separate.

Horizontal localization/lateralization depends on multiple auditory cues, especially:

- interaural time difference (ITD);
- interaural level difference (ILD);
- frequency-dependent cue weighting;
- spectral cues;
- arrival-time/precedence effects;
- reverberation and other cues for externalization.

Useful observatories surfaced in the SciSpace pass include:

- William M. Hartmann, `Localization and Lateralization of Sound` (2021);
- Macpherson & Middlebrooks, `Listener weighting of cues for lateral angle: The duplex theory of sound localization revisited` (JASA, 2002);
- Li, Baumgartner & Peissig, `Modeling perceived externalization of a static, lateral sound image` (2020).

These sources establish perceptual principles, not retro-chip register semantics.

The relevant inference boundary is:

```text
device routing
-> realized ear/speaker signals
-> ITD / ILD / spectrum / reverberant cues
-> perceived lateralization/localization hypothesis
```

A register called `pan` by an emulator is not a direct measurement of the last node.

## 6. Same apparent balance can arise from different native states

Several native states can produce similar coarse stereo balance while remaining physically different.

Examples:

```text
S-DSP:
(+64,+64) versus (-64,+64)

C352:
front-only versus front+rear folded down by a later mixer

QSound:
different pan/filter/delay states with similar average L/R energy
```

Therefore:

```text
same ILD / average L-R level
!= same native routing identity
```

This matters for both provenance and enhancement.

## 7. Routing state and musical part identity are different

A persistent musical part can move through native routing states over time.

Conversely, several parts can share the same routing coordinate.

Therefore:

```text
routing identity
!= persistent-part identity
```

A part tracker may attach a time-bearing routing trajectory to an already-supported part hypothesis, but should not create part identity from pan position alone.

## 8. Native routing should remain a vector/structured coordinate

Do not require every adapter to emit one common scalar.

Preserve device-native state such as:

```text
YM2612:
{left_enable, right_enable}

S-DSP:
{left_signed_gain, right_signed_gain}

C352:
{front_l, front_r, rear_l, rear_r, phase_flags}

QSound:
{pan_table_index, relevant mode/filter/delay context}
```

Higher generic features may then derive bounded descriptors:

```text
left-dominant
right-dominant
symmetric
anti-phase-like
front-bus-only
rear-energy-present
moving balance
```

where the realization actually supports them.

Do not discard the native vector after deriving the descriptor.

## 9. Stereo image and physical speaker topology are separate

A four-output device does not become intrinsically `surround` merely because an emulator labels outputs front/rear.

Likewise, a two-output chip does not itself specify the listener's speaker placement, headphone transfer function, room, or later console/board analog path.

Keep separate:

```text
chip output buses
board/console mixing
capture/playback channel mapping
speaker/headphone reproduction
listener percept
```

This is especially important when historical multi-output hardware is captured into a stereo preservation format.

## 10. Enhancement consequence

Source-native enhancement and headphone spatialization must not overwrite authored/native routing evidence.

A safe architecture is:

```text
native routing trajectory
-> historical/reference realization
-> optional enhancement/spatial projection
```

not:

```text
native pan register
-> guessed azimuth
-> replace original routing
```

An enhanced renderer may deliberately widen, externalize, or reconstruct a multi-bus scene, but the intervention must remain downstream and reversible relative to the reference evidence.

This keeps VGM Compiler's source-native rendering role distinct from a later Omniphony/headphone-spatialization layer.

## 11. Highest-information regressions

### S-DSP signed-routing test

Use the same mono source with:

```text
(+gain,+gain)
(-gain,+gain)
```

and verify that a scalar absolute-balance feature cannot distinguish the two while the native vector does.

### YM2612 routing test

Exercise all four L/R-enable combinations and ensure no intermediate continuous pan value is invented.

### C352 four-bus test

Keep the same source/voice episode while moving gain among front and rear buses and toggling phase flags.

Verify physical voice identity remains stable while routing trajectory changes.

### QSound table-index test

Choose two pan indices and preserve the exact table-index provenance plus resulting dry/wet coefficients. Do not store only a derived L/R ratio.

## Stop conditions

Stop rather than overclaim if:

- signed S-DSP gains are normalized into one scalar pan and phase information is lost;
- YM2612 hard route bits are described as continuous pan automation;
- C352 four-bus state is collapsed to stereo before provenance is recorded;
- QSound pan index is called an azimuth without reconstructing the downstream DSP/acoustic cues;
- equal L/R energy is treated as identical native routing;
- a chip output-bus label is equated with a listener-space location;
- spatial enhancement overwrites native routing instead of remaining a downstream transformation;
- a routing trajectory is used as proof of persistent musical-part identity.

Correction outranks coherence.
