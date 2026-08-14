# Upstream provenance

This file records the starting sources, reference builds, and major research/reference systems used by this repository.

The list is deliberately broader than the runtime dependency set. Game Music Interpreter treats mature repositories, standards, preservation tools, papers, documentation and emulators as **observatories over different strata of game-music execution**. A research reference does not automatically become a dependency, and its code is not copied into the common model merely because its concepts are useful.

## VGM foobar2000 component

Initial source supplied for this repository as `foo_input_vgm.7z`.

Imported source path:

`components/vgm/foo_input_vgm/`

The supplied component source carries the **Mozilla Public License 2.0** in its `LICENSE` file. Preserve that license with covered files and modifications.

The project expects libvgm plus foobar2000 SDK/support projects according to its Visual Studio project files. The imported wrapper is the editable foobar component baseline; libvgm should be tracked deliberately rather than copied anonymously into the wrapper tree.

Primary upstream library:

- ValleyBell/libvgm

VGM/VGZ is the enhancement design center. Existing GYM, DRO, and S98 input code is retained as upstream compatibility code but is not an enhancement priority.

## VGM specification and primary hardware documentation

Format semantics and device semantics are separate evidence layers.

### VGMRips VGM specification

Canonical format reference:

- `https://vgmrips.net/wiki/VGM_Specification`

Use the specification as the primary authority for VGM header fields, version gates, command bytes, operand sizes, chip-clock fields, dual-chip flags, data blocks, DAC Stream Control, wait/sample accounting and loop-field semantics.

Do not use an emulator implementation to redefine a format byte when the specification already defines it. Emulator and player code remain valuable independent controls for whether a parser implements the specification correctly.

Current code derived from this format-level role includes:

- `vgm_format_version.h`;
- `vgm_chip_clock.h`;
- `vgm_yamaha_register_write.h`;
- `vgm_dac_stream_command.h`;
- `tools/vgm_corpus_audit.py`.

### Sega/Yamaha development and hardware documents

Sega Retro preserves scans and archives of official development and hardware material, including categories for Mega Drive official documentation and broader hardware documentation.

High-value documents include material such as:

- the YM2612 manual;
- SN76489 documentation;
- Mega Drive development-system documentation;
- SNASM/Z80/68000 programming material;
- schematics and board-revision documents.

Use the **underlying official manual, application note, schematic, or development document** as primary evidence where applicable. A Sega Retro wiki summary, forum post, or editorial description remains secondary evidence unless independently corroborated.

These documents are especially useful for:

- documented versus later-discovered behavior;
- official register terminology and ordering;
- clocks, buses, memory maps and DAC behavior;
- what programmers were expected to rely on;
- discrete versus integrated hardware-revision distinctions.

They do not replace modern emulator, die-analysis, driver-source or measurement work. Instead they provide another independent observatory that can strengthen or falsify a reverse-engineered claim.

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
- `aaronsgiles/ymfm`: reusable Yamaha FM engine/channel/operator architecture; observed research head during the cross-chip Yamaha pass: `81aec25ccbb98f4873a255f7551ac4dadac59b4a`
- `ValleyBell/qsound-hle`: QSound DSP behavior and source-domain spatial architecture
- `munt/munt`: MT-32/CM-32L family synthesis and device behavior
- `nukeykt/Nuked-SC55`: low-level Sound Canvas reference implementation; licensing/ROM restrictions mean research reference unless a legally compatible route is established
- MAME: whole-machine and device implementations that preserve system context around sound hardware and can expose behavior omitted by isolated music players

The current Yamaha comparison uses ymfm as an implementation observatory rather than as proof that all Yamaha FM families share one register ontology. OPN, OPM, OPL and OPLL similarities graduate only when their independently different register/state implementations support the same semantic relation; explicit negative controls are retained when they do not.

### xSF envelope and platform execution semantics

- `kode54/psflib`, observed at `95509e0c6f13d769593bbf51a1b0e0efdc355ba1`: shared xSF envelope, tags, CRC, and library traversal order
- `kode54/lazyusf2`, observed at `421f00bcaa1988b8e1825e91780129f24fbd1aa0`: USF/Nintendo 64 ROM and Project64 save-state upload semantics
- `RGBA-CRT/vio2sf-fork`, observed at `cbad66408b72d3bdc9f6c5ba724fe3e17f996865`: 2SF/Nintendo DS ROM-map and reserved `SAVE` semantics
- `vgmtrans/vgmtrans`, observed at `083f7c71fe773078061eb785573621082c3e0d1c`: independent PS1 AKAO and Nintendo DS SDAT structure observatory

These references support a small shared container/dependency mechanism and
separate PSF1, USF, and 2SF effective-object loaders. They do not support one
shared runtime, driver model, sequence model, voice model, or playback claim.
See `research/xsf-execution-observatories.md`.

### Driver and sequence semantics

