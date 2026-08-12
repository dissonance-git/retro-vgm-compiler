# Music representation systems

## Purpose

This document records architectural lessons from mature music-production, sequencing, transcription and symbolic-analysis systems that can improve VGM Tooling's internal model.

These projects are research inputs, not runtime dependencies. Their code is not copied into VGM Tooling. The objective is to identify durable representation obligations and implement the needed mechanisms independently.

The central question is:

> What musical information must remain explicit so that a machine can reason across composition, performance, synthesis, routing, audio and perception without collapsing them into one representation?

## DAW and session systems

### LMMS

LMMS separates several concepts that are easy to conflate in a decoder:

- song-level arrangement;
- reusable patterns;
- piano-roll note material;
- automation;
- instruments;
- mixer channels;
- effect chains;
- MIDI import/export.

Useful lesson:

```text
arrangement
!= pattern
!= note event
!= automation trajectory
!= instrument
!= mixer route
!= effect graph
```

VGM Tooling should preserve these distinctions whenever equivalent structure can be recovered from a driver, tracker, MML source or execution trace.

### Ardour

A mature DAW session contains more than audio and MIDI regions. It must preserve routing, playlists/regions, automation, tempo/meter state, processing order, buses and persistent session identity.

Useful lesson:

- the rendered mix is a projection of a larger session graph;
- tempo and meter mappings are independent objects;
- automation is persistent time-varying state, not a sequence of unrelated events;
- routing and processing topology are part of the musical realization;
- editing structure and acoustic output should remain separable.

For VGM Tooling, an emulated soundtrack can similarly have a larger execution graph than the final PCM mix reveals.

## Audio graph libraries

### AudioKit

AudioKit models audio processing through connected nodes. Nodes expose connections, parameters, runtime state and audio formats, and MIDI events can be scheduled with sample offsets.

Useful lesson:

- a running audio object has identity independent of its parameters;
- graph connections are first-class state;
- parameter state and event scheduling are separate;
- sample-offset scheduling is a legitimate semantic requirement, not merely an implementation detail;
- reset/bypass/start/stop state can affect acoustic realization without changing musical score identity.

These distinctions map naturally onto chip voices, effects, buses and source-native enhancement graphs.

## Inverse musical recovery

### Basic Pitch

Basic Pitch is useful primarily as a comparison for the inverse direction:

```text
audio
-> frame activity
-> onset activity
-> pitch contours
-> note hypotheses
-> MIDI projection
```

Its internal representation is richer than the MIDI it ultimately exports. Continuous pitch contours and overlapping note behavior can exceed what a simple MIDI projection represents cleanly.

Useful lesson:

> An export format is not necessarily the internal reasoning model.

For VGM Tooling:

- VGM-to-MIDI and SPC-to-MIDI are diagnostic projections;
- recovered notes should retain their source trajectories and provenance;
- continuous pitch/control state should not be quantized merely for export;
- an audio-derived event and a source-proved event must retain different evidence status.

The project has an unusual advantage over generic automatic music transcription: when source execution is available, many hidden states that transcription systems must infer can be known exactly.

## Symbolic music processing

### Partitura

Partitura demonstrates a useful score-side temporal model. It preserves score parts and time points containing objects such as notes, rests, measures, voices, staves and time signatures. It also distinguishes raw timeline coordinates from beat-time mappings.

Useful lesson:

- musical time can have multiple simultaneous coordinate systems;
- score objects have intervals, not merely onsets;
- voices and staves are structural identities, not acoustic channels;
- measure/meter structure can coexist with performance timing;
- conversion between time domains should be explicit.

This supports VGM Tooling's decision to keep source, authored, driver, device, sample, acoustic and perceptual time domains separate.

### music21

music21 is a broad computational-musicology toolkit and is useful as evidence that musical reasoning needs structures above individual notes.

Candidate reasoning objects include:

- pitch and interval relations;
- chords and harmonic function;
- key/scale context;
- voices;
- meter;
- phrase and formal structure;
- transformations and equivalence relations.

VGM Tooling does not need to reproduce every musicological feature. It does need a place to represent such analyses without pretending they are device state or auditory-stream truth.

