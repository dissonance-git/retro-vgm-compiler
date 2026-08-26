# VGM / Genesis upstream reference map

Status: durable research re-entry map for VGM transport, Genesis driver semantics, YM2612/FM device behavior, source extraction, and later source-quality experiments. It consolidates both the August 2026 quarry and useful references recovered from project commit history.

## Research question

```text
authored source / sequence / driver
        ↓
driver execution
        ↓
YM2612 + SN76489 + DAC state
        ↓
VGM/VGZ device-facing log
        ↓
source-aware rendering / inverse analysis
        ↓
Omniphony or other presentation
```

Different projects observe different arrows. A chip emulator can be excellent device evidence while knowing nothing about the original driver grammar. A driver replayer can preserve sequence meaning while using a replaceable sound core.

## Format authority: VGMRips VGM specification

Use the VGM specification as primary authority for:

- headers and version gates;
- command bytes and operands;
- chip clocks and dual-chip flags;
- data blocks and DAC Stream Control;
- wait/sample accounting;
- loop and GD3 structure.

```text
VGM bytes
→ format semantics
→ device state
→ performed behavior
→ musical inference
```

Do not let libvgm, MAME, or another emulator redefine a format byte that the specification already defines.

VGMRips technical documentation, preservation discussions, and driver material remain valuable secondary routes for obscure driver/history questions.

## Primary playback/device integration: `ValleyBell/libvgm`

Use libvgm for:

- VGM/VGZ parsing and replay architecture;
- device/core integration;
- timing and resampling boundaries;
- multi-chip playback behavior;
- controlled command/source observers;
- forward comparisons against the foobar VGM path.

VGM Compiler already carries a substantial libvgm integration history, including guarded command observers, source taps, resampler parity, YM2151 observation, and pinned-source integration tests. Search the 2026-08-17 through 2026-08-24 libvgm commits before adding a new observer.

Important boundary:

```text
libvgm playback truth
!= VGM format specification
!= original driver/source truth
```

The old libvgm YM2612 core is also historically useful because it documents lineage from MAME and exposes per-channel muting, but it should not be the sole modern YM2612 accuracy oracle.

## Die/low-level accuracy control: `nukeykt/Nuked-OPN2`

Use Nuked OPN2 for:

- very high accuracy YM3438/OPN2 behavior;
- die-shot/reverse-engineering-based low-level timing;
- YM2612-compatible mode;
- pin/output behavior and internal-clock stepping;
- an accuracy control against higher-level FM cores.

This is especially valuable when a register-level or timing behavior is disputed.

Do not infer original SMPS/GEMS command semantics from a perfect OPN2 emulation.

## Cross-family Yamaha observatory: `aaronsgiles/ymfm`

Use ymfm for:

- a readable common framework spanning OPN/OPM/OPL/OPLL families;
- YM2612, YM3438, and YMF276 comparisons;
- channel/operator/register relationships;
- controlled cross-chip negative tests.

The project already used ymfm to establish that shared Yamaha FM abstractions must be earned rather than assumed.

It is also the strongest existing quarry for a future Enhanced OPN2 ladder:

```text
YM2612
→ YM3438 / OPN2C
→ YMF276 / OPN2L
→ separately tested studio-precision experiment
```

Earlier Enhanced research specifically identified YMF276 as a promising same-family higher-ceiling baseline because it preserves the six-channel OPN2 surface while changing output/mixing precision characteristics.

## Whole-machine/system control: MAME

Use MAME when isolated sound-core testing is insufficient.

Useful roles:

- Mega Drive bus/system context around YM2612 access;
- hardware configuration differences such as discrete YM2612 versus integrated YM3438;
- independent device behavior;
- cross-machine Yamaha-family comparisons.

The current MAME OPN devices are built around ymfm, so MAME and ymfm are not fully independent arithmetic implementations. MAME's extra value is system integration and machine context.

For Mega Drive specifically, the inspected machine configuration distinguishes:

```text
YM2612 at master_clock / 7
YM3438 replacement for integrated later hardware
```

Treat that as machine/configuration evidence, not composer intent.

## Primary hardware/development documentation: Sega Retro as index

