# Import manifest

These are the exact inputs supplied when the repository was initialized.

## VGM component source

- File: `foo_input_vgm.7z`
- Repository copy: `imports/foo_input_vgm.7z`
- SHA-256: `93d71695fdad062dee47aefa3f857683e4a057302d1a069958eecf5dd18c60ff`
- Extracted source tree observed locally: 41 files, approximately 340 KiB
- License in supplied source: Mozilla Public License 2.0

The archive identity is immutable, but the current GitHub transport copy is known to be truncated and is not accepted as a build input. Canonical Windows builds recover `foo_input_vgm_v0.30.7z` from `https://uu.getuploader.com/foobar2000/download/248` into disposable build state and require the SHA-256 above before extraction. This is a recovery route for the same audited source object, not permission to substitute v0.31 or another release. Normal development occurs from the expanded source under `components/vgm/`; the historical archive is never edited or repacked.

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
