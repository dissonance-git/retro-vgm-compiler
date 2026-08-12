# Audio programming languages

## Purpose

Audio and music programming languages are research inputs for VGM Tooling's common musical execution model.

They are not runtime dependencies and their syntax should not be copied into a new project-specific language merely for novelty. The value is architectural: mature systems reveal which distinctions remain useful when a symbolic or algorithmic musical idea becomes a timed synthesis process and finally audio.

The central question is:

> What concepts are needed to preserve causality from musical instruction through scheduling, synthesis, control and signal flow to acoustic output?

VGM Tooling needs the same concepts in the opposite direction when recovering musical meaning from VGM traces, machine snapshots, driver data and device execution.

## Main lesson

Do not model a music program as one undifferentiated event stream.

A useful common decomposition is:

```text
authored score / pattern / program
        ↓
scheduler / temporal process
        ↓
instrument definition
        ↓
instrument or voice instance
        ↓
control graph / trajectories
        ↓
DSP / synthesis graph
        ↓
routing / buses / effects
        ↓
audio stream
```

Any source may omit or collapse some layers. Provenance must record which layers are explicit, recovered, derived or unknown.

## Three especially useful pressure tests

The current research gives three systems particularly clear roles in VGM Tooling. They are not proposed dependencies.

### OpenMusic: upper musical structure

OpenMusic belongs primarily in `docs/music-representation-systems.md`, but it completes the comparison here by pressure-testing the layer above raw execution.

It asks whether VGM Tooling can preserve explicit musical objects, relations and transformations such as voice, rhythm, phrase and higher structure while retaining the route back to executable evidence.

```text
musical object / structure
        ↓
transformation / organization
        ↓
performance or synthesis control
```

The lesson is not to copy a visual composition environment. It is to ensure the common graph has somewhere honest to put musical structure that is neither a device register nor a perceptual guess.

### SuperCollider: synthesis and running execution

SuperCollider pressure-tests the middle of the graph:

```text
instrument definition
        ↓
running synth / voice instance
        ↓
control state
        ↓
unit-generator / synthesis graph
        ↓
buses and execution order
```

This is useful for distinguishing an instrument definition from one instantiated voice, and both from the physical or software slot currently executing them.

### Max/MSP: mutable realtime signal and control topology

Max/MSP pressure-tests realtime graph semantics:

```text
message / event flow
≠ control state
≠ signal-rate flow
```

It is useful for asking whether the common model can represent explicit topology, stateful processing, feedback, routing and graph mutation without reducing the result to a fixed list of notes or a final PCM bus.

Together, these systems cover three different strata:

```text
OpenMusic      → musical organization
SuperCollider → synthesis execution
Max/MSP       → realtime control / signal topology
```

They are research observatories over the same larger process, not a three-part VGM Tooling runtime stack.

## Structured Audio precedent

MPEG-4 Structured Audio is a particularly close historical precedent.

Its toolset separates:

- SAOL, an orchestra language for synthesis and DSP algorithms;
- SASL, a score/control language for instrument instantiation and control;
- a sample-bank format;
- MIDI control semantics;
- a scheduler that maps structural control into real-time sound generation.

This is important because it standardizes not one synthesis method but a way to describe synthesis methods. FM, sampling, physical modelling and other synthesis systems can live under the same execution framework.

VGM Tooling extends this idea in a different direction. It must support both forward execution and inverse recovery:

```text
symbolic / structured source
        ↓
execution model
        ↓
reference audio

legacy trace / snapshot / executable state
        ↓
recovered execution model
        ↓
musical reasoning
```

The internal model should therefore describe synthesis and scheduling without assuming one chip family or one source syntax.

## Time and scheduling

### ChucK

ChucK makes logical time a language-level concept. Time and duration are native values and concurrent programs advance explicit logical time.

Useful lesson:

- time is part of semantics, not incidental metadata;
- simultaneous processes can share one deterministic temporal world;
- event identity and execution order must survive concurrency;
- a musical reasoning model needs exact temporal coordinates plus causal ordering.

VGM Tooling should be able to distinguish source time, logical performance time, device time, sample time and loop-local time when those differ.

### TidalCycles

Tidal models a pattern approximately as a query from a timespan to events active during that span. It uses rational cyclical time and supports transformations that alter temporal structure while retaining a compositional relationship to the original pattern.

Useful lesson:

- a pattern need not be stored as a flat list of notes;
- repeated and transformed structures can remain symbolic;
- event intervals matter, not only point onsets;
- continuous and discrete control patterns can share temporal structure;
- musical repetition, rotation, stretching and subdivision are transformations over time rather than duplicated event data.

This is relevant to driver loops, tracker patterns, MML loops, SMPS sequences and repeated VGM structures.

## Multiple execution rates

### Csound

Csound distinguishes initialization-time, control-rate and audio-rate computation and separates instrument definitions from score events.

Useful lesson:

```text
configuration / initialization
≠ musical/control trajectory
≠ audio-rate signal
```

VGM Tooling already encounters the same distinction in another form:

- one-time chip or driver initialization;
- note/envelope/modulation/control updates;
- continuously evolving oscillator/sample/DSP state.

The common model should preserve an update-rate or temporal-domain concept rather than pretending every value is the same kind of event.

## Synthesis and routing graphs

### SuperCollider

SuperCollider separates the client language from the synthesis server. Synth definitions describe connected unit generators, running synths exist as nodes, node trees define execution order, and audio/control buses connect running objects.

Useful lesson:

- instrument definition and running instance are different identities;
- a synthesis object can itself be a graph;
- graph topology and execution order are part of the acoustic result;
- buses and effects are first-class routing objects rather than properties attached to notes;
- buffers are reusable source/state objects independent of any one note instance.

This maps cleanly onto FM patches/operators, sample objects, QSound/effect paths, module partials and device mixers.

### Faust

Faust treats DSP programs as mathematical signal processors expressed through block-diagram composition. Its compiler has a semantic stage that produces signal expressions before target-specific code generation.

Useful lesson:

- DSP meaning can have an intermediate representation independent of implementation language or target platform;
- graph composition can be reasoned about before rendering samples;
- a signal-producing mechanism is not identical to its generated machine code;
- deterministic DSP graphs are suitable for provenance and equivalence testing.

VGM Tooling should similarly distinguish a recovered synthesis graph from the emulator implementation used to execute it.

### Cmajor

Cmajor explicitly distinguishes endpoint kinds such as streams, events and values between processors and graphs.

Useful lesson:

```text
stream
≠ event
≠ persistent value/state
```

This distinction is highly useful for game-music execution:

- PCM/audio and continuous modulation are streams;
- note-on, key-on and register writes can be events;
- patch parameters, routing state and configuration may be persistent values.

The project should avoid forcing these into one event representation.

### Max/MSP / Pure Data

Visual dataflow languages distinguish message/control flow from signal-rate connections and make processing topology explicit.

Useful lesson:

- graph topology can be a primary representation;
- control edges and signal edges are semantically different;
- stateful processing objects and feedback paths need explicit identity;
- routing, spatial and effect processing can be represented before final summation;
- realtime graph changes may alter execution without changing the identity of the underlying musical source.

The project does not need their visual UI model to benefit from their graph semantics.

## Symbolic and notation languages

MML, Alda, ABC, LilyPond and related score languages represent musical intention at a substantially higher level than device traces.

They help identify concepts such as:

- notes, rests and ties;
- duration and meter;
- parts/voices;
- articulation;
- tempo;
- repeats and hierarchy;
- instrument selection;
- notation-specific structure.

MML is especially important because many dialects also contain device-specific synthesis and driver instructions. It can therefore span both common musical semantics and chip realization semantics in one authored representation.

