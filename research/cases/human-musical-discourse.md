# Human musical discourse

## Purpose

VGM Tooling is intended to understand music well enough to discuss it naturally, not merely to emit correct technical labels.

The project already separates source, execution, synthesis, musical performance, musical structure, acoustic realization, auditory interpretation, listener response, and musicological context. That is an evidence architecture. It is **not** by itself a model of how people actually talk about music.

This pass studies a different question:

> Given a grounded understanding of what is happening in a piece, how do different kinds of people naturally describe what they hear, what they intended, or what they would change?

The answer matters because a technically correct sentence can still be a bad musical description.

For example:

```text
upper-register activity increased while simultaneous part count rose
```

may support a natural description such as:

```text
it opens up here
```

The second sentence is not a lossy substitute for the first. It is a different **discourse projection** over the same evidence.

## Central finding

Human musical language is strongly metaphorical, relational, embodied, and purpose-dependent.

This is true for lay listeners, critics, musicians, composers, producers, mixing engineers, mastering engineers, and scholars. Expertise changes vocabulary and precision, but does not eliminate metaphor.

People routinely describe sound and music through other experiential domains:

- motion;
- space;
- force;
- weight;
- texture/material;
- light and colour;
- breath and bodily state;
- architecture/container relations;
- conversation/social action;
- narrative and arrival;
- energy, pressure, release, and momentum.

This is not merely decorative language. Research on musical metaphor, timbre semantics, music criticism, and recording discourse shows that these mappings are central to how people conceptualize and communicate musical experience.

Therefore:

```text
natural musical description
!= technical terminology with simpler words
```

and:

```text
metaphor
!= unsupported claim
```

A metaphorical description can be well grounded when its supporting musical/acoustic changes are explicit.

## Discourse projection, not another truth layer

Do not add a `human_language` semantic layer to the musical execution graph.

Human-facing language is a **projection** over evidence-bearing claims.

```text
source / execution / synthesis evidence
        ↓
performance + structure + acoustic/perceptual analyses
        ↓
comparison across time / parts / sections
        ↓
discourse projection
        ├─ listener
        ├─ reviewer / critic
        ├─ composer / musician
        ├─ producer
        ├─ engineer
        └─ forensic / technical
```

The projection may select different facts, different metaphors, and different levels of precision for different audiences without changing the underlying graph.

A person can also switch registers. A producer may speak casually in one sentence and technically in the next. These are discourse modes, not immutable human categories.

## Why the previous phrasing failed

A sentence such as:

> this return is musically larger because the upper texture widened, this part migrated into a brighter timbre, the bass articulation changed, and the programmed echo now reinforces the phrase boundary

contains plausible analytical ideas, but it compresses several discourse communities into one unnatural sentence.

A listener or reviewer is more likely to say something like:

```text
when it comes back, it suddenly feels bigger
```

A producer might say:

```text
the return needs to hit bigger
```

or:

```text
that extra top line gives the return the lift
```

A mix engineer might say:

```text
the return opens up and the low end has more bite
```

Only after being asked *why* should VGM Tooling descend into a more technical explanation.

The mistake was not the analysis. The mistake was forcing the analysis vocabulary into the first human-facing sentence.

## Discourse communities

### Ordinary listener

Primary concerns:

- what stands out;
- what changed;
- what it feels like;
- what it reminds them of;
- where attention goes;
- whether the music feels bigger, smaller, faster, heavier, stranger, calmer, brighter, darker, more exciting, emptier, fuller, etc.;
- memorable moments and transitions.

Typical language is experiential and comparative:

```text
it gets really big here
that little sound sneaks in
this part feels like it's floating
then everything drops out
this bit keeps pulling you forward
the bass starts bouncing more
it suddenly feels darker
it sounds like it's far away
this is the part that gets stuck in my head
```

The project must not assume that an ordinary listener wants a theory label for every perception.

### Reviewer / critic

A reviewer commonly combines:

- listening description;
- evaluation;
- metaphor;
- cultural comparison;
- genre vocabulary;
- narrative framing;
- production commentary;
- selective technical language.

Research on music criticism finds motion metaphors especially prominent, with container/space and linguistic-creation metaphors also common. Critical writing frequently treats music as something that moves, rises, falls, surges, drifts, gathers, collapses, opens, closes, contains, speaks, argues, breathes, or transforms.

Reviewer language is often more image-rich than ordinary conversation:

