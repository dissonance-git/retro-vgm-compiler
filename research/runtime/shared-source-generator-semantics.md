# Shared source-generator semantics

## Status

Cross-device research input for source-generator identity, physical voices, noise synthesis, causal isolation, and persistent-part inference.

## Central correction

A physical voice/channel is not universally backed by one independent oscillator or source generator.

Several unrelated sound chips expose one evolving source process which can be consumed by multiple separately controlled output resources.

Therefore:

```text
physical voice identity
!= source-generator identity
```

and:

```text
number of audible/resource channels
!= number of independent synthesis generators
```

This pass focuses on shared noise generators because they provide unusually clear examples.

## 1. Super NES S-DSP

Pinned observatory:

- `ares-emulator/ares`
- commit `b80f67d38312648d197762121c3a27b02c0887db`

The S-DSP owns one global noise state including:

```text
noise frequency/counter
noise LFSR
```

`misc30` advances that global LFSR according to the device noise-frequency state.

At each voice's synthesis stage, if the voice's NON/noise-selection bit is active, the normal BRR/interpolated source is replaced by the **current global noise LFSR value**.

Only after this source selection does the individual voice apply its own envelope and later its own L/R volume/routing.

Thus two simultaneous noise-enabled S-DSP voices can have:

```text
same instantaneous stochastic source value
+ different envelopes
+ different pitch registers that are irrelevant to noise source traversal
+ different volume/pan/echo sends
```

They remain separate physical voice resources, but they do not own independent noise oscillators.

## 2. PlayStation SPU

Pinned observatory:

- `stenzek/duckstation`
- commit `5fd366809053fe287291de7a39752c4d5d5b146b`

DuckStation stores the SPU noise process globally:

```text
noise count
noise level
```

and updates it through `UpdateNoise()`.

`GetVoiceNoiseLevel()` simply returns the shared `noise_level`.

When sampling any physical voice:

```text
if noise mode for this voice:
    source sample = shared GetVoiceNoiseLevel()
else:
    source sample = this voice's interpolated ADPCM

source sample
-> this voice's ADSR
-> this voice's volume/routing
```

Therefore multiple SPU voices can simultaneously consume one shared stochastic source while remaining independently enveloped and routed.

This resembles the S-DSP at the higher causal level without implying identical LFSR/timing arithmetic.

## 3. AY-3-8910 / YM2149

Pinned independent observatories:

- `mamedev/mame` commit `1e1b6ce78a1645805bb5eef4049e3f1d3f926194`
- `openMSX/openMSX` commit `0d462ce723c0850940c348a348d96908c9ea7ad1`

The AY-family chip has three tone channels but one noise generator.

MAME keeps one device-global noise RNG/output state rather than one noise process per channel.

openMSX independently models a single `NoiseGenerator` with a 17-bit shift-register process verified in its comments against real AY8910/YM2149 behavior.

Each channel's mixer control determines whether that common noise process participates alongside or instead of the channel's own tone-generator state.

The implementation effectively evaluates each channel against the same noise timeline, while the channel retains its own:

- tone generator;
- mixer enable state;
- fixed/envelope amplitude relation.

Thus:

```text
AY channel A noise
AY channel B noise
AY channel C noise
```

are not three independently randomized generators.

They are three channel-level uses of one evolving device source.

## 4. Source generator, voice resource, and output contribution are separate objects

The evidence supports at least these distinct coordinates:

```text
source generator
-> source value/trajectory
-> one or more consuming physical resources
-> per-resource envelope/gain/routing
-> acoustic contributions
```

For shared noise:

```text
one LFSR/noise process
-> voice A
-> voice B
-> voice C
```

can coexist with:

```text
voice A envelope != voice B envelope
voice A routing != voice B routing
```

Therefore the common graph should not force source-generator state into one physical-voice object.

Existing `synthesis_object`, `physical_slot`, `causes`, `routes_to`, `controls`, and time-bounded edges are sufficient to represent this without a new universal primitive.

