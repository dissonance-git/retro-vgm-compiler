# Creator-blind VGM song cache

This cache makes repeated creator/style research operate on a parsed song object instead of reopening the VGM/VGZ for every hypothesis.

The immutable files under `tests/corpus/` remain source authority. Capsules are rebuildable research accelerators, not replacements for those files.

## Ownership boundary

```text
immutable VGM/VGZ source
        ↓ parse once
creator-blind song capsule
        ↓ cheap projections
feature views / part matchers / phrase models / similarity matrices

role-credit index ───────────────────────────────┘
```

Creator, composer, arranger, programmer, and artist labels are **never stored in a song capsule**. Role evidence lives separately. This keeps the same cached song usable for blind experiments and for different historical-role questions.

`tools/creator_blind_song_cache.py` currently preserves, for Genesis VGM/VGZ:

- ordinary full YM2612 key-on snapshots;
- event tick, physical channel, F-number, block, algorithm, feedback, AMS/FMS, and pan;
- dictionary-encoded core/full patch state observed at each key-on;
- per-channel event membership, raw relative-semitone interval tokens, and raw onset gaps;
- PSG writes and Game Gear stereo writes;
- DAC stream command observations;
- lightweight realization counters and VGM timing.

Physical YM2612 channels remain observations, not persistent musical-part identity. Future part recovery, phrase segmentation, rhythm normalization, motion filtering, realization models, and similarity functions should derive from capsules instead of reparsing source audio.

## No hash treadmill

Routine cache reuse does not hash every file. A capsule is reused when its schema/extractor generation and recorded source size still match. Use `--refresh` when source bytes or extractor semantics intentionally change. Exact corpus identity remains owned by the existing corpus manifests and Git history.

## Build a creator's admitted songs once

The Sonic 3 research role index is:

```text
research/projects/sonic3/role-credit-index.jsonl
```

For example:

```bash
python tools/creator_blind_song_cache.py build-creator \
  --credits research/projects/sonic3/role-credit-index.jsonl \
  --creator "Tatsuyuki Maeda" \
  --role composer
```

That selects the currently admitted Maeda composition controls and writes one capsule per song under:

```text
research/cache/vgm-song-capsules/<corpus-id>/
```

A second identical invocation reuses the capsules without parsing the VGM/VGZ again.

## Build every admitted composer

The role index now contains 50 admitted composition controls across seven creators:

- Tatsuyuki Maeda: 26
- Jun Senoue: 12
- Haruyo Oguro: 5
- Naofumi Hataya: 4
- Tomonori Sawada: 1
- Masaru Setsumaru: 1
- Seirou Okamoto: 1

`The Scorching Sand` remains excluded because its Golden Axe III soundtrack assignment conflicts with Tomonori Sawada's later creator statement.

Build the entire admitted composer world with one command:

```bash
python tools/build_admitted_composer_caches.py
```

The helper fans `build-creator` across every admitted composer. Because capsules are keyed by source song rather than creator, repeated creator/role views reuse the same parsed object instead of duplicating it.

To pre-cache a whole control soundtrack, including negatives:

```bash
python tools/creator_blind_song_cache.py build-corpus \
  tests/corpus/golden-axe-iii-genesis-vgz
```

To inspect role evidence without touching source music:

```bash
python tools/creator_blind_song_cache.py lookup \
  --credits research/projects/sonic3/role-credit-index.jsonl \
  --creator "Tatsuyuki Maeda" \
  --role composer
```

## Research rule

Pairwise matrices are derived views. They may be committed as frozen experiment evidence, but they are not the reusable cache because changing the similarity function would make them obsolete. Song capsules sit earlier in the pipeline so a new research generation can recompute cheap derived features without reopening the binary corpus.
