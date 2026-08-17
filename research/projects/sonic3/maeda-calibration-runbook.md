# Tatsuyuki Maeda blind calibration runbook

This preregisters the control experiment that must succeed before any unresolved Sonic 3 cue may be scored as a Tatsuyuki Maeda candidate. It calibrates a feature model; it does **not** assign Sonic 3 authorship.

## Evidence lanes

- `structural`: primary composition-facing aggregate from the current YM2612 key-on trajectory model.
- `structural_pitch`: interval, interval-bigram, and contour diagnostic.
- `structural_rhythm`: tempo-normalized onset-gap diagnostic.
- `realization`: arrangement / sequence / sound-data / patch / routing / implementation-facing evidence.

Pitch and rhythm are subviews of the same structural evidence, not independent confirmations. Realization can never satisfy a composition gate.

The held-out target is `tests/corpus/sonic-3-knuckles`. It is forbidden from all calibration audits.

## Control worlds

1. `golden-axe-iii-genesis-vgz`: 10 Maeda, 4 Hataya, 5 Oguro, 1 uncontested Sawada. `The Scorching Sand` remains a quarantined documentary conflict and is excluded from scoring.
2. `sonic-3d-blast-genesis-vgm`: complete 24-track role map. Composition is Maeda 10 / Senoue 12 / Setsumaru 1 / Okamoto 1. Arrangement/programming is Maeda 10 / Senoue 10 / Setsumaru 4. Setsumaru and Okamoto are composition singletons, not learnable classes.
3. `j-league-pro-striker-2-vgz`: six whole-soundtrack Maeda-positive Genesis controls.
4. `super-columns-vgm`: future cross-platform stress only. The current extractor is Genesis/YM2612-centric and must not mix Game Gear PSG evidence into the primary calibration.

## Stage A: freeze creator-blind features

Run before loading documentary labels into any scoring process:

```bash
python tools/cross_soundtrack_vgm_audit.py \
  tests/corpus/golden-axe-iii-genesis-vgz \
  tests/corpus/sonic-3d-blast-genesis-vgm \
  tests/corpus/j-league-pro-striker-2-vgz \
  --include-within-soundtrack \
  --neighbors 0 \
  --json research/projects/sonic3/maeda-calibration-frozen-audit.json
```

Required contract:

```text
model = blind cross-soundtrack Genesis VGM creator audit
label_policy contains: No composer/artist metadata
```

The audit must contain no Sonic 3 target tracks and no creator labels. Record its SHA-256 before unblinding. Any feature change creates a new experiment.

## Stage B: Maeda-vs-rest unblinding

```bash
python tools/maeda_calibration_eval.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --k 3 \
  --json research/projects/sonic3/maeda-calibration-results.json
```

The evaluator fails closed on a non-blind audit, held-out Sonic 3 leakage, incomplete Sonic 3D coverage, or missing scored controls.

Read structural and realization lanes separately. Report Golden Axe III within-soundtrack, Sonic 3D Blast within-soundtrack, aggregate Genesis cross-soundtrack, and cross-soundtrack results by query world. Primary statistics are precision@k, chance precision, lift over chance, MRR, and best-positive-minus-best-negative margin.

## Stage C: count-matched Maeda null

```bash
python tools/maeda_calibration_null.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --k 3 \
  --permutations 5000 \
  --seed 20260816 \
  --json research/projects/sonic3/maeda-calibration-null.json
```

Golden Axe III and Sonic 3D Blast retain their observed Maeda counts; J.League remains a fixed whole-soundtrack positive world. Scorching Sand stays excluded. Empirical p-values use the plus-one correction `(1 + null >= observed) / (1 + permutations)`.

## Stage D: Sonic 3D creator specificity

```bash
python tools/sonic3d_role_specificity_eval.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --families research/projects/sonic3/sonic3d-role-family-policy.json \
  --json research/projects/sonic3/sonic3d-role-specificity-results.json
```

The family policy is mandatory. Every query excludes candidates from the same zone/work/source-lineage family. Act 1 cannot retrieve Act 2 of the same zone, and the related Robotnik 2 / Robotnik 3 Boss-2 lineage cannot retrieve within itself.

Composition uses structural evidence to distinguish recurring Maeda and Senoue classes. Setsumaru and Okamoto remain singleton sentinels and may cause errors. Arrangement/programming uses realization to distinguish Maeda, Senoue, and Setsumaru; it cannot satisfy a composition gate.

## Stage E: primary family-block role null

Family correlation must remain present in the primary null:

```bash
python tools/sonic3d_role_specificity_null.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --families research/projects/sonic3/sonic3d-role-family-policy.json \
  --permutation-unit family-block \
  --permutations 5000 \
  --seed 20260816 \
  --json research/projects/sonic3/sonic3d-role-specificity-null.json
```

