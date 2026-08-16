# Nonlinear device-mixing semantics

## Status

Cross-device research input for channel isolation, stems, acoustic realization, reference rendering, and enhancement.

## Central correction

The final output of a game sound chip is not universally the linear sum of independently renderable channel signals.

Several unrelated devices contain nonlinear DAC/mixer behavior or finite-width saturation at the point where channel contributions meet.

Therefore:

```text
render(all channels together)
!= universally sum(render(each channel separately))
```

and:

```text
exact per-channel source trajectory
!= exact additive output stem
```

This is independent of the earlier shared-feedback and cross-resource-modulation problems. Even channels with independent source trajectories can fail to add linearly at the mixer.

## 1. NES / Famicom APU

Pinned observatory:

- `ares-emulator/ares`
- commit `b80f67d38312648d197762121c3a27b02c0887db`
- `ares/fc/apu/apu.cpp`

The APU output path explicitly precomputes two nonlinear DAC surfaces.

### Pulse pair

The two pulse channels are first combined as an integer pulse amplitude and then mapped through:

```text
pulseDAC[pulse1 + pulse2]
```

using the familiar nonlinear electrical-model relation implemented in ares.

Thus:

```text
pulseDAC(A + B)
!= generally pulseDAC(A) + pulseDAC(B)
```

### Triangle + noise + DMC

Triangle, noise and DMC are similarly passed as a joint three-dimensional state into:

```text
dmcTriangleNoiseDAC[dmc][triangle][noise]
```

whose value is generated from a nonlinear combined relation.

The final mono APU sample is:

```text
pulse-group nonlinear output
+
TND-group nonlinear output
```

not five independent channel samples added with fixed gains.

This is a direct executable counterexample to additive channel reconstruction.

## 2. AY-3-8910 / YM2149 analog mixing

Pinned observatory:

- `mamedev/mame`
- commit `1e1b6ce78a1645805bb5eef4049e3f1d3f926194`
- `src/devices/sound/ay8910.cpp`

MAME's AY/YM implementation contains an explicit analog resistor-network model motivated by chip measurements.

Its documentation describes each volume level as selecting a resistance in a network which also contains pull-up/pull-down and external load resistances.

For mixed-output configurations, MAME builds a multidimensional table over the simultaneous channel-level combination.

The table-generation code computes the resulting electrical network for combinations of channel states, and `mix_3D()` indexes that joint table from the current A/B/C channel levels.

Thus the mixed analog output is a function:

```text
Vout = F(levelA, levelB, levelC, envelope/mode state, load model)
```

rather than merely:

```text
Vout = f(A) + f(B) + f(C)
```

for all configurations.

This matters particularly when reconstructing the analog mixed pin/output behavior rather than exposing idealized channel contributions before that circuit.

The exact electrical law is AY/YM/board/configuration-specific and must not be generalized into a universal PSG mixer formula.

## 3. Super NES S-DSP finite-width accumulation

Pinned observatory:

- `ares-emulator/ares`
- commit `b80f67d38312648d197762121c3a27b02c0887db`
- `ares/sfc/dsp/voice.cpp`

Each voice applies its L/R volume and adds the result into the main-volume accumulator.

The accumulated value is clamped to the device's finite output range during the mix path.

Therefore, near saturation:

```text
clamp(A + B)
!= clamp(A) + clamp(B)
```

and separately rendered channel signals cannot necessarily be added afterward to reproduce the exact historical dry mix.

Shared echo adds further stateful/nonlinear complications discussed separately in:

- `research/shared-feedback-dsp-state.md`

The important result here is that even the nominal dry accumulation has finite arithmetic.

## 4. Three independent reasons stems may fail to add

The project now has three distinct mechanisms which must not be conflated.

### A. Live cross-resource coupling

See `research/cross-resource-modulation-semantics.md`.

```text
source A changes source B's trajectory
```

Rendering B without A changes B itself.

### B. Shared feedback/effect state

See `research/shared-feedback-dsp-state.md`.

```text
A + B feed one persistent stateful effect
```

Past contributions interact in shared memory/history.

### C. Nonlinear final mixing

This pass:

```text
A and B can remain independent before mix
but
F(A, B) != F(A, 0) + F(0, B)
```

All three break naive additive stem reconstruction for different reasons.

A correct architecture should preserve which mechanism is responsible.

## 5. Physical contribution and additive stem are different coordinates

It remains useful to capture a channel's pre-mix contribution exactly.

For example:

```text
NES pulse 1 digital amplitude
NES pulse 2 digital amplitude
```

