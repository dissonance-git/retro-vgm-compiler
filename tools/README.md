# Tools

`tools/` contains reusable repository-facing commands. A tool exposes an operation. It does not become a second database, manual registry, or status diary.

For repository-wide navigation start at [`../README.md`](../README.md). For a concept-scoped projection run:

```bash
python tools/repository_catalog.py --focus cadence
python tools/repository_catalog.py --focus cadence --limit 8 --json
```

Use the full mechanical inventory only when repository shape itself is the question:

```bash
python tools/repository_catalog.py
python tools/repository_catalog.py --json
```

The catalog is generated on demand from tracked files. It is not committed documentation. Current derived relations include local C/C++ includes, local Markdown links, and tracked CMake paths.

Do not maintain filename inventories or parallel relation graphs that the repository can derive exactly. A low selection ratio is not success by itself; repository-representation changes are evaluated by [`../research/validation/repository-representation-benchmark.md`](../research/validation/repository-representation-benchmark.md).

## Ownership rule

```text
canonical source / manifest / research contract
        ↓
      tool
        ↓
derived result / validation / projection
```

Do not hide durable attribution labels, corpus provenance, research decisions, or semantic contracts inside a utility when a canonical owner exists.

Add or extend a tool when a repeatable operation lacks a route, a validation should be executable, expensive work can be cached safely, or several bounded research cases need the same operation. Prefer extending an existing operation when input/output semantics and ownership are the same.
