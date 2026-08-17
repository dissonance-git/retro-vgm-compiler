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
| libvgm resampler parity regression | `tests/integration/libvgm-source/` | migrated |
| old inference singleton / Prism hint bus | current provenance-aware musical-role model | superseded |
| old build/release workflow knowledge | `tools/build_private_foobar_components.ps1` reconstructs, patches, builds, verifies exports/architectures, packages, reopens the final archives, and hashes both components | migrated |
| old SNESAPU/source-player linkage proof | current builder compiles patched x86 SNESAPU first and passes its include/lib paths explicitly to canonical `spcplayer.vcxproj` | migrated |
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

## Current safeguards

The deletion gate is intentionally layered so a late Windows build is confirming binary integration, not discovering basic migration mistakes:

- `tests/test_no_live_vgmspc_dependency.py` rejects clone/fetch URLs, the retired scaffold identifiers, and maintained-code reads of `imports/vgmspc`. The import tree is archival evidence only.
- `tests/spc/test_active_spc_patch_contract.py` pins the active current-parent/current-child graph and the real four-argument pre-BRR `__stdcall` ABI; stale migration helpers cannot silently return to the live chain.
- `tests/spc/test_materialize_foo_snesapu.py` invokes the complete materializer in a temporary directory and inspects the composed result: SPCP v3, both provider exports, studio reconstruction preparation, source transport, native sibling `spcplayer.exe` launch geometry, current Spatial runtime, removal of the dead `m_Enhancer.reset()` state, and lowercase `enhanced` UI wording.
- `tests/vgm/test_enhanced_wording.py` enforces lowercase descriptive `enhanced` across VGM and explicitly selected maintained SPC surfaces while excluding historical anchor literals that guarded patchers must preserve exactly.
- `tools/verify_private_component_packages.py` reopens the final `.fb2k-component` files and accepts only the exact flat sibling payload required by each runtime. It rejects missing/extra entries, nested or traversal paths, case-colliding names, and zero-byte runtime files. `tests/test_private_component_package_contract.py` exercises those failure modes with synthetic archives.
- `tests/test_private_component_builder_contract.py` pins deterministic output paths and asserts pre-package PE machine checks for x64 Omniphony/VGM/SPC component binaries and x86 SNESAPU/spcplayer binaries. It also requires the final archive verifier.
- `tests/private_components/CMakeLists.txt` carries these retirement, wording, materialization, package, builder, 48 kHz, source-transport, and Spatial lifecycle contracts in the same cheap preflight that runs before external dependency compilation.
- `tools/build_private_foobar_components.ps1` contains no `vgmspc` checkout. Historical provenance appears only in its output manifest.

## Destructive deletion gate

The old repository/directory may be deleted after one clean Windows run proves all of the following from a fresh Retro VGM Compiler checkout:

- core/private-component tests pass, including retirement, wording, builder, and package contracts;
- both materializers succeed;
- pinned libvgm source-capture tests pass, including linear resampler parity;
- patched x86 SNESAPU exports `SetDSPSourceCapture`, `GetDSPSourceData`, `SetDSPPreBrrProvider`, and `SetDSPStudioSourceProvider`;
- canonical `spcplayer.exe`, `foo_snesapu.dll`, and `foo_input_vgm.dll` build with the asserted x86/x64 identities;
- Omniphony ABI validation succeeds;
- reference/enhanced and stereo/Spatial fallbacks remain independent and fail closed;
- both `.fb2k-component` archives pass the final payload verifier with their required sibling runtime dependencies;
- `tests/test_no_live_vgmspc_dependency.py` passes.

GitHub Actions has so far failed before executing the Windows job (`steps: []`, with no downloadable job log), so this execution proof has not occurred. That infrastructure failure is not counted as a code failure and is the only remaining reason this ledger does not authorize destructive deletion yet.
