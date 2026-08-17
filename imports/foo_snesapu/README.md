# foo_snesapu parent bootstrap

This directory is the immutable historical parent-host seed used to reconstruct the private `foo_snesapu` component without consulting the former `dissonance-git/vgmspc` repository at build time.

## Provenance

- migration source repository: `dissonance-git/vgmspc`
- source commit: `2b7ec8bbd7326eabee3ba39bb91130b9b128e74b`
- source commit message: `spc: splice framed source packets across foobar requests`
- source parent tree: `foo_snesapu/foobar2000/foo_snesapu`
- source tree SHA: `a3cc09f2508cda3a3a1fa9d6b558bd430a663dc5`

That source cut is deliberate. It contains the proven x86-child to x64-parent `[PCM][TLEM][SRCE-v2]` process transport before later presentation experiments. Current source quality and Spatial behavior are applied afterward from `patches/snesapu/` and current `components/spc/` code.

## Fidelity boundary

The files mutated by the current guarded patch chain are retained byte-for-byte. Their Git blob SHA in this repository is the same as at the pinned source commit:

| file | historical Git blob SHA |
|---|---|
| `input_snesapu.cpp` | `e25b123bf71e64e14bfa89169d2f2d8caf8c38c9` |
| `input_snesapu.hpp` | `f1c3dad16bf6a0abd7eeccc13cde28ba472f70c7` |
| `preferences_snesapu.cpp` | `c2664d0eeba34eb3226dc141b5e20429194e9bfd` |
| `resource.h` | `37b84caec9c008e12aa6056c62d649872a222a8b` |
| `resource.rc` | `7f21b4b05423ad28992bab359edc2603c0e480e7` |
| `spc_source_block.h` | `1da3554a2498e30437cef394c724fb9a06f57163` |
| `spcplayer_controller.cpp` | `78c3e765faa893c7dff24665fe52a09fbf004c25` |
| `spcplayer_controller.h` | `5b65bcf43e6be92a6aedbf2929bfb5a712d69ce9` |
| `foo_snesapu.vcxproj` | `d64b1d482e6ad665ab5fe0edc68f52d95fd99a24` |

Additional exact historical source retained here includes `PCH.cpp`, `stdafx.h`, `id666.hpp`, `idex666.hpp`, `script700.hpp`, `foo_snesapu.txt`, `snesapu/APU.h`, `snesapu/SPC700.h`, and `snesapu/Types.h`.

`script700.cpp` is retained as a UTF-8 semantic normalization because the connector normalizes its historical Japanese source encoding when exporting it. `snesapu/DSP.h` and `snesapu/SNESAPU.h` are compile-only ABI normalizations: they preserve the declarations, constants and structures consumed by the x64 parent, while the actual SNESAPU implementation is rebuilt independently from the pinned SPCPlay source. None of these three files is a guarded patch anchor.

## Deliberate omissions

The bootstrap does **not** copy the old repository wholesale. The following are intentionally absent:

- `MemoryModule/` and the obsolete in-process x86 component path;
- `foo_snesapu_x86_old.*`;
- IDE-only `.filters` files;
- `role_classifier_snes.h`, `semantic_mixer_71.h`, `semantic_role.h`, `semantic_stereo_enhancer.h`, `spc_adapter.h`, and the old singleton inference bus, all superseded by the current source/evidence model;
- checked-in `spcplayer/lib/Win32/snesapu.lib` and other generated binaries;
- copied foobar2000 SDK, WTL, libvgm, SPCPlay and Omniphony dependency trees.

The canonical build reconstructs external dependencies separately and rebuilds generated libraries. `tools/materialize_foo_snesapu.py` is the supported path from this bootstrap to a patchable component source tree.

## Retirement condition

This bootstrap is provenance, not an alternate implementation. New work belongs in `components/spc/`, `patches/snesapu/`, `model/`, or tests. The old migration-source repository is deletable only after the fresh Windows build/package proof succeeds with no external read from that repository.
