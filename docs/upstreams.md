# Upstream provenance

This file records the starting sources, reference builds, and major research/reference systems used by this repository.

The list is deliberately broader than the runtime dependency set. VGM Tooling treats mature repositories, standards, preservation tools, papers, documentation and emulators as **observatories over different strata of game-music execution**. A research reference does not automatically become a dependency, and its code is not copied into the common model merely because its concepts are useful.

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
- MAME: whole-machine and device implementations that preserve system context around sound hardware and can expose behavior omitted by isolated music players

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

- `libgme/game-music-emu`: common playback interface across multiple executable/ripped game-music formats
- `tildearrow/furnace`: multi-system tracker/emulation architecture with extensive chip/channel state and multiple emulator cores
- Hoot: broad PC-88/PC-98/X68000/FM Towns/MSX/arcade/home-system driver execution, including external MIDI-module routes
- Modizer and its integrated audio-library set: useful integration archaeology for how many independent replay/emulation engines, formats and system-specific assumptions can coexist behind one practical player

Broad players are valuable because they reveal the **integration boundary**: where supposedly similar engines genuinely share a contract and where source-specific behavior resists normalization. VGM Tooling should learn from those boundaries without reproducing a giant switchboard of opaque third-party engines as its own conceptual model.

## Music representation and production research

These systems pressure-test the layers above device execution.

### Symbolic and score representations

- music21: computational musicology and higher musical relations above individual note events
- Partitura: score/performance representation, parts, voices, time points and explicit mappings between musical time coordinates
- MEI / Music Encoding Initiative: structured music notation and metadata/provenance representation
- MusicXML and Humdrum: additional notation/analysis interchange references to inspect where materially useful

The purpose is not to make VGM Tooling a notation editor. These systems expose requirements for representing meter, voice, phrase, harmony, form, score identity and other musical structures without confusing them with driver/device state.

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

The key lesson is that an output format may be smaller than the internal evidence used to derive it. VGM Tooling should preserve the richer evidence when source execution provides it.

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

The architecture in `docs/musical-execution-model.md` should continue to be compared with established research on:

- symbolic-music versus audio representations;
- score/audio alignment and performance realization;
- note/performance ontologies;
- music information retrieval;
- auditory scene analysis;
- concurrent and sequential auditory grouping;
- differentiable/controllable synthesis and resynthesis;
- score-informed source separation;
- music cognition and computational musicology;
- provenance-aware digital music preservation.

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
