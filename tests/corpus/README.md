# Real-music corpus storage

This directory is the permanent repository home for user-supplied real VGM/VGZ/SPC regression fixtures governed by [`tests/CORPUS.md`](../CORPUS.md).

Canonical layout:

```text
tests/corpus/
├── manifest.json
├── sonic-3-knuckles.sha256
├── front-mission-gun-hazard.sha256
├── sonic-3-knuckles/
│   └── 58 immutable .vgz files
└── front-mission-gun-hazard/
    └── 61 immutable .spc files
```

The runnable files themselves are the canonical committed evidence objects for these two sets. They are preserved byte-for-byte. Do not retag, normalize, recompress, rename for metadata cleanup, or otherwise rewrite them.

`manifest.json` records corpus-level provenance and whole-set digests. The adjacent `.sha256` inventories record SHA-256 for every runnable file. Each set also records its expected Git tree SHA-1, so verification can detect path changes and byte drift at both the object and directory levels.

Use `tools/corpus_import.py` to verify the committed corpus:

```text
python tools/corpus_import.py --verify
```

To index a future direct-file corpus already placed under `tests/corpus/<id>/`:

```text
python tools/corpus_import.py --record-existing --id <id>
```

Archive import remains supported when an archive itself is intended to be preserved:

```text
python tools/corpus_import.py --archive soundtrack.zip --id <id>
```

An archive is a delivery container, not a mandatory duplicate of a direct-file corpus. If the runnable files are the canonical supplied objects, the repository does not need to retain a ZIP as well.

Current committed pressure surface:

- Sonic 3 & Knuckles: 58 VGZ objects;
- Front Mission: Gun Hazard: 61 SPC snapshots;
- total: 119 immutable real-music fixtures.

The corpus is intentionally committed to this private repository so future renderer, parser, execution, semantic, attribution-safeguard, and listening passes can exercise real files without depending on chat attachment lifetime.
