# Upstream and reference evidence registry

This research owner records external sources, exact build pins, specifications, implementations, literature, and observatories that define VGM Compiler's current external evidence surface.

It is not a project-wide semantic contract. Operational code and build tooling decide whether a pinned input is actually consumed; durable semantic law lives in [`../../docs/architecture.md`](../../docs/architecture.md).

A reference is not automatically a runtime dependency. Code, standards, manuals, preservation tools, papers, and emulators expose different strata of the same problem.

```text
primary specification / source / measurement
        ↓
source-family implementation evidence
        ↓
independent implementation controls
        ↓
project-owned semantic model
```

Exact pins below are evidence identities. Conceptual usefulness never grants permission to copy code or to treat one upstream as a universal ontology.

## Build and integration pins

| Input | Current evidence identity | Role |
| --- | --- | --- |
| `foo_input_vgm` | 0.31 archive, SHA-256 `e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1` | foobar2000 VGM bootstrap under `imports/bootstrap/foo_input_vgm-0.31.base64-parts/` |
| `ValleyBell/libvgm` | `64e1de284e9a4305c54dd162ee8c33539a9bc0d1` | primary VGM playback/device implementation control |
| `Win32-WTL/WTL` | `d1cd80e9ce76c4d79da4cf556401ad7a970ce46f` | private foobar2000 component build input |
| foobar2000 SDK | release `2025-03-07` | private component SDK input |
| `dgrfactory/spcplay` | `fc770e268ecacb4523699e2edc5c0efdf80957d6`, GPL-2.0 | editable SPCPlay/SNESAPU source and runtime observatory |
| `dissonance-git/Omniphony-Headphones` | `c5ff2988e2b088dc200f9ca76032f3b452706262`, Rust 1.88.0 | spatial-presentation integration input |

`tools/reconstruct_vgm031_bootstrap.py` verifies the canonical `foo_input_vgm` archive. `tools/materialize_foo_input_vgm.py` combines that immutable input with project-owned source under `components/vgm/` and guarded transforms under `patches/foo_input_vgm/`.

The private build applies the maintained `patches/libvgm/` stack and validates `tests/integration/libvgm-source/` against the exact patched tree. The build itself remains the operational authority for whether these pins are current and usable.

## Format and hardware authority

The canonical VGM format reference is the VGMRips VGM Specification. Use the specification as primary authority for header/version rules, clocks, command bytes and operands, data blocks and DAC Stream Control, wait/sample accounting, loop/GD3 structure, and reserved-field semantics.

```text
VGM bytes
→ format semantics
→ chip/device state
→ performed behavior
→ musical inference
```

An emulator or player is an implementation control, not permission to redefine a byte already specified by the format.

Official manuals, application notes, schematics, development-system documents, and hardware-revision documentation are primary evidence where applicable. High-value classes include Yamaha YM2612 material, SN76489 material, Mega Drive development documentation, Z80/68000 documentation, console/arcade schematics, and board/chip revision material. Preservation indexes are useful routes, but prefer the underlying official artifact when available.

## Device and synthesis observatories

Current comparison surfaces include:

- `ValleyBell/libvgm`: multi-chip playback/device architecture;
- `nukeykt/Nuked-OPN2` and related Nuked cores: high-fidelity FM/PSG controls;
- `aaronsgiles/ymfm` at `81aec25ccbb98f4873a255f7551ac4dadac59b4a`: Yamaha FM comparison surface;
- `Wohlstand/libOPNMIDI` and `Wohlstand/OPN2BankEditor`: OPN-family synthesis and patch handling;
- `ValleyBell/qsound-hle`: QSound DSP and source-domain spatial architecture;
- `dgrfactory/spcplay` and `aikiriao/spc700`: SPC700/S-DSP execution controls;
- `munt/munt`, `nukeykt/Nuked-SC55`, and MAME: additional device/system controls.

Similarity among OPN, OPM, OPL, and OPLL enters shared semantics only when independent evidence supports the same relation. Register-layout resemblance alone is insufficient.

## xSF container and platform observatories

The shared xSF envelope is intentionally smaller than the platform runtime below it.

Current pinned comparison sources:

- `kode54/psflib` `95509e0c6f13d769593bbf51a1b0e0efdc355ba1`;
- `kode54/viogsf` `6c43a9926a6a85fbb736ea8f5f7f6c4f59ed3d64`;
- `loveemu/gsfopt` `41538ea8bb9e3087f0c485e937467ed6b354f7b6`;
- `loveemu/saptapper` `ff7ec3e4da1f1ffc3bcc05793268036a319b4466`;
- `ipatix/agbplay` `0960aadec72dddbefc144216886d86bef220a0bb`;
- `kode54/lazyusf2` `421f00bcaa1988b8e1825e91780129f24fbd1aa0`;
- `RGBA-CRT/vio2sf-fork` `cbad66408b72d3bdc9f6c5ba724fe3e17f996865`;
- `CyberBotX/NCSF` `fe1b91afec25fe18a10fe1697f95341e8dd5a44d`;
- `CyberBotX/in_xsf` `74fceae1f09f2e42afff4f71fb68b1952f494916`;
- `fincs/FSS` `c0f69d6105e8877ca8ff3b929d230e49e05726c7`;
- `vgmtrans/vgmtrans` `083f7c71fe773078061eb785573621082c3e0d1c`.

