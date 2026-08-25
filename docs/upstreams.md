# Upstream and reference registry

This document records the external sources, exact build pins, specifications, implementations, and research observatories that define VGM Compiler's **current evidence surface**.

A reference is not automatically a runtime dependency. Code, standards, manuals, preservation tools, papers, and emulators expose different strata of the same problem. VGM Compiler uses them to constrain claims while keeping its shared semantic model project-owned.

```text
primary specification / source / measurement
        ↓
source-family implementation evidence
        ↓
independent implementation controls
        ↓
project-owned semantic model
```

Conceptual usefulness never grants permission to copy code. Exact pins below are evidence identities, not claims that one upstream defines a universal ontology.

## Canonical build inputs

### foo_input_vgm 0.31

The canonical foobar2000 VGM component bootstrap is the exact `foo_input_vgm` 0.31 archive represented under:

```text
imports/bootstrap/foo_input_vgm-0.31.base64-parts/
```

Canonical archive SHA-256:

```text
e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1
```

`tools/reconstruct_vgm031_bootstrap.py` verifies and materializes the archive. `tools/materialize_foo_input_vgm.py` combines that immutable input with project-owned source under `components/vgm/` and guarded transformations under `patches/foo_input_vgm/`.

The supplied component source is MPL-2.0. Preserve the license obligations of covered files and modifications.

### libvgm

Primary VGM playback/device dependency:

```text
ValleyBell/libvgm
commit 64e1de284e9a4305c54dd162ee8c33539a9bc0d1
```

The private build applies the maintained source-observation patch stack under `patches/libvgm/` and runs `tests/integration/libvgm-source/` against that exact patched tree before compiling the VGM component.

libvgm is a playback/device implementation observatory. It does not override VGM format semantics defined by the format specification.

### WTL

```text
Win32-WTL/WTL
commit d1cd80e9ce76c4d79da4cf556401ad7a970ce46f
```

Used by the current private foobar2000 component build.

### foobar2000 SDK

```text
release 2025-03-07
https://www.foobar2000.org/downloads/SDK-2025-03-07.7z
```

The private builder validates exact project identities before use.

### SPCPlay / SNESAPU

Editable source and runtime observatory:

```text
dgrfactory/spcplay
commit fc770e268ecacb4523699e2edc5c0efdf80957d6
license GPL-2.0
```

VGM Compiler patches and builds the source directly. The reference package recorded in `imports/MANIFEST.md` is behavioral evidence, not a substitute for editable source.

### Omniphony

```text
dissonance-git/Omniphony-Headphones
commit c5ff2988e2b088dc200f9ca76032f3b452706262
Rust toolchain 1.88.0
```

Omniphony owns general spatial presentation. VGM Compiler owns source identity, source-supported routing, and game-music semantics. The private build validates the source FFI ABI before packaging.

## VGM format authority

Canonical format reference:

- VGMRips VGM Specification: `https://vgmrips.net/wiki/VGM_Specification`

Use the specification as primary authority for:

- header fields and version gates;
- clock fields and dual-chip flags;
- command bytes and operand widths;
- data blocks and DAC Stream Control;
- wait/sample accounting;
- loop and GD3 structure;
- reserved-field semantics.

The evidence boundary is:

```text
VGM bytes
→ format semantics
→ chip/device state
→ performed behavior
→ musical inference
```

An emulator or player is an independent implementation control, not permission to redefine a byte already specified by the format.

## Hardware and development documentation

Official manuals, application notes, schematics, and development-system documents are primary evidence where applicable. High-value material includes:

- Yamaha YM2612 documentation;
- SN76489 documentation;
- Mega Drive development-system material;
- Z80/68000 programming documentation;
- console and arcade schematics;
- board and chip revision documentation.

Sega Retro is a useful preservation index for many of these documents. Prefer the underlying official artifact over a wiki summary when the primary document is available.

These sources constrain:

- register terminology and ordering;
- clocks, buses, memory maps, and DAC behavior;
- documented programmer-facing behavior;
- hardware-revision distinctions;
- which mechanisms were officially exposed versus established through later measurement or reverse engineering.

Manuals complement rather than replace emulator, die-analysis, driver-source, and measurement evidence.

## Device and synthesis observatories

