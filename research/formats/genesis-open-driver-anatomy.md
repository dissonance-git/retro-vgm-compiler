# Genesis open-driver comparative anatomy

Status: source-grounded research input for generic execution semantics. Named games, composers, studios and driver families are provenance, never renderer controls.

The Mega Drive/Genesis is an unusually strong comparative platform because many unrelated software systems converge on the same YM2612 + SN76489 + DAC hardware. The compiler should use these drivers to learn invariant semantic categories while preserving each driver's native vocabulary and timing model.

## Core rule

```text
same chip state != same upstream command
same opcode != same meaning across scopes
same driver family != same revision behavior
same audible result != same command ancestry
```

## Sonic 2 SMPS specimen

Repository: https://github.com/MainMemory/s2-sound-driver-plus

The Sonic 2 Z80 disassembly exposes track state directly. Important state includes:

- playback/rest state;
- SFX override;
- do-not-attack-next-note;
- modulation enable/state;
- per-track tempo divider;
- transpose;
- detune;
- note-fill master + timeout;
- saved duration;
- FM/PSG channel assignment;
- voice/instrument identity;
- PSG envelope/flutter state;
- loop counters and gosub return stack;
- DAC/FM6 ownership;
- global tempo and speed-shoe tempo state.

The semantic consequence is that note articulation and timing are not recoverable from YM2612 writes alone. Note fill, retrigger suppression, modulation and duration all depend on driver state that can collapse to similar register trajectories.

The original driver should remain a separate specimen from later Sonic-family engines. Port/conversion layers explicitly have to compensate for differences between Sonic driver revisions, especially PSG pitch/envelope conventions, DAC sample sets and coordination-flag semantics.

## FlameDriver specimen

Repositories:

- https://github.com/TheBlad768/s2disasm-flamedriver
- https://github.com/TheBlad768/Sonic-Clean-Engine-S.C.E.-/tree/flamedriver
- upstream lineage referenced by source: https://github.com/flamewing/flamedriver

The embedded driver identifies itself as `SonicDriverVer = 5` and exposes a richer playback-control model than classic Sonic 2:

- PSG noise / FM3 special mode;
- do-not-attack-next-note;
- SFX override;
- alternate literal-frequency mode;
- track-at-rest;
- pitch-slide mode;
- sustain-frequency mode;
- track-playing state.

Track RAM also carries:

- tempo divider;
- transpose;
- modulation control and modulation-envelope index/sensitivity;
- FM/PSG voice identity;
- AMS/FMS/pan;
- saved duration;
- frequency;
- voice-song provenance;
- detune;
- volume envelope;
- SSG-EG state;
- note-fill state;
- modulation pointer/wait/speed/delta/steps;
- loop counters and stack.

The note parser provides direct semantic behavior:

- `bitNoAttack` is cleared before scanning the next track event, but coordination flags can set it before the note is finalized;
- normal note handling performs key-off before reading a new pitch unless suppression/override state prevents it;
- pitch slide can consume a per-note detune/slide value;
- alternate-frequency mode consumes literal frequency values rather than note-table indexes;
- per-track `TempoDivider` multiplies duration before countdown;
- when a real new attack is admitted, modulation-envelope index/sensitivity, volume envelope and note-fill timeout are reset;
- if no-attack is set, those attack-reset side effects are skipped too, so retrigger suppression is broader than merely omitting YM register $28;
- key-on and key-off both honor override/no-attack state;
- a YM key transition clears sustain-frequency state.

The source comments retain compatibility paths attributed to non-Sonic drivers, including Battletoads-style pitch-slide behavior and Dyna Brothers 2 sustain-frequency behavior. These are useful evidence that FlameDriver is a deliberate cross-driver generalization rather than a pristine historical Sonic 2 model.

## Cube / Iwadare-family specimen

Repositories:

- https://github.com/CubeTaguchiCentral/CubeWiz
- https://github.com/CubeTaguchiCentral/CubeDocs
- https://github.com/CubeTaguchiCentral/CubeAssets
- https://github.com/CubeTaguchiCentral/CubeTools

CubeDocs documents four command sets sharing one sequence format:

- YM FM;
- YM6 DAC;
- PSG tone;
- PSG noise.

This proves that opcode semantics are channel-scoped. Example: `$FA` means stereo panning in the FM command set but sets YM Timer B in the PSG tone command set. `$FC` also changes meaning by channel family, covering release/sustain/slide behavior for FM, sample-stop behavior for DAC, and release behavior for PSG.

Cube song headers carry YM Timer B as sound-update frequency, so timing belongs to the driver semantic layer. A command cannot be interpreted correctly without both its channel scope and timing domain.

`CubeAssets` preserves ASM-level musical streams from real games. Gley Lancer assets contain commands such as:

- `inst`;
- `vol`;
- `setRelease`;
- `vibrato`;
- `note` / `noteL`;
- `wait` / `waitL`;
- `countedLoopStart` / `countedLoopEnd`;
- `repeatStart` and alternate ending sections;
- `mainLoopStart` / `mainLoopEnd`.

The Cube-to-Furnace conversion documentation explicitly records lossy or structurally different translations for vibrato, portamento, note shifting, detune, loops and simultaneous pattern boundaries. This is unusually strong evidence for semantic loss across representation changes.

