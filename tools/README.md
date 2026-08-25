# Tools

`tools/` contains reusable repository-facing commands. A tool should expose an operation, not become a second database or a status diary.

For repository-wide navigation start at [`../README.md`](../README.md). For a concept-scoped, context-efficient projection run:

```bash
python tools/repository_catalog.py --focus cadence
python tools/repository_catalog.py --focus cadence --limit 8 --json
```

Use the full mechanical inventory only when repository shape itself is the question:

```bash
python tools/repository_catalog.py
python tools/repository_catalog.py --json
```

The catalog is generated on demand and printed to stdout. It is not committed documentation. Focus mode begins with path/content matches, then expands exact mechanical relations derived from tracked files before applying the context cap.

Current derived relation classes are intentionally small:

```text
C/C++ quoted local include   -> includes / included_by
local Markdown link          -> links_to / linked_from
tracked CMake path           -> registers / registered_by
```

These edges exist only in the projection. Do not maintain a parallel graph file for relations the repository can derive exactly.

A low selection ratio is not success by itself. Repository-representation changes are evaluated by [`../research/validation/repository-representation-benchmark.md`](../research/validation/repository-representation-benchmark.md): compression must preserve or increase required-file/relation recall, verified task success, and validation quality while reducing context or routing cost.

## Tool families

### Repository / corpus maintenance

```text
repository_catalog.py
corpus_import.py
check_project_identity.py
check_libvgm_patches.py
run_core_tests.py
```

### Creator / attribution research

```text
attribution_control_registry.py
blind_attribution_match_manifest.py
creator_blind_song_cache.py
build_admitted_composer_caches.py
cross_soundtrack_vgm_audit.py
maeda_calibration_*.py
vgm_creator_feature_audit.py
```

### Source-family inspection

Prefer the existing family prefix before creating another utility:

```text
vgm_*
genesis_*
spc_*
nsf_*
xsf_*
z80_*
```

## Ownership rule

```text
canonical source / manifest / research contract
        ↓
      tool
        ↓
derived result / validation / projection
```

Do not hide durable attribution labels, corpus provenance, research decisions, or semantic contracts inside a utility when a canonical data/document owner exists.

Add a tool when a repeatable operation lacks a route, a validation should be executable, expensive work can be cached safely, or several research cases need the same bounded operation. Prefer extending an existing tool when input/output semantics and ownership are the same.