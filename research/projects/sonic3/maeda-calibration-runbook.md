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
   - complete 24-track per-track-credit partition
   - 10 Maeda controls
   - the other 14 fixtures are non-Maeda controls
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

## Stage B: documentary unblinding

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

## Stage C: count-matched documentary-label null

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

## Primary readouts

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

Do **not** expose unresolved Sonic 3 cues to Maeda scoring unless the `structural` view satisfies all of the following:

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

This is a minimum gate, not proof of creator identity. If it fails, improve the representation on controls and rerun as a new frozen experiment. Do not tune on Sonic 3.

Realization may be informative even when structural calibration fails, but it can support only arrangement / sequencing / programming / toolchain hypotheses.

## Ablation status before Sonic 3 promotion

Several requested confounds are already removed by construction in the composition-facing path:

- patch / voice identity is absent from `structural`, `structural_pitch`, and `structural_rhythm`,
- realization-specific pan, algorithm, feedback, patch, PSG, DAC, and routing behavior is absent from structural scoring,
- pitch is represented by relative semitone motion rather than absolute key, making the pitch lens transposition-tolerant,
- onset gaps are divided by each physical channel's median positive gap before quantization, making the rhythm lens tempo-normalized,
- cross-soundtrack scoring excludes same-soundtrack candidates and now reports each query world separately.

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

If the structural calibration gate survives the controls, empirical null, and ablations, create a new frozen blind extraction for unresolved Sonic 3 cues and compare them against the already-frozen control representation.

The allowed conclusion format is:

```text
Maeda candidate: supported / mixed / not supported by calibrated structural similarity
```

Never:

```text
composed by Tatsuyuki Maeda
```

unless independent historical evidence later closes that gap.
