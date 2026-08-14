# Perceptual grouping hierarchy

## Question

What perceptual objects sit between exact source/device execution and the way listeners actually hear musical texture?

The existing project distinction was already correct:

```text
physical slot
!= voice episode
!= persistent musical part
!= auditory stream
```

A fresh music-cognition pass shows that this still stops too early.

Auditory organization in music is hierarchical. Individual acoustic components can group into auditory events/sources; events can connect over time into auditory streams; several such streams or source images can fuse into a higher **textural stream/layer**; and the complete mixture can itself be heard as one musical stream or whole.

A better evidence ladder is therefore:

```text
physical synthesis state
-> acoustic components/events
-> auditory event/source hypotheses
-> auditory streams through time
-> textural stream/layer hypotheses
-> whole musical-stream/scene organization
-> musical discourse
```

These are perceptual/analytical projections. None overwrites exact source identity.

## Literature anchors from this pass

The SciSpace pass recovered several directly relevant bodies of work:

- Albert Bregman / auditory scene analysis: auditory streams organize sound components over time into perceptual source-like entities.
- Kunio Kashino on music scene analysis: explicitly distinguishes physical input from perceptual/music concepts.
- David Huron, *Hierarchical Streams*: distinguishes source-like auditory images, intermediate textural streams, and the overall musical stream.
- Ben Duane, *Auditory Streaming Cues in Eighteenth- and Early Nineteenth-Century String Quartets*: treats individual parts or groups of parts as possible textural streams and computationally examines grouping cues.
- Ragert et al., *Segregation and integration of auditory streams when listening to multi-part music*: shows that listeners both separate and integrate concurrent parts.
- McAdams and collaborators on orchestral grouping: concurrent, sequential, and segmental grouping cues predict blends, melodic streams, layers, and perceptual boundaries.
- Kokoras on auditory fusion: attack density, timbral similarity, register, dynamics, and related spectral properties can cause multiple streams to fuse into one coherent texture.

The transferable result is not a universal Western orchestration theory. It is the existence of distinct grouping levels and explicit acoustic/perceptual cues.

## Persistent musical part is not the same as perceptual stream

A source/driver may preserve a persistent authored part even when a listener does not hear it as a separately segregated stream.

Example:

```text
part A
part B
part C
```

may remain three exact or strongly supported musical parts while the rendered sound is heard as:

```text
one blended pad/string/textural layer
```

Conversely, one programmed part may generate events that perceptually split into more than one stream under sufficiently large register, timbre, timing, or spatial changes.

Therefore:

```text
persistent part count
!= perceived stream count
```

and:

```text
source identity continuity
!= guaranteed perceptual continuity
```

## Textural stream is the missing intermediate object

The most useful addition from the literature is the **textural stream** or perceptual layer.

A textural stream can contain:

- one authored part;
- several doubled parts;
- several instruments moving together;
- a chordal/pad texture;
- a rhythmic accompaniment layer;
- a fused percussion field;
- other source combinations that function perceptually as one layer.

Thus:

```text
musical part
-> may participate in one or more perceptual grouping hypotheses

textural stream
-> may contain one or more parts/auditory streams
```

This should be represented as a many-to-many analytical relation rather than a new identity equivalence.

## Grouping cues can be tested against exact execution evidence

The project has an unusual advantage over ordinary MIR research: for many game-music sources, it can know the lower execution state exactly or nearly exactly.

This allows controlled experiments such as:

```text
known source parts
+ known synthesis trajectories
+ known onset/pitch/timbre/routing changes
-> render audio
-> auditory grouping model or human annotation
-> compare perceptual grouping against exact source organization
```

Useful grouping cues from the literature include:

- onset/offset synchrony;
- harmonicity;
- pitch proximity;
- common frequency motion / pitch comodulation;
- common amplitude motion;
- timbre/spectral similarity;
- spatial proximity;
- temporal continuity;
- attack density;
- register;
- dynamics;
- repetition/predictability;
- learned/schema-based musical expectations.

No single cue should become the grouping ontology.

## Why this matters for retro game music

