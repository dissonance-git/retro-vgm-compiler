# FM operator trajectory semantics

## Question

What exact execution state must VGM Compiler preserve for Yamaha-style FM synthesis before it can make claims about a persistent musical part, a performed pitch, an instrument/patch identity, or a heard timbre?

This pass pressure-tests the OPN-family surface against OPM and OPL3 rather than assuming that all Yamaha FM chips share one register ontology.

The central result is that a physical FM channel is not a single oscillator and a programmed FM patch is not the performed sound object.

## Evidence method

The device behavior in this pass was triangulated across independent implementation lineages and technical literature.

Pinned implementation observatories:

- `nukeykt/Nuked-OPN2` `335747d78cb0abbc3b55b004e62dad9763140115`
- `aaronsgiles/ymfm` `81aec25ccbb98f4873a255f7551ac4dadac59b4a`
- `jotego/jt12` `45f4854f9ab43368f5a514857299ab7dfae4e6ab`
- `nukeykt/Nuked-OPM` `f209e6ed3712032b641d53ce8fb24824eae6adc3`
- `nukeykt/Nuked-OPL3` `cfedb09efc03f1d7b5fc1f04dd449d77d8c49d50`

Technical/documentary anchors:

- Yamaha YM2608 OPNA Application Manual, especially the FM operator, algorithm, phase-generator, envelope-generator, LFO, and CH3-mode sections
- John M. Chowning, *The Synthesis of Complex Audio Spectra by Means of Frequency Modulation*, Journal of the Audio Engineering Society 21(7), 1973
- Yamaha FM-synthesis patents describing multiple operation units/operators and configurable modulation connections
- die/decap evidence credited by the Nuked implementations, including YM3438/YM2151/YMF262 observations

The Chowning paper supplies synthesis theory, not bit-exact Yamaha-chip semantics. The Yamaha manual and independently agreeing implementations are the stronger evidence for device behavior.

## OPN / YM2612 / YM3438

### Channel-level programmed frequency is only the start

The familiar OPN channel pitch surface is FNUM plus block.

Nuked OPN2 then derives an operator phase increment using additional state:

```text
channel FNUM/block
+ channel PMS / live LFO phase modulation
+ operator detune
+ operator multiplier
-> operator phase increment
```

Its phase-generator path applies LFO phase modulation to FNUM, applies block scaling, adds/subtracts the slot's detune adjustment, then multiplies by the slot's multiplier.

ymfm independently derives the same structure. It caches channel block/FNUM, computes keycode-dependent detune and per-operator multiple, and either caches a phase step or marks it dynamic when LFO phase modulation is active.

JT12 expresses the same law as hardware-style blocks:

```text
FNUM + block
-> PM offset
-> phase increment

FNUM + block
-> keycode
-> detune

phase increment + detune + multiplier
-> operator phase accumulation
```

Therefore:

```text
channel FNUM/block
!= operator phase increment
```

and:

```text
nominal channel frequency
!= exact operator frequency trajectory
```

when detune, multiplier, or phase modulation is active.

### Operator frequency is not automatically musical pitch

An FM operator may be a carrier, a modulator, or part of a feedback path depending on the algorithm.

The Yamaha manual defines eight four-operator algorithms and explicitly distinguishes carrier and modulator roles. JT12 independently implements the algorithm as routing decisions between previous/internal operator outputs.

Chowning's general FM result explains why this matters musically: the relation between carrier and modulator frequencies controls the placement of spectral components, while modulation amount changes their distribution.

Consequently:

```text
operator phase frequency
!= independently heard note pitch
```

A modulator may strongly determine timbre without appearing as a separately perceived sinusoidal pitch. Even a carrier frequency is not sufficient by itself to establish the heard fundamental of a complex FM result.

Do not convert every operator phase increment to a MIDI note and call the result four musical voices.

## Algorithm and feedback are execution topology

The algorithm number is not merely a patch label. It determines the directed modulation graph between operators and which operator outputs reach the channel output.

Feedback adds a self-referential modulation path to the designated operator.

The exact lower device state therefore needs both:

```text
operator node state
```

and:

```text
modulation/routing edges
```