are exact lower device coordinates.

But converting each into a separately DAC-shaped WAV and calling those WAVs additive reference stems changes the historical mixer topology.

Prefer terminology such as:

```text
pre-mix channel contribution
mixer input trajectory
counterfactual solo render
final mixed output
```

rather than using `stem` for all four.

## 6. Exact stem strategies

Several useful output modes are possible, but their semantics differ.

### Pre-mix/device-coordinate stems

Export each source at the strongest exact point before the nonlinear shared mixer.

Advantages:

- preserves source/channel identity;
- observationally clean;
- suitable for analysis.

Limitation:

- values may not be directly comparable to final acoustic amplitude;
- simple WAV summation may not reconstruct reference output.

### Counterfactual solo renders

Run the historical device with only one contribution active at the mixer.

This answers:

> what would the chip output if only this source contributed?

It is useful, but the results need not sum to the historical full mix.

### Contribution attribution around a nonlinear mixer

Mathematical attribution methods can divide a nonlinear result among inputs, but any such division is an analytical projection unless the hardware itself supplies it.

Do not present it as an original hardware stem.

### Reference reconstruction bundle

For exact reconstruction, preserve:

```text
all pre-mix source trajectories
+
shared mixer/DAC state and law
+
board/output configuration where relevant
```

Then the final reference mix is reproduced by executing the common mixer, not by summing post-hoc stems.

## 7. Difference signals are not source signals

A tempting isolation technique is:

```text
full_mix - mix_without_channel_A
```

For a nonlinear system this gives the **marginal effect of A in the context of the other active channels**.

That marginal effect can depend on B, C, and the current mixer state.

Therefore:

```text
full - without_A
!= intrinsic channel_A waveform
```

in general.

It can still be a useful causal attribution projection if labeled honestly.

## 8. Enhancement consequence

A source-native enhanced renderer may intentionally linearize or increase headroom in a historical mixer.

Examples:

```text
preserve channel trajectories
+ replace nonlinear/low-headroom mixer with high-precision linear sum
```

or:

```text
preserve nonlinear transfer curve
+ compute internally at higher precision
```

These are different interventions.

Neither should replace the reference path silently.

Historical nonlinearities can be audible platform character, implementation limitation, or both. Whether relaxing them better approximates intended realization is a higher evidence question.

## 9. Perceptual consequence

A nonlinear mixer can create level-dependent interaction between parts even when the composition/driver treats them independently.

Thus perceived balance/timbre may change when another part enters without either part changing its own programmed volume.

A human-facing explanation can eventually say something like:

```text
"the combined texture compresses/saturates differently when both layers hit together"
```

only after the device-level interaction has been established.

Do not mistake mixer interaction for an authored automation curve.

## 10. Highest-information synthetic regressions

### NES

Select two pulse amplitudes `A` and `B` for which:

```text
pulseDAC[A+B] != pulseDAC[A] + pulseDAC[B]
```

and record the exact residual.

Repeat for triangle/noise/DMC combinations.

### AY/YM mixed output

For a configuration using the analog mixed-output model, choose two or three nonzero channel levels and compare:

```text
F(A,B,C)
```

against isolated table outputs summed afterward.

Retain board/load-model provenance.

### S-DSP

Drive two voices so their summed dry contribution crosses a clamp boundary.

Verify:

```text
historical shared accumulation
!= sum of independently clamped solo renders
```

while remaining equal in a low-level region where no clamp is reached.

The paired low/high test proves that additivity is **state/level-dependent**, not simply absent everywhere.

## 11. Common model pressure test

No new common graph primitive is earned.

The existing model already supports:

- source/voice nodes;
- `contributes_to` edges;
- bus/effect/synthesis objects;
- transforms;
- acoustic contributions.

The important implementation rule is that a bus/mixer node can represent a **joint transform of several inputs**, rather than forcing every `contributes_to` edge to imply linear additivity.

## Stop conditions

Stop rather than overclaim if:

- post-DAC/channel solo WAVs are assumed to add exactly on NES;
- AY/YM analog mixed output is reconstructed as a sum of isolated channel tables without validating the circuit/model;
- finite saturation is ignored when claiming exact S-DSP stems;
- `full mix - muted mix` is called the intrinsic source waveform on a nonlinear mixer;
- nonlinear interaction is mistaken for a driver-level volume change;
- an enhanced linearized mixer is called reference hardware behavior;
- pre-mix exact coordinates are discarded merely because they are not directly additive audio stems.

Correction outranks coherence.
