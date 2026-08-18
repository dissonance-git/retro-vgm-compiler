# Genesis driver source ledger

Status: durable provenance and re-entry ledger for the Genesis driver/toolchain quarry.  
Scope: sources, artifacts, and research leads that can reveal semantics above YM2612/SN76489/DAC execution. This file is not a renderer-control table and must never turn game, composer, studio, or driver identity into an audio-control feature.

This ledger exists so research sources do not disappear into chat history. Conceptual conclusions live in `../genesis-driver-dialect-census.md`, `../genesis-open-driver-anatomy.md`, `genesis-authoring-driver-toolchain-quarry.md`, and `genesis-driver-source-vgm-boundary.md`. This file owns the durable source/checkpoint inventory and the next exact experiments enabled by those sources.

## Research question

The remaining gap above accurate Genesis chip execution is often not another register. It is the software and authoring cloud that produced the register stream:

```text
authored source / MML / ASM
        ↓
compiler / assembler / conversion tool
        ↓
driver sequence + instruments + envelopes + samples
        ↓
logical tracks / scheduling / channel arbitration / control flow
        ↓
YM2612 + SN76489 + DAC execution
        ↓
VGM/VGZ device-facing projection
```

Each arrow can erase distinctions. The compiler should recover exact upstream semantics only when evidence survives, otherwise preserve a bounded hypothesis or explicit information gap.

## User-supplied and high-priority Genesis observatories

### Sonic Retro organization

Organization route:

- https://github.com/orgs/sonicretro/repositories

High-value repositories already identified:

- https://github.com/sonicretro/s1disasm
- https://github.com/sonicretro/s2disasm
- https://github.com/sonicretro/skdisasm
- https://github.com/sonicretro/smps-rips
- https://github.com/sonicretro/SMPSPlay-DLL

Why they matter:

- S1 supplies an earlier 68k SMPS branch with sound priorities and its own tempo/update behavior.
- S2 supplies the original Sonic 2 driver state and coordination semantics rather than a later compatibility implementation.
- S&K supplies the Z80 Type 2 DAC branch with richer track state including no-attack, alternate-frequency, pitch-slide, sustain-frequency, modulation/volume envelopes, FM3-special behavior, source voice references, loops/stacks, and SFX override.
- `smps-rips` supplies driver-specific data/configuration alongside songs, instruments, envelopes, drum mappings, PCM material, and driver binaries.
- SMPSPlay-family tooling provides a forward execution/logging observatory for known-source -> VGM controls.

Important law:

```text
S1 SMPS != S2 SMPS != S3/S&K SMPS != later FlameDriver behavior
```

They are related lineages with revision-specific semantics, timing, bugs, capabilities, and data layouts.

### Sonic 2 original and enhanced specimens

Repositories:

- https://github.com/MainMemory/s2-sound-driver-plus
- https://github.com/sonicretro/s2disasm

The Sonic 2 specimen exposes a useful set of driver-native state above chip execution:

- no-attack / retrigger suppression;
- transpose and detune;
- modulation state;
- note-fill master/timeout;
- per-track tempo divider plus global tempo state;
- logical-track control flow and stacks;
- PSG envelope state;
- SFX override;
- DAC/FM6 ownership.

A key derived distinction is that timing and articulation can depend on driver state. `note_fill` cannot be interpreted correctly without the timing domain that scales/counts it.

### FlameDriver lineage and source songs

Repositories:

- https://github.com/flamewing/flamedriver
- https://github.com/TheBlad768/s2disasm-flamedriver
- https://github.com/TheBlad768/Sonic-Clean-Engine-S.C.E.-/tree/flamedriver

`flamewing/flamedriver` is the upstream implementation lineage and contains the driver itself, SMPS2ASM support material, family-specific directories, DAC support, and real ASM song material. The source-song material is especially important because it can provide a known upstream semantic answer key instead of forcing inference from VGM.

High-value paired experiment:

```text
source-native song ASM
        ↓
FlameDriver / SMPS lowering
        ↓
chip execution
        ↓
VGM/VGZ projection
        ↓
blind reverse inference
        ↓
compare with hidden ASM answer key
```

Measure separately:

- exact recovered semantics;
- ambiguous semantics where several source explanations fit the same execution;
- genuinely lost source distinctions;
- reconstruction errors.

FlameDriver must not be used as a proxy for historical Sonic 2 or S&K behavior. Its compatibility paths and generalizations are themselves evidence that family identity and revision behavior must stay separate.

### `vgm2smps` reverse-lowering control

Repository:

- https://github.com/Ivan-YO/vgm2smps

This tool converts flattened VGM execution back toward Sonic 1 SMPS. Its own README is valuable because it documents the inverse problem directly: note/frequency reconstruction and note-duration reconstruction can occasionally be wrong, and its alternate instrument reconstruction is heuristic.

Use it as a scientific control for the distinction:

```text
candidate reconstructed upstream sequence
!= source-native authored semantics
```

Future test:

1. take a known source ASM/SMPS song;
2. execute to VGM;
3. hide source;
4. reconstruct with Retro VGM Compiler and `vgm2smps` independently;
5. compare both with the hidden source;
6. classify every semantic field as recovered, ambiguous, lost, or wrong.

### Cube / Iwadare-family material

Repositories:

- https://github.com/CubeTaguchiCentral/CubeWiz
- https://github.com/CubeTaguchiCentral/CubeDocs
- https://github.com/CubeTaguchiCentral/CubeAssets
- https://github.com/CubeTaguchiCentral/CubeTools

`CubeAssets` is especially valuable because it preserves command-level ASM song material. It exposes source-native operations such as instrument selection, volume, release control, vibrato, notes/waits, loops, repeated sections, alternate endings, and main-loop structure before these become chip writes.

Cube also established two generic invariants already promoted into the semantic model:

```text
same opcode byte can mean different things in FM / DAC / PSG scopes
command meaning can depend on driver timing state such as YM Timer B
```

Therefore semantic capability is scope-aware and timing-domain-aware.

### Multi-driver and signature observatories

Repositories:

- https://github.com/Awuwunya/MDmusicPlayer
- https://github.com/Awuwunya/GEMS2ASM
- https://github.com/jvisser/md-driver-signatures

`MDmusicPlayer` is useful as a comparative-driver laboratory because it hosts multiple driver families behind one playback environment, including GEMS and SMPS material. `md-driver-signatures` provides a route toward automatic code-family identification from ROM evidence, currently including GEMS and Krisalis signatures.

Firewall:

```text
driver signature match -> code-family evidence
driver signature match !-> composer identity
driver signature match !-> authored musical intention
```

### XGM / XGM2 controlled transformed-runtime bench

Repository:

- https://github.com/Ganso/md-soundtest

This project gives a controlled XGM versus XGM2 bench from shared source material and exposes practical CPU/PCM/resource behavior. It is useful for testing what a later runtime changes while holding source content relatively stable.

Because XGM-family material can be derived from VGM, its new runtime scheduling/container semantics must remain downstream of the original capture. A transformed runtime cannot manufacture source-native ancestry that its input no longer carried.

### KokonoePlayer-Lite transformed-runtime specimen

Repository:

- https://github.com/karmic64/KokonoePlayer-Lite

This is another VGM-derived runtime with its own song-slot priority, pause/resume/stop behavior, sample playback, and configurable processing interval. It is a useful negative control:

```text
new runtime semantics are real
but they are not evidence of original composer/driver semantics
```

### Echo, MiniMusic, MDSDRV and modern open-source controls

Repositories:

- https://github.com/sikthehedgehog/Echo
- https://github.com/sikthehedgehog/minimusic
- https://github.com/superctr/MDSDRV
- https://github.com/superctr/ctrmml
- https://github.com/Stephane-D/SGDK
- https://github.com/vladikcomper/MegaPCM

These provide independent implementations of several recurring abstract semantics:

- note attack/release;
- pitch change without retrigger;
- ties/slurs and gate control;
- detune/transpose/portamento/modulation;
- logical-track versus physical-channel assignment;
- SFX priority/channel stealing;
- PCM versus FM6 arbitration;
- raw-register escape;
- loops/subroutines;
- transformed-runtime buffering/scheduling.

A generic capability is trusted more when it recurs independently across different driver languages, but each native token and execution path must remain provenance-separated.

## Composer / programmer / toolchain testimony

These documentary sources tell the compiler what distinctions may have existed upstream. They are external annotations until linked to a particular source artifact or execution.

### Hitoshi Sakimoto / Terpsichorean

- https://www.timeextension.com/features/interview-it-felt-very-computer-y-to-give-english-names-to-things-hitoshi-sakimoto-on-creating-his-famous-terpsichorean-sound-driver

Useful consequence: composition, sequencing/sound data, driver programming, and hardware control did not necessarily belong to cleanly separated roles. Sakimoto's workflow is a concrete example of a composer/programmer building the representation through which the music was authored.

### Yuzo Koshiro / MUCOM88 and MML

Primary/repository routes already harvested in the driver census include Koshiro interviews plus the preserved MUCOM88 ecosystem and MML documentation. The durable lesson is that composer-facing MML can explicitly encode articulation, timing, pitch control, instrument/control state, macros, and direct hardware escapes before lowering.

Important boundary:

```text
MML dialect + revision is part of the source identity
```

Do not retroactively claim every command in a modern/preserved MUCOM88 reference existed in every historical version Koshiro used.

### Hiroshi Kubota and Sega SMPS tooling

First-party Sega preservation route:

- https://hiddenpalace.org/News/Sega_of_Japan_Sound_Documents_and_Source_Code

Useful consequence: Sega's PC-98 authoring/assembly/ROM-RAM-board workflow and differing driver branches show that the development toolchain is part of the execution ancestry. A driver revision can remove or alter expressive commands, so `unsupported` and `unknown` must remain separate capability states.

### Yoshiaki Kashima / Sonic-family Z80 lineage

Kashima-related lineage evidence is useful for revision history and later Sonic-family branches. Personal attribution must remain conservative unless source headers, first-party documentation, or comparably strong evidence supports a specific claim. The compiler needs the branch behavior, not a romanticized ownership story.

### Other harvested composer/programmer controls

The existing driver dialect census preserves research leads and documentary evidence for:

- Matt Furniss + Shaun Hollingworth custom tracker/driver workflow;
- Noriyuki Iwadare and Cube-family development constraints;
- Hiroshi Kawaguchi rewriting Sega drivers in assembler;
- Chris Huelsbeck and bespoke sampled-audio support;
- Tim Follin + Dean Belfield one-off driver work.

These exist to prevent one Sega/Sonic workflow from becoming a universal model of Genesis composition.

## Academic / SciSpace quarry retained

Methodological controls already preserved in the driver census and ancestry documents include:

- Kevin R. Burke, *Hard Limitations and Soft Possibilities*;
- James Newman, *Driving the SID chip: assembly language, composition and sound design for the C64*;
- Karen Collins, *In the Loop: Creativity and Constraint in 8-bit Video Game Audio*;
- Kenneth B. McAlpine, *Bits and Pieces: A History of Chiptunes*.

The literature supports treating software affordances, coding practice, and historical tool constraints as part of the compositional medium. It does not identify a fixture-specific driver command and cannot override source-native runtime evidence.

## User-supplied archival packs

Two local 7z archives were supplied in the conversation and have been preserved by exact hash in this ledger. Their internal contents have **not** been inspected in the current runtime because no 7z reader is installed here. No claim depends on unseen archive contents.

### `SMPS_Research_Pack_v5.7z`

- observed local size: approximately 13 MiB
- SHA-256: `9e680ab8ed0e784afdb1cb2c534ac8abf3a13009a747552966484901f1518fbc`
- state: exact archive identity observed; contents not yet enumerated

### `SMPS_Research_Pack_v5_DriverDisasm.7z`