Game music routinely creates perceptual layers from implementation resources that do not map cleanly onto musical parts.

Examples include:

- doubled PSG/FM voices creating one thicker lead;
- several SNES sample voices forming one string or choir pad;
- detuned FM carriers/resources perceived as one instrument;
- layered PCM percussion forming one rhythmic texture;
- octave doubling that remains one functional line;
- echo channels or programmed repeats that reinforce one phrase rather than becoming independent parts;
- arpeggiated hardware-limited patterns perceptually implying a chordal layer.

The interpreter should be able to say both:

```text
three execution sources are active
```

and:

```text
they function perceptually as one blended layer
```

without contradiction.

## Human musical discourse becomes easier downstream

This hierarchy helps solve an earlier language problem.

Humans often describe music at the textural-stream level rather than at the physical-channel or exact-part level:

- "the strings open up";
- "the pad gets wider";
- "the percussion drops out";
- "a brighter layer comes in";
- "the bass and kick lock together";
- "the melody doubles at the octave";
- "everything thins out before the return".

These statements may refer to groups of sources/parts, not one exact track.

The system should therefore generate human discourse from the strongest appropriate grouping level while retaining links to all lower evidence.

## Segregation and integration are both active

The literature also blocks a simplistic model where auditory analysis only tries to separate everything.

Listeners simultaneously:

```text
segregate
```

to hear distinct lines/sources, and:

```text
integrate
```

to hear chords, textures, accompaniment fields, and the piece as a coherent whole.

So the goal is not "maximum stem separation."

A useful auditory model must represent both:

- why two things are heard apart;
- why several things are heard together.

This is particularly important for source-native rendering, because enhancement that increases separation too aggressively could destroy intentional or musically useful fusion.

## Rendering consequence

Source-native enhancement should preserve not only exact source identity but also important perceptual grouping relations when possible.

A proposed enhancement can be evaluated at several levels:

```text
source identity preserved?
part identity preserved?
stream segregation preserved or improved?
textural fusion preserved where musically important?
foreground/background hierarchy preserved?
whole-scene coherence preserved?
```

This gives a more musical definition of "same piece, clearer realization" than maximizing technical separation alone.

## Cross-cultural guardrail

Textural/grouping cues are not a license to universalize one orchestration tradition.

Some grouping mechanisms are low-level perceptual cues; others depend on learned schemas, genre conventions, and musical culture.

Every higher grouping interpretation should retain model/cultural scope where relevant.

## Suggested representation

No new canonical graph primitive is required yet.

A bounded analytical representation can use typed hypotheses such as:

```text
auditory_event
  evidence: acoustic trajectory + grouping cues

auditory_stream
  members: auditory events over time
  continuity evidence: pitch/timbre/time/space/common fate

textural_stream
  members: auditory streams and/or persistent parts
  grouping evidence: synchrony/comodulation/timbre/register/dynamics/function

musical_scene
  members: textural streams
  foreground/background/whole relations
```

Each object should include:

- time span;
- members;
- model/analysis identity;
- grouping cues used;
- confidence;
- provenance to source/device/acoustic evidence.

## Highest-information next tests

1. Choose one cue with obvious doubling and test whether two exact parts are better represented as one textural stream.
2. Choose one cue where one source changes register/timbre enough to test perceptual stream splitting.
3. Compare a dense and sparse arrangement of the same motif to see whether stream organization changes while part identity remains stable.
4. Use a source-native enhancement render and verify that increased clarity does not accidentally destroy a deliberate blend.
5. Add human annotations from listener/reviewer/producer language and map each statement to part, auditory-stream, textural-stream, or whole-scene level.

## Stop conditions

Stop rather than guess if:

- a hardware channel is called an auditory stream by default;
- every authored part is assumed to be separately heard;
- every perceived layer is assumed to correspond to one source;
- source separation is treated as perceptual truth;
- a textural blend is destroyed merely because more isolation is technically possible;
- one listener/model's grouping is stored as universal fact;
- human-language descriptions lose their route back to lower evidence.

Correction outranks coherence.