## MiniMusic specimen

Repository: https://github.com/sikthehedgehog/minimusic

MiniMusic 1.19 is a small Z80-resident driver with a documented runtime bytecode. Important semantics include:

- explicit key-on and key-off;
- `$61` cancel-next-key, allowing pitch/time progression without performing the key operation;
- loops and subroutines;
- instrument selection;
- transpose;
- attenuation;
- panning + AMS/PMS;
- FM/PSG channel assignment;
- SFX takeover of selected channels;
- explicit SFX priority.

Its `$61` behavior is an independent implementation of the same abstract `pitch_without_retrigger` capability seen in Sonic-family no-attack behavior, but with a distinct native token and execution model.

## Echo / ESF specimen

Repository: https://github.com/sikthehedgehog/Echo

Echo provides another independent sequence language with note attack/release, volume, frequency, instruments, samples, delays, loops, SFX channel locking, FM parameter changes and direct register writes. Frequency changes without note attack provide another independent `pitch_without_retrigger` control.

## MDSDRV + ctrmml specimen

Repositories:

- https://github.com/superctr/MDSDRV
- https://github.com/superctr/ctrmml

This pair provides a composer-facing authoring layer plus runtime driver. The MML distinguishes tie/slur, quantized articulation, release, shuffle, grace notes, transpose, detune, portamento, modulation, FM3 special mode, PCM settings and direct register writes before lowering to sequence data.

This is important because it supplies source-native authored semantics that need not survive into VGM.

## GEMS specimens and signatures

Repositories:

- https://github.com/Awuwunya/MDmusicPlayer
- https://github.com/Awuwunya/GEMS2ASM
- https://github.com/realmonster/GEMS
- https://github.com/jvisser/md-driver-signatures

`MDmusicPlayer` contains common GEMS 68K/Z80 driver source plus game-specific material for Aladdin, Sonic Spinball and Vectorman. This is stronger evidence than a format-only decoder because it exposes a runnable multi-driver environment.

`md-driver-signatures` contains Radare2 signatures for GEMS and Krisalis. This suggests a future detector that can attach a bounded driver-family identity to ROM evidence without relying on filename/game metadata.

Important firewall:

```text
signature match -> candidate/exact code-family evidence
signature match !-> composer identity
signature match !-> fixture-specific authored intention
```

## MDmusicPlayer as comparative laboratory

Repository: https://github.com/Awuwunya/MDmusicPlayer

The player is designed to support multiple sound drivers simultaneously and includes both GEMS and SMPS material. It is useful for testing whether the same musical asset, channel policy or hardware state is represented differently by different driver ecosystems.

## XGM / XGM2 controlled bench

Repository: https://github.com/Ganso/md-soundtest

This project produces separate XGM and XGM2 ROMs from the same source material and reports conversion size, PCM-channel occupancy, Z80 load, DMA-related load and unsupported driver features. It also encourages emulator and real-hardware comparison.

This is valuable as an executable transformed-runtime control. It can measure software cost and audible/runtime differences caused by driver choice while holding source material constant.

## KokonoePlayer-Lite transformed-runtime specimen

Repository: https://github.com/karmic64/KokonoePlayer-Lite

KokonoePlayer-Lite converts VGM into a Z80-resident runtime. It exposes:

- multiple priority song slots;
- pause/resume/stop per slot;
- sample playback;
- a configurable base processing interval expressed in 44.1 kHz sample units;
- explicit CPU-quality tradeoffs when changing that interval.

Because its lineage begins from VGM, any new scheduling/priority semantics introduced by KokonoePlayer are runtime semantics, not recovered source-native authoring intent.

## New model invariants earned by the source comparison

### 1. Semantic scope is first-class

A native token must be interpreted within a bounded scope such as:

```text
global
logical track
physical channel
FM channel
DAC channel
PSG tone channel
PSG noise channel
song slot
driver queue
```

### 2. Timing domain is first-class

Meaning can depend on clocks and divisors above the chip:

```text
driver update
VBlank update
PAL-compensated update
global tempo accumulator
per-track tempo divider
YM Timer B
VGM 44.1 kHz sample clock
source/emulator sample clock
```

### 3. Retrigger suppression is not just missing register writes

Sonic-family and MiniMusic evidence show that suppressing a key transition can also preserve/reset different driver-side envelope, note-fill and articulation state. The semantic object is therefore `pitch_without_retrigger` / linked articulation, not merely `YM2612 key-on absent`.

### 4. Logical part identity can diverge from physical channel identity

SFX override, channel stealing, priority song slots, PCM/FM6 arbitration and driver queues all prove that hardware channel continuity is insufficient to define persistent musical-part continuity.

### 5. Transformed runtime semantics must stay downstream

VGM-derived drivers such as XGM-family workflows and KokonoePlayer can add scheduling, priority and buffering semantics after capture. Those semantics are real runtime evidence but cannot be back-projected into the original composer/driver layer without independent provenance.

Implementation: `model/execution_semantic_scope.h` and `tests/model/execution_semantic_scope_test.cpp` preserve scope and timing-domain annotations without making any named driver a runtime decision feature.
