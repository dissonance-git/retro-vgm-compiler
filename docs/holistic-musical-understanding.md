# Holistic musical understanding

## Primary objective

The highest-level musical understanding is the project goal.

Everything below it, from exact bits and bytes through driver execution, synthesis state, separated source audio, perceptual organization, formal analysis, and documentary research, exists because it can improve, constrain, explain, or validate that understanding. Those lower layers are not equal end goals merely because they are technically difficult.

A successful system should be able to encounter a game soundtrack and develop an integrated understanding comparable in breadth to a strong critic, musicologist, composer, arranger, producer, and audio engineer working together.

That means being able to reason about the soundtrack as music rather than returning a bag of measurements.

## What the top layer should understand

Depending on the soundtrack and the available evidence, the system should be able to synthesize claims about:

- melodic language, recurring cells, motifs, themes, transformations, and long-range recall;
- rhythm, groove, meter, pacing, syncopation, repetition, variation, and temporal feel;
- harmony, modality, tonality, voice leading, counterpoint, harmonic rhythm, ambiguity, and cadence;
- form, phrase behavior, sectional function, buildup, arrival, departure, return, interruption, and release;
- instrumentation, synthesis vocabulary, sample language, timbral families, register, density, and orchestration;
- arrangement roles such as foreground, accompaniment, bass foundation, punctuation, countermelody, texture, and transition;
- production and mix hierarchy, including dynamics, spectral balance, effects, contrast, intimacy, scale, and spatial organization;
- spatial composition: width, depth, diffusion, foreground/background organization, movement, environmental contribution, and changes in spatial presentation across musical form;
- expressive trajectory, atmosphere, tension, expectation, dramatic pacing, and the relationship between local gestures and larger emotional arcs;
- stylistic language, influences, historical lineage, genre relationships, platform aesthetics, and the degree to which a soundtrack accepts, exploits, or resists its technological context;
- relationships among tracks, including shared themes, harmonic or timbral vocabularies, character/location associations, transformation networks, and the larger architecture of the soundtrack;
- interactive or game-functional behavior when relevant, including looping, layering, state changes, transitions, adaptive form, and how musical design serves gameplay or narrative;
- plausible compositional and production strategies that explain how the musical system hangs together.

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

The first two can still support deep musical criticism. Documentary intent strengthens a claim when it exists, but lack of an interview should not reduce analysis to surface description.

## Lower layers are instrumental

Use the lowest layer that materially improves the musical question.

Examples:

```text
exact sequence/driver evidence
    useful when it resolves phrasing, allocation, articulation, looping, or transformation

chip/source isolation
    useful when it resolves instrumentation, counterpoint, spatial routing, effects, or mix hierarchy

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

For example, a section may feel larger not because of one feature but because several changes coincide:

```text
register expands
+ harmonic rhythm changes
+ countermelody enters
+ bass articulation becomes more active
+ spatial field widens
+ reverb/environment contribution changes
+ a familiar motif returns in transformed form
        ↓
perceived structural arrival
```

The system should reason about that conjunction as one musical event.

Likewise, apparent genre, mood, or style descriptors should be unpacked into the musical mechanisms that create them, then recombined into natural human language.

## Spatial understanding

Spatial analysis is not complete when pan values or source coordinates are recovered.

The top-level questions are musical:

- Which elements occupy foreground, middle ground, background, or diffuse environment?
- Does width or depth change with form?
- Are important arrivals reinforced by expansion or contraction of the scene?
- Does ambience bind the ensemble together or separate layers?
- Are echoes and reverberant returns rhythmic, atmospheric, structural, or all three?
- Do moving or asymmetric sources function as ornament, call-and-response, destabilization, spectacle, or texture?
- Does a recurring instrument retain a spatial identity across sections or tracks?
- How does the soundtrack use spatial contrast to create intimacy, scale, distance, danger, dreamlike ambiguity, or release?
- How do historical device-authored spatial controls relate to what a listener actually perceives?

Exact routing and source separation are valuable because they make these questions easier to answer correctly.

## Soundtrack-scale understanding

A game soundtrack is not merely a folder of independent tracks.

The system should be able to build a soundtrack-level model containing relationships such as:

```text
track
↕ motif/theme transformations
track
↕ instrumentation/timbre families
track
↕ harmonic or modal language
track
↕ narrative/location/character function
track
↕ production and spatial vocabulary
track
```

This makes it possible to discuss a score's internal language, recurring strategies, exceptions, developmental arcs, and stylistic fingerprints across the entire game.

## Evaluation target

A useful end-state test is not simply:

> Can the system decode this format?

or:

> Can it identify the chords?

The stronger test is:

> After studying this soundtrack, can the system explain what makes it musically itself, how its parts cooperate across time, how individual tracks function within the whole score, what the composer/arranger/producer is doing at important moments, and what a perceptive listener is likely to hear as structurally meaningful?

If the answer is weak, more low-level machinery is justified only when it is likely to improve that answer.