```text
the groove lurches forward
synths bloom around the melody
the arrangement stays skeletal
percussion keeps needling the beat
the chorus bursts open
the track never quite settles
the bass drags everything downward
the melody keeps circling the same thought
the production turns claustrophobic
```

Evaluation is part of the genre. A reviewer may describe the same musical behavior positively or negatively depending on context.

Therefore VGM Tooling must distinguish:

```text
observed / inferred musical behavior
!= evaluative judgment about that behavior
```

### Composer / songwriter / performer

Creator language often centers on:

- intention;
- shape;
- movement;
- contrast;
- pacing;
- tension/release;
- thematic identity;
- expressive gesture;
- physical playability;
- space and silence;
- interaction among parts;
- emotional or narrative target.

Common forms include:

```text
I wanted it to breathe
I needed more space there
this part had to keep moving
I wanted the melody to feel suspended
the second section needed a different character
that rhythm gives it the push
I wanted the line to answer the first one
this is where it finally arrives
```

An external interview can document such intent. VGM Tooling must never invent intent merely because a technical pattern resembles one.

```text
musical effect
!= documented creator intention
```

### Producer

Producer language is frequently **goal-oriented and intervention-oriented**.

The producer asks what the song needs rather than merely what the signal contains.

Common concepts include:

- lift;
- energy;
- vibe;
- momentum;
- impact;
- contrast;
- drop;
- build;
- hook;
- making room;
- thinning or filling an arrangement;
- bringing something forward;
- letting something breathe;
- making a section hit;
- getting out of the singer's way;
- adding or removing information;
- making transitions feel inevitable or surprising.

Examples of producer-style descriptions:

```text
the chorus needs more lift
pull that part out so the hook hits harder
let the verse breathe
the drums need to push the song more
this section has too much information
bring the bass in later so the entrance matters
mute that there and let the vocal carry it
it needs another gear when the chorus lands
```

This discourse often sits directly between musical structure and studio manipulation.

### Mixing / mastering / audio engineer

Engineering language connects perception to controllable signal relationships.

Common families include:

```text
POSITION
forward • back • close • distant • centered • off to the side

WIDTH / SPACE
wide • narrow • open • boxed-in • roomy • dry • wet • deep • shallow

SPECTRAL CHARACTER
bright • dark • warm • cold • harsh • smooth • airy • dull • muddy • brittle

MASS / BODY
thin • thick • full • hollow • weight • body • heft • beef

TRANSIENT / FORCE
punch • snap • bite • smack • hit • impact • soft • rounded

COHESION
blend • glue • separate • poke out • disappear • mask • fight

DYNAMICS / MOTION
breathe • pump • choke • clamp • open up • flatten • jump out

CLARITY / DETAIL
clean • clear • defined • smeared • blurred • crowded
```

These words are not one-to-one synonyms for DSP parameters.

For example:

```text
"open"
```

might involve some combination of spectral balance, reduced masking, increased width, increased room/reverb audibility, lower density, altered dynamics, or source arrangement.

Likewise:

```text
"punch"
```

may depend on transient shape, low-frequency balance, crest factor, timing, masking, arrangement, and playback context.

The system must avoid pseudo-precision such as mapping `spectral_centroid > X` directly to `bright` as universal truth.

### Forensic / technical explanation

This is the mode VGM Tooling already handles best.

It can state:

- exact source commands;
- driver state;
- register values;
- patch/sample identity;
- control trajectories;
- timing;
- voice allocation;
- synthesis/routing state;
- measured acoustic properties;
- analytical features and provenance.

This mode is essential, but it should normally be the **answer to “why?”**, not the only way the system knows how to discuss music.

## Recurrent metaphor families

### Motion

```text
move • push • pull • drag • rush • lurch • glide • float
climb • fall • rise • sink • settle • drive • bounce • swing
circle • wander • charge • creep • snap back • arrive
```

Potential support can include tempo/rhythm, onset density, contour, bass motion, phrase trajectory, dynamic change, repetition, syncopation, directional pitch movement, or structural expectation.

### Space and container

```text
open • close • widen • narrow • deep • shallow • forward • back
room • crowded • empty • boxed-in • surrounding • distant • inside
```

Potential support can include source count, register occupancy, stereo/spatial spread, masking, reverb/delay, spectral occupancy, arrangement density, and foreground/background relations.

### Weight and force

```text
heavy • light • weighty • punchy • hard • soft • hits • slams
pushes • presses • snaps • bites • kicks • lands
```

