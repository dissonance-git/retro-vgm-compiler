# Shared feedback DSP state

## Status

Cross-device research input for realization, effects, stem isolation, causal attribution, and reference/enhanced rendering.

## Central correction

A physical voice's instantaneous dry output is not always the complete causal contribution of that voice to the later mix.

On several unrelated game-audio devices, voices feed a shared stateful effect subsystem whose memory can outlive the initiating voice episode.

Therefore:

```text
voice-local dry contribution
!= complete causal contribution to the mix
```

and, more cautiously:

```text
physical voice ended
!= all acoustic consequences of that voice ended
```

This does **not** imply that useful stems are impossible. It means stem semantics must state how shared state is handled.

## 1. Super NES S-DSP echo

Independent implementations agree on the causal shape.

### ares

Pinned observatory:

- `ares-emulator/ares`
- commit `b80f67d38312648d197762121c3a27b02c0887db`
- `ares/sfc/dsp/voice.cpp`
- `ares/sfc/dsp/echo.cpp`

Each voice first contributes its left/right amplitude to the main mix. If the voice's echo-enable state is active, the same voice contribution is also accumulated into the shared echo input/output accumulator.

The echo unit then:

1. reads prior echo samples from APU RAM;
2. keeps an eight-sample history per channel;
3. applies the programmable eight-tap FIR;
4. combines the current echo-send accumulator with filtered history through the feedback coefficient;
5. writes the result back to the echo buffer in APU RAM;
6. mixes the filtered echo return with the main output according to echo volume.

Thus a voice can deposit energy into a memory-backed feedback system which affects samples produced after the original voice contribution has changed or ended.

### blargg `snes_spc`

Independent observatory:

- `blarggs-audio-libraries/snes_spc`
- commit `ec8ee2bbe30451614c1d02a83f7af1c97d497d45`

The implementation independently retains echo input/output and echo-history state, confirming that the shared feedback/history model is not an ares-specific abstraction.

## 2. PlayStation SPU reverb

Two independent emulator lineages again agree on the broader causal structure.

### DuckStation

Pinned observatory:

- `stenzek/duckstation`
- commit `5fd366809053fe287291de7a39752c4d5d5b146b`
- `src/core/spu.cpp`

For every output frame, DuckStation:

```text
samples all 24 voices
-> accumulates the normal dry L/R mix
-> separately accumulates voices whose reverb-send bits are active
-> adds optional CD/external reverb inputs
-> runs the combined input through ProcessReverb
-> adds the reverb return to the final L/R mix
```

The reverb state includes:

- a reverb base/current address in SPU RAM;
- 32 programmable reverb registers;
- downsample and upsample history buffers;
- prior reverb input/output state.

### Beetle/Mednafen

Independent observatory:

- `libretro/beetle-psx-libretro`
- commit `12bdb844261f3d8f43d470f569eb3ace0dcad049`
- `mednafen/psx/spu.c`

The implementation similarly accumulates voice outputs selected by `Reverb_Mode` into a shared reverb input and processes that through a memory-backed work area addressed relative to `ReverbWA`/`ReverbCur`.

Its current comments additionally document active hardware-validation work around saturation and reverb timing. Those details are implementation/hardware questions below the general shared-state result and should remain separately provenance-scoped.

## 3. Capcom QSound

Pinned observatory:

- `mamedev/mame`
- commit `1e1b6ce78a1645805bb5eef4049e3f1d3f926194`
- `src/devices/sound/qsoundhle.cpp`

MAME's QSound HLE is based on disassembled DSP code.

Its saved state explicitly contains:

```text
echo feedback
echo length
last echo sample
echo delay line
echo delay position
wet delay/filter state
dry delay/filter state
```

During normal update, per-voice echo contributions are combined into one `echo_input`, the shared echo processor produces `echo_output`, and that output is then routed into the shared wet/dry filter/delay paths.

QSound therefore supplies a third independent device family where shared effect memory is downstream of many physical voices.

## 4. The effect subsystem is its own time-bearing object

The correct conceptual path is not:

```text
voice -> instantaneous effect coefficient -> output
```

but closer to:

```text
voice trajectories
-> per-voice effect sends
-> shared effect input
+
prior shared effect state
-> shared DSP transition
-> wet return
-> final realization
```

The shared effect state has its own lifetime and trajectory.

Useful coordinates include, where device-specific evidence supports them:

```text
send membership / gain
shared input sum
feedback memory/work-area contents
filter/delay state
DSP coefficients
read/write address or delay cursor
wet return
```

Do not force those into one cross-device register schema. The common claim is the causal role, not identical hardware structure.

## 5. Tail ownership is causal, not merely instantaneous

Suppose voice A sounds at time `t0`, sends into feedback, then stops.

