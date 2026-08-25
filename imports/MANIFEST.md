# Import manifest

`imports/` contains immutable external inputs required to reproduce repository-owned build, analysis, and forensic paths. Imported evidence is not an editable working tree.

## foo_input_vgm 0.31 bootstrap

The canonical VGM foobar2000 bootstrap is the exact `foo_input_vgm` 0.31 source archive.

```text
canonical archive size     66,250 bytes
canonical archive SHA-256  e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1
ZIP entries                43
source files               41
component version marker   0.31
license in supplied tree   Mozilla Public License 2.0
```

The archive is stored as immutable base64 transport parts under:

```text
imports/bootstrap/foo_input_vgm-0.31.base64-parts/
```

That directory represents one canonical source object. `tools/reconstruct_vgm031_bootstrap.py` concatenates the parts in a fixed order, verifies the base64 length and hash, decodes the archive, verifies archive length, SHA-256, ZIP structure, and version marker, then writes the disposable ignored path:

```text
imports/foo_input_vgm-0.31.zip
```

VGM component development starts from the verified bootstrap, applies project-owned overlays from `components/vgm/`, and applies guarded transformations from `patches/foo_input_vgm/`. The reconstructed archive remains immutable input evidence.

## SPCPlay / SNESAPU behavioral reference

The SPCPlay/SNESAPU package is behavioral reference evidence used to reconcile editable SNESAPU source.

```text
package               spcplay-2.21.3.9130.zip
SHA-256               bd778800295dac01297934ade357a01b4326510ac335822e84615cdbe735123c
reported release      2.21.3
build date            2026-07-23
license in readme     GNU GPL v2.0
```

Known contents include `readme.txt`, `snesapu.dll`, and `spcplay.exe`. Its high-quality capabilities include high sample rates, float output, sinc interpolation, and higher-precision echo/FIR processing. Those behaviors are reference evidence for source reconciliation and source-native enhancement work.

Binary/reference packages remain evidence objects. Editable upstream source retains independent provenance and is never silently replaced by a reference binary.