A flat vector of four operator parameters loses an important part of the synthesis mechanism.

Still:

```text
FM algorithm graph
!= persistent musical-part graph
```

The graph describes synthesis signal flow inside one hardware resource, not authored contrapuntal voices.

## Key state is per operator on OPN

OPN key-on register semantics include a four-bit operator mask.

ymfm returns both a target channel and operator mask from the key-on write. JT12 similarly separates the target channel from the four operator key bits.

Therefore a single physical FM channel can contain operators in different key/envelope states.

This invalidates a naïve lifetime rule such as:

```text
channel key-on -> one indivisible FM voice lifetime
```

The stronger representation is:

```text
physical channel allocation
+ per-operator key/envelope trajectories
```

A higher-level musical-note/part episode may later bind several operator trajectories together when driver/device evidence supports that grouping.

## Envelope state is autonomous performed state

Each operator owns time-evolving envelope-generator state.

Programmed attack/decay/sustain/release parameters and total level are controls, not the instantaneous amplitude.

Between writes, the envelope generator continues to evolve according to clock, key state, rate scaling, SSG-envelope mode where supported, and current phase/state.

Therefore:

```text
operator ADSR parameters
!= operator amplitude trajectory
```

and:

```text
patch parameters
!= performed FM state
```

The same evidence boundary found in periodic pitch sweep reappears here in amplitude: the register log can preserve the causes while the performed trajectory remains latent until the device transition law is executed.

## LFO state is autonomous and cross-cuts pitch and amplitude

OPN-family LFO state evolves between writes.

The Yamaha manual defines phase-modulation sensitivity and amplitude-modulation sensitivity separately. Nuked and ymfm both maintain an advancing LFO state and consume it during phase and/or envelope/output calculation.

With phase modulation enabled:

```text
static FNUM/block
+ static PMS
+ evolving LFO phase
-> time-varying operator phase increment
```

So an operator's effective frequency can change between direct frequency-register writes.

This places OPN FM in the same broad evidence class as the previously tested autonomous/coupled periodic mechanisms, but the implementation is source-specific and must not be collapsed into NES sweep or HuC6280 LFO semantics.

## CH3 special/effect and CSM modes

OPN CH3 is the strongest counterexample to:

```text
one physical channel -> one base frequency
```

The Yamaha YM2608 application manual states that CH3 effect/CSM modes allow separate F-Number settings for the four slots/operators.

JT12 independently implements three special FNUM/block pairs in addition to the ordinary channel frequency and selects them according to the currently evaluated CH3 operator.

ymfm likewise substitutes special `multi_block_freq` values for the relevant channel-2 operators when multi-frequency mode is active.

Thus the correct relation is mode-dependent:

```text
normal OPN channel:
    one channel FNUM/block source
    -> four operator-local frequency transforms

CH3 special/effect mode:
    per-operator FNUM/block sources
    -> four operator-local frequency transforms
```

In CSM mode, Timer A also participates in key-on/off behavior, so timer state becomes causally relevant to the performed operator-envelope trajectory.

This is why `physical channel` and `musical voice` must remain separate concepts.

## OPM / YM2151 pressure test

OPM preserves the operator-state pattern but rejects an OPN-shaped frequency schema.

### Different channel pitch coordinates

OPM starts from channel key code and key fraction rather than OPN FNUM/block.

Nuked OPM derives an internal phase-generator frequency coordinate from:

```text
channel key code + key fraction
+ channel PMS / live LFO phase modulation
+ operator DT2
-> internal frequency/keycode state
+ operator DT1
+ operator multiplier
-> operator phase increment
```

OPM therefore adds a second detune class, DT2, to the operator frequency transformation.

This means the common representation should not contain fields named `fnum` and `block` merely because OPN uses them.

Use a typed source-specific programmed pitch coordinate and preserve the transformation path that maps it to each operator's phase increment.

### OPM LFO is not merely OPN LFO with different register numbers

OPM has its own LFO controls and frequency representation. Its exact modulation path must remain OPM-specific even though the higher relation survives:

```text
programmed pitch coordinate
+ autonomous modulation state
+ operator-local transforms
-> operator phase trajectory
```

