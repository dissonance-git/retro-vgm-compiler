# Holistic musical understanding

This document is the canonical north-star contract for what VGM Compiler means by **understanding music**. `architecture.md` defines evidence and semantic boundaries; this document defines the musical object the system is trying to recover.

## Target

VGM Compiler should understand a cue and a soundtrack as coherent music, not as a bag of notes, register writes, patches, features, or classifier scores.

A useful whole-song model must explain relationships among:

- melody, bass, inner voices, countermelody, accompaniment, percussion, and persistent parts;
- pitch, interval, contour, register, rhythm, meter, groove, articulation, dynamics, and silence;
- harmony, tonal center, chord spelling/function, harmonic rhythm, bass-harmony interaction, voice leading, and counterpoint;
- motifs, transformations, phrase roles, cadence, continuation, return, prolongation, and formal hierarchy;
- patch/sample identity, timbre, synthesis, texture, orchestration, doubling, role transfer, effects, and spatial behavior;
- section-level and whole-work tension, pacing, contrast, recurrence, buildup, release, and closure;
- relationships among cues across a soundtrack;
- work/version/port/arrangement relationships;
- creator-specific grammar and historically scoped attribution hypotheses;
- natural descriptions appropriate to listeners, musicians, critics, theorists, producers, engineers, or forensic users.

These are not independent feature columns. The value lies in their constraints on one another over time.

## Whole-song object

The desired model is relational and hierarchical:

```text
performed events / trajectories
        ↓
persistent parts and gestures
        ↓
local pitch / rhythm / timbre relations
        ↓
motifs / harmonic spans / phrase regions
        ↓
phrase roles / cadences / contrapuntal relations
        ↓
sections and transformations
        ↓
whole-work formal and orchestration plan
        ↓
soundtrack-scale thematic and stylistic relations
```

A section is not just a timestamp interval. It is a region whose role may be supported by recurrence, contrast, tonal plan, phrase syntax, orchestration, texture, motif behavior, and transition history.

A return is not merely `same notes again`. It may retain a motif while changing bass, harmony, register, instrumentation, density, articulation, cadence strength, or source realization.

## Musical identity survives implementation change

The same musical idea may appear in different technical forms:

```text
symbolic sequence
↕
driver execution
↕
physical synthesis
↕
performed gesture
↕
auditory organization
↕
arrangement / version / port
```

The goal is to recognize what persists and what changes without flattening those representations into one.

Useful questions include:

- Is this the same motif transposed, reharmonized, fragmented, extended, rhythmically altered, or timbrally reassigned?
- Does a bass change alter the harmonic function of retained upper material?
- Does a repeated phrase become more closing because of register, orchestration, or cadence rather than pitch content alone?
- Is a port preserving the work while changing the realization, or changing the arrangement itself?
- Does a sound-design gesture function structurally as punctuation, transition, or thematic material?

## Parts before labels

Musical roles depend on persistent behavior, not hardware coordinates.

A melody may migrate between physical channels. A bass may change instrument. Several sources may double one line. A pad may temporarily become foreground. A rhythmic figure may move from percussion to pitched accompaniment.

The model should therefore reason about persistent parts and role hypotheses over time rather than treating one device channel as one musical voice.

Role vocabulary is descriptive, not absolute. Useful candidates include foreground melody, bass, inner voice, counterline, accompaniment, pad, ostinato, percussion, doubling, effect/gesture, transition material, and texture-bearing source.

Roles may coexist or change.

## Melody and motif

Melody should be modeled through more than absolute pitch sequences.

Relevant relations include:

- interval and contour shape;
- rhythmic cell and metric placement;
- registral trajectory;
- articulation and expressive pitch behavior;
- repetition, sequence, inversion-like change, fragmentation, augmentation/diminution where justified, extension, truncation, and mutation;
- harmonic and bass context;
- timbral/orchestration reassignment;
- phrase and formal function.

A motif becomes musically useful when the system can track its transformations and explain how those transformations participate in larger syntax.

## Rhythm, meter, and groove

Rhythm is not merely onset spacing.

The model should distinguish:

```text
source/event timing
performed timing
metric position
rhythmic cell
accent pattern
groove relation
phrase-level pacing
```

