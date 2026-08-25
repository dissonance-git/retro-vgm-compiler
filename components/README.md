# Components

`components/` owns **source-family, device, decoder, execution, and source-native rendering semantics**. Native representations are respected here before information is lifted into the shared musical model.

For repository-wide orientation see [`../README.md`](../README.md). For semantic boundaries see [`../docs/architecture.md`](../docs/architecture.md). Use `python tools/repository_catalog.py` when exact mechanical inventory is useful.

## Current families

| Path | Owns |
| --- | --- |
| `vgm/` | VGM/VGZ logged execution, device/register state, Genesis and other VGM-facing machinery |
| `spc/` | SPC snapshots, SPC700/S-DSP, BRR/sample state, source-aware SNES reconstruction |
| `psf/` | PSF1 / PlayStation effective objects, AKAO/SPU-facing investigations |
| `gsf/` | GSF / Game Boy Advance effective-object semantics |
| `usf/` | USF / Nintendo 64 effective-object semantics |
| `twosf/` | 2SF / Nintendo DS effective-object semantics |
| `ncsf/` | NCSF / selected-SDAT Nintendo DS semantics |
| `xsf/` | common xSF envelope, dependency, overlay, and provenance behavior only |

A shared xSF container mechanism does **not** make platform execution models equivalent.

## Routing rule

```text
native bytes / executable object
→ matching component family
→ source-specific semantics and gaps
→ shared model only where evidence supports the same musical relation
```

Do not put a chip-, driver-, container-, or platform-specific mechanism into `model/` merely because multiple experiments inspect it.

## New family rule

Before adding another component:

1. Check whether the source family already has an owner here or in source-family audit tooling.
2. Add a first-class component only when durable implementation semantics need a stable owner.
3. Keep format/container commonality separate from platform/runtime equivalence.
4. Promote a shared abstraction only after materially different families force the same contract.

> Shared abstractions should be discovered by agreement and disagreement.
