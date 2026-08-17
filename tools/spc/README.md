# SPC tools

This shelf owns executable SPC research operations. For component semantics, see `../../components/spc/README.md`; for repository-wide navigation, see `../../CATALOG.md`.

## Creator-blind calibration path

```text
SPC fixture
→ forensic/spc_forensic_features        controlled runtime execution
→ creator_blind_spc_cache.py            persistent song-centered sidecar
→ capture_blind_panel.py                 opaque panel projection
→ freeze_forensic_sidecars.py            frozen creator-blind geometry
→ evaluate_cube_calibration.py           post-freeze role-evidence reveal
```

The expensive object is the song capture, not the panel. A song should therefore be executed once for a compatible forensic generation and reused across future opaque panels.

## Current tools

```text
creator_blind_spc_cache.py        reusable creator-blind forensic sidecars
capture_blind_panel.py            panel capture/reuse + freeze orchestration
freeze_forensic_sidecars.py       integrity gate + part-profile geometry freeze
evaluate_cube_calibration.py      evidence-safe CUBE reveal/evaluation
correlate_spc_snapshot_state.py   snapshot-state correlation
inspect/correlation helpers       bounded source/runtime investigations
patch_snes_spc_*.py               instrumentation/runtime patch maintenance
forensic/                         compiled controlled-execution extractor
```

## Boundary

```text
cached forensic sidecar
!= creator label
!= frozen experiment matrix
!= authorship result
```

Creator/candidate/track-title/game metadata must not enter the forensic extraction object. Historical role evidence is joined later from the canonical Sonic 3 admissions/policy files.

Do not create another SPC parser merely to answer an attribution question if the existing snapshot/runtime/forensic machinery already exposes the needed evidence.
