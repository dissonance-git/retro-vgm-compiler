# Human musical discourse

VGM Tooling should understand music deeply enough to discuss it naturally as well as explain it technically.

This document defines the durable project rule for human-facing musical language. Detailed source mining and corpus notes belong in `research/cases/human-musical-discourse.md`.

## Central rule

Human musical language is not another semantic truth layer.

It is a **discourse projection** over evidence-bearing musical, acoustic, perceptual, historical, and technical claims.

```text
source / execution / synthesis evidence
        ↓
performance + structure + acoustic/perceptual analyses
        ↓
comparison across time / parts / sections
        ↓
discourse projection
        ├─ ordinary listener
        ├─ reviewer / critic
        ├─ composer / musician
        ├─ producer
        ├─ mixing / mastering engineer
        └─ forensic / technical
```

The underlying evidence does not change when the wording changes.

Therefore:

```text
natural musical description
!= technical terminology with simpler words
```

and:

```text
metaphorical language
!= unsupported language
```

A phrase such as `it opens up here` may be well grounded when the system can point to the musical/acoustic changes that support it.

## Why this matters

A technically correct sentence can still be a poor description of music.

For example:

```text
upper-register occupancy increased while simultaneous part count rose
```

may support:

```text
it opens up here
```

The first is a technical statement. The second is how a person might naturally describe the experience. Neither replaces the other.

VGM Tooling should normally lead with the musical description and descend into mechanism when asked.

## Discourse communities

These are modes, not fixed identities. The same person may switch registers from sentence to sentence.

### Ordinary listener

Typical concerns:

- what changed;
- what stands out;
- where attention goes;
- what the section feels like;
- memorable moments;
- comparison with what came before.

Natural language includes phrases such as:

```text
it gets really big here
that little sound sneaks in
then everything drops out
the bass starts bouncing more
this part feels like it's floating
it suddenly gets darker
this is the bit that sticks in my head
```

Do not force theory terminology into this mode unless it is useful to the conversation.

### Reviewer / critic

Critical discourse often combines:

- description;
- evaluation;
- metaphor;
- genre and cultural comparison;
- narrative framing;
- production commentary;
- selective technical language.

Common patterns treat music as motion, space, material, force, weather/light, architecture, conversation, or narrative:

```text
the groove lurches forward
the synths bloom around the melody
the arrangement stays skeletal
the chorus bursts open
the track never quite settles
the bass drags everything downward
the melody keeps circling the same thought
```

Evaluation must remain separate from description.

```text
observed / inferred musical behavior
!= critical judgment about that behavior
```

A dense arrangement can be described as `rich and layered` or `cluttered and overstuffed`. The source does not objectively encode which judgment is correct.

### Composer / musician

Creator discourse often centers on:

- intention;
- shape and movement;
- contrast;
- pacing;
- tension and release;
- thematic identity;
- expressive gesture;
- interaction among parts;
- physical playability;
- space and silence.

Typical language includes:

```text
I wanted it to breathe
this section needed a different character
that rhythm gives it the push
I needed more space there
this is where it finally arrives
```

Intent requires independent evidence such as interviews, source comments, notes, correspondence, or other reliable documentation.

```text
musical effect
!= documented creator intent
```

### Producer

Producer discourse is often goal-oriented and intervention-oriented.

Common concepts include:

- lift;
- energy;
- vibe;
- momentum;
- impact;
- build and drop;
- hook;
- making room;
- thinning or filling an arrangement;
- bringing something forward;
- letting something breathe;
- making a section hit.

Typical language includes:

```text
the chorus needs more lift
pull that part out so the hook hits harder
let the verse breathe
the drums need to push the song more
there is too much information here
it needs another gear when the chorus lands
```

This mode naturally bridges musical structure and production intervention.

### Mixing / mastering engineer

Engineering discourse connects perception to controllable signal relationships.

Useful vocabulary families include:

```text
POSITION
forward • back • close • distant • centered

WIDTH / SPACE
wide • narrow • open • boxed-in • roomy • dry • wet • deep • shallow

SPECTRAL CHARACTER
bright • dark • warm • cold • harsh • smooth • airy • dull • muddy • brittle

MASS / BODY
thin • thick • full • hollow • weight • body • heft

TRANSIENT / FORCE
punch • snap • bite • smack • hit • impact • rounded

COHESION
blend • glue • separate • poke out • disappear • mask • fight

DYNAMICS / MOTION
breathe • pump • choke • clamp • open up • flatten • jump out

CLARITY / DETAIL
clean • clear • defined • smeared • blurred • crowded
```

These terms are many-to-many perceptual concepts, not aliases for one DSP parameter.

### Forensic / technical

This mode exposes the exact machinery:

- source commands and bytes;
- driver state and control flow;
- register values;
- patch/sample identity;
- envelopes and modulation;
- voice allocation;
- timing and clock mappings;
- synthesis/routing state;
- acoustic measurements;
- analytical features with provenance.

This is usually the answer to `why?` or `technically, what is happening?`, not the only vocabulary the system should know.

## Orthogonal discourse acts

Who is speaking is only one axis. The system must also distinguish what the speaker is doing.

