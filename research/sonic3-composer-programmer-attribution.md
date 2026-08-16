# Sonic 3 composer / arranger / programmer attribution control

Status: active project control  
Primary goal: use Sonic 3 & Knuckles as a hard real-world test of composer-level understanding without collapsing composition, arrangement, programming, source company, driver behavior, version history, or soundtrack-local production into one `artist` label.

Related:

- `research/composer-grammar-attribution.md`
- `research/sonic3-vgm-gun-hazard-spc.md`
- `research/sonic-smps-pitch-recovery.md`
- `research/musicological-authorship-attribution.md`
- `research/retro-composition-programming-listening.md`
- `docs/composer-level-understanding.md`

## Research archive

The project background includes a user-supplied offline archive saved on 2026-08-15 containing:

- all 18 saved pages of **The Sonic 3 Music Ramble**;
- all 29 saved pages of **The "Sonic The Hedgehog 3 & Knuckles" Quest for Music Composer Research**;
- the saved Sonic Retro **Sonic the Hedgehog 3/Development/Music** page.

The two forum threads contain 921 saved posts in total. They are research evidence, not an automatic truth set. Individual posts range from first-hand recollection and cited documentary evidence to explicit speculation, technical inference, correction of earlier claims, and ordinary discussion.

This distinction is essential because the discussion demonstrates that Sonic 3 attribution is a moving evidence problem rather than a static credits table.

## Sonic 3 is the target environment, not the whole training world

The most important control principle is now:

```text
SONIC 3
held-out target environment

OTHER SOUNDTRACKS BY THE SAME PEOPLE
creator-generalization controls
```

A model trained only inside Sonic 3 can accidentally learn:

- the Sonic 3 driver;
- Universal Voice Bank usage;
- one shared FM patch vocabulary;
- one production period;
- one sound team;
- one platform;
- one group of arrangers/programmers;
- one game-specific formal vocabulary;
- related cue families;
- prototype/final lineage.

Therefore a Sonic 3 attribution should become much stronger when the same proposed creator grammar also appears in **different soundtracks by the same person**.

The decisive question is not merely:

> Which Sonic 3 tracks resemble one another?

It is:

> Which musical behaviors follow Maeda, Nagao, Setsumaru, Senoue, or another historically plausible contributor outside Sonic 3, and then reappear in a held-out Sonic 3 cue?

## Two independent axes

### Axis A: one work across representations

A Sonic work may be observable through:

```text
MIDI-like source / transcription
MML-like or sequence source
SMPS data
prototype sequence
final sequence
VGM execution
FM / PSG / DAC state
rendered audio
later port / arrangement
```

These views should teach one another without being collapsed into one canonical representation.

### Axis B: one creator across soundtracks

A composer/programmer may appear across:

```text
Sonic 3 & Knuckles
other Mega Drive games
SNES games / SPC controls
other Sega projects
later platforms / PSF-family controls
MIDI / source-available material
```

A creator rule becomes much more interesting when it survives a change of soundtrack, platform, arranger, or implementation environment.

## Core role separation

The project must represent at least these separately:

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver / engine programming
!= patch / FM-voice design
!= sound-effect work
!= source company / team
!= final rendering
```

The forum archive repeatedly describes cases where the person who supplied or composed musical material was not necessarily the person who converted, embellished, arranged, optimized, or programmed the Mega Drive realization.

Therefore a technical fingerprint recovered from VGM or SMPS may strongly support an **arranger/programmer** hypothesis while providing weak direct support for a **composer** hypothesis.

But the inverse boundary is equally important: arrangement, timbre, or synthesis evidence is not forbidden from composer attribution when independent historical evidence shows that the composer controlled that layer.

The rule is:

```text
all representations may contribute
+
role provenance determines what each contribution means
```

## Symbolic source is not one uniform MIDI layer

The Music Ramble discussion describes the Sonic 3 production sources as heterogeneous and asynchronous. The proposed historical picture includes material arriving from several parties and in different states, with MIDI-like source data in some cases and substantial Mega Drive-side alteration in others.

The durable architectural lesson is:

```text
incoming symbolic material
        ↓
arrangement / editing / embellishment
        ↓
SMPS sequence and control decisions
        ↓
YM2612 / PSG / DAC execution
        ↓
