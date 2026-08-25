# Cross-representation, cross-soundtrack composer attribution

Status: active research and benchmark design  
Purpose: make composer attribution a downstream test of genuine musical understanding, especially for games whose soundtrack credits name several composers without track-level assignments.

Related:

- `docs/composer-level-understanding.md`
- `docs/holistic-musical-understanding.md`
- `research/musicological-authorship-attribution.md`
- `research/sonic3-composer-programmer-attribution.md`

## Target

The target is not an opaque style classifier, and it is not a score-only or MIDI-only fingerprint.

The system should learn a composer by studying **many representations of many independent works across multiple soundtracks** whenever the evidence permits.

```text
REPRESENTATION AXIS
MIDI / MML / tracker / native sequence / VGM / SPC / PSF / audio

WORK AXIS
independent cues and work families

SOUNDTRACK AXIS
different games / soundtracks / projects / platforms / years

ROLE AXIS
composition / arrangement / programming / patch-sample design / realization
```

The composer model should reason across all four axes without collapsing them.

```text
many representations
of many independent works
across several soundtracks
        ↓
deep musical models
        ↓
recurring creator-specific relations
        ↓
cross-soundtrack composer grammar
        ↓
held-out cue attribution
        ↓
explanation + counterevidence + uncertainty
```

## Why different soundtracks matter

A single soundtrack is a dangerous environment for attribution because it contains shared local causes:

- one driver or engine;
- one patch/sample library;
- one platform;
- one sound team;
- one director;
- one production period;
- one genre/function vocabulary;
- recurring themes and derivative cues;
- shared arrangers or programmers.

A model trained only within that soundtrack can appear to identify composers while actually identifying local production conditions.

Therefore the preferred composer profile should contain controls from other soundtracks whenever available.

```text
composer A
├── soundtrack 1
│   ├── independent work family 1
│   └── independent work family 2
├── soundtrack 2
│   ├── independent work family 3
│   └── independent work family 4
└── soundtrack 3
    └── independent work family 5
```

The central question becomes:

> What remains recognizably this composer when the soundtrack around them changes?

## Two independent generalization axes

### Same work across representations

A single cue may be observable as:

```text
MIDI / MML / tracker / native sequence
↕
VGM / SPC / executable state
↕
patches / samples / device behavior
↕
rendered audio
↕
external transcription
```

These views can teach one another what the musical object is while preserving source-native distinctions.

### Same composer across soundtracks

The composer may appear in:

```text
game A on Genesis
game B on SNES
game C on PlayStation
game D using MIDI/streamed audio
```

The useful question is which habits survive those changes, which transform, and which vanish because they belonged to a particular collaborator, toolchain, or platform.

The strongest evidence sits where both axes agree:

```text
same creator-specific relation
observed through several representations
and recurring across unrelated soundtracks
```

## Composer grammar is multi-view

No representation is privileged.

### Structural / symbolic view

- melody;
- bass and harmonic motion;
- rhythm;
- phrase;
- motif transformation;
- counterpoint;
- cadence;
- form;
- loop and recurrence strategy.

### Arrangement / orchestration view

- register;
- density;
- doubling;
- role assignment;
- countermelody strategy;
- textural contrast;
- orchestration of returns and transitions.

### Timbre / synthesis view

- patch/sample choices where creator-controlled;
- envelope and articulation behavior;
- modulation habits;
- timbral contrast;
- synthesis changes tied to form;
- relationships between timbre and musical role.

### Performance / execution view

- expressive pitch behavior;
- attack/release behavior;
- dynamic contour;
- groove microtiming where recoverable;
- articulation patterns;
- use of silence and negative space;
- implementation choices that shape phrase perception.

### Soundtrack-level view

- thematic reuse;
- cue-family transformations;
- recurring solutions to location/character/state functions;
- harmonic/timbral pairings;
- formal strategies repeated across unrelated cues;
- how one soundtrack differs from another while retaining deeper creator-specific behavior.

The grammar should model relationships among these views rather than concatenate separate feature lists.

Example:

```text
retained melodic cell
+ changed bass motion
+ widened register
+ brighter timbral assignment
+ delayed cadence
→ characteristic return strategy
```

If that strategy appears in several unrelated works across several soundtracks, it becomes much stronger evidence than a single patch or chord statistic.

## Role scope, not modality censorship

