# Tools

`tools/` contains reusable repository-facing commands. Before writing another script, check this shelf and the generated inventory from `repository_catalog.py`.

For repository-wide navigation, start at [`../CATALOG.md`](../CATALOG.md).

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

The exact current filenames are enumerated by:

```text
python tools/repository_catalog.py
```

## Ownership rule

A tool should be a **thin executable route to an existing owner**, not a second home for project state.

```text
canonical source / manifest / research contract
        ↓
      tool
        ↓
derived result / validation / projection
```

Do not hide durable attribution labels, corpus provenance, research decisions, or semantic contracts inside a utility when a canonical data/document owner exists.

## Addition rule

Add a new tool only when at least one is true:

- a repeatable operation has no existing command;
- a validation should be executable rather than prose-only;
- expensive parsing can be converted into reusable cached work;
- several research cases need the same bounded operation.

Prefer extending an existing tool when the new behavior shares the same input/output contract and owner.
