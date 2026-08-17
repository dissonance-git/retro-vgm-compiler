# vgmspc retirement execution gate

The source migration is structurally complete, but `dissonance-git/vgmspc` must
not be destructively deleted until the Windows execution gate below succeeds from
a clean Retro VGM Compiler checkout.

Run from the repository root on Windows with Visual Studio 2022 C++ x86/x64
tools, Python, CMake/CTest, Git, 7-Zip, NASM, Rustup and Cargo available:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_vgmspc_retirement_gate.ps1
```

Optional isolated work/output locations can be supplied with `-WorkRoot` and
`-OutputRoot`.

## What one successful run proves

The wrapper first runs `tools/build_private_foobar_components.ps1`, whose initial
preflight proves the exact committed source state, full dependency-free root core
suite, the nine SNESAPU causal-source contracts, VGM/SPC materializer composition,
playback-option orthogonality, fail-closed Spatial replacement ordering, naming,
Omniphony ABI/loader contracts, package/bundle contracts and the no-live-vgmspc
dependency rule.

The builder then reconstructs the official/pinned external dependencies, patches
and builds the exact pinned libvgm tree, runs the external libvgm source/resampler
regression against that same patched tree, builds Omniphony, VGM, patched x86
SNESAPU, x86 spcplayer and x64 foo_snesapu, and verifies architecture, exports,
private PE import boundaries, archive payloads, runtime sibling layout, packaged
Omniphony ABI, packaged spcplayer startup, source provenance, hashes and final
bundle metadata.

After that canonical build succeeds, the retirement wrapper extracts the exact
packaged SPC component and compiles
`tests/integration/snesapu-runtime/snesapu_provider_export_smoke.cpp` as x86. The
smoke loads the packaged `SNESAPU.dll`, resolves `SetDSPPreBrrProvider` and
`SetDSPStudioSourceProvider` by name using the exact stdcall signatures, and calls
their null-provider reset operations. This proves the newly patched provider ABI
is not merely present in the export table but executable through a real x86
process.

Finally the wrapper re-runs the outer bundle verifier on the exact final ZIP.
Only then does it print:

```text
vgmspc retirement execution gate PASSED.
```

The gate never deletes a repository, local checkout, or archive automatically.
Destructive deletion is a separate human action after the successful proof.

## Current status

**Not yet executed successfully.** GitHub hosted Windows jobs have so far failed
before runner assignment (`steps: []`, no downloadable job log), so they provide
no evidence either for or against the code. Until a real Windows execution reaches
and passes the command above, `vgmspc` remains read-only migration evidence.
