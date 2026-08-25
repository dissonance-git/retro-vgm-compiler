# Music representation systems

## Purpose

This document records durable representation lessons from mature music-production, sequencing, transcription, symbolic-analysis, computer-assisted-composition and perception systems.

These projects are research inputs, not runtime dependencies. The objective is to identify what information must remain explicit so VGM Compiler can reason across composition, executable performance, synthesis, routing, audio, perception and whole-song structure without collapsing them into one representation.

The current question is stronger than “what common data model should we use?”

> What linked representations are required so the system can understand a supported soundtrack as a coherent heard song while remaining able to descend into the exact machine evidence that produced every claim?

## Research-source method

Different mature systems expose different strata:

```text
score / notation
→ authored and structural music

driver / sequencer / tracker
→ executable performance logic

synth / audio language
→ synthesis and control semantics

DAW / routing graph
→ arrangement, automation, signal topology

VGM / SPC / executable rip
→ preserved execution from below

transcription / MIR
→ inverse recovery from audio

music cognition / perception
→ organization after acoustic rendering

historical source / practitioner evidence
→ how composition and technical realization were actually divided or entangled

human musical discourse
→ how listeners, critics, creators, producers and engineers naturally verbalize what they hear, intend, evaluate or would change
```

No single system supplies the VGM Compiler ontology. A concept earns common-model status only when it solves a real cross-source problem without destroying source-specific evidence.

## Broad replay systems: common interfaces do not imply common semantic depth

Modizer, Game Music Emu, OpenMPT, Furnace and related systems demonstrate two complementary facts:

```text
one player surface
!= one execution ontology

common operation
+
source-dependent capability
```

A broad frontend may integrate many engines while retaining substantial engine-specific state. A compact replay API may be useful even when some backends cannot expose richer semantics. Richer sources such as trackers should not be forced down to the weakest backend simply to make every adapter look alike.

Therefore VGM Compiler treats feature/capability availability as evidence:

```text
not exposed
!= absent

unknown
!= false

not applicable
!= unavailable
```

## DAWs and session systems: the final mix is a projection

LMMS, Ardour and related DAW/session systems preserve distinctions among:

- arrangement;
- reusable patterns/regions;
- note/performance material;
- automation;
- instruments;
- mixer/routing topology;
- effects;
- tempo/meter maps;
- persistent session identity.

These distinctions remain useful in executable game music even though the historical authoring workflow may not have used a DAW.

```text
musical material
!= automation
!= synthesis object
!= route / bus
!= effect graph
!= final PCM
```

For attribution, however, this does **not** require arrangement and sound programming to become separate style-fingerprint coordinates. In retro game music, register/voicing/texture decisions and modulation/effects/control decisions often belong to one continuous realization practice. The representation may preserve the underlying technical distinctions while attribution groups them as `ARRANGEMENT / SOUND PROGRAMMING` when that is the historically safer analytical scope.

## Audio graphs: running objects, parameters and topology are different things

AudioKit, SuperCollider, Max/MSP, Pure Data, Cmajor, Faust and related systems reinforce:

- a synthesis definition is distinct from a running instance;
- persistent parameter state is distinct from discrete events;
- control flow is distinct from audio-rate flow;
- graph connections and execution order can affect sound;
- buffers/samples are reusable objects;
- reset/start/stop/bypass state can alter realization without changing musical work identity.

These map naturally onto chip voices, patches, samples, effect paths, buses and source-native enhancement graphs.

## Inverse recovery: export formats are not internal truth

Basic Pitch, VGM/SPC-to-MIDI tools and related inverse systems show that internal evidence can be richer than the eventual export.

```text
audio / device execution
→ activity / trajectory evidence
→ event hypotheses
→ MIDI or notation projection
```

Therefore:

- MIDI export is diagnostic, not canonical;
- continuous pitch/control state should not be quantized merely for export;
- audio-derived events and source-proved events retain different evidence status;
- persistent identities and musical structure should be inferred above retained lower evidence rather than baked into a lossy conversion.

This does not make MIDI unimportant. When MIDI is itself a source artifact, it is first-class source evidence with its own semantics. When MIDI is generated from another source, it is a projection. The same distinction applies to notation, piano-roll views and reconstructed score-like data.

## Symbolic sequence evidence is broader than MIDI

The project uses `symbolic sequence` in a broad musical sense.

```text
MIDI
MML
tracker rows/effects
SMPS / GEMS / N-SPC / SSEQ
validated native driver bytecode
score-like source
reconstructed note/performance events
```

All can expose composer-facing or arranger-facing information such as notes, rests, durations, logical tracks, instruments, articulation, modulation, loops, macro structure, tempo and control flow. Their exact semantics differ, and those differences must be preserved.

A decoded hexadecimal sequence whose command grammar is validated can be more musically explicit than a VGM register trace even though it is not stored in a conventional notation format. Conversely, a VGM or SPC execution trace may preserve articulation, synthesis or allocation behavior that a symbolic transcription omits.

The target is therefore not `convert everything to MIDI`. The target is `recover and align every useful representation of the musical program`.

