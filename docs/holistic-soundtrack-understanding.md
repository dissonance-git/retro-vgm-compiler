# Holistic soundtrack understanding

## North star

Retro VGM Compiler exists to understand game music as music.

The primary success condition is not recovering more bytes, registers, channels, stems, notes, chords, or metadata in isolation. Those are supporting representations. The primary success condition is that the system can build a coherent, revisable model of a game's soundtrack at the level a strong critic, composer, musician, producer, and musicologist would recognize as genuine understanding.

A successful system should be able to listen to or inspect a soundtrack and explain, in ordinary musical language, what makes the score itself work.

Its ideal output should feel as though the analyst has internalized the soundtrack's compositional logic from the inside, while still refusing to invent undocumented creator intent.

```text
PRIMARY OBJECT
holistic understanding of the soundtrack as a musical world

SUPPORTING EVIDENCE
score / sequence / driver / chip / sample / audio / historical / perceptual data

OPTIONAL DEPTH
exact explanation of how a particular implementation produced a particular audible event
```

The lower layers matter because they can make the top-level understanding deeper, more accurate, more specific, and more defensible. They are not the reason the project exists.

## What "understand the soundtrack" means

The system should be able to construct and discuss at least the following dimensions together rather than as disconnected feature reports.

### Composition

- melodic language;
- interval and contour habits;
- motivic construction and transformation;
- rhythmic vocabulary and groove;
- harmonic language, tonal centers, modality and chromatic behavior where applicable;
- bass writing;
- counterpoint and voice leading;
- phrase design;
- cadence and closure behavior;
- repetition, variation and development;
- local and large-scale form.

### Arrangement and orchestration

- persistent musical roles;
- register and spacing;
- density and negative space;
- voicing;
- foreground/background relationships;
- doubling and contrast;
- instrumental or synthesized color as an arranging decision;
- transitions and sectional re-orchestration;
- how technical voice limits became musical choices where relevant.

### Sound design and production

- timbral vocabulary;
- patch/sample families;
- articulation and envelope behavior;
- spatial organization;
- ambience, delay and other effects;
- mix hierarchy;
- texture and spectral balance;
- production signatures that materially shape musical identity.

### Dramatic and game function

- title/menu/field/battle/dungeon/cutscene/character/location function;
- relation between musical pacing and gameplay pacing;
- tension, release, anticipation and reward;
- how repetition and looping are made musically tolerable or expressive;
- how music changes the perceived identity of a place, event, character or mechanic;
- thematic callbacks and transformations across game states;
- diegetic or world-building function where relevant.

### Soundtrack-scale identity

- what unifies the score;
- what deliberately contrasts inside it;
- recurring harmonic, melodic, rhythmic, textural and timbral fingerprints;
- families of tracks and exceptions;
- thematic networks across the game;
- progression of musical language across the game's arc;
- stylistic lineage and historical context;
- relationship to the platform's technology without reducing the score to that technology;
- relationship to the composer's or sound team's wider body of work when evidence supports it.

### Critical and musicological judgment

The system should be capable of arguments such as:

```text
why this soundtrack is unusually coherent
why a track feels structurally weaker or stronger than another
why a battle theme creates urgency without simply being faster
why a town theme feels intimate rather than merely quiet
why a late-game reprise changes the meaning of an earlier motif
why the score sounds characteristic of this composer while still departing from their usual habits
why a technological limitation became an aesthetic signature rather than merely a defect
```

Evaluation is interpretive, not encoded truth. The system should distinguish evidence-backed description from critical judgment while still being capable of both.

## The composer-level target

"As if the composer wrote it" means depth of internalized musical logic, not fabricated biographical intent.

The system should be able to reason counterfactually from the learned soundtrack model:

- what kinds of continuation would fit or violate this track's language;
- which variation techniques are characteristic of this score;
- what a return is preserving and what it is changing;
- which instrumental/timbral substitutions would preserve the arrangement's function;
- what musical problem a transition appears to solve;
- how a cue balances loopability against directional form;
- how a motif can be transformed while remaining recognizable;
- which aspects belong to the composition and which belong to implementation.

This is a stronger test of understanding than naming a chord or decoding a register.

