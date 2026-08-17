# libgme SPC native-source observer seam

## Status

Pinned implementation contract for exposing exact S-DSP pre-pan voice PCM from a current public libgme/Game Music Emu dependency.

This note narrows the source-observation boundary. It does not claim that the dependency patch has already been wired into a foobar frontend.

## Upstream basis

Current public upstream:

- https://github.com/libgme/game-music-emu
- `gme/Spc_Dsp.cpp`
- `gme/Spc_Emu.cpp`
- `gme/Spc_Emu.h`
- `gme/Fir_Resampler.h`

The relevant SPC modules are LGPL 2.1-or-later in the current public source.

A second independent implementation was inspected for corroboration:

- https://github.com/nununoisy/spc-presenter-rs
- bundled `external/snes-apu-spcp`

Both implementations expose the same causal synthesis boundary.

## Exact signal boundary

Current libgme computes each physical voice in `Spc_Dsp::run()` approximately as:

```text
BRR decode or noise source
-> pitch / PMON consequences
-> Gaussian interpolation
-> envelope
-> signed mono voice `output`
-> multiply by per-voice VOLL / VOLR
-> accumulate into dry master L/R
-> if EON: accumulate routed L/R into shared echo input
```

The desired dry-source observation is the mono `output` value immediately before the VOLL/VOLR multiplication.

This value is already clamped/contained in the signed 16-bit domain by the current synthesis arithmetic and is suitable for transport as `int16_t` device-native amplitude.

It is **not**:

- a final stereo stem;
- master-volume adjusted;
- echo-return adjusted;
- post-`Spc_Filter`;
- a musical note identity;
- an instrument identity;
- an authored 3-D coordinate.

## Why this is better than mute/subtract extraction

The observer reads the causal internal source signal before downstream route mixing.

It therefore preserves:

- BRR/noise realization;
- interpolation;
- envelope state;
- physical voice timing;
- pitch-modulation consequences already present in the synthesized signal.

It avoids baking in:

- signed native stereo routing;
- shared echo-return state;
- global master output gain;
- final output filtering.

Mute/subtract would instead estimate a source by changing the machine state or renderer configuration and comparing final mixes. That is unnecessary when the source signal is directly observable inside the emulator.

## Observer contract already defined in this repository

See:

- `components/spc/spc_native_source_capture.h`

The neutral callback boundary is:

```cpp
using spc_native_source_observer = void (*)(
    void* user,
    std::uint32_t sample_rate,
    std::uint64_t native_sample,
    const std::int16_t* source,
    std::size_t voice_count);
```

Required values for the current S-DSP path:

```text
sample_rate = 32000
voice_count = 8
source[i] = pre-VOLL/VOLR mono output for physical DSP voice i
```

The callback is observation-only.

## Minimal dependency patch shape

Inside one `Spc_Dsp::run()` hardware-frame iteration:

1. create a local eight-element signed 16-bit source array;
2. as each voice computes its final mono `output`, copy that value into the corresponding slot;
3. leave all existing VOLL/VOLR, PMON, echo, master-mix, FIR, register, and mute behavior untouched;
4. after all eight voices have completed for that hardware frame, invoke the observer once;
5. increment a native hardware-frame ordinal independently of whether an observer is installed.

Pseudocode only:

```cpp
int16_t observed_source[8] = {};

for each DSP voice:
    ... existing synthesis ...
    int output = ...;
    observed_source[voice_index] = static_cast<int16_t>(output);
    ... existing VOLL/VOLR + dry/echo accumulation ...

if (observer != nullptr)
    observer(user, 32000, native_sample, observed_source, 8);
++native_sample;
```

The actual patch must use the dependency's existing loop/index structure rather than refactoring synthesis merely to make the callback convenient.

## Native ordinal semantics

`native_sample` is an S-DSP hardware-frame ordinal for the controlled execution.

It must be monotonic while the emulated execution advances, including frames generated internally by skip/resampler machinery if the dependency counter lives at the DSP boundary.

The capture object does not require the first ordinal of a new trace to be zero. `reset_trace()` only clears the expected-continuation relation, allowing the next observed frame to establish a fresh start.

This matters for seeking:

- a restart-from-beginning seek may naturally restart the dependency ordinal;
- an emulator `skip()` seek may advance the dependency ordinal through discarded frames.

Both are acceptable as long as the host declares a new semantic playback epoch and starts a fresh source-capture trace.

## Frontend lifecycle around seek and skip

The observer must be active only around protected audible render windows.

Recommended frontend flow:

