# Musical execution model

## Purpose

VGM Tooling supports radically different digital music representations without forcing their source semantics into one file format.

The stable common abstraction begins **after source-specific parsing or execution**, while retaining routes back to the exact source representation.

```text
MML / authored source ─────┐
VGM register log ──────────┤
SPC machine snapshot ──────┤
MIDI event stream ─────────┤
SMPS / GEMS / MDX ─────────┤
tracker/module data ────────┤
ROM-derived driver data ────┘
              │
              ▼
source-specific parser / compiler / executor
              │
              ▼
exact source / execution / synthesis state
              │
              ▼
provenance-aware musical execution graph
              │
       ┌──────┼───────────┐
       ▼      ▼           ▼
 reasoning  rendering   libaural / forensics
```

Do not normalize the source representation prematurely. Normalize **meaning after execution**.

The objective is for higher-level reasoning to ask the same musical questions regardless of whether the evidence arrived as an authored MML command, YM2612 registers, an SPC700 RAM snapshot, MIDI messages, a tracker pattern, or a known driver sequence.

The common model must remain descendable to exact source evidence whenever a distinction matters.

## Research basis

The model is being pressure-tested against many independent systems that expose different strata of the same musical phenomenon:

- authored music languages and notation;
- game drivers and sequencers;
- logged device execution;
- executable machine-state formats;
- trackers and MIDI/module synthesis;
- whole-machine emulators and broad multi-engine players;
- audio programming and DSP languages;
- DAWs and routing/session graphs;
- symbolic-music and score systems;
- transcription and inverse-recovery systems;
- music cognition and auditory-scene research.

No one reference system owns the architecture. Common abstractions survive only when they solve real problems across several source families without erasing useful native state.

See `docs/music-representation-systems.md`, `docs/audio-programming-languages.md`, and `docs/upstreams.md`.

## Source classes

Use established preservation terminology where practical.

### Authored symbolic / programming representations

Examples: MML dialects, score/program source, tracker source material and other composer-facing symbolic systems.

Potential evidence:

- explicit notes/rests/ties;
- duration, tempo and meter;
- parts/tracks/voices;
- loops, macros, branches and other program structure;
- instrument/program selection;
- articulation, modulation and automation;
- chip- or device-specific synthesis instructions.

Authored truth does not automatically prove the exact later acoustic realization. Compiler, driver and target device remain part of the route.

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

Examples: SMPS, GEMS, N-SPC, MDX, PMD and other known music-engine data formats.

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

## Semantic layers

The implemented common graph keeps these layers distinct.

### 1. Source representation

Exact bytes, commands, memory locations, files, addresses, driver objects, annotations and native source timing.

This layer is never replaced by an inferred musical description.

### 2. Authored program

Composer-/programmer-facing musical structure when it survives or is directly available.

Examples:

- MML commands;
- score/pattern objects;
- explicit parts and loops;
- authored tempo or meter;
- program/instrument selection;
- authored control flow or transformations.

This layer may be unavailable for trace-only sources.

### 3. Driver execution

What the sequencer, driver or executable music program is doing while the source runs.

Examples:

- CPU/driver track state;
- sequence position;
- scheduler events;
- logical-track state;
- hardware-channel allocation;
- sample pointers;
- driver loops and branches;
- clock/timing state.

### 4. Synthesis

Objects and state that generate or transform sound.

Examples:

- FM patch/operator graph;
- BRR/PCM/ADPCM sample object;
- PSG oscillator/noise source;
- wavetable;
- MIDI-module partial/patch where known;
- echo/reverb/effect object;
- QSound source and spatial-processing state;
- routing buses and signal-processing topology.

A hardware slot is not an object identity. Use stable content/state identity where possible.

### 5. Musical performance

Source-independent musical operations supported by source execution.

Candidate objects include:

- note/event onset and release;
- continuous pitch trajectory;
- dynamics/amplitude trajectory;
- articulation/envelope trajectory;
- persistent musical voice/part;
- instrument identity or instrument hypothesis;
- rhythmic event;
- authored spatial/routing trajectory;
- effect-send trajectory.