The shared relation is earned; the numeric law is not.

## OPL3 / YMF262 pressure test

OPL3 changes the resource topology more radically.

### Two-operator and four-operator resources

Nuked OPL3 explicitly distinguishes:

- ordinary 2-op channels;
- the first half of a linked 4-op pair;
- the second half of a linked 4-op pair;
- drum/rhythm resources.

In 4-op mode, writes to the primary channel's FNUM/block propagate to the paired channel, and key-on/off operates over all four slots. The two channel resources are then connected according to the combined 4-op algorithm state.

Therefore:

```text
physical register channel count
!= active synthesis-voice count
```

and a fixed `four operators per channel` schema would fail OPL immediately.

### OPL operator phase

OPL3 again derives per-slot phase from a channel FNUM/block plus slot-local multiplier, with vibrato/phase effects applied in the device-specific path.

The multiplier mapping is itself OPL-specific and must not reuse OPN's law merely because both fields are called `multiple`.

OPL3 also supports selectable per-slot waveforms. The waveform selection participates in synthesis realization and therefore belongs below musical interpretation.

### Topology can change at runtime

Switching 4-op pairing changes which physical resources participate in one synthesis graph.

This means the FM representation cannot assume that graph membership is immutable for the whole file.

Use a time-bearing graph/resource configuration when the source can change synthesis topology during playback.

## The patch boundary

A conventional FM patch can usefully describe a stable programmed parameter bundle such as:

```text
algorithm
feedback
operator multiplier/detune
operator total level
operator envelope parameters
operator AM enable
waveform where applicable
```

But this object must remain distinct from execution state.

A performed realization additionally depends on:

```text
physical resource allocation/mode
programmed pitch coordinate(s)
operator key state
operator envelope state
operator phase accumulator
live LFO state
mode/timer state
algorithm routing
feedback history
output routing/pan
```

Therefore:

```text
patch != instrument identity
patch != performed note
patch != physical voice episode
patch != current timbre
```

Patch similarity is useful evidence for driver/toolchain/style research, but it is not composer identity or musical-part identity.

## Revised FM execution object

The strongest cross-family structure supported by OPN, OPM, and OPL3 is not a generic channel record. It is a source-specific, time-bearing operator graph attached to physical synthesis resources.

Conceptually:

```text
FM execution state
=
device + clock convention
+ physical channel/resource allocation
+ synthesis mode/topology
+ programmed pitch-coordinate source(s)
+ operator nodes
+ modulation/feedback edges
+ global modulation sources
+ output routing
```

Each operator node may retain:

```text
physical operator/slot identity
programmed local parameters
resolved phase-increment trajectory
phase accumulator trajectory where needed
key state
resolved envelope state
amplitude modulation state
waveform identity where supported
```

This is an evidence structure, not a promise that every device will expose every field.

Device-specific adapters remain authoritative for numeric semantics.

## Pitch evidence hierarchy for FM

A more precise FM-specific pitch route is now earned:

```text
programmed channel/slot pitch coordinate
-> operator-local phase-increment trajectory
-> operator modulation graph
-> synthesized acoustic trajectory
-> auditory pitch evidence
-> performed-note / spelling hypothesis
```

Do not skip from FNUM/KC directly to note spelling.

And do not assume that all operator frequencies should be exposed as musical pitches. They are synthesis parameters first.

## Timbre evidence hierarchy for FM

Likewise:

```text
static patch parameters
+ time-evolving operator state
+ modulation graph
+ feedback
+ global LFO
+ output stage
-> realized spectrum through time
-> auditory/timbral evidence
-> human timbre description
```

This fits Chowning's fundamental result that FM timbre depends on frequency relationships and changing modulation depth, while preserving the much richer device-specific execution evidence of Yamaha hardware.

## Real-corpus controls already available

The permanent corpus already gives several materially different FM controls:

- `sonic-3-ym2612-psg` — YM2612/OPN2 plus PSG
- `wanderers-from-super-scheme-ym2608` — YM2608/OPNA
- `cameltry-ym2610` and `raimais-ym2610b` — OPNB-family controls
- `outrun-segapcm` and `tetris-system16-ym2151` — YM2151/OPM
- `bucket-relay-champ-ymf262` — YMF262/OPL3
- `truxton-ym3812` — YM3812/OPL2
- `disc-station-ym2413` — YM2413/OPLL negative/generalization control

