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

## Evidence, policy, and derived state

Keep these classes separate:

```text
source / documentary evidence     tracked when required for reproducibility
research policy / admission data  tracked canonical input
frozen preregistration            tracked experimental contract
frozen result used as evidence    tracked with provenance and stated reason
analysis / feature cache          disposable derived state
score / matrix / projection       regenerated unless explicitly promoted
```

Generated caches belong under ignored runtime paths such as `research/cache/` or another caller-selected ignored location. Scores, matrices, projections, reports, and feature capsules are not repository truth merely because they are expensive to compute.

Before committing a generated object, its owning experiment must state why regeneration is insufficient and what evidential obligation requires the exact bytes to remain tracked. Without that promotion record, keep it disposable.

A filename such as `matrix`, `validation`, `calibration`, or `result` is not enough to classify an object. Frozen panels, preregistrations, admissions, policies, and result records that an experiment explicitly treats as evidence remain tracked. Delete only derived state whose owner identifies it as reproducible intermediate output.

A research result may support a roadmap or durable-contract change, but promotion is explicit.

## Routing

Use the durable question as the route:

- general musical/creator methods → `music/`
- named integrative Sonic 3 program → `projects/sonic3/`
- source/driver/chip investigations → `formats/`
- runtime/state investigations → `runtime/`
- upstream/specification/representation observatories and controls → `validation/`
- rendering/equivalence/reconstruction research → `rendering/`

External evidence registries explain why a source matters. Exact mutable build pins belong to the operational owner that verifies them, while immutable imported-package identities belong to `imports/MANIFEST.md`.

## Placement law

Before adding research material:

1. Find the durable question that already owns the claim.
2. Distinguish evidence, method contract, canonical policy input, derived object, and bounded result.
3. Treat named games as controls unless the game itself is the integrative research problem.
4. Extend an owner before creating a peer narrative.
5. Keep current project status out of research prose unless it is part of a frozen experiment.
6. Keep surveys, literature syntheses, upstream registries, observatories, and pressure tests in `research/`, not `docs/`.
7. Promote a result into durable project law only after a discriminating experiment earns the generalization.
8. Do not track a mechanically derivable inventory or generated analysis object when the owner can reproduce it exactly.

Navigation compression must preserve evidence boundaries:

```text
source semantics != runtime execution != heard result
mechanism evidence != attribution evidence
exact source != derived cache != experiment result
one game observation != cross-system law
```

A good refactor makes research cheaper to re-enter without making claims easier to overstate.
