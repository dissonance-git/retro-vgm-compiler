# Genesis driver source ledger

Status: durable provenance and re-entry ledger for the Genesis driver/toolchain quarry.

Scope: sources, artifacts, and research leads that can reveal semantics above YM2612/SN76489/DAC execution. Game, composer, studio, or driver identity must never become renderer-control features.

Conceptual conclusions live in:

- `../genesis-driver-dialect-census.md`
- `../genesis-open-driver-anatomy.md`
- `genesis-authoring-driver-toolchain-quarry.md`
- `genesis-driver-source-vgm-boundary.md`
- `smps-research-pack-harvest.md`

## Research question

```text
authored source / MML / ASM
        ↓
compiler / assembler / conversion tool
        ↓
driver sequence + instruments + envelopes + samples
        ↓
logical tracks / scheduling / arbitration / control flow
        ↓
YM2612 + SN76489 + DAC execution
        ↓
VGM/VGZ device-facing projection
```

Each arrow can erase distinctions. Recover upstream semantics only when evidence survives; otherwise preserve a bounded hypothesis or an explicit information gap.

## High-priority source observatories

### Sonic Retro

- https://github.com/sonicretro/s1disasm
- https://github.com/sonicretro/s2disasm
- https://github.com/sonicretro/skdisasm
- https://github.com/sonicretro/smps-rips
- https://github.com/sonicretro/SMPSPlay-DLL

Important law:

```text
S1 SMPS != S2 SMPS != S3/S&K SMPS != later FlameDriver behavior
```

These are related lineages with revision-specific semantics, timing, bugs, capabilities, and layouts.

### Sonic 2 original and enhanced specimens

- https://github.com/MainMemory/s2-sound-driver-plus
- https://github.com/sonicretro/s2disasm

Useful driver-native state includes no-attack/retrigger suppression, transpose, detune, modulation, note-fill state, per-track tempo division, PSG envelope state, control-flow stacks, SFX override, and DAC/FM6 ownership.

### FlameDriver

- https://github.com/flamewing/flamedriver
- https://github.com/TheBlad768/s2disasm-flamedriver
- https://github.com/TheBlad768/Sonic-Clean-Engine-S.C.E.-/tree/flamedriver

FlameDriver provides readable source and real song material for forward/inverse semantic tests. It must not be treated as a pristine proxy for historical Sonic 2 or S&K behavior.

### Reverse-lowering control

- https://github.com/Ivan-YO/vgm2smps

Use it as a control for:

```text
candidate reconstructed upstream sequence
!= exact source-native authored semantics
```

### Cube / Iwadare family

- https://github.com/CubeTaguchiCentral/CubeWiz
- https://github.com/CubeTaguchiCentral/CubeDocs
- https://github.com/CubeTaguchiCentral/CubeAssets
- https://github.com/CubeTaguchiCentral/CubeTools

Cube material proves that opcode meaning can depend on channel scope and driver timing state. ASM song assets also provide known upstream control-flow and articulation semantics.

### Multi-driver and signature observatories

- https://github.com/Awuwunya/MDmusicPlayer
- https://github.com/Awuwunya/GEMS2ASM
- https://github.com/realmonster/GEMS
- https://github.com/jvisser/md-driver-signatures

Firewall:

```text
driver signature match -> code-family evidence
driver signature match !-> composer identity
driver signature match !-> authored intention
```

### Transformed-runtime controls

- https://github.com/Ganso/md-soundtest
- https://github.com/karmic64/KokonoePlayer-Lite
- https://github.com/Stephane-D/SGDK

A runtime derived from VGM may add real scheduling/container semantics, but it cannot recreate source-native ancestry that was already absent from the capture.

### Independent modern driver controls

- https://github.com/sikthehedgehog/Echo
- https://github.com/sikthehedgehog/minimusic
- https://github.com/superctr/MDSDRV
- https://github.com/superctr/ctrmml
- https://github.com/vladikcomper/MegaPCM

These provide independent examples of attack/release, pitch change without retrigger, ties/slurs, detune/transpose/portamento, channel stealing, PCM/FM6 arbitration, raw-register escape, loops/subroutines, and priority behavior.

## Harvested SMPS transport-pack findings

The supplied `.7z` packs were inspected and then deleted as transport media. Their durable value is summarized in `smps-research-pack-harvest.md`.

