# Upstream provenance

This file records the starting sources and reference builds used by this repository.

## VGM foobar2000 component

Initial source supplied for this repository as `foo_input_vgm.7z`.

Imported source path:

`components/vgm/foo_input_vgm/`

The supplied component source carries the **Mozilla Public License 2.0** in its `LICENSE` file. Preserve that license with covered files and modifications.

The project expects libvgm plus foobar2000 SDK/support projects according to its Visual Studio project files. The imported wrapper is the editable foobar component baseline; libvgm should be tracked deliberately rather than copied anonymously into the wrapper tree.

Primary upstream library:

- ValleyBell/libvgm

VGM/VGZ is the enhancement design center. Existing GYM, DRO, and S98 input code is retained as upstream compatibility code but is not an enhancement priority.

## SPC foobar2000 component

Legacy component:

- `foo_snesapu`
- original developer: kode54 / Christopher Snowhill
- historical source repository: `https://gitlab.com/kode54/foo_snesapu`

The foobar component is a wrapper around SNESAPU.DLL. Its historical public release is old; do not treat that component release date as the desired SNESAPU rendering baseline.

## SNESAPU / SPCPlay

Editable SNESAPU implementation lineage:

- dgrfactory/spcplay
- repository default development branch: `develop`
- project description: SNES SPC700 Player + Improved SNESAPU.DLL
- upstream license: GPL-2.0

Local reference build supplied at repository creation:

- `spcplay-2.21.3.9130`
- files: `spcplay.exe`, `snesapu.dll`, `readme.txt`
- supplied build timestamp: 2026-07-23

Important: the supplied SPCPlay package is a **newer binary/behavior reference**, not a substitute for editable source. The editable SNESAPU source must be brought forward to the behavior represented by the newest supplied files before enhancement changes are trusted.

The initial SNESAPU source import should preserve upstream file history/provenance as clearly as practical.

## Research/reference implementations

These are research inputs, not automatically vendored dependencies.

- `dgrfactory/spcplay`: current SNESAPU behavior and SPC internals
- `ValleyBell/libvgm`: VGM playback/device architecture
- `ValleyBell/SMPSPlay`: higher-level Sega SMPS behavior and channel structure
- `nukeykt/Nuked-OPN2` and related Nuked cores: high-fidelity FM/PSG reference implementations
- `ValleyBell/qsound-hle`: QSound DSP behavior and source-domain spatial architecture

A research reference does not grant permission to copy code. Check its license before reuse.

## Omniphony

Related repository:

- `dissonance-git/Omniphony-Headphones`

Omniphony consumes the enhanced playback result and remains the general headphone-spatial layer. Do not move chip-specific code into it.

## libaural

Related repository:

- `dissonance-git/libaural`

libaural may use SPC/VGM internal state as ground truth for artificial-hearing research. It is not a mandatory realtime dependency of these foobar components.
