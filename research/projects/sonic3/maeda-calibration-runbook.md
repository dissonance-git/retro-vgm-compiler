# Tatsuyuki Maeda blind calibration runbook

This is the preregistered execution path for testing whether the current VGM feature model can rediscover known Tatsuyuki Maeda controls before any unresolved Sonic 3 cue is allowed into the experiment.

## Claim boundary

This experiment calibrates a feature model. It does **not** assign Sonic 3 authorship.

The evidence lanes remain separate:

- `structural` is the preregistered composition-facing aggregate inferred from the current physical-channel YM2612 musical trajectory model.
- `structural_pitch` isolates interval, interval-bigram, and contour evidence.
- `structural_rhythm` isolates tempo-normalized onset-gap evidence.
- the two structural subviews are diagnostics of the same underlying key-on evidence, not independent corroboration.
- `realization` is arrangement / sound-data / driver / patch / routing-facing evidence.
- success in `realization` alone must never be translated into a composition credit.

The held-out target is `tests/corpus/sonic-3-knuckles`. It is forbidden from the calibration audit.

## Frozen control worlds

Use only these Genesis worlds in the first executable calibration:

1. `tests/corpus/golden-axe-iii-genesis-vgz`
   - same-game positive and negative controls
   - 10 Maeda controls
   - Hataya, Oguro, and uncontested Sawada tracks are non-Maeda controls
   - `11 - The Scorching Sand.vgz` is quarantined and excluded from scoring
2. `tests/corpus/sonic-3d-blast-genesis-vgm`
   - complete 24-track per-track role map
   - composition: Maeda 10, Senoue 12, Setsumaru 1, Okamoto 1
   - arrangement/programming: Maeda 10, Senoue 10, Setsumaru 4
   - only Maeda and Senoue are recurring composition classes; Setsumaru and Okamoto are singleton sentinels
   - same zone/work/source-lineage family candidates are excluded from role-specificity retrieval
   - two corpus-tail role mappings are `derived_audio_correspondence` and must be removed in a mandatory sensitivity replication
3. `tests/corpus/j-league-pro-striker-2-vgz`
   - six whole-soundtrack Maeda-positive controls
   - useful only in cross-soundtrack retrieval because this world contains no internal non-Maeda controls

`tests/corpus/super-columns-vgm` is deliberately excluded. The current extractor is Genesis/YM2612-centric; Super Columns becomes a cross-platform stress test only after a platform-neutral musical representation exists.

## Stage A: creator-blind extraction

Run this before loading `maeda-calibration-policy.json` into any scoring process:

```bash
python tools/cross_soundtrack_vgm_audit.py \
  tests/corpus/golden-axe-iii-genesis-vgz \
  tests/corpus/sonic-3d-blast-genesis-vgm \
  tests/corpus/j-league-pro-striker-2-vgz \
  --include-within-soundtrack \
  --neighbors 0 \
  --json research/projects/sonic3/maeda-calibration-frozen-audit.json
```

The output must declare:

```text
model = blind cross-soundtrack Genesis VGM creator audit
label_policy contains: No composer/artist metadata
```

No composer labels are permitted in this file. Do not include the Sonic 3 corpus.

Freeze an integrity digest before unblinding:

```bash
sha256sum research/projects/sonic3/maeda-calibration-frozen-audit.json
```

Record the digest with the result artifact or commit message. If the audit changes, it is a new experiment.

## Stage B: documentary Maeda unblinding

Only after Stage A is frozen:

```bash
python tools/maeda_calibration_eval.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --k 3 \
  --json research/projects/sonic3/maeda-calibration-results.json
```

The evaluator must fail closed if the input is not the creator-blind audit contract, the held-out Sonic 3 target appears, the Sonic 3D Blast partition is incomplete, or any required scored control is missing.

## Stage C: count-matched Maeda-label null

```bash
python tools/maeda_calibration_null.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --k 3 \
  --permutations 5000 \
  --seed 20260816 \
  --json research/projects/sonic3/maeda-calibration-null.json
```

The null preserves the observed Maeda count separately inside Golden Axe III and Sonic 3D Blast. J.League Pro Striker 2 remains fixed as a whole-soundtrack Maeda-positive world. The disputed Scorching Sand track remains excluded.

All empirical p-values use the plus-one correction:

```text
(1 + number of null scores >= observed score) / (1 + permutations)
```

## Stage D: Sonic 3D Blast role specificity with anti-sibling exclusion

Use the same frozen Stage A audit. This asks whether recurring creators can be distinguished inside one soundtrack after related-work shortcuts are removed.

