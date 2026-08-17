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

The evaluator must fail closed if:

- the input is not the creator-blind audit contract,
- the Sonic 3 held-out target appears in the audit,
- the Sonic 3D Blast partition is incomplete,
- any required scored control is missing.

## Stage C: count-matched Maeda-label null

Use the same frozen Stage A audit. Shuffle labels only after feature extraction:

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

The reported empirical p-value uses the plus-one correction:

```text
(1 + number of null scores >= observed score) / (1 + permutations)
```

This tests whether the documentary labels align with the frozen feature geometry better than count-matched fake credit maps. It still does not test Sonic 3 authorship.

## Stage D: Sonic 3D Blast role specificity with anti-sibling exclusion

Use the same frozen Stage A audit again. This stage asks a harder question than Maeda-vs-rest: does the feature geometry distinguish recurring creators inside one soundtrack after obvious related-work shortcuts are removed?

```bash
python tools/sonic3d_role_specificity_eval.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --families research/projects/sonic3/sonic3d-role-family-policy.json \
  --json research/projects/sonic3/sonic3d-role-specificity-results.json
```

The family policy is mandatory. Every query excludes candidates from the same zone/work/source-lineage family. In particular:

- Act 1 cannot retrieve Act 2 of the same zone as a creator win,
- `Robotnik 2` cannot retrieve the related `Robotnik 3` / unused-1D Boss-2-family variant as a creator win,
- family exclusion is applied identically in composition and arrangement/programming lanes.

This prevents the experiment from confusing work identity with creator identity.

### Stage D composition lane

The preregistered composition classes are:

- Tatsuyuki Maeda: 10 fixtures
- Jun Senoue: 12 fixtures

Masaru Setsumaru and Seirou Okamoto each have one composition fixture. They remain singleton sentinels. They may steal a top-1 retrieval and count as an error, but they must never be treated as learned creator clusters.

Evaluate `structural` as the primary composition-facing view. `structural_pitch` and `structural_rhythm` are diagnostic subviews only.

Primary Stage D composition statistics:

- balanced top-1 accuracy,
- Maeda recall,
- Senoue recall,
- mean reciprocal rank,
- mean best-same-creator-minus-best-other-creator margin,
- singleton top-1 intrusions,
- number of same-family candidate instances excluded.

### Stage D arrangement/programming lane

Use only `realization` and the recurring role classes:

- Tatsuyuki Maeda: 10 fixtures
- Jun Senoue: 10 fixtures
- Masaru Setsumaru: 4 fixtures

This lane can calibrate arrangement / sequence / sound-data / implementation fingerprints only. It is forbidden from satisfying a composition gate.

## Stage E: anti-sibling role-label permutation null

Stage D must also beat randomized role maps. Use the same frozen audit and the same fixed family policy:

```bash
python tools/sonic3d_role_specificity_null.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --families research/projects/sonic3/sonic3d-role-family-policy.json \
  --permutations 5000 \
  --seed 20260816 \
  --json research/projects/sonic3/sonic3d-role-specificity-null.json
```

The null keeps the feature vectors and every `family_id` fixed. It permutes documentary role labels across tracks while preserving the exact observed label multiset separately for composition and arrangement/programming. Therefore every null trial retains:

- composition counts 10 Maeda / 12 Senoue / 1 Setsumaru / 1 Okamoto,
- arrangement/programming counts 10 Maeda / 10 Senoue / 4 Setsumaru,
- the exact same same-family candidate exclusions as the observed result,
- the same singleton treatment in the composition lane.

The primary composition null statistics are:

- balanced top-1 accuracy,
- minimum recall across the learnable composition classes,
- mean reciprocal rank,
- mean best-same-creator-minus-best-other-creator margin.

The same statistics are produced for pitch, rhythm, and realization as diagnostics. They are not alternative composition pass routes.

## Primary Maeda readouts

For each of `structural`, `structural_pitch`, `structural_rhythm`, and `realization`, inspect:

1. Golden Axe III within-soundtrack retrieval
2. Sonic 3D Blast within-soundtrack retrieval
3. Genesis cross-soundtrack retrieval
4. Genesis cross-soundtrack retrieval split by held-out query world

Primary statistics:

