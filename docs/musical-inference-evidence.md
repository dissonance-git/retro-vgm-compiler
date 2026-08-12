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
        ↓
listener response

musicological context
↕ links artifacts, structures, performances, documents, and external evidence
  across the pipeline rather than sitting after it
```

Each layer answers a different question. The arrows show relationships, not a mandatory serial cognitive pipeline and not a ladder of increasing truth.

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

This layer describes perceptual organization of the acoustic realization.

Legitimate hypotheses can include:

- concurrent auditory events;
- sequential auditory streams;
- fusion/segregation;
- perceived source continuity;
- salience and masking when treated as perceptual organization;
- perceived beat/meter;
- attention-dependent grouping when the claim concerns which acoustic events are organized together.

These are listener/model claims, not source facts.

A perceptual model should retain its assumed listener population, cultural exposure, model identity, corpus/training context, and input representation when those matter.

### Listener response

`listener_response` is distinct from auditory organization.

It represents how a listener or explicit listener model responds to the perceived music. Candidate claims include:

- expectation and predictive uncertainty;
- information content / surprise;
- familiarity and recognition;
- memory activation;
- felt or perceived emotional response, with the response construct stated explicitly;
- pleasure;
- groove / urge to move;
- motor entrainment;
- attention state when the question is what the listener attends to rather than how sound is grouped;
- aesthetic judgment.

These outputs may depend strongly on:

- short-term musical context;
- long-term learned statistics;
- corpus/style exposure;
- cultural background;
- familiarity with the piece;
- episodic associations;
- preference;
- training;
- movement/dance experience;
- task and current context;
- the psychological mechanism or prediction model being used.

Therefore:

```text
musical feature
!= listener response
```

and:

```text
same source + same performance + same acoustic realization
can legitimately produce different listener-response hypotheses
under different listener/model contexts.
```

The detailed pressure pass is recorded in `research/cases/music-affect-memory-entrainment.md`.

### Musicological context

`musicological_context` is cross-cutting rather than a downstream listener layer.

It represents contextual, historical, documentary, source-critical, versioning, and attribution claims that relate artifacts and musical objects across the other layers.

Candidate claims include:

- work identity;
- version/revision identity;
- arrangement identity;
- port/adaptation relations;
- source-witness relations;
- derivation/transmission hypotheses;
- release chronology;
- documented composer/arranger credits;
- stylistic attribution hypotheses;
- archival/catalog identifiers;
- external historical context.

A central distinction is:

```text
artifact identity
!= musical similarity
!= musicological work/version identity
```

Two files can be exact different artifacts while strongly structurally similar. Strong similarity can support a same-work hypothesis without proving historical identity. External documentation can provide a stronger independent evidence route.

Likewise:

```text
stylistic similarity
!= composer identity
```

Musicological claims must preserve their documentary, analytical, or external provenance. See `research/cases/musicological-version-identity.md`.

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

And even one fixed auditory organization can support different listener responses because familiarity, expectation, preference, memory, culture, or task differs.

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
expectation / tension response under a specified listener/model
```

The first claim can be source/performance truth. Later claims increasingly depend on analytical or listener models.

### Example: meter and groove

```text
explicit authored meter          exact authored
validated driver meter           exact/derived driver
meter inferred from events       structural hypothesis
beat/meter heard from audio      auditory-interpretation hypothesis
syncopation/complexity analysis  structural/model-dependent analysis
urge to move / groove            listener-response hypothesis
```

One familiar label must not collapse those routes.

## Multiple interpretations are a feature, not an error

Music-analysis research provides strong independent support for preserving competing interpretations:

- multiple harmonic analyses can satisfy shared low-level constraints;
- hierarchical analyses can differ among expert/theoretical systems;
- voice-separation models can propose alternate trajectories;
- expectation models can differ by corpus, context, or listener assumptions;
- meter/beat models can disagree while the exact event timing remains fixed;
- emotion/groove responses can differ across listeners over identical musical evidence;
- source/version genealogies can differ while the observed artifact differences stay fixed.

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

These claims belong to `musicological_context` when represented as analytical/contextual claims, while the external documents/artifacts supporting them remain source objects/annotations.

For example:

```text
file hash                  exact source identity
musical diff               derived structural relation
same work/arrangement      musicological-context claim
stylistic similarity       analytical result
composer attribution       musicological-context claim with external/model evidence
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

Cross-version comparison should distinguish musical structure from container/serialization accidents, and similarity from historical identity.

Useful comparison dimensions can include:

- source/artifact identity;
- program/control-flow similarity;
- performance-event similarity;
- instrument/sample/synthesis similarity;
- form/structural similarity;
- acoustic similarity;
- documented work/version/arrangement relations.

A single scalar similarity should not erase which dimensions were compared or which invariances were allowed.

## Feature systems

GitHub/literature comparisons with jSymbolic, music21, Humdrum, hrep, Essentia, madmom, Partitura, OpenMusic, MIRtoolbox, pyIDyOM, MusicDiff-related tooling, OpenMPT, and related systems support several design constraints for analysis features.

An analysis feature explicitly distinguishes:

```text
feature name
claim layer
availability
value
evidence status / confidence
provenance / model context
supporting graph nodes/edges
```

Availability is one of:

```text
present
unknown
unavailable
not_applicable
```

A missing feature is not zero.

A feature that is exact for one source family may be unavailable or hypothetical in another.

Example:

```text
MML authored pitch      explicit authored
tracker note pitch      explicit authored
YM2612 pitch relation   derived synthesis/device state
SPC pitch-rate relation synthesis truth conditional on runtime observation
performed SPC pitch     may remain unknown without sample/tuning continuity
audio pitch estimate    inverse-analysis output
felt emotion            listener response, unavailable from source alone
shared work identity    musicological context, may be hypothesis or documented externally
```

The project should prefer the strongest available source evidence and fall back only when the source stops answering the question.

## Observatories added by the current passes

The current comparison set now includes, among others:

- music psychology / auditory organization;
- music cognition and expectation;
- music emotion mechanisms;
- groove and sensorimotor entrainment;
- memory, familiarity, and statistical learning;
- cross-cultural music cognition;
- harmonic/tonal cognition;
- computational voice leading;
- probabilistic and multiple-interpretation harmonic analysis;
- hierarchical music analysis;
- computational music-analysis epistemology;
- digital musicology / source comparison / versions;
- MusicDiff/source-witness comparison and multi-source visualization;
- cover/version identification and transformation-invariant similarity;
- symbolic feature systems such as jSymbolic;
- Humdrum symbolic topology;
- hrep's symbolic/acoustic/sensory harmony representations;
- Essentia/madmom audio-MIR pipelines;
- MIRtoolbox emotion/feature pipelines;
- pyIDyOM expectation/viewpoint/corpus machinery;
- OpenMPT tracker semantics.

See:

- `research/cases/music-cognition-theory-musicology.md`
- `research/cases/music-affect-memory-entrainment.md`
- `research/cases/tracker-semantic-pressure.md`
- `research/cases/musicological-version-identity.md`

## What the current model now supports

Two deeper research passes justified semantic distinctions not captured cleanly by the earlier eight-layer graph:

```text
auditory_interpretation
!= listener_response

source identity / structural similarity
!= musicological_context
```

`musicological_context` is cross-cutting rather than another stage after listening.

No new node kind or edge kind was required for either distinction.

Existing vocabulary plus the source-relative `analysis_feature` carrier is sufficient for the current controls:

- `musical_structure` + `musical_relation` for theory-level analyses and measured structural relations;
- `auditory_interpretation` + `auditory_event` / `auditory_stream` for perceptual organization;
- `listener_response` analysis features for expectation, memory, emotion, groove, pleasure, attention, and related listener/model outputs;
- `musicological_context` relation nodes/features for work, version, arrangement, chronology and attribution claims;
- `part`, `voice_instance`, and `physical_slot` for identity separation;
- exact/derived/hypothesis evidence states;
- provenance-bearing nodes/edges/features;
- `external_annotation` for externally supplied evidence.

This keeps the graph small while allowing materially different questions to remain visibly different.

## Executable controls

Current model regressions protect:

- competing theory-level analyses over unchanged performance evidence;
- competing auditory-stream interpretations over unchanged acoustic evidence;
- external historical/attribution annotations without contaminating execution truth;
- source-relative feature availability across Genesis, SPC, and tracker-shaped evidence;
- explicit feature claim layers;
- identical lower musical evidence producing different listener-response hypotheses under different modeled listener/learning contexts;
- two exact different artifacts carrying separate structural-similarity, same-work, documented-arrangement, chronology and attribution claims without collapsing them into one identity flag.

The next analysis algorithm should consume these evidence-bearing features rather than invent a lowest-common-denominator representation.

## Related documents

- `docs/musical-execution-model.md`
- `docs/music-representation-systems.md`
- `docs/persistent-musical-identity.md`
- `docs/openmusic-libraries.md`
- `research/cases/music-cognition-theory-musicology.md`
- `research/cases/music-affect-memory-entrainment.md`
- `research/cases/tracker-semantic-pressure.md`
- `research/cases/musicological-version-identity.md`
