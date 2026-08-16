# Composer-level understanding

## North star

Game Music Interpreter is trying to understand game music deeply enough to reason about it the way a strong composer can reason about another composer's work.

The strongest practical benchmark for that depth is **blind composer attribution among historically plausible candidates**. But attribution must emerge from understanding music along two independent axes:

```text
AXIS A: same musical work across representations
MIDI / MML / tracker / sequence / VGM / SPC / PSF / audio / transcription

AXIS B: same composer across different scores
other games / soundtracks / platforms / teams / years / arrangers
```

The first axis teaches the system what one musical object looks like from several technical and perceptual viewpoints.

The second axis teaches the system what remains characteristic of a composer when the surrounding project changes.

The strongest composer evidence lies where these axes intersect.

```text
many representations of many works
        ↓
shared musical understanding
        ↓
recurring creator-specific relationships
        ↓
cross-score composer grammar
        ↓
blind attribution of a held-out cue
        ↓
explanation + counterevidence + uncertainty
```

A correct composer label without a musically defensible explanation is not enough. It may indicate metadata leakage, work-family leakage, patch-bank recognition, driver recognition, arranger recognition, platform recognition, or memorization.

The intended causal direction is:

```text
understand songs deeply
→ compare independent works across scores
→ discover recurring composer grammar
→ attribution becomes possible
```

not:

```text
predict a composer label
→ call whatever predicted it "understanding"
```

## No privileged representation

There is no master representation.

MIDI, MML, tracker data, native sequence bytecode, SMPS, GEMS, SSEQ, VGM, SPC, PSF-family execution, chip state, BRR samples, FM patches, rendered audio, and external transcription all reveal different slices of the same musical object.

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
track / soundtrack relationships
```

No layer is automatically more musical than another. A symbolic sequence may reveal exact pitch and phrase structure. VGM may expose articulation or modulation omitted by the sequence. SPC may expose sample and envelope behavior that changes musical role. Audio may reveal grouping, emphasis, texture, and timbral relationships that are not obvious from any single source representation.

The shared model should learn from all of them while preserving where each fact came from.

## Cross-representation understanding

For one work, the system should align representations without flattening them.

```text
MIDI track
!= driver logical track
!= chip channel
!= physical voice episode
!= persistent musical part
!= auditory stream
```

Likewise:

```text
MIDI program
!= FM patch
!= BRR sample
!= tracker instrument
```

and:

```text
MIDI note
!= sequence command
!= device pitch state
!= performed pitch
!= heard pitch
!= notation spelling
```

A correspondence can be strong without becoming an equivalence.

The goal is to recover a shared musical object whose evidence may be distributed across several representations.

## Cross-score understanding

Composer understanding must generalize beyond one soundtrack.

A composer profile built only from one game can accidentally learn:

- that game's driver;
- its shared patch or sample bank;
- its arranger;
- its platform constraints;
- its genre or narrative function;
- a single production period;
- one collaborator's habits;
- one family of related cues.

So composer grammar should be learned from **independent work families across different scores whenever possible**.

Useful control structure:

```text
composer A
├── score 1
│   ├── work family 1
│   └── work family 2
├── score 2
│   ├── work family 3
│   └── work family 4
└── score 3
    └── work family 5
```

Then test whether candidate rules survive:

```text
leave one work family out
leave one soundtrack out
leave one platform out
leave one arranger out
leave one period of the composer's career out
```

A rule that only exists inside one score is a **score-local grammar** until further evidence shows otherwise.

A rule that recurs across several unrelated scores becomes stronger composer evidence.

## Composer evolution is allowed

A composer is not a frozen fingerprint.

Style can change with:

- age and career period;
- hardware and available tools;
- collaborators;
- genre and dramatic function;
- project direction;
- musical influences;
- production constraints;
- deliberate stylistic experimentation.

The model should therefore distinguish:

```text
stable long-range habits
score-local habits
period-specific habits
collaborator-dependent habits
one-off experiments
```

The goal is not to force every work by one composer into one centroid. It is to learn a structured region of musical behavior and the trajectories within it.

## What deep song understanding means

The system should increasingly understand relations such as:

- which melodic material is structural and which is figuration;
- how melody changes through repetition, mutation, sequence, fragmentation, extension, reharmonization, articulation, or timbral reassignment;
- how bass motion changes harmony, phrase direction, groove, and tension;
- how inner voices and countermelodies behave independently of foreground melody;
- how rhythmic cells organize the groove and migrate between parts;
- how timbre, register, articulation, synthesis and dynamics change the function of the same pitch material;
- where phrases begin, extend, answer, evade closure, or return;
- how cadences are prepared, delayed, weakened, redirected, or intensified;
- how formal sections relate rather than merely where they start and stop;
- how a musical idea survives different arrangements, channels, patches, samples, platforms, or versions;
- which decisions recur across independent works and scores strongly enough to form a composer grammar.

The goal is not a bag of features. It is a model of **how musical decisions across several representational layers constrain and transform one another over time**.

## Cross-representation composer grammar

A composer grammar is a probabilistic, evidence-bearing model of recurring musical behavior across securely attributed independent works.

It should not be restricted to score-like features. It can include any recurring behavior that is plausibly attributable to the composer in the historical context.

### Structural / symbolic grammar

- melodic interval and contour behavior;
- bass and harmonic motion;
- rhythmic cells and metric placement;
- phrase construction;
- motif development;
- counterpoint and voice leading;
- cadence and form;
- loop and recurrence strategy.

### Arrangement / orchestration grammar

- register assignment;
- doubling and voice distribution;
- foreground/background hierarchy;
- density changes;
- instrument-role migration;
- countermelody deployment;
- textural contrast;
- arrangement of repeated material.

### Timbre / synthesis grammar

- recurring patch or sample families where historically attributable;
- envelope and articulation preferences;
- modulation behavior;
- timbral contrast strategy;
- how synthesis changes across form;
- relationships between timbre and musical role.

### Performance / realization grammar

- attack/release behavior;
- expressive pitch control;
- groove microtiming where recoverable;
- dynamic contour;
- articulation patterns;
- use of silence and negative space;
- interaction between implementation and phrase shape.

### Soundtrack / score-level grammar

- thematic reuse;
- cue-family relationships;
- transformation across locations, acts, characters, states, or versions;
- recurring harmonic/timbral pairings;
- how different tracks solve similar formal or dramatic problems;
- how one score differs from another while retaining deeper creator-specific behavior.

These dimensions interact. A useful creator-specific rule may cross several layers:

```text
retained melodic cell
+ changed bass motion
+ widened register
+ brighter timbral assignment
+ delayed cadence
→ characteristic return strategy
```

That is richer than any one score, patch, or MIDI feature by itself.

## Role scope, not modality censorship

Game music often distributes creative work across several people:

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver / engine programming
!= patch / sample design
!= performance realization
```

