# Periodic pitch trajectory semantics

## Question

When does a timed register-write log contain enough information to describe a periodic chip's performed pitch trajectory directly, and when must VGM Compiler advance autonomous device state between writes before the pitch evidence is even defined?

This pass pressure-tests the existing nominal-frequency helpers against materially different periodic sound generators:

- AY-3-8910 / YM2149
- Konami SCC / K051649
- NES APU pulse channels
- Game Boy Sound 1 / channel 1
- HuC6280 wavetable PSG with LFO

The purpose is not to build one generic oscillator abstraction. It is to identify which semantic boundaries survive across devices and which do not.

## Evidence method

For device behavior, prefer triangulation across:

1. an established emulator/core;
2. an independent implementation lineage;
3. original manuals, patents, measured-hardware work, or peer-reviewed literature where available.

Pinned implementation observatories used in this pass include:

- ValleyBell/libvgm `61fc6725644886abc3168e240e4e51588d74bdf7`
- openMSX/openMSX `0d462ce723c0850940c348a348d96908c9ea7ad1`
- true-grue/ayumi `07c08b4874c359169e4a028edf73f046d8b763e2`
- LIJI32/SameBoy `213a12ce93d66b105a113debd9396306066a7cfc`
- mgba-emu/mgba `afd6f14eaf8bd35214ed3fb9dc69a92bfc3877a9`
- ares-emulator/ares `b80f67d38312648d197762121c3a27b02c0887db`
- Mednafen mirror lineage `e46995e49cbaf1e9996a8e29cc5be16d7fe6d834`

Documentary anchors include:

- General Instrument AY-3-8910/8912 Programmable Sound Generator Data Manual
- US4933980A, which uses AY-3-8912 devices as the concrete programmable-sound-generator example and explicitly describes their ability to sustain programmed sounds without continued processor intervention
- Nintendo Game Boy Programming Manual / Nintendo AGB Programming Manual Sound 1 sections
- Donahue, Mao & McAuley, *The NES Music Database: A multi-instrumental dataset with expressive performance attributes* (2018), arXiv:1806.04278
- Cordes et al., *Systematic Reverse Engineering of Cycle-Accurate Emulators* (TCHES 2023), DOI 10.46586/tches.v2023.i4.156-186, as a methodological precedent for deriving timing models from observation and validating them against ground truth rather than assuming implementation structure

Community documentation can remain a useful lead, but should not outrank hardware evidence or independently agreeing implementations.

## The first correction: `register frequency` is not one semantic class

The existing project law already says:

```text
register frequency
!= nominal frequency
!= programmed pitch
!= performed pitch
!= heard pitch
!= note spelling
```

This pass sharpens the first part further.

Across these chips, a register can represent at least three different roles:

```text
A. static oscillator parameter
B. seed/control for an autonomous state machine that later mutates pitch
C. base coordinate consumed by another live modulator
```

All three may superficially look like an integer "frequency" or "period" register.

They are not interchangeable.

## AY-3-8910 / YM2149

### Programmed state

Each tone channel has a 12-bit tone-period coordinate. Noise and envelope periods are separate shared generators. Mixer and amplitude registers choose how those generators combine.

The General Instrument architecture is explicitly designed so that, once registers are programmed, the PSG continues producing and sustaining the sound without the CPU repeatedly rewriting the current waveform state.

### Evolved state

Both libvgm/MAME-derived AY code and the independent Ayumi implementation contain autonomous state that advances between writes:

- per-channel tone counter
- per-channel square-wave output phase/state
- shared noise counter and LFSR state
- envelope counter
- envelope step/segment/direction/hold state

Ayumi makes the distinction especially explicit:

```text
set_tone(period)
-> stores tone_period

update_tone()
-> advances tone_counter
-> toggles tone output when the period is reached
```

and similarly advances noise and envelope state continuously.

openMSX/libvgm additionally preserve measured behavior important during rapid period modulation: the tone generator counts upward toward its period, and a period write clamps or preserves counter state rather than conceptually creating a fresh oscillator from a static frequency formula.

### Earned boundary

For ordinary fixed-period tone pitch:

```text
programmed period trajectory
-> nominal oscillator-frequency trajectory
```

can be derived piecewise from writes.

But:

```text
programmed period trajectory
!= exact realized waveform trajectory
```

because exact phase, mixer output, envelope, and noise state require time evolution between writes.

AY therefore demonstrates **autonomous synthesis-state evolution without ordinary autonomous pitch-parameter evolution**.

Do not force a three-layer pitch model onto AY when a two-layer pitch relation is sufficient.

## SCC / K051649

### Programmed state

Each of five channels has:

- a 32-sample waveform
- 12-bit period
- volume
- enable state

The project already protects two different SCC clock coordinate conventions:

```text
VGM-declared SCC clock -> /16 nominal-frequency convention
normalized SCC core clock -> /32 nominal-frequency convention
```

Those can describe the same oscillator while using different input coordinates.

### Evolved state

openMSX independently exposes:

