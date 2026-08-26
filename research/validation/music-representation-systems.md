# Music representation systems observatory

This research owner records what external music-representation, sequencing, synthesis, analysis, and computer-assisted-composition systems pressure-test in VGM Compiler. These systems are observatories, not runtime dependencies and not sources of project-wide semantic authority.

The governing question is:

> Which distinctions must remain explicit so source-native execution can be related to musical structure without flattening either one?

Detailed OpenMusic evidence lives in [`openmusic-libraries.md`](openmusic-libraries.md). Durable semantic and evidence laws live in [`../../docs/architecture.md`](../../docs/architecture.md). The musical target lives in [`../../docs/musical-understanding.md`](../../docs/musical-understanding.md).

Research conclusions here are candidates for promotion. They become durable project law only when explicitly adopted by a contract under `docs/`.

## Linked representations

No single representation spans the full problem without loss.

```text
authored score / pattern / program
        ↓
scheduler / temporal process
        ↓
instrument definition
        ↓
running voice / instance
        ↓
control trajectories
        ↓
synthesis / DSP graph
        ↓
routing / effects
        ↓
audio stream
```

The inverse path is equally important:

```text
machine execution / audio
        ↓
state + events + timing
        ↓
performed gestures / persistent parts
        ↓
musical relations and structure
        ↓
whole-work interpretation
```

Forward and inverse representations can share vocabulary while retaining different provenance and evidence states.

## Recurrent pressure points

External systems repeatedly expose the value of keeping these boundaries explicit:

```text
IDENTITY
source object
!= instrument definition
!= running voice
!= hardware/software execution slot
!= persistent musical part
!= auditory stream

TIME
source clock
!= scheduler time
!= musical/score time
!= voice-local time
!= sample/audio time

CONTROL
discrete event
!= persistent state
!= continuous trajectory
!= modulation relation

SYNTHESIS
generator / oscillator
!= sample region
!= operator / partial topology
!= envelope / modulation
!= stateful DSP

ROUTING
source route
!= bus / mix relation
!= effect graph
!= output / spatial route

EVIDENCE
exact
!= derived
!= hypothesis
```

When a source does not expose one of these distinctions, absence of evidence must not be converted into a false value.

## Final mix as projection

DAWs, trackers, sequencers, score systems, synthesis languages, and game-music formats expose different slices of causality. A final render cannot replace arrangement, control, synthesis, routing, timing, or source identity when those relations are known.

Likewise, MIDI, notation, piano roll, chord labels, stems, spectral models, and transcriptions can be useful projections without becoming canonical state. The same file format may instead be first-class source evidence when it was actually authored or preserved as such. Evidence role follows provenance, not extension.

## Program is not execution

Procedural and interactive music systems pressure-test another boundary:

```text
static musical program
!= legal future transitions
!= one realized traversal
!= resulting audio
```

Loops, branches, variables, macros, allocation, and interaction can make a program richer than one expanded event list. Where source evidence supports it, control flow and source-program structure should remain separable from captured runtime traces.

## Cross-representation teaching

Different representations may supervise one another only through explicit correspondences.

```text
known symbolic relation
        ↕ alignment
known execution / synthesis relation
        ↓
validated cross-representation mapping
        ↓
stronger inference where one side is missing elsewhere
```

Examples include symbolic tracks teaching persistent-part continuity through hardware allocation, execution traces revealing articulation omitted by notation, and exact patch/sample state constraining timbre or identity hypotheses.

The mapping never proves that an unrelated soundtrack used the supervising format, tool, or historical workflow.

## Analysis projections

Voice separation, rhythm quantization, motif discovery, harmonic analysis, spectral decomposition, and constraint solving create analysis-specific views above retained evidence.

```text
exact evidence
        ↓
analysis feature projection
        ↓
one or more candidate relations
        ↓
validation / comparison
```

A quantized rhythm does not rewrite exact source time. A separated voice does not become a source-proved part. A reconstruction candidate does not become recovered historical truth. Competing candidates may coexist.

## External-system families

The useful observation is the distinction each family pressures, not its syntax or API.

| System family | Research pressure |
| --- | --- |
| score / symbolic systems | explicit parts, voices, durations, hierarchy, transformations |
| trackers / sequencers | executable patterns, effects, loops, control flow, authored timing |
| SuperCollider-style synthesis systems | definition vs running instance vs control vs signal graph |
| Max/Pure Data-style graph systems | event flow vs control state vs signal-rate topology |
| DAWs | arrangement vs automation vs synthesis vs routing vs final mix |
| OpenMusic / CAC systems | higher musical objects, constraints, transformations, alternative analyses |
| MIR / transcription systems | inverse recovery while preserving uncertainty and richer internal evidence |
| spectral / resynthesis systems | partial, envelope, transient, and residual intermediates for bounded reconstruction |

No external family defines the VGM Compiler ontology by itself.

## Admission test

Before proposing an external-system distinction for shared project semantics, ask:

1. What current inference or verification problem does it solve?
2. Which independent source families need the distinction?
3. What information would normalization erase?
4. Can the result descend to its supporting evidence?
5. Is the new representation canonical state, a derived projection, or a hypothesis?
6. Which discriminating test would falsify the proposed generalization?

Research succeeds when it produces sharper obligations, tests, and relations. Promotion into `docs/` is a separate act.