## Harmony corpora

### Free MIDI Chords

Large chord/progression corpora are useful as fixtures rather than architecture.

Potential uses:

- validate chord recognition from exact source notes;
- test transposition invariance;
- test Roman-numeral/function representations;
- distinguish chord identity from voicing;
- test rhythmic variants of the same harmonic progression;
- compare source-proved pitch content with higher-level harmonic hypotheses.

Mood labels or other subjective annotations should remain external annotations, not objective musical truth.

## Algorithmic sequencers

### Orca

Orca is useful because it makes a procedural sequencer explicit rather than hiding it inside a DAW timeline. Operators run on frames or triggers, generate and mutate values, produce Euclidean rhythms, schedule MIDI notes and emit OSC/UDP control.

Useful lesson:

- future musical behavior can be generated rather than stored;
- a pattern can be an executable process;
- control flow and state mutation can be musically meaningful;
- logical frame time can differ from downstream MIDI/device/audio time;
- the same running program can emit several kinds of control endpoints.

This maps to adaptive game-music drivers, MML macros, tracker effects, loops and any executable sequence where the future event list is not fully materialized in the source.

## Musical structure layer

The combined research supports a separate `musical_structure` layer above raw performance events and below or alongside perceptual interpretation.

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

These objects can have different evidence states.

### Exact

Explicitly authored or directly represented.

Examples:

- a time signature in a score source;
- an explicit MML tempo command;
- a driver loop boundary when the driver grammar is known.

### Derived

Deterministically or strongly constrained from lower-level truth.

Examples:

- note pitch from an exact chip frequency trajectory;
- repeated section boundaries from validated driver control flow;
- a transposition relation between two exact note sequences.

### Hypothesis

Analytical interpretation with alternatives.

Examples:

- harmonic function;
- phrase boundary inferred from activity;
- bass/melody role;
- motif identity under substantial transformation;
- stylistic or arranger fingerprint.

Musical analysis must never overwrite the source/performance layer that supports it.

## Projection law

Derived representations are views of the common graph.

Examples:

- MIDI export;
- piano roll;
- notation/score view;
- chord timeline;
- source stem;
- register dump;
- VGM-to-MIDI transcription;
- SPC-to-MIDI transcription;
- libaural auditory-stream output;
- LLM summary.

```text
common execution / musical graph
        |
        +-> MIDI projection
        +-> score projection
        +-> chip-state projection
        +-> audio/stem projection
        +-> harmonic-analysis projection
        +-> perceptual projection
```

No projection becomes canonical merely because it is convenient to inspect.

## Capture-fidelity law

Evidence status and capture quality are orthogonal.

An exact byte or register command can be exact relative to the file being analyzed while the file itself is an incomplete or transformed capture of the original execution.

The common model therefore needs to represent conditions such as:

- runtime capture;
- transformed trace;
- incomplete initialization or pre-roll;
- suspected logging artifact;
- external annotation.

This is important for VGM, emulator logging, ROM extraction and any converted preservation object.

## Resulting common model

The current architecture is converging on a typed temporal graph:

```text
source representation
        ↓
authored program / score / pattern
        ↓
driver and scheduler execution
        ↓
musical performance events / trajectories
        ↓
instrument definitions and synthesis objects
        ↓
running voice instances
        ↓
physical execution slots
        ↓
routing / effect / signal graph
        ↓
acoustic realization
        ↓
auditory interpretation

musical-structure relations can connect the relevant layers
without replacing any of them.
```

The implementation lives in `model/musical_execution_graph.h` and should stay small until real adapters force additional abstractions.

## Sources to continue mining

High-value source classes still include:

- DAW session models and automation;
- symbolic-music analysis systems;
- MusicXML, MEI and Humdrum semantics;
- tracker engines and pattern effects;
- MML dialects and compilers;
- native game sound drivers;
- whole-machine emulators;
- VGM/SPC/MIDI conversion tools;
- automatic music transcription;
- score-informed source separation;
- computational music cognition;
- auditory scene analysis.

Research should extract obligations and test cases, not accumulate dependencies.
