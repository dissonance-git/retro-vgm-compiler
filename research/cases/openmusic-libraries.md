# OpenMusic library research

## Status

Research input, not a runtime dependency plan.

OpenMusic itself was already part of the VGM Tooling research stack before this pass. The purpose of this case is to preserve the newly mined **OpenMusic library ecosystem** that had not yet been examined in comparable depth.

The question is not whether VGM Tooling should embed OpenMusic. It is:

> Which established OpenMusic libraries expose musical-analysis, representation, constraint, synthesis, resynthesis, orchestration, and spatialization concepts that pressure-test the current provenance-aware musical execution model?

The durable rule remains:

```text
exact source / driver / device evidence
        ↓
project-owned common model
        ↓
higher musical inference when evidence permits
        ↓
optional reconstruction / rendering experiments
```

No higher analysis may overwrite the lower evidence that supports it.

## Primary source

OpenMusic library catalog:

- `https://openmusic-project.github.io/libraries`
- primary GitHub organization: `openmusic-project`

The catalog is broader than the OpenMusic core and includes symbolic analysis, pattern modeling, voice separation, rhythm quantification, constraint programming, additive/spectral analysis, synthesis control, orchestration, spatial scenes, score export, rewriting systems, harmonic analysis, and dictionary-based sound models.

This pass focused on libraries that touch current VGM Tooling frontiers rather than treating every catalog entry as equally relevant.

## 1. Persistent musical voice / part identity

### Streamsep

Repository:

- `openmusic-project/Streamsep`

Streamsep performs polyphonic stream/voice separation with single-link agglomerative clustering. Its current implementation:

- explodes polyphonic input into individual events;
- prevents overlapping events from joining the same monophonic output stream unless an overlap tolerance permits it;
- computes event proximity from temporal distance and pitch distance;
- uses a perceptual mel-frequency scale for pitch distance;
- allows independently weighted time and pitch terms;
- can follow lines that cross registers;
- groups events into output streams without changing the source events themselves;
- is currently cubic in the number of events and is therefore analysis machinery, not a realtime playback primitive.

This directly pressure-tests the unresolved VGM Tooling boundary:

```text
physical voice episode
!= persistent musical voice / part
```

The reusable idea is **candidate grouping from event relations**, not the OpenMusic data structures or exact implementation.

For VGM Tooling, a future grouping experiment can operate on source-supported performance events and use features such as:

- exact/derived onset and duration;
- device-native or normalized pitch;
- continuity across rests;
- overlap;
- register;
- source/sample/timbre similarity;
- driver identity when available;
- physical-allocation changes;
- provenance confidence.

Any resulting persistent `part` remains a hypothesis unless stronger authored/driver evidence proves it.

Relevant literature found in this pass includes work on musical voice integration/segregation, symbolic voice separation, stream segregation, and comparisons between voice and stream segmentation. These works independently support treating persistent voice assignment as a separate inference problem rather than equating it with playback-channel identity.

## 2. Repetition, motif, phrase, and style structure

### LZ

Repository:

- `openmusic-project/LZ`

The library describes itself as automatic modeling of musical style. Its source operates on symbolic note information containing pitch, onset, duration, velocity, and channel, and contains Lempel-Ziv / pattern-tree / probabilistic suffix-tree style machinery with configurable information filters.

A particularly useful idea is that the same event sequence can be modeled through different feature projections rather than requiring one canonical tokenization. The implementation exposes separate filters for combinations such as:

- pitch;
- new pitch;
- pitch + duration;
- duration;
- velocity;
- old/new pitch relations;
- secondary information used during reconstruction.

For VGM Tooling this suggests:

```text
exact performance graph
        ↓
feature projection chosen for one analysis
        ↓
repetition / motif / style model
```

rather than:

```text
convert source to MIDI
        ↓
make MIDI the truth
```

Useful future applications include:

- repeated phrase discovery;
- motif recurrence under transposition or changed allocation;
- section similarity;
- accompaniment-pattern recurrence;
- instrument-role recurrence;
- driver-pattern recovery;
- detecting structure that survives a hardware-channel move.

Literature pressure supports this direction through information-theoretic and compression-based models of repetition, similarity, variable-length motif dictionaries, and multi-part musical structure.

### Patterns

Repository:

- `openmusic-project/Patterns`

Patterns adapts Rick Taube's item-stream/pattern-stream concepts and explicitly points to related pattern machinery in SuperCollider.

Useful pressure:

```text
pattern
!= expanded event list
```

A repeated or generative musical process may remain a symbolic process even when one execution has already emitted concrete events. This aligns with the current separation between static program/control flow and one realized execution trace.

### Profile

Repository:

- `openmusic-project/Profile`

Profile is a library for control of melodic profiles. Its main relevance is the concept of **melodic contour/profile as an object above absolute note identity**.

This is potentially useful for comparing melodic identity across:

- transposition;
- changed physical channel allocation;
- changed instrumentation;
- variation and ornamentation;
- prototype/final arrangements.

### Morphologie

Repository:

- `openmusic-project/Morphologie`

Morphologie provides analysis, pattern recognition, morphological classification, and reconstruction of symbolic/numeric sequences and geometric profiles.

The transferable idea is that sequence identity can be tested through **shape and transformation relations**, not only literal event equality.

This is relevant to motif/phrase comparison but does not justify a new graph primitive yet. Existing `musical_relation`, `transforms`, `repeats`, `derived_from`, and hypothesis evidence can represent the result when a real experiment requires it.

## 3. Rhythm recovery without destroying exact time

### RQ

Repository:

- `openmusic-project/RQ`

RQ performs rhythm quantification through k-best parsing of segmented input. It recursively subdivides candidate rhythmic trees and ranks alternative transcriptions using at least two separate criteria:

- **distance** from the observed timing;
- **complexity/readability** of the resulting rhythmic tree.

This is highly relevant to VGM Tooling because source/device time and authored/notated time are already separate domains.

The reusable law is:

> Quantization should produce an authored-time hypothesis and an explicit mapping from source/performance time. It must not rewrite exact source timestamps.

A future VGM/SPC rhythm-analysis path should therefore be capable of retaining several candidate authored rhythms with their own fit/complexity evidence while the exact driver/device/sample timing remains unchanged beneath them.

This also reinforces the existing rule that competing high-level hypotheses should remain available until evidence separates them.

## 4. Constraint programming as a reconstruction formalism

### Situation

Repository:

- `openmusic-project/Situation`

Situation is an established OpenMusic constraint-programming system.

### Clouds

Repository:

- `openmusic-project/Clouds`

Clouds is another OpenMusic musical-constraints library. Related published work models musical composition, analysis, and instrumentation problems as constraint-satisfaction problems and uses adaptive search to obtain exact or approximate solutions.

### Cluster Engine

Repository:

- `PHRaposo/Cluster-Engine-Library-for-OpenMusic`

Cluster Engine solves polyphonic constraint-satisfaction problems in which pitch and rhythmic structure can be restricted by user-defined rules. It explicitly allows different rules to constrain different aspects of the result, for example melody and harmony independently.

### OMGecode and related systems

The OpenMusic catalog also records OMGecode and other constraint libraries for melodic, harmonic, contrapuntal, and rhythmic problems.

The important transfer is not a particular solver. It is the separation:

```text
known variables / exact evidence
+
unknown variables
+
constraints
+
objective / preference
+
search
=
candidate realization
```

This is promising for later **constraint-relaxed source-conditioned reconstruction**.

A reconstruction experiment could ask for the highest-quality realization that satisfies invariants such as:

- notes and timing preserved;
- authored modulation preserved;
- source/sample identity respected;
- loop/control-flow behavior preserved;
- deliberate chip effects preserved;
- unsupported bandwidth/storage limitations allowed to relax;
- reconstruction uncertainty kept explicit.

This is a research formulation, not yet a realtime playback architecture.

Relevant literature in this pass includes constraint programming for computer-assisted composition, OMClouds/adaptive-search work, generic music-constraint systems, and computer-assisted orchestration.

## 5. Spectral and timbre representation

### Esquisse

Repository:

- `openmusic-project/Esquisse`

Esquisse provides spectral-music and intervallic-manipulation tools. It is a useful upper-level bridge between pitch/interval structure and spectral organization.

Potential VGM Tooling questions include whether two exact source objects share:

- harmonic organization;
- interval structure;
- spectral-envelope tendencies;
- transposition-related timbre;
- multisample/instrument-family relationships.

### OM-pm2

Repository:

- `openmusic-project/OM-pm2`

OM-pm2 connects OpenMusic to IRCAM's pm2 analysis/synthesis kernel. It supports:

- additive analysis;
- partial extraction;
- chord-seq extraction from prominent partials over time;
- resynthesis from partial lists/chord-seqs;
- SDIF-based analysis results.

The OpenMusic wrapper does **not** include the pm2 kernel itself, so this is a research/reference source, not an attractive runtime dependency.

For VGM Tooling the important concept is an intermediate partial/spectral representation that can be derived from an exact source sample or isolated source render and then compared against source-native pitch/envelope truth.

Possible future experiment:

```text
exact BRR / PCM / isolated FM render
        ↓
partial + spectral-envelope analysis
        ↓
source-conditioned timbre model
        ↓
higher-rate resynthesis candidate
```

