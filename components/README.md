# Components

`components/` owns **source-family and device-specific semantics**. It is where native representations are respected before information is lifted into shared musical meaning.

For repository-wide navigation, start at the repository map in [`../README.md`](../README.md).

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

A shared xSF container mechanism does **not** make the platform execution models equivalent.

## Routing rule

```text
native bytes / object
→ matching component family
→ source-specific semantics
→ shared model only where the evidence supports the same musical relation
```

Do not put a chip-, driver-, container-, or platform-specific mechanism into `model/` merely because more than one experiment wants to inspect it.

## New family rule

Before creating another component:

1. Check the root `README.md` repository map and the generated repository inventory.
2. Check whether the family already exists in corpus/audit tooling without yet needing a first-class runtime component.
3. Add a component only when the source family has durable implementation semantics that need an owner.
4. Share abstractions only after materially different families force the same contract.

> Shared abstractions should be discovered by agreement and disagreement.
