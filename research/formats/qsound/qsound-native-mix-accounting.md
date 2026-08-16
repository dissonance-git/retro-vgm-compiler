# QSound native mix accounting

## Question

Once GMI can observe QSound's 19 causal pre-pan source lanes, what additional renderer evidence can be extracted without pretending the shared QSound environment is a set of independent per-source wet stems?

The useful boundary is not an invented stem model. It is the exact native-rate accounting already used by the historical superctr QSound renderer.

## Pinned upstream boundary

The implementation target remains the superctr QSound core in the pinned libvgm lineage used by the VGM component:

```text
ValleyBell/libvgm
61fc6725644886abc3168e240e4e51588d74bdf7
```

Relevant implementation:

```text
emu/cores/qsound_ctr.c
```

The existing native source observer exposes:

```text
16 PCM source outputs
+ 3 ADPCM source outputs
= 19 pre-pan causal source samples
```

The mix observer added by `0007-qsound-native-mix-observer.patch`, with freshness guarded by `0008-qsound-native-mix-validity.patch`, observes later terms from the same coherent native DSP timeline.

`0009-qsound-native-echo-send-witness.patch` adds one deliberately narrow verifier witness: the 16 signed PCM echo-contribution words read from the same native superctr state as the source samples and `echo_input`. They are not extra audio lanes.

## Exact captured native terms

For each native QSound DSP tick the project-side mix frame can carry:

- absolute native sample index;
- native sample rate;
- `accounting_valid`;
- 16 signed PCM echo-contribution words;
- shared `echo_input`;
- shared `echo_output`;
- left/right `wet_post_delay` branch values;
- left/right `dry_post_delay` branch values;
- left/right historical reference output.

The wet/dry branch values are captured immediately before the historical final branch sum, DSP rounding and output clamp.

In the audited core the upper part of the relation is structurally:

```text
wet post-delay + dry post-delay
        ↓
Q14-style DSP round
        ↓
clamp to [-32767, +32767]
        ↓
reference output
```

The observer names already-existing state and return values. It does not introduce a second renderer, clone DSP state or alter the branch state machines.

## Freshness law

A native timeline tick existing does **not** imply fresh mix accounting exists for that tick.

The QSound core also executes initialization and filter-refresh states. Those states can advance the native DSP timeline without running the normal-render path that computes fresh echo and wet/dry branch accounting.

Therefore `0008-qsound-native-mix-validity.patch` makes freshness explicit:

```text
every DSP tick begins accounting_valid = false
        ↓
normal-render update completes
        ↓
accounting_valid = true
```

Only the normal render path earns fresh accounting.

On a tick where accounting is unavailable, observer fields, including the PCM echo-send witnesses, are exported as unavailable/zero rather than silently reusing stale values from an earlier normal-render tick.

This distinction is evidence, not a second timeline. Source frames and mix frames still share one native sample index.

## Exact echo-input closure law

The first internal QSound environment identity can now be tested without solving native-to-consumer timing.

For each fresh normal-render native tick the recovered superctr relation is:

```text
for PCM voice i in 0..15:
    echo_input += (source[i] * pcm_echo_contribution[i]) << 2
```

The 3 ADPCM voices do not own the corresponding echo-contribution register in the recovered register map and therefore do not enter this sum.

`components/vgm/enhancement/qsound_echo_input_recomposition.h` independently recomputes this relation from:

```text
native source frame
+
same-tick PCM echo-send witness
```

and compares the result with captured `echo_input`.

The verifier:

1. requires `accounting_valid == true`;
2. requires the source and mix frame to carry the same native sample index;
3. evaluates products and the running sum in a wide type;
4. refuses to certify any tick where the intended multiply-by-four term or running accumulator leaves the historical signed-32-bit domain;
5. compares the resulting exact value with captured `echo_input`.

This is deliberately narrower than reproducing the echo effect. It proves or falsifies only the first causal boundary:

```text
16 PCM source samples
+
16 signed echo-send words
        ↓
shared echo_input
```

A passing result does not establish `echo_output`, FIR, delay or final stereo behavior.

## Exact reference recomposition law

`components/vgm/enhancement/qsound_reference_recomposition.h` provides a deterministic ruler for the final stereo arithmetic.

For a frame with `accounting_valid == true`, it reconstructs each historical output channel from:

```text
wet_post_delay[channel]
+
dry_post_delay[channel]
```

using explicit integer semantics matching the recovered QSound renderer:

1. form the branch sum in a wide type;
2. reject the historical signed-32-bit overflow domain instead of inventing compiler-specific behavior;
3. add the DSP rounding bias `0x2000`;
4. apply explicit arithmetic-right-shift-equivalent Q14 rounding;
5. clamp to `[-32767, +32767]`.

If the reconstructed value equals the observer's historical reference output, the frame earns exact final-output recomposition under that defined domain.

An unavailable mix tick does not earn recomposition merely because it has a native timestamp.

## Command-layer environment evidence

The native mix observer is complemented by a separate command-layer state model for QSound environmental controls.

`qsound_environment_control_state` records only control words actually observed in VGM `0xC4` traffic, including:

- global echo feedback `0x93`;
- echo end/delay position `0xD9`;
- left/right wet filter-table selection;
- left/right dry filter-table selection;
- left/right wet delay;
- left/right dry delay;
- delay-update command;
- next-state command;
- left/right wet output volume;
- left/right dry output volume.

The existing `qsound_block_capture` owns one C4 timing parser and routes source-local controls and shared environment controls into separate typed streams. It does not create a second capture architecture for the environment.

`qsound_environment_control_replay` provides a bounded replay of the environment stream into `qsound_environment_control_state`. It validates the complete bounded timeline before mutation. Environment overflow or a backward timeline therefore applies zero writes rather than leaving a plausible-looking partial shadow.

That separation matters:

```text
source-local route/send evidence
!=
shared environment control evidence
!=
internal shared DSP trajectory
```

The command shadow deliberately does not manufacture mode-specific initialization values, decoded FIR taps, echo history, delay cursors or state-machine progress that were not observed in the command stream.

## What the captured terms mean

The 19 `voice_output` lanes are causal source-audio evidence before QSound's dry/wet pan tables.

The 16 PCM echo-send words are exact same-tick verification witnesses for how PCM source audio enters the shared echo accumulator. They are not wet audio.

`echo_input` is one shared aggregate assembled by the historical engine. It is not nineteen independent echo lanes.

`echo_output` is the output of one shared echo state machine. Its future value can depend on source activity from earlier ticks.

`wet_post_delay[L/R]` and `dry_post_delay[L/R]` are the exact final delayed branch contributions used by the historical stereo sum for a fresh normal-render tick.

They are useful because they expose causal accounting and permit exact validation. They are **not** proof that the QSound environment is separable into independently editable source wet stems.

## What this does not mean

The following claims remain invalid:

- 19 independent QSound wet stems exist;
- final QSound stereo is universally the linear sum of 19 independently processed source stems;
- `wet_post_delay` is a pure reverb-only return;
- `dry_post_delay` is an untouched source-only path;
- one source can be removed from the wet branch without changing shared echo/FIR/delay history;
- observed shared environment controls alone reconstruct the full hidden DSP state;
- exact `echo_input` recomposition establishes exact `echo_output` behavior;
- native QSound samples can be labeled directly with consumer/output sample indices;
- QSound pan words are authored 3-D coordinates.

Shared echo memory, filters, delays, state-machine behavior and finite arithmetic all survive above the 19 pre-pan source lanes.

This is the same general warning established by `shared-feedback-dsp-state.md`: a source's direct lifetime and its causal contribution to shared effect state are not the same boundary.

## Current safest representation

The strongest representation currently earned is:

```text
exact VGM C4 timing
    ├─ source-local QSound controls
    └─ observed shared environment controls
         └─ bounded atomic command-state replay

one coherent native QSound timeline
    ├─ 19 exact pre-pan causal source lanes
    └─ fresh shared mix accounting when available
          ├─ 16 same-tick PCM echo-send witnesses
          ├─ echo_input
          ├─ echo_output
          ├─ final wet branch L/R
          ├─ final dry branch L/R
          └─ historical stereo witness

exact bounded rulers
    ├─ PCM sources + echo sends -> echo_input
    └─ wet/dry final branches -> historical stereo output
```

This is stronger than final stereo alone and stronger than register state alone, while remaining weaker than fictional independent wet stems or a replayed QSound DSP.

## Consumer-rate consequence

The future source-bus problem still requires one coherent rate/time conversion.

The native QSound rate remains:

```text
chip clock / 2 / 1248
```

libvgm's resampler then maps the native engine to the destination/output timeline.

A correct future source bus therefore needs one coherent shared-phase conversion for all time-varying audio evidence that came from this native engine. It must not instantiate unrelated resamplers for the 19 source lanes.

The same phase/time mapping should also provide an explicit correspondence for native mix-accounting witnesses. Discrete freshness flags and command events must remain discrete evidence rather than being interpolated as if they were audio.

The echo-input closure ruler intentionally runs before this conversion because both sides of its equation live on the same native tick.

## Reference-render ownership

The foobar wrapper attaches both QSound observers only around normal reference decode blocks.

It detaches them on normal completion, exception paths and seek. Discarded seek audio is not admitted as evidence for the next audible block.

The live mix callback copies the 16 echo-send witness words, shared echo accounting and final branch values into the bounded project-side mix frame. If the source-audio and mix-accounting sidecars disagree on native sample rate, frame count or first native sample index, both are invalidated. The wrapper does not repair the discrepancy.

Most importantly:

```text
historical libvgm QSound render -> p_chunk
observer sidecars              -> evidence only
```

No current QSound sidecar rewrites audible playback.

## Tests and current evidence state

Deterministic rulers now include:

- `tests/vgm/qsound_control_capture_test.cpp`;
- `tests/vgm/qsound_environment_control_state_test.cpp`;
- `tests/vgm/qsound_environment_control_replay_test.cpp`;
- `tests/vgm/qsound_native_source_capture_test.cpp`;
- `tests/vgm/qsound_native_mix_capture_test.cpp`;
- `tests/vgm/qsound_echo_input_recomposition_test.cpp`;
- `tests/vgm/qsound_reference_recomposition_test.cpp`;
- `tests/vgm/test_qsound_native_source_patch.py`;
- `tests/vgm/test_qsound_native_mix_patch.py`;
- `tests/vgm/test_qsound_native_mix_validity_patch.py`;
- `tests/vgm/test_qsound_native_echo_send_patch.py`;
- `tests/vgm/test_qsound_live_source_wiring.py`;
- `tests/vgm/test_qsound_live_mix_wiring.py`.

`tools/run_core_tests.py` discovers all dependency-free `*_test.cpp` rulers automatically. The manual `core-tests` workflow also names the QSound source, environment, mix, echo-input and final-output rulers explicitly.

`tools/check_libvgm_patches.py` discovers all `patches/libvgm/*.patch` files in sorted order, so `0009` automatically enters pinned-upstream patch verification after `0008`.

GitHub-hosted execution remains unavailable while runner jobs are rejected before start by the account billing/spending limit, so this is **not** a green-CI claim. This environment also does not have a reachable pinned libvgm checkout, so the newly extended `0001..0009` patch series has not been executed through `git apply --check` here.

The project has not yet earned a direct before/after full-render parity result for the patched pinned libvgm core on a real QSound corpus. Source-level preservation and deterministic rulers are strong evidence, but they are not a substitute for that parity experiment.

## Current claim

The earned statement is now:

> GMI can observe QSound's 19 native pre-pan causal source lanes and, on every native DSP tick where normal-render accounting is fresh, carry the exact PCM echo-send witnesses needed to independently reconstruct the shared echo input under a declared signed-32-bit domain, while also observing the later shared echo and final wet/dry branch accounting needed to reconstruct the historical stereo sample under its declared integer boundary. Timed shared-environment controls remain separate evidence, and historical playback remains authoritative.

The stronger statement remains unearned:

> GMI has 19 independent wet/dry QSound stems, a fully replayed QSound environment, or a complete consumer-rate QSound source bus ready for Omniphony.

## Next discriminating rung

The next scientific rung should stay inside the native QSound timeline:

1. execute the full `0001..0009` patch checker against the pinned libvgm checkout;
2. run the new echo-input closure ruler on real QSound frames;
3. preserve the observed command/environment state needed by the recovered echo process;
4. implement the shared echo state alone and compare predicted `echo_output` against the native observer;
5. only after echo-state closure, move outward through pre-FIR branch construction, FIR/delay state and consumer-rate conversion;
6. keep direct patched-versus-reference full-render parity as the protected behavioral control throughout.

That sequence turns one coupled renderer into a ladder of independently falsifiable identities instead of replacing it with a large speculative clone.
