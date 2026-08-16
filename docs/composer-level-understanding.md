# Composer-level understanding

## North star

Game Music Interpreter is ultimately trying to understand a game song deeply enough to reason about it the way a strong composer can reason about another composer's work.

The strongest practical benchmark for that depth is **blind composer attribution among historically plausible candidates**.

If a game credits several composers but does not identify the composer of every cue, the system should eventually be able to study the known works, recover each composer's recurring musical grammar, analyze an unknown cue, and say which composer is best supported **because of how the music is constructed**.

That does not make attribution the definition of understanding. It makes attribution an unusually demanding exam for understanding.

```text
native source truth
        +
symbolic note / sequence evidence
        +
execution and synthesis evidence
        +
heard musical organization
        ↓
persistent parts
        ↓
melody • bass • harmony • rhythm
        ↓
phrase • motif • cadence • counterpoint • form
        ↓
recurring compositional rules and transformations
        ↓
composer grammar
        ↓
blind attribution among plausible credited composers
        ↓
explanation + uncertainty + counterevidence
```

A correct composer label without a musically defensible explanation is not enough. It may indicate shortcut learning, metadata leakage, patch-library recognition, driver recognition, or memorization of a related cue.

The intended causal direction is:

```text
understand the song
→ understand the composer's grammar
→ attribution becomes possible
```

not:

```text
predict a composer label
→ call whatever predicted it "understanding"
```

## What composer-level understanding means

The system should increasingly understand relations such as:

- which melodic material is structural and which is ornament or figuration;
- how a melody develops through repetition, mutation, sequence, fragmentation, extension, or reharmonization;
- how the bass changes harmonic meaning, phrase direction, and tension;
- how inner voices and countermelodies behave independently of foreground melody;
- which rhythmic cells organize the groove and how they are varied;
- where phrases begin, extend, answer one another, evade closure, or return;
- how cadences are prepared, delayed, weakened, redirected, or intensified;
- how formal sections relate rather than merely where they start and stop;
- how a composer preserves identity while changing register, harmony, rhythm, texture, or orchestration;
- which choices recur across unrelated works strongly enough to form a compositional grammar.

The goal is not a bag of features. It is a model of **how musical decisions constrain and transform one another over time**.

For example, the system should eventually be able to distinguish:

```text
uses syncopation
```

from:

```text
repeatedly delays the same phrase-internal melodic arrival across different harmonic contexts,
then releases the displacement at the sectional return
```

The second is much closer to a compositional rule.

## Composer grammar

A composer grammar is a probabilistic, evidence-bearing model of recurring musical decisions across securely attributed, independent works.

It may contain several interacting subgrammars.

### Melodic grammar

- interval-transition tendencies;
- contour archetypes;
- leap and recovery behavior;
- scale-degree behavior relative to harmony;
- sequence construction;
- phrase-peak placement;
- repetition versus mutation of cells;
- approach and departure behavior around structural tones.

### Bass and harmonic grammar

- root and non-root bass motion;
- pedal behavior;
- chromatic approaches;
- harmonic rhythm;
- chord-function transitions;
- modal mixture and altered-degree behavior;
- bass counterpoint against repeated upper material;
- relation between harmonic change and phrase structure.

### Rhythmic grammar

- onset placement relative to meter;
- syncopation strategy;
- characteristic rhythmic cells;
- anticipation and delay;
- note-duration relationships;
- density curves;
- rhythmic dialogue between parts.

### Phrase and formal grammar

- phrase-length distributions;
- antecedent/consequent behavior;
- asymmetry and extension;
- cadence placement;
- transition construction;
- loop-boundary behavior;
- return versus development strategy;
- section proportion and preparation.

### Motif-development grammar

- literal repetition;
- transposition;
- rhythmic transformation;
- truncation and extension;
- fragmentation;
- sequencing;
- reharmonization;
- bass substitution under retained upper material;
- recurrence at different formal levels.

### Counterpoint and voice-leading grammar

- motion relationships between parts;
- imitation;
- inner-voice activity;
- dissonance preparation and resolution;
- common-tone treatment;
- voice crossing;
- structural doubling versus independent motion.

### Tension and release grammar

- cadential shapes;
- delayed resolution;
- dominant or pedal prolongation;
- phrase-end register behavior;
- density accumulation and withdrawal;
- suspension and appoggiatura behavior;
- how returns resolve, redirect, or withhold tension.