## 5. Correlation is exact device causality, not accidental similarity

When two noise-enabled voices use the same shared generator, correlated microscopic waveform structure is not evidence that they are the same musical part.

It follows directly from the device topology.

Therefore:

```text
waveform correlation
!= persistent-part identity
```

and:

```text
shared source generator
!= shared musical role
```

Two independent percussion parts can consume the same hardware noise generator through different envelopes/routing.

Conversely, one musical part could use both tone and noise resources over time.

## 6. Muting/isolation consequence

A safe mute must distinguish:

```text
stop/reset shared generator
```

from:

```text
remove one consumer's final contribution
```

Resetting or freezing the shared noise process to isolate one channel can alter every other consumer and later device state.

Therefore a reference-faithful physical-channel solo should normally let the global generator continue evolving and suppress only the selected consumer/output contribution at a downstream safe point.

This is analogous to the shared-feedback-DSP result, but the causal direction differs:

```text
shared generator:
shared past/current source -> many voices

shared feedback DSP:
many voices -> one shared persistent downstream state
```

Both defeat naive per-voice independence.

## 7. Pitch interpretation consequence

A voice's pitch/frequency register may become irrelevant or partially irrelevant when a different shared source is selected.

On S-DSP and SPU noise-mode voices, the source waveform is supplied by the global noise generator rather than the voice's ordinary sampled source traversal.

Thus:

```text
voice pitch register present
!= ordinary pitched source active
```

A pitch feature extractor must consult source-selection mode before converting a nominal voice-rate coordinate into a performed-pitch claim.

## 8. Instrument/timbre consequence

Noise timbre can depend on a combination such as:

```text
shared noise generator rate/state
+ per-voice envelope
+ filter/interpolation path where relevant
+ gain/pan/effects
```

The shared generator may be the common physical source, while the per-voice envelope/routing creates perceptually distinct percussion/noise gestures.

Do not treat `noise source ID` as instrument identity.

## 9. Persistent-state consequence

The global generator can continue advancing even when one consumer is silent or disabled, depending on the device rules.

Therefore a future noise episode should preserve separately:

```text
generator lifetime/state trajectory
consumer membership intervals
consumer envelope/output lifetimes
```

Do not reset generator phase merely because one voice episode ends.

## 10. Highest-information synthetic regression

For a device with shared noise:

1. enable noise on two physical consumers at the same time;
2. give them intentionally different envelopes/volumes/pans;
3. verify the source noise coordinate is identical at both taps for each device tick/sample;
4. verify the final consumer outputs differ according to local state;
5. mute one consumer only after the shared-source tap and confirm the other consumer trajectory is unchanged;
6. reset or perturb the shared generator and confirm both consumers change;
7. preserve one generator identity plus two consumer identities in the execution graph.

Cross-device claim:

```text
one evolving generator can feed multiple physical output resources
```

Device-specific claim:

```text
exact generator algorithm/rate and exact tap point
```

## 11. Broader synthesis lesson

This joins several earlier corrections:

```text
physical slot != voice episode != persistent musical part
physical slot != immutable synthesis role
physical resources are not always causally independent
```

with a new one:

```text
physical slot != independent source generator
```

The device is better understood as a time-varying synthesis graph than as a row of self-contained MIDI-like channels.

## Stop conditions

Stop rather than overclaim if:

- one noise oscillator is invented per physical voice on a chip with a shared generator;
- correlated noise consumers are merged into one musical part solely because their source waveform matches;
- muting one consumer resets/freezes shared generator state and is still called observationally neutral;
- a voice pitch register is interpreted as active pitched traversal while noise-source substitution is selected;
- shared noise-generator identity is promoted to instrument identity;
- AY's three channel outputs are treated as three independent noise LFSRs;
- source-generator lifetime is tied automatically to one consumer voice episode.

Correction outranks coherence.