- `period[channel]`
- waveform position
- phase/count state
- per-channel increment

A frequency update sets:

```text
incr = 0 when period <= 8
incr = 32 otherwise
```

and resets the internal count. Some deformation modes also reset waveform position.

This independently supports the project's existing low-period halt guard.

### Earned boundary

SCC again separates:

```text
period -> nominal oscillator rate
```

from:

```text
period + exact phase + waveform position + waveform contents
-> exact realized waveform trajectory
```

Unlike NES/Game Boy/HuC6280 below, ordinary SCC operation does not require inventing an autonomous pitch-control trajectory just because the waveform phase advances.

The important autonomous state is mostly **phase/wavetable realization state**, not a second hidden pitch parameter.

## NES APU pulse channels

NES pulse channels cross a different boundary.

### Programmed state

CPU writes provide:

- an 11-bit timer period
- sweep enable
- sweep divider period
- sweep direction/negation
- sweep shift count
- duty/envelope/length controls

### Evolved pitch state

In ares, the half-frame clock invokes each pulse channel's sweep unit. The sweep unit maintains its own counter and can rewrite its internal `pulsePeriod` without any CPU frequency-register write occurring at that moment.

The transformation is stateful and channel-specific:

```text
change = pulsePeriod >> shift

positive sweep:
    pulsePeriod += change

negative sweep:
    pulsePeriod -= change
    pulse 1 additionally subtracts one
```

The sweep logic also has range/muting consequences.

Thus:

```text
last values written to $4002/$4003 or $4006/$4007
!= current effective pulse period
```

whenever sweep is active.

### Literature connection

NES-MDB is especially relevant to the interpreter's architecture. The paper starts from VGM-like timed APU register writes and then **emulates APU behavior to derive an expressive score**. That published pipeline independently supports the need for a transformation between write-log evidence and performed musical state.

However, NES-MDB then projects the emulated state into note/velocity/timbre categories for MIR purposes. VGM Compiler should preserve the lower device trajectory first and make that symbolic projection reversible and explicitly downstream.

### Earned boundary

For NES pulse channels:

```text
programmed timer/sweep controls
-> autonomously evolved effective timer period
-> nominal oscillator-frequency trajectory
-> realized pulse waveform
```

The middle layer is mandatory when sweep is active.

## Game Boy Sound 1

Game Boy Sound 1 independently reproduces the same broad phenomenon through a different hardware family.

### Documentary evidence

Nintendo's programming manuals describe Sound 1 as a rectangular-wave generator with frequency sweep and specify repeated sweep updates from the previous frequency state at a programmable interval derived from 128 Hz.

The sweep is therefore explicitly recursive in time rather than a static interpretation of NR13/NR14.

### SameBoy

SameBoy exposes particularly detailed autonomous sweep state:

- shadow sweep frequency/sample length
- sweep addend
- sweep countdown
- delayed sweep-calculation countdown
- overflow checks
- trigger/restart timing
- model/revision-specific write glitches

Its implementation demonstrates that the exact current Sound 1 frequency state cannot be recovered by simply reading the latest NR13/NR14 write pair.

### mGBA

mGBA independently keeps a `realFrequency` inside the sweep state. On trigger it seeds that state from the programmed channel frequency, and subsequent sweep updates mutate it on the audio frame sequencer.

### Earned boundary

For Game Boy Sound 1:

```text
NR13/NR14 programmed frequency
+ NR10 sweep control
+ trigger state
+ frame-sequencer timing
-> sweep shadow/effective frequency trajectory
-> realized square-wave trajectory
```

Again:

```text
register-write trajectory != performed pitch trajectory
```

when sweep is active.

The exact Game Boy implementation is not interchangeable with NES sweep merely because both are called `sweep`.

## HuC6280

HuC6280 provides the strongest counterexample to treating pitch evolution as only a periodically updated scalar.

### Ordinary programmed state

Each wavetable channel has a 12-bit frequency coordinate and a 32-sample waveform.

The existing project helper correctly gives an ordinary nominal wavetable rate from the programmed frequency coordinate when LFO is not transforming it.

### Live cross-channel LFO

Mednafen's HuC6280 implementation exposes the hardware-LFO path directly.

When LFO is active:

```text
channel 1 current waveform sample
-> signed modulation value around 0x10
-> shifted according to LFO depth
-> added to channel 0 base frequency coordinate
-> channel 0 effective frequency cache
```

Conceptually:

```text
effective_ch0_period(t)
=
base_ch0_period
+ transform(current_ch1_wave_sample(t), lfo_depth)
```

Channel 1's own waveform stepping is separately scaled by the LFO frequency register.

The Ootake-derived HuC6280 implementation carried by libvgm independently implements the same structural law: channel 0 phase advancement consumes its base frequency plus a shifted live sample from channel 1, while channel 1 advances at its LFO-scaled rate.

### Useful negative observatory

ares currently contains an explicit TODO at this exact boundary: it recognizes that channel 1 is the LFO modulator and suppresses it from the normal mix when LFO is enabled, but its current PCE PSG core marks channel-0 frequency modulation by channel 1 as unimplemented.

