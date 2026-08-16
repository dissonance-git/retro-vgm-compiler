# Cross-cultural pitch, tuning, rhythm, and meter pressure pass

## Status

Research input for the provenance-aware musical execution model.

This pass extends the existing music-cognition, theory, psychology, and musicology research into ethnomusicology and cross-cultural computational music research. Its purpose is not to add a catalog of traditions to the core model. It is to find assumptions that look universal only because many music-computing systems were built around Western notation, 12-tone equal temperament, and common-practice meter.

## Question

> Which pitch and rhythm claims are genuinely source/acoustic facts, which are culturally or theoretically scoped categories, and which apparently familiar concepts must be allowed to remain unknown or not applicable?

## Core result

The current semantic layers are sufficient. The pass does **not** justify a new layer.

It does require stricter representation discipline:

```text
acoustic frequency
!= device-native pitch coordinate
!= authored pitch symbol
!= tuning-relative interval
!= scale degree / melodic category
!= perceived pitch category

exact event timing
!= beat
!= meter
!= named rhythmic cycle / framework
!= listener entrainment
```

Higher analysis must state the representation/theory/tradition that makes a categorical pitch or rhythmic claim meaningful.

## 1. GitHub observatory: DaMuSc

Repository:

- `jomimc/DaMuSc`

DaMuSc is a cross-cultural database of musical scales assembled from ethnomusicological sources.

Several implementation/data-design choices are particularly important for VGM Tooling:

- theoretical scales and measured scales are separate source classes;
- theory scales can be specified through exact frequency ratios;
- measured scales can come from instrument tunings or computational analysis of recordings and are stored in cents as reported by the source;
- octave-normalized scales are **derived** from the raw data rather than assumed to be the only legitimate representation;
- the processing code explicitly says not all scales are automatically assumed to have octave equivalence;
- for measured scales, tonic may be unclear and the repository explicitly notes that the concept of tonic may not even be relevant for a given scale;
- individual scales retain links to the ethnomusicological sources and measurement methods that support them.

This gives VGM Tooling a direct cross-cultural analogue of its source-evidence law:

```text
raw measured/theoretical evidence
        ↓
optional culturally/theoretically justified transformation
        ↓
scale representation
```

not:

```text
all pitch data
→ force into 12-TET note names + tonic + octave class
```

## 2. GitHub observatory: SymbTr

Repository:

- `MTG/SymbTr`

SymbTr is a large machine-readable symbolic collection of Turkish makam music. The current repository describes thousands of works spanning many makams, usuls, and forms and provides text, MusicXML, MIDI, PDF, and `mu2` representations.

The important pressure is not the repertoire size. It is that the same musical source can have:

- culture-specific symbolic pitch categories;
- makam identity;
- usul/rhythmic identity;
- score structure;
- microtonal notation;
- multiple exchange/projection formats.

MusicXML or MIDI are therefore projections of the repertoire, not proof that Western equal-tempered pitch or generic MIDI timing is the ontology.

SymbTr's documentation also points to microtonal notation software and score-analysis/synthesis tools rather than pretending conventional Western notation captures every relevant distinction.

## 3. GitHub observatory: Turkish makam tuning/intonation dataset

Repository:

- `MTG/otmm_tuning_intonation_dataset`

The dataset pairs recordings from several Turkish makams with associated SymbTr scores specifically to test note-modeling, tuning, and intonation analysis methods.

This is a valuable forward/inverse pressure case:

```text
symbolic score category
        ↕
performed recording
        ↓
measured intonation/tuning
```

The symbolic note and the performed pitch are related but not identical objects.

## 4. GitHub observatory: adaptive SymbTr synthesis

Repository resolved by GitHub as:

- `hsercanatli/symbtrsynthesis`

The synthesizer can render a SymbTr score using either:

- theoretical intervals;
- tuning extracted from performed pitches in a related recording.

This is nearly a perfect executable demonstration of the distinction VGM Tooling needs:

```text
same symbolic score
        │
        ├─ theoretical tuning realization
        └─ performance-derived tuning realization
```

Therefore:

```text
authored pitch category
!= one fixed acoustic frequency
```