heard musical object
```

A surviving MIDI, prototype sequence, final SMPS sequence, VGM, and later port can be related views of one work without being interchangeable copies.

Differences such as added bass, rewritten PSG, changed intros, voice assignments, or altered channel behavior should be modeled as transformations and creative/technical evidence, not automatically repaired away.

## Evidence classes from the archive

### Documentary / first-hand evidence

The saved material includes or discusses direct composer/arranger statements, interviews, old websites, game/source credits, prototype evidence, and private correspondence.

One uncertainty control comes from Dissident93's 2019 post #26 in the composer-research thread: some older Uwabo/Nagao information was remembered from direct Facebook contact, but the original messages were no longer recoverable and the contributor was not remembered as completely certain about the full extent of the work.

That means:

```text
reported first-hand recollection
!= preserved primary document
!= exact exhaustive track list
```

The project should retain that testimony with uncertainty rather than promote it to exact ground truth.

The same archive also raises reused pre-Sonic-3 material, including the Special Stage / Blue Spheres lineage to SegaSonic Bros. The saved Development/Music page documents a previously created Yoshiaki Kashima song being repurposed for Sonic 3.

Therefore:

```text
appears in Sonic 3
!= necessarily composed specifically for Sonic 3
```

### Version evidence

Prototype, final, Collection/PC, soundtrack, later-port and later-arrangement versions are evidence objects, not nuisance duplicates.

They can reveal:

- which material existed by a particular build;
- whether one arrangement was replaced or rewritten;
- whether the same composition received a different programmer/arranger;
- which note, bass, PSG, voice, modulation, and channel-layout decisions survived;
- whether later versions descend from an earlier symbolic source rather than the final Mega Drive realization.

The interpreter must support:

```text
same work identity
+
different realization lineage
+
different arrangement/programming hypothesis
```

without overwriting either identity.

### Realization fingerprints

Candidate evidence from the forum discussion includes:

- exact FM voice / patch reuse;
- Universal Voice Bank use versus locally included voices;
- modulation-control patterns and recurring `smpsModSet` families;
- song-header initial values;
- PSG participation and recurring PSG techniques;
- channel layout and sharing choices;
- glissando / pitch-control habits;
- `smpsNoAttack` and related articulation idioms;
- panning/control behavior;
- optimization state, calls, loops and repeated-section factoring;
- characteristic additions during conversion/arrangement;
- reuse of the same technical choices in other games associated with the same programmer.

These are not equally observable from VGM. Some require SMPS/source evidence. VGM can expose downstream patch state, register/control trajectories, timing, and physical voice activity.

The shared model must record which layer actually supports each feature.

## A critical correction case: inherited labels can be wrong

The saved 2026 Music Ramble gives a useful adversarial example.

The saved Development/Music page presents Sonic 3's `Continue` and `Stage Clear - Act Passed` as Jun Senoue compositions/arrangements based on Sonic 3D Blast evidence.

Music Ramble post #339 (2026-08-08) explicitly revisits that interpretation and argues that the Sonic 3D Blast credit gap may have been interpreted incorrectly, while pointing to implementation details considered more Nagao-like.

For Game Music Interpreter this is not yet a historical verdict. It is a regression shape:

```text
inherited metadata / old interpretation
+
new source-layout evidence
+
new technical evidence
        ↓
