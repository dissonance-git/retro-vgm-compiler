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

Different source families retain different cache schemas because they expose different evidence. **Song-centered identity is shared; source semantics are not flattened.**

Creator, composer, arranger, programmer, artist, cue-candidate, and attribution labels are never stored in a song cache object. Historical role evidence is joined later from its canonical owner.

## Genesis VGM/VGZ backend

`tools/creator_blind_song_cache.py` preserves:

- ordinary full YM2612 key-on snapshots;
- event tick, physical channel, F-number, block, algorithm, feedback, AMS/FMS, and pan;
- dictionary-encoded core/full patch state observed at each key-on;
- per-channel event membership, raw relative-semitone interval tokens, and raw onset gaps;
- PSG writes and Game Gear stereo writes;
- DAC stream command observations;
- lightweight realization counters and VGM timing.

Physical YM2612 channels remain observations, not persistent musical-part identity.

Cache location:

```text
research/cache/vgm-song-capsules/<corpus-id>/
```

## SPC backend

`tools/spc/creator_blind_spc_cache.py` promotes the existing controlled forensic runtime output into a persistent song-centered object rather than inventing another SPC parser.

```text
SPC snapshot
→ instrumented snes_spc controlled execution
→ runtime trace / replay
→ label-blind voice episodes + persistent-part profiles
→ persistent forensic sidecar
```

The cached sidecar already contains:

- controlled execution duration and source byte count;
- runtime capture completeness diagnostics;
- replay continuity diagnostics;
- voice-episode and transition counts;
- persistent-part profile count;
- normalized inter-onset geometry;
- performed-pitch interval geometry where supported;
- pitch contour and evidence confidence;
- exact runtime/instrumentation provenance emitted by the extractor.

Its model remains:

```text
label-blind SPC forensic feature sidecar
```

Cache location:

```text
research/cache/spc-song-capsules/<corpus-id>/
```

Panel ids such as `cue-001` are experiment projections. They do not become cache identities. `tools/spc/capture_blind_panel.py` now reuses the song cache and only then copies the creator-blind sidecar into an opaque panel output for freezing.

## No hash treadmill

Routine source-cache reuse does not hash every VGM/VGZ/SPC again.

Genesis reuse checks the cache/extractor generation and source size. SPC reuse checks:

- expected label-blind forensic model;
- source size recorded by controlled execution;
- requested capture duration;
- existing freezer integrity/label-leakage contract.

Use `--refresh` / `--refresh-cache` when source bytes or extractor/runtime semantics intentionally change. Exact source identity remains owned by corpus manifests and Git history.

Frozen experiment matrices may hash their small derived sidecars for immutable experiment identity. That is separate from routine source-cache reuse.

## Canonical composer-control world

The current Sonic 3 calibration world joins two existing owners at runtime:

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

The CUBE controls remain owned by `attribution-control-admissions.jsonl` and `cube-calibration-policy.json`. They are not copied into the Genesis routing index.

`The Scorching Sand` remains operationally admitted as a Tomonori Sawada Genesis composition control per the project decision; its conflicting later recollection remains attached as counterevidence.

## Build controls

Genesis controls only:

```bash
python tools/build_admitted_composer_caches.py
```

That reports the grounded SPC controls as `backend_unavailable_tracks` rather than pretending they are unsupported or feeding them into a VGM parser.

After building the existing forensic SPC runtime:

```bash
cmake -S tools/spc/forensic -B build-spc-forensic -DCMAKE_BUILD_TYPE=Release
cmake --build build-spc-forensic --parallel 2
```

build the full 66-control cached world with:

```bash
python tools/build_admitted_composer_caches.py \
  --spc-extractor build-spc-forensic/spc_forensic_features
```

The default SPC controlled-execution duration is 5 seconds, matching the frozen CUBE blind-panel contract. Change it explicitly with `--spc-seconds`; a different duration is a different reusable capture and invalidates routine reuse.

To pre-cache a whole source-family control soundtrack, including negatives:

```bash
python tools/creator_blind_song_cache.py build-corpus \
  tests/corpus/golden-axe-iii-genesis-vgz

python tools/spc/creator_blind_spc_cache.py build-corpus \
  tests/corpus/terranigma-spc \
  --extractor build-spc-forensic/spc_forensic_features
```

## Research rule

Pairwise matrices, rankings, attribution scores, and frozen panel geometry are derived experiment views. They may be committed as evidence for a specific experiment, but they are not the reusable song cache because changing the comparison method would make them obsolete.

The reusable cache sits earlier in the pipeline so future research generations can change musical projections without reopening or re-executing the binary corpus.
