# Music cognition, theory, and musicology pressure pass

## Status

Research input for the provenance-aware musical execution model.

This pass extends the existing repository/literature method into music cognition, psychology of music, music theory, computational music analysis, digital musicology, and cross-cultural music research.

The governing question is:

> What can VGM Tooling legitimately know at each layer, and what evidence is required before a claim may move from source fact to musical analysis, perceptual interpretation, or historical/musicological assertion?

The answer is not one universal music-analysis model. The useful result is an evidence discipline that allows several descriptions of the same music to coexist without allowing one layer to overwrite another.

## Core distinction

`evidence_status` and `semantic_layer` answer different questions.

```text
semantic layer
= what kind of thing is being claimed?

evidence status
= how strongly is that claim supported?

provenance
= why, where, and under what observation/model/context is it supported?
```

A claim can therefore be exact at one layer and hypothetical at another.

Example:

```text
exact VGM register writes
        ↓
derived physical voice episode
        ↓
derived pitched activity
        ↓
hypothesized persistent part
        ↓
hypothesized harmonic function
        ↓
hypothesized listener expectation
```

None of the higher claims changes the lower evidence.

## 1. Auditory organization and music psychology

### Concurrent versus sequential grouping

Music-perception research distinguishes at least two related problems:

- **concurrent grouping**: which simultaneously present acoustic components belong to one auditory event/source;
- **sequential grouping**: which events over time belong to one continuing auditory stream.

Experimentally supported cues include:

- harmonicity;
- onset synchrony;
- common frequency/amplitude change or common fate;
- pitch proximity;
- timbre similarity;
- temporal continuity;
- spatial/lateralization cues;
- tempo-dependent tolerance for dissimilarity.

Stephen McAdams' work on auditory organization in music and earlier auditory-stream literature make a particularly important point for VGM Tooling: perceived melody and rhythm depend partly on how events are grouped into streams.

This supports the existing separation:

```text
musical part
!= physical source
!= auditory event
!= auditory stream
```

A perceptual grouping model should therefore create `auditory_event` / `auditory_stream` hypotheses in `auditory_interpretation`. It must not silently rewrite `part` identity.

### Temporal coherence

Temporal-coherence models propose that features such as pitch, location, and spectral content can become bound when their activity covaries over time.

This is especially relevant to VGM Tooling because executable source state can provide unusually strong answer keys for common modulation, onset synchrony, voice continuity, and source count.

Potential future use:

```text
known executable source state
        ↓
controlled render
        ↓
perceptual grouping model sees audio only
        ↓
compare inferred grouping with source truth
```

This remains an auditory-model experiment, not a replacement for source identity.

## 2. Musical expectation, prediction, tension, beat, and meter

### Expectation is model/context dependent

Work by Pearce/Wiggins and related information-dynamics research treats musical expectation as probabilistic prediction learned from musical regularities. Other work separates short-term, long-term, knowledge-driven, data-driven, and veridical expectation.

Useful consequence:

> `expected`, `surprising`, `stable`, or `tense` are not intrinsic byte-level properties of a note or chord.

They are outputs of a specified listener/model/context.

A future expectation hypothesis should retain at least:

- the musical observations it conditions on;
- model/theory identity;
- corpus or learned context when applicable;
- cultural/style scope when applicable;
- confidence or predictive quantity;
- provenance to the lower musical events/structure.

### Beat and meter have multiple possible truth routes

Meter can be:

- **exact** when an authored source explicitly states it;
- **derived** when a validated driver/source grammar deterministically establishes it;
- **hypothesized structural analysis** when inferred from events;
- **perceptual** when describing listener entrainment or beat induction.

These are not interchangeable claims.

The same rule applies to tempo, syncopation, tonal center, phrase boundary, and other apparently familiar music-theory objects.

### Tonal hierarchy and tension

Tonal-hierarchy and harmonic-expectation research shows that listeners can learn statistical and hierarchical pitch/chord relations, while sensory and cognitive models need not make identical predictions.

VGM Tooling should therefore distinguish:

```text
pitch content
chord representation
harmonic relation
functional analysis
listener expectation
perceived tension
```

A Roman-numeral label, tonal-function label, or tension score may be useful, but it is an analytical/perceptual projection unless the source explicitly encodes the relevant object.

## 3. Music theory and computational music analysis

### Multiple harmonic analyses are legitimate objects

Condit-Schultz, Ju, and Fujinaga's automated harmonic-analysis work explicitly computes multiple analyses satisfying basic constraints rather than forcing one rule system to masquerade as objective truth.

This strongly agrees with the current graph's ability to retain competing hypothesis nodes/relations.

Useful rule:

> A music-theory analysis should identify its analytical system and preserve alternatives when the evidence does not uniquely determine one reading.

Examples include:

- chord spelling;
- key/tonal center;
- harmonic function;
- non-chord-tone interpretation;
- voice-leading relation;
- phrase segmentation;
- prolongational/hierarchical analysis;
- form;
- motif identity under transformation.

