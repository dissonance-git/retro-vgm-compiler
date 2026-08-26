# Holistic musical understanding

This document is the canonical north-star contract for what VGM Compiler means by **understanding music**. `architecture.md` defines evidence and semantic boundaries. `vgm-compiler-roadmap.md` defines what is implemented, active, and next.

This document must stay independent of current implementation status.

## Target

VGM Compiler should understand a cue and a soundtrack as coherent music, not as a bag of notes, register writes, patches, features, or classifier scores.

A useful model explains relationships among:

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

These are not independent feature columns. Their value lies in how they constrain one another over time.

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

The system should identify what persists and what changes without flattening those representations into one identity.

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

Useful role candidates include foreground melody, bass, inner voice, counterline, accompaniment, pad, ostinato, percussion, doubling, effect/gesture, transition material, and texture-bearing source. Roles may coexist or change.

## Melody and motif

Melody should be modeled through interval and contour shape, rhythmic cell and metric placement, register, articulation, expressive pitch behavior, harmonic context, timbral assignment, phrase function, and transformation history.

Motivic relations may include repetition, sequence, fragmentation, extension, truncation, rhythmic change, transposition, inversion-like change, augmentation or diminution where justified, and reassignment among parts or timbres.

A motif becomes musically useful when the system can explain how its transformations participate in larger syntax.

## Rhythm, meter, and groove

Rhythm is not merely onset spacing.

Keep distinct:

```text
source/event timing
performed timing
metric position
rhythmic cell
accent pattern
groove relation
phrase-level pacing
```

Scheduler, driver, sample, or device timing effects should not be silently reinterpreted as authored microtiming. Groove and entrainment are listener/model claims built on performance and auditory evidence, not source facts.

## Harmony and tonal organization

Harmonic understanding must keep sounding pitches, figuration, chord identity, function, and larger tonal plan separate.

The model should be able to reason about structural versus ornamental tones, root/quality/inversion candidates, local tonal centers, bass function, voice leading, harmonic rhythm, chromatic alternatives, and longer preparation/prolongation/resolution relations.

Roman numerals and chord names are analytical projections, not source truth.

## Phrase syntax and cadence

A local sonority or arrival does not determine phrase role by itself.

The model should represent evidence for roles such as:

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

Local cadence morphology must remain separable from grouping, continuation, later resolution, recurrence, and larger formal scope. The same local event may carry different roles at different scales.

Cadence class should therefore emerge from compatible harmonic, voice-leading, grouping, and phrase-role evidence rather than a lookup table or a circular closure test.

## Counterpoint and voice leading

Counterpoint is not reducible to vertical chord labels.

Represent relations among persistent parts through motion type, imitation, registral crossing, preparation and resolution where supported, bass/upper-part dependencies, countermelody identity, texture, density, and phrase interaction.

Voice leading can disambiguate harmony and phrase syntax; harmony can constrain voice-leading interpretation. Preserve underdetermined alternatives rather than making either layer omniscient.

## Arrangement and orchestration

Arrangement is part of musical structure.

Relevant relations include role assignment, register and spacing, doubling, density, texture, entry/exit strategy, timbral contrast, call/response, role transfer, source-family constraints, spatial behavior, and how these participate in form.

A retained melodic cell with changed bass, widened register, brighter timbre, thicker doubling, and delayed cadence is one relational transformation, not five unrelated feature deltas.

## Timbre and sound design

In game music, timbre and synthesis may be structurally musical.

Patch/sample selection, envelope, modulation, detune, noise, PCM behavior, FM topology, routing, rhythmic effects, and source-specific gestures can participate in instrument identity, articulation, foreground/background assignment, phrase punctuation, thematic identity, section contrast, and realization grammar.

Do not discard these to obtain a cleaner score. Do not assume every low-level parameter was a conscious compositional decision.

## Whole-work understanding

A cue model should connect local evidence into larger relations among presentation, continuation, contrast, transformation, return, cadence, loop behavior, release, and unresolved continuation without assuming one universal form template.

Whole-work understanding should explain repeated and transformed regions, hierarchy among boundaries, thematic persistence, orchestration plan, tonal/harmonic planning, and meaningful anomalies.

It should make questions such as `why does this return feel different?` answerable through evidence-backed relations rather than a feature dump.

## Soundtrack-scale understanding

A soundtrack is not merely a folder of independent cues.

The model should study thematic families, quotations, recurring formal or orchestration strategies, cue-function relations where historically grounded, instrumentation and production palettes, exceptions, version/port relationships, collaborator/toolchain boundaries, and creator-specific habits that survive changes of project or platform.

A soundtrack grammar is descriptive evidence, not a requirement that every cue follow one centroid.

## Creator grammar and attribution

Blind creator attribution is a demanding downstream benchmark for musical understanding, not the definition of understanding itself.

Strong controls compare independent axes such as:

```text
same work / different representation
same creator / different soundtrack
same soundtrack / different creator
same driver / different creator
same creator / different platform
```

A creator grammar should model recurring **relations**, not merely feature averages.

Keep composition, arrangement, sequence/sound-data programming, driver programming, patch/sample design, and final realization distinct. A correct label without a defensible musical and historical route is not sufficient.

Detailed experimental design belongs under `../research/music/` or a named integrative testbed under `../research/projects/`.

## Cross-representation teaching

Different representations may supervise one another where a validated alignment exists.

```text
known symbolic part / note / articulation
        ↕
validated driver / hardware realization
        ↓
learn relation while retaining provenance
        ↓
stronger inference when one side is missing elsewhere
```

Cross-supervision can improve inference without proving that another historical source used the supervising representation, driver, or toolchain.

## Human musical explanation

Natural language should project the same evidence at the register appropriate to the user.

A listener may need `the track opens up here`. A theorist may need the register, density, harmonic, and phrase relations supporting that claim. A producer may care about spectral and spatial organization. A forensic user may need exact provenance.

The wording changes; the evidence does not. See `human-musical-discourse.md`.

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

Real corpus controls matter because synthetic examples can accidentally encode the assumptions of the model being tested.

## North-star question

> What prevents VGM Compiler from understanding this music more completely, and what discriminating experiment would remove that uncertainty?

The output format is not the project. The recovered relationships, evidence, uncertainty, and musical meaning are the project.