Game music may also contain scheduler, driver, sample, or device timing effects that should not be silently reinterpreted as authored microtiming.

Groove and entrainment are listener/model claims built on performance and auditory evidence, not source facts.

## Harmony and tonal organization

Harmonic understanding must keep sounding pitches, figuration, chord identity, function, and larger tonal plan separate.

Useful questions include:

- Which active tones are structural and which are passing, neighboring, suspended, anticipated, arpeggiated, echoed, or ornamental?
- What root/quality/inversion candidates fit a harmonic span?
- What local tonal centers compete?
- Is a chromatic sonority tonicization, modal mixture, prolongation, substitution, or something a tonal label does not explain well?
- How does bass motion alter function?
- How do upper voices prepare or resolve tendency tones?
- What is the harmonic rhythm?
- Which local event participates in a longer preparation/prolongation/resolution relation?

Roman numerals and chord names are analytical projections, not source truth.

## Phrase syntax and cadence

Phrase understanding is now an active frontier because local arrival morphology can disagree with larger-scale continuation.

The compiler already represents:

- part-level phrase-boundary evidence;
- cross-part consensus and phrase regions;
- tonal/key/chord-degree hypotheses;
- harmonic verticalities, transitions, harmonic rhythm, voice leading, and bass interaction;
- cadential-arrival evidence;
- independent formal-closure evidence;
- authentic, half, leading-tone-resolution, deferred-resolution, and deceptive-close candidate morphology;
- an arbitration state that preserves conflict instead of forcing one answer.

The next step is positive **phrase-role evidence**.

A region after an arrival may establish or support:

```text
ending
continuation
new-phrase onset
reroute
return
prolongation
delayed resolution
nested local close inside global continuation
```

For an Ionian `V → VI` arrival, the key question is not whether a lookup table calls it deceptive. The key question is what the surrounding phrase does with the VI arrival, how independent grouping evidence behaves, and whether a later authentic closure belongs to the same larger process.

The same local event may carry different roles at different formal scales.

## Counterpoint and voice leading

Counterpoint is not reducible to vertical chord labels.

The model should represent relations among persistent parts through:

- contrary, similar, parallel, oblique, and independent motion;
- imitation and staggered recurrence;
- registral crossing and exchange;
- preparation and resolution of dissonance where the analytical framework supports it;
- bass/upper-part dependencies;
- countermelody identity;
- texture and density changes;
- phrase-role interactions.

Voice leading can disambiguate harmony and phrase syntax; harmony can also constrain voice-leading interpretation. Preserve the dependency cycle as competing supported hypotheses rather than making either layer omniscient.

## Arrangement and orchestration

Arrangement is part of musical structure, not decoration pasted onto a score.

Useful relations include:

- which part carries which function over time;
- register and spacing;
- doubling/unison/octave relations;
- density and texture;
- entry/exit strategy;
- timbral contrast;
- call/response;
- role transfer;
- source-family constraints;
- effect and spatial behavior tied to form;
- how returns are intensified, reduced, opened, darkened, or otherwise transformed.

A retained melodic cell with changed bass, widened register, brighter timbre, thicker doubling, and delayed cadence is one musical transformation, not five unrelated feature deltas.

## Timbre and sound design

In game music, timbre and synthesis may be compositionally structural.

Patch/sample selection, envelope, modulation, detune, noise, PCM behavior, FM topology, filter/routing state, rhythmic effects, and source-specific gestures can participate in:

- instrument identity;
- articulation;
- foreground/background assignment;
- phrase punctuation;
- thematic identity;
- section contrast;
- dramatic function;
- creator-specific realization grammar.

The system should not throw these away to obtain a cleaner score. It should also avoid pretending every low-level implementation parameter was a conscious compositional decision.

## Whole-work understanding

A complete cue model should connect local evidence into larger relations:

```text
opening / setup
→ presentation of material
→ development / continuation / contrast
→ return / transformation
→ cadence / loop / release / unresolved continuation
```

This is illustrative, not a universal form template.

The system should identify repeated and transformed regions, hierarchy among boundaries, loop function, thematic persistence, orchestration plan, tonal/harmonic planning, and meaningful anomalies.