```bash
python tools/sonic3d_role_specificity_eval.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --families research/projects/sonic3/sonic3d-role-family-policy.json \
  --json research/projects/sonic3/sonic3d-role-specificity-results.json
```

The family policy is mandatory. Every query excludes candidates from the same zone/work/source-lineage family. Act 1 cannot retrieve Act 2 of the same zone as a creator win, and `Robotnik 2` cannot retrieve the related `Robotnik 3` / unused-1D Boss-2-family variant as a creator win.

### Composition lane

The recurring classes are Maeda 10 and Senoue 12. Setsumaru and Okamoto are singleton sentinels: they may steal a top-1 retrieval and count as an error, but they must never be treated as learned creator clusters.

`structural` is primary. `structural_pitch` and `structural_rhythm` are diagnostic subviews only.

Primary statistics are balanced top-1 accuracy, Maeda recall, Senoue recall, mean reciprocal rank, mean best-same-minus-best-other margin, singleton top-1 intrusions, and number of same-family candidate instances excluded.

### Arrangement/programming lane

`realization` uses recurring role classes Maeda 10, Senoue 10, Setsumaru 4. This lane can calibrate implementation fingerprints only and is forbidden from satisfying a composition gate.

## Stage E: anti-sibling role-label permutation null

```bash
python tools/sonic3d_role_specificity_null.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --families research/projects/sonic3/sonic3d-role-family-policy.json \
  --permutations 5000 \
  --seed 20260816 \
  --json research/projects/sonic3/sonic3d-role-specificity-null.json
```

The null keeps feature vectors and every `family_id` fixed while permuting role labels across tracks. It preserves the exact observed label multiset separately for composition and arrangement/programming, including singleton treatment.

The primary composition null statistics are balanced top-1 accuracy, minimum recall across learnable classes, mean reciprocal rank, and mean best-same-minus-best-other margin. Pitch, rhythm, and realization nulls are diagnostics, not alternative composition pass routes.

## Stage F: direct-mapping-only replication

Two committed Sonic 3D corpus-tail mappings are explicitly derived rather than direct title/role matches:

- `19 - Special Stage 2.vgm` -> `Sonic 3 Bonus Stage`
- `22 - Robotnik 3.vgm` -> `unused 1D`

A valid creator signal must survive removing both. The complete 24-track audit and complete role/family maps are still validated first; only then are `derived_audio_correspondence` rows excluded.

Observed 22-track sensitivity panel:

```bash
python tools/sonic3d_role_specificity_eval.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --families research/projects/sonic3/sonic3d-role-family-policy.json \
  --exclude-mapping-state derived_audio_correspondence \
  --json research/projects/sonic3/sonic3d-role-specificity-direct-map.json
```

Matched 22-track permutation null:

```bash
python tools/sonic3d_role_specificity_null.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --families research/projects/sonic3/sonic3d-role-family-policy.json \
  --exclude-mapping-state derived_audio_correspondence \
  --permutations 5000 \
  --seed 20260816 \
  --json research/projects/sonic3/sonic3d-role-specificity-direct-map-null.json
```

The selected-panel counts must become:

- composition: Maeda 10, Senoue 10, Setsumaru 1, Okamoto 1
- arrangement/programming: Maeda 10, Senoue 9, Setsumaru 3

This is a sensitivity replication of the same hypothesis, not an independent evidence multiplier.

## Primary Maeda readouts

For each of `structural`, `structural_pitch`, `structural_rhythm`, and `realization`, inspect Golden Axe III within-soundtrack retrieval, Sonic 3D Blast within-soundtrack retrieval, Genesis cross-soundtrack retrieval, and cross-soundtrack retrieval split by held-out query world.

Primary statistics are `precision_at_k`, `chance_precision_at_k`, `precision_lift_over_chance`, `mean_reciprocal_rank`, `mean_best_positive_minus_best_negative`, and empirical permutation p-values.

Raw precision is never sufficient. The `structural` aggregate remains the preregistered promotion statistic. Pitch and rhythm explain why it succeeds or fails; they do not multiply the evidence count.

## Preregistered calibration gate

Do **not** expose unresolved Sonic 3 cues to Maeda scoring unless every primary structural requirement below is satisfied.

### Maeda-vs-rest control gate