Documented creator intent remains a separate evidence class. The system may say what a musical decision accomplishes without claiming that the composer consciously intended that exact explanation unless documentary evidence exists.

## Whole-score before microscope

The default analytical direction should be top-down and iterative.

```text
whole soundtrack impression / hypotheses
        ↓
track families and soundtrack-scale relations
        ↓
individual track form, gesture, harmony, rhythm, timbre and function
        ↓
important musical moments and mechanisms
        ↓
only where useful: source / driver / device / byte-level explanation
        ↑
new lower evidence may revise the higher model
```

This does not mean ignoring exact evidence. It means asking exact questions because they discriminate among meaningful musical hypotheses.

A low-level investigation is high-value when it changes or strengthens the musical model. Examples:

- revealing that an apparent echo is programmed counterpoint rather than DSP delay;
- showing that a recurring timbre is one transformed instrument rather than several unrelated patches;
- proving that a bass articulation is authored rather than an emulator artifact;
- recovering persistent parts that expose a hidden voice-leading relation;
- distinguishing composition from arrangement or implementation in an attribution question.

A low-level result that does not affect musical understanding may still be useful engineering infrastructure, but it should not be mistaken for progress on the primary objective.

## Default output

When asked about a game soundtrack, the preferred output is not a telemetry report and not a sequence of isolated labels.

It should resemble a strong long-form critical/musicological account that can move fluidly among scales:

```text
SOUNDTRACK
what musical world does this score create?

TRACK FAMILY
what unifies these cues and why are they differentiated?

TRACK
how does this piece unfold and what is its dramatic function?

MOMENT
what makes this passage work?

MECHANISM
what musical relationship produces that effect?

IMPLEMENTATION
only if useful: how was that relationship realized technically?
```

The answer should synthesize rather than merely enumerate.

## Validation target

A serious evaluation should ask the system to produce a holistic soundtrack review and then interrogate it from several directions.

### Global understanding

Can it explain:

- the score's overall identity;
- its principal musical contrasts;
- its recurring compositional grammar;
- its thematic and timbral networks;
- its relation to game structure;
- its historical/stylistic position?

### Track understanding

For representative cues, can it explain:

- formal trajectory;
- melody, harmony, rhythm and bass interaction;
- orchestration and texture;
- production/sound design;
- dramatic/game function;
- relation to other tracks?

### Cross-track reasoning

Can it recognize:

- transformed motifs;
- shared progression or bass schemas;
- recurring arranging devices;
- intentional exceptions;
- soundtrack-scale pacing and contrast;
- reused material whose function changes in context?

### Counterfactual understanding

Can it distinguish plausible from implausible continuations or alterations and explain why, using the soundtrack's own learned grammar rather than generic genre rules?

### Evidence descent

When challenged, can it descend from a holistic claim to the strongest relevant evidence available?

This is a support test, not the headline test.

## Failure modes

The project should treat the following as failures even when the underlying extraction is technically correct:

- a feature inventory with no coherent interpretation;
- track-by-track summaries that never form a soundtrack-level model;
- chord labels without harmonic or dramatic meaning;
- timbre descriptions disconnected from arrangement and composition;
- implementation trivia presented as if it were musical analysis;
- critic-like prose that cannot distinguish tracks or defend its claims;
- generic genre adjectives substituted for mechanisms;
- a technically perfect explanation of one event that contributes nothing to understanding the score;
- invented creator intent;
- treating every soundtrack through one Western-theory template;
- treating platform limitations as the primary identity of the music.

## Research priority rule

When choosing between two possible next tasks, prefer the one with greater expected gain in holistic musical understanding.

A useful rough priority test is:

```text
Will this work help the system understand
    the soundtrack,
    a track's musical logic,
    a relation among tracks,
    a compositional/arranging habit,
    a dramatic function,
    or a meaningful ambiguity
better than it does now?
```

If yes, it is primary research.

If it mainly improves extraction, playback, provenance, or implementation detail without changing the musical model, it is supporting infrastructure. Supporting infrastructure remains necessary, but it should be justified by the higher question it unlocks whenever possible.

## Durable rule

> **The project is not trying to understand the machine and eventually reach the music. It is trying to understand the music, using as much of the machine as necessary to make that understanding exceptional.**