Earlier project research used Sega Retro as a preservation route to official material, including:

- YM2612 manuals / Mega Drive sound-tool documentation;
- SN76489 documentation;
- Mega Drive development-system material;
- 68000/Z80 programming material;
- schematics and board revisions.

Prefer the underlying official scan/manual over a wiki summary.

The repository already extracted primary YM2612 claims from the Mega Drive sound-tool manual, including:

- six FM channels;
- four operators per channel;
- eight algorithms;
- 9-bit DAC resolution;
- two register banks partitioning channels 1-3 and 4-6;
- documented YM2203 software-compatibility relationship.

Search commit `ccb470d...` and the hardware-reference data before translating those facts again.

## Driver/source oracle: `ValleyBell/SMPSPlay`

SMPSPlay is one of the highest-value forward controls in the whole Genesis program.

Its documented surface includes:

- configurable SMPS commands/drums;
- driver-specific FM/PSG frequency rules;
- modulation and volume envelopes;
- FM/PSG/DAC drum behavior;
- instrument tables;
- DAC rates based on Z80 cycle calculations;
- compressed/uncompressed DAC banks;
- many SMPS dialect/variant quirks;
- VGM logging and automatic loop logging.

This enables a known-answer test:

```text
known SMPS source + config
→ SMPSPlay driver execution
→ VGM log
→ hide source
→ VGM Compiler inverse analysis
→ compare with source answer key
```

Use source-command identity, track state, modulation/envelope behavior, and driver timing as hidden truth. Do not reward the inverse side for inventing source facts that the VGM projection erased.

## Matched alternative driver: `ValleyBell/GEMSPlay`

GEMSPlay gives a critical same-hardware negative control.

It exposes the GEMS data split:

```text
instrument
envelope
sequence
sample
```

and includes surviving/cleaned GEMS Z80 assembly for multiple versions.

Use it to separate:

```text
Genesis hardware behavior
from
SMPS-specific driver behavior
```

SMPS and GEMS targeting the same YM2612/PSG/DAC hardware is precisely why this comparison is powerful.

## Sonic Retro corpus and disassemblies

Earlier research had already identified these as high-priority source observatories:

- `sonicretro/smps-rips`
- `sonicretro/s1disasm`
- `sonicretro/s2disasm`
- `sonicretro/skdisasm`
- `sonicretro/SMPSPlay-DLL`

The `smps-rips` corpus carries much more than rendered music. It includes source-level side information such as command definitions, driver definitions, drum mappings, instrument sets, modulation envelopes, PSG envelopes, pointers, and some driver binaries.

Use the disassemblies to tie a ripped sequence back to actual driver code and revision-specific behavior.

Hard rule recovered from the earlier source ledger:

```text
S1 SMPS != S2 SMPS != S3/S&K SMPS != later FlameDriver
```

Same lineage does not imply identical opcode semantics, timing, bugs, or capabilities.

## Additional Genesis driver/toolchain controls already found

Do not lose these when a task widens beyond Sonic:

### Historical / readable SMPS variants

- `MainMemory/s2-sound-driver-plus`
- `flamewing/flamedriver`
- `TheBlad768/s2disasm-flamedriver`

Use for forward/inverse tests with readable driver semantics. FlameDriver is not a pristine proxy for historical Sonic revisions.

### Reverse-lowering

- `Ivan-YO/vgm2smps`

Use as competition/prior art for reconstructing a plausible upstream sequence from device logs.

```text
reconstructed SMPS-like source
!= exact authored source
```

### GEMS / multi-driver / signatures

- `Awuwunya/MDmusicPlayer`
- `Awuwunya/GEMS2ASM`
- `realmonster/GEMS`
- `jvisser/md-driver-signatures`

Driver signatures can support code-family hypotheses. They do not establish composer identity.

### Independent modern drivers

- `sikthehedgehog/Echo`
- `sikthehedgehog/minimusic`
- `superctr/MDSDRV`
- `superctr/ctrmml`
- `vladikcomper/MegaPCM`
- `Stephane-D/SGDK`

