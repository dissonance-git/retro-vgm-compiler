# Creator feature cache

This directory is the analysis-ready catalog for creator-attribution research.

## Rule

**Parse a song once. Research it many times.**

VGM/VGZ binaries are ingestion inputs, not the normal query surface. Each song gets one creator-blind JSON capsule under `tracks/<soundtrack-id>/`. Later experiments load those capsules and operate on small feature objects or similarity matrices.

The cache deliberately separates two concerns:

- **features/evidence**: creator-blind material extracted from the music stream;
- **labels**: documentary composer / arranger / programmer evidence stored in separate research policy or catalog files.

Do not put composer names into song capsules. A label is overlaid only after blind extraction when an experiment needs ground truth.

## Current capsule views

A single ingestion operation stores both a reusable event layer and the current derived views:

1. `canonical_events`
   - every accepted ordinary full YM2612 key-on
   - VGM tick, physical channel, FNUM, block
   - patch fingerprints, algorithm, feedback, AMS/FMS, pan
   - this is the future-facing layer: new interval, timing, part, phrase, register, or patch ideas can usually be derived from JSON without reopening the VGM
2. `gen1`
   - pooled relative-semitone interval histogram
   - interval bigrams
   - contour
   - normalized onset-gap rhythm
   - channel usage / density
   - patch and realization summaries
3. `gen2_parts`
   - interval and interval-bigram histograms kept separately for each active YM2612 channel
   - enough to perform permutation-invariant part matching without reparsing audio
4. `gen3_motion_parts`
   - Gen-2 parts with repeated-note `0` intervals removed
   - all bigrams touching `0` removed
   - enough to run the frozen Gen-3 motion matcher directly from cache

The cache stores evidence/features, not creator scores. Scores are cheap derived products and can be regenerated for any reference/query set.

## Commands

Ingest a corpus once:

```bash
python tools/vgm_creator_feature_cache.py \
  tests/corpus/golden-axe-iii-genesis-vgz \
  --soundtrack-id golden-axe-iii-genesis-vgz
```

The default destination is:

```text
research/music/creator-feature-cache/tracks/<soundtrack-id>/
```

Existing song capsules are reused. `--refresh` is intentionally explicit and should be used only when the extractor/schema or source corpus intentionally changes.

Build a creator-blind matrix later without touching a VGM:

```bash
python tools/vgm_creator_cached_similarity.py \
  research/music/creator-feature-cache/tracks/golden-axe-iii-genesis-vgz \
  research/music/creator-feature-cache/tracks/sonic-3d-blast-genesis-vgm \
  --view gen3-motion-parts \
  --json /tmp/matrix.json
```

## Composer catalogs

A composer catalog is a **small label/index overlay** pointing at cached song identities. `catalogs/tatsuyuki-maeda.json`, for example, lists the 26 primary Genesis tracks currently supported by documentary Maeda composer evidence without duplicating their musical data.

That means adding another creator should usually cost only:

1. ingest any songs not already cached;
2. add documentary role labels to a separate catalog;
3. run queries over the existing feature library.

No repeated soundtrack-wide parsing is required.

## Cache invalidation

Routine research does not hash or revalidate every source file. If a source corpus or extractor intentionally changes, refresh the affected capsules once and commit the new cache generation. Otherwise the checked-in cache is the analysis input.
