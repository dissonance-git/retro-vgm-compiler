# Music representation systems

## Purpose

This document records durable representation lessons from mature music-production, sequencing, transcription, symbolic-analysis, computer-assisted-composition and perception systems.

These projects are research inputs, not runtime dependencies. The objective is to identify what information must remain explicit so VGM Tooling can reason across composition, executable performance, synthesis, routing, audio, perception and whole-song structure without collapsing them into one representation.

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
```

No single system supplies the VGM Tooling ontology. A concept earns common-model status only when it solves a real cross-source problem without destroying source-specific evidence.

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

Therefore VGM Tooling treats feature/capability availability as evidence:

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

This directly supports VGM Tooling's separation of program points/control-flow structure from execution traces.

## Time is plural

Score/performance/audio alignment research and programmable-music systems independently show that authored, driver, device, sample, acoustic and perceptual time are related but not identical.

Mappings may be piecewise because of tempo changes, expressive timing, scheduler behavior, resampling, latency, loops and alignment uncertainty.

Therefore time-domain mappings are evidence objects, not hidden convenience conversions.

## Multiple hypotheses are normal

Music tracking, voice separation, harmonic analysis, listener models and source/version work all demonstrate cases where several interpretations can fit unchanged lower evidence.

VGM Tooling therefore prefers:

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
- Which claims are exact, which are analysis, and which depend on a listener/model?

No new `song` ontology is justified yet. The existing graph and analysis-feature system should first support this as a bounded synchronized projection.

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

MIDI, piano roll, notation, MEI, register dump, stems, chord timelines, perceptual outputs, listener-response views, version/source comparisons and LLM/song summaries are projections of the evidence state.

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
        └─→ synchronized whole-song reasoning projection
```

No projection becomes canonical merely because it is convenient.

## What the comparison set has established

1. MIDI cannot be the canonical model.
2. A chronological event list is insufficient.
3. Physical channel identity is not musical identity.
4. Rendered audio is a projection of a larger causal graph.
5. Musical structure requires explicit representation above raw performance.
6. Capture quality and evidence status are different axes.
7. Forward and inverse models should meet where possible.
8. Higher representations must remain descendable to source evidence.
9. Static control flow is not one realized execution.
10. Cross-domain time must be mapped explicitly.
11. Missing support is not musical absence.
12. Auditory organization is not listener response.
13. Musical similarity is not work/version identity.
14. Technical realization similarity is not composer proof.
15. Whole-song understanding requires synchronized access to several layers rather than another lowest-common-denominator format.

Research should continue extracting obligations and tests, not accumulating dependencies.