The bulk song-rip payload is largely redundant with public Sonic Retro material. Spot checks of supplied Sonic 3 Hydrocity sequence files matched the corresponding `sonicretro/smps-rips` Git blobs exactly.

The useful nonredundant findings are:

- pointer-base rules differ by 68k, Ristar-like, and Z80 dialects;
- the same command byte can mean different operations in different SMPS families;
- tick multipliers interact with note timeouts differently on 68k versus Z80;
- note/rest/delay continuation differs between families;
- modulation and envelope arithmetic differ between pre-SMPS, 68k, and Z80;
- operator order and FM/PSG pitch tables vary by dialect;
- FM volume semantics differ between family implementations;
- the old extractor source contains useful structural driver-detection ideas;
- a broad historical driver/disassembly collection suggests strong cross-revision differential fixtures;
- DAC cycle-count tables show that source-native PCM rate can depend on executable loop timing rather than an explicit rate field.

Do **not** preserve the transport archives, bundled executables, IDA databases, or duplicate public rips merely because they were supplied.

## New calibration directions earned by the harvest

### Driver dialect differentials

Build small executable fixtures that falsify incorrect genericization of:

```text
pointer interpretation
command meaning
timing domain
note/rest continuation
tick-multiplier behavior
envelope arithmetic
instrument/operator order
pitch tables
```

### Structural driver detector

The old extractor suggests a project-native detector:

```text
ROM/executable bytes
→ structural loader evidence
→ candidate driver image
→ code-family evidence
→ bounded dialect/revision hypothesis
→ pointer/table recovery
```

Port only generic ideas. Do not copy opaque detector labels as truth. Require negative controls and confidence-bearing evidence.

### DAC playback-clock evidence

Future native-driver analysis should represent PCM playback timing explicitly:

```text
sample identity
+ playback loop / timer provenance
+ cycle timing
+ memory source
+ interpolation / DPCM behavior
→ performed PCM object
```

### Broad blind inverse tests

Use several SMPS families, not only Sonic:

```text
known driver + known raw sequence
→ forward execution
→ hide source
→ observe device-facing trajectory
→ blind reconstruct
→ compare with hidden sequence
```

Score command capability, note/rest structure, articulation, timing, transpose/detune, modulation/envelope behavior, control flow, instrument identity, and DAC/sample events separately.

## Generic semantic invariants

Implemented model distinctions now include semantic ancestry, dialect/revision, artifact role, capability state, semantic scope, and timing domain.

Durable rules:

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

## Calibration program

### A. Source ASM -> driver -> VGM -> blind inverse

Preferred controls include source-native FlameDriver/SMPS ASM songs and Cube ASM assets. Grade each semantic field as exact, non-unique reconstruction, ambiguous, lost, or wrong.

### B. Historical driver -> independent reconstruction differential

Compare command/state traces and chip writes between historical disassemblies and independent readable implementations, not only final PCM.

### C. Same-hardware, different-driver decoys

Use SMPS, GEMS, Cube, Echo/MDSDRV-like controls, and transformed runtimes to separate genuine driver semantics from generic YM2612/PSG behavior.

### D. Reverse-lowering competition

Compare Retro VGM Compiler with independent reverse-lowering tools on hidden-source fixtures. Plausible rendering similarity does not earn exact source-evidence status.

## Current implementation checkpoint

The research layer is now broad enough that further source hunting should be discriminator-driven.

The immediate engineering priority remains the standalone local Genesis real-trace/corpus path using the existing source-aware libvgm + Nuked OPN2 + PSG source plane.

Re-entry:

```text
components/vgm/foo_input_vgm/src/source_aware_vgm_player.h
patches/libvgm/
components/vgm/enhancement/genesis_realtime_musical_omniphony_pipeline.h
SPC real trace runner as behavioral template
58-file Sonic 3/S&K frozen VGZ corpus + SHA manifests
```

Target:

```text
VGZ bytes
→ pinned/patched libvgm PlayerA + source-aware capture
→ exact FM1-6 + DAC + PSG1-4 source planes
→ Genesis realtime musical/Omniphony pipeline
→ passive ABI 0.4 renderer
→ block + continuity validators
→ creator/game/title-blind JSON sidecar
→ local SHA-driven corpus orchestrator
```

No pair-aware presentation control and no `separation_pressure` promotion until preregistered real-corpus evidence actually executes and passes.