# Import manifest

These are the exact source and reference inputs used by the private component build.

## VGM component source

- File: `foo_input_vgm-0.31.zip`
- Build workspace path: `imports/foo_input_vgm-0.31.zip`
- Version marker: `0.31`
- Archive SHA-256: `e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1`
- Component source files: 41
- Component source-tree SHA-256: `36a25ee0cc5d9e8df6c7f7f3f0f06ce305dbbe27dbf8abbc28caa97a8ddb64fc`
- License in supplied source: Mozilla Public License 2.0

The user-supplied 0.31 source tree is the canonical VGM bootstrap for the private foobar build. GitHub text-only connector transport cannot safely publish the binary ZIP directly, so the repository currently also carries exact base64 transfer slices under `.delivery-safe/`. The canonical builder helper reconstructs those slices only when the checked-out ZIP is not already the exact archive above, verifies the base64 transport SHA-256 and final archive SHA-256, and refuses to continue on any mismatch. The transfer slices are transport, not a second source object.

The older `imports/foo_input_vgm.7z` / uploader.jp 0.30 recovery route is retained only as historical repository lineage and is not an accepted private-build input. Normal development occurs from the materialized 0.31 base plus current project-owned overlays and guarded patches; the canonical source archive itself is never semantically edited in place.

## SPCPlay / SNESAPU behavioral reference

- Supplied file: `spcplay-2.21.3.9130.zip`
- SHA-256: `bd778800295dac01297934ade357a01b4326510ac335822e84615cdbe735123c`
- Build files dated: 2026-07-23
- Contents:
  - `readme.txt` — 16,005 bytes
  - `snesapu.dll` — 63,488 bytes
  - `spcplay.exe` — 206,848 bytes
- Reported player/SNESAPU release: v2.21.3
- License stated by the supplied readme: GNU GPL v2.0

This package is a **behavioral reference for source synchronization**, not the editable SPC component source and not the architecture to wrap directly.

The supplied readme documents that the improved SNESAPU path can output up to 32-bit float / 96 kHz and already includes deliberately non-hardware-faithful quality options such as sinc interpolation and higher-precision echo/FIR processing. Those capabilities are useful evidence when reconciling the editable source before this repository adds its own enhancement layer.

The binary ZIP is intentionally treated as a reference artifact. Editable SNESAPU code must retain clear upstream provenance.
