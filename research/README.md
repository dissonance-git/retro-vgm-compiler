# Research

`research/` is organized by **research program**, not by the date a pass was run or by whichever game happened to supply the latest evidence.

The goal is a small number of durable trunks with experiments as chapters beneath them.

## Tree

```text
research/
├── projects/     named integrative testbeds
├── music/        composer-level, musicological, perceptual, and cognitive work
├── runtime/      cross-system execution, synthesis, control, sample, and state semantics
├── formats/      source-, driver-, chip-, and platform-specific investigations
├── validation/   observatories, controls, pressure tests, and external research surfaces
└── rendering/    fidelity, equivalence, reconstruction, and counterfactual rendering
```

## Projects

A named game gets a project folder only when the game is itself an **integrative research testbed** spanning several evidence layers.

### Sonic 3

`projects/sonic3/` is one research program, not several neighboring cases. Its project README owns the broad attribution question, while focused investigations remain separate chapters:

```text
projects/sonic3/
├── README.md
├── rom-forensics.md
├── smps-pitch-recovery.md
├── cross-representation-controls.md
└── curated-attribution-hypotheses.json
```

This keeps ROM forensics, SMPS inverse recovery, VGM/cross-format controls, documentary evidence, and composer/arranger/programmer attribution coupled without flattening their evidence boundaries.

A game that merely supplies evidence for one mechanism does **not** get its own project trunk. For example, the Chrono Cross AKAO and provenance investigations live under `formats/` as PS1-format research rather than becoming a Chrono Cross project.

## Placement rule

Before adding a new top-level research document, ask:

1. Is this a chapter of an existing project or program?
2. Does an existing folder already own the research question?
3. Is the named game only a corpus/control for a more general mechanism?
4. Would a new file create a parallel narrative that should instead extend an existing project README or focused experiment?

Prefer extending the existing trunk.

Create a new folder only when several investigations genuinely share one durable research question. Avoid one-file folders and avoid returning to a flat root of narrowly named passes.

## Evidence rule

Folder consolidation is organizational, not epistemic. Moving two investigations together does not make their claims equivalent.

Keep intact distinctions such as:

```text
composition != arrangement != programming != driver behavior
source semantics != runtime execution != heard result
mechanism evidence != attribution evidence
one game-specific observation != cross-system rule
```

Research should become easier to navigate without becoming easier to overclaim.
