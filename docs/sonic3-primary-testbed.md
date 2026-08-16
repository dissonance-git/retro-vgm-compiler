# Sonic 3 primary integration testbed

Status: active primary integration testbed  
Target corpus: `tests/corpus/sonic-3-knuckles`  
Research control: `research/sonic3-composer-programmer-attribution.md`  
ROM forensics: `research/sonic3-rom-forensic-attribution.md`

## Purpose

Sonic the Hedgehog 3 & Knuckles is the project's primary end-to-end testbed for holistic musical understanding.

It is not the project ontology, and success on Sonic 3 is not sufficient evidence that an abstraction generalizes to every game-music system. It is the most demanding currently assembled laboratory in which many project layers can be tested against one another with unusually rich independent evidence.

The testbed should **build the interpreter while testing it**.

```text
new capability
    ↓
apply to Sonic 3 under a bounded blind test
    ↓
compare against independent evidence / intervention / control
    ↓
find the failure mode
    ↓
improve shared or source-specific machinery
    ↓
re-run unchanged benchmark
```

A capability should not be added merely to make Sonic 3 attribution easier. Sonic 3 supplies adversarial pressure that should improve the general machinery used by VGM, SPC, native sequence formats, PSF-family sources, and future representations.

## Why Sonic 3 is unusually useful

The available evidence spans several independent layers:

```text
historical discussion and documentary claims
curated but revisable attribution hypotheses
prototype and final versions
SMPS sequence source / disassembly
Z80 sound-driver source and research notes
raw ROM / binary layout and provenance residue
YM2612 / PSG / DAC execution
58 committed VGM/VGZ target fixtures
same-driver / different-soundtrack controls
same-creator / different-soundtrack controls
cross-platform VGM controls
cross-architecture SPC controls
later ports / arrangements where lineage is defensible
```

This makes it possible to ask whether an inference survives deliberate changes in representation, realization, soundtrack, platform, collaborator, driver, patch library, binary organization, and historical label.

## Orthogonal ROM-forensics lane

The testbed also asks a separate historical/provenance question:

> Did the shipped or prototype ROM preserve anything about who created, imported, converted, or grouped the music?

Some games retain literal filenames, source paths, author initials, tool strings, or other development residue. Even when symbolic names are stripped, binary organization can preserve weaker traces of the production process.

For Sonic 3/S&K, investigate:

```text
literal strings / names / initials
source-path or filename residue
tool/export signatures
music pointer-table order
physical storage order
Z80 bank boundaries
alignment and padding
shared local voice/envelope/sample resources
prototype/final orphaned or duplicated data
exact/near-exact regions shared with other games
```

A crucial control is already established: visible names such as `AIZ1.asm`, `HCZ2.asm`, `mus_AIZ1`, and the song filenames in `smps-rips` are reconstruction/archive labels unless byte-level evidence proves that the original ROM contained them. Initial searches of the major reconstructed Sonic 3/S&K sources found no obvious literal candidate-composer names. This is not yet proof of absence.

ROM forensics must run in two modes:

```text
MUSICAL BLIND MODE
exclude filenames, tags, disassembly labels, textual ROM residue,
known IDs that leak cue identity, and attribution metadata

FORENSIC MODE
allow strings, paths, tool signatures, pointer topology,
bank placement, region identity, and other binary provenance evidence
```

A filename that directly names a creator can be excellent historical evidence while being a catastrophic shortcut for a musical-understanding benchmark. Therefore forensic evidence is frozen separately and used as independent validation after musical outputs are frozen.

The more subtle question is especially interesting if literal names are absent:

> **Does the topology of the ROM preserve the topology of the production process?**

See `research/sonic3-rom-forensic-attribution.md`.

## Testbed law

Every important new musical capability should eventually answer at least one Sonic 3 question that lower layers could not answer before.

Examples:

```text
persistent-part tracking
→ can execution-only VGM recover musical parts that remain coherent across channel reuse?

pitch recovery
→ can performed pitch be reconstructed without collapsing detune/modulation into nearest MIDI notes?

phrase analysis
→ can prototype/final versions be aligned by phrase despite changed realization?

motif analysis
→ can related material be recognized after transposition, rearrangement, or timbral change?

harmonic analysis
→ can the system explain bass reinterpretation and non-chord tones rather than merely list pitch classes?

form analysis
→ can Act 1 / Act 2 and prototype/final relationships be described as transformations of musical structure?

composer grammar
→ do relationships recur across independent soundtracks after driver/platform/timbre confounds are removed?

human discourse
→ can the result be explained in language a composer, producer, engineer, reviewer, or musicologist would recognize?
```

