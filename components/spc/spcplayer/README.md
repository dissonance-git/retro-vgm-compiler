# source-aware spcplayer

`spcplayer.exe` is the 32-bit child process used by the private foobar SPC path. It owns the process seam around the patched SNESAPU reference renderer; source semantics themselves remain in `components/spc/` and the SNESAPU patch contract remains in [`../../../patches/snesapu/README.md`](../../../patches/snesapu/README.md).

## Process contract

```text
SPCP setup/input
→ patched SNESAPU reference execution
→ protected reference PCM
   + optional timing evidence
   + optional causal source/control planes
→ framed stdout stream
→ x64 foobar parent
```

The child must not invent musical identity or spatial geometry. It transports exact/bounded runtime evidence produced by the patched source engine.

## SRCE v2 ABI

The canonical wire definition is:

```text
components/spc/snesapu_source_wire_v2.h
```

Both child and parent-side transport use that owner. Compatibility names in `spcplayer.h` exist only for the historical parent-controller surface and must not become a second wire definition.

Versioned transport/wire names are compatibility contracts. Rename them only with complete consumer migration and ABI validation.

## Build dependency

The child links against the same freshly built patched SNESAPU source tree whose source-capture exports are being validated. Do not use a stale checked-in `snesapu.lib`.

Required MSBuild properties:

- `SNESAPUIncludeDir`: headers from the exact patched SNESAPU tree;
- `SNESAPULibDir`: directory containing the matching generated `snesapu.lib`.

Example:

```text
msbuild components/spc/spcplayer/spcplayer.vcxproj \
  /p:Configuration=Release \
  /p:Platform=Win32 \
  /p:SNESAPUIncludeDir=<patched-spcplay>/SNES/SNESAPU \
  /p:SNESAPULibDir=<patched-spcplay>/build/lib/Win32
```

The Windows/private-build workflow owns exact staging paths.

## Provenance

This process seam was recovered from the former `dissonance-git/vgmspc` lineage at the proven SRCE-v2 transport cut. Git history and the immutable import/bootstrap owners retain the migration detail; this README keeps only the current process, ABI, and build obligations.

The source retains its applicable Mozilla Public License 2.0 notices.