- observed local size: approximately 23 MiB
- SHA-256: `872319651092281e3b3ef6f9d15ed6ae5153f3885d6e50d6f1cb805e821c70ad`
- state: exact archive identity observed; contents not yet enumerated

When an environment with 7z support is available:

1. enumerate paths without modifying the archives;
2. hash extracted source objects individually;
3. classify raw/original versus cleaned/reconstructed material;
4. compare overlapping driver disassemblies with public Sonic Retro / FlameDriver sources;
5. preserve disagreements rather than choosing one silently;
6. use exact source-song objects as hidden answer keys for forward/inverse tests where licensing/provenance permit.

## Generic semantic invariants earned so far

The source quarry has already justified code-level distinctions in:

- `model/execution_semantic_provenance.h`;
- `model/execution_semantic_dialect.h`;
- `model/execution_semantic_scope.h`.

The durable rules are:

```text
same chip state != same upstream intention
same opcode != same meaning across driver/scope
same driver family != same revision behavior
same physical channel != same logical musical part
not observed != unsupported
unsupported != unknown
transformed runtime != authoring source
reconstructed source candidate != exact authored source
```

The model also preserves explicit information gaps rather than inventing upstream semantics.

## Calibration program enabled by this ledger

### A. Source ASM -> driver -> VGM -> blind inverse

Highest-value route because it measures semantic loss directly.

Preferred controls include real FlameDriver/SMPS ASM songs and Cube ASM assets where the forward execution path can be pinned.

Grade fields separately:

```text
exactly recovered
strongly reconstructed but non-unique
ambiguous
lost in projection
incorrectly reconstructed
```

### B. Historical driver -> independent reconstruction differential

Examples:

```text
Sonic disassembly <-> FlameDriver compatibility path
S&K disassembly <-> readable reconstructed/runtime implementation
raw/cleaned GEMS assembly <-> player/reconstruction
```

Compare command/state traces and chip writes, not only final PCM.

### C. Same-hardware, different-driver decoys

Use SMPS, GEMS, Cube, Echo/MDSDRV-like modern controls, and transformed runtimes to identify which inferred features are truly driver-semantic rather than generic YM2612/PSG behavior.

### D. Reverse-lowering competition

Compare Retro VGM Compiler against tools such as `vgm2smps` on hidden-source fixtures. A tool that guesses a plausible sequence should not receive exact-evidence status merely because the rendered result is close.

## Current implementation checkpoint

As of the source-quarry checkpoint:

- semantic ancestry can preserve distinct upstream causes above identical synthesis state;
- dialect/revision/artifact-role identity is first-class;
- capability state distinguishes `supported`, `unsupported`, and `unknown`;
- semantic scope is first-class;
- timing domain is first-class;
- the semantic regressions are registered in CTest;
- the repository-owned local fallback runner remains available while hosted GitHub Actions are unavailable.

The research layer is now sufficiently broad that further source hunting should be discriminator-driven. The next engineering priority is the standalone local Genesis real-trace/corpus path using the existing source-aware libvgm + Nuked OPN2 + PSG source plane, not another playback implementation.

## Re-entry handle

Resume implementation from current `main` by inspecting:

```text
components/vgm/foo_input_vgm/src/source_aware_vgm_player.h
patches/libvgm-source/ and/or the current libvgm materialization path
existing Genesis realtime Omniphony/governor pipeline
SPC real trace runner as the behavioral template
58-file Sonic 3/S&K frozen VGZ corpus + SHA manifests
```

Target:

```text
VGZ bytes
→ pinned/patched libvgm PlayerA + SourceAwareVGMPlayer
→ exact FM1-6 + DAC + PSG1-4 source planes
→ existing Genesis realtime musical/Omniphony pipeline
→ passive ABI 0.4 renderer
→ block + continuity validators
→ creator/game/title-blind JSON sidecar
→ local SHA-driven corpus orchestrator
```

No pair-aware presentation control and no `separation_pressure` promotion until the preregistered real corpus evidence actually executes and passes.