Use these to pressure-test ties/slurs, retrigger rules, detune/transpose/portamento, loops/subroutines, channel stealing, raw-register escape, PCM/FM6 arbitration, and priority behavior without overfitting to SMPS.

## Chip-execution to musical-projection controls

Earlier research also identified:

- historical/current Paul Jensen / ValleyBell `vgm2mid` work;
- `jkarenko/vgm2midi`.

Use them for prior art in deriving note-like events from chip-frequency/register trajectories.

Do not make MIDI the project ontology. FM operator state, PSG envelopes, continuous control, DAC identity, driver state, and many effect relations do not fit cleanly into a note-list projection.

## Instrument and OPN-family tooling

### `Wohlstand/libOPNMIDI`

Use it for:

- OPN2/OPNA synthesis;
- large configurable FM patch banks;
- comparing multiple YM2612 emulator cores under one higher-level workload;
- channel-pressure and instrument-allocation experiments;
- MIDI-to-VGM and DAC test utility leads.

Its support for several OPN emulators is useful for differential testing precisely because the high-level musical workload can remain fixed while the core changes.

Do not use MIDI-bank semantics as evidence for the original game's driver/instrument object.

### `Wohlstand/OPN2BankEditor`

Use it for:

- OPN-family patch extraction;
- conversion/comparison;
- instrument-bank inspection;
- corpus-side FM patch archaeology.

It can help identify repeated or related parameter sets. Patch similarity alone is not composer identity or exact source provenance.

## `vgmtrans/vgmtrans`

Use it for driver-aware recovery of sequence, instrument, and sample collections across many game formats.

It is most useful as a representation/recovery observatory and a source of negative tests:

```text
what a driver-aware extractor can recover
versus
what a pure device-log analyzer can recover
```

## Hoot / Hoot Archive

Use Hoot as a broad original-driver execution and driver-corpus observatory for Japanese computer/arcade ecosystems.

Earlier research explicitly warned not to treat historical Hoot VGM logging as timing authority. Its value is the layer above the chip: identifying the driver, its data, and how the original music program executes.

## Broader integration controls already cataloged

These are useful when the question becomes "how much common frontend can coexist with backend-specific semantics?":

- `libgme/game-music-emu`
- OpenMPT / libopenmpt
- `tildearrow/furnace`
- `yoyofr/modizer`

The useful lesson from the earlier quarry is:

```text
shared playback frontend
!= shared semantic depth
```

Furnace is also useful for chip-state and multi-core experiments, while Modizer is valuable as evidence that a huge frontend can still preserve backend-specific options/state.

## QSound as a spatial/source-domain analog

`ValleyBell/qsound-hle` is not Genesis, but it remains a useful architectural comparison because QSound has source-domain spatial processing before final stereo.

Use it as a cross-chip pressure test for the distinction:

```text
source identity + native route/effect evidence
!= final stereo mix
```

Do not import QSound's spatial mechanism into OPN2 merely because both can feed Omniphony.

## FM surround / widening implementation controls

The first Genesis Surround implementation should follow software-FM mixer
practice rather than assign musical roles to physical FM channels.

### `Wohlstand/libOPNMIDI` + Nuked OPN2 full panning

libOPNMIDI exposes a full-panning mode for OPN2 emulator cores. Its OPN2 mixer
keeps both hardware output gates enabled and calls the emulator-side `writePan`
extension instead. Nuked OPN2 implements that extension with a pan-law table
whose center values are approximately 0.707 of full scale on each side.

Useful law:

```text
synthesize FM channel once
→ apply continuous constant-power gains at mixer/output boundary
→ do not reinterpret operator algorithm or instrument role
```

This is directly useful to Surround because VGM Compiler already owns exact
per-channel source PCM. The extension can happen after source capture rather
than by altering YM2612 synthesis.

### `Wohlstand/libADLMIDI` / `nukeykt/Nuked-OPL3`

The OPL3 software extensions use the same kind of continuous pan law, while the
native OPL3 architecture can retain four A/B/C/D output buses. This is valuable
as two separate controls:

- continuous software panning is a mixer operation, not an FM timbre operation;
- genuine multibus hardware should stay multibus rather than being flattened
  before presentation.