- `precision_at_k`
- `chance_precision_at_k`
- `precision_lift_over_chance`
- `mean_reciprocal_rank`
- `mean_best_positive_minus_best_negative`
- empirical permutation `p` for the same higher-is-better statistics

Raw precision is never sufficient. A Maeda-heavy candidate pool can produce superficially good precision by chance, which is why lift over the query-specific positive fraction and the count-matched permutation null are both mandatory.

The `structural` aggregate remains the preregistered promotion statistic. Pitch and rhythm subviews explain *why* it succeeds or fails; they do not multiply the evidence count.

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

The three `p <= 0.05` requirements are an intersection gate: all three primary structural environments must clear the threshold. Pitch, rhythm, realization, reciprocal-rank, and margin p-values remain diagnostics and do not create extra ways to pass.

### Sonic 3D creator-specificity gate

The primary `structural` role-specificity result must additionally satisfy:

- `same_family_candidates_excluded == true`,
- balanced top-1 accuracy strictly greater than `0.50`,
- Maeda recall strictly greater than `0.50`,
- Senoue recall strictly greater than `0.50`,
- mean best-same-creator-minus-best-other-creator margin greater than `0`,
- empirical role-permutation `p <= 0.05` for structural balanced top-1 accuracy,
- empirical role-permutation `p <= 0.05` for structural mean best-same-creator-minus-best-other-creator margin,
- singleton top-1 intrusions are reported explicitly and are not removed post hoc.

The two role-specificity significance requirements are an intersection gate. Both must pass. `minimum_learnable_class_recall` and reciprocal-rank permutation p-values are diagnostics; raw Maeda and Senoue recall thresholds remain mandatory so one creator cannot carry the balanced score while the other collapses.

A role-specificity result that succeeds only because Act 1 retrieves Act 2, or because one Boss-2-family variant retrieves the other, is invalid by construction.

This is a minimum calibration gate, not proof of creator identity. If it fails, improve the representation on controls and rerun as a new frozen experiment. Do not tune on Sonic 3.

Realization may be informative even when structural calibration fails, but it can support only arrangement / sequencing / programming / toolchain hypotheses.

## Ablation status before Sonic 3 promotion

Several requested confounds are already removed by construction in the composition-facing path:

- patch / voice identity is absent from `structural`, `structural_pitch`, and `structural_rhythm`,
- realization-specific pan, algorithm, feedback, patch, PSG, DAC, and routing behavior is absent from structural scoring,
- pitch is represented by relative semitone motion rather than absolute key, making the pitch lens transposition-tolerant,
- onset gaps are divided by each physical channel's median positive gap before quantization, making the rhythm lens tempo-normalized,
- cross-soundtrack scoring excludes same-soundtrack candidates and reports each query world separately,
- Sonic 3D creator-specificity scoring excludes same-family candidates,
- Sonic 3D creator-specificity significance is measured against role-count-preserving permutations with family geometry fixed.

Still required before a strong Sonic 3 promotion:

- verify that the result is not carried by only pitch or only rhythm without understanding that asymmetry,
- improve persistent-part recovery so physical YM2612 channels are not silently treated as stable compositional voices,
- repeat the control experiment after any material feature-model change,
- keep composition-facing and realization-facing evidence separate throughout.

Any ablation that destroys the Maeda signal lowers confidence. An implementation-heavy signal that disappears when realization features are removed is not evidence of composition.

## Quarantined adversarial probe

After the scored calibration is frozen, `Golden Axe III / The Scorching Sand` may be passed through the same feature space as an **unscored adversarial probe**.

Ask where it falls naturally. Do not use the answer to alter the training labels or the calibration score. Its documentary conflict makes it useful precisely because the experiment cannot safely assume which side is true.

## Only then: Sonic 3 candidate search

If the structural calibration gate survives the Maeda controls, both empirical nulls, anti-sibling role-specificity control, and ablations, create a new frozen blind extraction for unresolved Sonic 3 cues and compare them against the already-frozen control representation.

The allowed conclusion format is:

```text
Maeda candidate: supported / mixed / not supported by calibrated structural similarity
```

Never:

```text
composed by Tatsuyuki Maeda
```

unless independent historical evidence later closes that gap.