### Yamaha and related synthesis

- `ValleyBell/libvgm`: multi-chip playback/device architecture.
- `nukeykt/Nuked-OPN2` and related Nuked cores: high-fidelity FM/PSG reference implementations.
- `aaronsgiles/ymfm` at `81aec25ccbb98f4873a255f7551ac4dadac59b4a`: Yamaha FM engine/channel/operator comparison surface.
- `Wohlstand/libOPNMIDI`: OPN-family synthesis and instrument-bank handling.
- `Wohlstand/OPN2BankEditor`: OPN-family patch extraction, conversion, and comparison.

Similarity among OPN, OPM, OPL, and OPLL is admitted into shared semantics only when independently different implementations support the same relation. Register layout similarity alone is insufficient.

### QSound

- `ValleyBell/qsound-hle`: QSound DSP behavior and source-domain spatial architecture.

QSound is especially valuable for distinguishing pre-pan source identity from the final stereo mix.

### SNES / SPC

- `dgrfactory/spcplay`: SPC700/S-DSP execution and SNESAPU behavior.
- `aikiriao/spc700`: executable SPC700/S-DSP simulation used by higher-level recovery tools.

The project keeps SPC execution, source/sample identity, DSP state, and inferred musical parts as separate layers.

### Other device/system controls

- `munt/munt`: MT-32/CM-32L synthesis and device behavior.
- `nukeykt/Nuked-SC55`: low-level Sound Canvas reference implementation, subject to its ROM/licensing constraints.
- MAME: system-context device execution and hardware behavior beyond isolated music-player abstractions.

## xSF container and platform observatories

The shared xSF envelope is intentionally smaller than the platform runtime underneath it.

Current comparison sources include:

- `kode54/psflib` at `95509e0c6f13d769593bbf51a1b0e0efdc355ba1`: xSF envelope, tags, CRC, and library traversal.
- `kode54/viogsf` at `6c43a9926a6a85fbb736ea8f5f7f6c4f59ed3d64`: GSF/GBA execution.
- `loveemu/gsfopt` at `41538ea8bb9e3087f0c485e937467ed6b354f7b6`: GSF upload headers, address mapping, dependency overlay, and ROM reconstruction.
- `loveemu/saptapper` at `ff7ec3e4da1f1ffc3bcc05793268036a319b4466`: GSF construction and entry/load/size header emission.
- `ipatix/agbplay` at `0960aadec72dddbefc144216886d86bef220a0bb`: independent GBA/GSF execution control.
- `kode54/lazyusf2` at `421f00bcaa1988b8e1825e91780129f24fbd1aa0`: USF/N64 ROM and save-state upload semantics.
- `RGBA-CRT/vio2sf-fork` at `cbad66408b72d3bdc9f6c5ba724fe3e17f996865`: 2SF/Nintendo DS ROM-map and reserved `SAVE` semantics.
- `CyberBotX/NCSF` at `fe1b91afec25fe18a10fe1697f95341e8dd5a44d`: NCSF construction, SDAT structure, sequence selection, SSEQ/SBNK/SWAR, and PLAYER behavior.
- `CyberBotX/in_xsf` at `74fceae1f09f2e42afff4f71fb68b1952f494916`: independent GSF/NCSF mapping control.
- `fincs/FSS` at `c0f69d6105e8877ca8ff3b929d230e49e05726c7`: Nintendo DS sequence/player implementation control.
- `vgmtrans/vgmtrans` at `083f7c71fe773078061eb785573621082c3e0d1c`: independent PS1 AKAO and Nintendo DS SDAT structure observatory.

These support a small shared container/dependency mechanism plus separate platform effective-object loaders. They do **not** support one shared runtime, driver model, sequence model, voice model, or playback claim.

## Driver and sequence semantics

Useful execution/source observatories include:

- `ValleyBell/SMPSPlay`: Sega SMPS track, instrument, modulation, and channel-allocation behavior.
- `ValleyBell/GEMSPlay`: GEMS sequence/driver behavior and dynamic FM/PSG/DAC allocation.
- `vgmtrans/vgmtrans`: driver-aware sequence, instrument, and sample recovery across many formats.
- Hoot / Hoot Archive: Japanese-computer and arcade driver execution/data corpora.
- VGMRips driver documentation and technical discussions: driver identity, command semantics, extraction, playback, and obscure platform details.

