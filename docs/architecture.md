# VGM Compiler architecture

This document owns the shared semantic, provenance, evidence, identity, time, and abstraction laws used across source families.

The musical north star belongs in [`musical-understanding.md`](musical-understanding.md). Current project priority belongs in [`vgm-compiler-roadmap.md`](vgm-compiler-roadmap.md). Source-family investigations and literature belong under `research/`.

## 1. Linked representations

Game music may survive through authored source, native sequence data, register logs, executable rips, machine snapshots, ROM/driver code, samples/patches, rendered audio, transcriptions, and documentary evidence.

These are complementary sensors, not one universal format.

```text
MIDI track
!= driver logical track
!= hardware channel
!= physical voice episode
!= persistent musical part
!= auditory stream
```

```text
MIDI note
!= sequence command
!= device frequency state
!= performed pitch
!= heard pitch
!= notation spelling
```

```text
program/instrument label
!= patch/sample identity
!= acoustic descriptor
!= audible instrument family
!= orchestration role
```

Cross-representation alignment may be many-to-many, time-varying, uncertain, and still useful.

No output representation is canonical merely because it is convenient. MIDI, notation, chord labels, stems, prose, PCM, reconstructed source, and attribution remain projections or claims with their own provenance.

## 2. Source-native semantics first

A shared model begins only after the strongest source-native semantics available to that family are preserved.

- authored symbolic/programming sources may expose notes, durations, logical parts, loops, instruments, articulation, modulation, and control flow directly;
- VGM/VGZ establishes logged device commands/timing but does not automatically recover the original driver track or authored score;
- SPC/NSF/HES/KSS/xSF-style objects may preserve executable state that requires controlled execution before higher semantics are available;
- validated driver/sequence formats may expose stronger logical-track semantics than register logs while still requiring target-device realization;
- rendered audio contributes acoustic/perceptual evidence that exact symbolic or execution state may not contain.

A downstream observation does not erase a stronger upstream fact. Different representations may answer different questions.

## 3. Four orthogonal claim coordinates

Every material claim should keep four coordinates recoverable.

### Semantic layer

Examples include source representation, authored program, driver execution, synthesis/routing, musical performance, musical structure, acoustic realization, auditory interpretation, listener response, and musicological context.

The ordering is explanatory, not a truth ladder.

### Evidence state

```text
exact
  directly represented or deterministically recovered from validated state

derived
  deterministic or tightly constrained transformation of stronger support

hypothesis
  interpretation with plausible alternatives
```

A hypothesis never overwrites its support.

### Provenance

A claim must retain enough support to answer:

```text
which artifact / executor / observer / model / document produced it?
which lower claims does it depend on?
which transformation connected them?
which limitations and alternatives remain?
```

### Availability/capture state

```text
unknown != false
unavailable != absent
not-applicable != unavailable
missing capture != silence
exact relative to one artifact != complete historical preservation
```

Do not fill unavailable evidence with zeros, false booleans, empty labels, or invented source history.

## 4. Identity is scoped

The word `identity` is unsafe without a noun.

```text
artifact identity
!= source-object identity
!= patch/sample identity
!= physical voice episode
!= persistent musical-part identity
!= auditory-stream identity
!= work/version identity
!= authorship identity
```

One work may have many byte-distinct artifacts. One musical part may migrate across physical voices. Several sources may fuse perceptually. Shared patches may appear across unrelated works.

### Persistent musical identity

Hardware resources are not stable musical entities.

Prefer the strongest available continuity evidence:

1. explicit authored part identity;
2. validated driver/logical-track identity;
3. stable exact synthesis/sample/patch identity when musically relevant;
4. control continuity such as pitch/modulation/envelope trajectories;
5. temporal/contour/articulation continuity;
6. pitch/register/overlap constraints;
7. perceptual grouping when the question concerns hearing.

When explicit identity is absent, model candidate successor relations among bounded events/voice episodes and build trajectories from those relations. Do not replace that evidence graph with one universal continuity score.

