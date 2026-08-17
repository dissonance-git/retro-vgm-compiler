# Retro VGM Compiler catalog

Fast repository orientation. **This is a map, not source truth.**

> One canonical home per object. Many routes to it.

Use this file to answer **“do we already have this, and where does it live?”** before a repository-wide search.

## Entry

```text
current main
→ README.md                         project identity
→ AGENTS.md                         operating law
→ CATALOG.md                        this map
→ docs/retro-vgm-compiler-roadmap.md
→ smallest canonical owner below
→ recent commits for that surface
```

Do not reread or broadly search the whole repository for ordinary work.

## Shelves

```text
model/          shared musical/evidence semantics that earned sharing
components/     source-family and device-specific machinery
tests/          regressions + immutable real corpus
research/       bounded investigations and named testbeds
docs/           current architecture / reasoning contracts
tools/          reusable executable operations
imports/        preserved upstream/import provenance
patches/        maintained patch material
```

Dense shelves have their own `README.md`. Descend there before searching globally.

## Canonical routes

| Need | Route |
| --- | --- |
| project objective | `README.md` |
| working rules | `AGENTS.md` |
| current frontier | `docs/retro-vgm-compiler-roadmap.md` |
| implemented source family | `components/README.md` → family |
| shared musical semantics | `model/README.md` → exact model header/test |
| existing corpus / soundtrack | `tests/corpus/README.md` → `manifest.json` → set |
| research program | `research/README.md` → owning trunk/project |
| existing utility | `tools/README.md` → exact tool |
| current documentation | `docs/README.md` |
| historical project prose | `docs/history/` |
| mechanical repository inventory | `tools/repository_catalog.py` |

## Source families

First-class components currently exist for:

```text
VGM/VGZ • SPC • PSF1 • GSF • USF • 2SF • NCSF • shared xSF envelope semantics
```

A corpus family is not automatically an implemented semantic frontend. Shared xSF packaging does not make platform execution models equivalent.

## Research geography

```text
research/
├── projects/     integrative named testbeds
├── music/        musical understanding / creator grammar / attribution methods
├── runtime/      execution / synthesis / state
├── formats/      source / driver / chip / platform investigations
├── validation/   controls / observatories / pressure tests
├── rendering/    fidelity / reconstruction / equivalence
├── enhancement/  bounded enhanced-rendering experiments
└── cache/        reusable derived analysis, never source truth
```

## Sonic 3 route

```text
research/projects/sonic3/README.md
→ canonical policy / admission file for the question
→ frozen preregistration for the exact generation
→ creator-blind cache only when reusable parsed analysis is needed
→ execution/ result for that generation
```

Important distinctions:

```text
attribution-control-admissions.jsonl   grounded role evidence
cube-calibration-policy.json           CUBE calibration rules / holdouts
role-credit-index.jsonl                Genesis cache-routing controls, not master history
research/cache/                        derived creator-blind song analysis
tools/build_admitted_composer_caches.py runtime join + backend routing
```

Never reconstruct creator labels from corpus tags when a canonical admission/policy already exists.

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver programming
!= patch / sample design
!= final realization
```

## Generated inventory

For exact mechanical enumeration:

```text
python tools/repository_catalog.py

→ docs/generated/repository-catalog.md
→ docs/generated/repository-catalog.json
```

Generated projections never become writable truth and intentionally do not hash the repository again.

## Addition law

```text
canonical owner exists? → extend it
otherwise               → choose the smallest existing shelf
new peer object          → only when a real new distinction needs an owner
```

**A successful refactor leaves fewer places an agent must search.**
