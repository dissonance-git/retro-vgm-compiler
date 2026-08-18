# SMPS research-pack harvest

Status: inspected transport-pack harvest. The original `.7z` containers were temporary transfer media and were deleted after inspection. No archive is part of the project contract.

Purpose: preserve only the information that materially improves Genesis driver/source understanding, while identifying what was redundant with public source observatories already used by Retro VGM Compiler.

## What was inspected

Two supplied transport packs were unpacked with the runtime's native `libarchive` support.

The first pack contained a large SMPS rip/tool corpus:

- 11,412 extracted files;
- about 25.6 MB uncompressed payload;
- 4,178 `.bin` objects;
- 1,345 `.smy` sequence objects;
- 827 `.txt` notes/configuration files;
- hundreds of `.s3k`, `.sm2`, `.sm1`, `.trs`, `.sf3`, `.psm`, `.psz`, `.ini`, and related driver-specific sequence/data files;
- source code for SMPS extraction, driver extraction, Z80 cycle counting, PCM/DPCM conversion, Sega CD extraction, conversion and optimization utilities.

The second pack contained a driver-disassembly / timing corpus:

- 542 extracted files;
- about 208 MB uncompressed payload, dominated by IDA databases;
- 145 `.asm` disassemblies;
- 201 driver `.bin` objects;
- 143 `.idb` analysis databases;
- DAC timing/cycle tables;
- SMPS classification and command/terminology notes;
- historical and prototype driver specimens across pre-SMPS, 68k SMPS, Z80 SMPS, DAC subdrivers, 32X variants and special branches.

The raw archives themselves are not retained.

## Redundancy result

A large fraction of the first pack is transport redundancy rather than unique project evidence.

Its `Rips/` layout closely matches the public Sonic Retro `smps-rips` corpus already in the source ledger. As a byte-level spot check, the supplied Sonic 3 `03 Hydrocity 1.B0BC.s3k` and `04 Hydrocity 2.C0C6.s3k` objects have Git blob identities exactly matching the corresponding files in `sonicretro/smps-rips`.

Therefore:

```text
bulk copied SMPS song/rip payload
→ redundant as a durable local archive
→ use public/pinned source when possible
```

The pack still helped because it exposed old research tooling, classification notes and a broad side-by-side driver corpus in one place.

The `.idb` files are also not useful as canonical project evidence. They are large editor-analysis artifacts. The paired driver binary plus readable ASM/disassembly is the more portable evidence surface.

## Useful delta 1: SMPS dialect differences are more granular than family names

The command/terminology material records several distinctions that should become testable driver-dialect features rather than prose-only knowledge.

### Pointer interpretation

Observed family rules include:

```text
68k song-header pointers       relative to song header
68k coordination pointers      relative to pointer + 1
Ristar-like coordination ptrs  relative to pointer + 2
Z80 pointers                   absolute
```

Consequence: a raw address operand is not semantically interpretable without driver dialect/revision.

### Same command byte, different driver meaning

Examples in the supplied terminology include `$EA` meaning different things in different families:

```text
Z80 Type 1  -> update-rate control
Z80 Type 2  -> DAC playback
68k         -> tempo control
```

This strengthens the existing law:

```text
opcode byte + payload != semantic command
```

At minimum, interpretation requires dialect/revision and semantic scope.

### Tick multiplier / note-time interaction

The supplied notes distinguish:

```text
68k tick multiplier -> note length
Z80 tick multiplier -> note length + note-timeout behavior
```

This matters for articulation inference. A note timeout cannot be translated into a source-independent duration without the driver's timing rule.

### Note/rest continuation behavior

A documented divergence is:

```text
68k Note Rest Delay Delay -> Note Rest Rest
Z80 Note Rest Delay Delay -> Note Rest Note
```

with drum-channel exceptions in Z80 behavior.

This is an excellent falsifier for any decoder that treats delay bytes as source-independent musical rests.

### Modulation and envelope arithmetic

The pack explicitly separates 68k and Z80 modulation algorithms and records different envelope-multiplier arithmetic:

```text
pre-SMPS -> unsigned multiplier; zero has special non-multiply behavior
68k      -> signed multiplier; multiplication by zero is legal
Z80      -> unsigned multiplier applied as multiplier + 1
```

It also records revision-specific envelope control-byte behavior, including Sonic 1/2 and several non-Sonic exceptions.

Consequence: an envelope byte stream needs a dialect-owned interpreter. Reusing a generic envelope machine risks producing plausible but historically wrong trajectories.

### FM instrument/operator order and pitch tables

The notes preserve variant FM operator ordering plus distinct 68k/Z80 FM and PSG note-frequency tables.

These distinctions are useful for source-native sequence parsing and for detecting a wrong driver hypothesis before it contaminates higher musical inference.

### FM volume semantics

The pack records a family distinction between:

- 68k volume application to algorithm-selected output operators;
- Z80 volume application based on total-level values carrying the relevant mask bit.

This is another case where equivalent audible attenuation can arise through different driver semantics.

## Useful delta 2: the old extractor is a driver-detection quarry

The supplied `SMPSExtract` source is more valuable than the bundled executable.

Its source contains explicit routines for:

- distinguishing Z80 versus 68k SMPS;
- separating Z80 Type 1 and Type 2 families;
- separating 68k subtypes;
- detecting special Sonic-family / clone variants;
- locating general pointer lists;
- recovering music/SFX/modulation/volume-envelope pointer tables;
- handling banked Z80 driver fragments;
- scanning ROM-side 68k copy/decompression routines that materialize Z80 sound RAM;
- recognizing several game-specific loading patterns.

This suggests a project-native detector architecture:

```text
ROM / executable bytes
→ structural loader evidence
→ candidate driver image(s)
→ code-family detector
→ bounded dialect/revision hypothesis
→ pointer/table recovery
→ semantic decoder selected by proven capability set
```