The family-block null keeps feature vectors and family IDs fixed. Complete label packages move only among selected families with the same track count, with labels permitted to reorder inside the target family. This preserves global role counts, family-size distribution, the multiset of within-family label compositions, singleton treatment, and the same-family exclusion geometry while breaking creator-to-feature alignment.

A track-wise shuffle is a **secondary sensitivity only**:

```bash
python tools/sonic3d_role_specificity_null.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --families research/projects/sonic3/sonic3d-role-family-policy.json \
  --permutation-unit track \
  --permutations 5000 \
  --seed 20260816 \
  --json research/projects/sonic3/sonic3d-role-specificity-track-null.json
```

Track-wise significance cannot rescue a failed family-block result. The two nulls are sensitivity views of one dataset, not independent evidence.

## Stage F: direct-mapping-only replication

Two Sonic 3D tail mappings are explicitly `derived_audio_correspondence` rather than direct title matches:

- `19 - Special Stage 2.vgm` -> `Sonic 3 Bonus Stage`
- `22 - Robotnik 3.vgm` -> `unused 1D`

The full 24-track audit and maps are validated first, then both rows are removed:

```bash
python tools/sonic3d_role_specificity_eval.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --families research/projects/sonic3/sonic3d-role-family-policy.json \
  --exclude-mapping-state derived_audio_correspondence \
  --json research/projects/sonic3/sonic3d-role-specificity-direct-map.json
```

Matched 22-track primary null:

```bash
python tools/sonic3d_role_specificity_null.py \
  research/projects/sonic3/maeda-calibration-frozen-audit.json \
  --policy research/projects/sonic3/maeda-calibration-policy.json \
  --families research/projects/sonic3/sonic3d-role-family-policy.json \
  --exclude-mapping-state derived_audio_correspondence \
  --permutation-unit family-block \
  --permutations 5000 \
  --seed 20260816 \
  --json research/projects/sonic3/sonic3d-role-specificity-direct-map-null.json
```

Expected selected-panel counts are composition Maeda 10 / Senoue 10 / Setsumaru 1 / Okamoto 1 and arrangement/programming Maeda 10 / Senoue 9 / Setsumaru 3. The 22-track null must be recomputed on the selected geometry, never reused from the 24-track distribution.

## Preregistered pass gate

**Sonic 3 stays sealed unless every primary structural condition passes.**

Maeda-vs-rest:
- positive structural precision lift in Golden Axe III, Sonic 3D Blast, and aggregate Genesis cross-soundtrack retrieval;
- structural precision-lift permutation `p <= 0.05` in all three environments;
- positive cross-soundtrack lift when each of Golden Axe III, Sonic 3D Blast, and J.League is the query world;
- positive mean best-Maeda-minus-best-non-Maeda margin in both mixed-composer same-game worlds;
- no scored result depends on Scorching Sand.

Sonic 3D 24-track specificity:
- same-family candidates excluded;
- structural balanced top-1 accuracy `> 0.50`;
- Maeda recall `> 0.50` and Senoue recall `> 0.50`;
- structural best-same-minus-best-other margin `> 0`;
- **family-block** permutation `p <= 0.05` for structural balanced accuracy and structural margin;
- singleton intrusions reported, never removed post hoc.

Sonic 3D 22-track direct-map replication must satisfy the same directional thresholds and the same two matched **family-block** significance thresholds, with `included_track_count == 22` and the derived mapping state excluded. If either 24-track or 22-track panel fails, the creator-specificity gate fails.

Pitch/rhythm subviews, realization, track-wise nulls, 24/22 replications, and multiple statistics are diagnostics or robustness checks. They must not be counted as independent evidence multipliers or used to create alternate pass routes.

## Remaining representation caveat

The current structural extractor is already patch-free, relative-pitch based, and median-gap normalized, so patch identity, absolute key, and raw tempo are not primary structural shortcuts. The major remaining representation limitation is **persistent-part recovery**: physical YM2612 channels are not guaranteed to be stable compositional voices. A material change to that representation requires rerunning the entire control experiment from a new frozen blind audit.

## Quarantined adversarial probe

After scored calibration is frozen, Golden Axe III `The Scorching Sand` may be projected into the same feature space as an **unscored** adversarial probe. Its location can be observed, but it cannot alter labels, thresholds, or calibration scores.

## Only then: Sonic 3

Only after every gate above survives may unresolved Sonic 3 cues enter a new frozen blind extraction. The allowed conclusion is limited to:

```text
Maeda candidate: supported / mixed / not supported by calibrated structural similarity
```

Never convert similarity alone into `composed by Tatsuyuki Maeda`; independent historical evidence is required for a historical assignment.
