# Holistic musical understanding

## Primary objective

The highest-level musical understanding is the project goal.

Everything below it, from exact bits and bytes through driver execution, synthesis state, separated source audio, symbolic reconstruction, perceptual organization, formal analysis, and documentary research, exists because it can improve, constrain, explain, or validate that understanding.

A successful system should be able to encounter a game soundtrack and develop an integrated understanding comparable in breadth to a strong critic, musicologist, composer, arranger, producer, and audio engineer working together.

For multi-composer soundtracks, one of the strongest tests of that understanding is whether the system can recover enough recurring **compositional grammar** to attribute held-out cues among the historically plausible credited composers for musical reasons rather than implementation shortcuts.

```text
source / sequence / execution truth
        ↓
heard and symbolic musical organization
        ↓
parts • melody • bass • rhythm • harmony
        ↓
phrase • motif • cadence • counterpoint • form
        ↓
track-level musical model
        ↓
soundtrack-wide relationships
        ↓
composer grammars from securely attributed works
        ↓
blind attribution as a capstone stress test
```

Attribution is therefore not a side feature. It is one demanding way to test whether the integrated musical model has become deep enough to distinguish individual compositional thought from shared platform, genre, driver, library, and arrangement conventions.

## What the top layer should understand

Depending on the soundtrack and the available evidence, the system should synthesize claims about:

- melodic language, recurring cells, motifs, themes, transformations, and long-range recall;
- rhythm, groove, meter, pacing, syncopation, repetition, variation, and temporal feel;
- harmony, modality, tonality, voice leading, counterpoint, harmonic rhythm, ambiguity, and cadence;
- form, phrase behavior, sectional function, buildup, arrival, departure, return, interruption, extension, and release;
- instrumentation, synthesis vocabulary, sample language, timbral families, register, density, and orchestration;
- arrangement roles such as foreground, accompaniment, bass foundation, punctuation, countermelody, texture, and transition;
- production and mix hierarchy, including dynamics, spectral balance, effects, contrast, intimacy, scale, and spatial organization;
- expressive trajectory, atmosphere, tension, expectation, dramatic pacing, and the relationship between local gestures and larger arcs;
- stylistic language, influences, historical lineage, genre relationships, platform aesthetics, and the degree to which a soundtrack accepts, exploits, or resists its technological context;
- relationships among tracks, including shared themes, harmonic or timbral vocabularies, character/location associations, transformation networks, and the larger architecture of the soundtrack;
- interactive or game-functional behavior when relevant, including looping, layering, state changes, transitions, adaptive form, and how musical design serves gameplay or narrative;
- recurring compositional rules that distinguish one composer from another within the same project when the evidence supports that distinction.

The result should read as a coherent interpretation of a musical work or soundtrack, not as a checklist with one sentence per feature.

## Composer-grade understanding without invented intent

The aspirational standard is composer-grade structural understanding: the system should understand how the parts support one another well enough to discuss why a choice works, what alternatives would change, how a passage develops prior material, and how local decisions participate in the identity of the whole.

This does not license invented biography or undocumented intention.

Keep these distinct:

```text
what the music demonstrably does
!=
what a compositional model strongly suggests
!=
what documentary evidence says the creator intended
```

The first two can still support deep musical criticism and attribution. Documentary intent strengthens a claim when it exists, but lack of an interview should not reduce analysis to surface description.

## Understanding before attribution

The project should not optimize directly for composer labels and then interpret the winning classifier after the fact.

The correct direction is:

```text
recover the piece
→ understand the piece
→ discover recurring rules across known works
→ build composer grammar
→ test an unknown cue
```

A system that cannot explain the unknown cue's melodic construction, bass logic, phrase behavior, motif transformations, counterpoint, harmony, cadence, and form has not earned a composer attribution simply because a statistical model selects the right name.

The attribution benchmark is intentionally hard because it asks whether the model has learned something stable about musical thought rather than incidental encoding.

## Composition grammar and realization grammar are different

Game music often has several creative layers:

```text
composition
arrangement
sequence / sound-data programming
patch / sample design
driver / engine programming
final realization
```

These may involve different people.