```text
track start / seek / decoder reset
    -> disable source observer
    -> perform emulator start/skip/reset work
    -> reset runtime semantic trace
    -> reset native source capture trace
    -> reset host playback session epoch

normal decode window
    -> begin runtime capture window
    -> begin native source capture block
    -> install/enable observers
    -> protected Spc_Emu::play(...)
    -> disable observers
    -> close captures
    -> feed matching reference window into spc_runtime_host_pipeline
```

Do not leave the source capture active across `Spc_Emu::skip()` and then interpret the skipped native frames as audible host frames.

## 32 kHz is the first exact host-audio path

Current public `Spc_Emu` states that S-DSP hardware output is natively 32 kHz.

When the requested output rate is 32 kHz:

```text
Spc_Emu::play_()
-> play_and_filter()
-> APU native 32 kHz output
```

The `Fir_Resampler` path is bypassed.

Therefore one protected reference frame corresponds to one native DSP frame before the later stereo `Spc_Filter` stage.

Repository implementation:

- `components/spc/spc_native_exact_source_storage.h`
- `spc_runtime_host_pipeline::consume_native_reference_window(...)`

The exact native path rejects all non-32-kHz host rates.

## Why 44.1/48 kHz remain evidence-only today

At non-native rates, current libgme uses `Fir_Resampler<24>`.

Its output alignment depends on internal state including:

- FIR input history;
- `imp_phase`;
- the resampler step/skip pattern;
- buffered unread input;
- skip behavior around seek;
- the actual rounded ratio selected by `time_ratio()`.

A simple absolute-rate ratio does not reproduce that state machine.

Therefore:

```text
32000 Hz protected output
    -> exact dry-source PCM may be admitted

other protected output rates
    -> exact runtime routing/effect evidence may still be admitted
    -> dry PCM stays unavailable until FIR phase/history is observed or mirrored
```

Do not linearly interpolate the native voice frames and call that emulator-exact source audio.

## Post-mix `Spc_Filter` is a separate frontier

`Spc_Emu::play_and_filter()` runs the APU and then applies `Spc_Filter` to the final interleaved stereo output.

The native pre-pan source observer sits inside the APU/DSP and therefore does not include this final filtering stage.

Consequences:

- exact pre-pan source observation is still valid;
- native source lanes plus VOLL/VOLR do not yet imply bit-exact reconstruction of the protected `Spc_Emu` stereo output;
- a source-aware renderer must explicitly decide where equivalent post-render coloration belongs;
- `spatial_capabilities_for(spc).exact_linear_recomposition` must remain false.

Do not independently run a nonlinear or stateful final-output stage on every voice unless its separability has been proven.

## Shared echo remains shared

The pre-pan dry observer does not solve echo decomposition.

Current S-DSP flow routes selected voice contributions into a shared stereo echo accumulator, delay RAM, FIR history, and feedback network.

Therefore:

```text
per-voice EON/send state
!= per-voice wet-return stem
```

A future shared-effect observer should expose the shared echo return as its own bus evidence/audio object. It must not copy the complete wet return into every sending voice.

## Composer evidence and interpretation boundary

The companion synthesis in:

- `research/runtime/source-aware-host-rendering-evidence.md`

records composer testimony that SNES pan, volume, effects, channel deployment, and echo choices could be intentional parts of the composition/arrangement.

That supports preserving native routing and effect topology.

It does not permit composer testimony to overwrite runtime state. Runtime/device evidence remains authoritative.

## Current claim boundary

The repository now has executable model/capture machinery for:

- exact runtime physical-voice identity and generation;
- exact signed VOLL/VOLR evidence;
- exact per-voice echo-send evidence;
- bounded host playback epochs;
- a neutral native pre-pan source callback contract;
- exact 32 kHz native-source storage and host-pipeline admission.

The repository does **not yet** have the actual libgme dependency observer wired into a live foobar SPC decoder.

Until that final dependency/frontend connection exists, keep the global SPC capability table conservative: no family-level claim that isolated dry PCM is currently available in live playback.

## Stop conditions

Stop and correct the implementation if any of these occur:

- source capture is taken after VOLL/VOLR and then advertised as pre-pan;
- PMON consequences are removed from a voice in pursuit of a cleaner-looking stem;
- SRCN changes are promoted to automatic new physical-voice identities;
- observer callbacks mutate DSP synthesis state;
- skipped seek audio is attached to audible host frames;
- a simple 32k→44.1/48k ratio is called exact libgme resampling;
- final `Spc_Filter` coloration is silently lost while claiming reference equivalence;
- shared echo return is duplicated per sending voice;
- missing or discontinuous source frames are repaired without explicit evidence;
- the family capability table is upgraded before the live dependency path is wired and validated.

Correction outranks coherence.