Whole-work understanding should make it possible to answer `why does this return feel different?` with an evidence-backed relation rather than a feature dump.

## Soundtrack-scale understanding

A soundtrack is not merely a folder of independent cues.

The model should be able to study:

- thematic families and quotations;
- shared or transformed motives;
- recurring harmonic, rhythmic, timbral, or formal strategies;
- cue-function families such as battle, location, menu, character, victory, danger, or transition where historical/game context supports them;
- instrumentation and production palettes;
- exceptions that intentionally break the soundtrack grammar;
- version/port relationships;
- collaborator and toolchain boundaries;
- creator-specific habits that survive changes of project, platform, or arrangement.

A soundtrack grammar is descriptive evidence, not a guarantee that every cue follows one centroid.

## Composer and creator grammar

Blind creator attribution is a demanding downstream benchmark for musical understanding, not the definition of understanding itself.

The strongest design studies two independent axes:

```text
AXIS A: same work across representations
MIDI / MML / tracker / sequence / VGM / SPC / xSF / audio / transcription

AXIS B: same creator across independent works and soundtracks
different games / platforms / years / collaborators / functions
```

A creator grammar should model recurring **relations**, not merely feature averages.

Example:

```text
retained melodic cell
+ altered bass motion
+ widened register
+ timbral reassignment
+ delayed cadence
→ recurring return strategy
```

If that relation appears across unrelated works and soundtracks under securely scoped role evidence, it becomes stronger creator evidence than a single patch, chord statistic, or driver signature.

### Generalization controls

Prefer validation such as:

```text
leave one work family out
leave one soundtrack out
leave one platform out
leave one collaborator/arranger out
leave one career period out
```

A rule found only inside one soundtrack is soundtrack-local until independent evidence shows otherwise.

Style is allowed to evolve. The model should distinguish stable long-range habits, period-specific habits, soundtrack-local habits, collaborator-dependent habits, platform-dependent habits, and one-off experiments.

### Role scope

Attribution must preserve:

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver programming
!= patch / sample design
!= final realization
```

All modalities may contribute. Role provenance determines what the contribution means.

A correct label without a defensible musical and historical route is not sufficient. It can indicate metadata leakage, work-family leakage, driver recognition, patch-bank recognition, arranger recognition, platform recognition, or memorization.

Detailed experimental design belongs in `../research/music/` and named testbeds such as `../research/projects/sonic3/`.

## Cross-representation teaching

Different representations should supervise one another where a real alignment exists.

```text
known symbolic part / note / articulation
        ↕
validated driver / hardware realization
        ↓
learn mapping while retaining provenance
        ↓
stronger inference when one side is missing elsewhere
```

Examples:

- validated logical tracks teach persistent-part behavior through hardware allocation;
- symbolic transcriptions provide anchors for testing inverse recovery;
- exact VGM/SPC control reveals articulation or timbral distinctions omitted by notation;
- sample/patch continuity can support identity through channel reassignment;
- MML/tracker control flow teaches how loops and effects appear downstream.

This is cross-supervision, not proof that an unrelated historical game used the supervising format or driver.

## Human musical explanation

Natural language should project the same evidence at the register appropriate to the user.

A listener may need `the track opens up here`. A theorist may need the register/density/harmonic/phrase changes supporting that claim. A producer may care about spectral and spatial organization. A forensic user may need exact event provenance.

The wording changes; the underlying evidence does not.

See `human-musical-discourse.md`.

## Validation philosophy

A rich model needs adversarial tests, not demonstrations alone.

Useful patterns include:

```text
native source → execute → hide upper semantics → recover upward → compare
```

```text
representation A → model
representation B → model
compare relations without assuming equivalence
```

```text
model → backend B → re-analyze B → model'
measure declared semantic preservation
```

```text
same creator / different soundtrack
same soundtrack / different creator
same driver / different creator
same creator / different platform
```

Real corpus controls matter because synthetic examples can accidentally encode the assumptions of the model being tested.

## North-star question

At every frontier ask:

> What prevents VGM Compiler from understanding this music more completely, and what discriminating experiment would remove that uncertainty?

The output format is not the project. The recovered relationships, evidence, uncertainty, and musical meaning are the project.
