# Sonic 3 composer / arranger / programmer attribution control

Status: active project control  
Primary goal: use Sonic 3 & Knuckles as a hard real-world test of composer-level understanding without collapsing composition, arrangement, programming, source company, driver behavior, or version history into one `artist` label.

Related:

- `research/sonic3-vgm-gun-hazard-spc.md`
- `research/sonic-smps-pitch-recovery.md`
- `research/musicological-authorship-attribution.md`
- `research/retro-composition-programming-listening.md`
- `docs/composer-level-understanding.md`

## Research archive

The current project background includes a user-supplied offline archive saved on 2026-08-15 containing:

- all 18 saved pages of **The Sonic 3 Music Ramble**;
- all 29 saved pages of **The "Sonic The Hedgehog 3 & Knuckles" Quest for Music Composer Research**;
- the saved Sonic Retro **Sonic the Hedgehog 3/Development/Music** page.

The two forum threads contain 921 saved posts in total. They are research evidence, not an automatic truth set. Individual posts range from first-hand recollection and cited documentary evidence to explicit speculation, technical inference, correction of earlier claims, and ordinary discussion.

This distinction is essential because the discussion itself demonstrates that Sonic 3 attribution is a moving evidence problem rather than a static credits table.

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

Therefore a technical fingerprint recovered from VGM or SMPS may strongly support an **arranger/programmer** hypothesis while providing little or no direct support for a **composer** hypothesis.

That boundary is a first-class regression requirement.

## Symbolic source is not one uniform MIDI layer

The 2025-2026 Music Ramble discussion describes the Sonic 3 production sources as heterogeneous and asynchronous. The proposed historical picture includes material arriving from several parties and in different states, with MIDI-like source data in some cases but substantial Mega Drive-side alteration in others.

The durable architectural lesson is independent of the exact historical workflow:

```text
incoming symbolic material
        ↓
arrangement / editing / embellishment
        ↓
SMPS sequence and control decisions
        ↓
YM2612 / PSG / DAC execution
```

A surviving MIDI, Collection MIDI, prototype sequence, final SMPS sequence, and final VGM may therefore be related views of one musical work without being interchangeable copies.

The Music Ramble specifically discusses examples where the Mega Drive arrangement appears to add or rewrite material relative to a MIDI-like source, including bass, PSG activity, intros, channel use, and other sequence details.

Game Music Interpreter should model those differences as transformations and correspondences, not as transcription errors to erase.

## Evidence classes from the forum archive

### Documentary / first-hand evidence

The saved material includes or discusses direct composer/arranger statements, interviews, old websites, game/source credits, prototype evidence, and private correspondence.

One important uncertainty control comes from Dissident93's 2019 post #26 in the composer-research thread: he records that some older Uwabo/Nagao information came from his direct Facebook contact, but that the original messages were no longer recoverable, and that Nagao was not remembered as completely certain about the full extent of his contributions.

That means:

```text
reported first-hand recollection
!= preserved primary document
!= exact exhaustive track list
```

The project should preserve that evidence as useful testimony with uncertainty rather than promoting it to exact ground truth.

The same post also raises the possibility of reused pre-Sonic-3 material, noting the Special Stage / Blue Spheres lineage to SegaSonic Bros. The saved Development/Music page likewise documents that a previously created Yoshiaki Kashima song was repurposed for Sonic 3.

Therefore:

```text
appears in Sonic 3
!= necessarily composed specifically for Sonic 3
```

### Version evidence

Prototype, final, Collection/PC, soundtrack, later-port and later-arrangement versions are evidence objects, not nuisance duplicates.

The archive repeatedly uses version differences to reason about:

- which material existed by a particular build;
- whether one arrangement was replaced or substantially rewritten;
- whether the same composition received a different programmer/arranger;
- which note, bass, PSG, voice, modulation, and channel-layout decisions survived;
- whether later versions descend from an earlier symbolic source rather than the final Mega Drive realization.

A key proposed example is Sky Sanctuary, where the Music Ramble argues that prototype and final programming characteristics may indicate different realization authors even though the musical work remains recognizably related.

The interpreter must therefore be able to represent:

```text
same work / cue identity
+
different realization lineage
+
different arrangement/programming hypothesis
```

without forcing either identity to overwrite the other.

### Realization fingerprints

The later Music Ramble develops a practical programmer/arranger fingerprint vocabulary. Candidate evidence includes:

- exact FM voice / patch reuse;
- Universal Voice Bank use versus locally included voices;
- modulation-control patterns and recurring `smpsModSet` parameter families;
- song-header initial values;
- PSG participation and recurring PSG techniques;
- channel layout and channel-sharing choices;
- glissando / pitch-control habits;
- `smpsNoAttack` and related articulation/control idioms;
- panning and other control behavior;
- optimization state, calls, loops and repeated-section factoring;
- characteristic additions made during conversion/arrangement;
- reuse of the same technical choices in other games associated with the same programmer.

These are not all equally observable from VGM. Some require SMPS/source evidence, while VGM can expose downstream patch state, register/control trajectories, channel behavior and timing.

The shared model must record which layer actually supports each feature.

## A critical correction case: inherited labels can be wrong

The saved 2026 Music Ramble supplies a particularly useful adversarial example.

The saved Development/Music page currently presents Sonic 3's `Continue` and `Stage Clear - Act Passed` as Jun Senoue compositions/arrangements based on Sonic 3D Blast evidence.