See `docs/composer-level-understanding.md`.

## Cross-representation teaching without collapse

Different formats should supervise and pressure-test one another wherever a real correspondence can be established.

```text
known symbolic part / note / articulation
        ↕ alignment
known driver / hardware realization
        ↓
learned relation between musical and execution evidence
        ↓
stronger inference when one side is missing elsewhere
```

Examples:

- validated SMPS, GEMS or SSEQ logical tracks can teach what stable part identity looks like after hardware allocation;
- MIDI or score transcriptions can provide note/voice anchors for testing VGM/SPC inverse recovery;
- VGM patch state can reveal timbral or articulation distinctions absent from a score-like transcription;
- SPC BRR/sample-version evidence can reveal source continuity across physical voice reassignment;
- tracker or MML control flow can teach how loops, macros and effects appear in downstream execution.

This is cross-supervision, not historical toolchain inference. A useful mapping learned from one source family does not prove an unrelated commercial soundtrack used that source family.

The common model must preserve distinctions such as:

```text
MIDI track
!= authored voice
!= driver logical track
!= hardware channel
!= physical voice episode
!= persistent musical part
!= auditory stream
```

and:

```text
MIDI program
!= tracker instrument
!= FM patch
!= sample object
!= audible timbral role
```

A correspondence may be many-to-many, time-varying and uncertain. Source-native objects remain intact. Common abstractions are added only where independent systems earn them.

> Everything should be allowed to help everything else, but shared learning must increase understanding without deleting uniqueness.

## Symbolic systems: musical structure exists above note events

Partitura, music21, MEI, Humdrum and related systems show that useful musical reasoning may require explicit:

- parts and voices;
- intervals/durations;
- score and performance time mappings;
- meter and beat hierarchy;
- harmony and voice leading;
- motifs and transformations;
- phrases, sections and form;
- metadata/provenance.

These objects are valuable reasoning structures without becoming device truth or one universal ontology.

## OpenMusic: upper musical objects and transformations

OpenMusic pressure-tests the layer above raw execution. Its useful contribution is the ability to represent musical objects and transformations explicitly while connecting them to later performance/synthesis processes.

The broader library pass also supplied useful controls for:

- persistent voice/stream grouping;
- rhythm quantization without overwriting exact timing;
- motif/contour projections;
- constraint-based reconstruction;
- spectral/resynthesis intermediates;
- candidate orchestration and spatial-scene projections.

See `docs/openmusic-libraries.md`.

## Algorithmic and interactive sequencing: the future need not be a stored event list

Orca, TidalCycles, Antescofo and other procedural/interactive systems show:

```text
static musical program
!= legal future transitions
!= one realized traversal
!= resulting audio
```

A pattern can be executable state. Loops, branches, variables, mutations and player/game input can change future musical behavior without a pre-expanded event list.

This directly supports VGM Compiler's separation of program points/control-flow structure from execution traces.

## Time is plural

Score/performance/audio alignment research and programmable-music systems independently show that authored, driver, device, sample, acoustic and perceptual time are related but not identical.

Mappings may be piecewise because of tempo changes, expressive timing, scheduler behavior, resampling, latency, loops and alignment uncertainty.

Therefore time-domain mappings are evidence objects, not hidden convenience conversions.

## Multiple hypotheses are normal

Music tracking, voice separation, harmonic analysis, listener models and source/version work all demonstrate cases where several interpretations can fit unchanged lower evidence.

VGM Compiler therefore prefers:

```text
lower evidence
        ↓
several explicit hypotheses
        ↓
comparison / later discrimination
```

instead of overwriting evidence to satisfy a projection that demands one answer.

## Current representation model

The durable model is no longer an eight-stage endpoint. It is a linked evidence field with common semantic layers and cross-cutting historical context:

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
↕ cross-cuts artifacts, performances, structures, renders and external evidence
```

These arrows are common relationships, not a mandatory serial pipeline and not a ladder of increasing truth.

The current graph also preserves:

- static program/control flow versus one runtime trace;
- event, persistent value, control and stream distinctions;
- synthesis object versus running voice versus physical slot;
- persistent musical part versus auditory stream;
- explicit cross-domain time mappings;
- exact/derived/hypothesis evidence;
- source-relative feature availability;
- provenance on objects and relations.

## Song-level reasoning is a synchronized projection over the evidence field

The common graph is the substrate, not the final listening-level output.

A whole-song analysis should align relevant layers so one span can be inspected simultaneously as:

```text
source / driver state
+ symbolic note / sequence evidence
+ synthesis / sample / patch state
+ programmed expression
+ physical voice episodes
+ musical events / parts
+ acoustic realization
+ auditory grouping
+ texture / register / density
+ motif / phrase / section / form
+ repetition / loop behavior
+ historical / attribution context
```

This allows the system to reason about the actual music rather than merely list technical features.

Important queries include:

- What changed musically at this section boundary?
- Why does a return sound larger despite similar note material?
- Which exact programmed controls create this heard gesture?
- Where does a musical part migrate among physical voices?
- How do timbre and texture organize the form?
- How does the loop close musically rather than only at a byte address?
- Which symbolic and execution representations agree about this phrase, and where do they disagree?
- Which claims are exact, which are analysis, and which depend on a listener/model?

No new `song` ontology is justified yet. The existing graph and analysis-feature system should first support this as a bounded synchronized projection.

## Human discourse is another projection, not another truth layer

Whole-song understanding is incomplete if the system can only emit technical feature language.

Listeners, critics, composers, producers, mixing/mastering engineers and technical analysts routinely describe the same musical evidence differently. Human musical discourse is strongly metaphorical, relational and purpose-dependent. Words such as `open`, `heavy`, `bright`, `dark`, `punchy`, `floating`, `driving`, `breathing`, `lift`, `glue`, `bite`, `sneaks in`, `drops out` and `comes home` are normal musical language, including among experts.

This does **not** justify a new semantic layer in the graph.

```text
evidence field
        ↓
