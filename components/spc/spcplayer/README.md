# source-aware spcplayer

This directory is the canonical Retro VGM Compiler home for the 32-bit `spcplayer.exe` child used by the foobar SPC component.

It was mined from `dissonance-git/vgmspc` at the SRCE-v2 transport cut `2b7ec8bbd7326eabee3ba39bb91130b9b128e74b` (`spc: splice framed source packets across foobar requests`). The executable behavior preserved here is the useful process seam, not the old repository layout:

```text
SPCP input
    ↓
patched SNESAPU reference render
    ↓
reference PCM
    + optional TLEM block-end evidence
    + optional SRCE v2 causal source/control planes
    ↓
stdout framed stream consumed by the x64 foo_snesapu parent
```

## One SRCE v2 ABI

The historical child duplicated SRCE v2 constants and the 24-byte packet header in `spcplayer.h`. That duplication has been removed. Both the child and the current parent-side transport now use:

```text
components/spc/snesapu_source_wire_v2.h
```

`spcplayer.h` retains compatibility names only so the historical parent controller can be migrated without creating a second wire definition.

## SNESAPU is an explicit build dependency

The old `vgmspc` tree checked in `spcplayer/lib/Win32/snesapu.lib`. That binary is deliberately not migrated. `spcplayer.vcxproj` requires:

- `SNESAPUIncludeDir`: headers from the exact SNESAPU source tree being built;
- `SNESAPULibDir`: directory containing the freshly generated `snesapu.lib` from that same patched build.

Example:

```text
msbuild components/spc/spcplayer/spcplayer.vcxproj \
  /p:Configuration=Release \
  /p:Platform=Win32 \
  /p:SNESAPUIncludeDir=<patched-spcplay>/SNES/SNESAPU \
  /p:SNESAPULibDir=<patched-spcplay>/build/lib/Win32
```

The eventual Windows integration workflow owns the exact staging paths. The invariant is that the child must link against the same patched SNESAPU build whose source-capture exports are verified, never a stale repository binary.

## License and provenance

The migrated `spcplayer` source retains its Mozilla Public License 2.0 notices. The preserved historical migration ledger records why this code moved and which portions of the old project were intentionally not carried forward.
