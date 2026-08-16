# Holistic musical understanding

## Primary objective

The highest-level musical understanding is the project goal.

Everything below it, from exact bits and bytes through driver execution, synthesis state, symbolic reconstruction, sample/patch behavior, rendered audio, perceptual organization, formal analysis, and documentary research, exists because it can improve, constrain, explain, or validate that understanding.

A successful system should encounter a game soundtrack and build an integrated model comparable in breadth to a strong critic, musicologist, composer, arranger, producer, and audio engineer working together.

For multi-composer soundtracks, one of the strongest tests is whether the system can recover enough recurring creator-specific grammar to attribute held-out cues for the right musical reasons.

The understanding must generalize along two axes:

```text
same work across different representations
+
same composer across different soundtracks
```

The first asks whether the system can reconcile MIDI/MML/native sequence/VGM/SPC/PSF/audio/etc. as complementary views of one musical object.

The second asks whether it can recognize deeper habits that follow a composer from soundtrack to soundtrack while platforms, collaborators, libraries, genres, and production conditions change.

## The integrated path

```text
source-native truth
        ↓
representation-specific semantics
        ↕
cross-representation correspondences
        ↓
persistent musical parts
        ↓
melody • bass • rhythm • harmony • timbre • articulation
        ↓
phrase • motif • cadence • counterpoint • form • arrangement
        ↓
integrated song model
        ↓
relationships within one soundtrack
        ↓
relationships across different soundtracks by the same creator
        ↓
cross-soundtrack creator grammar
        ↓
blind attribution as a capstone stress test
```

Attribution is not a side classifier. It is one demanding way to test whether the musical model has become deep enough to distinguish creator-specific thought from shared platform, driver, arranger, patch bank, genre, or soundtrack-local conventions.

## What the top layer should understand

Depending on the evidence, the system should synthesize claims about:

- melodic language, recurring cells, motifs, themes, transformations, and long-range recall;
- rhythm, groove, meter, syncopation, repetition, variation, and temporal feel;
- harmony, modality, tonality, voice leading, counterpoint, harmonic rhythm, ambiguity, and cadence;
- phrase behavior, formal function, buildup, arrival, departure, extension, return, interruption, and release;
- instrumentation, synthesis vocabulary, sample language, timbral families, register, density, and orchestration;
- foreground/background roles, accompaniment, bass foundation, punctuation, countermelody, texture, and transition;
- articulation, dynamics, pitch-control behavior, and negative space;
- production and mix hierarchy when relevant to the musical organization;
- expressive trajectory, tension, expectation, pacing, and local-to-global relationships;
- stylistic lineage and platform aesthetics without confusing them with composer identity;
- relationships among tracks inside one soundtrack;
- relationships among unrelated works by the same composer across different soundtracks;
- which traits are stable, period-specific, collaborator-specific, platform-specific, soundtrack-local, or one-off experiments.

The result should be a coherent interpretation, not a checklist of measurements.

## Composer-grade understanding without invented intent

The aspirational standard is composer-grade structural understanding: the system should understand how musical choices support one another well enough to discuss why a passage works, what alternatives would change, how material develops, and how local decisions participate in the identity of the whole.

Keep distinct:

```text
what the music demonstrably does
!=
what a musical model strongly suggests
!=
what documentary evidence says the creator intended
```

Deep analysis does not require invented biography or intention.

## No privileged representation

No source family is the master musical truth.

A symbolic sequence may reveal exact note and phrase structure. VGM can reveal articulation, modulation, channel behavior, and synthesis details. SPC can reveal sample, envelope, pitch-rate, and runtime voice behavior. Audio can reveal grouping, emphasis, texture, and timbral relationships that are not obvious from source data alone.

```text
MIDI / MML / tracker / native sequence
        ↕
driver execution
        ↕
chip / DSP / sample behavior
        ↕
performed gesture
        ↕
auditory organization
```

Everything may teach everything else, but native semantics stay intact.

This means:

```text
MIDI track
!= driver logical track
!= physical channel
!= physical voice episode
!= persistent musical part
!= auditory stream
```

and:

```text
MIDI program
!= FM patch
!= BRR sample
!= tracker instrument
```

A correspondence may be strong without becoming an equivalence.

## Cross-representation understanding

For one work, the system should ask which musical relations survive across representations.

Examples:

- does the melody inferred from VGM agree with the native sequence or external MIDI transcription?
- does an SPC sample change correspond to an articulation or orchestration change in the symbolic layer?
- does the rendered audio support the persistent-part grouping inferred from execution state?
- do several native channels cooperate to realize one persistent musical role?
- does one symbolic part migrate across physical resources without losing musical identity?

Disagreement is evidence. It may reveal transcription loss, allocation effects, synthesis-dependent articulation, incomplete runtime capture, or incorrect inference.

## Cross-soundtrack understanding

Understanding one soundtrack is not enough to identify a composer robustly.

A soundtrack contains many shared local causes:

- one engine or driver;
- one platform;
- one production period;
- one patch/sample bank;
- one sound team;
- recurring cue functions;
- shared arrangers/programmers;
- related themes and derivative cues.

So creator-level grammar should be tested across **different soundtracks** whenever possible.

```text
composer A
├── soundtrack 1
│   ├── independent work family 1
│   └── independent work family 2
├── soundtrack 2
│   ├── independent work family 3
│   └── independent work family 4
└── soundtrack 3
    └── independent work family 5
```

The key question is:

> What musical behaviors follow the composer when the soundtrack around them changes?

## Composer grammar is multi-view

A creator-specific grammar can include several evidence families.

### Structural / symbolic