old attribution must remain revisable
```

No confidence system is acceptable if an old label silently dominates contradictory new evidence.

## Composer understanding is multi-view

A composer model should not be restricted to note/score features.

Possible creator-facing evidence includes:

### Structural / symbolic

- melodic interval and contour behavior;
- phrase construction;
- bass-motion strategy;
- harmonic rhythm and progression habits;
- cadential behavior;
- motivic recurrence and transformation;
- rhythmic cells and syncopation;
- counterpoint and inner-voice behavior;
- formal proportions and developmental strategy.

### Arrangement / orchestration

- register assignment;
- doubling;
- density change;
- countermelody deployment;
- treatment of returns;
- texture hierarchy.

### Timbre / synthesis

- FM/PSG/sample choices where creator-controlled;
- envelope and articulation behavior;
- modulation habits;
- timbral contrast tied to musical form.

### Performance / execution

- expressive pitch behavior;
- attack/release patterns;
- dynamic contour;
- timing and negative-space behavior where recoverable.

A composition-structure match with a realization mismatch is meaningful. A realization match with a composition mismatch is also meaningful. Neither should be averaged into one anonymous similarity score.

## Existing within-Sonic-3 controls

The committed 58-file Sonic 3 & Knuckles VGM set already supplies useful **within-soundtrack** controls:

```text
prototype ↔ final
Act 1 ↔ Act 2
Sonic 3 ↔ Sonic & Knuckles replacement cue
same cue family ↔ changed realization
shared patch vocabulary ↔ different musical material
similar musical material ↔ changed patch/control vocabulary
```

These are valuable for understanding what changes inside the target environment, but they are not sufficient by themselves to establish a general composer grammar.

Named research targets from the saved discussions include:

- Sonic 3 Title and derivative jingles;
- Sonic 3 1-Up;
- Continue / VS Results;
- Act Clear / Act Passed;
- Data Select;
- Hydrocity Acts 1/2;
- Carnival Night prototype/final material;
- Launch Base prototype/final material;
- Sky Sanctuary prototype/final material;
- Knuckles theme prototype/retail relationships;
- Balloon Park;
- the Buxer/Jackson-team cue family;
- Sega/Cube/Opus boundaries where evidence permits.

A named target is not a presumed answer.

## Test program

### Test 1: blind VGM realization extraction

Before supplying creator labels, extract per-track downstream realization features:

```text
FM patch fingerprints
patch reuse graph
YM2612 operator/control profile
pitch-control trajectory profile
panning/control profile
PSG activity profile
DAC usage
physical-channel activity/density
register/control transition statistics
```

These features are useful for realization clustering and as composer confound controls. They are not automatically composer evidence.

### Test 2: composition-facing reconstruction from VGM/sequence evidence

Recover as much as evidence permits of:

```text
note-like events
relative pitch trajectories
onset/duration relationships
persistent musical parts
melody / bass / inner-voice candidates
rhythmic cells
phrase boundaries
motif recurrence / transformation
harmonic and formal relations
```

The goal is to turn VGM/SPC execution into a usable musical view, not to pretend the source was MIDI.

### Test 3: paired-version stability

For prototype/final and Act1/Act2 pairs, measure separately:

```text
musical-work continuity
arrangement/programming continuity
```

Questions include:

- Does melodic/bass/rhythmic structure remain while patch/control vocabulary changes?
- Does a final version preserve a prototype realization fingerprint?
- Does an Act 2 arrangement retain the work while changing creator-facing arrangement traits?

### Test 4: frozen Sonic 3 holdout

Do not train or tune on disputed targets such as Continue/Act Clear once they become evaluation items.

Report:

- supporting creator-grammar dimensions;
- contradicting dimensions;
- representation provenance;
- nearest controls by each evidence family;
- confidence;
- alternative hypotheses;
- realization/programming hypothesis separately.

### Test 5: cross-soundtrack same-creator controls

This is the major evidence upgrade.

Add VGM/SPC/PSF/MIDI/MML/tracker or other material from **different soundtracks** with known work by the same plausible Sonic 3 contributors.

The decisive question is:

> Does a proposed Sonic 3 behavior travel with the person outside Sonic 3, or does it stay with the Sonic 3 soundtrack/toolchain/team?

Prefer controls that vary one factor at a time where possible:

```text
same person + different soundtrack + same driver family
same person + different soundtrack + different driver/platform
same driver/toolchain + different person
same patch/library + different person
same composer + different arranger/programmer
same arranger/programmer + different composer
same composer + different soundtrack + different synthesis architecture
```

SPC controls are especially useful because they can force Genesis-specific synthesis fingerprints to disappear while deeper musical relations either survive or fail.

### Test 6: leave-one-soundtrack-out creator grammar

Once a candidate has material from several soundtracks:

```text
train grammar on soundtrack A + B
hold out soundtrack C
```

Then rotate.

A proposed creator habit that cannot generalize across soundtracks should remain soundtrack-local or period-specific until better evidence appears.

### Test 7: confound interventions

Rerun candidate matching under conditions such as:

```text
without patch/sample identity
without platform-specific features
without soundtrack-local features
without arranger/programmer features
transposition-normalized
tempo-normalized
without related work families
```

The delta tells us what the attribution was actually using.

### Test 8: creator evolution

Do not assume a composer is frozen.

Classify candidate rules as:

```text
stable long-range habit
career-period habit
soundtrack-local habit
collaborator-dependent habit
platform-dependent habit
one-off experiment
```

A later soundtrack that contradicts an earlier rule is evidence about evolution, not merely a failed classifier.

## Required output shape

A future Sonic 3 attribution should resemble:

```text
TRACK / VERSION

composer hypothesis
  candidate(s)
  cross-soundtrack creator rules
  within-Sonic-3 support
  representation convergence
  counterevidence
  confound sensitivity
  confidence

arrangement / programming hypothesis
  candidate(s)
  patch / modulation / channel / control support
  cross-soundtrack programmer controls
  counterevidence
  confidence

source / company hypothesis
  evidence
  confidence

version lineage
  prototype/final/PC/later relationships

unresolved conflicts
  preserved explicitly
```

Never compress this to one `artist = X` field.

## Immediate next step

1. Run the first blind VGM realization/trajectory audit over the committed Sonic 3 corpus.
2. Treat its output as a **within-soundtrack baseline**, not a composer model.
3. Keep disputed cue labels out of extraction and tuning.
4. Add the same-creator VGM/SPC controls from other soundtracks unchanged when they arrive.
5. Re-run the same feature extraction before tuning weights or adding creator-specific rules.
6. Promote only relations that survive independent works, role checks, and ideally cross-soundtrack validation into `composer_grammar_evidence`.

> **Sonic 3 is the question. Other soundtracks by the same creators are the controls that tell us whether we have learned the people or merely the game.**