A technical capability that cannot yet climb into a musical question remains useful infrastructure, but it has not completed its testbed journey.

## Five nested benchmark rings

### Ring 1: exact reconstruction from execution

Use source-available SMPS material as a hidden oracle where lineage is secure.

```text
SMPS source
  exact logical tracks
  notes / rests / durations
  calls / loops / jumps
  voices
  modulation / detune / articulation commands
        ↓ execute
VGM
        ↓ hide SMPS
interpreter reconstruction
        ↓
compare recovered musical object against source
```

Measure separately:

- onset recovery;
- relative-pitch recovery;
- duration/rest recovery where observable;
- logical-part continuity;
- false part joins and splits;
- loop/form recovery;
- modulation/detune handling;
- motif and phrase preservation;
- uncertainty calibration.

The source is a teacher for evaluation, not a license to assume every VGM came from SMPS or that source tokens equal heard musical events.

### Ring 2: prototype/final intervention

Prototype/final pairs are natural experiments.

Possible transformations include:

```text
same composition + changed instrumentation/programming
same composition + changed arrangement
same cue family + structural rewrite
complete cue replacement
changed driver semantics
changed samples / envelopes / modulation
```

The system should learn which coordinates remain stable and which move.

Required outputs distinguish at least:

```text
work identity
composition structure
arrangement
sound-data programming
patch/sample vocabulary
driver environment
final realization
```

Do not reduce this ring to one version-similarity score.

### Ring 3: driver/toolchain leakage

Sonic & Knuckles and Sonic 3D Blast provide an unusually strong matched control because documented development builds reuse essentially the same S&K sound-driver environment while the soundtrack changes.

The test is causal in spirit:

```text
feature follows driver into unrelated soundtrack
→ likely implementation/toolchain evidence

feature follows creator across soundtrack while driver changes
→ stronger creator-facing evidence
```

Driver-specific behavior should be modeled explicitly so the creator model can subtract or condition on it rather than merely hoping machine learning ignores it.

### Ring 4: cross-soundtrack creator generalization

The committed external controls currently include complete soundtracks selected around Tatsuyuki Maeda, Tomonori Sawada, Masayuki Nagao, Masaru Setsumaru, Miyoko Takaoka, and Masanori Hikichi.

The attribution labels are not extraction inputs.

The blind order is:

```text
freeze extractor
        ↓
run Sonic 3 + external soundtrack controls
        ↓
freeze features / neighbors / learned relations
        ↓
unblind only the permitted evaluation metadata
        ↓
measure what followed person vs soundtrack vs toolchain
```

Prefer leave-one-soundtrack-out evaluation over random track splits.

A model must not win by recognizing:

- the same work family;
- a prototype/final pair;
- a soundtrack-specific patch bank;
- one driver;
- one platform;
- one collaborator;
- cue function;
- file metadata.

### Ring 5: cross-architecture creator generalization

SPC controls are intentionally valuable because Genesis implementation vocabulary disappears.

A useful creator-facing relation should be able to survive cases such as:

```text
YM2612 / PSG arrangement
        ↓
composer-facing musical relation
        ↑
SPC700 / S-DSP arrangement
```

The common object is not a MIDI transcription. It is the musical relation earned independently from each representation.

Future PSF-family, tracker, MML, MIDI-like, audio, or other source controls should join this ring when their provenance is strong enough.

## Two orthogonal invariance axes

The testbed formalizes two different kinds of invariance.

### Work invariance

What survives changes to the realization of one musical work?

Candidate transformations:

- transposition;
- tempo change;
- patch/sample replacement;
- channel reassignment;
- platform change;
- different arrangement;
- prototype/final rewrite;
- later port;
- audio versus symbolic versus execution representation.

### Creator invariance

What survives when the person writes different music?

Candidate transformations:

- different cue;
- different soundtrack;
- different game;
- different collaborator;
- different platform;
- different driver;
- different sound palette;
- different career period.

The strongest composer evidence lies where a relation is robust across both axes without erasing genuine creator evolution.

## Composer grammar is a trajectory, not a fingerprint

Do not model a composer as one static centroid.

The testbed should distinguish:

```text
stable long-range habit
career-period habit
soundtrack-local habit
collaborator-conditioned habit
platform-conditioned habit
arranger/programmer contribution
one-off experiment
```

A contradiction from another soundtrack is evidence about the shape of the creator model. It is not automatically an error to discard.

