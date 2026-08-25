# Import manifest

`imports/` contains preserved external inputs required to reproduce repository-owned build, analysis, or forensic paths. Imported evidence is not an editable working tree. Superseded migration material belongs in Git history rather than a permanent active shelf.

## foo_input_vgm 0.31 bootstrap

The canonical VGM foobar2000 bootstrap is the exact user-supplied `foo_input_vgm` 0.31 source archive.

```text
canonical archive size     66,250 bytes
canonical archive SHA-256  e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1
ZIP entries                43
source files               41
component version marker   0.31
license in supplied tree   Mozilla Public License 2.0
```

Because the repository connector originally could not publish the binary archive safely, the exact archive is represented as immutable base64 transport parts under:

```text
imports/bootstrap/foo_input_vgm-0.31.base64-parts/
```

That directory is **transport for one canonical source object**, not a second source tree. `tools/reconstruct_vgm031_bootstrap.py` concatenates the parts in a fixed order, verifies the base64 length/hash, decodes the archive, verifies archive length/SHA-256/ZIP structure/version marker, and writes the disposable ignored path:

```text
imports/foo_input_vgm-0.31.zip
```

Current development occurs from the verified 0.31 bootstrap plus project-owned overlays under `components/vgm/` and guarded transforms under `patches/foo_input_vgm/`. Never semantically edit the reconstructed archive in place.

The older 0.30 bootstrap and the retired `vgmspc` migration ledgers remain recoverable from Git history but no longer occupy the active import shelf or participate in current builds.

## SPCPlay / SNESAPU behavioral reference

The supplied SPCPlay/SNESAPU package is a behavioral reference used to reconcile editable SNESAPU source, not the editable source tree itself.

Recorded reference:

```text
package               spcplay-2.21.3.9130.zip
SHA-256               bd778800295dac01297934ade357a01b4326510ac335822e84615cdbe735123c
reported release      2.21.3
build date            2026-07-23
license in readme     GNU GPL v2.0
```

Known contents include `readme.txt`, `snesapu.dll`, and `spcplay.exe`. The improved SNESAPU path exposes higher-quality options such as high sample rates, float output, sinc interpolation, and higher-precision echo/FIR processing; those are reference evidence when reconciling editable source before VGM Compiler adds its own source-native enhancement layer.

Binary/reference packages remain evidence objects. Editable upstream source must retain clear provenance and must not be silently replaced by a reference binary.
