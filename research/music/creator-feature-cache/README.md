# Creator feature extraction

This directory owns the tracked inputs that make creator-attribution feature extraction interpretable and reproducible. Generated feature capsules are reusable local acceleration, not repository truth.

## Ownership

```text
creator-feature-cache/
├── README.md       extraction / cache contract
└── catalogs/       tracked documentary label and admission data

research/cache/creator-feature-cache/
└── tracks/         ignored generated feature capsules
```

The naming retains “feature cache” because the tooling exposes a reusable cache, but the tracked directory contains only the contract and canonical catalog inputs. Generated cache bytes live under the repository-wide ignored `research/cache/` owner.

## Rule

**Parse a song once when useful. Research the resulting view many times. Regenerate it when needed.**

VGM/VGZ binaries remain the source evidence. A generated capsule is a creator-blind projection that reduces repeated parsing cost. It must never outrank the source or silently become a second canonical music database.

Documentary labels remain separate from extracted music features. Do not put composer or artist names into song capsules. Overlay catalog labels only after blind extraction when an experiment needs ground truth.

## Capsule views

The current extractor can cache:

- `canonical_events`: accepted ordinary full YM2612 key-ons with timing, physical channel, FNUM/block, patch fingerprints, algorithm, feedback, AMS/FMS, and pan;
- `gen1`: pooled relative-semitone interval, interval-bigram, contour, rhythm, channel-density, patch, and realization summaries;
- `gen2_parts`: interval and interval-bigram views per active YM2612 physical channel;
- `gen3_motion_parts`: the Gen-2 view with repeated-note zero intervals and zero-touching bigrams removed.

These are analysis projections. `canonical_events` is deliberately closer to observed execution than the higher feature views, but it is still a generated cache of source evidence rather than authored notation or persistent-part proof.

Scores and similarity matrices are cheaper derived products and should normally be regenerated rather than tracked.

## Generate

```bash
python tools/vgm_creator_feature_cache.py \
  tests/corpus/golden-axe-iii-genesis-vgz \
  --soundtrack-id golden-axe-iii-genesis-vgz
```

Default output:

```text
research/cache/creator-feature-cache/tracks/<soundtrack-id>/
```

Existing local capsules are reused. `--refresh` explicitly recomputes them after a source, extractor, or schema change. `--out` may point elsewhere when a bounded experiment needs its own disposable workspace.

A later similarity query can operate entirely on the generated projection:

```bash
python tools/vgm_creator_cached_similarity.py \
  research/cache/creator-feature-cache/tracks/golden-axe-iii-genesis-vgz \
  research/cache/creator-feature-cache/tracks/sonic-3d-blast-genesis-vgm \
  --view gen3-motion-parts \
  --json /tmp/matrix.json
```

## Catalogs

`catalogs/` is different from the cache. It contains tracked documentary label/admission data that a research protocol may treat as canonical input. For example, `catalogs/tatsuyuki-maeda.json` identifies tracks supported by documentary role evidence without duplicating their musical bytes.

Adding another creator should normally require only:

1. source/corpus evidence already admitted by the relevant research owner;
2. documentary labels in a tracked catalog or policy input;
3. generated creator-blind capsules when computational reuse is worthwhile;
4. a bounded experiment over those inputs.

## Promotion boundary

Do not commit routine cache refreshes. If a particular generated object becomes necessary evidence for a frozen experiment, promote it deliberately into that experiment's evidence owner with provenance and an explicit reason it must be retained. Promotion is an evidence decision, not a cache policy.