These are sufficient to make the next pass executable without adding another soundtrack.

## Next executable tests

### 1. OPN2 operator-state capture

On one bounded Sonic 3 cue, recover separately:

```text
channel programmed FNUM/block
per-operator DT/MUL
per-operator key transitions
algorithm/feedback changes
PMS/AMS/LFO state
per-operator resolved phase increment
per-operator envelope phase/state
CH3 normal/special-mode state
```

Do not infer musical part identity yet.

### 2. CH3 adversarial search

Search the real OPN/OPNA corpus for intervals using CH3 special/effect/CSM mode.

If found, assert that the operator frequency-source mapping diverges from the normal one-channel/one-FNUM assumption.

If no fixture uses it, retain a synthetic regression and report the real-corpus absence rather than fabricating coverage.

### 3. OPM comparison

Capture the equivalent lower state on a bounded YM2151 fixture and compare only semantic relations that survive the change from FNUM/block to KC/KF + DT2.

The test should fail if an OPN-specific coordinate is silently projected onto OPM.

### 4. OPL3 comparison

Capture a bounded YMF262 fixture and identify whether it uses 2-op or 4-op mode over time.

When 4-op pairing is active, represent the linked resource graph explicitly rather than as two unrelated hardware channels.

### 5. Patch-versus-performance control

Find two intervals that reuse the same static operator parameter bundle at different programmed pitches or with different LFO/key/envelope histories.

Demonstrate:

```text
same patch hash
!= same performed trajectory
```

Then find repeated executions of the same pitch/patch whose lower execution state is equivalent enough to support a reusable patch identity without making that patch a musical-part identity.

### 6. Human interpretation stays downstream

Only after exact operator/resource trajectories exist should the system ask questions such as:

- does this behave like a bass patch, bell, brass-like attack, metallic percussion, pad, or lead?
- which operator appears to control attack brightness?
- is a modulation change functioning perceptually as vibrato, tremolo, timbral motion, or a transition effect?

Those are musical/auditory interpretations over the execution evidence, not register aliases.

## Interaction with source/driver analysis

Native driver evidence such as SMPS or GEMS may expose a higher-level patch/instrument object before writes reach the FM chip.

Preserve the route:

```text
driver patch/instrument reference
-> driver patch definition
-> device register programming
-> FM operator graph state
-> acoustic realization
```

Do not identify the driver patch object with the device's current execution state. Driver macros/modulation/envelopes can continue transforming the programmed device state across ticks.

This route is particularly valuable for the existing Sega/SMPS/GEMS source material because it can eventually provide a forward vertical slice from authored/driver semantics into the exact operator trajectories described here.

## Interaction with xSF expansion

The PSF1/USF/2SF expansion now reconstructs exact platform-specific effective objects but deliberately stops before runtime/device execution.

That is compatible with this work. When those runtimes arrive, their sound hardware should pressure-test these evidence boundaries rather than inherit an FM-shaped model.

In particular, future PlayStation SPU, Nintendo 64 audio, and Nintendo DS audio execution will test whether the broader distinction

```text
programmed source/runtime state
!= physical synthesis trajectory
!= persistent musical part
```

survives outside the VGM/chip-log world.

## Stop conditions

Stop rather than guess if:

- one channel FNUM/KC value is being reported as every operator's exact frequency without applying local transforms;
- an operator frequency is being promoted directly to a musical-note identity;
- a static patch is being treated as current performed timbre;
- operator key state is discarded because the enclosing channel stayed allocated;
- CH3 special/CSM mode is flattened to the normal OPN channel frequency;
- OPM KC/KF is coerced into OPN FNUM/block merely to share a struct;
- OPL 4-op pairing is represented as two independent 2-op musical voices;
- algorithm/routing edges are discarded while retaining only operator parameter vectors;
- LFO/envelope/feedback history is ignored when it materially changes the performed output;
- a technical patch similarity is promoted to composer identity or authored-part identity.

Correction outranks coherence.
