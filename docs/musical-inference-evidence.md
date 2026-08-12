# Musical inference and evidence

## Purpose

VGM Tooling studies one musical phenomenon through many source families and research traditions. The project therefore needs a strict answer to a deceptively simple question:

> What can we legitimately know at each layer, and how strongly can we know it?

Detailed archaeology belongs in `research/cases/`. This document records the durable evidence rules that survived comparison across executable game-music sources, music cognition, expressive-performance research, music theory, MIR, digital musicology, synthesis research and practitioner history.

## Central law

Later abstraction is not automatically greater truth.

```text
source representation
        ↓
authored program
        ↓
driver execution
        ↓
synthesis
        ↓
musical performance
        ↓
musical structure
        ↓
acoustic realization
        ↓
auditory interpretation
        ↓
listener response

musicological context
↕ cross-cuts artifacts, structures, performances, renders and external evidence
```

The arrows show common relationships, not a mandatory serial pipeline and not a ladder of increasing truth.

Separately:

```text
exact

derived

hypothesis
```

states how strongly one claim is supported.

And provenance records why the claim is supported, which source/model/observer produced it, what evidence it depends on, and what limitations apply.

Therefore:

```text
semantic layer
!= evidence status
!= provenance
!= source/capture quality
```

The same piece can support exact facts, deterministic derivations and several incompatible hypotheses at once.

## Layer-relative knowledge

### Source representation

Legitimate claims include bytes, file/container structure, addresses, explicit metadata, stored CPU/RAM/DSP state, command/log order, embedded sample/data blocks and source-local identifiers.

An exact source fact is exact **relative to that source artifact**. Capture completeness is a separate question.

### Authored program

When preserved directly, legitimate claims can include notes/rests/ties, written pitch/rhythm, parts/tracks, tempo/meter, instrument/program selection, loops/macros/control flow, articulation and modulation instructions.

Do not reconstruct this layer by fiat from a lower trace merely because an authored representation would be convenient.

### Driver execution

Legitimate claims can include validated logical tracks, scheduler state, sequence position, runtime program points, control flow, allocation policy and one realized execution trace.

```text
static program
!= legal transitions
!= one runtime traversal
```

### Synthesis

Legitimate claims can include patch/sample/synthesis objects, physical synthesis resources, bounded running voice instances, oscillator/operator/sample state, device parameters, routing/effects state and rebuildable device-state views.

Physical resource identity is not automatically musical identity.

### Musical performance

Legitimate claims can include note-like or pitched-activity events, performed timing, pitch/control trajectories, dynamics, articulation, persistent musical parts when supported and higher performance gestures derived from exact programmed controls.

A performance event can be derived from execution without proving authored notation.

### Musical structure

This layer contains analyses such as beat/meter, key/tonal center, harmony, voice leading, motifs, phrases, sections, form, syntax/prolongation, repetition, equivalence, texture and other relations above individual events.

These claims may depend on a theory, representation, segmentation, style or corpus. Several analyses may legitimately coexist.

> Structural analysis must identify the assumptions that make the result meaningful.

### Acoustic realization

Legitimate claims include waveform/spectrum, loudness, spectral envelope, partials, transients, rendered timing, overlap, spatial cues and other measurable properties of the produced sound.

When exact source state exists, audio-derived estimates do not replace it. They are observations of another projection.

### Auditory interpretation

This layer describes how an acoustic realization may be organized perceptually into events and streams.

Candidate claims include concurrent grouping, sequential streams, fusion/segregation, perceived continuity, foreground/background, masking/salience, perceived beat/meter, and pitch/timbre/spatial organization.

These are listener/model claims, not source facts.

### Listener response

`listener_response` is distinct from auditory organization.

It covers expectation, predictive uncertainty, surprise/information content, familiarity, recognition, memory activation, emotion, pleasure, groove/urge to move, motor entrainment, attention and aesthetic judgment under an explicit listener/model context.

```text
same source + same performance + same acoustic realization
can support different listener-response hypotheses
under different learning / cultural / memory / task contexts
```

### Musicological context

`musicological_context` is cross-cutting rather than a final downstream layer.