These are especially useful for the layer above chip registers:

```text
driver identity
+ command/control flow
+ sequence/instrument/sample data
→ executable musical behavior
```

A driver corpus is not automatically a format or timing authority. Claims remain scoped to the evidence source that supports them.

## Execution-to-music recovery controls

Converters are useful because they expose which musical variables can be recovered from live device execution and where a smaller target representation loses information.

Useful controls include:

- `aikiriao/spc2midi-tsuu`: SPC execution to note/pitch-bend/pan/volume/program/effect-send concepts.
- `aikiriao/spc700`: execution substrate used by SPC recovery work.
- `vgm2mid` implementations by Paul Jensen / ValleyBell.
- `jkarenko/vgm2midi`: readable VGM timing and chip-frequency-to-note conversion reference.

MIDI is not the target architecture. It cannot preserve the full FM, PSG, sample, effect, control, provenance, or structural state available in richer executable sources.

```text
rich execution evidence
→ musical projection
→ MIDI-like output
```

The projection may be useful while still being strictly smaller than the evidence that produced it.

## Broad replay and integration observatories

- `libgme/game-music-emu`: compact common playback interface across several executable/ripped formats.
- `OpenMPT/openmpt` / libopenmpt: tracker/module playback with pattern, row, channel, subsong, and module-state access.
- `tildearrow/furnace`: multi-system tracker/emulation architecture with chip/channel state and multiple cores.
- Hoot: multi-platform driver execution.
- `yoyofr/modizer`: integration of a broad set of replay engines including libvgm, GME, libopenmpt, Furnace, UADE, sidplayfp, NSFPlay, PMD/MDX, xSF players, and vgmstream.

Their combined lesson is capability-aware abstraction:

```text
shared frontend
!= shared semantic depth
```

A common playback operation can be useful while richer source families retain source-specific structure. VGM Compiler should not fabricate semantic parity merely because two sources share a player API.

## Music representation observatories

These systems pressure-test the layers above device execution.

### Symbolic / score

- music21: computational musicology and higher musical relations.
- Partitura: score/performance representation, voices, time points, and mappings among musical time coordinates.
- MEI: structured notation plus metadata/provenance relationships.
- MusicXML and Humdrum: additional interchange/analysis comparison surfaces.

They expose requirements for meter, voice, phrase, harmony, form, score identity, and related upper-layer structures without making those structures device state.

### Computer-assisted composition

- OpenMusic: explicit musical objects, constraints, transformations, and compositional/analytical structure.

See `docs/openmusic-libraries.md` and `docs/music-representation-systems.md`.

### DAW / session / graph systems

- LMMS: arrangement, pattern, automation, instrument, mixer, and effect distinctions.
- Ardour: session identity, regions/playlists, tempo/meter maps, routing, buses, and processing topology.
- AudioKit: node graphs, runtime object identity, parameter state, and sample-offset event scheduling.

These reinforce that a final mix is a projection of a larger execution graph rather than a complete representation of the musical object.

### Inverse analysis

- `spotify/basic-pitch`: frame/onset/pitch-contour evidence followed by a smaller note/MIDI projection.
- chord/progression corpora: useful fixtures for harmony, transposition, and analytical robustness when provenance is appropriate.

### Algorithmic sequencing

- `hundredrabbits/Orca`: executable pattern/state generation and event-network behavior.

This is a useful pressure test for music whose future events are generated by running state and control flow rather than stored as a pre-expanded note list.

## Audio-programming-language observatories

Useful comparisons include:

- MPEG-4 Structured Audio / SAOL / SASL: synthesis definition, score/control, samples, MIDI-like control, and scheduler separation.
- SuperCollider: instrument definitions, running synth instances, unit-generator graphs, buses, and execution order.
- Max/MSP and Pure Data: explicit message/control versus signal-rate topology.
- Csound: initialization, control-rate, and audio-rate computation.
- ChucK: explicit logical time and concurrent temporal execution.
- Faust: semantic DSP/signal graphs independent of generated target code.
- Cmajor: distinct stream, event, and persistent-value endpoints.
- TidalCycles: symbolic patterns as time-domain processes.
- Sonic Pi and related live-coding systems: evolving program state during musical time.
- Alda, ABC, LilyPond, and related authored text/notation systems where their structural distinctions are useful.

