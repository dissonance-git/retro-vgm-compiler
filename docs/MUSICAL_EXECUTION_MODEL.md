# Musical execution model

## Purpose

VGM Tooling supports radically different digital music representations without forcing their source semantics into one file format.

The stable abstraction begins **after source-specific execution**.

```text
VGM register log ───────┐
SPC machine snapshot ───┤
MIDI event stream ──────┤
SMPS / GEMS / MDX ──────┤
tracker/module data ─────┤
ROM-derived driver data ─┘
            │
            ▼
source-specific parser / executor
            │
            ▼
source and synthesis state
            │
            ▼
common musical execution model
            │
      ┌─────┼──────────┐
      ▼     ▼          ▼
 reasoning  libaural   rendering
```

Do not normalize the source representation prematurely. Normalize **meaning after execution**.

The objective is for higher-level reasoning to ask the same musical questions regardless of whether the evidence arrived as YM2612 registers, an SPC700 RAM snapshot, MIDI messages, a tracker pattern, or a known driver sequence.

## Source classes

Use established preservation terminology where practical.

### Logged execution traces

Examples: VGM/VGZ.

Strong evidence:

- exact device commands present in the log;
- command timing;
- register state;
- embedded PCM/ROM blocks when present;
- device clocks and routing represented by the format.

Usually not proved by the log alone:

- original driver track identity;
- original sequence opcode;
- composer-facing instrument names;
- persistent musical-source identity when a driver dynamically reallocates hardware channels.

### Ripped executable state / machine snapshots

Examples: SPC, NSF/NSFe, HES, KSS, PSF-family formats where executable machine state is retained.

Potential evidence:

- CPU/program state;
- sound RAM;
- driver code and data;
- sample/instrument memory;
- live hardware state;
- enough executable context to recover higher-level semantics by controlled execution or driver-specific analysis.

The exact information available differs by format and system. Do not assume every executable/ripped format exposes the same layers.

### Driver / sequence formats

Examples: SMPS, MDX and other known music-engine data formats.

Potential evidence:

- logical tracks;
- note/rest/tie events;
- instrument/program identity;
- modulation and articulation commands;
- loops and control flow;
- allocation policy into physical devices.

These can contain stronger musical semantics than a register log while still requiring a device model to produce sound.

### Symbolic performance formats

Example: MIDI.

Potential evidence:

- explicit note-on/note-off events;
- velocity;
- controllers;
- pitch bend;
- program/bank changes;
- channel routing;
- SysEx and device-specific control when preserved.

The synthesis engine may be external to the file. A MIDI note does not prove the acoustic realization without the target module/synth state.

### Tracker/module formats

Potential evidence:

- explicit tracks/channels;
- instruments/samples;
- notes;
- per-step effects;
- pattern/order structure;
- mixing parameters.

Again, tracker channel identity is a source representation, not automatically one perceptual auditory stream.

## Layers

The common model keeps these layers distinct.

### 1. Source representation

Exact bytes, commands, memory locations, files, addresses, driver objects, and source timing.

This layer is never replaced by an inferred musical description.

### 2. Execution state

What the digital machine is doing while the source runs.

Examples:

- CPU/driver track state;
- sequence position;
- hardware-channel allocation;
- register writes;
- sample pointers;
- operator state;
- effect routing;
- clock/timing state.

### 3. Synthesis objects

Objects that generate or transform sound.

Examples:

- FM patch/operator set;
- BRR/PCM/ADPCM sample object;
- PSG oscillator/noise source;
- wavetable;
- MIDI-module partial/patch where known;
- echo/reverb/effect object;
- QSound source and spatial-processing state.

A hardware slot is not an object identity. Use stable content/state identity where possible.

### 4. Musical performance

Format-independent musical concepts supported by the source execution.

Candidate objects include:

- note/event onset and release;
- continuous pitch trajectory;
- dynamics/amplitude trajectory;
- articulation/envelope trajectory;
- persistent musical voice/part;
- instrument identity or instrument hypothesis;
- rhythmic event;
- phrase/loop/section boundary;
- authored spatial/routing trajectory;
- effect-send trajectory.

This is **not MIDI conversion**. Continuous controls, non-note sounds, FM/noise behavior, sample triggering, and effects remain first-class.

### 5. Acoustic realization

The sound produced by a reference or enhanced renderer.

Keep source stems available before final summation where practical.

### 6. Auditory interpretation

What a listener may hear as events and streams.

Owned primarily by libaural research:

- concurrent grouping;
- sequential grouping;
- persistent auditory streams;
- fusion/separation;
- foreground/background;
- masking;
- pitch/timbre/loudness/spatial percepts;
- melody, rhythm and other musical relations as perceived.

Physical source structure is unusually strong ground truth, but it does not dictate perception one-to-one.

## Evidence status

Every transition upward must preserve how the claim was obtained.

Use ordinary evidence states rather than pretending all common fields are equally certain.

### Exact

Directly represented or deterministically recovered from the source/executor.