Potential support can include low-frequency energy, transient behavior, dynamics, rhythm, articulation, accent placement, distortion, density, and expectation.

### Material and texture

```text
thick • thin • smooth • rough • silky • grainy • glassy • fuzzy
crunchy • brittle • smeared • clean • velvety • metallic • woody
```

Potential support can include timbre, spectrum, modulation, distortion, partial structure, synthesis topology, noise content, density, and articulation.

### Light, colour, and temperature

```text
bright • dark • warm • cold • glowing • sparkling • murky
washed-out • vivid • pale • saturated
```

Potential support can include spectral balance, harmonic content, dynamics, register, distortion, reverberation, and learned timbral conventions.

### Body and breath

```text
breathes • chokes • inhales • exhales • pulses • tenses • relaxes
loose • tight • stiff • fluid
```

Potential support can include dynamic range, compression-like behavior, envelope timing, rhythmic elasticity, phrase spacing, rests, sustain, and density.

### Social action and conversation

```text
answers • interrupts • takes over • steps back • sneaks in • argues
supports • shadows • follows • calls • responds • leaves room
```

Potential support can include part-entry order, imitation, call/response, overlap, foreground/background changes, register handoff, motif transfer, or accompaniment relations.

### Narrative and structure

```text
builds • drops • returns • comes home • turns • stalls • hangs
breaks away • finally arrives • refuses to resolve • resets
```

Potential support can include section recurrence, formal contrast, harmonic expectation, loop behavior, density, motif return, cadence-like behavior, or externally documented game context.

### Energy and vibe

```text
lift • momentum • energy • intensity • drive • release • groove
feel • pocket • tension • pressure • vibe
```

These are higher-order summaries. They usually require a bundle of evidence rather than one low-level feature.

## No one-to-one phrase dictionary

A major implementation warning from this pass is that human descriptors are **many-to-many**.

```text
one technical change
→ several possible human descriptions

one human description
← several possible technical causes
```

Example:

```text
"the chorus opens up"
```

could be supported by:

- more voices entering;
- higher register activity;
- wider stereo distribution;
- less masking;
- more audible ambience;
- longer note sustain;
- brighter timbre;
- a louder master level;
- reduced low-mid density;
- or several of these together.

Therefore do not implement:

```text
if width > threshold:
    say "opens up"
```

Instead, a discourse renderer should receive a **support bundle** plus comparison context.

Conceptually:

```text
claim / comparison
+ supporting observations
+ confidence
+ discourse mode
+ requested detail
→ natural description
```

The wording is a projection. The support bundle is the evidence.

## Same evidence, different human registers

Suppose a section boundary has the following supported changes:

```text
+ two additional parts enter
+ upper-register activity increases
+ stereo spread increases
+ bass attack becomes shorter / more accented
+ overall level rises slightly
```

Possible projections:

### Listener

```text
It suddenly gets bigger and more energetic when that part comes back.
```

### Reviewer

```text
The return bursts open, with the upper parts giving the groove a new sense of lift.
```

### Composer / musician

```text
The return works because the parts spread out and the bass starts articulating the pulse more strongly.
```

### Producer

```text
That extra top layer and harder bass attack are what make the return hit.
```

### Engineer

```text
The return gets wider and brighter, while the bass gains more bite.
```

### Forensic

```text
At the section boundary, simultaneous part count rises from N to N+2, register occupancy expands upward, measured stereo width increases, bass envelope/attack state changes, and integrated level rises by the measured amount.
```

None of the first five should claim an unsupported mechanism. None of them needs to recite every supporting fact.

## Conversational descent

The desired interaction pattern is progressive disclosure.

```text
USER: What happens here?

VGM TOOLING:
It opens up and starts pushing harder.

USER: What changed?

VGM TOOLING:
A higher part comes in, the bass gets more pointed, and the sound spreads out.

USER: What exactly makes the bass more pointed?

VGM TOOLING:
[driver / envelope / articulation explanation with exact provenance]
```

The system should be capable of reaching register writes and source bytes without forcing that level into the first answer.

## Intent language needs independent evidence

Phrases such as:

```text
the composer wanted...
the producer was trying to...
this was meant to...
```

are historical/intent claims.

They require evidence such as:

- interview;
- production notes;
- correspondence;
- source comments;
- session documentation;
- explicit authored annotation;
- other reliable documentary evidence.

Without that evidence, VGM Tooling can say:

```text
this makes the section feel...
this functions like...
this creates...
```

under an appropriate analytical/listener model, but not invent creator intent.

