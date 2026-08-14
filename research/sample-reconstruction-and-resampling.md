# Sample reconstruction and resampling

## Status

Cross-device research input for sampled-voice realization, reference parity, and source-native enhancement.

## Central correction

Decoded sample values do not uniquely determine the waveform emitted at the device output rate.

```text
sample/source values
+ traversal/rate trajectory
+ reconstruction/interpolation law
-> realized waveform
```

Therefore:

```text
decoded sample sequence != device-rate acoustic realization
```

and replacing interpolation is a rendering intervention even when sample bytes, pitch, loop, envelope, and routing remain unchanged.

## Super NES S-DSP

Pinned observatory:

- `ares-emulator/ares`
- commit `b80f67d38312648d197762121c3a27b02c0887db`
- `ares/sfc/dsp/gaussian.cpp`

The S-DSP reconstructs between decoded BRR samples with a four-tap Gaussian interpolation process selected by the voice's fractional sample position. The implementation also preserves the hardware-like finite arithmetic/clamping behavior.

Thus the BRR decoder output is an intermediate source trajectory, not yet the final voice waveform.

```text
BRR bytes
-> decoded PCM history
-> fractional sample position
-> Gaussian interpolation
-> envelope/routing
```

A renderer which substitutes linear, sinc, or another higher-quality resampler is no longer reference S-DSP realization even if every musical event remains identical.

## PlayStation SPU

Pinned observatory:

- `stenzek/duckstation`
- commit `5fd366809053fe287291de7a39752c4d5d5b146b`
- `src/core/spu.cpp`

The SPU voice state contains distinct sample and interpolation indices, and DuckStation includes the PS1 Gaussian coefficient table used by its interpolation path.

As on S-DSP, decoded ADPCM blocks are not themselves the final device-rate voice signal.

The path includes:

```text
SPU ADPCM blocks
-> decoded sample history
-> fractional voice counter/sample position
-> PS1 reconstruction/interpolation
-> ADSR
-> volume/routing/modulation/reverb paths
```

This is separate from the block-cache/mutable-memory timing documented in `research/mutable-sample-memory-observation.md`.

## Namco C352

Pinned observatory:

- `mamedev/mame`
- commit `1e1b6ce78a1645805bb5eef4049e3f1d3f926194`
- `src/devices/sound/c352.cpp`

C352 gives an especially strong control because interpolation is mode-dependent inside one device.

MAME keeps both `sample` and `last_sample`. When the relevant filter/interpolation flag permits it, the current output is linearly interpolated from those values according to the fractional counter. When the mode disables that interpolation, the voice uses the current sample directly.

Therefore, for the same:

```text
sample bytes
sample address trajectory
frequency/rate
volume
loop state
```

changing only the interpolation/filter mode can change the realized waveform.

This proves that reconstruction mode is a separate device-state coordinate rather than a property of sample identity.

## Sample identity and reconstruction identity are separate

A reusable sample/source object should not silently absorb one renderer's interpolation method.

Keep separate:

```text
sample identity / encoded source
sample decode law
source-address trajectory
rate/fractional-phase trajectory
reconstruction/interpolation law
voice envelope/gain
routing/effects
```

This also protects sample-source attribution: matching an upstream waveform does not mean the historical console emitted that waveform without its native interpolation/filtering stage.

## Reference renderer versus enhanced renderer

The reference path should preserve the historically supported reconstruction law where parity is claimed.

A source-native enhanced path may deliberately change it, for example:

```text
same decoded samples
same timing/pitch
same loops
same envelope
same routing
+
higher-quality reconstruction filter
```

That is a particularly clean enhancement experiment because the intervention can be bounded to one stage.

But the result should be labeled something like:

```text
constraint-relaxed reconstruction
```

rather than `more accurate reference` unless historical evidence says the native interpolation was not part of the intended realization.

## Higher-quality source substitution is a different intervention

Do not confuse:

```text
replace sample bytes with a higher-quality upstream source
```

with:

```text
keep historical sample bytes and change reconstruction filter
```

or:

```text
change both
```

Those transformations answer different questions and should be independently switchable/testable.

## Perceptual consequence

Interpolation can alter:

- high-frequency attenuation;
- aliasing/imaging;
- transient shape;
- phase/frequency response;
- apparent brightness or smoothness.

A human-facing description such as `smoother`, `brighter`, or `less grainy` belongs above an acoustic comparison, not directly on the interpolation-mode register.

## Highest-information regressions

### C352 same-source control

Render one synthetic sample trajectory twice with all state equal except interpolation/filter mode.

Verify:

```text
sample identity equal
source-address trajectory equal
pitch/rate trajectory equal
output waveform unequal
```

### S-DSP / PS1 reference control

For a short known decoded sample history and fractional phase, verify the adapter's reconstructed value against the pinned reference implementation at several fractional positions and edge/clamp cases.

### Enhanced-renderer control

Run native and replacement resamplers from the exact same decoded/traversal evidence. The diff should be attributable only to the reconstruction stage.

## Common-model pressure test

No new common graph primitive is earned. Reconstruction can remain a time-bearing synthesis transform between decoded source/traversal evidence and acoustic contribution.

## Stop conditions

Stop rather than overclaim if:

- decoded sample PCM is called the final chip waveform;
- sample identity includes one renderer's interpolation method by default;
- replacing Gaussian/linear interpolation is called reference-equivalent without a parity test;
- a higher-quality source substitution is conflated with a higher-quality resampler;
- a perceived brightness/smoothness change is inferred directly from a mode bit without acoustic evidence.

Correction outranks coherence.
