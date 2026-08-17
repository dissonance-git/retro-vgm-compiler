# Generated projections

This directory is for deterministic, rebuildable navigation or analysis projections.

Generated files are **not canonical writable truth**. Their owners are the source tree, manifests, research records, and exact implementation they summarize.

Repository inventory is generated with:

```text
python tools/repository_catalog.py
```

which writes:

```text
docs/generated/repository-catalog.md
docs/generated/repository-catalog.json
```

Use `--check` when a checkout already commits those projections and you want to detect staleness.

Do not place hand-maintained project decisions, provenance, role credits, or research conclusions here merely because they are convenient to query.
