# Timbre, instrument identity, and organology pressure pass

## Status

Research input for the provenance-aware musical execution model.

This pass targets one of the major unresolved bridges in VGM Tooling: when may an exact synthesis object, sample, patch, or source be called an "instrument," and what does that word mean at different layers?

## Core distinction

```text
authored instrument label
!= synthesis object identity
!= acoustic timbre
!= perceived timbre/source category
!= organological or historical instrument identity
!= musical role
```

The current semantic layers and `analysis_feature` mechanism are sufficient. No new semantic layer, node kind, or edge kind is justified by this pass.

## 1. Synthesis identity is often stronger than instrument naming

Executable game-music sources can directly prove objects such as:

- an exact YM2612 patch/operator state;
- one BRR sample version at a proven RAM generation;
- one PSG/noise source;
- one MIDI program/module patch when the device state is known;
- one tracker instrument/sample object;
- one driver instrument index or explicit authored label.

Those are strong source/synthesis facts.

But a patch or sample is not automatically a stable semantic instrument name.

Examples:

- one FM patch can be used for melody in one section and accompaniment in another;
- two patch definitions can intentionally emulate the same instrument family;
- one BRR sample can be transposed across a wide register;
- one tracker instrument can map different notes to different samples;
- one authored instrument label can describe an intended semantic target rather than the literal synthesis mechanism;
- a chip patch can be deliberately synthetic and have no acoustic-instrument equivalent.

Therefore:

```text
exact synthesis_object
can exist while
semantic instrument identity = unknown or not applicable
```

## 2. Timbre is multidimensional and context dependent

Psychophysical timbre research, especially work by Stephen McAdams and collaborators, models timbre as multidimensional rather than one scalar color.

Timbre depends on spectral, temporal, and spectrotemporal properties and interacts strongly with:

- fundamental frequency;
- playing effort / loudness;
- articulation and excitation;
- attack/decay behavior;
- source mechanics;
- listening context and previous source knowledge.

Important consequence for VGM Tooling:

> an acoustic timbre descriptor is an observation of one realization of a source object, not the permanent identity of that object.

A stable FM patch or BRR sample may produce changing spectral centroid, brightness, envelope shape, partial balance, or attack characteristics across pitch and dynamics.

Do not hash an acoustic descriptor vector and call it instrument identity.

## 3. Source mechanics and perceptual timbre are related but not identical

Research on sound-source mechanics shows that listeners' timbre judgments carry information about excitation type and instrument family, but identification confusions are structured and imperfect.

Timbre can support source-category inference while remaining a perceptual/acoustic representation rather than direct physical truth.

For VGM Tooling:

```text
known synthesis mechanism
        synthesis truth

acoustic descriptors
        acoustic realization

perceived timbre/source family
        auditory interpretation

named physical/historical instrument
        authored or musicological claim depending on evidence route
```

These claims may agree, but agreement must be measured rather than assumed.

## 4. Instrument classifiers are model outputs

### GitHub observatory: Essentia

Repository:

- `MTG/essentia`

Essentia's documented high-level classifiers include instrumentation/timbre-related outputs alongside genre, mood, western/non-western, tonal/atonal, danceability, voice/instrumental, and brightness labels.

The documentation makes several epistemic constraints explicit:

- models are trained on annotated collections;
- accuracy depends on the training data;
- model versions depend on Essentia versions;
- newer TensorFlow models can supersede older SVM models;
- classifier input requires the descriptor layout expected by training;
- evaluation uses cross-validation and can be biased by artist/album leakage if the split is careless.

This is exactly the provenance VGM Tooling should preserve for any audio-derived instrument label.

An output such as:

```text
instrument_family = violin
confidence = 0.81
```

is an `auditory_interpretation` or model-analysis hypothesis. It cannot overwrite an exact source patch/sample identity.

## 5. Polyphonic mixtures weaken instrument inference

Instrument-identification literature commonly reports substantially harder conditions in polyphonic mixtures than for isolated tones. Systems often need joint pitch/source models, note templates, NMF, learned embeddings, or classifiers restricted to a known instrument set.

VGM Tooling has a major advantage when executable source state exists:

```text
source-native isolated synthesis contributions
        ↓
render each known source independently
        ↓
measure timbre / classifier output per source
```

That is stronger than asking an audio classifier to rediscover sources from the final mix.

Use generic mixture inference only where source isolation is genuinely unavailable.

## 6. Natural instrument emulation is not binary identity

Research comparing natural orchestral instruments with sampled, FM-synthesized, and hybrid emulations found that listeners can distinguish natural instruments from imperfect synthesized counterparts even when broad instrument categories are recognizable.

This is directly relevant to game-audio enhancement.

A chip patch might have:

