# Creator feature cache

This directory is the analysis-ready catalog for creator-attribution research.

## Rule

**Parse a song once. Research it many times.**

VGM/VGZ binaries are ingestion inputs, not the normal query surface. Each song gets one creator-blind JSON capsule under `tracks/<soundtrack-id>/`. Later experiments load those capsules and operate on small feature objects or similarity matrices.

The cache deliberately separates two concerns:

- **features**: creator-blind evidence extracted from the music stream;
- **labels**: documentary composer / arranger / programmer evidence stored in separate research policy or index files.

Do not put composer names into song capsules. A label should be overlaid only after blind extraction when an experiment needs ground truth.

## Current capsule views

A single ingestion pass stores all currently useful views:

1. `gen1`
   - pooled relative-semitone interval histogram
   - interval bigrams
   - contour
   - normalized onset-gap rhythm
   - channel usage / density
   - patch and realization summaries
2. `gen2_parts`
   - the interval and interval-bigram histograms kept separately for each active YM2612 channel
   - enough to perform permutation-invariant part matching without reparsing audio
3. `gen3_motion_parts`
   - Gen-2 parts with repeated-note `0` intervals removed
   - all bigrams touching `0` removed
   - enough to run the frozen Gen-3 motion matcher directly from cache

The cache stores features, not scores. Scores are cheap derived products and can be regenerated for any reference/query set.

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

Existing song capsules are reused. `--refresh` is intentionally explicit and should be used only when the extractor/schema changes.

Build a creator-blind matrix later without touching a VGM:

```bash
python tools/vgm_creator_cached_similarity.py \
  research/music/creator-feature-cache/tracks/golden-axe-iii-genesis-vgz \
  research/music/creator-feature-cache/tracks/sonic-3d-blast-genesis-vgm \
  --view gen3-motion-parts \
  --json /tmp/matrix.json
```

## Composer catalogs

A composer catalog should be a **small label/index overlay** pointing at cached song identities. For example, a Tatsuyuki Maeda catalog can reference all documentary Maeda-positive tracks plus explicit non-Maeda controls without duplicating any musical features.

That means adding another creator should usually cost only:

1. ingest any songs not already cached;
2. add documentary role labels to a separate index;
3. run queries over the existing feature library.

No repeated soundtrack-wide parsing is required.

## Cache invalidation

Routine research does not hash or revalidate every source file. If a source corpus or extractor intentionally changes, refresh the affected capsules once and commit the new cache generation. Otherwise the checked-in cache is the analysis input.