Martin Galway's surviving Wizball source is especially valuable because the file itself documents design/code/music/arrangement authorship and the surrounding interview record contains his own descriptions of changing character, flow, and breathing. That creates an unusually strong control between source evidence and creator discourse.

## Review language adds evaluation

Reviewer language requires an additional distinction:

```text
description
!= evaluation
```

For the same dense arrangement:

```text
"rich and layered"
```

and:

```text
"cluttered and overstuffed"
```

may refer to similar observable density while expressing different judgments.

VGM Tooling should be able to describe the behavior without pretending one evaluation is objectively encoded in the source.

When generating criticism-like language, evaluation should be explicitly requested or tied to a declared critical/listener perspective.

## Human language corpora as observatories

### Song Describer Dataset

The Song Describer Dataset contains roughly 1.1k human-written descriptions for more than 700 permissively licensed recordings. It is valuable because the captions are natural free-form descriptions rather than a fixed MIR tag vocabulary.

Useful role:

- learn what information ordinary annotators choose to mention;
- inspect phrase structure and descriptor combinations;
- compare technical feature salience with human salience;
- build evaluation fixtures for natural description.

It should not become the project's musical ontology.

### MusicCaps

MusicCaps contains more than 5,000 short clips described in free text by musicians, plus aspect lists covering instrumentation, mood, tempo, rhythm, vocal properties, production and other audible characteristics.

Useful role:

- musician/expert caption vocabulary;
- relation between structured aspects and free prose;
- examples of selective description rather than exhaustive analysis.

### Music-review corpora

Pitchfork and other review corpora are useful for studying critic discourse at scale. Existing research already mines Pitchfork reviews for figurative evaluation and vocal-sound descriptors.

Useful role:

- metaphor families;
- evaluative language;
- production vocabulary;
- how critics move between cultural comparison, sound description, structure and judgment;
- what reviewers omit even when technical analysis could say more.

Copyrighted review text should be treated as research material, not copied wholesale into project fixtures.

## Practitioner observatories

### Producers and mixers

Studio interviews show repeatedly that practitioners communicate with language such as:

- chorus `lift`;
- making a section `hit`;
- music `breathing`;
- sounds moving `forward` or `back`;
- parts `fighting` or `making room`;
- compression `gluing` a mix;
- low end being `tight` or `floppy`;
- vocals being `in your face`;
- a mix becoming `open`, `harsh`, `small`, `wide`, `punchy`, or `deep`.

These descriptions often appear immediately beside precise EQ, compression, automation, routing or level decisions. Human metaphor and technical causality coexist naturally in expert discourse.

### Composer / musician direction

Musicians and producers frequently give intentionally underspecified directions such as:

```text
more exciting here
leave more space
make it move
don't fill there
let it breathe
```

The performer or engineer interprets these goals through domain knowledge.

This is important for VGM Tooling: natural language should communicate musical **function and perception**, while the evidence graph supplies the detailed mechanics when needed.

## Implication for LLM-facing representation

The LLM should not receive only a bag of low-level descriptors and be asked to improvise prose.

A safer input is a time-aligned evidence bundle such as:

```text
SPAN
section / local time / loop-relative position

WHAT CHANGED
parts entered/exited
register distribution
rhythmic activity
pitch/melodic contour
instrument/timbre state
dynamics/articulation
spatial/routing state
repetition/return relation

WHAT STAYED THE SAME
motif / harmony / bass pattern / pulse / instrumentation where supported

PERCEPTUAL HYPOTHESES
foreground/background
grouping
salience
relative brightness/weight/openness/etc. under a declared model

CONTEXT
previous span
following span
known game function
known documentary intent

EVIDENCE
exact / derived / hypothesis
support references
confidence / limitations
```

The language model can then produce audience-appropriate prose without losing the provenance boundary.

## Evaluation obligations

A discourse projection should be tested separately from the underlying musical analysis.

### 1. Recognizability

A listener hearing the relevant passage should recognize that the description refers to what actually happens.

### 2. Grounding

Every concrete causal/detail claim in the prose must be traceable to supporting graph evidence or an explicitly identified perceptual/analytical model.

### 3. Register fit

A listener answer should not sound like a measurement report. An engineering answer should not hide actionable signal relationships behind poetic prose.

### 4. Progressive detail

The system should answer simply first when the question is simple, then descend when asked.

### 5. Counterfactual sensitivity

If the evidence bundle changes materially, the description should change. A generic caption that fits every song is a failure.