At `t1 > t0`:

```text
voice A dry output = 0
shared wet return contains state caused partly by A
```

A frame-local attribution algorithm which inspects only currently active voices would therefore report no contribution from A even though removing A from the earlier history would change the current output.

This suggests two distinct questions:

```text
Who is producing dry energy now?
```

versus

```text
Which past sources causally contribute to the current realized output?
```

VGM Compiler should not silently answer the second with the first.

## 6. Why naive solo/mute stems can be wrong

There are several possible stem semantics.

### A. Dry physical-voice stem

Expose the voice before shared DSP.

This is exact for that coordinate, but deliberately omits the voice's effect return.

Label it accordingly.

### B. Historical-mix wet return as one shared stem

Keep all voices running normally and export:

```text
voice dry stems
+
shared echo/reverb stem
```

This preserves the exact shared DSP trajectory but does not assign the wet tail to individual voices.

For many analytical tasks this is the safest exact representation.

### C. Counterfactual per-source wet attribution

Re-run the entire causal history with one source removed or selected while preserving the effect algorithm.

This can answer useful causal questions, but the result is a **counterfactual decomposition**, not necessarily a set of additive original stems.

### D. Per-source parallelized effect instances

Run one independent effect processor per source from zero state.

This can be useful musically but is not the historical device execution unless the device itself had independent effect instances.

Do not call it exact hardware isolation.

## 7. Exact additive reconstruction is not guaranteed

For an ideal linear effect with no clipping/quantization and identical zero initial state, superposition can permit an additive decomposition.

The historical devices here contain integer arithmetic, finite-width state, quantization, saturation/clamping, address wrapping, and other implementation details.

Therefore:

```text
sum(render each source independently through cloned effect)
```

is not universally guaranteed to equal:

```text
render(all sources together through one historical shared effect)
```

Even where the result happens to match for a bounded cue, that equivalence should be tested rather than assumed.

This is especially important around loud feedback tails and saturation boundaries.

## 8. Persistent musical part and persistent acoustic consequence are different

The project's existing hierarchy distinguishes:

```text
physical slot
voice episode
persistent musical part
auditory stream/textural layer
```

This pass adds pressure from a different direction:

```text
voice episode lifetime
!= effect-state contribution lifetime
```

A note/part can stop producing new excitation while its acoustic realization continues as an echo/reverb tail.

Do not extend persistent musical-part identity merely because a tail remains. The tail is a realization consequence of earlier musical activity.

## 9. Interaction with dynamic allocation

Dynamic allocation makes shared effects especially dangerous for hardware-channel stems.

A logical part can move between physical voices while all those physical voices feed the same shared effect memory.

Therefore:

```text
physical voice number
+ current echo/reverb send
```

is insufficient to recover a persistent wet musical part.

The preferred route remains:

```text
proven logical/persistent source identity
-> dynamic physical allocations
-> device-local sends
-> shared DSP state
-> realization
```

## 10. Reference versus enhanced rendering

The reference renderer must preserve the device's historical shared-state behavior, including effect memory and finite arithmetic where reference parity is claimed.

An enhanced renderer may deliberately replace or improve an implementation stage, but must name the intervention.

Examples of distinct hypotheses:

```text
same sends + same delay/FIR topology + higher internal precision
same dry realization + reconstructed studio-quality reverb
preserve historical wet/dry balance but remove quantization noise
```

None should silently replace the historical reference.

Because shared effects are often musically intentional, they are not generic hardware damage to be stripped away.

## 11. Highest-information regression

A future shared-DSP synthetic test should use at least two excitation sources and a nonzero feedback path.

For each supported device:

1. excite source A and send it to the shared effect;
2. stop A;
3. verify the effect return persists after A's dry output becomes zero;
4. excite source B while A's tail remains;
5. compare the historical shared-effect render against separate cloned-effect renders;
6. record whether finite arithmetic makes the decomposition exactly additive;
7. verify that muting only the final dry contribution does not accidentally clear or alter shared effect memory.

The test should distinguish:

```text
dry-source trajectory
shared-state trajectory
wet-return trajectory
final mix trajectory
```

## Stop conditions

Stop rather than overclaim if:

- a stopped voice is assumed to have no later acoustic consequence;
- a shared echo/reverb return is assigned to whichever voice is active at the current frame;
- physical-channel soloing is called an exact musical stem without specifying effect-state handling;
- cloned per-source effects are called historical hardware behavior;
- additive stem reconstruction is assumed without testing finite arithmetic;
- shared DSP state is copied into every voice object as though it were voice-local;
- an intentional historical effect is removed by an enhanced renderer without explicit evidence/provenance;
- a reverb tail is mistaken for continued persistent-part activity.

Correction outranks coherence.