## 5. Programmed control can be musical evidence

Exact low-level control is not automatically implementation residue.

```text
exact programmed control
!= derived musical gesture
!= expressive interpretation
```

Pitch envelopes, gate behavior, vibrato, detune, duty changes, FM-operator motion, sample retriggers, rhythmic echo, modulation, routing, and dynamic trajectories may be exact execution facts.

A phrase such as `scooped attack` is an interpretation supported by those facts, not a replacement for them.

## 6. Time is plural

Authored, score, driver, device, sample, acoustic, and perceptual time are related but not interchangeable.

Mappings may be piecewise because of tempo changes, loops, scheduler behavior, modulation, resampling, latency, capture gaps, or alignment uncertainty. Preserve time mappings as evidence when later claims depend on them.

Callback boundaries, decoder blocks, and logging chunks are transport unless the source proves otherwise.

## 7. Higher analysis descends to support

Musical understanding is a dependency graph, not a flat feature table and not a universal processing ladder.

Higher analysis may summarize lower evidence but may not silently repair uncertainty, create missing part identity, or bootstrap itself circularly.

Important separations include:

```text
performed pitches != chord spelling != harmonic function
boundary != phrase role
sonority != cadence
section boundary != form
source behavior != listener expectation
musical effect != documented creator intention
similarity != authorship
```

Several analyses may coexist when the evidence underdetermines theory.

Detailed targets for melody, harmony, phrase syntax, counterpoint, form, arrangement, timbre, whole-work structure, soundtrack structure, and creator grammar live in [`musical-understanding.md`](musical-understanding.md).

## 8. Attribution roles remain distinct

Keep creative/technical roles separate:

```text
composition
!= arrangement
!= sequence/sound-data programming
!= driver/engine programming
!= patch/sample design
!= final realization
```

Feature similarity can rank hypotheses. It cannot establish historical authorship by itself.

Documentary evidence is exact relative to the document/witness, not automatically proof of the role being inferred.

## 9. Shared-model admission

A mechanism becomes shared only when materially different source families require the same semantic relation without losing useful native evidence.

```text
shared implementation convenience != shared semantic law
```

A shared container does not imply one runtime. Similar chip families do not imply one register ontology. Two analyzers wanting the same helper does not make that helper a musical universal.

Promote abstractions using agreement and disagreement across independent sources.

## 10. Rendering and transformation

The accurate/reference path remains the scientific control.

Source-native enhancement may relax implementation ceilings only when the adopted musical/instrument identity survives and the intervention is explicit, reversible, and independently testable. See [`source-native-enhanced-rendering.md`](source-native-enhanced-rendering.md).

Spatial presentation consumes source-aware objects without rewriting source authorship. See [`omniphony-realtime-spatial-path.md`](omniphony-realtime-spatial-path.md).

A backend may intentionally lose information, but the loss must be named.

```text
source A
→ semantic model
→ backend B
→ re-analyze B
→ semantic model'
```

Semantic round trips are verification surfaces. Byte identity is required only where the relevant contract says so.

## 11. Cross-project boundary

VGM Compiler owns game-music source semantics, execution/reconstruction, musical analysis, rendering, and source-native playback bridges.

libaural may contribute auditory evidence. Omniphony may consume source-aware spatial objects. Helix may own broader research continuity and cross-project evidence.

Evidence may cross boundaries. Do not copy another project's ontology, durable database, workspace state, or current-status system into this repository.

## 12. Architectural test

For a proposed shared abstraction ask:

1. What exact problem does it solve?
2. Which materially different source families require it?
3. What native distinctions could it erase?
4. Which semantic layer/evidence state/provenance does it carry?
5. What happens when required evidence is unavailable?
6. Can every higher claim descend to support?
7. Is this durable semantic law or merely implementation convenience?

If the answer is unclear, keep the mechanism source-local or research-local until a discriminating test earns promotion.
