# vgmspc retirement migration

`dissonance-git/vgmspc` has been mined as a migration source. It is not a sibling architecture and is no longer a live build dependency of Retro VGM Compiler.

Pinned source states used during the audit:

- final source head inspected: `f3368941213c841fdb59f601196401aef30c257a`
- audited SPC parent transport cut: `2b7ec8bbd7326eabee3ba39bb91130b9b128e74b`
- migration target: `dissonance-git/retro-vgm-compiler` `main`

## Current status

**Executable extraction is structurally complete. Fresh Windows execution validation is still pending because GitHub has not assigned a hosted runner to the materialization job.** The old repository is no longer needed as a source/build input, but destructive deletion remains gated on the clean build/package proof below.

The failed hosted materialization job is not evidence of a source failure: GitHub created the job record but assigned no executable steps (`steps: []`) and produced no job log blob. Nothing in the repository was executed by that run.

## Migration law

1. Preserve unique executable knowledge, tests, build/release knowledge, and provenance.
2. Prefer newer Retro VGM Compiler descendants where both repositories encode the same mechanism.
3. Do not revive heuristics rejected by the newer evidence model.
4. Reconstruct replaceable upstream dependencies from immutable public pins instead of carrying copied dependency trees.
5. New implementation work belongs in `components/`, maintained transformations in `patches/`, tests in `tests/`, and immutable historical seeds in `imports/`.
6. Deletion requires a clean build/package from this repository with no path, checkout, binary, artifact, or manual step that reads `vgmspc`.

## Final disposition

| vgmspc area | Canonical disposition | State |
|---|---|---|
| VGM reusable source/enhancement code | newer descendants in `components/vgm/` and `patches/foo_input_vgm/` | migrated / superseded |
| VGM foobar host/project shell | `imports/foo_input_vgm.7z` + `tools/materialize_foo_input_vgm.py` + current overlays | migrated |
| SPC foobar parent host | audited seed in `imports/foo_snesapu/parent/`, protected by historical Git-blob checks | migrated |
| SPC child process | canonical `components/spc/spcplayer/`; stale checked-in import library removed | migrated |
| SPC parent/child source transport | active `apply_current_parent_source_transport.py` + `apply_current_child_source_transport.py`; current child is patched from its real SRCE-v2 baseline rather than replaying stale migrations | migrated |
| stale five-argument pre-BRR pointer fix | incompatible with the real four-argument SNESAPU `__stdcall` ABI and removed from the active chain | retired |
| SNESAPU source capture / pre-BRR / studio providers | current `patches/snesapu/apply_private_snesapu.py` stack | migrated |
| old SPC semantic/spatial presentation | current causal-source Omniphony runtime; guessed placement machinery not copied | superseded |
| old VGM Omniphony/source-player patches | current selected-source + delivered-route architecture | superseded |
| inferred Genesis PSG-above-YM policy/test | rejected by current evidence discipline; authored routing has stronger dedicated controls | retired |
| libvgm resampler/source-capture regression | `tests/integration/libvgm-source/`, now run against the exact pinned/patched libvgm tree before VGM component compilation | migrated |
| old inference singleton / Prism hint bus | current provenance-aware musical-role model | superseded |
| old build/release workflow knowledge | `tools/build_private_foobar_components.ps1` reconstructs, patches, tests, builds, verifies exports/architectures/runtime ABI, packages, reopens the final archives/bundle, and hashes both components | migrated |
| old SNESAPU/source-player linkage proof | current builder compiles patched x86 SNESAPU first and passes its include/lib paths explicitly to canonical `spcplayer.vcxproj`; packaged-child startup proves sibling DLL resolution | migrated |
| copied libvgm tree | `ValleyBell/libvgm@64e1de284e9a4305c54dd162ee8c33539a9bc0d1`, fingerprinted against the old copy | retired copy / pinned upstream |
| copied WTL tree | `Win32-WTL/WTL@d1cd80e9ce76c4d79da4cf556401ad7a970ce46f`, fingerprinted against the old copy | retired copy / pinned upstream |
| copied foobar SDK | official `SDK-2025-03-07.7z`; builder checks historical project fingerprints before use | retired copy / official reconstruction |
| SPCPlay submodule/copy | `dgrfactory/spcplay@fc770e268ecacb4523699e2edc5c0efdf80957d6` | pinned upstream |
| checked-in `snesapu.lib` and other generated binaries | rebuilt from canonical pinned source | retired |