The holistic system should therefore maintain at least two broad grammatical views.

### Composition grammar

- melody;
- bass and harmonic motion;
- rhythm;
- phrase construction;
- motif development;
- counterpoint and voice leading;
- cadence;
- form;
- recurring relations between those dimensions.

### Realization grammar

- patch and sample vocabulary;
- instrument assignment;
- modulation and articulation controls;
- PSG/FM/sample deployment;
- channel allocation;
- panning and hardware control;
- driver idioms;
- optimization and sequence-programming habits.

The same cue can therefore support:

```text
composer A
+
arranger/programmer B
+
shared team patch bank
```

without contradiction.

## Symbolic and sequence evidence is a composer-facing bridge

`MIDI-like` information in this project means symbolic note and sequence information in the broad sense, not the MIDI file format specifically.

Depending on the source family, composer-facing evidence may appear as:

- MIDI notes, tracks, controllers, bends, tempo and meter;
- MML or another text music language;
- tracker notes, rows, effects and instrument commands;
- SMPS, GEMS, N-SPC, SSEQ or another native driver/sequence language;
- validated sequence bytecode recovered from ROM, RAM, executable data or decoded hex;
- score-like source, source code or tables that establish note/rest/duration/instrument/loop structure;
- reconstructed performance events inferred upward from VGM, SPC or other execution evidence;
- external transcriptions retained explicitly as external evidence.

These representations can expose note succession, rests, phrase timing, logical tracks, instrument changes, loops, articulation, modulation and control flow.

They are among the strongest bridges toward composer-level understanding because they often sit near the decisions a composer or arranger actually made.

But they are not the destination and they are not automatically interchangeable.

```text
MIDI track
!= MML voice
!= tracker channel
!= driver logical track
!= physical chip channel
!= persistent musical part
```

The objective is not to turn every source into MIDI. It is to recover as much of the underlying musical program as the evidence supports, then reason upward into composition.

## Cooperative representation law

Every supported representation should be allowed to teach the others while retaining its own semantics.

```text
symbolic source / sequence
        ↕
driver and allocation behavior
        ↕
chip / DSP / sample execution
        ↕
rendered audio and auditory organization
        ↓
shared musical interpretation
        ↓
composer grammar
```

A source with explicit logical tracks and notes can teach the system what persistent musical identity looks like after allocation into hardware resources. VGM or SPC can teach the system where a score-like representation misses synthesis, articulation, sample, allocation or runtime details. Audio can pressure-test whether implementation distinctions actually become meaningful heard distinctions.

This is cross-supervision, not normalization.

A correspondence may be strong without becoming an identity statement. The system should preserve native objects and express mappings, alignments, transformations, support and uncertainty between them.

> Everything may help everything else understand the music, but nothing may erase what makes a source uniquely informative.

## Lower layers are instrumental

Use the lowest layer that materially improves the musical question.

Examples:

```text
exact sequence/driver evidence
    useful when it resolves phrasing, allocation, articulation, looping, or transformation

chip/source isolation
    useful when it resolves instrumentation, counterpoint, effects, or mix hierarchy

rendered audio
    useful when the heard result matters more than hidden implementation state

score-like reconstruction
    useful when pitch/rhythm/harmony/form are the discriminating questions

musicological/documentary evidence
    useful when identity, lineage, version, attribution, or historical practice matters
```

Do not descend to bytes merely because bytes are available. Do not stop at bytes merely because they are exact.

Traceability is valuable when it improves confidence, exposes causality, distinguishes alternatives, or permits correction. It is supporting infrastructure for understanding, not the definition of understanding.

## Holistic synthesis law

Top-level reasoning should integrate mutually constraining evidence rather than analyze dimensions independently and concatenate the outputs.

For example, a section may become structurally larger because several changes coincide:

```text
register expands
+ harmonic rhythm changes
+ countermelody enters
+ bass articulation becomes more active
+ a familiar motif returns in transformed form
        ↓
structural arrival
```

Likewise, a composer grammar should not be a flat feature vector if stronger relations are available.

Prefer:

```text
composer repeatedly reharmonizes a retained upper motif by changing bass motion at phrase-extension points
```

over:

```text
composer has high rate of chord X
```