Game music often distributes creative work across several people:

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver / engine programming
!= patch / sample design
!= final realization
```

All representations may contribute to composer attribution, but each contribution must carry role provenance.

A timbral, arrangement, patch, articulation, or control habit may support composer attribution when historical evidence shows the composer authored or reliably controlled that layer.

The same feature should be downgraded or reassigned when it is more plausibly inherited from a programmer, arranger, shared library, platform, or driver.

So the law is:

```text
all modalities may contribute
+
role provenance determines what the contribution means
```

not:

```text
only note/score features count
```

## Cross-soundtrack composer model

A composer should not be reduced to one centroid.

Style can evolve with:

- career period;
- new collaborators;
- platform and tooling;
- genre and dramatic function;
- project direction;
- musical influences;
- deliberate experimentation.

The model should distinguish:

```text
stable long-range habits
period-specific habits
soundtrack-local habits
collaborator-dependent habits
platform-dependent habits
one-off experiments
```

A composer grammar is therefore a structured region with trajectories, not a frozen fingerprint.

## Validation protocol

For a soundtrack credited to composers A, B, C and D:

### 1. Freeze the historical candidate set

Preserve:

```text
candidate
historically documented role
securely attributed works
uncertain works
excluded works
source/provenance
```

### 2. Group by work family

Prototypes, ports, reprises, remixes, act variants, jingles derived from a theme, and alternate arrangements of one musical work stay in one split group.

```text
same work family
→ never on both train and test sides
```

### 3. Prefer cross-soundtrack controls

For every candidate, gather secure works from other soundtracks where possible.

Desired validation modes:

```text
leave-one-work-family-out
leave-one-soundtrack-out
leave-one-platform-out
leave-one-arranger-out
leave-one-career-period-out
```

### 4. Build multi-view grammars

Do not restrict composer evidence to symbolic notation. Keep separate evidence families for:

```text
structure
arrangement
synthesis/timbre
performance/execution
soundtrack-level relationships
```

Each family carries role provenance and confidence.

### 5. Use matched decoys

Examples:

```text
same driver + different composer
same patch/sample bank + different composer
same arranger + different composer
same composer + different arranger
same composer + different platform
same composer + different soundtrack
similar game function + different composer
```

### 6. Intervene on confounders

Rerun attribution under masks/normalizations such as:

```text
without timbre
without patch/sample identity
without platform-specific features
without arranger/programmer features
without soundtrack-local features
transposition-normalized
tempo-normalized
without related versions
```

The change in result is evidence about what the system was actually using.

### 7. Require an explanation bundle

A result should resemble:

```text
candidate: composer B

cross-soundtrack support
  phrase-extension rule recurs in soundtrack 1 and soundtrack 3
  bass-against-repeated-melody strategy recurs across two platforms
  motif transformation survives a different arranger

current cue
  strong phrase/form match
  strong melodic-development match
  medium arrangement/timbre match
  cadence behavior is atypical

representation convergence
  native sequence and VGM agree on part/motif structure
  audio supports the inferred foreground/background grouping

confound checks
  survives patch masking
  survives leave-one-soundtrack-out validation
  survives transposition normalization

role boundary
  implementation subgrammar instead resembles programmer C
```

The explanation should be generated from evidence, not invented after the winning label is selected.

### 8. Allow abstention

Possible outputs include:

```text
composer A probable
composer B plausible
A/B collaboration or shared-source possibility
none of the known profiles fit strongly
insufficient recovered musical structure
```

## Same-composer controls from other soundtracks

Additional VGM, SPC, PSF-family, tracker, MIDI, MML, or other sets by the same composers are among the most valuable future controls.

They let us ask whether a proposed habit travels with the person or stays behind with the soundtrack.

```text
feature follows composer across soundtracks
→ stronger creator evidence

feature stays with one soundtrack across composers
→ likely soundtrack/toolchain/team evidence

feature follows one arranger across composers
→ realization evidence

feature survives different representations of one work
→ stronger evidence that the underlying musical relation was recovered correctly
```

The ideal discovery is not:

```text
composer X uses patch fingerprint 0x1234
```

It is closer to:

> Across unrelated games, different hardware, and different collaborators, composer X repeatedly constructs phrase peaks, bass countermotion, motif transformations, timbral reinforcement, and formal returns using a recognizably related strategy.

## Literature and GitHub observatories

The current research pass uses:

- Simonetta's 2025 systematic survey of symbolic composer attribution for validation and confound warnings;
- Rodríguez-Algarra, Sturm and Dixon for intervention-based confound testing;
- Kempfert/Wong for interpretable musicologically motivated composer features;
- MUS-ROVER for interpretable compositional rules and traces;
- Foscarin et al. for musicologist-readable explanation concepts;
- `DDMAL/jSymbolic2` as a modular feature-taxonomy and encoding-bias observatory;
- `DIDONEproject/musif` and `music_symbolic_features` as independent symbolic-feature and benchmarking observatories.

These are pressure tests and methodological references, not canonical representations for VGM Compiler.

## Evaluation ladder

```text
L0  source parsing
L1  note / sequence / performance recovery
L2  persistent parts
L3  melody / bass / rhythm / harmony / timbre / articulation
L4  phrase / motif / counterpoint / cadence / form / arrangement
L5  integrated cross-representation song model
L6  recurring rules across independent works
L7  cross-soundtrack composer grammar
L8  blind attribution among matched plausible composers
L9  attribution survives soundtrack/platform/representation confound tests
L10 musically persuasive, traceable explanation
```

A high L8 score without L3-L7 understanding is suspicious, not impressive.

## Current implementation direction

`model/creative_attribution_hypothesis.h` is the outer role firewall.

The next inner layer should represent cross-representation, cross-soundtrack grammar evidence explicitly, including:

- creator candidate;
- role scope;
- representation source;
- soundtrack identity;
- work-family identity;
- musical dimension;
- learned relation/rule;
- supporting independent works;
- supporting independent soundtracks;
- cross-platform survival;
- contradictory observations;
- confound-intervention sensitivity;
- confidence and provenance.

This keeps the architecture causal:

```text
understand representations
→ understand songs
→ compare independent works
→ compare independent soundtracks
→ discover composer grammar
→ validate grammar
→ attribute held-out cue
```

> **The strongest composer model is not a fingerprint of one soundtrack. It is a structured account of what musical behaviors follow the composer across different works, representations, soundtracks, platforms, and collaborators.**
