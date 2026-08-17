# Tatsuyuki Maeda blind calibration runbook

This is the preregistered execution path for testing whether the current VGM feature model can rediscover known Tatsuyuki Maeda controls before any unresolved Sonic 3 cue is allowed into the experiment.

## Claim boundary

This experiment calibrates a feature model. It does **not** assign Sonic 3 authorship.

The evidence lanes remain separate:

- `structural` is composition-facing evidence inferred from the current physical-channel YM2612 musical trajectory model.
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

## Primary readouts

Read structural and realization views independently.

For each view inspect:

1. Golden Axe III within-soundtrack retrieval
2. Sonic 3D Blast within-soundtrack retrieval
3. Genesis cross-soundtrack retrieval

Primary statistics:

- `precision_at_k`
- `chance_precision_at_k`
- `precision_lift_over_chance`
- `mean_reciprocal_rank`
- `mean_best_positive_minus_best_negative`

Raw precision is never sufficient. A Maeda-heavy candidate pool can produce superficially good precision by chance, which is why lift over the query-specific positive fraction is mandatory.

## Preregistered calibration gate

Do **not** expose unresolved Sonic 3 cues to Maeda scoring unless the structural view satisfies all of the following:

- positive precision lift over chance in Golden Axe III,
- positive precision lift over chance in Sonic 3D Blast,
- positive precision lift over chance in Genesis cross-soundtrack retrieval,
- positive mean best-Maeda-minus-best-non-Maeda margin in both same-game worlds,
- no result depends on the quarantined Scorching Sand label.

This is a minimum gate, not proof of creator identity. If it fails, improve the representation on controls and rerun as a new frozen experiment. Do not tune on Sonic 3.

Realization may be informative even when structural calibration fails, but it can support only arrangement / sequencing / programming / toolchain hypotheses.

## Required ablations before Sonic 3 promotion

The current first-pass audit already normalizes transposition-facing interval features and normalized timing gaps, but the full Maeda promotion path still requires explicit ablations for:

- patch / voice identity removed,
- platform-specific realization removed,
- tempo sensitivity checked,
- transposition sensitivity checked,
- each soundtrack used as the query world while same-soundtrack candidates are excluded,
- composition-facing and realization-facing evidence kept separate.

Any ablation that destroys the Maeda signal lowers confidence. An implementation-heavy signal that disappears when realization features are removed is not evidence of composition.

## Quarantined adversarial probe

After the scored calibration is frozen, `Golden Axe III / The Scorching Sand` may be passed through the same feature space as an **unscored adversarial probe**.

Ask where it falls naturally. Do not use the answer to alter the training labels or the calibration score. Its documentary conflict makes it useful precisely because the experiment cannot safely assume which side is true.

## Only then: Sonic 3 candidate search

If the structural calibration gate survives the controls and ablations, create a new frozen blind extraction for unresolved Sonic 3 cues and compare them against the already-frozen control representation.

The allowed conclusion format is:

```text
Maeda candidate: supported / mixed / not supported by calibrated structural similarity
```

Never:

```text
composed by Tatsuyuki Maeda
```

unless independent historical evidence later closes that gap.
