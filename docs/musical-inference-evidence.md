# Musical inference and evidence

## Purpose

VGM Tooling studies one musical phenomenon through many source families and research traditions. The project therefore needs a strict answer to a deceptively simple question:

> What can we legitimately know at each layer?

This document is the durable synthesis for that question. Detailed source mining belongs in `research/cases/` and source-family docs; this file records the constraints that survived comparison across executable game-music sources, music cognition, music psychology, music theory, computational music analysis, digital musicology, and MIR implementations.

## The central law

Later abstraction is not automatically greater truth.

```text
source bytes/state
        ↓
execution
        ↓
performance
        ↓
musical structure
        ↓
acoustic realization
        ↓
auditory interpretation
```

Each layer answers a different question.

Separately:

```text
exact
 derived
 hypothesis
```

states how strongly one claim is supported.

And provenance records why the claim is supported, which source/model/observer produced it, and what limitations apply.

Therefore:

```text
semantic layer != evidence status != provenance
```

The same piece of music may support exact claims, derived claims, and several incompatible hypotheses simultaneously.

## Layer-relative knowledge

### Source representation

Legitimate claims include facts that are directly present in the source artifact:

- bytes;
- file/container structure;
- addresses;
- explicit metadata;
- stored CPU/RAM/DSP state;
- command/log order;
- embedded samples/data blocks;
- source-level identifiers.

An exact source fact is exact **relative to that source**. Capture completeness remains a separate question.

### Authored program / score / symbolic source

When the source actually preserves authored semantics, legitimate claims can include:

- notes/rests/ties;
- written pitch/rhythm;
- explicit part/track identity;
- tempo/meter commands;
- instrument/program selection;
- loops/macros/control flow;
- articulations/modulation instructions.

Do not reconstruct this layer by fiat from a lower trace merely because an authored representation would be convenient.

### Driver execution

Legitimate claims can include:

- validated logical tracks;
- scheduler state;
- control flow;
- sequence event order;
- allocation policy;
- runtime program points;
- one realized execution path.

A static program and one runtime traversal remain distinct.

### Synthesis

Legitimate claims can include:

- instrument/sample/patch objects;
- physical synthesis resources;
- bounded voice instances;
- oscillator/operator/sample state;
- device parameters;
- routing/effects state;
- exact or rebuildable device-state views.

Physical resource identity is not automatically musical identity.

### Musical performance

Legitimate claims can include, when evidence supports them:

- note-like or pitched-activity events;
- performance timing;
- pitch/control trajectories;
- dynamics/articulation;
- persistent musical parts;
- performance gestures.

A performance event can be derived from execution without proving its authored notation.

### Musical structure

This layer contains analyses such as:

- beat/meter hierarchy;
- key/tonal center;
- chord/harmony relations;
- tonal function;
- voice leading;
- motifs and transformations;
- phrases/sections/form;
- syntax/prolongation;
- repetition and equivalence relations.

These claims often depend on an analytical theory, representation, segmentation, style, or corpus. Several analyses may legitimately coexist.

A useful rule is:

> structural analysis must identify the analytical assumptions that make the result meaningful.

### Acoustic realization

Legitimate claims include measurable properties of produced sound:

- waveform/spectrum;
- loudness/level;
- spectral envelope;
- partials;
- transients;
- spatial cues;
- rendered timing;
- masking and overlap measurements.

When exact source state is available, audio-derived estimates must not replace it. They remain observations of a different projection.

### Auditory interpretation

Legitimate hypotheses can include:

- concurrent auditory events;
- sequential auditory streams;
- fusion/segregation;
- salience;
- masking;
- perceived beat/meter;
- listener expectation;
- surprise;
- tension;
- attention-dependent grouping.

These are listener/model claims, not source facts.

A perceptual model should retain its assumed listener population, cultural exposure, model identity, corpus/training context, and input representation when those matter.

## Perceptual grouping versus musical identity

Music psychology repeatedly supports cues such as:

- onset synchrony;
- harmonicity;
- common fate/common modulation;
- pitch proximity;
- timbre similarity;
- temporal continuity;
- spatial/lateralization similarity;
- tempo-dependent grouping.

Those cues are valuable for auditory-stream hypotheses and sometimes for unresolved persistent-part hypotheses.

But:

```text
part
!= voice_instance
!= physical_slot
!= auditory_stream
```

A listener can fuse several sources into one stream or segregate one authored source into several perceptual components.

## Music-theory claims need theory provenance

Common labels can conceal different epistemic strength.

### Example: harmony

```text
exact simultaneous pitch state
        ↓
deterministic pitch-class projection
        ↓
chord spelling / root analysis
        ↓
harmonic-function interpretation
        ↓
expectation / tension
```

The first claim can be source/performance truth. Later claims increasingly depend on analytical or listener models.

### Example: meter

```text
explicit authored meter          exact authored
validated driver meter           exact/derived driver
meter inferred from events       structural hypothesis
beat/meter heard from audio      perceptual hypothesis
```