song-level comparison / interpretation
        ↓
discourse projection
        ├─ ordinary listener
        ├─ reviewer / critic
        ├─ composer / musician
        ├─ producer
        ├─ mixing / mastering engineer
        └─ forensic / technical
```

The discourse renderer should also know what communicative act is being performed:

```text
describe • compare • interpret • evaluate
diagnose • direct • explain • report documented intent
```

These acts have different evidence obligations. Evaluation is not source truth. Creator intent requires documentary evidence. A diagnosis must remain linked to the observations that support it.

### Many-to-many language law

Natural descriptors must not become hard-coded aliases for individual features.

```text
one technical change
→ several possible human descriptions

one human description
← several possible technical causes
```

For example, `it opens up here` might be supported by more parts, higher-register activity, reduced masking, greater spatial spread, more ambience, lower density, longer sustain, a timbral change, or several of these together.

Therefore do not implement:

```text
feature threshold → stock adjective
```

Prefer:

```text
claim / comparison
+ support bundle
+ confidence
+ discourse mode
+ discourse act
+ requested detail
→ natural description
```

The wording is a projection. The support bundle carries the evidence.

### Progressive disclosure

The default user-facing register should resemble a knowledgeable listening companion rather than a telemetry report.

```text
What happens here?
→ It opens up and starts pushing harder.

What changed?
→ A higher part comes in, the bass gets more pointed, and the sound spreads out.

What exactly makes the bass more pointed?
→ descend into driver / envelope / articulation evidence with provenance
```

Natural language must never strengthen the evidence status underneath it. The system should be able to sound human and still descend to exact source truth.

See `docs/human-musical-discourse.md` and `research/cases/human-musical-discourse.md`.

## Role-relative attribution

A single composer-style vector is unsafe. The current analytical coordinates are:

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

The representation may keep arrangement, automation, driver commands and synthesis state technically distinct while attribution groups arrangement and sound programming into one historically safer coordinate.

A strong match in one coordinate must not silently promote another.

## Projection law

MIDI, piano roll, notation, MEI, register dump, stems, chord timelines, perceptual outputs, listener-response views, version/source comparisons, human discourse and LLM/song summaries may all be projections of the evidence state when generated by the system. Some of the same forms, especially MIDI or notation, may instead be original source evidence when ingested. Evidence role depends on provenance, not file extension.

```text
common execution / evidence graph
        │
        ├─→ MIDI / score projection
        ├─→ chip-state projection
        ├─→ audio / stem projection
        ├─→ musical-analysis projection
        ├─→ perceptual projection
        ├─→ listener-response projection
        ├─→ musicological comparison projection
        ├─→ synchronized whole-song reasoning projection
        └─→ human discourse projection
```

No projection becomes canonical merely because it is convenient.

## What the comparison set has established

1. MIDI cannot be the canonical model, but MIDI can be first-class source evidence when it is genuinely present.
2. Symbolic music is broader than MIDI: MML, trackers, native sequence bytecode, driver tracks and reconstructed note events can expose different composer-facing structure.
3. A chronological event list is insufficient.
4. Physical channel identity is not musical identity.
5. Rendered audio is a projection of a larger causal graph.
6. Musical structure requires explicit representation above raw performance.
7. Capture quality and evidence status are different axes.
8. Forward and inverse models should meet where possible.
9. Cross-format supervision is valuable only when source-specific semantics survive the mapping.
10. Higher representations must remain descendable to source evidence.
11. Static control flow is not one realized execution.
12. Cross-domain time must be mapped explicitly.
13. Missing support is not musical absence.
14. Auditory organization is not listener response.
15. Musical similarity is not work/version identity.
16. Technical realization similarity is not composer proof.
17. Whole-song understanding requires synchronized access to several layers rather than another lowest-common-denominator format.
18. Natural musical language is a discourse projection over evidence, not a new truth layer.
19. Human descriptors are many-to-many and must not be reduced to universal feature-to-adjective mappings.
20. The system should normally speak musically first and descend into exact technical mechanism on demand.
21. The final evaluation target is composer-level understanding of how musical decisions cooperate, not successful conversion into any one representation.

Research should continue extracting obligations and tests, not accumulating dependencies.
