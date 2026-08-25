# Research

`research/` is organized by **durable research program**, not date, game count, or latest experiment. Research can be dense; duplication and ambiguous ownership are the enemies.

> Few trunks, many chapters. One canonical owner per question.

## Shelf

```text
research/
├── projects/     named integrative testbeds
├── music/        musical understanding / creator grammar / attribution methods
├── runtime/      execution / synthesis / control / state
├── formats/      source / driver / chip / platform investigations
├── validation/   observatories / controls / pressure tests
└── rendering/    fidelity / equivalence / reconstruction / enhancement
```

A named game gets a project folder only when the game itself is an integrative testbed. Otherwise it remains a corpus/control under the general program that owns the question.

Derived analysis caches are runtime state, not research documents. Tools may materialize them under ignored `research/cache/` or another caller-selected path; they are never source truth and are never committed.

## Fast routing

| Question | Owner |
| --- | --- |
| whole-track / soundtrack understanding | `music/` + `docs/musical-understanding.md` |
| composer grammar / attribution method | `music/` |
| Sonic 3 integrative attribution case | `projects/sonic3/` |
| Genesis driver/toolchain source quarry and re-entry | `formats/genesis/genesis-driver-source-ledger.md` |
| source / driver / chip mechanism | `formats/` |
| runtime/state relation | `runtime/` |
| external pressure test | `validation/` |
| rendering equivalence / reconstruction / enhancement | `rendering/` |

For Genesis source semantics, the ledger is provenance/re-entry, while conceptual and experimental owners remain the format-specific documents around it. Inventory, generic model conclusions, comparative driver anatomy, and forward/inverse validation are separate concerns even when they share evidence.

## Sonic 3

`projects/sonic3/` is one program spanning attribution, ROM/SMPS evidence, cross-soundtrack controls, and validation generations.

Use:

```text
projects/sonic3/README.md
→ canonical policy / admission file
→ exact frozen preregistration
→ creator-blind derived cache if required
→ execution / result for that generation
```

Do not merge these ownership layers:

```text
attribution-control-admissions.jsonl   grounded role evidence
cube-calibration-policy.json           CUBE rules, holdouts, transfer boundary
role-credit-index.jsonl                Genesis cache-routing controls
derived cache                          disposable creator-blind song objects
```

`tools/build_admitted_composer_caches.py` joins canonical admissions with routing controls at runtime. Cross-format controls remain visible without copying attribution state into a second source of truth.

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver programming
!= patch / sample design
!= final realization
```

Similarity can support a calibrated candidate. It cannot establish historical authorship by itself.

## Placement law

Before adding research material:

1. Find the existing durable question that owns it.
2. Distinguish evidence, method contract, reusable derived object, and bounded result.
3. Treat named games as controls unless the game itself is the integrative research problem.
4. Extend an owner before creating a peer narrative.
5. Move a result into a durable contract only after the experiment has earned that generalization.

Navigation compression must never erase evidence boundaries:

```text
source semantics != runtime execution != heard result
mechanism evidence != attribution evidence
exact source != derived cache != experiment result
one game observation != cross-system law
```

A good refactor makes research cheaper to re-enter without making claims easier to overstate.