This is scientifically useful. A mature emulator can be a valid observatory for one device surface while still being incomplete for another. Playback success must never be promoted to semantic completeness.

### Earned boundary

HuC6280 requires:

```text
programmed base frequency
+ programmed LFO controls
+ modulator waveform contents
+ modulator phase trajectory
-> live effective frequency trajectory
-> realized carrier trajectory
```

Here the hidden/evolved pitch state is not merely a sweep register shadow. It is a **coupled oscillator transformation**.

## Revised trajectory model

The pass does **not** support one universal three-step chain for every periodic chip.

Instead, the source-specific adapter should expose a typed transformation graph with whichever nodes the device actually has.

### Type 1: direct-period oscillator

Typical fixed AY tone or ordinary SCC channel:

```text
programmed period trajectory
-> nominal frequency trajectory
-> phase/waveform realization
```

### Type 2: autonomous pitch-state machine

NES pulse sweep or Game Boy Sound 1 sweep:

```text
programmed base period/frequency
+ programmed sweep controls
+ trigger/frame timing
-> evolved effective period/frequency state
-> nominal frequency trajectory
-> waveform realization
```

### Type 3: coupled modulation

HuC6280 hardware LFO:

```text
programmed carrier base coordinate
+ programmed modulator controls
+ live modulator waveform state
-> time-varying effective carrier coordinate
-> nominal frequency trajectory
-> waveform realization
```

These are transformations, not three new universal ontological layers.

## New evidence law

A stronger version of the project's pitch law is now earned:

```text
latest frequency-register value
!= current effective oscillator coordinate
```

unless the source-specific device semantics prove that equivalence for the current mode.

And:

```text
nominal-frequency helper
!= pitch-trajectory interpreter
```

A nominal helper answers a local coordinate conversion question. A trajectory interpreter must also account for every time-evolving device mechanism capable of changing the effective coordinate or its audibility.

## What the register log does and does not preserve

A VGM-style register log can still contain enough information to reconstruct these autonomous effects **if**:

- all causally relevant writes are preserved;
- timing resolution is sufficient;
- the device's internal transition law is known;
- reset/initial state is known or bounded;
- clock-coordinate conventions are explicit.

Therefore:

```text
register log != performed trajectory
```

does **not** mean:

```text
register log necessarily destroyed the performed trajectory
```

Often the trajectory is latent and recoverable by execution.

This is exactly the kind of representation boundary VGM Compiler should preserve.

## Consequences for corpus analysis

The next executable pass should use existing controls:

- AY: `antarctic-adventure-ay8910`
- SCC: `fuusen-pentai-scc`
- HuC6280: `star-parodier-huc6280`
- NES: `star-soldier-nes-apu-vgm` and `star-soldier-nsf`
- Game Boy: `motocross-maniacs-game-boy`

For each real track, capture two timelines separately:

```text
1. programmed control trajectory
2. evolved effective oscillator trajectory
```

Then assert equality only in modes where the chip semantics guarantee it.

For NES/Game Boy sweep and HuC6280 LFO, the test should deliberately find intervals where the effective trajectory changes without a corresponding direct carrier-frequency-register write at that time.

## Same-work NES control

The Star Soldier NSF/VGM pair becomes more valuable under this model.

The VGM representation supplies downstream timed APU writes.
The NSF representation supplies executable code/data that generates APU behavior.

A future comparison can ask:

```text
NSF execution
-> programmed APU control trajectory
-> evolved effective APU trajectory

versus

captured VGM writes
-> evolved effective APU trajectory
```

without ever assuming NSF track/source structure corresponds one-to-one with VGM channels or commands.

If both routes yield the same downstream evolved trajectory over a bounded region, that establishes a much stronger representation equivalence than comparing raw writes or rendered PCM alone.

## Literature lesson

The most useful literature result is not a chip formula but a methodological alignment.

NES-MDB distinguishes composition from expressive performance and derives the latter by executing/emulating timed APU control data. Cycle-accurate reverse-engineering literature likewise emphasizes constructing and validating transition/timing models rather than assuming internal behavior from external snapshots.

VGM Compiler extends those ideas vertically:

```text
encoded control evidence
-> exact device transition semantics
-> evolved physical performance state
-> reversible musical projections
```

The musical projection remains downstream evidence, not a replacement for the machine trajectory that produced it.

## Stop conditions

Stop rather than guess if:

- a nominal-frequency formula ignores an active sweep/LFO/modulation mode;
- a register snapshot is being used as current effective pitch without advancing autonomous state;
- a waveform-phase change is mislabeled as a pitch change;
- two devices are merged because both mechanisms are colloquially called `sweep` or `LFO`;
- a mature emulator's unimplemented path is mistaken for hardware absence;
- a MIDI/note projection is allowed to overwrite the exact evolved device coordinate;
- reset, trigger, clock, or frame-sequencer provenance is missing.

Correction outranks coherence.
