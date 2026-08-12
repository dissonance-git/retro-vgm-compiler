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

### Reference emulation and device behavior

- `dgrfactory/spcplay`: current SNESAPU behavior and SPC internals
- `ValleyBell/libvgm`: VGM playback/device architecture
- `nukeykt/Nuked-OPN2` and related Nuked cores: high-fidelity FM/PSG reference implementations
- `aaronsgiles/ymfm`: reusable Yamaha FM engine/channel/operator architecture
- `ValleyBell/qsound-hle`: QSound DSP behavior and source-domain spatial architecture
- `munt/munt`: MT-32/CM-32L family synthesis and device behavior
- `nukeykt/Nuked-SC55`: low-level Sound Canvas reference implementation; licensing/ROM restrictions mean research reference unless a legally compatible route is established

### Driver and sequence semantics

- `ValleyBell/SMPSPlay`: Sega SMPS track/instrument/modulation/channel-allocation behavior
- `ValleyBell/GEMSPlay`: GEMS sequence/driver behavior and dynamic FM/PSG/DAC allocation
- `vgmtrans/vgmtrans`: driver-aware recovery of sequence, instrument and sample collections across many game formats
- Hoot / Hoot Archive: broad Japanese-computer driver corpus and examples of running original game music drivers/data inside an emulated target environment
- VGMRips sound-driver documentation: driver identity, file formats, commands, platform-specific extraction and playback knowledge

The Hoot/VGMRips corpus is especially useful for understanding the layer above chip registers: which driver is running, what data it consumes, and how a song is commanded to play. Hoot itself is not treated as a VGM logging reference because historical Hoot logging routes have known timing/emulation limitations.

### Chip execution to musical semantics

- `aikiriao/spc2midi-tsuu`: modern SPC-to-MIDI work that runs an SPC simulator and maps executed S-DSP voice state to note, pitch-bend, pan, volume/expression, sample/program and effect-send concepts
- `aikiriao/spc700`: SPC700/S-DSP simulator used by `spc2midi-tsuu`, including a MIDI-oriented DSP implementation
- historical `spc2midi` by Gigo/Hill: prior art for recovering note-like events from SPC execution
- historical/current `vgm2mid` work by Paul Jensen / ValleyBell: prior art for deriving note events from VGM chip-frequency/register trajectories
- `jkarenko/vgm2midi`: modern readable reference for translating VGM command timing and several chip frequency models into MIDI-like note events

These converters are not the target architecture. MIDI is too small to preserve all FM, PSG, sample, effect and continuous-control semantics. Their value is showing which musical variables can be derived reliably from device execution and where information is lost by forcing the result into MIDI.

### High-level synthesis / symbolic rendering

- `Wohlstand/libOPNMIDI`: OPN2-based MIDI synthesis and bank/instrument handling
- `stuerp/foo_midi`: broad realtime MIDI playback/component architecture and multiple historical synth backends
- `Wohlstand/OPN2BankEditor`: YM2612/OPN-family instrument extraction, conversion and comparison

### Broad game-music execution

- `libgme/game-music-emu`: common playback interface across multiple executable/ripped game-music formats
- `tildearrow/furnace`: multi-system tracker/emulation architecture with extensive chip/channel state and multiple emulator cores
- Hoot: broad PC-88/PC-98/X68000/FM Towns/MSX/arcade/home-system driver execution, including external MIDI-module routes

A research reference does not grant permission to copy code. Check its license before reuse.

## Representation research

The architecture in `docs/musical-execution-model.md` should also be compared with established research on:

- symbolic-music versus audio representations;
- score/audio alignment and performance realization;
- note/performance ontologies;
- auditory scene analysis;
- concurrent and sequential auditory grouping;
- differentiable/controllable synthesis and resynthesis.

The project should borrow established terminology and experimentally useful distinctions rather than invent new names for ordinary concepts.

## Omniphony

Related repository:

- `dissonance-git/Omniphony-Headphones`

Omniphony consumes the enhanced playback result and remains the general headphone-spatial layer. Do not move chip-specific code into it.

## libaural

Related repository:

- `dissonance-git/libaural`

libaural may use executable game-music internal state as ground truth for artificial-hearing research. It is not a mandatory realtime dependency of playback frontends.

The intended relationship is:

```text
VGM Tooling
exact source / driver / synthesis state
        ↓
reference acoustic render
        ↓
libaural
inferred auditory events and streams
        ↓
comparison against known hidden state
```

This lets game-music execution act as a controlled testground for general machine hearing without teaching libaural chip-specific formats.