The detector must emit evidence and confidence, not a filename-derived game label.

Potential implementation route:

- port only generic structural ideas, not copied code;
- express signatures as repository-owned evidence contracts;
- validate against independent public disassemblies;
- require a negative-control family that shares hardware but not SMPS structure;
- keep `unknown` distinct from `unsupported`.

## Useful delta 3: broad historical driver differential corpus

The driver-disassembly pack contains enough cross-family material to become a very strong *quarry* for designing public/reproducible tests.

Examples represented include:

- pre-SMPS Z80;
- pre-SMPS 68k;
- SMPS Z80 FM Type 1;
- SMPS Z80 FM Type 2;
- SMPS Z80 DAC Type 1;
- SMPS Z80 DAC Type 2;
- special Sonic 2 beta/final branches;
- Sonic 3 / Sonic & Knuckles Type 2 branches and prototypes;
- 68k SMPS;
- 68k DAC subdrivers;
- Treasure-specific DAC drivers;
- 32X variants;
- Pico/Sega-CD-related SMPS material.

Especially useful Sonic specimens include readable disassemblies for:

- Sonic 2 Simon Wai beta;
- Sonic 2 Beta 4 / Beta 5;
- Sonic 2 Final;
- Sonic 3 retail;
- Sonic & Knuckles prototype 1994-05-25;
- Sonic and Crackers beta;
- plus binaries for additional intermediate Sonic 2 and Sonic 3/S&K prototypes.

This enables revision-differential tests such as:

```text
same semantic feature across revisions
→ same abstract capability?
→ same native token?
→ same state side effects?
→ same timing arithmetic?
→ same emitted YM2612/PSG trajectory?
```

Do not commit the third-party disassembly corpus wholesale. Prefer public/pinnable original or reconstructed sources for executable tests, using this harvest to decide which differences deserve independent reproduction.

## Useful delta 4: DAC timing tables are high-value execution evidence

The supplied DAC notes contain instruction-cycle measurements for many Z80/68k DAC drivers, including loop offsets, samples emitted per loop, memory source, and cycle formulas.

This is materially useful because source-native PCM pitch/rate may be determined by driver execution timing rather than by an explicit sample-rate field.

The Sonic lineage illustrates the point:

```text
Sonic 2 beta/final
Sonic 3
Sonic & Knuckles
Sonic 3D Blast
Sonic and Crackers
```

have related but not identical playback loops/cycle counts.

The broader tables also include drivers that:

- interpolate samples;
- switch between PCM and DPCM loops;
- run multiple DAC channels;
- keep cycle time constant with dummy work when a channel is idle;
- use timer-controlled PCM playback;
- change memory source between RAM and ROM;
- have special uninterruptable/update interactions.

New project rule:

> When recovering PCM semantics from executable/native driver evidence, sample identity is insufficient. Playback clock provenance and driver loop timing are part of the performed object.

This should eventually feed an executable `dac_playback_clock_evidence` or equivalent representation rather than becoming another hard-coded game table.

## Useful delta 5: the ripped sequence corpus can become inverse-reconstruction controls

The raw song objects are not needed as a permanent copied archive because much is already public, but their diversity suggests a much stronger test design.

Instead of validating reverse inference only on Sonic-family SMPS:

```text
many SMPS dialects
+ known driver/config
+ raw sequence
→ forward execute
→ hide source sequence
→ observe VGM/chip execution only
→ blind reconstruct
→ compare against hidden sequence
```

The test should score fields independently:

```text
command capability
note/rest structure
articulation/retrigger relation
timing domain
transpose/detune
modulation/envelope behavior
control flow
instrument identity
DAC/sample event identity
```

This directly measures which authored/driver semantics survive device-facing projection.

## What remains redundant

Do not preserve or duplicate these merely because they were present in the transport pack:

- thousands of raw rips already available in public `sonicretro/smps-rips`;
- bundled `.exe` utilities when source exists;
- IDA `.idb` databases;
- duplicated PCM/WAV conversions where the original sample plus conversion rule is sufficient;
- game-name classifications as runtime truth;
- old detection labels that are not independently validated;
- copied third-party disassemblies where a public, pinnable source exists.

## What should be promoted into project machinery

Priority order:

1. **Driver dialect differential fixtures** for pointer rules, command meaning, timing, envelope math and note/rest continuation.
2. **ROM/driver structural detector** inspired by the extractor's generic code-shape/pointer-table logic, with evidence/confidence and negative controls.
3. **DAC playback-clock evidence** so native PCM reconstruction carries executable timing provenance.
4. **Cross-revision Sonic driver tests** using public/pinnable disassemblies or reconstructed sources.
5. **Broad blind SMPS inverse tests** across several driver families, not only Sonic.

These are more valuable than retaining the archives themselves.

## Provenance firewall

The packs are research leads, not authoritative upstream artifacts by default.

```text
pack filename/path
!= historical provenance
IDA label
!= original symbol
research classification
!= exact driver identity
reconstructed ASM
!= original source code
raw rip
!= authored source project
```

Whenever a finding becomes an executable project claim, prefer a public/pinned primary or independently reproducible source and record how the correspondence was established.

## Re-entry

Use this document together with:

- `genesis-driver-source-ledger.md`;
- `../genesis-driver-dialect-census.md`;
- `../genesis-open-driver-anatomy.md`;
- `genesis-authoring-driver-toolchain-quarry.md`;
- `genesis-driver-source-vgm-boundary.md`.

The next implementation frontier remains the standalone local Genesis real-trace/corpus path. The pack harvest improves the **upstream semantic model and future calibration suite**; it does not justify delaying the already-preregistered FM/PSG/DAC real-corpus governor experiment.