The relational description explains a compositional behavior.

## Soundtrack-scale understanding

A game soundtrack is not merely a folder of independent tracks.

The system should build a soundtrack-level graph containing relations such as:

```text
track
↕ motif/theme transformations
track
↕ harmonic or modal language
track
↕ phrase/form habits
track
↕ instrumentation/timbre families
track
↕ game / character / location function
track
```

When multiple composers are credited, securely attributed tracks can also support composer-specific subgraphs.

```text
known work families for composer A
        ↓
recurring musical rules
        ↓
composer A grammar

known work families for composer B
        ↓
recurring musical rules
        ↓
composer B grammar
```

Unknown cues can then be tested against those grammars while remaining fully held out.

## Cross-platform same-composer controls

Same-composer music on different machines is a particularly valuable control.

Surface timbre and implementation should change strongly across Genesis VGM, SNES SPC, PSF-family, MIDI, tracker and other sources. Some arrangement/programming habits may also change.

More portable signals should include, where genuinely characteristic:

- melodic contour and transformation;
- phrase construction;
- bass/harmonic strategy;
- rhythmic grammar;
- motivic development;
- counterpoint;
- cadence and formal behavior.

If those relations follow the composer across platforms while patch, sample, driver and channel fingerprints disappear, the evidence becomes substantially more composer-facing.

## Confound interventions

Any composer-attribution benchmark should actively test what the system is using.

Rerun under conditions such as:

```text
full evidence
without patch/sample identity
without instrumentation
without platform-specific features
without arranger/programmer features
transposition-normalized
tempo-normalized
without related versions or derivative cues
```

If the attribution collapses under one intervention, that sensitivity is part of the result.

A high score is less interesting than knowing **why** the score is high.

## Human musical discourse

Human-facing language is a projection over evidence, not another truth layer.

The same musical object may be discussed naturally as a listener, critic, composer, theorist, producer, engineer, or forensic analyst.

The aspirational standard is composer-grade structural understanding without invented intent: the system should understand how musical choices support one another well enough to discuss why a passage works, how material develops, how an arrangement creates contrast, and why a held-out cue resembles one composer's established grammar more than another's.

An attribution explanation should use meaningful musical concepts such as:

- phrase extension;
- bass counterpoint;
- motif fragmentation;
- rhythmic displacement;
- reharmonized return;
- cadence avoidance;
- recurring interval/contour grammar.

It should not expose only opaque vector dimensions.

See `docs/human-musical-discourse.md`, `docs/composer-level-understanding.md`, and `research/composer-grammar-attribution.md`.

## Evaluation target

A useful end-state test is not simply:

> Can the system decode this format?

or:

> Can it identify the chords?

or:

> Can it export plausible MIDI?

The stronger test is:

> After studying this soundtrack, can the system explain what makes it musically itself, how its parts cooperate across time, how themes and formal strategies recur, and how individual composers within the project differ in the way they construct music?

For a multi-composer game, an even harder capstone test is:

> Given only securely attributed controls for the credited composers and a completely held-out cue, can the system identify the most plausible composer from composition-facing musical grammar, survive matched-decoy and confound tests, and explain the result in musically meaningful terms?

A further composer-level test is counterfactual reasoning: what changes if the bass motion, articulation, register, orchestration, countermelody, phrase continuation or motif transformation changes while other material stays fixed?

## Evaluation ladder

```text
L0  source parsing
L1  note / sequence / performance recovery
L2  persistent musical parts
L3  melody / bass / rhythm / harmony
L4  phrase / motif / counterpoint / cadence / form
L5  integrated track-level musical model
L6  recurring compositional rules across independent works
L7  composer grammar
L8  blind attribution among plausible credited composers
L9  robustness to confound interventions and platform changes
L10 musically persuasive, traceable explanation
```

If attribution reaches L8 while the system is weak at L3-L7, investigate leakage before celebrating the score.

## Completion principle

Lower-level machinery is justified when it helps the system climb this ladder.

> **The project succeeds when exact source knowledge grows into a musical model deep enough to explain the song, compare compositional grammars across works, and make difficult authorship distinctions for the right musical reasons.**