- `ValleyBell/SMPSPlay`: Sega SMPS track/instrument/modulation/channel-allocation behavior
- `ValleyBell/GEMSPlay`: GEMS sequence/driver behavior and dynamic FM/PSG/DAC allocation
- `vgmtrans/vgmtrans`: driver-aware recovery of sequence, instrument and sample collections across many game formats
- Hoot / Hoot Archive: broad Japanese-computer driver corpus and examples of running original game music drivers/data inside an emulated target environment
- VGMRips sound-driver documentation and development forums: driver identity, file formats, commands, platform-specific extraction, playback knowledge, preservation history and obscure implementation details

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

### Broad game-music execution and integration

- `libgme/game-music-emu`: common playback interface across multiple executable/ripped game-music formats; useful both for its shared operations and for the places where capability varies by emulator
- `OpenMPT/openmpt` / libopenmpt: tracker/module playback with richer access to patterns, rows, channels, subsongs and module state than a generic PCM replay boundary
- `tildearrow/furnace`: multi-system tracker/emulation architecture with extensive chip/channel state and multiple emulator cores
- Hoot: broad PC-88/PC-98/X68000/FM Towns/MSX/arcade/home-system driver execution, including external MIDI-module routes
- `yoyofr/modizer`: practical integration of a very large set of replay engines, including libvgm, Game Music Emu, libopenmpt, Furnace, UADE, sidplayfp, NSFPlay, PMD/MDX, XSF-family players, vgmstream and others

Modizer is valuable precisely because its breadth does **not** erase backend differences. Its player layer retains substantial engine-specific state and options. That makes it a useful boundary case for Game Music Interpreter:

```text
shared frontend
≠ shared semantic depth
```

Game Music Emu shows the complementary pattern: a compact common playback API can remain useful while explicitly allowing some features or metadata to be unsupported or unknown for particular emulator types.

OpenMPT shows that richer source families should not be forced down to that least-common-denominator boundary when pattern/structure state is actually available.

Together these systems motivate capability-aware adapters rather than fabricated semantic parity. A permanent Game Music Interpreter capability schema is deferred until concrete adapters require one.

## Music representation and production research

These systems pressure-test the layers above device execution.

### Symbolic and score representations

- music21: computational musicology and higher musical relations above individual note events
- Partitura: score/performance representation, parts, voices, time points and explicit mappings between musical time coordinates
- MEI / Music Encoding Initiative: structured music notation and metadata/provenance representation
- MusicXML and Humdrum: additional notation/analysis interchange references to inspect where materially useful

The purpose is not to make Game Music Interpreter a notation editor. These systems expose requirements for representing meter, voice, phrase, harmony, form, score identity and other musical structures without confusing them with driver/device state.

### Computer-assisted composition

- OpenMusic: explicit musical objects, transformations and compositional/analytical structure above raw event execution

OpenMusic is particularly useful as a pressure test for the upper half of the common model: whether recovered musical objects and relations can remain explicit and transformable while retaining provenance back to executable source truth.

### DAWs, session systems and audio graphs

- LMMS: arrangement, pattern, automation, instrument, mixer and effect distinctions
- Ardour: session identity, regions/playlists, routing, tempo/meter maps, buses and processing topology
- AudioKit: node graphs, runtime object identity, parameters and sample-offset event scheduling

These systems reinforce that a final mix is a projection of a larger graph and that arrangement, automation, synthesis, routing and effects are not properties of a single note list.

### Inverse transcription and analysis

- `spotify/basic-pitch`: useful inverse case where frame/onset/pitch-contour evidence is richer than the eventual MIDI projection
- chord/progression corpora such as `ldrolez/free-midi-chords`: potential fixtures for harmony and transposition tests rather than architecture

The key lesson is that an output format may be smaller than the internal evidence used to derive it. Game Music Interpreter should preserve the richer evidence when source execution provides it.

### Algorithmic sequencing

- `hundredrabbits/Orca`: executable pattern/state generation and event-network behavior

This is useful for game music because future events can be generated by running state and control flow rather than stored as a pre-expanded note list.

See `docs/music-representation-systems.md`.

## Audio programming language research

Audio and synthesis languages expose the machinery between musical instruction and PCM.

Important comparisons include:

- MPEG-4 Structured Audio / SAOL / SASL: synthesis language, score/control language, samples/MIDI semantics and scheduler as separable concepts
- SuperCollider: instrument definitions, running synth instances, unit-generator graphs, buses and execution order
- Max/MSP and Pure Data: explicit control/message versus signal-rate graphs and mutable realtime processing topology
- Csound: initialization, control-rate and audio-rate computation
- ChucK: explicit logical time and concurrent temporal execution
- Faust: semantic DSP/signal graphs independent of generated target code
- Cmajor: distinct stream, event and persistent-value endpoints
- TidalCycles: symbolic patterns as time-domain processes rather than flat note arrays
- Sonic Pi, Extempore/Impromptu and related live-coding systems: dynamic musical programs whose state evolves while time continues
- Alda, ABC, LilyPond and other text/notation languages where they expose useful authored structure

The project does not need their languages or user interfaces. Their value is the execution distinctions they make explicit.

See `docs/audio-programming-languages.md`.

## Representation research and literature