These support a small shared container/dependency mechanism plus separate platform effective-object loaders. They do not support one shared runtime, driver model, sequence model, voice model, or playback claim.

## Driver and source observatories

Useful mechanism controls include `ValleyBell/SMPSPlay`, `ValleyBell/GEMSPlay`, `vgmtrans/vgmtrans`, Hoot/Hoot Archive, and VGMRips technical documentation/discussions.

```text
driver identity
+ command/control flow
+ sequence/instrument/sample data
→ executable musical behavior
```

A driver corpus is not automatically a format or timing authority. Claims remain scoped to the evidence source that supports them.

Execution-to-music controls include `aikiriao/spc2midi-tsuu`, `aikiriao/spc700`, Paul Jensen / ValleyBell `vgm2mid` implementations, and `jkarenko/vgm2midi`. MIDI is a smaller projection and not the project architecture.

Broad replay/integration controls include `libgme/game-music-emu`, OpenMPT/libopenmpt, Furnace, Hoot, and Modizer. Their shared lesson is only a research hypothesis until promoted:

```text
shared frontend
!= shared semantic depth
```

## Music-representation observatories

Upper-layer systems are tracked in [`music-representation-systems.md`](music-representation-systems.md). Detailed OpenMusic pressure evidence lives in [`openmusic-libraries.md`](openmusic-libraries.md).

Representative comparison families include music21, Partitura, MEI, MusicXML, Humdrum, OpenMusic, LMMS, Ardour, AudioKit, Basic Pitch, symbolic chord/progression corpora, and Orca.

Audio-programming-language pressure evidence currently summarized by this registry includes MPEG-4 Structured Audio / SAOL / SASL, SuperCollider, Max/MSP, Pure Data, Csound, ChucK, Faust, Cmajor, TidalCycles, Sonic Pi, Alda, ABC, and LilyPond. Create a separate research owner only when a distinct experiment requires one.

## Research literature

Representative retained literature includes:

- Roger B. Dannenberg, *Music Representation Issues, Techniques, and Systems*, 1993, DOI `10.2307/3680940`;
- Adriano Baratè, Goffredo Haus, Luca A. Ludovico, *Music Representation of Score, Sound, MIDI, Structure and Metadata All Integrated in a Single Multilayer Environment Based on XML*, 2008, DOI `10.4018/978-1-59904-663-1.CH014`;
- Diogo Cocharro et al., *A Review of Musical Rhythm Representation and (Dis)similarity in Symbolic and Audio Domains*, 2021, DOI `10.1007/978-3-030-78451-5_10`;
- Roger B. Dannenberg and Christopher Raphael, *Music Score Alignment and Computer Accompaniment*, DOI `10.1184/r1/6607616`;
- Carlos E. Cancino-Chacón et al., *Computational Models of Expressive Music Performance: A Comprehensive and Critical Review*, 2018, DOI `10.3389/FDIGH.2018.00025`;
- Johanna Devaney, Daniel McKemie, Amy L. Morgan, *pyAMPACT*, 2024, arXiv `2412.05436`;
- Akira Maezawa and Hiroshi G. Okuno, *Bayesian Audio-to-Score Alignment Based on Joint Inference of Timbre, Volume, Tempo, and Note Onset Timings*, 2015, DOI `10.1162/COMJ_A_00286`;
- Zeyu Jin and Roger B. Dannenberg, *Formal Semantics for Music Notation Control Flow*, ICMC 2013;
- Florent Jacquemard and Clément Poncelet Sanchez, *Antescofo Intermediate Representation*, 2014, arXiv `1404.7335`;
- James McDermott and Una-May O'Reilly, *An Executable Graph Representation for Evolutionary Generative Music*, GECCO 2011, DOI `10.1145/2001576.2001632`;
- Andreas Arzt and Gerhard Widmer, *Towards Effective 'Any-Time' Music Tracking*, 2010, DOI `10.3233/978-1-60750-676-8-24`.

Literature supplies established distinctions and falsifiable test ideas. It does not import an ontology wholesale.

## Source use and licensing

For every external implementation:

1. record the question it helps answer;
2. inspect its license before reuse;
3. distinguish conceptual evidence from imported implementation;
4. preserve attribution and licenses for legitimately imported code;
5. prefer project-owned implementation for shared execution/reasoning semantics;
6. keep source-specific upstream code source-specific rather than laundering it into a universal abstraction.

## Cross-project boundaries

Omniphony owns general spatial presentation. VGM Compiler supplies source identity and source-supported route/trajectory evidence; chip-specific behavior stays here.

libaural owns general artificial-hearing research. VGM Compiler may provide exact hidden execution state and reference renders as controlled truth surfaces without making libaural chip-format-aware.

The purpose of this registry is reproducible evidence and re-entry, not a dependency collage or a second architecture document.