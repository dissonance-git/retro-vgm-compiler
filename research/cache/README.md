# Creator-blind song cache

Repeated creator/style research should operate on a reusable song object instead of reopening or re-executing the source for every hypothesis.

The immutable files under `tests/corpus/` remain source authority. Caches are rebuildable research accelerators, not replacements for those files.

## One cache law, source-specific backends

```text
immutable source
      ↓ expensive source-specific lift once
creator-blind song object
      ↓ cheap projections
parts / motifs / phrase models / feature views / similarity experiments

canonical role evidence ───────────────────────────────────────────┘
```

Different source families retain different cache schemas because they expose different evidence. **Song-centered identity is shared; source semantics are not flattened.** Creator, composer, arranger, programmer, artist, cue-candidate, and attribution labels are never stored in a song cache object. Historical role evidence is joined later from its canonical owner.

## Genesis VGM/VGZ backend

`tools/creator_blind_song_cache.py` preserves ordinary YM2612 key-on snapshots, timing, patch state, FM realization fields, raw interval/onset geometry, PSG/stereo writes, DAC observations, and lightweight realization counters.

```text
research/cache/vgm-song-capsules/<corpus-id>/
```

Physical YM2612 channels remain observations, not persistent musical-part identity.

## SPC backend

`tools/spc/creator_blind_spc_cache.py` promotes the existing controlled forensic runtime output into a persistent song-centered object rather than inventing another SPC parser.

```text
SPC snapshot
→ instrumented snes_spc controlled execution
→ runtime trace / replay
→ label-blind voice episodes + persistent-part profiles
→ persistent forensic sidecar
```

The sidecar preserves controlled-execution duration/source size, capture and replay completeness, voice/transition counts, persistent-part profiles, normalized onset geometry, performed-pitch interval geometry when supported, pitch contour, evidence confidence, and extractor/runtime provenance.

Its model remains:

```text
label-blind SPC forensic feature sidecar
```

SPC caches are song-centered **and duration-keyed**, so useful 5-second and 10-second captures coexist rather than evicting one another:

```text
research/cache/spc-song-capsules/<corpus-id>/<seconds>s/<source-file>.json
```

A complete capture that yields zero admissible persistent-part profiles is still cached as a negative result. Normal frozen-matrix admission still rejects such a cue. This prevents “no usable parts found” from becoming forgotten work that gets executed again.

Panel ids such as `cue-001` are experiment projections, not cache identities. `tools/spc/capture_blind_panel.py` now reuses the song cache before producing opaque panel outputs for freezing.

## No hash treadmill

Routine source-cache reuse does not hash every VGM/VGZ/SPC again. Genesis reuse checks cache/extractor generation and source size. SPC reuse checks expected forensic model, source size, capture duration, and the existing creator-blind integrity contract.

Use `--refresh` / `--refresh-cache` when source bytes or extractor/runtime semantics intentionally change. Exact source identity remains owned by corpus manifests and Git history. Frozen experiments may hash their small derived sidecars for immutable experiment identity; that is separate from routine source-cache reuse.

## Canonical composer-control world

The Sonic 3 calibration world joins two owners at runtime:

```text
research/projects/sonic3/role-credit-index.jsonl
    51 Genesis VGM/VGZ cache-routing controls

research/projects/sonic3/attribution-control-admissions.jsonl
    grounded cross-format role evidence, including 15 CUBE SPC controls
```

The combined admitted composer view currently contains **66 controls across nine composers**:

- Tatsuyuki Maeda: 26
- Jun Senoue: 12
- Miyoko Takaoka: 11
- Haruyo Oguro: 5
- Masanori Hikichi: 4
- Naofumi Hataya: 4
- Tomonori Sawada: 2
- Masaru Setsumaru: 1
- Seirou Okamoto: 1

CUBE controls remain owned by `attribution-control-admissions.jsonl` and `cube-calibration-policy.json`; they are not copied into the Genesis routing index. `The Scorching Sand` remains operationally admitted as a Tomonori Sawada Genesis composition control, with the conflicting later recollection retained as counterevidence.

## Build controls

Without an SPC executable:

```bash
python tools/build_admitted_composer_caches.py
```

The 15 grounded SPC controls are reported as `backend_unavailable_tracks`, not mislabeled as unsupported and never fed to the VGM parser.

Build the existing forensic runtime:

```bash
cmake -S tools/spc/forensic -B build-spc-forensic -DCMAKE_BUILD_TYPE=Release
cmake --build build-spc-forensic --parallel 2
```

Then build the full 66-control cached world:

```bash
python tools/build_admitted_composer_caches.py \
  --spc-extractor build-spc-forensic/spc_forensic_features
```

The default SPC duration is 5 seconds, matching the frozen CUBE blind-panel contract. `--spc-seconds` selects another persistent duration-specific cache object.

Whole soundtrack controls can also be pre-cached:

```bash
python tools/creator_blind_song_cache.py build-corpus \
  tests/corpus/golden-axe-iii-genesis-vgz

python tools/spc/creator_blind_spc_cache.py build-corpus \
  tests/corpus/terranigma-spc \
  --extractor build-spc-forensic/spc_forensic_features
```

## Research rule

Pairwise matrices, rankings, attribution scores, and frozen panel geometry are experiment views, not the reusable song cache. The reusable object sits earlier in the pipeline so future research generations can change musical projections without reopening or re-executing the binary corpus.