## Curated Sonic 3 labels are hypotheses

`research/sonic3-curated-attribution-hypotheses.json` is a post-hoc evaluation set.

Its rules are deliberate:

```text
ground_truth = false
revisable = true
Sega Sound Team != one person
```

Named composer assignments are the user's best current knowledge, not guaranteed historical facts.

Therefore:

```text
blind model agrees
→ support for current hypothesis

blind model disagrees
→ inspect model AND label evidence
```

No extraction or representation-learning stage may read these labels.

## Capability ladder

The Sonic 3 testbed grows with the project.

### L0: exact container/device facts

Available now:

- VGM command timing;
- YM2612/PSG/DAC state;
- immutable corpus hashes;
- prototype/final inventory;
- external soundtrack control inventory;
- reconstructed numbered music-table and raw SMPS-rip references for binary forensics.

### L1: physical performance evidence

Available in part:

- ordinary YM2612 full key-ons;
- relative pitch trajectories;
- patch/control fingerprints;
- physical-channel activity;
- conservative execution features.

### L2: persistent musical identity

Required next:

- trajectory-level persistent-part tracking;
- allocation-change recovery;
- overlap/conflict handling;
- alternate identity hypotheses;
- role-neutral part continuity.

### L3: compositional structure

Required:

- note/rest/duration reconstruction where supported;
- melody/bass/inner-voice/accompaniment roles;
- rhythmic cells and groove relations;
- motifs and transformations;
- harmony and structural bass;
- voice leading/counterpoint;
- phrase/cadence segmentation;
- form and developmental relations.

### L4: multi-view work understanding

Required:

- SMPS ↔ VGM correspondence;
- prototype ↔ final correspondence;
- later-version correspondence;
- source-versus-realization transformation explanations;
- agreement and disagreement across representations.

### L5: creator grammar

Required:

- cross-work rules;
- cross-soundtrack rules;
- matched driver/platform controls;
- confound interventions;
- career-period/evolution model;
- role-scoped composition versus arrangement/programming evidence.

### L6: blind attribution stress test

Required:

- frozen candidate set;
- held-out disputed cues;
- leave-one-soundtrack-out evaluation;
- balanced per-candidate metrics;
- calibration and abstention;
- supporting and contradicting musical explanations;
- label revision when independent evidence warrants it.

A correct label without the lower explanatory ladder does not count as full success.

## Testbed outputs

Every serious Sonic 3 experiment should preserve machine-readable output with:

```text
experiment id
code / model version
corpus hashes
representation inputs
hidden/evaluation labels policy
feature families enabled
confound interventions
work-family grouping
soundtrack grouping
platform / driver grouping
ROM-forensic mode and allowed leakage surface
evidence provenance
predictions / neighbors / correspondences
supporting evidence
counterevidence
confidence / calibration
abstentions
known limitations
```

Do not overwrite earlier contradictory runs. Improvements should be comparable over time.

## Immediate build order

1. Make the testbed inventory and blind VGM baseline runnable from one entry point.
2. Implement the ROM-forensics audit: strings, pointer/storage order, banks, regions, exact cross-build matches, and leakage controls.
3. Integrate trajectory-level persistent musical parts into the VGM analysis path.
4. Add SMPS-source oracle comparisons for secure source/realization pairs.
5. Build phrase/motif/rhythm features on persistent parts, not physical channels.
6. Add harmonic/voice-leading/form relations.
7. Add an SPC composer-facing extractor that reaches the same earned musical relation space without pretending SPC is MIDI.
8. Freeze cross-soundtrack outputs before revealing routing/attribution metadata.
9. Run confound interventions and matched driver controls, especially Sonic 3/S&K versus Sonic 3D Blast.
10. Freeze the musical result before admitting ROM-forensic provenance evidence.
11. Only then attempt serious blind composer attribution and historical evidence fusion.
12. Feed every discovered failure back into the general project architecture and re-run the same testbed.

## Success condition

The ultimate Sonic 3 result is not:

```text
track X = composer Y
```

It is closer to:

```text
The system reconstructed how the cue is built,
understood which relationships survive its different representations,
separated composition from arrangement/programming and driver artifacts,
learned which deeper relations recur in independent soundtracks,
recognized where the candidate composer changed over time,
and attributed the cue with traceable supporting and contradicting musical evidence.
```

> **Sonic 3 is the project's musical wind tunnel: every layer can be stressed there, but every lesson must be allowed to escape the tunnel and improve the general interpreter.**
