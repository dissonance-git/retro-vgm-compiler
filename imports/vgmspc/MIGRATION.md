# vgmspc retirement migration

`dissonance-git/vgmspc` is a migration source, not a sibling architecture and not a permanent dependency of Retro VGM Compiler.

Pinned source state for this audit:

- repository: `dissonance-git/vgmspc`
- source head inspected: `f3368941213c841fdb59f601196401aef30c257a`
- source-head message: `ci: run complete focused VGM source test suite`
- migration target: `dissonance-git/retro-vgm-compiler` `main`

The old repository or local checkout is deletable only when every non-third-party capability below is either present canonically in Retro VGM Compiler or explicitly retired here with an evidence-backed reason. Merely copying the old tree into `imports/` does not count as migration.

## Migration law

1. Preserve unique executable knowledge, tests, build/release knowledge, and provenance.
2. Prefer the newer Retro VGM Compiler implementation when both repositories encode the same mechanism.
3. Do not revive heuristics that the newer evidence model intentionally rejected.
4. Do not vendor replaceable upstream SDKs/libraries just because the old build tree contained them. Preserve exact provenance/pins and reproduce them from the canonical build instead.
5. Move source-specific host code under the owning component (`components/vgm/` or `components/spc/`), shared evidence only after the abstraction is independently justified, maintained transformations under `patches/`, tests under `tests/`, and immutable external provenance under `imports/`.
6. The final proof of retirement is a clean build/package from this repository with no path, checkout, submodule, artifact, or manual step that requires `vgmspc`.

## Inventory and disposition

| vgmspc area | Disposition | Canonical target / reason | State |
|---|---|---|---|
| `foo_input_vgm/src` reusable Genesis source/enhancement primitives | Mine by semantic comparison, not wholesale copy | `components/vgm/foo_input_vgm/src/` plus `components/vgm/enhancement/`; much of this code already has a newer descendant here | in progress |
| `foo_input_vgm` foobar host source, resources, `.sln` / `.vcxproj` | Preserve the still-required host/build knowledge | source-specific VGM host area under `components/vgm/` | open |
| `foo_snesapu` foobar/SPC player host source and projects | Preserve the still-required host/build knowledge | source-specific SPC host area under `components/spc/` | open |
| `patches/libvgm` | Superseded by the newer combined source-capture patcher | `patches/libvgm/apply_source_capture.py` | retired |
| SNESAPU source-capture / transport patches | Compare against and keep the newer chain | `patches/snesapu/`; current repository already contains the maintained source-capture, SRCE transport and enhanced paths | in progress |
| VGM/Omniphony plugin patch logic | Compare against current selected-source and delivered-route transport | `patches/foo_input_vgm/`; current chain already renders finalized selected Genesis sources through Omniphony | in progress |
| `tests/vgm/genesis_source_plane_test.cpp` relative chip-height assertions | Intentionally superseded | old test asserts inferred PSG-above-YM placement; current source model keeps listener-space placement neutral unless authored/perceptual evidence earns it | retired |
| authored YM2612 / Game Gear route assertions embedded in that test | Preserve where not already covered | current authored-route and route-transport tests | verify |
| `tests/vgm/linear_source_resampler_test.cpp` | Unique useful regression | migrate/adapt under `tests/vgm/`; pins libvgm linear startup pre-roll, reset retention, segmentation and rate/volume parity | open |
| `inference/` generic headers | Mine by model comparison | likely ancestors of `model/`; copy only relations not already represented more rigorously | open |
| `.github/workflows/build.yml` | Mine build/release procedure, not repository layout | canonical Windows component build/package workflow in this repository | open |
| `.github/workflows/snesapu-source.yml` | Mine unique SNESAPU validation/build behavior | canonical SPC validation/build workflow or tests | open |
| `.github/workflows/spc-source-player.yml` | Mine unique source-player validation/build behavior | canonical SPC validation/build workflow or tests | open |
| `Directory.Build.props` / `Directory.Build.targets` | Preserve only rules still required by imported projects | canonical component build support | open |
| `SDK-2025-03-07`, `WTL`, `libvgm-master`, `third_party`, `submodules` dependency copies/checkouts | Do not treat copies as project knowledge | preserve upstream URLs, exact required revisions, patch order and build flags; fetch/reconstruct dependencies reproducibly | open |
| old `genesis_omniphony_projection.h` inferred depth/height policy | Intentionally superseded | current exact authored-route transport + selected-source Omniphony runtime avoids device/pitch-to-position invention | retired |
| binaries, intermediate outputs, generated release packages | Do not migrate as source of truth | regenerate from canonical source/build; retain hashes only when scientifically useful | open |

## Proven build knowledge already identified

The old Windows integration build establishes several requirements that must survive retirement:

- libvgm source capture is patched before its static libraries are built;
- the focused VGM source tests run against the patched libvgm semantics;
- Omniphony source ABI and Windows payload are built before plugin packaging;
- SNESAPU source capture is patched before building the x86 `SNESAPU.dll`;
- source-aware `spcplayer.exe`, `foo_snesapu`, and `foo_input_vgm` are separate build products;
- `.fb2k-component` packages place runtime dependencies beside the component DLL that loads or launches them;
- the combined release was gated on required files/exports rather than packaging best-effort outputs.

These are migration facts, not a mandate to preserve the old directory layout or historical junction hacks.

## Dependency provenance observed in vgmspc

The old tree references foobar2000 SDK, WTL, libvgm and Omniphony as external dependencies. Its later build additionally pins SPCPlay/SNESAPU and Omniphony revisions and pins a Rust toolchain. The canonical build must record exact revisions at the point where they are actually required, rather than inheriting an opaque copied dependency directory.

## Retirement checklist

`vgmspc` is fully mined only when all rows above are `migrated`, `superseded`, or `retired`, and these end-to-end checks are reproducible from a fresh Retro VGM Compiler checkout:

- core tests;
- patched libvgm source-capture tests, including linear resampler parity;
- source-aware VGM component build;
- patched SNESAPU + source-player + SPC component build;
- Omniphony ABI validation;
- reference/enhanced and stereo/spatial playback combinations where applicable;
- foobar component packaging with all runtime dependencies present;
- no build or test step reads from a `vgmspc` path or repository.

Until those conditions hold, `vgmspc` is read-only migration evidence. New implementation work belongs here.