This means evidence must be role-scoped, but it does **not** mean entire modalities should be excluded from composer attribution.

A timbral, arrangement, patch, articulation, or control habit can support composer attribution when historical evidence shows the composer authored or reliably controlled that layer.

Conversely, the same feature should not support composer attribution when it is more plausibly inherited from a programmer, shared library, driver, platform, or arranger.

So the rule is:

```text
all representations may contribute
+
every contribution carries role provenance
```

not:

```text
only score-like features count
```

## The strongest generalization test

The most valuable composer signal should survive some combination of:

```text
different song
different soundtrack
different game
different platform
different synthesis architecture
different arranger/programmer
different patch/sample library
different tempo / key / register
different point in the composer's career
```

Not every trait must survive every change. The pattern of what survives and what changes is itself part of the composer's model.

This makes same-composer VGM/SPC/PSF/MIDI/MML sets from other games especially valuable controls.

## Attribution as capstone benchmark

For a game credited to several composers:

1. freeze the historically plausible candidate set;
2. keep disputed cues completely held out;
3. group prototypes, ports, reprises, arrangements and derivative cues by work family;
4. build candidate grammars from independent known works across multiple scores where possible;
5. use matched decoys such as same driver/different composer and same composer/different platform;
6. run leave-one-score-out and leave-one-platform-out tests;
7. intervene on suspected shortcuts such as patch identity, timbre, instrumentation, tempo and arranger/programmer features;
8. require a structured musical explanation and explicit counterevidence;
9. allow abstention.

A result should look more like:

```text
candidate: composer B

cross-score evidence
  melodic transformation rule recurs in three unrelated games
  phrase-extension behavior survives two platforms
  bass/harmonic strategy survives a different arranger

current cue
  strong melodic match
  strong phrase/form match
  medium timbral/arrangement match
  cadence behavior is atypical

confound checks
  survives patch masking
  survives transposition normalization
  survives leave-one-score-out validation

realization axis
  implementation fingerprint instead resembles programmer C
```

The prose should be generated from evidence, not invented after the name is chosen.

## Composer questions are the intermediate evaluation surface

Before attribution is credible, the system should answer questions a composer would ask:

- What is the governing musical idea?
- Which material is structural and which is figuration?
- What does the bass do to harmony and phrase direction?
- Which voices are independent, doubled, decorative, or contrapuntal?
- Where does tension come from, and what releases it?
- How is repetition made non-redundant?
- What changes when a section returns?
- Which transformations preserve identity and which create a new idea?
- Which habits recur across different works?
- Which habits recur across different scores?
- Which traits disappear when the platform or arranger changes?
- Which traits evolve over the composer's career?

A system that cannot answer these questions has not earned a composer attribution simply because a classifier emits one.

## Evaluation ladder

```text
L0  source parsing
L1  note / sequence / performance recovery
L2  persistent musical parts
L3  melody / bass / rhythm / harmony / timbre / articulation
L4  phrase / motif / counterpoint / cadence / form / arrangement
L5  integrated cross-representation song model
L6  recurring rules across independent works
L7  cross-score composer grammar
L8  blind attribution among matched plausible composers
L9  attribution survives score/platform/representation confound tests
L10 explanation is musically persuasive and traceable to evidence
```

A high L8 score without L3-L7 understanding is suspicious, not impressive.

## Completion criterion

For a mature source family, the desired path is:

```text
native truth
→ representation-specific semantics
→ cross-representation musical identity
→ deep song understanding
→ recurring rules across independent works
→ recurring rules across different scores
→ cross-score composer grammar
→ blind attribution stress tests
→ natural composer-level explanation
```

> **The destination is understanding a composer's musical thinking across works, representations, and scores deeply enough that authorship can emerge as a defensible consequence of the music itself.**