Music Ramble post #339 (2026-08-08) explicitly revisits that interpretation. The post argues that the Sonic 3D Blast credit data contains a gap after the credited Game Over cue, and that earlier interpretation may have incorrectly carried the preceding Jun Senoue credit across uncredited entries. It then points to implementation evidence that the author considers more Nagao-like, including Universal Voice Bank use, modulation patterns, PSG behavior and prototype/final sequence differences.

For Game Music Interpreter this is not yet a verdict about the historical author. It is a regression shape:

```text
inherited metadata / prior interpretation
        +
new source-layout evidence
        +
new technical realization evidence
        ↓
previous attribution must become revisable
```

No confidence system is acceptable if an old label silently dominates contradictory new evidence.

## Composer understanding versus realization attribution

The archive also exposes the inverse danger: identifying the programmer is not the same as understanding the composer.

Music Ramble post #207 explicitly notes that FM voices alone cannot identify composers reliably because much of the soundtrack appears to have been programmed or arranged by someone other than the original composer.

The project should therefore maintain two different feature families.

### Composition-facing features

Examples:

- melodic interval and contour habits;
- phrase construction;
- bass-motion strategy;
- harmonic vocabulary and harmonic rhythm;
- cadential behavior;
- motivic recurrence and transformation;
- rhythmic cells and syncopation;
- counterpoint and inner-voice behavior;
- formal proportions and developmental strategy.

### Arrangement / programming-facing features

Examples:

- FM patch vocabulary;
- modulation and articulation controls;
- PSG deployment;
- voice-bank behavior;
- channel allocation/layout;
- register assignment and doubling;
- panning/control idioms;
- optimization/call/loop structure;
- implementation-specific embellishment.

A composition match with a realization mismatch is meaningful. A realization match with a composition mismatch is meaningful. Neither should be averaged into one anonymous similarity score.

## Existing high-value corpus contrasts

The committed 58-file Sonic 3 & Knuckles VGM set already supplies several useful within-project controls:

```text
prototype ↔ final
Act 1 ↔ Act 2
Sonic 3 ↔ Sonic & Knuckles replacement cue
same cue family ↔ changed realization
shared patch vocabulary ↔ different musical material
similar musical material ↔ changed patch/control vocabulary
```

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
- the Sega/Cube/Opus boundaries where evidence permits.

A named target is not a presumed answer.

## Test program

### Test 1: blind realization-feature extraction

Before supplying any artist/programmer labels, extract per-track realization features from the VGM corpus:

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

The output must not contain artist names.

Success criterion: technically meaningful repeated realization families emerge without metadata leakage.

### Test 2: paired-version stability

For prototype/final and Act1/Act2 pairs, measure separately:

```text
composition-like continuity
arrangement/programming continuity
```

Do not use one similarity score.

Questions include:

- Does melodic/bass/rhythmic structure remain while patch/control vocabulary changes?
- Does a final version preserve a prototype's realization fingerprint?
- Does an Act 2 arrangement retain the composition while moving toward another programmer family?

### Test 3: known-control calibration

Use only tracks whose role attribution is sufficiently documented for the specific coordinate being tested.

For example, a confirmed composition credit can calibrate composition features but cannot automatically calibrate programmer features. A confirmed arrangement/programming statement can calibrate realization features but cannot automatically calibrate composition.

Every control must declare:

```text
person
role
track/version
source
confidence
what the control is allowed to supervise
what it is forbidden to supervise
```

### Test 4: disputed-target holdout

Do not train or tune on disputed targets such as the newer Continue/Act Clear questions.

After fingerprints are calibrated on controls, score disputed tracks independently and report:

- supporting dimensions;
- contradicting dimensions;
- nearest controls by each feature family;
- whether evidence is source/driver/device/compositional;
- confidence;
- alternative hypotheses.

No automatic historical verdict.

### Test 5: cross-game same-artist controls

The next major evidence upgrade is additional VGM/SPC material from games with known work by the same composers/arrangers/programmers.

These controls should answer the decisive question:

> Does a proposed Sonic 3 fingerprint travel with the person outside Sonic 3, or is it merely a Sonic 3 toolchain/library artifact?

Prefer controls that vary one factor at a time where possible:

```text
same person + same driver family + different game
same person + different driver/platform
same driver/toolchain + different person
same shared patch/library + different person
same composer + different arranger/programmer
same arranger/programmer + different composer
```

SPC controls are especially valuable because they can test whether composition-facing habits survive a completely different synthesis architecture while implementation-facing fingerprints appropriately disappear or transform.

### Test 6: composer-level musical fingerprint

Once note/sequence and persistent-part recovery is strong enough, build a separate composition analysis over:

- melody;
- bass;
- harmony;
- rhythm;
- phrase/form;
- motivic transformation;
- counterpoint;
- repetition/variation;
- arrangement-independent structural relations.

This is the layer that should eventually answer questions like `does this track think like composer X?`, while the realization layer answers `does this track appear to have been programmed/arranged like programmer Y?`.

The two answers may disagree, and disagreement is evidence.

## Required output shape

A future Sonic 3 attribution result should resemble:

```text
TRACK / VERSION

composition hypothesis
  candidate(s)
  structural support
  counterevidence
  confidence

arrangement / programming hypothesis
  candidate(s)
  patch / modulation / channel / control support
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

Run the first blind realization-fingerprint audit over the committed Sonic 3 VGM corpus. The initial goal is not to solve attribution. It is to determine whether VGM-visible patch/control/channel features reproduce any stable families at all and which candidate features are too toolchain-wide to discriminate people.

When additional same-artist VGM/SPC controls are added, rerun the exact same feature extraction unchanged before tuning weights or adding new fingerprint rules.