This is **not MIDI conversion**. Continuous controls, non-note sounds, FM/noise behavior, sample triggering, effects and executable patterns remain first-class.

### 6. Musical structure

Relations above individual performance events when the evidence supports them.

Candidate objects include:

- meter and beat hierarchy;
- tempo maps;
- chord/harmony relations;
- key/scale context;
- melodic contour;
- rhythmic pattern;
- phrase/section/form;
- motif and transformation relations;
- voice-leading relations;
- repeated or equivalent structures.

Some may be authored, some deterministically derived, and some analytical hypotheses. Musical structure never overwrites the lower performance or source layers.

### 7. Acoustic realization

The sound produced by a reference or enhanced renderer.

Keep source contributions available before final summation where practical. A rendered waveform is a projection of the larger execution graph, not the canonical identity of that graph.

### 8. Auditory interpretation

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

## Typed temporal graph

The implementation in `model/musical_execution_graph.h` is deliberately small but already encodes several distinctions that repeatedly appeared across the research corpus.

### Flow kinds

```text
event
→ discrete occurrence: key-on, note, trigger, scheduler event, register write

value
→ persistent state: patch, routing, configuration

control
→ time-varying parameter/control relationship

stream
→ continuous output such as PCM or audio-rate signal
```

Do not force all four into one chronological event representation.

### Object kinds

The current graph can represent objects such as:

- source objects;
- sections and patterns;
- parts;
- musical events and relations;
- instrument definitions;
- synthesis objects;
- running voice instances;
- physical execution slots;
- parameters and sample buffers;
- effects and buses;
- acoustic contributions;
- auditory events/streams;
- projections.

### Relation kinds

Current relations include concepts such as:

- contains;
- causes;
- schedules;
- instantiates;
- realizes;
- occupies;
- controls;
- routes to;
- transforms;
- contributes to;
- groups into;
- derived from;
- same identity as;
- repeats;
- projects to.

These relations are intentionally more informative than a generic edge and allow one exact object to participate in several representations without duplication.

## Evidence status

Every transition upward must preserve how the claim was obtained.

### Exact

Directly represented or deterministically recovered from the source/executor.

Examples:

- VGM YM2612 register write at an exact tick;
- SPC DSP SRCN value;
- MIDI note-on message;
- exact BRR bytes at an address;
- known SMPS opcode parsed from a validated driver format;
- explicit MML tempo or note command.

### Derived

Deterministic or strongly constrained transformation of exact state.

Examples:

- frequency in hertz from YM2612 F-number/block;
- note-like pitch trajectory derived from a chip frequency trajectory;
- content hash identifying the same BRR sample across different SRCN slots;
- a hardware voice's amplitude trajectory from exact envelope and gain state;
- a repeated section relation from validated control flow.

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

## Capture quality is a separate axis

Evidence status and capture fidelity are orthogonal.

A register write can be exact with respect to a VGM file while the file itself may be:

- a runtime capture;
- transformed;
- incomplete;
- missing initialization or pre-roll;
- affected by a suspected logging artifact;
- supplemented by an external annotation.

The graph therefore preserves capture/provenance flags independently of exact/derived/hypothesis status.

## Identity law

The same musical object may occupy different physical locations over time or across files.

```text
musical event / part
        ↓ realizes through
synthesis object / voice
        ↓ occupies
physical execution slot
        ↓ produces
acoustic contribution
        ↓ may become
auditory event / stream
```

Every arrow can be one-to-one, one-to-many, many-to-one or time-varying.

Examples include:

- GEMS allocating notes to whatever FM channel is currently free;
- the same BRR sample appearing at different SPC SRCN numbers across songs;
- a MIDI instrument being moved to another MIDI channel;
- a tracker instrument being triggered on multiple pattern channels;
- one module note expanding into several synthesis partials;
- several physical sources perceptually fusing into one stream.

Stable identity should use the strongest available combination of:

- authored identity;
- driver-track identity;
- exact source object/content identity;
- instrument/patch identity;
- temporal continuity;
- control continuity;
- musical relation;
- provenance.

Do not infer persistent identity from channel number alone.

## Time law

All adapters must provide an exact or explicitly qualified time coordinate.