- melodic contour and interval relations;
- bass and harmonic motion;
- rhythm and meter;
- phrase construction;
- motif development;
- counterpoint and voice leading;
- cadence and form.

### Arrangement / orchestration

- register and density;
- doubling;
- countermelody strategy;
- foreground/background organization;
- instrument-role migration;
- textural treatment of returns and transitions.

### Timbre / synthesis

- patch/sample choices where creator-controlled;
- envelope/articulation habits;
- modulation behavior;
- timbral contrast;
- synthesis changes tied to form.

### Performance / execution

- expressive pitch control;
- attack/release behavior;
- dynamic contour;
- microtiming where recoverable;
- silence and negative space;
- interaction between implementation and phrase shape.

### Soundtrack-level

- thematic reuse;
- cue-family transformation;
- repeated solutions to location/character/state functions;
- recurring harmonic/timbral pairings;
- how the composer solves similar musical problems in different projects.

These views should constrain one another rather than be concatenated blindly.

## Role scope, not modality censorship

Game music often distributes creative work across several people:

```text
composition
!= arrangement
!= sequence / sound-data programming
!= patch / sample design
!= driver / engine programming
!= final realization
```

This does not mean timbre, arrangement, or execution are forbidden from composer attribution.

It means every observation must carry role provenance.

A patch, articulation, or orchestration habit may be genuine composer evidence when the composer historically controlled that layer. The same feature may instead belong to an arranger, programmer, shared library, or toolchain in another soundtrack.

So the law is:

```text
all representations may contribute
+
role provenance determines what each contribution means
```

## Composer evolution

A composer is not a frozen centroid.

The model should distinguish:

```text
stable long-range habits
career-period habits
soundtrack-local habits
collaborator-dependent habits
platform-dependent habits
one-off experiments
```

A useful composer grammar is therefore a structured region with trajectories, not one static vector.

## Holistic synthesis law

Top-level reasoning should integrate mutually constraining evidence.

For example:

```text
retained melodic cell
+ changed bass motion
+ widened register
+ brighter timbral assignment
+ countermelody entry
+ delayed cadence
        ↓
structural return with increased weight
```

If a related strategy recurs across independent works and different soundtracks, it can become creator-level evidence.

Prefer relational claims such as:

```text
composer repeatedly reharmonizes a retained upper motif by changing bass motion at phrase-extension points
```

over flat statistics such as:

```text
composer uses chord X frequently
```

## Soundtrack-scale understanding

A soundtrack is not a folder of unrelated tracks.

Build a graph containing relations such as:

```text
track
↕ motif/theme transformations
track
↕ harmonic/modal language
track
↕ phrase/form habits
track
↕ instrumentation/timbre families
track
↕ game / character / location function
track
```

For multi-composer projects, securely attributed tracks can form composer-specific subgraphs.

Those subgraphs should then be compared with the same composers' work in other soundtracks.

## Strong validation controls

Preferred validation modes include:

```text
leave-one-work-family-out
leave-one-soundtrack-out
leave-one-platform-out
leave-one-arranger-out
leave-one-career-period-out
```

Matched-decoy controls include:

```text
same driver + different composer
same patch/sample bank + different composer
same arranger + different composer
same composer + different arranger
same composer + different platform
same composer + different soundtrack
similar cue function + different composer
```

## Confound interventions

Any composer-attribution benchmark should actively test what the system is using.

Rerun under conditions such as:

```text
full evidence
without patch/sample identity
without instrumentation
without platform-specific features
without arranger/programmer features
without soundtrack-local features
transposition-normalized
tempo-normalized
without related versions or derivative cues
```

If attribution collapses under one intervention, that sensitivity is part of the result.

A high score is less interesting than knowing why the score is high.

## Human musical discourse

Human-facing language is a projection over evidence, not another truth layer.

The system should be able to explain not only what a passage contains but what the musical relationships are doing:

- phrase extension;
- bass counterpoint;
- motif fragmentation;
- rhythmic displacement;
- reharmonized return;
- cadence avoidance;
- timbral reinforcement of structural events;
- recurring cross-soundtrack transformation strategies.

An attribution explanation should expose those concepts, not opaque latent dimensions.

## Evaluation target

A useful end-state test is not simply:

> Can the system decode this format?

or:

> Can it identify the chords?

or:

> Can it export plausible MIDI?

The stronger test is:

> Can the system understand a work across its available representations, understand a composer across unrelated soundtracks, and explain which musical behaviors survive changes in platform, collaborators, arrangement, and production?

For a multi-composer game, the capstone becomes:

> Given securely attributed controls from this soundtrack and other soundtracks by the same candidates, can the system identify the most plausible composer of a completely held-out cue, survive matched-decoy and confound tests, and explain the result in musically meaningful terms?

## Evaluation ladder

```text
L0  source parsing
L1  note / sequence / performance recovery
L2  persistent musical parts
L3  melody / bass / rhythm / harmony / timbre / articulation
L4  phrase / motif / counterpoint / cadence / form / arrangement
L5  integrated cross-representation song model
L6  recurring rules across independent works
L7  cross-soundtrack composer grammar
L8  blind attribution among plausible credited composers
L9  robustness to soundtrack/platform/representation confound tests
L10 musically persuasive, traceable explanation
```

If attribution reaches L8 while the system is weak at L3-L7, investigate leakage before celebrating the result.

## Completion principle

Lower-level machinery is justified when it helps the system climb this ladder.

> **The project succeeds when exact source knowledge grows into a cross-representation musical model, that model generalizes across independent works and soundtracks, and difficult authorship distinctions emerge for defensible musical reasons.**
