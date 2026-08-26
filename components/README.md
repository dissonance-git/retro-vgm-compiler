# Components

`components/` owns **source-family, device, decoder, execution, and source-native rendering semantics**. Native representations are respected here before information is lifted into the shared musical model.

For repository-wide orientation see [`../README.md`](../README.md). For semantic boundaries see [`../docs/architecture.md`](../docs/architecture.md). Use `python tools/repository_catalog.py --focus <family>` when exact mechanical inventory is useful.

## Routing rule

```text
native bytes / executable object
→ matching component family
→ source-specific semantics and gaps
→ shared model only where evidence supports the same musical relation
```

A shared container or transport mechanism does not imply shared platform execution semantics. Do not put a chip-, driver-, container-, or platform-specific mechanism into `model/` merely because multiple experiments inspect it.

## New family rule

Before adding another component:

1. Check whether the source family already has an owner here or in source-family audit tooling.
2. Add a first-class component only when durable implementation semantics need a stable owner.
3. Keep format/container commonality separate from platform/runtime equivalence.
4. Promote a shared abstraction only after materially different families force the same contract.

Directory membership is mechanical inventory and should be discovered from the tree rather than copied into this README.

> Shared abstractions should be discovered by agreement and disagreement.
