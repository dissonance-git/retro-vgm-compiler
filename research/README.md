# Research

`research/` is organized by **durable research program**, not by date, latest game, or latest experiment.

For repository-wide navigation, start at [`../CATALOG.md`](../CATALOG.md). This file only owns the research geography.

> **Few trunks, many chapters. One canonical owner per question.**

## Research shelf

```text
research/
├── projects/     named integrative testbeds
├── music/        musical understanding, composer grammar, attribution methods
├── runtime/      execution, synthesis, control, sample, and state semantics
├── formats/      source-, driver-, chip-, and platform-specific investigations
├── validation/   observatories, controls, pressure tests, external comparisons
├── rendering/    fidelity, equivalence, reconstruction, counterfactual rendering
├── enhancement/  bounded enhanced-rendering experiments
└── cache/        reusable derived analysis objects; never source truth
```

Do not create another peer-level shelf for a method that belongs inside one of these.

## Fast routing

| Research question | Canonical route |
| --- | --- |
| Whole-track / soundtrack musical understanding | `music/` + current semantic docs |
| Composer grammar / attribution method | `music/` |
| Sonic 3 integrative attribution case | `projects/sonic3/` |
| Source/driver/chip-specific mechanism | `formats/` |
| Runtime/execution/state relation | `runtime/` |
| External implementation or literature pressure test | `validation/` |
| Reference/enhanced equivalence or reconstruction | `rendering/` / `enhancement/` |
| Reusable parsed song analysis | `cache/` |

A named game gets a project folder only when the game itself is an **integrative testbed spanning several evidence layers**. A game that merely supplies evidence for one mechanism remains a corpus/control under the owning general program.

## Sonic 3

`projects/sonic3/` is one research program. Do not split composer attribution, ROM forensics, SMPS recovery, cross-soundtrack controls, and validation generations into neighboring projects.

Current entry route:

```text
projects/sonic3/README.md
→ role-credit-index.jsonl
→ exact frozen preregistration for the generation being executed
→ ../cache/README.md when reusable song capsules are relevant
→ execution/ for bounded results
```

Important current infrastructure:

```text
research/projects/sonic3/role-credit-index.jsonl
research/cache/README.md
tools/creator_blind_song_cache.py
tools/build_admitted_composer_caches.py
```

Do not reconstruct creator controls from corpus tags when the role-credit index already owns the admitted role-scoped labels.

The role firewall remains:

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver programming
!= patch / sample design
!= final realization
```

Similarity can support a calibrated candidate. It cannot by itself establish historical authorship.

## Cache law

`research/cache/` contains **derived reusable projections**, not source evidence.

The canonical source remains the immutable corpus object. Creator labels stay outside creator-blind song capsules. Expensive source parsing should be performed once when a reusable early representation can safely support many later feature projections.

Do not store every experiment-specific similarity matrix as if it were the reusable cache. Matrices, rankings, and evaluation reports belong to the bounded experiment that produced them.

## Placement rule

Before adding a research file, ask:

1. Which existing durable question owns it?
2. Is this evidence, a reusable derived object, a method contract, or a bounded result?
3. Is the named game the research question, or merely a control/source?
4. Would adding a peer document create a second narrative for state already owned elsewhere?

Prefer extending the existing owner.

Create a new folder only when several investigations genuinely share one durable question. Avoid one-file folders, date-shaped geography, and flat roots full of narrowly named passes.

## Evidence rule

Navigation compression is organizational, not epistemic. It must not erase distinctions such as:

```text
composition != arrangement != programming != driver behavior
source semantics != runtime execution != heard result
mechanism evidence != attribution evidence
exact source != derived cache != experiment result
one game-specific observation != cross-system rule
```

A good refactor makes research cheaper to re-enter without making claims easier to overstate.
