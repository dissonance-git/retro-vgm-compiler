# Research

`research/` is organized by **durable research program**, not by date, game count, or latest experiment.

For repository-wide orientation, start at [`../CATALOG.md`](../CATALOG.md).

> Few trunks, many chapters. One canonical owner per question.

## Shelf

```text
research/
├── projects/     named integrative testbeds
├── music/        musical understanding / creator grammar / attribution methods
├── runtime/      execution / synthesis / control / state
├── formats/      source / driver / chip / platform investigations
├── validation/   observatories / controls / pressure tests
├── rendering/    fidelity / equivalence / reconstruction
├── enhancement/  bounded enhanced-rendering experiments
└── cache/        reusable derived analysis, never source truth
```

A game gets a project folder only when it is itself an integrative testbed. Otherwise it remains a corpus/control under the general program that owns the question.

## Fast routing

| Question | Owner |
| --- | --- |
| whole-track / soundtrack understanding | `music/` + current semantic docs |
| composer grammar / attribution method | `music/` |
| Sonic 3 integrative attribution case | `projects/sonic3/` |
| source/driver/chip mechanism | `formats/` |
| runtime/state relation | `runtime/` |
| external pressure test | `validation/` |
| rendering equivalence/reconstruction | `rendering/` / `enhancement/` |
| reusable parsed-song analysis | `cache/` |

## Sonic 3

`projects/sonic3/` is one program spanning attribution, ROM/SMPS evidence, cross-soundtrack controls, and validation generations.

Use:

```text
projects/sonic3/README.md
→ canonical policy / admission file for the question
→ exact frozen preregistration
→ cache only if reusable parsed analysis is needed
→ execution/ result for that generation
```

Do not merge these ownership layers:

```text
attribution-control-admissions.jsonl   grounded role evidence
cube-calibration-policy.json           CUBE rules, holdouts, transfer boundary
role-credit-index.jsonl                Genesis cache-routing controls
cache/                                 derived creator-blind song objects
```

`tools/build_admitted_composer_caches.py` joins canonical admissions with the Genesis routing controls **at runtime**. Cross-format controls therefore remain visible without copying CUBE admissions into a second credit table.

Corpus artist tags are locator evidence only. They do not override canonical admissions or historical role evidence.

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver programming
!= patch / sample design
!= final realization
```

Similarity can support a calibrated candidate. It cannot establish historical authorship by itself.

## Cache law

`research/cache/` contains reusable **derived projections**, never source evidence.

```text
immutable source
→ parse once
→ creator-blind song capsule
→ many cheap feature / experiment projections
```

Creator labels stay outside capsules. Experiment-specific similarity matrices, rankings, and reports stay with the bounded experiment rather than becoming the reusable cache.

## Placement law

Before adding research material:

1. Find the existing durable question that owns it.
2. Distinguish evidence, method contract, reusable derived object, and bounded result.
3. Treat named games as controls unless the game itself is the integrative research problem.
4. Extend an owner before creating a peer narrative.

Navigation compression must never erase evidence boundaries:

```text
source semantics != runtime execution != heard result
mechanism evidence != attribution evidence
exact source != derived cache != experiment result
one game observation != cross-system law
```

A good refactor makes research cheaper to re-enter without making claims easier to overstate.