## Preserved evidence

The SPC parent seed records exact historical Git object identity for every file consumed by guarded patch anchors. `tools/materialize_foo_snesapu.py` verifies those hashes before applying current transformations.

The old copied dependency trees were fingerprinted before replacement:

- libvgm root `CMakeLists.txt`: `1f8fb7f99ec45e1d2af12231f624498e6e252732`
- WTL `Include/atlapp.h`: `4b3fe38dfd930dfedf1fe642d5a2fe7d167ac099`
- foobar SDK `foobar2000_SDK.vcxproj`: `a1074e4aa8b2fc03cbc1738c9cddd912158bff67`
- foobar SDK `pfc.vcxproj`: `57cbc91551935cd6f12c13a0e41c4c6bf601ac94`

The live build verifies these identities instead of assuming that a similarly named dependency is equivalent.

The Omniphony compatibility seam is also pinned at source level. Retro VGM Compiler requires source ABI major `0`, minor `3`; `dissonance-git/Omniphony-Headphones@0fabccb165e6d957cefecc6eeb1264467e7406a4` defines `ABI_MAJOR = 0` and `ABI_MINOR = 3` in `omniphony-renderer/source_ffi/src/lib.rs`. Its exported `omniphony_source_abi_major()` and `omniphony_source_abi_minor()` functions are zero-argument C ABI functions returning `u32`, matching the dynamic loader contract.

## Current safeguards

The deletion gate is intentionally layered so a late Windows build is confirming binary integration, not discovering basic migration mistakes:

- `tests/private_components/CMakeLists.txt` is the first-stage deletion preflight. It now invokes `tests/run_full_core_suite.py`, which runs the complete dependency-free root `GAMEAUDIO_BUILD_CORE_TESTS` suite with the same generator/platform and then the SNESAPU source-chain harness before any external dependency checkout.
- `tests/spc_provider_contracts/CMakeLists.txt` makes nine existing child-side source tests executable in the deletion gate: BRR/DIR playback topology, native/enhanced source-hook bridges, pre-BRR packet/provider, studio packet/provider, SRCE-v2 transport, and source-object projection. These cover wrapped addresses, callback behavior, malformed packets, exact reconstruction, interpolation changes, live DIR/SRCN/loop remaps, impossible rates, non-finite inputs, and fail-closed source rejection.
- `tools/verify_build_source_provenance.py` requires a real 40-hex `HEAD` and no staged/unstaged tracked modifications. The builder captures that commit after the initial preflight and revalidates the same commit immediately before manifest creation, so a long build cannot silently change source underneath its provenance record. `tests/test_build_source_provenance_contract.py` falsifies dirty, staged, non-repository, and changed-clean-HEAD cases.
- `tests/test_no_live_vgmspc_dependency.py` rejects clone/fetch URLs, the retired scaffold identifiers, and maintained-code reads of `imports/vgmspc`. The import tree is archival evidence only.
- `tests/spc/test_active_spc_patch_contract.py` pins the active current-parent/current-child graph and the real four-argument pre-BRR `__stdcall` ABI; stale migration helpers cannot silently return to the live chain.
- `tests/vgm/test_materialize_foo_input_vgm.py` and `tests/spc/test_materialize_foo_snesapu.py` invoke the complete materializers and inspect the final composed hosts rather than merely checking patch-script presence. They pin current source transport, native companion launch geometry, modern Omniphony seams, and the audible fail-closed invariant: protected stereo is committed before Spatial is attempted, and the output chunk is replaced only after all source/route/renderer validation succeeds.
- `tests/vgm/test_enhanced_wording.py` enforces lowercase descriptive `enhanced` across VGM and explicitly selected maintained SPC surfaces while excluding historical anchor literals that guarded patchers must preserve exactly.
- `tools/build_private_foobar_components.ps1` patches/builds pinned libvgm, then configures/builds/tests `tests/integration/libvgm-source` against that exact patched tree before `foo_input_vgm` compilation.
- `tools/verify_private_component_packages.py` reopens the final `.fb2k-component` files and accepts only the exact flat sibling payload required by each runtime. It rejects missing/extra entries, nested or traversal paths, case-colliding names, zero-byte runtime files, wrong PE architectures, and missing named exports. On Windows it additionally extracts and loads the exact packaged Omniphony DLL from each component archive, executes its ABI version functions, and launches the exact packaged x86 `spcplayer.exe` far enough to reach its usage path, proving sibling `SNESAPU.dll` loader resolution.
- `tools/verify_omniphony_runtime_abi.py` resolves all six symbols used by `omniphony_dynamic_backend_loader`, executes the version functions, and requires ABI major `0` with minor `3` or newer within that major. `tests/test_omniphony_runtime_abi_contract.py` pins those values back to `model/omniphony_source_transport.h` and exercises compatible/incompatible fake runtimes portably.
- `tests/test_omniphony_sibling_loader_contract.py` pins component-local Omniphony lookup: the loader derives the directory of the embedding component DLL, attempts sibling `omniphony_source.dll` first, then applies the same ABI guard after loading.
- `tests/test_private_component_package_contract.py` exercises archive and PE failure modes with synthetic fixtures, including malformed images, wrong architecture, missing runtime exports, and packaged-child startup semantics.
- `tools/verify_private_component_bundle.py` audits the final outer ZIP: exact flat entries, manifest/package agreement, 40-hex source commit, 48 kHz contract, architecture map, SHA-256 agreement with the embedded component bytes, lowercase `enhanced` README wording, then re-runs the component verifier on the exact component archives extracted from the final bundle. `tests/test_private_component_bundle_contract.py` falsifies metadata/hash/architecture/naming/extra-file drift.
- `tests/test_private_component_builder_contract.py` pins deterministic output paths, same-tree libvgm integration ordering, source-provenance bookends, pre-package PE machine checks, final component verification, final bundle verification, and lowercase descriptive generated README wording.
- `tools/build_private_foobar_components.ps1` contains no `vgmspc` checkout. Historical provenance appears only in its output manifest.

## Destructive deletion gate

The old repository/directory may be deleted after one clean Windows run proves all of the following from a fresh Retro VGM Compiler checkout:

- the full dependency-free root core suite, nine SNESAPU source-chain contracts, and specialized private-component tests all pass;
- the exact source commit remains clean and unchanged from preflight through manifest creation;
- both VGM/SPC materializers succeed and their composed-output regressions pass;
- the pinned, patched libvgm source-capture/resampler integration suite passes before VGM component compilation;
- patched x86 SNESAPU exports `SetDSPSourceCapture`, `GetDSPSourceData`, `SetDSPPreBrrProvider`, and `SetDSPStudioSourceProvider`, while the source/provider contracts above exercise the corresponding reconstruction/fail-closed logic;
- canonical `spcplayer.exe`, `foo_snesapu.dll`, and `foo_input_vgm.dll` build with the asserted x86/x64 identities;
- the exact packaged Omniphony DLLs load and report a compatible 0.3+ ABI within major 0;
- the exact packaged `spcplayer.exe` starts and reaches its usage path with its sibling `SNESAPU.dll` present;
- reference/enhanced and stereo/Spatial combinations remain independent, and both generated hosts preserve protected stereo when source/route/renderer evidence fails;
- both `.fb2k-component` archives pass their exact payload/PE/runtime verifier;
- the final outer bundle passes manifest, SHA-256, architecture, naming and re-extracted component verification;
- `tests/test_no_live_vgmspc_dependency.py` passes.

GitHub Actions has so far failed before executing the Windows job (`steps: []`, with no downloadable job log), so this execution proof has not occurred. That infrastructure failure is not counted as a code failure and is the only remaining reason this ledger does not authorize destructive deletion yet.