### 6. Paraphrase robustness

Several natural phrasings may be equally valid. Evaluation should compare supported meaning, not demand one canonical sentence.

### 7. No invented intent

Creator-intent language requires documentary evidence.

### 8. No fake universality

Words such as `bright`, `tense`, `groovy`, `warm`, or `sad` may depend on culture, genre, listener, playback context, and learned convention. Preserve scope where it matters.

### 9. No metaphor-to-feature collapse

Do not define one universal DSP threshold for `open`, `warm`, `punchy`, `heavy`, `airy`, `dark`, etc.

## Immediate real-song controls

### Wizball title music

Martin Galway's 1987 `wizball.asm` is an unusually valuable object because it combines:

- original source;
- driver program;
- music data;
- explicit tune table;
- three concurrent title-music programs;
- frequency/pitch programs;
- envelopes;
- filter/control behavior;
- loops and calls;
- documentary authorship;
- surviving creator commentary about character, flow and breathing.

This makes it suitable for testing the full chain:

```text
source / driver fact
→ musical behavior
→ heard change
→ natural description
→ technical explanation on demand
```

The first success criterion is not whether the system can name every SID register. It is whether someone familiar with the track would hear the description and think:

```text
yes, that's the bit you're talking about
```

### Koshiro / MUCOM88

MUCOM88 remains the complementary control because it provides a cleaner authored-MML / driver / FM-programming lineage.

It is particularly useful for testing whether creator/programmer vocabulary about FM shaping and musical effect can be connected to exact authored controls without confusing implementation fingerprints with composition identity.

## What not to build yet

This pass does **not** justify:

- a permanent natural-language ontology;
- one dictionary from measurements to adjectives;
- a fixed listener persona;
- a critic-emulation subsystem;
- embedding copyrighted review prose into the runtime;
- replacing evidence-bearing analysis with an audio-captioning model;
- treating LLM fluency as proof of musical understanding.

The immediate need is a bounded projection/evaluation layer over existing evidence.

## Strongest resulting rule

VGM Tooling should be able to talk about music at the level the person asked for.

```text
human first answer
        ↓ on request
musical explanation
        ↓ on request
production / synthesis explanation
        ↓ on request
exact driver / device / source evidence
```

Natural language is the doorway into the graph, not a replacement for it.

## Research sources

Key literature and corpora inspected in this pass include:

- Lawrence M. Zbikowski, `Metaphor and Music`, 2008.
- Inesa Šeškauskienė and Totilė Levandauskaitė, `Conceptualising Music: Metaphors of Classical Music Reviews`, 2013, DOI `10.5755/J01.SAL.0.23.5268`.
- Nina Julich-Warpakowski, `Motion Metaphors in Music Criticism`, 2022, DOI `10.1075/milcc.10`.
- Paula Pérez-Sobrino and Nina Julich, `Let's Talk Music: A Corpus-Based Account of Musical Motion`, 2014, DOI `10.1080/10926488.2014.948800`.
- Charalampos Saitis and Stefan Weinzierl, `Concepts of timbre emerging from musician linguistic expressions`, 2017, DOI `10.1121/1.4988381`.
- Asterios Zacharakis, Konstantinos Pastiadis and Joshua D. Reiss, work on inter-language timbre semantics, 2014, DOI `10.1525/MP.2014.31.4.339`.
- Mads Walther-Hansen, work on metaphorical recording discourse and `Making Sense of Recordings`.
- Francis Rumsey, `Spatial Quality Evaluation for Reproduced Sound: Terminology, Meaning, and a Scene-Based Paradigm`, 2002.
- Nick Zacharov, Torben Holm Pedersen and Chris Pike, work on a common spatial-sound quality lexicon, 2016.
- Ilaria Manco et al., `The Song Describer Dataset`, 2023.
- Google / MusicLM, `MusicCaps`, 2023.
- Marcin Trojszczak, `Expressing negative opinions through metaphor and simile in popular music reviews`, 2024, DOI `10.1515/lpp-2024-0036`.
- Anthony T. Pinter et al., `P4KxSpotify`, 2020, DOI `10.1609/icwsm.v14i1.7355`.

Practitioner/reviewer sources inspected include interviews and mix discussions from Sound On Sound, Tape Op, MusicRadar, Mix, The Creative Independent, Pitchfork, and Martin Galway's published C64 source/interview material.

Detailed individual-source claims should continue to be checked against the primary interview/article when promoted into durable historical assertions.