One label must not collapse those routes.

## Multiple interpretations are a feature, not an error

Music-analysis research provides strong independent support for preserving competing interpretations:

- multiple harmonic analyses can satisfy shared low-level constraints;
- hierarchical analyses can differ among expert/theoretical systems;
- voice-separation models can propose alternate trajectories;
- expectation models can differ by corpus, context, or listener assumptions;
- meter/beat models can disagree while the exact event timing remains fixed.

VGM Tooling should therefore prefer:

```text
lower evidence
        ↓
several explicit hypotheses
        ↓
comparison / later discrimination
```

rather than forcing one early answer because a downstream projection wants a scalar label.

## Enculturation and cross-cultural scope

A major guardrail from cross-cultural music cognition is that VGM Tooling must not mistake familiar Western theory for universal ontology.

Some lower-level perceptual mechanisms may generalize broadly, while tonal hierarchies, metric expectations, harmonic syntax, stylistic conventions, and aesthetic judgments can be learned or culture-dependent.

Therefore no default model may silently assume:

- Western common-practice tonality;
- major/minor harmony;
- Roman-numeral function;
- four-part voice-leading rules;
- a specific meter prior;
- Western consonance preference;
- one universal scale/chord vocabulary.

Theory/cognition outputs should declare their scope when it matters.

## Musicology and historical evidence

Execution alone cannot answer every musically important question.

Musicology can introduce evidence about:

- work/version identity;
- arrangements and revisions;
- source witnesses;
- transmission;
- release chronology;
- performance practice;
- composer/arranger credits;
- archival documentation;
- cultural/historical context;
- style and attribution.

These claims can use the common graph when a bounded case needs them, but external historical evidence must remain visibly external.

For example:

```text
file hash                  exact source identity
musical diff               derived relation
same work/arrangement      musicological claim
stylistic similarity       analytical result
composer attribution       external-evidence claim
```

Do not upgrade stylistic resemblance into authorship.

This is especially relevant to the Sonic 3 attribution work.

## Work identity versus artifact identity

Different media can witness one conceptual musical object:

```text
prototype driver sequence
final driver sequence
port
SPC/VGM capture
MIDI transcription
rendered audio
score reconstruction
```

Conversely, similar bytes or musical fragments do not automatically prove one causal lineage.

Future cross-version comparison should distinguish musical structure from container/serialization accidents.

## Feature systems

GitHub/literature comparisons with jSymbolic, music21, Humdrum, hrep, Essentia, madmom, Partitura, OpenMusic, and related systems support several design constraints for analysis features.

A feature system should distinguish:

```text
feature definition
feature availability
feature value
derivation/support
analysis configuration
scope
```

A missing feature is not zero.

A feature that is exact for one source family may be unavailable or hypothetical in another.

Example:

```text
MML authored pitch      explicit
tracker note pitch      explicit
YM2612 pitch relation   derived from device state
SPC pitch-rate relation conditional on sample/tuning continuity
audio pitch estimate    inverse-analysis output
```

The project should prefer the strongest available source evidence and fall back only when the source stops answering the question.

## Observatories added by the current pass

The current comparison set now includes, among others:

- music psychology / auditory organization;
- music cognition and expectation;
- cross-cultural music cognition;
- harmonic/tonal cognition;
- computational voice leading;
- probabilistic and multiple-interpretation harmonic analysis;
- hierarchical music analysis;
- computational music-analysis epistemology;
- digital musicology / source comparison / versions;
- symbolic feature systems such as jSymbolic;
- Humdrum symbolic topology;
- hrep's symbolic/acoustic/sensory harmony representations;
- Essentia/madmom audio-MIR pipelines.

See `research/cases/music-cognition-theory-musicology.md` for the bounded research record.

## What the current model already supports

No new generic graph primitive is required by this pass.

Existing vocabulary is sufficient for the first controls:

- `musical_structure` + `musical_relation` for theory-level analyses;
- `auditory_interpretation` + `auditory_event` / `auditory_stream` for listener-level grouping;
- `part`, `voice_instance`, and `physical_slot` for identity separation;
- exact/derived/hypothesis evidence states;
- provenance-bearing nodes/edges;
- `external_annotation` for externally supplied evidence;
- ordinary attributes for theory/model/corpus/cultural scope until a concrete implementation proves a stronger shared abstraction is necessary.

This is desirable. More observatories produced **more precision without another conceptual machine**.

## Immediate executable control

The next regression should prove that one lower musical event set can simultaneously support:

- multiple theory-level analyses;
- multiple perceptual grouping hypotheses;
- external historical/attribution evidence;
- unchanged exact/derived execution and performance truth.

If the existing graph fails that test, the failure can justify the smallest new abstraction. If it passes, keep the graph small.

## Related documents

- `docs/musical-execution-model.md`
- `docs/music-representation-systems.md`
- `docs/persistent-musical-identity.md`
- `docs/openmusic-libraries.md`
- `research/cases/music-cognition-theory-musicology.md`
