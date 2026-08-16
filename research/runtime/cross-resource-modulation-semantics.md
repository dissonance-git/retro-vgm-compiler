# Cross-resource modulation semantics

## Status

Cross-device research input for synthesis causality, pitch trajectories, resource identity, and future musical interpretation.

## Central correction

A physical synthesis resource is not necessarily causally independent from its neighbors or from other resources in the device.

Several unrelated systems allow one resource's evolving state or output to alter another resource's synthesis trajectory.

The common law is deliberately narrow:

```text
resource-local programmed state
!= complete resource trajectory
```

when a live coupling edge is active.

Do **not** collapse the mechanisms below into one generic `sidechain`, `FM`, or `pitch modulation` enum. The useful evidence is the exact causal edge:

```text
source resource
+ source signal coordinate/stage
-> transform
-> target resource
+ target parameter/stage
```

## 1. Super NES S-DSP PMON

Pinned independent observatories:

- `ares-emulator/ares` commit `b80f67d38312648d197762121c3a27b02c0887db`
- `blarggs-audio-libraries/snes_spc` commit `ec8ee2bbe30451614c1d02a83f7af1c97d497d45`

The S-DSP PMON register enables pitch modulation per physical voice, with voice 0 explicitly excluded because there is no preceding voice to supply the modulator.

Both implementations agree on the causal route.

For voice `n` when PMON is active:

```text
voice n-1 sample/noise source
-> voice n-1 envelope
-> previous-voice mono output
-> scale/modulation transform
-> voice n current pitch step
```

The modulator is **not** the preceding voice's final left/right panned signal.

In ares, `voice3c` modifies the current latched pitch using the prior `latch.output`, and only later stages apply the current voice's L/R volume.

Blargg independently computes:

```text
m.t_pitch += ((m.t_output >> 5) * m.t_pitch) >> 10
```

where `m.t_output` has already had the source voice's envelope applied.

Thus:

```text
SNES PMON source coordinate
= previous physical voice's post-envelope, pre-pan mono output
```

and:

```text
PMON edge
= physical adjacency-sensitive amplitude/signal -> pitch coupling
```

## 2. PlayStation SPU PMON

See:

- `research/ps1-spu-pitch-semantics.md` where present in the repository's PS1 research lineage
- the existing SPU pitch helper/tests

Pinned observatories from the earlier pass include DuckStation, Beetle/Mednafen, MiSTer FPGA, and PSX-SPX documentation.

The broad causal structure independently resembles S-DSP PMON:

```text
physical voice n-1
post sample/noise selection
post ADSR
pre L/R volume
-> PMON transform
-> physical voice n phase/sample step
```

Again, voice 0 cannot use a preceding physical voice as a modulator.

The numeric law is **not** identical to the S-DSP law and retains known implementation/documentation disagreements at high-bit pitch and maximum-step boundaries.

Therefore:

```text
SNES PMON
and
PS1 PMON
```

support the same higher causal relation without earning a shared numeric implementation.

This is a useful example of a genuinely recurring hardware idea implemented by different chips.

## 3. HuC6280 LFO is a different coupling geometry

Pinned observatory:

- `libretro/beetle-pce-fast-libretro`
- commit `b211204c7026dff6e86e79b00185512e2421fff8`
- `mednafen/pce_fast/psg.c`

When the HuC6280 LFO mode is active, channel 0's effective frequency is recalculated from:

```text
channel 0 programmed frequency
+
(channel 1 live waveform/DDA sample - midpoint) << depth
```

while channel 1's own stepping frequency is separately scaled by the LFO frequency register.

Thus the causal edge is closer to:

```text
channel 1 wavetable oscillator
-> current waveform sample coordinate
-> depth transform
-> channel 0 frequency coordinate
```

This is **not** the SNES/PS1 previous-voice post-envelope amplitude path.

The source is a secondary oscillator/wavetable state intentionally repurposed as a modulation source.

Therefore:

```text
HuC6280 LFO
!= generic previous-voice PMON
```

although both demonstrate that a target channel's frequency register alone is insufficient to recover its performed pitch trajectory.

## 4. Yamaha FM is a graph, not adjacent-channel sidechaining

See:

- `research/fm-operator-trajectory-semantics.md`

Pinned observatories there include Nuked OPN2/OPM/OPL3, ymfm, JT12, Yamaha documentation, and Chowning's FM work.

Yamaha FM creates causal dependencies among **operators inside a configurable synthesis graph**.

Depending on the algorithm:

```text
operator output
-> another operator's phase/modulation input
```

or:

```text
operator output/history
-> feedback
-> same operator's modulation path
```

The operator graph can determine timbre while several operators belong to one physical channel-level synthesis resource.

