# Music affect, memory, expectation, and entrainment pressure pass

## Status

Research input for the provenance-aware musical execution model.

This pass continues the psychology-of-music frontier after the broader cognition/theory/musicology pass. Its specific purpose is to determine whether `auditory_interpretation` is sufficient for psychological claims such as expectation, emotion, familiarity, attention, groove, pleasure, memory activation, and motor entrainment.

## Question

Auditory organization asks:

```text
what acoustic components are perceived as one event or stream?
```

But many psychologically important music questions ask something different:

```text
how does a particular listener/model respond to the perceived music?
```

The distinction matters because the second class of claims can depend strongly on learned context, memory, culture, familiarity, attention, motor state, individual differences, and the psychological mechanism assumed by the model.

## Result

The pass supports one additional semantic layer:

```text
acoustic_realization
        ↓
auditory_interpretation
        ↓
listener_response
```

This is not a truth ladder. `listener_response` is not more correct than the lower layers. It represents a different claim type.

No new generic node kind is justified. Listener-response outputs can initially be carried by provenance-bearing `analysis_feature` values whose `claim_layer` is `listener_response`.

## 1. Emotion is mediated, not a source label

### Juslin / BRECVEMA-style mechanism pressure

Music-emotion research by Patrik Juslin, Daniel Västfjäll, Tuomas Eerola, and collaborators repeatedly argues that a direct mapping from musical surface features to listener emotion is inadequate unless the underlying induction mechanism is considered.

Mechanisms discussed across this literature include:

- brain-stem reflex;
- rhythmic entrainment;
- evaluative conditioning;
- emotional contagion;
- visual imagery;
- episodic memory;
- musical expectancy;
- aesthetic judgment/appraisal and related extensions.

The same musical passage can therefore produce different responses because different mechanisms, memories, contexts, or listeners are involved.

Useful law:

```text
musical feature
!= felt emotion
```

A tempo, mode, roughness, loudness, chord, or syncopation value may contribute evidence to an emotion model, but the model output is a listener/model response.

### GitHub observatory: MIRtoolbox `miremotion`

Primary repository:

- `olivierlar/mirtoolbox`

`miremotion` predicts dimensions/concepts such as activity, valence, tension, happy, sad, tender, anger, and fear from audio-derived features.

The implementation itself records the study/model from which feature selection and regression coefficients were taken and includes a calibration/version warning.

That is a useful implementation-level lesson:

> even a deterministic emotion predictor has model/training/version provenance and therefore does not turn its output into an intrinsic property of the source music.

For VGM Tooling, an emotion estimate should retain at least:

- response construct, e.g. felt vs perceived/expressed emotion;
- induction/prediction model;
- model/version/training context when relevant;
- listener/population assumptions;
- musical/acoustic evidence consumed;
- output confidence/uncertainty where available.

The source graph beneath it remains unchanged.

## 2. Expectation belongs to a listener/model state

### IDyOM literature

Information Dynamics of Music research models expectation through probabilistic prediction learned from musical regularities. The literature distinguishes short-term/dynamic context from long-term/schematic knowledge and also discusses veridical, sensory, and culturally learned knowledge.

Expectation therefore depends on more than the next note itself.

### GitHub observatory: `Kappers/pyidyom`

The inspected implementation makes the dependency explicit:

- melodies are projected into configurable viewpoints such as pitch, interval, contour, inter-onset interval, position in bar, and bar length;
- alphabets are derived from a corpus;
- source/target viewpoints are configurable;
- model types distinguish short-term and long-term variants;
- training/test context and incremental learning alter the predictive model.

Useful law:

```text
note/event
+
representation/viewpoint
+
learned model
+
context/history
→ expectation
```

not:

```text
note/event
→ intrinsic surprise
```

Information content, entropy, expectedness, or surprise should therefore be `listener_response` claims with model/corpus/context provenance.

## 3. Groove is not a rhythm-only scalar

Psychological and neuroscientific groove research defines groove around a pleasurable urge to move or sensorimotor engagement with music.

Research supports relations with factors such as:

- syncopation;
- event density;
- rhythmic complexity;
- beat salience/predictability;
- motor prediction;
- tempo;
- familiarity;
- listener training;
- style preference;
- cultural experience.

Several studies find nonlinear relationships between rhythmic complexity/syncopation and groove, while others show listener familiarity, style bias, biography, or expertise can outweigh simple structural descriptors.

Therefore distinguish:

```text
syncopation / event density / meter
        musical_structure

beat/rhythm tracking
        auditory_interpretation

urge to move / groove / motor entrainment / pleasure
        listener_response
```