The common model must support multiple time domains, including where applicable:

- source time;
- authored/score time;
- driver/scheduler time;
- device time;
- sample time;
- acoustic time;
- perceptual time.

It must also support:

- discrete events;
- intervals and durations;
- continuous control trajectories;
- simultaneous events;
- causal ordering;
- sub-frame/sample-accurate timing where the source supports it;
- loops without confusing looped playback time with source-address identity;
- seek/reset/replay provenance.

Do not quantize a high-resolution source merely to fit a MIDI-like event grid.

## Source-specific extensions

The common model is deliberately incomplete.

Each adapter may attach source-specific state that higher layers can inspect on demand.

Examples:

```text
YM2612 object
├ common: pitch/control trajectory, onset, dynamics, routing
└ source: operator registers, algorithm, feedback, LFO, DAC state

S-DSP voice
├ common: onset, pitch trajectory, sample object, envelope, routing
└ source: SRCN, BRR addresses, ADSR/GAIN, pitch modulation, noise, echo

MIDI note
├ common: onset, pitch, velocity, persistent part candidate
└ source: channel, program/bank, controller state, SysEx/device target
```

Higher-level reasoning should not need a different ontology for every device, but it must be able to descend into device-specific evidence whenever the distinction matters.

## Projection law

MIDI, notation, piano roll, register dump, source stems, chord timelines and LLM summaries are **projections** of the graph.

```text
musical execution graph
        │
        ├─→ MIDI projection
        ├─→ notation / score projection
        ├─→ chip-state projection
        ├─→ stem / audio projection
        ├─→ musical-analysis projection
        └─→ perceptual projection
```

A projection may be useful, standardized or lossless for a declared obligation without becoming the canonical internal model.

## LLM / reasoning interface

Do not stream every chip cycle or audio sample directly into an LLM context.

Expose a hierarchical, queryable representation:

```text
song / object
├ sections / loops / patterns
├ persistent musical-source hypotheses
├ instruments / synthesis objects
├ event and control timelines
├ musical-structure relations
├ routing / effects
├ acoustic renders / measurements
└ provenance
   └ exact source bytes / commands / addresses on demand
```

The reasoning engine should be able to ask questions such as:

- What is sounding at this instant?
- Which exact source instructions caused it?
- Is this the same instrument used in another track?
- Did the musical object move to another hardware channel?
- Which properties are authored, driver-generated, or device behavior?
- Which parts of this mix are direct sources versus authored effect energy?
- What musical relation persists across prototype/final arrangements?
- Is a repeated region a true program/section loop or only similar device behavior?
- What would a listener likely group together, and does libaural agree?

This allows compact reasoning without discarding exact low-level truth.

## Forward / inverse validation

The strongest validation cases connect both directions.

```text
authored symbolic source
→ known compiler / driver / scheduler
→ known device or synth
→ execution trace
→ VGM Tooling recovery
→ common model
```

The recovered model can then be compared with the authored source at the layers both sides actually support.

This is stronger than comparing one converter with another because the forward path supplies an independent answer key.

Useful controls include:

- MML → compiler/driver → device trace;
- known driver sequence → device execution;
- MIDI → known module → internal partials/audio;
- tracker pattern → engine execution;
- equivalent synthesis graph → two independent renderers;
- known structured source → audio → generic transcription, compared with source truth.

A finite successful round trip proves only the declared representation/obligation, not universal equivalence.

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
2. What authored/program structure survives?
3. What changes over time and in which time domain?
4. Which scheduler/driver operations are explicit or recoverable?
5. Which synthesis objects are active?
6. Which events can be represented musically without guessing?
7. Which persistent identities can be proved or strongly supported?
8. Which higher musical structures are exact, derived, hypothetical or unavailable?
9. What remains source-specific?
10. What must remain uncertain?
11. How does the state produce the reference acoustic output?

Once those questions have stable answers, higher reasoning should not care whether the input began as VGM, SPC, MIDI, MML, SMPS, GEMS, MDX, a tracker module, a whole-machine rip or another supported representation.

It should still be able to descend back into the native evidence whenever it needs to know why.
