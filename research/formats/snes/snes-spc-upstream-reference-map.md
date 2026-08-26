# SNES / SPC upstream reference map

Status: durable research re-entry map for external projects that can constrain SNES/SPC source separation, reference rendering, sample provenance, and later source-quality experiments. This is not a dependency lockfile and it does not promote any one implementation into universal authority.

## Research question

```text
SPC snapshot / driver state
        ↓
SPC700 + S-DSP execution
        ↓
dry voice trajectories + shared echo field
        ↓
reference stereo or source-separated presentation
```

Which external projects can test each layer, and where does each project's evidential authority stop?

## Primary current control: `dgrfactory/spcplay` / SNESAPU

Use it for:

- the editable SNESAPU implementation currently patched by VGM Compiler;
- direct S-DSP voice, routing, echo, FIR, interpolation, and mute behavior;
- differential dry/wet controls;
- later interpolation/HQ reconstruction experiments.

The upstream DSP API exposes several unusually useful controls:

```text
DSP_NOECHO  -> disable echo
DSP_NOMAIN  -> disable main output, leaving echo
DSP_NOFIR   -> disable FIR
INT_GAUSS   -> SNES-style Gaussian
INT_SINC    -> 8-point sinc
```

This is strong independent precedent for treating the native program path and the shared echo return as separable S-DSP products. It is also a useful offline oracle against the maintained realtime single-pass taps.

Relevant upstream surfaces include:

- `snesapu.dll/DSP.h`
- `snesapu.dll/DSP.asm`
- `snesapu.dll/APU.h`
- `snesapu.dll/readme.txt`

Do not use optional SNESAPU processing modes as automatic proof of historical hardware behavior. The exact mutable revision required by the build remains owned by the patch/build integration route.

## `blarggs-audio-libraries/snes_spc`

Use it for:

- compact high-accuracy S-DSP behavior;
- Gaussian interpolation reference behavior;
- independent mixer/source-tap placement checks;
- per-voice mute differentials.

Its DSP flow gives a useful boundary:

```text
BRR decode / noise
→ interpolation
→ envelope
→ voice output
→ signed VxVOLL / VxVOLR
→ main accumulator
→ optional EON echo-send accumulator
```

That makes it a strong falsifier for dry-voice tap placement. It does not make a cleaner non-Gaussian interpolation "more historically accurate."

Historical project work also built an isolated instrumented `snes_spc` forensic target. Search commit history around the 2026-08-16 `snes_spc` transformer/bridge commits before rebuilding that observatory from scratch.

## `libretro/bsnes-jg`

Use it for controlled reconstruction experiments.

The inspected implementation exposes:

```text
spc_interp = 0  -> Gaussian
spc_interp = 1  -> Sinc
```

and keeps SPC interpolation selection distinct from final output resampling.

This is useful for a clean later experiment:

```text
same BRR
same pitch trajectory
same envelope
same routing
same echo state
only interpolation changes
```

Do not call the sinc result the reference S-DSP realization.

## `gocha/split700`

Use it for sample identity and provenance work.

It can:

- extract BRR samples from SPC files;
- select samples by SRCN;
- preserve loop-point information;
- convert extracted BRR to WAV for inspection.

Useful chain:

```text
SRCN / BRR identity
→ extracted encoded sample
→ decoded waveform
→ upstream sample-library candidate search
→ explicit preparation fit
→ forward BRR validation
```

It does not preserve the complete performed voice. Pitch, envelope, PMON/NON, routing, and shared echo remain separate runtime evidence.

## `aikiriao/spc700` and `aikiriao/spc2midi-tsuu`

These were already identified in earlier project research and were easy to lose after the upstream-registry cleanup.

Use them for:

- executable SPC700/S-DSP state as a route toward note-like musical variables;
- observing which voice properties can be projected into note, pitch bend, pan, volume/expression, sample/program, and effect-send concepts;
- negative evidence about what a MIDI projection throws away.

They are not the target architecture. Their value is showing recoverability from execution and the information loss caused by flattening rich DSP state into MIDI.

Historical Gigo/Hill `spc2midi` work belongs in the same prior-art class.

## `danielburgess/Mesen2-Diz`

Use it as a forensic state observatory rather than as the canonical audio renderer.

Especially useful:

- SPC RAM capture;
- full emulator savestates;
- correlated periodic snapshots and traces;
- debugger inspection around difficult audible events.

The SplitTrace work can create reproducible bundles around a disputed event, making it useful when the protected render, source tap, and reconstructed state disagree.

## `osoumen/C700`

Use it as an architectural and synthesis-side pressure test.

Its explicit `EchoKernel` keeps separate:

- echo volume;
- FIR taps;
- delay;
- feedback/buffer state.

That is useful independent precedent for keeping a shared environmental/effect state distinct from dry voice synthesis. C700 is not an arbitrary-SPC reference playback authority.

## `mitsuhito-takamoto/SPCPlayPlugin`

Use it as a modern observability/integration lead around SPCPlay/SNESAPU.

The inspected plugin-side definitions expose DSP/voice state including `mOut`, the last sample output before channel volume, plus sample history and interpolation-related state.

This can help with tooling, state inspection, and plugin integration, but it is not independent arithmetic validation of SNESAPU itself.

## Historical wrapper: `foo_snesapu`

Earlier research recorded the kode54 / Christopher Snowhill foobar wrapper as historical integration provenance.

Use it to answer wrapper/component-history questions. Do not treat its old public release date or bundled runtime as the desired rendering baseline.

## Immediate source-separation consequence

The strongest current contract remains deliberately small:

```text
8 x dry S-DSP voice trajectories
+ authored signed per-voice L/R routing
+ per-voice EON/send state
+ 1 linked stereo native S-DSP echo return
→ Omniphony
```

SPCPlay's main-only / echo-only controls provide a valuable independent differential check, while the maintained single-pass source tap is preferable for realtime delivery because every lane shares one execution timeline.

Do not manufacture per-voice wet stems from the shared S-DSP feedback system.

## Later Enhanced / remaster quarry

Keep these as distinct interventions:

```text
A. native Gaussian S-DSP reference
B. same BRR + higher-quality reconstruction
C. proven pre-BRR/original-library source + exact preparation
D. inferred bandwidth extension / generative restoration
```

Useful projects by rung:

- A: SNESAPU, `snes_spc`;
- B: bsnes-jg, SNESAPU interpolation controls;
- C: split700 plus source-library/provenance tooling;
- D: research-only restoration literature, never silent source truth.

Earlier project commits around 2026-08-16 to 2026-08-17 already built evidence gates, original-sample registries, candidate ranking, source-lineage checks, and a studio sampler. Consult that history before reopening the Enhanced program.

## Highest-information regressions

1. Compare the maintained single-pass taps against SNESAPU `DSP_NOECHO` and `DSP_NOMAIN` renders.
2. Use `snes_spc` mute/mixer behavior as an independent dry-tap placement control.
3. Hold execution fixed and compare Gaussian versus sinc reconstruction.
4. Attach stable SRCN/BRR identities to upstream-sample candidate searches through split700.
5. Use Mesen2-Diz state bundles for timing/state disputes that cannot be settled from final PCM alone.

## Use law

```text
implementation convenience != hardware authority
cleaner interpolation != historical accuracy
BRR identity != performed voice
echo-send membership != per-voice wet stem
MIDI projection != source ontology
debug state != composer intent
```