A future groove model should not overwrite exact source timing or structural rhythm analysis.

## 4. Memory and familiarity change the response to identical music

Music-perception research distinguishes several memory/context timescales, including:

- echoic/sensory memory;
- working/short-term musical context;
- schematic long-term style knowledge;
- veridical memory for familiar pieces;
- episodic memory associated with a particular listening experience.

These stored representations support recognition and expectation and can change emotion, salience, segmentation, or groove judgments.

This gives VGM Tooling a powerful negative control:

> identical executable music can legitimately produce different listener-response claims under different memory/listener contexts.

The difference belongs in the response/model provenance, not in the source graph.

## 5. Attention is not source presence

A source can be physically present and acoustically measurable without being the attended object.

Attention can alter:

- salience;
- stream selection;
- memory encoding;
- expectation;
- perceived detail;
- task-dependent grouping.

Therefore:

```text
source exists
!= source is audible above threshold
!= source is grouped into a stream
!= source is attended
```

This distinction may later be useful to libaural, but the present pass only establishes the claim boundary.

## 6. Cultural and individual scope

The previous cross-cultural pass already established that tonal, metric, and aesthetic expectations can be enculturated.

The affect/groove literature adds individual-state effects:

- familiarity;
- preference;
- musical training;
- dancing/motor experience;
- episodic association;
- current context;
- population differences.

Consequently, `listener_response` outputs should identify their modeled listener/population/context whenever those assumptions materially affect the result.

No permanent listener taxonomy is justified in the core model yet. Existing feature provenance/details are sufficient for the first executable controls.

## 7. Why `auditory_interpretation` remains separate

`auditory_interpretation` should continue to describe how acoustic realization becomes perceptually organized:

- auditory events;
- auditory streams;
- fusion/segregation;
- perceived source continuity;
- possibly perceived beat/meter when the claim is about perceptual organization.

`listener_response` describes responses of a listener/model to that organized music:

- expectation/surprise;
- familiarity/recognition;
- emotion/affect;
- pleasure;
- groove/urge to move;
- motor entrainment;
- attention state;
- memory activation;
- aesthetic judgments.

There can be feedback in real human cognition, but the semantic distinction is still useful. A model may consume lower events directly without implying that the layers are a mandatory serial processing pipeline.

## 8. Feature examples

### Expectation

```text
pitch_event
    musical_performance / exact or derived

melodic_pattern
    musical_structure / derived or hypothesis

predicted_next_pitch_distribution
    listener_response / model output

information_content
    listener_response / model output
```

### Groove

```text
exact onset times
    musical_performance

syncopation index
    musical_structure / model-dependent analysis

perceived beat
    auditory_interpretation

urge_to_move
    listener_response
```

### Emotion

```text
RMS / spectral / rhythmic features
    acoustic or structural claims

perceived expression
    listener_response model output

felt emotion
    listener_response observation/hypothesis
```

Do not use the same feature name for felt and perceived emotion without an explicit response construct.

## 9. What this pass does not justify

Do not add:

- an intrinsic `emotion_of_song` property;
- a universal groove scalar;
- listener-independent surprise/tension/pleasure;
- a default Western-trained expectation model;
- a single fixed human listener profile;
- a new node kind for every psychological construct;
- a mandatory MIR/ML dependency in playback;
- a claim that listener response proves composer intent.

## 10. Implementation consequence

The pass justifies:

1. adding `semantic_layer::listener_response` after `auditory_interpretation`;
2. reusing `analysis_feature` with explicit `claim_layer` for first implementations;
3. requiring provenance/model/context for psychological outputs;
4. preserving multiple listener/model responses over one unchanged source/performance graph;
5. keeping `auditory_stream` and persistent musical `part` identity separate from listener-response values.

No new `node_kind` or `edge_kind` is required yet.

## Primary implementation observatories checked

- `olivierlar/mirtoolbox`, especially `miremotion`
- `Kappers/pyidyom`
- existing Essentia/madmom/music21/OpenMusic/Partitura/voice-separation sources already recorded in prior research cases

## Literature families checked

- auditory organization and stream segregation;
- music emotion induction mechanisms;
- musical expectation and Information Dynamics of Music;
- statistical learning and stylistic enculturation;
- music memory and familiarity;
- groove and sensorimotor entrainment;
- predictive timing and motor engagement;
- individual differences in groove/emotion;
- cross-cultural music cognition.

The durable result is the layer distinction, not allegiance to a specific psychology theory or predictor.