The architecture in `docs/musical-execution-model.md` is also pressure-tested against academic work. Literature is used in the same way as repositories: extract established distinctions, formal results and test cases, not a new dependency stack.

### General and multilayer music representation

- Roger B. Dannenberg, **Music Representation Issues, Techniques, and Systems**, *Computer Music Journal* 17(3), 1993. DOI `10.2307/3680940`. Useful for the longstanding distinction among notation, performance/control information and sound, and for the warning that no single representation is the one true form of music.
- Adriano Baratè, Goffredo Haus and Luca A. Ludovico, **Music Representation of Score, Sound, MIDI, Structure and Metadata All Integrated in a Single Multilayer Environment Based on XML**, 2008. DOI `10.4018/978-1-59904-663-1.CH014`. Useful for the IEEE 1599/MX multilayer model linking logical, structural, notational, performance and audio descriptions.
- Diogo Cocharro, Gilberto Bernardes, Gonçalo Bernardo and Cláudio Lemos Fonteles, **A Review of Musical Rhythm Representation and (Dis)similarity in Symbolic and Audio Domains**, 2021. DOI `10.1007/978-3-030-78451-5_10`. Useful for cross-modal and hierarchical representation questions.

### Score, performance and time alignment

- Roger B. Dannenberg and Christopher Raphael, **Music Score Alignment and Computer Accompaniment**, 2018 repository version DOI `10.1184/r1/6607616`. Useful for symbolic-to-audio event correspondence and the separation of score position from performed time.
- Carlos E. Cancino-Chacón, Maarten Grachten, Werner Goebl and Gerhard Widmer, **Computational Models of Expressive Music Performance: A Comprehensive and Critical Review**, 2018. DOI `10.3389/FDIGH.2018.00025`. Useful for distinguishing notated score from performed timing, dynamics, intonation and articulation.
- Johanna Devaney, Daniel McKemie and Amy L. Morgan, **pyAMPACT: A Score-Audio Alignment Toolkit for Performance Data Estimation and Multi-modal Processing**, 2024. arXiv `2412.05436`. Useful for explicit linking of symbolic notes, aligned audio regions and note-level performance descriptors.
- Akira Maezawa and Hiroshi G. Okuno, **Bayesian Audio-to-Score Alignment Based on Joint Inference of Timbre, Volume, Tempo, and Note Onset Timings**, *Computer Music Journal* 39(1), 2015. DOI `10.1162/COMJ_A_00286`. Useful for treating several score/audio correspondences as uncertain variables rather than assuming a fixed clock mapping.

### Control flow and interactive execution

- Zeyu Jin and Roger B. Dannenberg, **Formal Semantics for Music Notation Control Flow**, ICMC 2013. Useful for treating repeats/endings as static control-flow semantics distinct from the realized performance traversal and for mapping performance location back to static score position.
- Florent Jacquemard and Clément Poncelet Sanchez, **Antescofo Intermediate Representation**, 2014. arXiv `1404.7335`. Useful for a medium-level music-program IR based on finite-state control, variables, delays and concurrency, independent of source syntax and execution platform.
- James McDermott and Una-May O'Reilly, **An Executable Graph Representation for Evolutionary Generative Music**, GECCO 2011. DOI `10.1145/2001576.2001632`. Useful as precedent for executable graph representations and separation of large-scale control from realized outputs.
- Andreas Arzt and Gerhard Widmer, **Towards Effective 'Any-Time' Music Tracking**, 2010. DOI `10.3233/978-1-60750-676-8-24`. Useful for retaining and updating multiple high-level hypotheses in the presence of jumps, repeats, omissions and restarts.

### Current architectural consequences

This literature does not become a new ontology by citation. The current durable consequences are narrower:

1. static program/control flow and realized execution are separate;
2. mappings between score/authored, driver, device, sample and acoustic time are explicit evidence and may be piecewise;
3. competing high-level interpretations should remain alternatives until evidence separates them;
4. multiple linked representation layers are preferable to flattening the entire musical object into MIDI, notation or PCM.

These obligations are reflected in the current musical execution graph and its tests.

The project should continue comparing against research on:

- note/performance ontologies;
- music information retrieval;
- auditory scene analysis;
- concurrent and sequential auditory grouping;
- differentiable/controllable synthesis and resynthesis;
- score-informed source separation;
- music cognition and computational musicology;
- provenance-aware digital music preservation;
- interactive and adaptive game-music systems.

The project should borrow established terminology and experimentally useful distinctions rather than invent new names for ordinary concepts.

## Source use and licensing rule

A research reference does not grant permission to copy code.

For every external implementation:

1. record what question it helps answer;
2. inspect its license before any reuse;
3. distinguish conceptual influence from imported implementation;
4. preserve attribution and original licenses for code that is legitimately imported;
5. prefer independent project-owned implementation for the common execution/reasoning model;
6. keep source-specific upstream code source-specific rather than laundering it into a supposedly universal abstraction.

The goal is to synthesize a stronger model from established evidence, not to assemble a dependency collage.

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
Game Music Interpreter
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