- positive precision lift over chance in Golden Axe III,
- positive precision lift over chance in Sonic 3D Blast,
- positive precision lift over chance in aggregate Genesis cross-soundtrack retrieval,
- empirical permutation `p <= 0.05` for structural precision lift in Golden Axe III,
- empirical permutation `p <= 0.05` for structural precision lift in Sonic 3D Blast,
- empirical permutation `p <= 0.05` for structural precision lift in aggregate Genesis cross-soundtrack retrieval,
- positive cross-soundtrack precision lift when Golden Axe III is the query world,
- positive cross-soundtrack precision lift when Sonic 3D Blast is the query world,
- positive cross-soundtrack precision lift when J.League Pro Striker 2 is the query world,
- positive mean best-Maeda-minus-best-non-Maeda margin in both same-game worlds,
- no result depends on the quarantined Scorching Sand label.

The three primary `p <= 0.05` requirements are an intersection gate. Diagnostic p-values do not create extra ways to pass.

### Sonic 3D creator-specificity gate: complete 24-track role map

The primary `structural` result must satisfy:

- `same_family_candidates_excluded == true`,
- balanced top-1 accuracy `> 0.50`,
- Maeda recall `> 0.50`,
- Senoue recall `> 0.50`,
- mean best-same-minus-best-other margin `> 0`,
- empirical role-permutation `p <= 0.05` for structural balanced top-1 accuracy,
- empirical role-permutation `p <= 0.05` for structural mean best-same-minus-best-other margin,
- singleton top-1 intrusions are reported explicitly and never removed post hoc.

Both significance requirements must pass. Raw per-class recall thresholds remain mandatory so one creator cannot carry the aggregate.

### Sonic 3D creator-specificity gate: direct-mapping-only replication

The 22-track Stage F `structural` result must independently satisfy the same directional and significance conditions:

- `excluded_mapping_states == ["derived_audio_correspondence"]`,
- `included_track_count == 22`,
- `same_family_candidates_excluded == true`,
- balanced top-1 accuracy `> 0.50`,
- Maeda recall `> 0.50`,
- Senoue recall `> 0.50`,
- mean best-same-minus-best-other margin `> 0`,
- matched 22-track permutation `p <= 0.05` for structural balanced top-1 accuracy,
- matched 22-track permutation `p <= 0.05` for structural mean best-same-minus-best-other margin.

If the complete-map panel passes but the direct-mapping-only replication fails, the creator-specificity gate fails. The two panels are robustness checks of one hypothesis and must not be counted as two independent confirmations.

Any role-specificity result that succeeds through same-family siblings or uncertain corpus-tail mappings is invalid by construction.

This is a minimum calibration gate, not proof of creator identity. If it fails, improve the representation on controls and rerun as a new frozen experiment. Do not tune on Sonic 3.

Realization may be informative even when structural calibration fails, but it can support only arrangement / sequencing / programming / toolchain hypotheses.

## Ablation status before Sonic 3 promotion

Already removed or controlled in the composition-facing path:

- patch / voice identity,
- realization-specific pan, algorithm, feedback, patch, PSG, DAC, and routing behavior,
- absolute-key dependence through relative semitone motion,
- raw-tempo dependence through per-channel median-normalized onset gaps,
- same-soundtrack shortcuts in cross-soundtrack scoring,
- same-family sibling shortcuts in Sonic 3D role specificity,
- label-count advantage through permutation nulls,
- the two uncertain Sonic 3D corpus-tail role mappings through mandatory direct-mapping-only replication.

Still required before a strong Sonic 3 promotion:

- understand whether any structural result is carried only by pitch or only by rhythm,
- improve persistent-part recovery so physical YM2612 channels are not silently treated as stable compositional voices,
- repeat the entire control experiment after any material feature-model change,
- keep composition-facing and realization-facing evidence separate throughout.

Any ablation that destroys the Maeda signal lowers confidence. An implementation-heavy signal that disappears when realization features are removed is not evidence of composition.

## Quarantined adversarial probe

After the scored calibration is frozen, `Golden Axe III / The Scorching Sand` may be passed through the same feature space as an **unscored adversarial probe**. Ask where it falls naturally, but never use the answer to alter training labels or calibration scores.

## Only then: Sonic 3 candidate search

If the structural calibration gate survives the Maeda controls, both empirical null families, anti-sibling role-specificity tests, direct-mapping-only replication, and ablations, create a new frozen blind extraction for unresolved Sonic 3 cues and compare them against the already-frozen control representation.

The allowed conclusion format is:

```text
Maeda candidate: supported / mixed / not supported by calibrated structural similarity
```

Never:

```text
composed by Tatsuyuki Maeda
```

unless independent historical evidence later closes that gap.