### Furnace OPL3 surround panning

Furnace calls Nuked OPL3's four-channel generator and exposes the extra OPL3
output bits through its surround-panning command. It is useful precedent for
keeping per-channel FM output independent until a later bus-placement stage.

Do not copy OPL3's A/B/C/D hardware meaning onto OPN2. Copy the architecture:
preserve source first, route later.

### MiniDexed / DX-style FM mixing

MiniDexed mixes independent FM engines with sine/cosine panorama gains. This is
another clean example of FM synthesis remaining untouched while position is
applied as a constant-power mixer transform.

### Vital stereo-spread control

Vital is not an OPN2 emulator, but its oscillator path is a useful modern synth
control: stereo width is implemented with equal-power mixing at the output
stage rather than by changing oscillator identity.

### VGMPlay YM2612 `PseudoStereo` as a negative control

VGMPlay/libvgm carries an older YM2612 pseudo-stereo mode that alternates
left/right chip updates; its configuration warns that this can halve effective
chip update rate in one emulation mode.

Do not use this technique for VGM Compiler Surround. Width should come from
already-separated exact source PCM, not timing distortion, alternating samples,
phase inversion, or hidden rate changes.

### Product rule derived from these implementations

For arbitrary Genesis tracks:

```text
physical channel number != bass/lead/drum role

exact source lane
→ keep front anchor
→ constant-power horizontal spread
→ preserve original L/R hemisphere
→ Omniphony output
```

The current 7.1 implementation therefore gives every exact YM2612 FM/DAC and
SN76489 tone/noise lane the same power law. A low-discrepancy per-lane seed only
prevents all lanes from stacking at the same side/back depth. It carries no
musical semantics, and no source is allowed to lose its front anchor merely
because of which hardware channel happened to play it.
## Immediate source-separation consequence for Genesis VGM

For the current non-Enhanced path, prefer source products that are directly justified by execution:

```text
YM2612 FM1..FM6
+ YM2612 DAC when active/independent
+ PSG tone/noise sources
+ authored YM2612 pan bits / PSG route evidence
→ Omniphony
```

Do not make VGM Compiler decide a 3-D scene. The compiler should expose separable source contributions and source-native route evidence; Omniphony owns presentation.

Where a source is not cleanly separable, keep the protected reference mix rather than inventing a stem.

## Later Enhanced / remaster quarry

Earlier project research already established a useful FM hierarchy:

```text
reference YM2612
→ YM3438 OPN2C comparison
→ YMF276 OPN2L hardware-descendant baseline
→ studio-precision OPN2 experiment
```

Preserve:

- six channels;
- four operators per channel;
- eight OPN algorithms;
- key/write ordering;
- FNUM/BLOCK trajectory;
- operator ratios/detune/TL;
- envelopes and SSG-EG where supported;
- feedback;
- LFO/AMS/FMS;
- channel-3 special mode;
- authored pan;
- DAC identity.

Only then test relaxed numerical/output ceilings such as arithmetic precision, DAC-ladder behavior, alias control, and reconstruction bandwidth.

Enhanced is currently a later project. Consult the 2026-08-17 HQ backend/calibration and literature commits before reviving it.

## Highest-information experiments

1. SMPS source/config → SMPSPlay VGM → blind inverse reconstruction.
2. Repeat with GEMS as a same-hardware driver negative control.
3. Run the same YM2612 register trace through libvgm, Nuked OPN2, and ymfm/MAME-derived paths and classify disagreements by timing, arithmetic, or output-stage cause.
4. Use Sonic Retro source/disassembly alignment to determine which logical-track and driver facts survive VGM projection.
5. Use OPN2BankEditor/libOPNMIDI only after separating patch identity from driver/source identity.
6. Test Omniphony source presentation from clean FM/DAC/PSG source planes without compiler-authored spatial geometry.

## Use law

```text
VGM exactness != original driver recovery
chip accuracy != driver semantics
same hardware != same driver
same patch != same source identity
driver signature != composer identity
MIDI projection != source ontology
shared frontend != shared semantic depth
cleaner FM output != more faithful composition
```