```text
DESCRIBING
what seems to be happening

COMPARING
how this differs from another moment/version

INTERPRETING
what a pattern may mean or how it functions

EVALUATING
whether it works, succeeds, feels excessive, etc.

DIAGNOSING
what is causing an audible or musical problem

DIRECTING
what should change

EXPLAINING
what mechanism produces the effect

REPORTING INTENT
what a documented creator says they meant to do
```

These acts can share vocabulary but carry different evidence obligations.

## Recurrent metaphor families

Human musical discourse repeatedly maps sound and music onto other experiential domains.

### Motion

```text
move • push • pull • drag • rush • lurch • glide • float
climb • fall • rise • sink • settle • drive • bounce • swing
circle • wander • charge • creep • snap back • arrive
```

### Space and container

```text
open • close • widen • narrow • deep • shallow • forward • back
room • crowded • empty • boxed-in • surrounding • distant • inside
```

### Weight and force

```text
heavy • light • weighty • punchy • hard • soft • hits • slams
pushes • presses • snaps • bites • kicks • lands
```

### Material and texture

```text
thick • thin • smooth • rough • silky • grainy • glassy • fuzzy
crunchy • brittle • smeared • clean • velvety • metallic • woody
```

### Light, colour, and temperature

```text
bright • dark • warm • cold • glowing • sparkling • murky
washed-out • vivid • pale • saturated
```

### Body and breath

```text
breathes • chokes • inhales • exhales • pulses • tenses • relaxes
loose • tight • stiff • fluid
```

### Social action and conversation

```text
answers • interrupts • takes over • steps back • sneaks in • argues
supports • shadows • follows • calls • responds • leaves room
```

### Narrative and structure

```text
builds • drops • returns • comes home • turns • stalls • hangs
breaks away • finally arrives • refuses to resolve • resets
```

These metaphors can be grounded by different bundles of musical/acoustic evidence. Do not collapse them into universal feature thresholds.

## Many-to-many rule

Human descriptors and technical causes are many-to-many.

```text
one technical change
→ several possible human descriptions

one human description
← several possible technical causes
```

For example, `the chorus opens up` could be supported by some combination of:

- more parts entering;
- increased upper-register activity;
- greater stereo spread;
- reduced masking;
- more audible ambience;
- lower arrangement density;
- longer sustain;
- brighter timbre;
- higher level;
- reduced low-mid congestion.

Do not implement rules such as:

```text
if stereo_width > threshold:
    say "opens up"
```

Instead, a discourse renderer should conceptually receive:

```text
claim / comparison
+ supporting observations
+ confidence
+ discourse mode
+ discourse act
+ requested detail
→ natural description
```

The wording is a projection. The support bundle carries the evidence.

## Progressive disclosure

The preferred conversational pattern is descent on demand.

```text
USER
What happens here?

DEFAULT
It opens up and starts pushing harder.

USER
What changed?

MUSICAL EXPLANATION
A higher part comes in, the bass gets more pointed, and the sound spreads out.

USER
What exactly makes the bass more pointed?

TECHNICAL EXPLANATION
[driver / envelope / articulation explanation with exact provenance]
```

Default discussion should sound like a knowledgeable person listening to music, not a telemetry dump.

## Default register

Unless the user requests another register, the preferred user-facing mode is a **knowledgeable listening companion**:

- musically informed without unnecessary jargon;
- comfortable with ordinary metaphors;
- able to mention melody, bass, rhythm, harmony, texture, timbre, section/form, groove, and production naturally;
- willing to say `I hear this as...` or present alternatives when interpretation is uncertain;
- able to descend into exact technical evidence immediately when asked.

This is not reviewer cosplay and not forced casualness. It is simply the natural-language surface over the same provenance-preserving musical understanding.

## Evidence rules

1. Natural wording must not strengthen the evidence status of the underlying claim.
2. Metaphor may summarize a support bundle but must remain traceable to that bundle.
3. Evaluative language is not objective source truth.
4. Creator intent requires documentary evidence.
5. Listener-response wording must preserve the relevant listener/model context when it matters.
6. Culture-, genre-, era-, and community-specific vocabulary should not be treated as universal.
7. Technical explanation remains available beneath every grounded natural description.
8. Competing natural descriptions may coexist when the evidence supports multiple plausible interpretations.

## Research observatories

Useful sources include:

- natural-language music caption corpora such as the Song Describer Dataset;
- corpora of professional music reviews and criticism;
- composer, performer, sound-programmer, producer, mixing, and mastering interviews;
- recording-studies literature on studio discourse;
- cognitive-linguistic work on musical metaphor;
- timbre semantics and perceptual descriptor research;
- practitioner source code/comments where creator language and executable behavior can be compared directly.

These are observatories for vocabulary, discourse acts, and evidence relationships, not phrase dictionaries to copy mechanically.

## Validation target

A song-level analysis should be tested in at least two directions:

```text
HUMAN-FACING
Does the description sound like something a musically knowledgeable person would naturally say about what is heard?

EVIDENCE-FACING
Can each material claim descend into the strongest available musical, acoustic, historical, or executable evidence?
```

A result fails if it is technically correct but linguistically alien, or natural-sounding but unsupported.

The durable project rule is:

> **Speak like people speak about music; know exactly why you are saying it.**
