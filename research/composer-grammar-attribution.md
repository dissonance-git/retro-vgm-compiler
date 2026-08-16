# Composer-grammar attribution

Status: active research and benchmark design  
Purpose: make composer attribution a downstream test of genuine song understanding, especially for games whose soundtrack credits name several composers without track-level assignments.

Related:

- `docs/composer-level-understanding.md`
- `research/musicological-authorship-attribution.md`
- `research/sonic3-composer-programmer-attribution.md`
- `research/harmonic-formal-analysis.md`
- `research/perceptual-grouping-hierarchy.md`
- `research/human-musical-discourse.md`

## Target

The important target is not:

```text
input cue
→ opaque style classifier
→ composer label
```

It is:

```text
native / symbolic / execution evidence
        ↓
persistent parts and note-level organization
        ↓
melody • bass • harmony • rhythm • counterpoint
        ↓
phrase • motif • cadence • form • transformation
        ↓
composer-level explanation of how the cue is built
        ↓
comparison with the established compositional grammars
of the historically plausible credited composers
        ↓
role-scoped attribution hypothesis + explanation + uncertainty
```

Composer attribution is therefore a **capstone benchmark for musical understanding**.

A correct name without a musically defensible explanation is not sufficient evidence that the system understood the song.

## Why multi-composer game soundtracks are unusually strong tests

A game that credits several composers but does not assign every cue gives us a constrained historical candidate set without giving away the answer.

That is close to the closed-set musicological attribution problem:

```text
known soundtrack
known development context
known finite contributor set
partially known track assignments
unknown assignments for held-out cues
```

But game music makes the problem harder because several creative layers can belong to different people:

```text
composer
arranger / sequence programmer
sound programmer
patch / sample designer
driver / toolchain author
```

The system must therefore be able to reach a result such as:

```text
composition grammar → strongly resembles composer A
realization grammar → strongly resembles programmer B
patch vocabulary → shared team/library evidence
historical metadata → compatible with A/B team context
```

without averaging those observations into a single anonymous `style` score.

## Composer grammar rather than surface fingerprint

The desired object is a **composer grammar**: a probabilistic, evidence-bearing model of recurring decisions and relationships in securely attributed works.

It should describe not merely what elements occur, but how the composer tends to use them.

### Melodic grammar

Potential coordinates:

- interval-transition tendencies;
- contour archetypes and contour transformations;
- characteristic leaps and recovery behavior;
- scale-degree behavior relative to harmony;
- approach and departure patterns around structural tones;
- sequence construction;
- repetition versus mutation of melodic cells;
- phrase peak placement;
- register-relative contour rather than absolute register alone.

### Bass and harmonic-motion grammar

Potential coordinates:

- root motion and non-root bass behavior;
- pedal use;
- chromatic approach patterns;
- bass counterpoint against melody;
- harmonic rhythm;
- prolongation versus rapid functional movement;
- characteristic turnarounds;
- modal mixture / altered-degree behavior;
- chord-function transitions;
- relation between bass motion and phrase boundaries.

### Rhythmic grammar

Potential coordinates:

- onset distributions relative to meter;
- syncopation strategy;
- recurring rhythmic cells;
- phrase-level density curves;
- anticipation/delay habits;
- note-duration relationships;
- groove-preserving variations;
- rhythmic call/response between parts.

### Phrase and formal grammar

Potential coordinates:

- phrase-length distributions;
- antecedent/consequent relations;
- asymmetry and extension habits;
- cadence timing;
- introduction and pickup behavior;
- transition construction;
- loop-boundary management;
- return versus development strategy;
- section proportion;
- how new material is prepared and how old material re-enters.

### Motif-development grammar

Potential coordinates:

- literal repetition rate;
- interval-preserving transposition;
- rhythmic transformation;
- truncation / extension;
- fragmentation;
- sequencing;
- reharmonization;
- bass substitution under retained upper material;
- call/response transformation;
- recurrence at different formal levels.

### Counterpoint and voice-leading grammar

Potential coordinates:

- independence of concurrent parts;
- parallel versus contrary motion;
- imitation;
- inner-voice activity;
- dissonance preparation/resolution behavior;
- voice crossings;
- doubling behavior when compositionally structural rather than merely orchestrational;
- treatment of sustained/common tones across harmonic change.

### Cadence, tension and release grammar

Potential coordinates:

- preferred cadential shapes;
- degree of closure at loop boundaries;
- delayed resolutions;
- dominant or pedal prolongations;
- phrase-end register behavior;
- density withdrawal or accumulation around cadences;
- melodic suspension / appoggiatura behavior;
- whether formal return resolves, redirects, or intentionally withholds tension.

These dimensions are deliberately relational. `uses minor seventh chords` is weaker than `tends to place a particular harmonic move under a repeated melodic contour at phrase extension points`.

## Literature pass

### Simonetta 2025 systematic survey