OPL3 further allows runtime pairing of two 2-op channel resources into one 4-op topology.

Thus:

```text
FM operator modulation edge
!= hardware-channel adjacency edge
```

and:

```text
operator node
!= persistent musical part
```

The FM graph is synthesis topology first.

## 5. Same musical word can hide different causal layers

Terms such as:

- FM;
- pitch modulation;
- vibrato;
- sidechain;
- modulation source;

are useful human descriptions but are too broad to serve as exact device semantics.

For evidence, preserve at least:

```text
source resource identity
source signal coordinate
source signal stage
target resource identity
target parameter/stage
coupling transform
coupling enable/mode state
delay/adjacency/topology rule
```

Optional source-specific examples:

```text
SNES:
voice 3 post-envelope mono output
-> PMON formula
-> voice 4 pitch step

PS1:
voice 7 post-ADSR mono output
-> SPU PMON formula
-> voice 8 sample step

HuC6280:
channel 1 current wavetable sample
-> LFO depth transform
-> channel 0 frequency

YM2612:
operator 1 output/feedback history
-> selected algorithm edge
-> another operator's phase modulation path
```

These records may later support a higher musical interpretation such as `vibrato-like`, `FM brightness`, or `cross-modulated texture`, but the interpretation must remain downstream.

## 6. Resource identity does not imply causal independence

The earlier sampled-voice work found useful episode boundaries for independent playback resources.

This pass adds a guardrail:

```text
separate physical resource IDs
!= independent synthesis processes
```

A resource can have its own address/pitch/envelope/lifetime while still consuming another resource's live state.

Therefore a generic `voice episode` should not be forced to contain every cause of its output.

Instead preserve external causal edges when they exist.

The common execution graph already has provenance-bearing causal relations, so no new universal graph primitive is earned yet.

## 7. Isolation consequence

A solo or stem operation can break the original synthesis if it suppresses a modulator before the point where another resource consumes it.

For example:

```text
mute source voice at generator/envelope stage
-> target pitch trajectory changes
```

whereas:

```text
let source execute normally
mute only its final audible contribution after modulation tap
-> target trajectory can remain historically correct
```

The exact safe mute point is device-specific.

This extends the shared-feedback result:

- shared effects create **future-state dependence**;
- cross-resource modulation creates **live inter-resource dependence**.

A safe stem system must preserve both where reference-faithful execution is required.

## 8. Causal attribution consequence

At a given instant, target voice B may be the only audible carrier while source voice A is functioning primarily as a modulator.

Thus:

```text
not directly audible
!= musically/acoustically irrelevant
```

and:

```text
carrier output ownership
!= complete causal ownership of timbre/pitch
```

This is especially obvious in FM synthesis but also applies to SNES/PS1 PMON when a preceding resource exists mainly to control another voice.

A future human-facing explanation may legitimately say that one layer is `bending`, `driving`, or `shaping` another only after the causal edge has been established.

## 9. Pitch evidence consequence

For a coupled target resource:

```text
programmed pitch coordinate
+ autonomous local state
+ incoming modulation edge(s)
-> performed synthesis-rate trajectory
-> acoustic output
-> heard pitch evidence
```

Do not compute a note directly from the target frequency register when an incoming coupling is active.

This unifies the earlier periodic-pitch, FM, and SPU results without making their numeric laws generic.

## 10. Highest-information regression

A future generic causality test should not attempt to emulate all devices with one formula.

Instead each source-specific adapter should expose a bounded synthetic case with:

1. target resource programmed to a constant base pitch coordinate;
2. source resource changing while target registers remain untouched;
3. target resolved pitch/rate trajectory changing as a consequence;
4. source final-output mute applied **after** the modulation tap, showing the target still changes;
5. source synthesis muted **before** the modulation tap, showing the target no longer follows the same trajectory;
6. provenance linking target trajectory changes to the incoming edge.

Cross-device assertion:

```text
target performed trajectory changed without a target pitch-register write
```

Device-specific assertion:

```text
which source coordinate caused it, and by what law
```

## Stop conditions

Stop rather than overclaim if:

- every `pitch modulation` feature is mapped to one generic formula;
- an audible channel is assumed causally independent because it has a separate register slot;
- the final stereo output of a modulator is used when hardware taps a pre-pan/internal signal;
- HuC6280 LFO is described as previous-channel amplitude modulation;
- FM operators are treated as independent musical voices merely because they have independent phase/envelope state;
- muting a modulator changes a target trajectory but the result is still called an exact isolated stem;
- a target register timeline is called the performed pitch trajectory while live incoming modulation is active;
- a human term such as `vibrato` replaces the exact source/target causal evidence.

Correction outranks coherence.