Examples:

- VGM YM2612 register write at an exact tick;
- SPC DSP SRCN value;
- MIDI note-on message;
- exact BRR bytes at an address;
- known SMPS opcode parsed from a validated driver format.

### Derived

Deterministic or strongly constrained transformation of exact state.

Examples:

- frequency in hertz from YM2612 F-number/block;
- note-like pitch trajectory derived from a chip frequency trajectory;
- content hash identifying the same BRR sample across different SRCN slots;
- a hardware voice's amplitude trajectory from exact envelope and gain state.

### Hypothesis

Interpretation that may have alternatives.

Examples:

- persistent musical-source assignment across hardware-channel reallocations;
- bass/melody/harmony role;
- semantic instrument name;
- phrase function;
- arranger fingerprint;
- one physical source corresponding to one human auditory stream.

Hypotheses carry confidence and competing alternatives. They must never overwrite exact source truth.

## Identity law

The same musical object may occupy different physical locations over time or across files.

```text
physical slot identity
≠ persistent musical identity
```

Examples include:

- GEMS allocating notes to whatever FM channel is currently free;
- the same BRR sample appearing at different SPC SRCN numbers across songs;
- a MIDI instrument being moved to another MIDI channel;
- a tracker instrument being triggered on multiple pattern channels.

Stable identity should use the strongest available combination of:

- exact source object/content identity;
- driver track identity;
- instrument/patch identity;
- temporal continuity;
- control continuity;
- musical relation;
- provenance.

Do not infer persistent identity from channel number alone.

## Time law

All adapters must provide an exact or explicitly qualified time coordinate.

The common model must support:

- discrete events;
- continuous control trajectories;
- simultaneous events;
- sub-frame/sample-accurate timing where the source supports it;
- loops without confusing looped playback time with source-address identity;
- seek/reset/replay provenance.

Do not quantize a high-resolution source merely to fit a MIDI-like event grid.

## Source-specific extensions

The common model is deliberately incomplete.

Each adapter may attach source-specific state that higher layers can inspect on demand.

Examples:

```text
YM2612 event
├ common: pitch trajectory, onset, dynamics, routing
└ source: operator registers, algorithm, feedback, LFO, DAC state

S-DSP voice
├ common: onset, pitch trajectory, sample object, envelope, routing
└ source: SRCN, BRR addresses, ADSR/GAIN, pitch modulation, noise, echo

MIDI note
├ common: onset, pitch, velocity, persistent part candidate
└ source: channel, program/bank, controller state, SysEx/device target
```

Higher-level reasoning should not need a different ontology for every device, but it must be able to descend into device-specific evidence whenever the distinction matters.

## LLM / reasoning interface

Do not stream every chip cycle or audio sample directly into an LLM context.

Expose a hierarchical, queryable representation:

```text
song / object
├ sections / loops
├ persistent musical-source hypotheses
├ instruments / synthesis objects
├ event and control timelines
├ routing/effects
├ acoustic renders / measurements
└ provenance
   └ exact source bytes / commands / addresses on demand
```

The reasoning engine should be able to ask questions such as:

- What is sounding at this instant?
- Which exact source instructions caused it?
- Is this the same instrument used in another track?
- Did the musical object move to another hardware channel?
- Which properties are driver-authored versus device behavior?
- Which parts of this mix are direct sources versus authored effect energy?
- What musical relation persists across prototype/final arrangements?
- What would a listener likely group together, and does libaural agree?

This allows compact reasoning without discarding exact low-level truth.

## libaural bridge

VGM Tooling can generate paired observations unavailable in ordinary recordings:

```text
exact hidden source/performance state
              ↓
     reference acoustic render
              ↓
           libaural
              ↓
 inferred auditory events / streams
```

Because the source state is known, experiments can systematically vary:

- pitch crossings;
- onset synchrony;
- harmonicity;
- timbre similarity;
- common modulation;
- channel allocation;
- spatial routing;
- masking;
- echo/reverberant energy;
- source count;
- ambiguous grouping.

The target is not to teach libaural chip formats. The target is to use executable game music as controlled ground truth for general hearing.

## Rendering relationship

The same common execution model may drive multiple renderers:

```text
                 common execution state
                    /             \
                   /               \
        reference renderer     enhanced renderer
          hardware behavior    source-native quality
                   \               /
                    \             /
                      comparison
```

The enhanced renderer may use richer source knowledge, but it must never rewrite the source-performance model merely to justify an audible change.

## Adapter obligation

A new source adapter is successful when it can answer, as far as the source permits:

1. What exact digital state exists?
2. What changes over time?
3. Which synthesis objects are active?
4. Which events can be represented musically without guessing?
5. Which persistent identities can be proved or strongly supported?
6. What remains source-specific?
7. What must remain uncertain?
8. How does the state produce the reference acoustic output?

Once those questions have stable answers, the higher reasoning layer should not care whether the input began as VGM, SPC, MIDI, SMPS, MDX, a tracker module, or another supported representation.