### Hierarchical analyses are analyses, not hidden source bytes

Probabilistic models of Schenkerian/hierarchical analysis and work on GTTM-style structure show that several useful hierarchical organizations can be formalized computationally.

For VGM Tooling this supports `musical_structure`, but does not make one hierarchical theory canonical.

A source-proved loop or driver section boundary can be exact/derived while a phrase, prolongation, or formal interpretation over the same events remains hypothetical.

### Representation matters

The `pmcharrison/hrep` package is a useful implementation-level pressure test because it deliberately exposes multiple harmony representations:

- symbolic;
- acoustic;
- sensory.

One chord can therefore have several useful representations without one representation being the chord's universal ontology.

That matches VGM Tooling's projection law and reinforces the distinction between:

```text
source-supported pitch/synthesis state
        ↓
structural chord representation
        ↓
acoustic/sensory representation
        ↓
harmonic or cognitive interpretation
```

### Voice leading as optimization/model output

`pmcharrison/voicer` treats chord voicing as an optimization problem over an explicit cost function and uses dynamic programming/Viterbi search.

The transferable lesson is not the Western harmony assumptions. It is that a "best" realization or analysis is only meaningful relative to an explicit objective/model.

This is directly relevant to later constraint-relaxed reconstruction.

## 4. Feature systems and computational musicology tooling

### jSymbolic

`DDMAL/jSymbolic2` extracts symbolic features for MIR, music theory, and musicology and keeps feature definitions separate from feature values. It also resolves dependencies between features dynamically.

This is a strong design input for the current source-relative identity frontier.

Transferable obligations:

- feature value and feature definition are different objects/concepts;
- a feature may depend on other derived features;
- feature availability is source-relative;
- missing support must not become numeric zero;
- analysis configuration belongs to provenance;
- a corpus-level descriptor is not automatically a local event property.

VGM Tooling should implement these ideas project-natively rather than import jSymbolic's schema.

### Humdrum / humlib

Humdrum preserves symbolic musical structure through typed spines/tracks that can split and merge. `humlib` exposes track identity, timing, duration, and typed token semantics rather than flattening the representation to MIDI.

Useful pressure:

- symbolic source topology can itself carry identity;
- track/spine identity can change topology while retaining explicit source semantics;
- analysis should preserve source structure before projecting it into simpler event lists.

### music21

music21 remains useful as a broad symbolic/musicological laboratory for notes, intervals, chords, keys, meter, voices, streams, corpus analysis, and analytical transformations.

The important lesson is still that these high-level objects are useful reasoning structures, not a replacement for executable game-music truth.

### Essentia and madmom

`MTG/essentia` and `CPJKU/madmom` expose the audio-observation side:

- low-level spectral/temporal descriptors;
- tonal descriptors;
- onset/beat/downbeat estimates;
- learned or algorithmic MIR outputs;
- explicit evaluation tooling.

These systems are useful for validating the inverse path from rendered audio, especially where VGM Tooling can provide exact source-side answer keys.

They do not justify replacing known source pitch, onset, sample, device, or routing state with audio estimates.

## 5. Cross-cultural and enculturation pressure

Cross-cultural music cognition is a critical guardrail against accidentally baking Western common-practice theory into the ontology.

Research repeatedly distinguishes:

- lower-level perceptual mechanisms that may generalize broadly;
- learned tonal/rhythmic hierarchies;
- culture/style-specific conventions;
- culture-dependent aesthetic judgments.

Examples include findings that some forms of harmonic fusion/grouping can generalize across cultures even when consonance preference does not, and that tonal/metrical expectations are strongly shaped by exposure and cultural context.

Therefore:

> No Western tonal representation, Roman-numeral system, major/minor key model, four-part voice-leading rule, or common-practice metric prior may become the default definition of "musical structure" for VGM Tooling.

A theory/cognition model must declare its scope.

Useful context fields for future hypotheses include:

- theory/model name;
- corpus/training distribution;
- musical tradition/style;
- listener population or assumed enculturation;
- source representation used as input.

These can initially remain ordinary attributes/provenance. No new graph primitive is justified yet.

## 6. Musicology, versions, sources, and historical claims

### Work identity is not file identity

Digital-musicology research on versions and arrangements explicitly separates conceptual musical objects from the concrete documents/media that witness them.

This maps directly to VGM realities:

```text
musical work / cue / arrangement hypothesis
        ↕
prototype sequence
final sequence
regional version
port
SPC/VGM capture
MIDI transcription
rendered audio
```

Two byte-distinct artifacts may be versions of one musical object, while two byte-similar artifacts need not prove one historical causal path.

### Source comparison should compare musical structure, not serialization accidents

Work such as MusicDiff for MEI is useful because it distinguishes semantic musical differences from XML/file-format differences.

For VGM Tooling, a future cross-version diff should similarly prefer:

- source/driver event differences;
- pitch/rhythm/instrument/control-flow differences;
- synthesis/patch/sample differences;
- arrangement/routing differences;

