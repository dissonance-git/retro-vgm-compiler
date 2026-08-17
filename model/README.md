# Shared musical model

`model/` owns **source-independent musical and evidential contracts that have earned sharing across materially different source families**.

For repository-wide navigation, start at [`../CATALOG.md`](../CATALOG.md).

## Semantic ladder

The shared model is organized conceptually upward:

```text
provenance-aware observations
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

Individual headers implement bounded pieces of this ladder. The generated repository catalog enumerates exact filenames.

## Important distinction

`model/` is not a dumping ground for code used by two callers.

```text
shared implementation convenience
!= shared semantic law
```

BRR reconstruction, FM topology, driver allocation, xSF platform execution, and other source-native mechanisms remain in their owning components until independent source families genuinely require the same abstraction without erasing useful information.

## Navigation hints

Common filename families are descriptive:

```text
*pitch*               pitch evidence and bridges
*persistent_part*     identity above physical channels
*motif* / *phrase*    recurring material and form building blocks
*harmon* / *chord*    vertical and harmonic relations
*bass* / *voice*      cross-part relations
*cadential* / *section* formal evidence
*composer_grammar*    creator-blind recurring grammar evidence
*attribution*         role-aware evaluation/hypothesis contracts
*discourse*           human-readable projections over evidence
```

Search within `model/` after choosing the semantic layer. Avoid repository-wide search unless the catalog route fails.