```text
authored semantic target     "strings"
exact synthesis identity     YM2612 patch P
acoustic realization         chip-rendered timbre T
perceived category           string-like hypothesis
```

A future enhanced reconstruction could search for a higher-fidelity realization compatible with the source evidence, but it must not rewrite patch P into "literal violin source" unless stronger evidence exists.

## 7. GitHub observatory: OM-Orchidee

Repository:

- `openmusic-project/OM-Orchidee`

OM-Orchidee distinguishes:

- a `SOUNDTARGET` object defining the orchestration target;
- an `ORCHESTRA` object defining available realization resources;
- candidate orchestration solutions that can be browsed/exported.

This is a powerful conceptual pressure for reconstruction:

```text
acoustic / musical target
!= one predetermined instrument source
```

A target timbre may have several candidate realizations.

For VGM Tooling, later reconstruction can similarly ask:

> Which higher-fidelity synthesis/instrument realization best satisfies the exact musical/synthesis constraints and the measured target characteristics?

The solution remains a candidate realization, not recovered historical truth.

## 8. Organology and historical instrument identity

Organological categories classify instruments by aspects such as excitation/source mechanics, construction, or historical/cultural identity.

For pure electronic/chip synthesis, some acoustic-instrument classifications may simply be `not_applicable`.

For samples or explicitly named emulations, an organological label can be:

- exact authored metadata when the source names it;
- exact external annotation relative to a catalog;
- derived when linked to a documented instrument sample set;
- hypothetical when inferred from acoustic similarity.

Do not force every synthesis object into an acoustic-instrument family.

## 9. Musical role is yet another claim

An instrument/source can function as:

- melody;
- bass;
- accompaniment;
- pad;
- percussion;
- countermelody;
- effect;
- structural layer.

Those are musical-performance/structure roles, not timbre identities.

The same patch can change role without changing synthesis identity.

## 10. Feature consequences

Useful distinct questions include:

### Authored/source

```text
authored_instrument_token
driver_instrument_index
source_instrument_label
```

### Synthesis

```text
synthesis_object_id
sample_version_id
patch_content_identity
excitation_mechanism
```

### Acoustic

```text
spectral_centroid
spectral_envelope
attack_time
spectrotemporal_descriptor
partial_structure
```

### Auditory interpretation

```text
perceived_timbre_category
instrument_family_classifier_output
timbre_similarity
```

### Musicological context

```text
documented_instrument_identity
organological_class
historical_instrument_annotation
reference_instrument_hypothesis
```

### Musical performance / structure

```text
instrument_role
persistent_part_assignment
orchestration_relation
```

Do not collapse these into one `instrument` string.

## 11. Reconstruction guardrail

A high-quality reconstruction should preserve a ladder of obligations:

```text
exact authored/driver identity, when available
        ↓
exact synthesis/sample/patch constraints
        ↓
measured acoustic/timbre characteristics
        ↓
perceptual category evidence
        ↓
optional external/documentary instrument identity
        ↓
candidate enhanced realization
```

The lower source evidence outranks a generic classifier.

When the semantic acoustic-instrument target is unresolved, enhanced rendering should prefer transformations that preserve the source's timbral relationships rather than choosing a literal modern instrument by guess.

## 12. Immediate executable control

The existing feature/layer model should represent all of the following simultaneously:

1. one exact synthesis object identity;
2. two acoustic realizations of that same object at different pitch/dynamic conditions with different descriptor values;
3. one auditory instrument-family classifier hypothesis with explicit model provenance;
4. an authored instrument label that may agree or disagree with the classifier without changing synthesis truth;
5. an organological classification marked not-applicable for a deliberately synthetic source;
6. a musical-role hypothesis distinct from instrument identity.

If this passes, no new semantic layer or node kind is needed.

## What this pass does not justify

Do not add:

- one universal `instrument` string;
- classifier output as source identity;
- timbre descriptor vector as persistent instrument identity;
- automatic acoustic-instrument labeling for synthetic patches;
- one fixed timbre vector independent of pitch/dynamics/articulation;
- blind replacement of chip timbre with a modern acoustic instrument;
- a large instrument classifier as a realtime playback dependency;
- a permanent organology taxonomy in the common graph.

## Primary GitHub observatories checked

- `MTG/essentia`
- `openmusic-project/OM-Orchidee`
- existing OpenMusic, sample, tracker, Genesis, SPC, and musicological sources already recorded in the repo

## Literature families checked

- multidimensional timbre perception;
- source-mechanics and timbre perception;
- instrument identification from isolated and polyphonic audio;
- natural versus sampled/FM/hybrid instrument emulation;
- spectrotemporal representations of instrument timbre;
- orchestration and computer-aided timbre matching.

The durable result is another scoped-identity law: exact synthesis identity, acoustic timbre, perceived category, semantic label, musical role, and historical instrument identity are linked but non-identical claims.