Federico Simonetta, **Style-Based Composer Identification and Attribution of Symbolic Music Scores: A Systematic Survey**, TISMIR 8(1), 2025, DOI `10.5334/tismir.240`.

The survey covers 58 peer-reviewed studies and makes several points directly relevant here:

1. composer-classification accuracy is not automatically reliable authorship attribution;
2. candidate corpora should be as comparable as possible to the questioned work;
3. encoding, editorial, performance and representation choices can become confounders;
4. fine-grained attribution must distinguish individual compositional style from broader period/genre/school cues;
5. balanced accuracy / MCC and rigorous cross-validation are preferable to a single lucky accuracy result;
6. disputed work must remain fully held out from model construction;
7. multiple meticulous studies and musicological convergence are stronger than one high model score.

For game music, translate `period/editorial/performance` confounds into:

```text
driver family
shared patch bank
platform / chip
arranger / programmer
sound library
port / remake
prototype / final lineage
same-work or derivative-cue leakage
```

### Rodríguez-Algarra, Sturm and Dixon 2019

**Characterising Confounding Effects in Music Classification Experiments through Interventions**, TISMIR 2(1), 2019, DOI `10.5334/tismir.24`.

The important idea is experimental intervention: deliberately regulate or remove a suspected confounder and observe how the result changes.

For composer attribution, run paired conditions such as:

```text
full evidence
vs patch identity masked
vs instrumentation normalized
vs transposition normalized
vs tempo normalized
vs arranger/programmer features excluded
vs same-work/version relatives removed
vs platform-specific features excluded
```

If attribution collapses only when patch identity is removed, the model may have learned the sound programmer or shared library rather than the composer.

### Kempfert and Wong

**Where Does Haydn End and Mozart Begin? Composer Classification of String Quartets**.

This work is useful because it uses interpretable, musicologically motivated global features, including form-related features, to distinguish historically close composers rather than radically different style periods.

The transferable lesson is to build features from actual musicological hypotheses about how composers organize pieces, not merely whichever statistics are easiest to extract.

### Yu, Varshney, Garnett and Kumar

**Learning Interpretable Musical Compositional Rules and Traces**.

MUS-ROVER asks whether a machine can behave like a music theorist and learns interpretable compositional rules from symbolic music using pattern models.

This supports the present project's move from flat fingerprints toward learned rules and traces such as:

```text
when melodic condition X appears
composer often answers with transformation Y
under harmonic / metric context Z
```

A rule can become an explanation rather than just a weight in a classifier.

### Foscarin et al.

**Concept-Based Techniques for "Musicologist-friendly" Explanations in a Deep Music Classifier**.

This work is useful as an evaluation precedent: explanations should rise from low-level model features to musical concepts that a human analyst can inspect.

For Game Music Interpreter, an attribution claim should therefore expose concepts such as `phrase extension`, `bass counterpoint`, `motivic fragmentation`, `cadential delay`, or `metric displacement`, not only latent-vector dimensions.

## GitHub pass

### DDMAL/jSymbolic2

jSymbolic provides a broad, modular symbolic feature framework for MIR and computational musicology. Its architecture treats extracted musical properties as named feature definitions with dependencies rather than an anonymous vector.

Use it as:

```text
feature taxonomy observatory
+ encoding-bias warning
+ independent comparison extractor
```

Do not copy its feature set wholesale. Game Music Interpreter has source-native evidence and can preserve relationships that generic MIDI/MEI extractors cannot.

### DIDONEproject/musif

musif is a musicological symbolic feature library designed to be extended with custom features. The associated research reports improved classification when complementary feature families are combined.

This supports a modular grammar where melody, harmony, rhythm, form, counterpoint and other families can be evaluated independently before evidence is combined.

### DIDONEproject/music_symbolic_features

This repository contains reproducible benchmarks comparing symbolic feature extractors on composer/style corpora.

Use it as an external methodology control for:

- feature-family ablation;
- extractor disagreement;
- balanced evaluation;
- reproduction of known symbolic-composer benchmarks.

The goal is not to make Game Music Interpreter dependent on these tools. They are independent witnesses against which our native representation can be pressure-tested.

## Attribution protocol for a multi-composer game

Suppose a game credits composers A, B, C and D, with some track-level assignments known and some unknown.

### 1. Freeze historical candidate set

Create:

```text
candidate composer
role actually documented
securely attributed works
uncertain works
excluded works
source/provenance
```

Do not let disputed cues enter any candidate profile.

### 2. Group by work family before splitting

Reprises, act variants, jingles derived from themes, prototypes, ports, remixes and alternate arrangements of one musical work belong to the same split group.

```text
same work family
→ never appear on both sides of train/test
```

Otherwise the model can identify a song rather than a composer.

### 3. Build separate composition and realization views

For each securely attributed cue, extract at least:

```text
COMPOSITION VIEW
melody
bass / harmonic motion
rhythm
phrase / form
motif transformation
counterpoint / voice leading
cadential behavior

REALIZATION VIEW
patch/sample vocabulary
instrument assignments
channel allocation
modulation macros
articulation implementation
panning / hardware controls
driver/toolchain idioms
```

Only the composition view may directly support composer-style attribution.

The realization view may explain arrangement/programming authorship and may be used as a confound-control variable.

### 4. Learn candidate grammars from multiple works

A proposed characteristic should not become a composer habit because it occurs in one famous track.

Require recurrence across independent work families.

Prefer rules such as:

```text
motif X transformation strategy occurs across several unrelated works
```

over:

```text
track X contains this exact lick
```

### 5. Test cross-realization survival

The strongest composer features should survive some changes in:

- instrumentation;
- FM/PCM/sample vocabulary;
- platform;
- arranger/programmer;
- tempo;
- transposition;
- port/remake realization.

This is where additional SPC/VGM sets by the same composer become especially valuable.

If a feature follows the composer from Genesis VGM to SNES SPC while patch and driver features necessarily change, that is unusually useful evidence that the feature belongs to the composition layer.

### 6. Matched-decoy testing

For every held-out unknown cue, choose controls designed to defeat cheap shortcuts.

Examples:

```text
same driver + different composer
same patch bank + different composer
same arranger + different composer
same composer + different arranger
same composer + different platform
similar genre/function + different composer
```

The system should still recover the right composer-level similarity when trivial realization cues are tied or removed.

### 7. Confound interventions

Rerun the attribution under masked conditions.

A robust composition attribution should remain reasonably stable when non-compositional coordinates are removed.

Record sensitivity per candidate:

```text
full score
composition-only score
without timbre
without patch/sample identity
without platform features
without tempo
transposition-normalized
without version relatives
```

The deltas are evidence about what the system was actually using.

### 8. Require an explanation bundle

A candidate score should be accompanied by a structured explanation such as:

```text
candidate: composer B

melodic grammar
  strong support
  recurring ascending-cell mutation followed by downward recovery

bass / harmony
  medium support
  characteristic non-root bass motion under repeated upper phrase

phrase / form
  strong support
  recurring 4+4 phrase extended by two bars before loop return

motif development
  strong support
  fragment → sequence → reharmonized return pattern seen in held-out controls

counterevidence
  cadence behavior is atypical for composer B

confound checks
  result survives patch masking, transposition normalization and platform holdout

role boundary
  realization fingerprint instead resembles programmer C
```

The natural-language explanation should be derivable from these evidence objects rather than invented after the label is chosen.

### 9. Abstain when grammar evidence does not converge

Possible outputs include:

```text
composer A probable
composer B plausible
A/B collaborative or shared-source possibility
none of known profiles fit strongly
insufficient recovered musical structure
```

Even when the game's credited list makes a finite set historically likely, the implementation should retain an abstention path.

## Strongest future control: same composer across different machines

The planned extra SPC and VGM sets by the same artists are extremely important.

They let us search for invariants at several depths:

```text
surface timbre
likely changes heavily

arrangement / programming habits
may persist partly, especially with same programmer

melodic / rhythmic grammar
should persist more strongly if genuinely composer-specific

phrase / motif / harmonic strategy
should be among the most portable composer signals
```

The ideal discovery is not `composer X uses patch fingerprint 0x1234`.

It is closer to:

> Across unrelated games and different hardware, composer X repeatedly constructs foreground phrases with a particular contour/rhythm grammar, moves the bass against repeated upper material in a characteristic way, and transforms motifs at section returns using the same relational strategy.

That is a claim about musical thought rather than a file format.

## Evaluation ladder

```text
L0  source parsing
L1  note / sequence / performance recovery
L2  persistent parts
L3  melody / bass / rhythm / harmony
L4  phrase / motif / counterpoint / form
L5  reusable compositional rules
L6  composer grammar from independent known works
L7  blind attribution among matched credited composers
L8  attribution survives confound interventions and platform changes
L9  explanation is musically persuasive and traceable to evidence
```

A high L7 score without L3-L6 understanding is suspicious, not impressive.

## Current implementation direction

The role-scoped attribution model in `model/creative_attribution_hypothesis.h` is the outer firewall: realization evidence cannot silently become composer evidence.

The next inner layer should represent **composer-grammar evidence** explicitly, preserving:

- musical dimension;
- independent work-family support;
- cross-realization / cross-platform survival;
- supporting and contradicting observations;
- representation provenance;
- confound interventions;
- confidence ceilings when evidence is narrow.

Its output can then feed the `composition_structure` coordinate of the role-scoped attribution model.

This keeps the architecture causal:

```text
understand song
→ discover recurring composer grammar
→ validate grammar across independent controls
→ attribute held-out cue
```

rather than reversing the arrow and teaching the system to call whatever predicts a name `understanding`.
