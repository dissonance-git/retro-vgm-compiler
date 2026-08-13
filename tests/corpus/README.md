# Real-music corpus storage

This directory is the permanent repository home for user-supplied real VGM/VGZ/SPC regression fixtures governed by [`tests/CORPUS.md`](../CORPUS.md).

Canonical layout:

```text
tests/corpus/
├── manifest.json
├── archives/
│   ├── Sonic 3 & Knuckles(1).zip
│   └── Front Mission ~ Gun Hazard(1).zip
├── sonic-3-knuckles/
│   └── ... extracted .vgm/.vgz files ...
└── front-mission-gun-hazard/
    └── ... extracted .spc files ...
```

The original ZIP is an immutable evidence object. Keep it byte-for-byte and record its SHA-256 before relying on extracted fixtures. Extracted runnable files are also immutable and individually hashed. Do not retag, normalize, recompress, rename for metadata cleanup, or otherwise rewrite the supplied media bytes.

Use `tools/corpus_import.py` to preserve and extract an archive safely:

```text
python tools/corpus_import.py --archive "Sonic 3 & Knuckles(1).zip" --id sonic-3-knuckles
python tools/corpus_import.py --archive "Front Mission ~ Gun Hazard(1).zip" --id front-mission-gun-hazard
python tools/corpus_import.py --verify
```

The earlier structural pass observed 58 Sonic 3/Sonic & Knuckles VGZ objects and 61 Gun Hazard SPC snapshots. Those counts are useful expectations, not substitutes for hashing the actual supplied bytes during import.

The corpus is intentionally committed to this private repository so future renderer, parser, execution, semantic, attribution-safeguard, and listening passes can exercise real files without depending on chat attachment lifetime.