See `docs/audio-programming-languages.md`.

## Research literature

Literature is treated like source code and specifications: extract established distinctions, formal results, and falsifiable test ideas rather than importing an ontology wholesale.

High-value references include:

- Roger B. Dannenberg, **Music Representation Issues, Techniques, and Systems**, *Computer Music Journal* 17(3), 1993, DOI `10.2307/3680940`.
- Adriano Baratè, Goffredo Haus, Luca A. Ludovico, **Music Representation of Score, Sound, MIDI, Structure and Metadata All Integrated in a Single Multilayer Environment Based on XML**, 2008, DOI `10.4018/978-1-59904-663-1.CH014`.
- Diogo Cocharro et al., **A Review of Musical Rhythm Representation and (Dis)similarity in Symbolic and Audio Domains**, 2021, DOI `10.1007/978-3-030-78451-5_10`.
- Roger B. Dannenberg and Christopher Raphael, **Music Score Alignment and Computer Accompaniment**, DOI `10.1184/r1/6607616`.
- Carlos E. Cancino-Chacón et al., **Computational Models of Expressive Music Performance: A Comprehensive and Critical Review**, 2018, DOI `10.3389/FDIGH.2018.00025`.
- Johanna Devaney, Daniel McKemie, Amy L. Morgan, **pyAMPACT**, 2024, arXiv `2412.05436`.
- Akira Maezawa and Hiroshi G. Okuno, **Bayesian Audio-to-Score Alignment Based on Joint Inference of Timbre, Volume, Tempo, and Note Onset Timings**, 2015, DOI `10.1162/COMJ_A_00286`.
- Zeyu Jin and Roger B. Dannenberg, **Formal Semantics for Music Notation Control Flow**, ICMC 2013.
- Florent Jacquemard and Clément Poncelet Sanchez, **Antescofo Intermediate Representation**, 2014, arXiv `1404.7335`.
- James McDermott and Una-May O'Reilly, **An Executable Graph Representation for Evolutionary Generative Music**, GECCO 2011, DOI `10.1145/2001576.2001632`.
- Andreas Arzt and Gerhard Widmer, **Towards Effective 'Any-Time' Music Tracking**, 2010, DOI `10.3233/978-1-60750-676-8-24`.

Current architectural consequences:

1. static program/control flow and realized execution are distinct;
2. mappings among authored, driver, device, sample, and acoustic time are explicit evidence and may be piecewise;
3. competing high-level interpretations remain alternatives until evidence discriminates among them;
4. multiple linked representation layers are preferable to flattening the musical object into MIDI, notation, or PCM;
5. performance timing, dynamics, intonation, and articulation are trajectories rather than mere note labels;
6. executable control flow can be semantically important even when the resulting note sequence is superficially similar.

Ongoing literature comparison should cover auditory scene analysis, music cognition, MIR, computational musicology, score-informed source separation, controllable synthesis, provenance-aware preservation, and interactive/adaptive game music.

## Source use and licensing

For every external implementation:

1. record the question it helps answer;
2. inspect its license before reuse;
3. distinguish conceptual evidence from imported implementation;
4. preserve attribution and licenses for legitimately imported code;
5. prefer project-owned implementation for shared execution/reasoning semantics;
6. keep source-specific upstream code source-specific rather than laundering it into a universal abstraction.

The goal is a stronger evidence model, not a dependency collage.

## Cross-project boundaries

### Omniphony

`dissonance-git/Omniphony-Headphones` owns general spatial presentation. VGM Compiler supplies source identity and source-supported route/trajectory evidence. Chip-specific behavior stays in VGM Compiler.

### libaural

`dissonance-git/libaural` owns general artificial-hearing research. VGM Compiler can provide exact hidden game-music execution state as a controlled truth surface:

```text
VGM Compiler exact source / driver / synthesis state
        ↓
reference acoustic render
        ↓
libaural inferred auditory events / streams
        ↓
comparison against known hidden state
```

libaural does not need chip-specific formats to benefit from game-music execution as a controlled machine-hearing experiment.