It includes work/version identity, arrangements/revisions, ports/adaptations, source witnesses, derivation/transmission hypotheses, release chronology, credits, stylistic attribution and archival/catalog context.

Core distinction:

```text
artifact identity
!= musical similarity
!= work/version identity
!= authorship
```

## Evidence states

### Exact

Directly represented or deterministically recovered from a validated source/executor.

Examples:

- exact VGM register write and source tick;
- exact SPC RAM/DSP register image;
- MIDI note-on message;
- explicit MML command;
- validated driver opcode;
- exact event-time BRR bytes under proven RAM-generation continuity;
- exact external catalog annotation relative to that source.

### Derived

Deterministic or strongly constrained transformation of exact state.

Examples:

- device pitch relation from exact register state;
- conservative pitched-activity observation;
- bounded physical voice episode from validated lifecycle boundaries;
- rebuildable device state from ordered transitions;
- exact-to-derived musical gesture when the mapping is explicitly defined;
- structural repetition from validated control flow;
- measured similarity under a declared representation.

### Hypothesis

Interpretation with plausible alternatives.

Examples:

- persistent musical-part assignment without stronger authored/driver identity;
- bass/melody/accompaniment role;
- phrase boundary or harmonic function;
- semantic instrument name;
- listener stream assignment;
- listener-response prediction;
- same-work/derivation relation inferred from similarity;
- role-relative attribution candidate.

Hypotheses carry confidence and provenance and must never overwrite lower exact evidence.

## Capture quality is separate

An observation can be exact relative to an imperfect preservation object.

A VGM or other capture may be:

- complete or incomplete;
- transformed;
- missing initialization/pre-roll;
- affected by logging artifacts;
- externally annotated.

Capture quality and exact/derived/hypothesis status therefore remain orthogonal.

## Identity law

The word `identity` must always be scoped.

```text
artifact identity
!= source-object identity
!= sample/patch identity
!= physical voice episode
!= persistent musical-part identity
!= auditory-stream identity
!= work/version identity
```

The same work may appear in several byte-distinct artifacts. One musical part may move across physical channels. Several physical sources may fuse into one heard stream. None of those relationships is safe to infer from an implementation coordinate alone.

## Programmed expression is not implementation residue

Current practitioner and expressive-performance research makes one boundary especially important:

```text
exact programmed control
!= derived musical gesture
!= higher expressive interpretation
```

A pitch envelope, gate length, volume trajectory, vibrato, detune, duty-cycle change, FM operator change, sample retrigger, rhythmic echo or other programmed behavior can be exact execution evidence and still participate in higher musical expression.

Calling an exact pitch envelope a `scooped attack` is a musical interpretation supported by the envelope. The interpretation does not replace the control history.

This is protected by `tests/model/creative_role_attribution_test.cpp`.

## Timbre and instrument identity

The same exact synthesis object can produce different acoustic descriptors across pitch, dynamics and context. Conversely, perceptually similar sounds can arise from different synthesis parameters or algorithms.

Therefore:

```text
synthesis-object identity
!= acoustic descriptor value
!= perceptual instrument-family label
!= historical acoustic-instrument identity
```

An authored label such as `strings` may be exact at the authored layer. An audio classifier output such as `violin_family` remains a perceptual/model hypothesis. Neither proves one literal reference instrument.

A higher-quality reconstruction remains an acoustic-realization candidate, not recovered historical source truth.

This boundary is protected by `tests/model/timbre_instrument_identity_test.cpp`.

## Musical theory requires theory provenance

Common labels can hide very different epistemic routes.

### Harmony

```text
exact simultaneous pitch state
        ↓
deterministic pitch-class projection
        ↓
chord spelling / root analysis
        ↓
harmonic-function interpretation
        ↓
expectation / tension under a listener model
```

### Meter and groove

```text
explicit authored meter          exact authored
validated driver meter           exact/derived driver
meter inferred from events       structural hypothesis
beat/meter heard from audio      auditory hypothesis
urge to move / groove            listener-response hypothesis
```

No familiar label should collapse those routes.

## Competing interpretations are normal

Music analysis, voice separation, beat/meter tracking, listener models and source/version research all provide cases where several interpretations fit the same lower evidence.

VGM Tooling therefore prefers:

```text
lower evidence
        ↓
several explicit hypotheses
        ↓
comparison / later discrimination
```

rather than selecting one early answer merely because a projection wants a scalar label.

## Cross-cultural scope

Do not mistake familiar Western theory for universal ontology.

No default model may silently assume Western common-practice tonality, major/minor harmony, Roman-numeral function, four-part voice-leading rules, one meter prior, Western consonance preference or one universal scale/chord vocabulary.

Theory/cognition outputs should declare cultural/style/corpus scope when it matters.

## Role-relative attribution

A single `composer fingerprint` is unsafe, especially for retro executable music where composition and technical realization may be divided differently from project to project.

The current analytical coordinates are:

```text
COMPOSITION
melody • rhythm • harmony • form • motivic habits

ARRANGEMENT / SOUND PROGRAMMING
register • voicing • texture • channel roles • modulation • articulation
effects • envelopes • control idioms • machine-specific realization choices

DRIVER / TOOLCHAIN
command grammar • scheduler/allocation behavior • data layout • compiler/driver artifacts

PATCH / SAMPLE DESIGN
FM topology/parameters • waveforms • sample preparation • loop strategy

RENDERING
levels • echo/reverb strategy • mixing • hardware-specific realization
```

`ARRANGEMENT / SOUND PROGRAMMING` is intentionally one coordinate. Historical credits may distinguish arranger, programmer, sound designer and composer, but the analytical fingerprint should not invent a universal boundary where retro workflows often had none.

The same person may occupy several coordinates and several people may contribute to one cue.

```text
strong arrangement / sound-programming match
!= composer proof

strong driver match
!= composition proof

shared patch/sample habits
!= authorship proof
```

`tests/model/creative_role_attribution_test.cpp` protects the narrower executable requirement that technical realization evidence may coexist with an independent composition-style hypothesis while composer attribution remains unresolved.

## Whole-song reasoning

The evidence model exists so VGM Tooling can reason about the song as music without losing the machine underneath it.

At one aligned span, a song-level analysis may combine:

```text
source / driver evidence
+ synthesis / sample / patch state
+ programmed expression
+ physical voice episodes
+ performance events / part hypotheses
+ acoustic measurements
+ auditory grouping
+ texture / role / motif / phrase / section / form
+ loop / repetition behavior
+ historical / attribution context
```

This is a synchronized projection over the evidence field, not a new canonical ontology.

The safe rule is:

> **reason holistically, claim locally.**

A whole-song account may synthesize many layers, but each statement keeps its own evidence scope and provenance.

## Source-relative feature carrier

`model/analysis_feature.h` keeps analysis questions explicit through:

```text
feature name
claim layer
availability
value
evidence status / confidence
provenance
supporting graph nodes / edges
```

Availability is one of:

```text
present
unknown
unavailable
not_applicable
```

A missing feature is not numeric zero or false.

## Current executable controls

Current regressions protect, among other boundaries:

- static program structure versus runtime traversal;
- trace order versus timestamp;
- capture completeness and semantic resynchronization;
- device transitions versus musical-performance observations;
- bounded physical voice episodes versus persistent parts;
- competing persistent-part/voice-separation hypotheses;
- source-relative feature availability across Genesis, SPC and tracker-shaped evidence;
- competing theory-level analyses over unchanged performance evidence;
- competing auditory-stream interpretations over unchanged acoustic evidence;
- different listener-response hypotheses over identical lower evidence;
- artifact identity versus structural similarity versus work/version/history claims;
- cross-cultural pitch/rhythm scope;
- timbre/synthesis identity versus perceptual instrument interpretation;
- technical realization attribution versus composition-style attribution;
- exact programmed controls versus derived musical gestures.

No new generic graph primitive was required by the most recent timbre/attribution/listening passes.

## Related documents

- `README.md`
- `docs/musical-execution-model.md`
- `docs/music-representation-systems.md`
- `docs/persistent-musical-identity.md`
- `docs/openmusic-libraries.md`
- `research/cases/music-cognition-theory-musicology.md`
- `research/cases/music-affect-memory-entrainment.md`
- `research/cases/musicological-version-identity.md`
- `research/cases/timbre-instrument-organology.md`
- `research/cases/retro-composition-programming-listening.md`
