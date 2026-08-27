# Sonic 3 research project

Sonic 3 & Knuckles is an integrative research testbed for VGM Compiler.

The project asks:

> Can the system recover enough musical, implementation, version, and documentary structure to reason defensibly about composition, arrangement, programming, realization, and disputed authorship without collapsing those roles into one label?

This directory owns the bounded Sonic 3 research program and its frozen controls/evidence objects. It does not own general VGM Compiler semantics or creator-attribution law.

## Integrative model

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

No arrow means equivalence. Each layer may support, contradict, or refine another while retaining its own provenance.

## Creative-role boundary

Always preserve:

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver / engine programming
!= patch / voice design
!= soundtrack production
!= final device realization
```

A ROM, patch, driver, or SMPS fingerprint may strongly support an implementation or arrangement claim while carrying little direct composer evidence. Structural musical evidence may support composition-level reasoning even when another person implemented the final Mega Drive realization.

## Version boundary

Prototype, retail, Sonic 3, Sonic & Knuckles, later ports, soundtrack releases, and derivative arrangements are evidence objects in a lineage rather than interchangeable copies.

The model must allow:

```text
same musical work
+ different arrangement
+ different implementation
+ different version lineage
```

without losing any identity dimension.

## Attribution discipline

Sonic 3 is a held-out target environment, not the whole creator-training world.

Creator-facing rules become stronger only when they survive independent soundtracks, platforms, arrangers, drivers, and production contexts. Attribution is therefore a capstone pressure test of musical understanding, not metadata classification.

A defensible output separates:

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

A single `artist = X` result is not sufficient.

## Routing

Route by research obligation rather than filename inventory:

- broad role-aware attribution and controls stay in this project;
- harmonic/form pressure stays with the project's harmonic/form owner;
- ROM/driver/source recovery stays with the corresponding forensic owner;
- cross-representation controls stay with the cross-representation owner;
- frozen panels, preregistrations, admissions, mappings, and calibration policies remain tracked only when their exact bytes are current experimental evidence.

Use the tree or `tools/repository_catalog.py --focus sonic3` for current artifact inventory. Do not mirror every file here.

## Project rule

New work belongs here only when Sonic 3 itself is the integrative research object. A named Sonic 3 cue used merely as a generic mechanism test belongs with the mechanism owner instead.

Extend an existing chapter or evidence owner before creating a peer narrative. Promote general rules out of this project only after independent pressure earns them.