These subgrammars should remain interpretable enough that an attribution can eventually be explained in musical language.

## Symbolic music is broader than MIDI

In this project, symbolic note/sequence evidence means any representation that exposes authored or executable musical events above raw device state. MIDI is one member of that family, not its definition and not its canonical form.

Potential symbolic sources include:

- MIDI notes, tracks, programs, controllers, bends, tempo and meter;
- MML and other text music languages;
- tracker patterns, rows, effects and instrument commands;
- SMPS, GEMS, N-SPC and other native driver sequence languages;
- Nintendo DS SSEQ and related sequence structures;
- decoded sequence bytecode or hex whose command semantics are validated;
- score-like source or notation;
- source code or data tables that prove note, duration, instrument, loop or logical-track structure;
- reconstructed note/performance events inferred from execution when no authored sequence survives;
- external transcriptions used as explicitly external evidence.

The phrase `recover the MIDI` is therefore too narrow.

The real task is:

> recover as much of the latent musical program and score-like structure as the evidence supports, then use it to understand the composition.

## Cooperative representations law

Every representation is a sensor looking at a different aspect of the same musical object.

```text
MIDI / notation / MML / tracker / native sequence
        ↕
logical tracks • notes • rests • durations • control flow
        ↕
driver execution
        ↕
patches • samples • synthesis state • physical voice episodes
        ↕
VGM / SPC / executable-rip evidence
        ↕
rendered audio / auditory organization
        ↓
shared musical interpretation
        ↓
composer grammar
```

Information should flow in both directions where evidence permits.

A symbolic sequence can teach the system what known logical parts, notes, articulations and transformations look like after driver allocation and chip execution. Low-level execution can teach the system where a symbolic representation is incomplete, quantized, mistranscribed, or blind to synthesis and allocation behavior.

The system should exploit these correspondences for cross-supervision, calibration, testing and inference.

## Preserve uniqueness while sharing knowledge

Cross-representation learning must not flatten source families into a lowest-common-denominator pseudo-MIDI.

These are not interchangeable:

```text
MIDI track
!= MML voice
!= tracker channel
!= driver logical track
!= physical chip channel
!= physical voice episode
!= persistent musical part
```

Likewise:

```text
MIDI program
!= FM patch
!= BRR sample
!= tracker instrument
!= driver instrument object
```

and:

```text
MIDI note
!= sequence note command
!= device pitch state
!= sample playback rate
!= performed pitch
!= heard pitch
!= notation spelling
```

A correspondence can be strong without becoming an equivalence.

> Everything may teach everything else, but nothing is allowed to erase what makes a representation uniquely informative.

## Composition and realization must stay separate

Game music often distributes creative work across several people.

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver / engine programming
!= patch / sample design
!= final rendering
```

This is essential for composer attribution.

A YM2612 patch family, modulation idiom, PSG deployment pattern, BRR bank habit, driver macro, or channel-allocation strategy may identify an arranger or programmer while saying little about who wrote the underlying composition.

Composer-facing evidence should prioritize musical relations that can survive changes in realization:

```text
melody
bass / harmonic motion
rhythm
phrase
motif transformation
counterpoint / voice leading
cadential behavior
form
```

Realization-facing evidence remains valuable, but it must travel on a different attribution axis.

A strong result can legitimately be:

```text
composition grammar resembles composer A
arrangement / programming grammar resembles programmer B
shared patch bank is team-level evidence
```

## Cross-platform survival is a powerful test

The strongest composer signals should survive at least some changes in:

- platform;
- synthesis architecture;
- patch/sample vocabulary;
- arranger/programmer;
- tempo;
- transposition;
- port or remake realization.

This is why same-composer controls across VGM, SPC, MIDI, MML, tracker, PSF-family and other source types are valuable.

If a phrase-construction or motif-transformation habit follows the composer from Genesis FM to SNES sample playback while the sound-programming fingerprints disappear, that is unusually useful evidence that the habit belongs to the composition layer.

## Attribution is the capstone benchmark

For a game credited to several composers, a serious benchmark should proceed as follows.

### 1. Freeze the candidate set

Use historically plausible credited composers and preserve role evidence separately.

### 2. Keep disputed cues completely held out

Do not tune features or weights on the unknown cue.

### 3. Group related works together

Prototypes, final versions, ports, arrangements, reprises, act variants, jingles derived from themes, and other members of one work family must not leak across train and test.

### 4. Build composer grammars from independent known works

One famous lick does not define a composer. Recurrent relations across unrelated work families do.

### 5. Use matched decoys

Prefer controls that defeat shortcuts:

```text
same driver + different composer
same patch bank + different composer
same arranger + different composer
same composer + different arranger
same composer + different platform
similar cue function + different composer
```

### 6. Intervene on confounders

Rerun attribution after masking or normalizing suspected shortcuts:

```text
without timbre
without patch/sample identity
without platform features
without arranger/programmer features
transposition-normalized
tempo-normalized
without same-work relatives
```

A composer hypothesis that collapses only when patch identity is removed was probably using patch identity.

### 7. Require an explanation bundle

A result should look more like:

```text
candidate: composer B