and:

```text
one score identity
can legitimately map to multiple intonation realizations
```

The mapping itself is evidence and should preserve which tuning/reference produced it.

## 5. Literature: cross-cultural scale diversity

Research using DaMuSc and related corpora finds both convergence and diversity in scale systems across societies.

Important methodological consequence:

> population-level regularities do not authorize a universal per-piece encoding assumption.

Even when octave-like intervals, small step sizes, or approximately equidistant 5/7-note systems are statistically common, an individual source must still be represented according to the evidence it actually preserves.

Do not turn a cross-cultural distribution into a parser default.

## 6. Literature: measured intonation is not Western transcription error

Research on Georgian traditional singing and microtonal oral traditions demonstrates pitch organizations and continuous/drifting intonation that Western notation can represent poorly.

Some corpora contain:

- neutral intervals;
- systematic differences between melodic and harmonic interval sizes;
- gradual pitch drift;
- tuning systems inferred statistically over many performances rather than fixed a priori.

This gives a direct negative control for a future VGM reconstruction system:

```text
deviation from 12-TET
!= error to correct
```

Any pitch restoration or enhancement must preserve source-supported intonation before applying a tuning hypothesis.

## 7. Literature: pitch category is psychocultural as well as acoustic

Cross-cultural and psychocultural research emphasizes that physical frequency, perceived pitch, musical interval category, naming system, and culturally learned scale organization are distinct.

Even seemingly natural metaphors such as pitch being "high" or "low" are not represented identically across cultures.

For VGM Tooling the safe representation order is:

```text
physical/device frequency evidence
        ↓
performance pitch trajectory
        ↓
interval / tuning relation under explicit reference
        ↓
scale-degree / mode / makam / raga / theory category
        ↓
listener category / expectation under a specified model
```

The later categories must never rewrite the earlier physical evidence.

## 8. Rhythm and meter are also culturally scoped

Cross-cultural rhythm work finds both common biases and culture-specific priors.

A recent large comparison across many participant groups found sparse mental rhythm priors with small-integer-ratio categories across groups, while the relative importance of the categories varied substantially with local musical practice.

This is exactly the pattern the model should expect:

```text
possible cross-cultural regularity
+
culture-specific distribution
```

not a universal fixed meter vocabulary.

Ethnomusicological work also warns that some music is poorly described by models that require an isochronous lowest metrical pulse, while other traditions use named cyclic rhythmic frameworks whose semantic structure exceeds a generic numerator/denominator time signature.

Therefore:

```text
exact source timing
can exist without a justified meter claim
```

and:

```text
named rhythmic cycle/framework
may contain structure not captured by Western time signature alone
```

## 9. Feature consequences

The source-relative feature carrier should prefer precise questions rather than one ambiguous universal `pitch` field.

Candidate feature families include:

### Physical / synthesis pitch

```text
device_native_pitch_code
oscillator_frequency_hz
sample_playback_rate
```

These belong to `synthesis` when supported.

### Performance pitch

```text
performed_pitch_frequency_hz
performed_pitch_trajectory
performed_interval_ratio
```

These belong to `musical_performance` and can be exact/derived/hypothetical depending on source evidence.

### Authored symbolic pitch

```text
authored_pitch_token
authored_scale_degree
authored_mode_or_makam_category
```

These belong to `authored_program` when the source explicitly encodes them.

### Structural/theoretical pitch

```text
inferred_scale_degree
inferred_tonic
inferred_mode_or_scale
interval_category
```

These belong to `musical_structure` and require theory/tradition/reference provenance.

### Listener pitch category

Categorical perception or expectation belongs to `auditory_interpretation` or `listener_response` depending on the actual question.

## 10. The name `normalized_absolute_pitch` is unsafe

Earlier exploratory VGM/SPC feature work used the placeholder question `normalized_absolute_pitch` and deliberately left it unknown.

This pass shows why that wording should not become permanent API language.

It ambiguously mixes:

- absolute acoustic frequency;
- tuning/reference normalization;
- symbolic note category;
- possibly 12-TET/MIDI assumptions.

The safer future questions are representation-specific, e.g.:

```text
performed_pitch_frequency_hz
performed_pitch_log_frequency
pitch_interval_relative_to_explicit_reference
scale_degree_under_named_system
```

The current source adapters should stop exposing the ambiguous placeholder before any higher analysis begins depending on it.

## 11. Rhythm feature consequences

Avoid a universal feature that assumes every event stream has a Western-style meter.

Prefer questions such as:

```text
authored_rhythmic_framework
explicit_meter
inferred_beat_period
inferred_meter
named_cycle_or_usul
free_rhythm_hypothesis
microtiming_pattern
```

Each question gets its own claim layer, availability, evidence state, and model/tradition provenance.

For a source with exact timing but no justified metric organization:

```text
event_timing            present
explicit_meter          unavailable or not_applicable
inferred_meter          unknown
```

Do not invent 4/4 because a downstream library wants bars.

## 12. Cross-cultural guardrail for listener models

The previous `listener_response` pass already requires corpus/listener/model provenance.

Cross-cultural work makes the reason concrete:

- pitch expectations depend on learned musical systems;
- rhythm priors differ across populations/traditions;
- consonance preferences and tonal hierarchies can be enculturated;
- familiarity and cultural distance alter prediction and response.

Therefore a listener model trained on Western corpora cannot silently become the default human model for unrelated traditions.

## 13. Cross-cultural guardrail for reconstruction

For enhanced rendering/reconstruction:

```text
source-supported intonation/timing
        = invariant candidate

historical hardware quantization / bandwidth limits
        = possible relaxable constraint
```

But deciding which irregularity belongs to which category requires evidence.

Do not "correct":

- microtonal intervals toward 12-TET;
- expressive pitch drift toward a fixed grid;
- non-isochronous timing toward a metronomic subdivision;
- unfamiliar tuning toward Western consonance;
- culture-specific ornamentation toward generic note centers.

A reconstruction that destroys those properties can be higher fidelity acoustically while being lower fidelity musically.

## 14. Immediate executable controls

The current graph and `analysis_feature` mechanism should be able to represent the following without new ontology:

1. one exact acoustic/performed frequency supporting two different culturally/theoretically scoped categorical interpretations;
2. an exact authored microtonal pitch token whose performed frequency is unknown until a tuning/reference realization is selected;
3. the same authored symbolic score mapped to theoretical and performance-derived tuning realizations;
4. exact event timing with meter `unknown`, `unavailable`, or `not_applicable` rather than forced into a Western time signature;
5. exact named rhythmic-framework/usul evidence remaining distinct from inferred listener beat/meter;
6. octave equivalence and tonic identity remaining unresolved when the source/tradition does not justify them.

If those controls pass, no new semantic layer or node kind is needed.

## 15. What this pass does not justify

Do not add:

- a universal 12-TET pitch field;
- MIDI note number as canonical pitch;
- automatic octave equivalence;
- automatic tonic inference as source truth;
- a universal major/minor or diatonic scale ontology;
- a universal Western meter/time-signature model;
- a permanent taxonomy of world music traditions in the core graph;
- culture labels inferred from musical stereotypes;
- a default Western listener model;
- automatic tuning correction in enhanced rendering.

## Primary GitHub observatories checked

- `jomimc/DaMuSc`
- `MTG/SymbTr`
- `MTG/otmm_tuning_intonation_dataset`
- `hsercanatli/symbtrsynthesis`
- existing music21/OpenMusic/Partitura/IDyOM/MIR sources recorded in earlier cases

## Literature families checked

- large cross-cultural scale databases;
- scale evolution and cross-cultural regularities;
- microtonal/oral-tradition tuning extraction;
- Turkish makam tuning and score/audio linkage;
- Georgian traditional tuning and pitch drift;
- psychocultural models of musical interval;
- cross-cultural music cognition methodology;
- global rhythm priors;
- ethnomusicological meter/microrhythm/free-rhythm critiques;
- cross-cultural theories of music and musicality.

The durable result is a representation law: preserve physical/source coordinates first, attach cultural/theoretical categories only with explicit scope, and never mistake a familiar projection for universal musical truth.