rather than treating unrelated container/layout changes as musical changes.

### Style and authorship are not execution truth

Computational authorship/style work can be useful, but the output remains an attribution hypothesis unless independent documentary evidence proves authorship.

This is already important to the Sonic 3 subproject.

A style model may detect similarities in:

- interval/rhythm distributions;
- harmony;
- instrumentation;
- timbre/synthesis habits;
- phrase/form tendencies;
- driver/tool fingerprints.

None of those features alone proves a composer or arranger.

### Historical/contextual evidence should remain externally sourced

Musicological claims can depend on manuscripts, credits, interviews, release history, archival records, revision chronology, performance practice, cultural context, and scholarship that do not exist inside the executable music object.

For now VGM Tooling can carry such evidence as provenance-bearing source objects/annotations when a bounded case needs it. This pass does **not** justify adding a new universal `musicological_context` semantic layer.

## 7. Resulting evidence law

The combined pass supports this rule:

```text
SOURCE FACT
what bytes/state/events are actually present?

EXECUTION FACT
what did the validated program/device actually do?

MUSICAL PERFORMANCE CLAIM
what notes/gestures/parts are justified by that execution?

MUSICAL STRUCTURE CLAIM
what meter/harmony/motif/form/syntax analysis is supported?

ACOUSTIC CLAIM
what measurable sound was produced?

PERCEPTUAL CLAIM
how may a listener/model group, attend, expect, fuse, segregate, or experience it?

MUSICOLOGICAL CLAIM
what does external historical/cultural/source evidence support about work, version, style, transmission, or authorship?
```

These are linked descriptions of one phenomenon, not steps in a ladder where later automatically means truer.

## 8. High-value examples of layer-relative truth

### Meter

```text
MML time signature             exact authored
validated driver meter object  exact/derived driver
meter inferred from events     structural hypothesis
beat perceived from audio      perceptual hypothesis
```

### Harmony

```text
exact simultaneous pitches        performance fact
pitch-class set                    deterministic projection
chord spelling                     structural analysis
Roman-numeral function             theory-dependent hypothesis
surprise/tension                   listener/model hypothesis
```

### Voice

```text
physical hardware slot          execution coordinate
physical voice episode          synthesis identity
validated driver track          logical identity
persistent musical part         performance identity
perceived auditory stream       listener-level identity
```

### Version / authorship

```text
file hash                       exact source identity
musical diff                    derived relation
same arrangement/work          musicological hypothesis unless proven
stylistic similarity            analysis result
composer attribution            external-evidence hypothesis/claim
```

## 9. What this pass does not justify

Do not add yet:

- a universal Western harmony engine;
- a universal listener model;
- a canonical Schenkerian/GTTM representation;
- a `tension` scalar treated as source truth;
- a permanent culture taxonomy in the core graph;
- a new semantic layer solely because musicology exists;
- a large learned MIR dependency in normal playback;
- an all-purpose feature vector that substitutes zero for unavailable evidence.

## 10. Immediate implementation pressure

The existing graph already has enough vocabulary for the first executable control:

- `musical_structure` + `musical_relation` for theory-level hypotheses;
- `auditory_interpretation` + `auditory_event` / `auditory_stream` for listener-level hypotheses;
- `part`, `voice_instance`, and `physical_slot` for identity separation;
- provenance/evidence state for exact/derived/hypothesis claims;
- `external_annotation` for externally supplied evidence;
- ordinary attributes for model/corpus/cultural scope until a concrete adapter proves a stronger abstraction is necessary.

The next regression should prove that the same lower musical evidence may simultaneously support:

1. competing harmonic/theoretical interpretations;
2. competing perceptual grouping interpretations;
3. an external historical/authorship annotation;
4. exact/derived lower execution evidence that remains unchanged.

That test should add no new generic graph vocabulary unless the existing representation fails.

## Primary GitHub observatories checked in this pass

- `cuthbertLab/music21`
- `DDMAL/jSymbolic2`
- `humdrum-tools/humlib`
- `pmcharrison/hrep`
- `pmcharrison/voicer`
- `MTG/essentia`
- `CPJKU/madmom`
- existing OpenMusic/Partitura/MEI/voice-separation sources already recorded elsewhere in this repository

## Literature families checked in this pass

The pass included primary and review literature on:

- auditory stream segregation and integration;
- timbre/pitch/spatial grouping;
- temporal coherence/common fate;
- musical expectation and information dynamics;
- tonal hierarchy and harmonic expectation;
- rhythmic expectation, beat, meter, and syncopation;
- computational voice leading;
- probabilistic/hierarchical music analysis;
- multiple harmonic interpretations;
- computational music analysis epistemology;
- digital musicology, versions, arrangements, and source comparison;
- computational authorship/style attribution;
- cross-cultural music cognition and enculturation.

The durable result is the layer/evidence discipline above, not allegiance to any one theory or software package.
