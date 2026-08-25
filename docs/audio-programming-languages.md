# Audio programming languages

Audio and music programming languages are research observatories for VGM Compiler. They are not runtime dependencies and their syntax is not a template for a project-specific language. Their value is architectural: mature systems expose distinctions that remain useful when musical structure becomes timed synthesis and audio.

The governing question is:

> What concepts preserve causality from musical instruction through scheduling, synthesis, control, routing, and signal generation while also supporting inverse recovery from executable game-music sources?

## Execution strata

Do not model a music program as one undifferentiated event stream.

```text
authored score / pattern / program
        ↓
scheduler / temporal process
        ↓
instrument definition
        ↓
voice / instrument instance
        ↓
control trajectories
        ↓
synthesis / DSP graph
        ↓
routing / buses / effects
        ↓
audio stream
```

A source may expose, collapse, or omit any layer. Provenance records which layers are exact, decoded, reconstructed, inferred, or unknown.

The inverse path is equally important:

```text
machine execution
        ↓
state + events + timing
        ↓
performed voices / parts
        ↓
musical structure
        ↓
whole-track interpretation
```

Forward and inverse representations share vocabulary without sharing evidence status.

## Pressure-test systems

### OpenMusic: musical organization

OpenMusic pressure-tests explicit musical objects, relations, constraints, and transformations above execution. It asks whether VGM Compiler can represent voice, rhythm, phrase, motif, and larger organization without disconnecting those hypotheses from executable evidence.

```text
musical object / relation
        ↓
transformation / organization
        ↓
performance or synthesis control
```

The transferable lesson is explicit upper-layer structure, not visual patching syntax.

### SuperCollider: synthesis execution

SuperCollider pressure-tests the distinction between an instrument definition, a running synth/voice instance, control state, synthesis graph, execution order, and buses.

```text
instrument definition
        ↓
running voice instance
        ↓
control state
        ↓
unit-generator graph
        ↓
routing / execution order
```

For VGM Compiler this helps prevent a patch definition, one note episode, a hardware channel, and a persistent musical part from collapsing into one identity.

### Max/MSP: mutable realtime topology

Max/MSP pressure-tests explicit graph topology and the separation of event/message flow, control state, and signal-rate flow.

```text
message / event flow
!= control state
!= signal-rate flow
```

That distinction matters for dynamic routing, feedback, stateful processing, and source graphs whose topology changes while audio is running.

### Structured Audio: synthesis-independent execution contracts

MPEG-4 Structured Audio demonstrates a useful separation between orchestra/synthesis definition, score/control, sample resources, MIDI-style control, and scheduling. The transferable principle is that a common execution contract can describe multiple synthesis families without pretending those synthesis families are identical.

VGM Compiler needs the same abstraction boundary across FM, wavetable, PSG, sample playback, custom DSP, and software synthesis, while retaining source-family-specific state beneath it.

## Common model requirements

The shared execution model should preserve at least these distinctions when the source supports them:

```text
TIME
source clock
scheduler time
voice-local time
audio/sample time
cross-source alignment

IDENTITY
source object
instrument definition
voice instance
hardware/software execution slot
persistent musical part
auditory stream

CONTROL
discrete event
continuous trajectory
automation/state transition
modulation relationship

SYNTHESIS
oscillator / generator
sample region
operator / partial topology
envelope / modulation
stateful DSP

ROUTING
source route
bus / mix relationship
effect graph
output channel / spatial route

PROVENANCE
exact
parsed
derived
inferred
candidate
unknown
```

These are linked representations, not one universal flattened schema.

## Design rules

1. Keep source clocks and machine timing exact. Musical time is a derived projection when it is not explicit.
2. Keep definitions separate from instances. A patch or instrument definition is not the same object as a sounding voice.
3. Keep execution slots separate from musical identity. Channel reuse must not erase persistent-part hypotheses.
4. Keep control-rate and audio-rate behavior distinct when the distinction affects causality.
5. Preserve topology. A route, graph edge, or feedback relation can matter even when two renders sound similar locally.
6. Treat scheduler semantics as evidence. Ordering, quantization, latency, and deferred execution can change musical behavior.
7. Keep synthesis-family details below shared musical abstractions. Common vocabulary must not erase device semantics.
8. Make inverse uncertainty explicit. Recovered musical organization remains a hypothesis unless encoded by the source.
9. Keep rendering replaceable. The evidence model must survive alternate faithful or enhanced renderers.
10. Prefer a graph of typed relationships over a single event list whenever causality crosses layers.

## What VGM Compiler should not become

It should not become a new general-purpose music language merely because music languages are useful research inputs. It should not normalize all sources into MIDI, notation, or one synth graph. It should not let a convenient playback API become the semantic model.

The target is a provenance-preserving execution and understanding system that can move both directions:

```text
source-native execution → heard result → musical understanding
musical hypothesis → supporting execution evidence
```

The language research is successful when it improves those crossings while keeping source truth, execution state, musical inference, and rendering distinct.
