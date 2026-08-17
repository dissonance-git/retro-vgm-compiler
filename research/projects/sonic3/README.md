# Sonic 3 research project

Sonic 3 & Knuckles is an integrative research testbed for Retro VGM Compiler.

The project asks one broad question:

> Can the system recover enough musical, implementation, version, and documentary structure to reason defensibly about composition, arrangement, programming, realization, and disputed authorship without collapsing those roles into one label?

The project deliberately couples several evidence layers while preserving their boundaries.

## Project chapters

- [`attribution.md`](attribution.md) is the broad composer / arranger / programmer attribution program and control design.
- [`harmonic-form.md`](harmonic-form.md) is the executable harmonic/tonal/form pressure test, including the blind VGM surface-harmony lane and the promotion path into persistent-part, key, function, cadence, tonal-region, and creator-grammar evidence.
- [`psg-role-semantics.md`](psg-role-semantics.md) models Sonic 3's mixed YM2612+SN76489 arrangement surface: independent PSG tone, FM/PSG doubling or shadowing, PSG noise/percussion behavior, and the rule that hardware register position is not musical bass function.
- [`rom-forensics.md`](rom-forensics.md) studies ROM-level implementation evidence and how it constrains attribution without turning implementation into composition evidence.
- [`smps-pitch-recovery.md`](smps-pitch-recovery.md) studies inverse recovery of source-side SMPS pitch trajectories from downstream YM2612 execution.
- [`cross-representation-controls.md`](cross-representation-controls.md) provides paired VGM/SPC and cross-representation controls for separating composition, arrangement, implementation, and format effects.
- [`attribution-control-admissions.jsonl`](attribution-control-admissions.jsonl) is the role-specific, exact-fixture supervision boundary. Locator metadata cannot become a composer control without an independent admission here.
- [`cube-calibration-policy.json`](cube-calibration-policy.json) defines the evidence-safe Takaoka-versus-Hikichi calibration program, including strict soundtrack-held-out transfer, team-level validation worlds, joint-composition, disputed-mapping, weak-recollection, and third-party holdouts.
- [`spc-cube-blind-panel.json`](spc-cube-blind-panel.json) freezes the 31-cue Ancient Magic / Terranigma / America Oudan / Battle Master SPC panel used to create creator-blind runtime geometry before the evidence reveal.
- [`monster-maker-3-soundtest-mapping-hypothesis.json`](monster-maker-3-soundtest-mapping-hypothesis.json) preregisters the staged Monster Maker 3 pack-order versus documented sound-test mapping test. Its conditional Takaoka/Hikichi mappings are hypotheses, never admissions, until machine-state or independent musical evidence verifies the mapping.
- [`curated-attribution-hypotheses.json`](curated-attribution-hypotheses.json) is the machine-readable hypothesis surface used by the project tooling; it is revisable evaluation context rather than attribution ground truth.

## Shared project model

```text
documentary / historical evidence
            ↕
prototype / final / port lineage
            ↕
SMPS source and sequence semantics
            ↕
ROM / driver / sound-data implementation
            ↕
YM2612 / PSG / DAC execution
            ↕
persistent musical parts and pitch trajectories
            ↕
melody • bass • harmony • rhythm • form • arrangement
            ↕
creator grammar across independent soundtracks
            ↓
role-aware attribution hypotheses
```

No arrow means equivalence. Each layer can support, contradict, or refine another while retaining its own provenance.

## Role boundary

The project must always preserve:

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver / engine programming
!= patch / voice design
!= soundtrack production
!= final device realization
```

A ROM or SMPS fingerprint may strongly support an implementation or arrangement hypothesis while giving little direct composer evidence. Conversely, structural musical evidence can support composition-level reasoning even when the final Mega Drive realization was implemented by someone else.

## Version boundary

Prototype, retail, Sonic 3, Sonic & Knuckles, later ports, soundtrack releases, and derivative arrangements are evidence objects in a lineage, not duplicate files to be normalized away.

The project should be able to represent:

```text
same musical work
+
different arrangement
+
different implementation
+
different version lineage
```

without losing any of those identities.

## Attribution discipline

Sonic 3 is the held-out target environment, not the whole creator-training world.

Creator-facing rules become substantially stronger when they survive outside Sonic 3 across different soundtracks, platforms, arrangers, drivers, and production contexts.

The project therefore treats attribution as a capstone stress test of musical understanding rather than a metadata-classification task.

A future result should explain separately:

```text
composer hypothesis
arrangement / programming hypothesis
source / company hypothesis
version lineage
supporting evidence
counterevidence
confound sensitivity
unresolved alternatives
confidence
```

A single `artist = X` answer is not an acceptable project output.

## Project rule

New Sonic 3 research that participates in this same attribution / source / realization testbed belongs here rather than as a new peer-level file under `research/`.

Add a new chapter only when it has a genuinely distinct experimental object or evidence contract. Otherwise extend an existing chapter.
