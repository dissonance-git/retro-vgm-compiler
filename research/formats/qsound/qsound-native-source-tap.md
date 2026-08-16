# QSound native source tap

## Question

Where can GMI obtain causal QSound source audio without changing the historical QSound renderer or confusing the QSound core's native timeline with the foobar/output timeline?

## Upstream control

The implementation target is the superctr QSound core in the pinned libvgm lineage used by the VGM component. The audited upstream revision is:

```text
ValleyBell/libvgm
61fc6725644886abc3168e240e4e51588d74bdf7
```

The relevant implementation is `emu/cores/qsound_ctr.c`.

The core keeps:

```text
16 PCM voices
+ 3 ADPCM voices
= 19 voice_output values
```

`state_normal_update()` updates those source values before the per-source dry/wet QSound pan tables, shared echo, FIR, delay and final stereo stages consume them.

That makes `voice_output[19]` the current causal source-audio boundary for the CTR implementation.

It is not a proof that 19 independent wet stems exist. They do not. The environmental path remains shared state.

## Native-rate law

The CTR core reports its sample rate as:

```text
QSound native rate = chip clock / 2 / 1248
```

libvgm then connects that source rate to `RESMPL_STATE`, whose job is to pull however many native core samples are needed to produce the requested destination/output frames.

Therefore:

```text
one QSound native sample
!=
one foobar/output sample
```

A source tap that labels `voice_output` directly with output-frame indices would be temporally wrong whenever source and destination rates differ.

## Implemented tap

`patches/libvgm/0006-qsound-native-source-observer.patch` adds a read-only observer to the CTR core and a small VGMPlayer forwarding API.

Each observed native frame carries:

- QSound chip instance;
- native sample rate;
- absolute native sample index;
- exactly 19 signed pre-pan source samples.

The callback is invoked after the historical core has computed the sample used by the normal QSound stereo path. The callback receives a borrowed pointer; GMI copies the values immediately and never mutates core state.

The observer only hooks `FCC_CTR_`. Selecting another QSound core does not silently reinterpret its internals as equivalent source audio.

## Project-owned receiver

`components/vgm/enhancement/qsound_native_source_capture.h` stores the frames that libvgm's resampler actually pulls during one destination block.

It is fixed-capacity and allocation-free in the callback path.

The capture fails closed on:

- wrong chip instance under the current single-QSound VGM model;
- zero or changing native sample rate;
- null source data;
- any source count other than 19;
- native-sample gaps or reordering inside a block;
- capacity overflow.

An empty block is not automatically invalid. A resampler may legitimately satisfy a small destination interval without pulling a new native frame.

## Live foobar boundary

The modified VGM wrapper now scopes the native observer to normal decode blocks:

```text
begin destination block
        ↓
attach CTR source observer
        ↓
reference libvgm decode
        ├─ historical QSound stereo -> p_chunk
        └─ native 19-lane frames -> bounded sidecar
        ↓
detach observer
        ↓
validate source timeline
```

The observer is detached on both normal completion and exception paths. Seek explicitly detaches it and clears the block capture, because discarded seek audio is not source material for the next audible block.

The returned foobar audio remains the historical libvgm render. The sidecar is evidence only at this stage.

## What this does not yet earn

The following are still frontier work:

1. shared QSound echo/FIR/delay return extraction as a separate causal environmental path;
2. one coherent shared-phase conversion from the native timeline to the consumer/output timeline for all 19 lanes;
3. reference recomposition tests where finite arithmetic and shared state permit an exact or bounded claim;
4. source-bus handoff to Omniphony;
5. any modern 3-D placement or binaural presentation;
6. audible substitution of QSound reference stereo.

In particular, do not instantiate 19 unrelated resamplers. All lanes came from one QSound engine and must remain phase/time coherent.

## Tests

The current deterministic ruler includes:

- `tests/vgm/qsound_native_source_capture_test.cpp`
  - exact copy of all 19 lanes;
  - native rate and absolute native index;
  - continuity;
  - malformed frame rejection;
  - overflow fail-closed behavior;
- `tests/vgm/test_qsound_native_source_patch.py`
  - protects the pinned CTR source boundary, native-rate law and historical stereo assignments;
- `tests/vgm/test_qsound_live_source_wiring.py`
  - protects callback copying, decode-block scope, detach behavior, seek exclusion and continued reference-audio ownership.

The dependency-free C++ capture ruler has been compiled locally with C++17 and warnings-as-errors. The manual GitHub `core-tests` workflow owns the new test, but GitHub-hosted execution remains unavailable while the repository/account runner is rejected before jobs begin.

## Current claim

The earned statement is now:

> GMI can observe the superctr QSound implementation's 19 pre-pan causal source samples at their native DSP rate during real VGM playback, preserve their exact native ordering in a bounded sidecar, and do so without replacing the historical QSound stereo render.

The stronger statement remains unearned:

> GMI has a complete output-rate QSound source bus ready for Omniphony.

That requires the shared-return and coherent-rate-conversion work above.
