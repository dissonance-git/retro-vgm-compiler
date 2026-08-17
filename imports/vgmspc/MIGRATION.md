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
| `foo_input_vgm` foobar host source, resources, `.sln` / `.vcxproj` | Reconstruct from the immutable project bootstrap and current overlays instead of copying the stale vgmspc host tree | `imports/foo_input_vgm.7z` + `components/vgm/foo_input_vgm/` + `tools/materialize_foo_input_vgm.py`; smoke workflow does not consult vgmspc | migrated |
| `foo_snesapu/foobar2000/foo_snesapu` parent host | Preserve the proven process transport baseline, then apply the current enhanced/spatial patch stack | canonical SPC parent bootstrap/materializer still required; current private patch stack must stop requiring an external vgmspc checkout | open |
| `foo_snesapu/spcplayer` source-aware child | Migrated as canonical source, with the stale checked-in import library removed | `components/spc/spcplayer/`; SRCE v2 now comes from `components/spc/snesapu_source_wire_v2.h`, and the project requires a freshly built SNESAPU import library | migrated |
| `patches/libvgm` | Superseded by the newer combined source-capture patcher | `patches/libvgm/apply_source_capture.py` | retired |
| SNESAPU source-capture / SRCE v2 patches | Consolidated and then advanced beyond the old two-script chain | `patches/snesapu/apply_source_capture.py`, `patches/snesapu/upgrade_source_capture_v2.py`, `patches/snesapu/apply_private_snesapu.py`, and current SRCE v2 wire/storage/runtime work | migrated |
| old VGM source-player / Omniphony plugin patches | Superseded by the selected-source and delivered-route architecture | `patches/foo_input_vgm/`; finalized reference/enhanced source choices are transported to delivered audio and then rendered conditionally through Omniphony | retired |
| old SPC Omniphony plugin bridge semantics | Re-expressed by the current source-aware private runtime: source admission, absolute source clock/generation, reset/seek handling, and reference fallback remain explicit | `patches/snesapu/apply_spatial_omniphony_private_runtime.py` and `patches/snesapu/apply_private_component.py`; remaining dependency is the parent bootstrap, not the runtime semantics | migrated |
| `tests/vgm/genesis_source_plane_test.cpp` relative chip-height assertions | Intentionally superseded | old test asserts inferred PSG-above-YM placement; current source model keeps listener-space placement neutral unless authored/perceptual evidence earns it | retired |
| authored YM2612 / Game Gear route assertions embedded in that test | Already represented by stronger dedicated controls | `tests/vgm/authored_stereo_route_test.cpp` covers mute/left/right/both YM routing and Game Gear/SN76489 masks including noise | migrated |
| `tests/vgm/linear_source_resampler_test.cpp` | Preserved as an external-libvgm integration regression | `tests/integration/libvgm-source/linear_source_resampler_test.cpp` with standalone dependency-aware CMake and provenance README | migrated |
| `inference/chip_inference.h`, `chip_adapter.h`, `chip_hint_bus.h` | Intentionally superseded, not copied | current `model/realtime_musical_role_hypothesis.h` separates resemblance from confidence, records cue provenance, caps weak acoustic role evidence, and does not directly turn guessed roles into spatial coordinates; the old singleton Prism hint bus is obsolete | retired |
| `.github/workflows/build.yml` | Mine build/release procedure, not repository layout | canonical Windows component build/package workflow in this repository | open |
| `.github/workflows/snesapu-source.yml` | Mine unique SNESAPU validation/build behavior | canonical SPC validation/build workflow or tests | open |
| `.github/workflows/spc-source-player.yml` | Source/link contract is now explicit in the migrated project; still preserve its independent CI proof that the child links the freshly built patched DLL | `components/spc/spcplayer/spcplayer.vcxproj` plus a canonical Windows integration workflow | in progress |
| `Directory.Build.props` / `Directory.Build.targets` | Preserve only rules still required by imported projects | WTL include/toolset compatibility belongs in canonical component build support; old automatic patch invocations are superseded by explicit current patch chains | in progress |
| `SDK-2025-03-07`, `WTL`, `libvgm-master`, `third_party`, `submodules` dependency copies/checkouts | Do not treat copies as project knowledge | preserve upstream URLs, exact required revisions, patch order and build flags; fetch/reconstruct dependencies reproducibly | open |
| old `genesis_omniphony_projection.h` inferred depth/height policy | Intentionally superseded | current exact authored-route transport + selected-source Omniphony runtime avoids device/pitch-to-position invention | retired |
| `foo_snesapu/spcplayer/lib/Win32/snesapu.lib` | Do not migrate a stale import library | regenerated by the exact patched SNESAPU build and supplied to the canonical spcplayer project through `SNESAPULibDir` | retired |
| other binaries, intermediate outputs, generated release packages | Do not migrate as source of truth | regenerate from canonical source/build; retain hashes only when scientifically useful | open |

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

## SPC parent bootstrap cut

The current SPC private runtime deliberately targets `vgmspc@2b7ec8bbd7326eabee3ba39bb91130b9b128e74b`, the commit `spc: splice framed source packets across foobar requests`. That cut is useful because it already contains the proven x86-child to x64-parent `[PCM][TLEM][SRCE-v2]` transport while predating the later Omniphony object/runtime layer.

That commit is provenance, not an acceptable permanent dependency. The remaining SPC migration work is to preserve the required parent-host source from that cut inside this repository, or reconstruct it from an independently pinned upstream baseline plus the proven transport changes, then point `apply_private_component.py` at that internal materialization. Once that is done, the private patch stack must contain no instruction to obtain or read a `vgmspc` checkout.

## Dependency provenance observed in vgmspc

The old tree references foobar2000 SDK, WTL, libvgm and Omniphony as external dependencies. Its later build additionally pins SPCPlay/SNESAPU and Omniphony revisions and pins a Rust toolchain. The canonical build must record exact revisions at the point where they are actually required, rather than inheriting an opaque copied dependency directory.

Observed old release-build pins:

- SPCPlay/SNESAPU commit: `fc770e268ecacb4523699e2edc5c0efdf80957d6`
- Omniphony commit: `32cf0cba471d39768593cd42e0a768a4c47bc045`
- Rust toolchain: `1.88.0`

The old `.gitmodules` URLs identify the upstream foobar2000 SDK, WTL, libvgm and Omniphony repositories. Their copied checkout directories are not themselves migration payloads.

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
