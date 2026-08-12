# Music representation systems

## Purpose

This document records architectural lessons from mature music-production, sequencing, transcription, symbolic-analysis and computer-assisted-composition systems that can improve VGM Tooling's internal model.

These projects are research inputs, not runtime dependencies. Their code is not copied into VGM Tooling. The objective is to identify durable representation obligations and implement the needed mechanisms independently.

The central question is:

> What musical information must remain explicit so that a machine can reason across composition, performance, synthesis, routing, audio and perception without collapsing them into one representation?

## Research-source method

The comparison set is intentionally broad because different systems expose different strata of one musical process.

```text
score / notation system
→ exposes authored and structural music

driver / sequencer / tracker
→ exposes executable performance logic

synth / audio language
→ exposes synthesis and control semantics

DAW / routing graph
→ exposes arrangement, automation and signal topology

VGM / SPC / executable rip
→ exposes preserved execution from below

transcription / MIR system
→ exposes inverse recovery from audio

music cognition / perception
→ exposes organization after acoustic rendering
```

No one system is expected to supply the VGM Tooling ontology. The useful question is which distinctions repeatedly survive when the same phenomenon is viewed from several of these strata.

A concept earns common-model status only when it solves a real cross-source problem without destroying source-specific evidence.

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
→ frame activity
→ onset activity
→ pitch contours
→ note hypotheses
→ MIDI projection
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

### MEI

The Music Encoding Initiative is useful from a different direction: it treats musical notation and related structure as a richly encoded document rather than assuming a flat note list is sufficient.

Useful lesson:

- notation is itself structured data with hierarchy and relationships;
- score identity, written structure and performed realization should remain distinguishable;
- metadata and provenance can travel with musical objects without becoming the musical objects themselves;
- interchange formats can preserve substantial structure while still remaining projections rather than universal internal truth.

MEI is therefore a valuable comparison target for score-facing export/import and provenance questions. VGM Tooling should not force executable game-music state into an MEI-shaped ontology.

## Computer-assisted composition

### OpenMusic

OpenMusic is useful because it treats composition and musical analysis as transformations over explicit musical objects rather than merely as playback commands.

For VGM Tooling, the important comparison is not the visual interface. It is the ability to reason about objects and relations above individual events while still connecting them to executable or acoustic processes.

Useful lessons include:

- musical structure can itself be manipulated as data;
- pitch, rhythm, voice, phrase and transformation relations may need explicit objects above raw performance events;
- one musical object can have several useful projections without becoming several unrelated identities;
- analysis and generation can share representations while remaining distinct operations;
- symbolic structure can drive later synthesis without becoming identical to synthesis state.

This makes OpenMusic a useful pressure test on the upper half of the execution graph:

```text
musical object / structure
        ↓
transformation / organization
        ↓
performance or synthesis control
        ↓
acoustic realization
```

VGM Tooling should be able to recover or represent the upper objects when the evidence supports them, but must retain the lower source/driver/device route that justifies each recovered claim.

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
- MEI or another interchange projection;
- chord timeline;
- source stem;
- register dump;
- VGM-to-MIDI transcription;
- SPC-to-MIDI transcription;
- libaural auditory-stream output;
- LLM summary.

```text
common execution / musical graph
        │
        ├─→ MIDI projection
        ├─→ score / notation projection
        ├─→ chip-state projection
        ├─→ audio / stem projection
        ├─→ harmonic-analysis projection
        └─→ perceptual projection
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

## What the current comparison set has established

The present research corpus supports several durable constraints:

1. **MIDI cannot be the canonical model.** It is a useful projection but loses source-specific synthesis and some continuous-control structure.
2. **A chronological event list is insufficient.** Persistent values, trajectories, streams, graphs and executable patterns are distinct kinds of state.
3. **Physical channel identity is not musical identity.** Logical parts and synthesis objects may move, split or fuse across implementation slots.
4. **Rendered audio is a projection.** Routing, automation, effects and source identity can exist before final summation.
5. **Musical structure requires a layer above raw performance.** Harmony, meter, phrase, motif and form should not be stuffed into device state.
6. **Source quality and evidence status are different axes.** An exact observation can still come from an incomplete capture.
7. **Forward and inverse models should meet where possible.** Authored/known execution can become an answer key for recovery from traces or audio.
8. **The common model must stay descendable.** Every higher representation must retain a route back to the source bytes, commands, addresses, states or annotations that support it.

These are stronger than any one repository's data model because they have survived comparison across several independent source families.

## Sources to continue mining

High-value source classes still include:

- DAW session models and automation;
- symbolic-music analysis systems;
- MusicXML, MEI and Humdrum semantics;
- OpenMusic and related computer-assisted composition environments;
- tracker engines and pattern effects;
- MML dialects and compilers;
- native game sound drivers;
- whole-machine emulators;
- broad multi-engine players and integration layers;
- VGM/SPC/MIDI conversion tools;
- automatic music transcription;
- score-informed source separation;
- computational music cognition;
- auditory scene analysis.

Research should extract obligations and test cases, not accumulate dependencies.