The result must remain a reconstruction hypothesis, never evidence that the original uncompressed source has been recovered exactly.

### OM-SuperVP

Repository:

- `openmusic-project/OM-SuperVP`

OM-SuperVP connects OpenMusic symbolic/musical objects to SuperVP analysis, processing, and synthesis. It converts high-level objects and breakpoint functions into processing parameters and stores analysis results in SDIF.

The transferable idea is the explicit bridge:

```text
musical / symbolic control
        ↓
analysis / transformation parameters
        ↓
spectral processing
        ↓
resulting sound
```

while keeping the symbolic and acoustic representations distinct.

Again, SuperVP is external software and should be treated as a research oracle/prototype surface rather than a required foobar dependency.

### OM-Pursuit

The OpenMusic catalog describes OM-Pursuit as dictionary-based sound models for computer-aided composition. Public GitHub mirrors contain OpenMusic and Python/source material.

This is worth a later dedicated pass because dictionary-based models may provide another route between exact source samples and a compact timbre/object representation, particularly where partial models alone are inadequate.

### Literature pressure

The literature pass found independent work on:

- partial tracking;
- additive analysis/resynthesis;
- sinusoidal + transient + noise models;
- spectral-envelope estimation and manipulation;
- source-filter models for musical instruments;
- high-resolution time-frequency analysis.

These reinforce a useful decomposition:

```text
pitch / partial trajectories
+
spectral envelope
+
transient structure
+
residual / noise structure
```

rather than treating a low-rate legacy sample as an indivisible waveform that can only be interpolated.

No single decomposition is yet promoted into the common graph.

## 6. Symbolic control of synthesis backends

### OMChroma / OM2Csound

Repositories/catalog entries:

- `openmusic-project/OMChroma`
- OM2Csound

OMChroma is explicitly a high-level sound-synthesis control library. Together with OM2Csound it demonstrates that symbolic/compositional objects can control a separate synthesis backend without making the symbolic representation identical to the synthesizer implementation.

### OMCollider

The OpenMusic catalog records OMCollider as a library that represents SuperCollider unit generators and generates/runs `.scd` files from OpenMusic patches.

This is historical precedent for a research workflow in which a structural representation drives synthesis experiments while the synthesis engine remains replaceable.

For VGM Tooling this supports using SuperCollider/Csound/other systems as experimental renderers or test oracles while keeping the project-owned execution graph canonical.

## 7. Spatial scene representation

### OM-Spat

Repository:

- `openmusic-project/OM-Spat`

OM-Spat represents spatial scenes as matrix-like objects associating sound sources and spatial trajectories. It stores source trajectories and spatial attributes in SDIF and can render them through an external Spat renderer or stream them to a realtime environment.

Transferable distinction:

```text
source identity
!= spatial trajectory
!= spatial renderer
```

This is relevant downstream to libaural/Omniphony and to future source-aware scene metadata, but it does not reveal authored 3D truth from ordinary legacy game-music sources.

A chip channel number must never be treated as a spatial coordinate merely because the source is isolated.

## 8. Orchestration and target-based reconstruction

### OM-Orchidee

Repository:

- `openmusic-project/OM-Orchidee`

The OpenMusic client exposes separate objects for:

- a target sound;
- an available orchestra;
- communication with the orchestration search engine;
- returned candidate solutions.

The client itself is deprecated and depends on a working Orchidee service/database, so it is not a dependency candidate.

Its conceptual value is the **target / candidate resources / search / solution** separation.

For VGM Tooling a related future research question is:

> Given exact musical/synthesis evidence from a constrained legacy source, which higher-resolution synthesis realization best matches its supported structure while satisfying preservation constraints?

That is an aesthetic/reconstruction target. It must not be called recovered composer intent without independent evidence.

## 9. Representation systems and rewriting

### RepMus

Repository:

- `openmusic-project/Repmus`

RepMus is historical Music Representations-team material and remains useful as lineage for higher-level representation ideas.

### Rewrite and related rhythmic/pitch libraries

The OpenMusic catalog contains rewriting systems, rhythmic-constraint systems, pitch-field/time libraries, and pattern tools. Their shared lesson is that musical structure may be represented as **transformable relations and processes**, not only flat terminal events.

These should be mined when a concrete VGM driver/MML/tracker problem requires their particular distinction rather than imported pre-emptively.

## 10. What this pass changes

### No new common graph layer

The current `musical_structure` layer is sufficient for the newly identified analyses.

The current graph already contains useful ordinary primitives:

- `part`;
- `section`;
- `pattern`;
- `musical_relation`;
- `parameter`;
- `projection`;
- `groups_into`;
- `transforms`;
- `repeats`;
- `derived_from`;
- `same_identity_as`;
- exact / derived / hypothesis evidence states.

Therefore the library pass does **not** justify another ontology subsystem.

### What it does change

It sharpens the analysis discipline above exact execution:

```text
exact source evidence
        ↓
conservative performance events / controls
        ↓
feature projection for one analysis
        ↓
one or more musical-structure hypotheses
        ↓
validation / alternatives retained
```

Important rules:

1. **Exact source information outranks inference.** Do not infer a pitch, sample, track, or timing fact already known from the source.
2. **Analysis does not mutate its evidence.** Rhythm quantification, voice separation, motif detection, and harmonic analysis create higher objects/mappings rather than rewriting source/performance nodes.
3. **Physical voice and musical voice stay distinct.** Stream/voice separation is a hypothesis generator unless a driver/authored source proves identity.
4. **Feature projection is temporary.** An LZ model over pitch/duration or a Streamsep distance vector is an analysis view, not the canonical music representation.
5. **Alternative answers may coexist.** RQ-like k-best rhythmic interpretations and other competing analyses fit the existing hypothesis/provenance model.
6. **Constraint search belongs above truth, not inside it.** Reconstruction/search candidates must state which invariants they preserve and which dimensions they relax.
7. **Spectral reconstruction is conditional.** Partial/spectral/dictionary models can propose higher-quality realizations without claiming recovery of information that was never encoded.
8. **External OpenMusic/IRCAM kernels remain research references.** Project-owned playback should not acquire mandatory dependencies on OpenMusic, pm2, SuperVP, Orchidee, or Spat.

## 11. Recommended implementation order

The next implementation should remain bounded and project-owned.

1. Build an offline fixture from already-supported conservative performance events and physical voice episodes.
2. Implement a small **voice/stream grouping experiment** using established time/pitch/overlap features, with the output represented only as hypothesis-level grouping/part candidates.
3. Add adversarial controls where physical-channel continuity conflicts with musical continuity, including GEMS-style allocation changes and crossing lines.
4. Compare several established voice/stream separation rules rather than declaring Streamsep's metric canonical.
5. Only after grouping is validated, test contour/morphology and repetition models on the same evidence-preserving event objects.
6. Add rhythm-quantification experiments as authored-time hypotheses connected by explicit time mappings, never by moving exact source timestamps.
7. Keep spectral/partial/dictionary analysis offline until source-object identity and reference rendering are stable enough to make the experiment meaningful.
8. Treat constraint-based reconstruction as a later search layer whose preservation constraints are explicit and testable.

## 12. Literature retained from this pass

Representative literature used as pressure rather than authority:

- Szeto & Wong, stream segregation for polyphonic music databases, 2003/2006.
- Rafailidis, Cambouropoulos & Manolopoulos, musical voice integration/segregation, 2009.
- Guiomard-Kagan et al., comparison of voice and stream segmentation algorithms, 2015.
- Della Ventura, information-theoretic voice separation, 2018.
- Assayag/Dubnov lineage on information-theoretic / variable-length musical-style modeling and machine improvisation.
- Chen & Greer, Lempel-Ziv compression as a measure of symbolic musical repetition, 2023.
- Truchet & Codognet, musical constraint satisfaction with adaptive search / OMClouds, 2004.
- Anders and related work on generic music-constraint systems and rule application.
- Sandred, constraint solving for polyphonic score generation / Cluster Engine lineage.
- Maresz and IRCAM work on computer-assisted orchestration and sound targets.
- Rodet & Schwarz, spectral envelopes and additive/residual analysis-synthesis.
- Lazzarini, Timoney & Lysaght, partial tracking and additive analysis-synthesis approaches.
- broader work on sinusoidal, transient, residual, and source-filter models of musical timbre.

The point of the literature comparison is the same as the repository quarry: keep only distinctions that survive independent viewpoints and real VGM/SPC tests.

## Current conclusion

The skipped OpenMusic libraries substantially strengthen the **upper half** of VGM Tooling without changing its foundation.

They provide established ways to ask:

```text
Which physical episodes belong together musically?
What repetitions and transformations survive allocation changes?
What authored rhythm best explains exact performance timing?
What structural constraints must a reconstruction preserve?
What timbre representation survives beyond a legacy sample encoding?
What candidate realization best satisfies those constraints?
```

The project advantage remains that these questions can often begin from exact source/driver/device evidence that ordinary symbolic-MIR or audio-analysis systems do not possess.

That is the useful synthesis:

```text
OpenMusic / MIR / CAC methods
        +
exact executable game-music evidence
        =
higher musical reasoning without giving up source truth
```
