# Shared musical model

`model/` owns **source-independent musical and evidential contracts that have earned sharing across materially different source families**.

Start with [`../docs/architecture.md`](../docs/architecture.md) for common semantic/evidence law. Read [`../docs/vgm-compiler-roadmap.md`](../docs/vgm-compiler-roadmap.md) only when the task depends on current implementation status or priority. Use `python tools/repository_catalog.py --focus <concept>` when exact mechanical routing is useful.

## Semantic ladder

```text
provenance-aware observations
→ source / authored / driver ancestry and explicit gaps
→ pitch / timing / performance evidence
→ voice episodes and persistent parts
→ motifs and transformations
→ phrases and phrase relations
→ harmony / bass interaction / voice leading / counterpoint
→ cadence / sections / form
→ arrangement / orchestration / texture
→ creator-blind recurring grammar
→ role-aware attribution hypotheses
→ human-facing musical explanation
```

Higher layers may summarize lower evidence but may not erase uncertainty or provenance.

## Ownership boundary

`model/` is not a dumping ground for code used by two callers.

```text
shared implementation convenience
!= shared semantic law
```

BRR reconstruction, FM topology, driver allocation, xSF platform execution, and other source-native mechanisms remain in their owning components until materially different source families require the same abstraction without information loss.

The shared `execution_semantic_provenance` contract provides a common evidential shape for facts independent source families can establish: source/authored/driver ancestry, native-token preservation, documentary annotations, hypotheses, and explicit missing-information gaps. It does not make source-native driver grammars generic.

Choose the semantic layer first, then search within `model/`. Repository-wide search is the fallback, not the entry point.