melodic grammar
  strong support
  recurring ascending-cell mutation followed by downward recovery

bass / harmony
  medium support
  non-root bass movement under repeated upper phrase resembles independent controls

phrase / form
  strong support
  characteristic phrase extension before loop return

motif development
  strong support
  fragment → sequence → reharmonized return pattern

counterevidence
  cadence behavior is atypical

confound checks
  result survives patch masking, transposition normalization and platform holdout

realization axis
  arranger/programmer evidence instead resembles candidate C
```

The prose should be generated from the evidence, not invented after the name is chosen.

### 8. Allow abstention

Even when a game has a finite credited list, the system should retain states such as:

```text
composer A probable
composer B plausible
A/B collaborative or shared-source possibility
none of the known profiles fit strongly
insufficient recovered musical structure
```

## Composer questions are the intermediate evaluation surface

Before attribution is credible, the system should answer questions a composer would ask of the piece:

- What is the governing musical idea?
- Which material is structural and which is figuration?
- What does the bass do to the harmony and phrase direction?
- Which voices are independent, doubled, decorative, or contrapuntal?
- Where does tension come from, and what actually releases it?
- How is repetition made non-redundant?
- What changes when a section returns?
- Which transformations preserve identity and which create a new idea?
- How does register participate in form?
- Which rhythmic cells organize the groove?
- Which habits recur across independent works strongly enough to constitute a composer grammar?
- Which expected habits are conspicuously absent?

A system that cannot answer these questions has not earned a composer attribution simply because a classifier emits one.

## Counterfactual understanding

Composer-level understanding should support bounded counterfactual reasoning.

If the system claims to understand why a passage works, it should be able to predict some consequences of changing it:

```text
change bass motion
→ harmonic function / voice-leading pressure may change

remove countermelody
→ phrase dialogue or texture may collapse

preserve notes but change register/orchestration
→ formal weight may change while composition identity partly survives

replace a transformed motif with a literal repeat
→ developmental relationship may weaken
```

These are analysis hypotheses, not claims about undocumented creator intent.

## Evidence discipline

Keep separate:

```text
OBSERVED / EXACT
source bytes, commands, runtime state, documented sequence semantics

DERIVED
validated transformations of exact evidence

INFERRED
parts, note identities, harmony, phrase, motif, role, form, composer grammar

EXTERNAL
transcriptions, interviews, scores, credits, scholarship
```

Confidence should rise when independent representations and independent works converge and fall when they disagree.

Disagreement is often the most useful result.

## Evaluation ladder

```text
L0  source parsing
L1  note / sequence / performance recovery
L2  persistent musical parts
L3  melody / bass / rhythm / harmony
L4  phrase / motif / counterpoint / cadence / form
L5  interpretable compositional rules
L6  composer grammar from independent known works
L7  blind attribution among matched plausible composers
L8  attribution survives confound interventions and platform changes
L9  explanation is musically persuasive and traceable to evidence
```

A high L7 score without L3-L6 understanding is suspicious, not impressive.

## Completion criterion

The project is not finished with a source family when it can merely parse, replay, or export it.

For a mature source family, the desired path is:

```text
native truth
→ sequence / performance semantics where recoverable
→ persistent musical identity
→ pitch / rhythm / articulation / instrumentation
→ melody / bass / harmony / counterpoint
→ phrase / cadence / motif / form
→ reusable compositional rules
→ composer grammar
→ blind composer-attribution stress tests
→ natural composer-level explanation
```

Different formats will expose different rungs directly. The common system should use those differences as leverage.

> **The destination is not MIDI, note extraction, or a composer label. The destination is understanding the composition deeply enough that a defensible composer attribution can emerge from the musical grammar itself.**