Do not assume notation truth is performance truth. A written note, a driver event, a synthesis voice and a heard auditory stream remain separate identities.

## Live coding and dynamic execution

ChucK, SuperCollider, TidalCycles, Sonic Pi, Extempore and related systems demonstrate that a musical program can change while its temporal world continues running.

Useful lesson:

- program identity can persist through live mutation;
- the state before and after a mutation needs a causal boundary;
- scheduled future behavior and currently sounding state are distinct;
- reasoning about music may require both program structure and execution history.

This is relevant to game drivers that rewrite state dynamically, adaptive music and any future interactive formats.

## Proposed common execution primitives

The language comparison supports the following source-independent primitives.

### Time

- exact timestamp or qualified time coordinate;
- duration / interval;
- causal ordering;
- loop/cycle identity;
- update rate or temporal domain.

### Program structure

- part / logical process;
- sequence or pattern;
- loop / branch / transformation;
- scheduler event;
- program mutation where applicable.

### Synthesis

- instrument definition;
- synthesis object;
- running voice/instance;
- operator/partial/oscillator/sample subobject;
- parameter state;
- continuous control trajectory.

### Graph

- node;
- typed edge;
- event edge;
- control/value edge;
- signal/audio edge;
- bus;
- effect/routing node;
- buffer/sample object.

### Acoustic output

- isolated source contribution where obtainable;
- mixed bus contribution;
- reference output;
- enhanced output;
- exact render provenance.

These primitives do not replace source-specific extensions. A YM2612 operator, S-DSP BRR voice and MT-32 partial remain different source objects even when they implement analogous roles in the common graph.

## Reasoning consequence

The LLM-facing representation should be graph-like and hierarchical rather than a giant chronological register dump.

For example:

```text
musical part
├ event/pattern history
├ instrument definition
│  └ synthesis graph
├ running voice instances
│  ├ control trajectories
│  └ physical device allocation
├ routing/effects graph
├ acoustic contributions
└ provenance
   └ exact source commands / bytes / addresses
```

A query can descend only as far as needed.

This supports questions such as:

- Which program event caused this heard note?
- Is this control change part of the instrument definition or the performance?
- Did an instrument remain the same while its physical channel changed?
- Which effect node created this energy?
- Are two files different performances of the same program structure?
- Does a repeated VGM command region correspond to a true musical loop or only repeated device behavior?

## Validation opportunities

Audio programming languages create useful forward-reference tests.

### Symbolic-to-execution round trip

```text
MML / score / pattern
→ known compiler or scheduler
→ known synth/device
→ execution trace
→ VGM Tooling recovery
```

Compare the recovered model with the authored source.

### Same program, different synthesis

Hold score/pattern structure fixed while changing the synthesis engine.

Test what remains one musical identity through FM, sample, wavetable, module or software-synth realizations.

### Same synthesis graph, different implementation

Run an equivalent synthesis graph through two independent implementations and compare the recovered graph and acoustic result.

This can separate semantic identity from emulator/library implementation details.

### Perceptual bridge

Use exact structured state to generate controlled audio, then ask libaural to recover the perceptual organization from audio alone.

The forward program becomes an answer key without requiring libaural to know the source language.

## Research sources

Important comparison systems include:

- MPEG-4 Structured Audio: SAOL, SASL, sample banks and scheduler;
- ChucK;
- SuperCollider;
- Csound / MUSIC-N lineage;
- Faust;
- Cmajor;
- Max/MSP and Pure Data;
- TidalCycles and related pattern languages;
- Sonic Pi;
- Extempore / Impromptu;
- MML dialects and compilers;
- Alda;
- ABC notation;
- LilyPond;
- OpenMusic as the adjacent upper-level musical-object comparison;
- future language families that expose materially different semantics.

Use primary documentation and implementation sources where possible. Literature and other repositories are conceptual/reference inputs only. VGM Tooling's common implementation remains project-owned code.
