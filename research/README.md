# Research

`research/` owns evidence, experiments, controls, observatories, and bounded investigations. It does not own project identity, durable semantic law, the musical north star, or current project priority.

Those authorities are:

```text
README.md                         repository identity and routing
docs/architecture.md             durable semantic/evidence law
docs/musical-understanding.md    musical target
docs/vgm-compiler-roadmap.md     implemented / active / next state
```

Research is organized by durable question, not date, experiment order, or game count.

> Few trunks, many chapters. One canonical owner per question.

## Programs

```text
research/
├── projects/     named integrative testbeds
├── music/        musical understanding / creator grammar / attribution methods
├── runtime/      execution / synthesis / control / state
├── formats/      source / driver / chip / platform investigations
├── validation/   observatories / external evidence / controls / pressure tests
└── rendering/    fidelity / equivalence / reconstruction / enhancement
```

A named game gets a project folder only when the game itself is an integrative testbed. Otherwise it remains a corpus or control under the durable question it informs.

## Evidence, policy, and derived state

Keep these ownership classes separate:

```text
source / documentary evidence     tracked when required for reproducibility
research policy / admission data  tracked canonical input
frozen preregistration            tracked experimental contract
reusable result with current use  tracked only when it is itself evidence
analysis cache / feature cache    disposable derived state
score / matrix / projection       regenerated unless explicitly admitted as evidence
```

Generated analysis caches belong under ignored runtime paths such as `research/cache/` or another caller-selected ignored location. A tracked label catalog, admission file, policy, or preregistration is not a cache merely because a tool consumes it.

A research result may support a roadmap change or a durable contract change, but it does not silently become either. Promotion is explicit.

## Routing

| Question | Owner |
| --- | --- |
| general whole-track / soundtrack method research | `music/` |
| composer grammar / attribution method | `music/` |
| Sonic 3 integrative attribution case | `projects/sonic3/` |
| Genesis driver/toolchain source quarry and re-entry | `formats/genesis/genesis-driver-source-ledger.md` |
| source / driver / chip mechanism | `formats/` |
| runtime/state relation | `runtime/` |
| external upstream / specification / implementation evidence | `validation/upstreams.md` |
| external music-representation pressure test | `validation/music-representation-systems.md` |
| OpenMusic-specific pressure evidence | `validation/openmusic-libraries.md` |
| other external method / observatory / control | `validation/` |
| rendering equivalence / reconstruction / enhancement | `rendering/` |

For Genesis source semantics, the source ledger owns provenance and re-entry. Conceptual and experimental claims remain with the format-specific research owner that asks the question. Shared evidence does not require shared ownership.

## Sonic 3 testbed

`projects/sonic3/` owns the integrative Sonic 3 research program spanning attribution, ROM/SMPS evidence, cross-soundtrack controls, and frozen validation generations.

Use the narrowest current route:

```text
projects/sonic3/README.md
→ canonical policy / admission input
→ exact frozen preregistration
→ disposable creator-blind derived state if required
→ execution / result for that generation
```

Keep creative roles distinct:

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

1. Find the durable question that already owns the claim.
2. Distinguish evidence, method contract, canonical policy input, derived object, and bounded result.
3. Treat named games as controls unless the game itself is the integrative research problem.
4. Extend an owner before creating a peer narrative.
5. Keep status and priority out of research prose unless the statement is itself part of a frozen experimental contract.
6. Do not place surveys, literature syntheses, upstream registries, observatory notes, or pressure tests under `docs/` merely because their conclusions are useful.
7. Promote a result into a durable project contract only after a discriminating experiment earns the generalization.

Navigation compression must preserve evidence boundaries:

```text
source semantics != runtime execution != heard result
mechanism evidence != attribution evidence
exact source != derived cache != experiment result
one game observation != cross-system law
```

A good refactor makes research cheaper to re-enter without making claims easier to overstate.