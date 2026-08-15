# VGM / SPC designer-grade closure

Status: active closure control  
Scope: stable VGM through 1.71 and SPC v0.30 / S-SMP / S-DSP execution  
Purpose: replace informal percentage estimates with executable stop conditions for the claim "understood as if we designed it"

## Definition of 100%

For this project, `100%` does **not** mean recovering information that the preserved artifact never contained.

It means:

```text
for every preserved observable
→ its representation and transition law are known
→ its provenance is explicit
→ edge cases have executable controls

for every higher claim that is not uniquely recoverable
→ the loss/ambiguity boundary is explicit
→ competing explanations remain representable
→ no downstream inference is silently promoted to source truth
```

A proven information-loss boundary is closure. An unexplained unknown is not.

The claim is always scoped. VGM 1.72 is currently beta and is not included in the stable-VGM closure target. A future stable revision reopens the format-level closure gate.

## Validation method

Use four independent evidence classes where they exist:

1. format specification or first-party documentation;
2. mature independent implementations;
3. real immutable corpus objects;
4. synthetic/known-answer regressions that isolate one rule.

For emulation/rendering claims, compare state trajectories and continuous output rather than only successful playback. Perceptual enhancement is a separate listening-validation axis and must not be used to certify reference accuracy.

## VGM 1.71 closure matrix

### A. Container and transport

Required before format-level closure:

- [x] VGM/VGZ identification and gzip handling;
- [x] version-aware data offset;
- [x] version-aware chip-clock header fields through 1.71;
- [x] legacy 1.00/1.01 overloaded YM clock interpretation;
- [x] dual-chip and selected bit-31 variant semantics;
- [x] wait/sample accounting;
- [x] loop boundary and loop-duration validation;
- [x] variable-length `0x67` data blocks;
- [x] `0x68` PCM RAM write framing;
- [x] DAC Stream Control `0x90-0x95` framing;
- [x] command-width families sufficient to walk the complete 1.71 stream;
- [ ] EoF-offset consistency as an explicit validation state;
- [ ] GD3 framing/offset/version/string-block validation;
- [ ] reserved-header-byte validation;
- [ ] VGM 1.70 extra-header bounds, second-chip clocks and chip-volume list parsing;
- [ ] data-block type classification/provenance, including unknown-type skip behavior;
- [ ] version gates that distinguish stable 1.71 commands from 1.72 beta additions;
- [ ] synthetic command-table regression covering every assigned and reserved 1.71 opcode family.

Format-level `100%` is blocked until every unchecked item above is executable and the real corpus remains admitted.

### B. Device execution

VGM is a transport for many independent sound devices. Format closure is not device closure.

For each device family used by a target work, require:

```text
VGM command
→ device-native write / memory operation
→ autonomous device state evolution
→ source/resource trajectory
→ reference acoustic contribution
```

A family is closed only when its relevant modes, coupling, shared generators, mixing/effects, mutable memory and output topology are covered by tests or bounded as unsupported.

Genesis YM2612/SN76489 is currently the deepest device vertical slice. Important remaining audible gap:

- coherent isolated six-channel YM2612 rendering from one shared FM engine, preserving global/LFO/phase state and exact VGM-tick scheduling.

Do not obtain six stems by instantiating six unrelated emulators.

### C. Source/musical inversion

VGM normally preserves executed device-facing behavior, not the original driver program.

The Sonic SMPS control already proves a representative non-invertibility:

```text
SMPS note token + transpose + detune/modulation
→ executed YM2612 FNUM/BLOCK

executed FNUM/BLOCK
↛ unique original SMPS token/state in general
```

Designer-grade understanding therefore requires the inverse side to return candidate source explanations with provenance, not fabricate one source token.

This is a closed information boundary when the forward map is known and the surviving equivalence class is explicit.

## SPC closure matrix

### A. SPC v0.30 snapshot

Required snapshot facts:

- [x] header/signature and conventional file image;
- [x] SPC700 PC/A/X/Y/PSW/SP register image;
- [x] 64 KiB SPC700 RAM image;
- [x] 128-byte S-DSP register image;
- [x] optional/trailing-state distinction;
- [x] static voice-register slots kept distinct from live voice episodes;
- [x] BRR directory/sample storage identity kept distinct from SRCN and instrument identity;
- [x] mutable RAM generation tracking for runtime source identity.

### B. Executable continuation

SPC closure requires the preserved machine to continue, not merely parse.

Required runtime semantics include:

- [x] bounded physical voice episode lifecycle;
- [x] accepted KON / key-on-delay distinction;
- [x] KOFF as release rather than immediate episode end;
- [x] runtime source/SRCN/BRR continuity;
- [x] event-time RAM generations;
- [x] device-native pitch-rate observations;
- [x] envelope state observations;
- [x] noise-source selection;
- [x] signed left/right per-voice routing;
- [x] echo-send state;
- [x] cross-resource pitch-modulation causal model;
- [x] shared noise-generator model;
- [x] BRR loop realization depends on encoded flags plus directory/runtime state;
- [x] shared echo state treated as a persistent system rather than source-local decoration;
- [x] finite-width mix behavior kept distinct from an ideal additive stem model;
- [ ] editable playback core reconciled against a cycle-faithful independent S-SMP/S-DSP control at state-transition boundaries;
- [ ] deterministic per-voice dry PCM taps at the correct pre-routing/pre-main-mix coordinate;
- [ ] explicit shared echo-return tap with preserved feedback state;
- [ ] reference reconstruction test proving that source lanes + shared system state reproduce the protected reference path within the declared arithmetic boundary;
- [ ] seek/reset/save-state trajectories compared against uninterrupted execution.

Blargg's `snes_spc` is an important independent control because its accurate DSP reports validation against more than one hundred timing/behavior tests run on SNES hardware and models DSP register/memory access at exact SPC-cycle timing. It is an observatory, not a replacement architecture.

### C. Driver and musical semantics

An SPC snapshot may contain an arbitrary SPC700 music driver/program. S-DSP closure therefore does not imply exact logical-track/note/instrument semantics for every game.

For a specific work/driver, require:

```text
runtime code/data identity
→ driver lineage / command grammar
→ logical execution contexts
→ allocation into S-DSP voices
→ device trajectory
```

If driver identity is unknown, `note`, `instrument`, `track` and persistent-part claims remain hypotheses. This is not a defect in SPC decoding.

`100% SPC` is therefore two simultaneous claims:

1. the preserved machine state and its continuation are completely modeled at the chosen reference boundary;
2. every higher musical fact is either recovered through a proven driver path or explicitly represented as source-dependent/underdetermined.

## Spatial consequence

Designer-grade source understanding must precede source-aware spatial rendering because ordinary stems are not always a faithful causal decomposition.

The source bus must distinguish:

```text
source trajectory
!= exact additive stem
!= shared effect return
!= protected reference mix
```

Examples:

- S-DSP dry voice contribution can be isolated while echo remains one shared feedback system;
- PMON can make one physical voice's trajectory depend on another voice;
- finite-width/nonlinear mixing can make `render(all)` differ from `sum(render(each))`;
- signed routing is exact device evidence but not an authored 3-D coordinate.

## QSound as a spatial control

QSound is useful as a historical architecture teacher, not as the target renderer.

Its DSP keeps multiple PCM/ADPCM voices separate through per-source pan, distinct dry/wet transfer functions, shared echo, filtering and delay before the final stereo collapse.

Transferable law:

> preserve source identity until the spatial/presentation stage.

Do not copy QSound's final two-channel geometry as the ceiling. Omniphony can render a head-relative full sphere.

## GMI → Omniphony authority boundary

Game Music Interpreter owns source truth:

```text
stable source identity / generation
physical resource identity
source trajectory or isolated dry audio where valid
native signed stereo routing
shared-effect send and causal dependencies
persistent-part evidence/confidence
reference-mix control
```

Omniphony owns presentation:

```text
inferred position
extent/spread
distance/depth
height/rear placement
scene smoothing
head-relative binaural rendering
```

Native L/R routing constrains presentation but is never relabeled as authored 3-D geometry.

A persistent musical part may keep one presentation identity while physical voice allocation changes. Conversely, a physical voice reused for a new part must not drag the old position with it.

## Full-sphere game-music target

The target is not generic stereo widening and not a fixed virtual-speaker upmix.

```text
causal game-music sources
→ stable perceptual/musical grouping
→ conservative presentation scene
→ full-sphere object rendering
→ measured-HRIR / ITD / distance / reflection path
→ binaural headphones
```

Presentation policy should preserve the protected mix's center, bass/foundation, groove, source fusion and intentional native routing while using the source decomposition to create lateral, front/rear, depth and height separation that cannot be recovered reliably after stereo summation.

The shared wet field should be rendered as environment/space, not duplicated as fake point-source reverb stems.

## Stop condition

Do not call either format 100% because a file plays or a parser accepts it.

Closure is earned only when every unchecked observable above has either:

- an executable implementation and regression;
- an independent validation control;
- or a demonstrated information boundary explaining why the requested higher fact is not present in the source.

Correction outranks a convenient percentage.
