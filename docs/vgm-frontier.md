# VGM enhancement frontier

This document records the current engineering frontier. It is intentionally about what exists, not what is hoped for.

## Audible status

**foobar2000 playback is still the unmodified libvgm reference render.**

The enhanced renderers below currently run as source-state/shadow infrastructure or dependency-free testable cores. None should be described as an audible improvement until a controlled substitution build has been listened to and retained.

That is deliberate. Accuracy is the control; enhanced rendering must earn the right to replace each source family separately.

## Live source observation

The repo carries a libvgm patch series that exposes realtime playback state without reverse-compiling a VGM into a score:

1. `0001-realtime-command-observer.patch`
   - command + reset events in normal render and seek replay
2. `0002-resolved-ym2612-dac-observer.patch`
   - actual bytes consumed by legacy `0x80..0x8F` YM2612 DAC playback
3. `0003-dac-stream-source-observer.patch`
   - source-level modern DAC stream state after libvgm resolves bank/destination/rate/start/length
4. `0004-refresh-dac-stream-pcm-bank.patch`
   - refresh stream pointers if an appended PCM block reallocates a bank
5. `0005-fix-dac-stream-millisecond-length.patch`
   - convert millisecond stream lengths using `frequency * ms / 1000`

The wrapper maps those events onto the same output-sample timeline used by libvgm playback.

## Genesis source truth

`genesis_state` currently tracks two YM2612 instances and two SN76489-family instances.

YM2612 state includes:

- six channels
- exact register cache
- key/operator mask
- F-number/block with real Yamaha high-byte latch semantics
- algorithm/feedback
- authored L/R routing
- AMS/FMS
- LFO
- channel-3 special mode/CSM frequencies
- all four operator parameter sets: DT/MUL/TL/KS/AR/AM/DR/SR/SL/RR/SSG-EG
- DAC enable and resolved DAC source activity

PSG state includes tone periods, attenuation, noise state/control and stereo mask.

No instrument-name, importance, width, height or other semantic inference is stored as source truth.

## Enhanced source engines implemented

### SN76489-family PSG

`sn76489_enhanced`

- four isolated mono stems: three tone + noise
- floating-point 2 dB attenuation ladder
- oversampled PolyBLEP square reconstruction
- source-faithful noise LFSR
- sample-accurate timed writes inside a foobar block
- fast no-output state advance for seeks/shadow playback
- real device clock divider, feedback taps, LFSR width, Sega zero-period behavior and output polarity
- explicit fallback for unvalidated NCR/T6W28 variants

An objective regression test compares off-harmonic alias energy against a naive high-pitch square wave.

### Classic YM2612 DAC

`ym2612_dac_enhanced`

- isolated floating-point PCM stem
- exact direct `$2A/$2B` state
- exact resolved legacy VGM DAC bytes
- `0x80` is the source zero point
- interpolation only between PCM points that actually exist
- hard authored DAC enable/disable boundaries
- null-output shadow advancement

### Modern VGM source-bank PCM to YM2612 DAC

`ym2612_pcm_stream`

- consumes the original libvgm PCM bank directly
- source write frequency is authoritative
- step/base interleaving preserved
- start/length semantics preserved
- reverse and loop preserved
- windowed-sinc/Lanczos reconstruction over the original source bytes
- no need to infer sample timing from final stereo or post-DAC output

All 256 VGM stream IDs remain logically separate in the wrapper shadow state.

## YM2612 FM frontier

The complete YM2612 register timeline can now be captured allocation-free with exact block sample offsets.

`ym2612_fm_backend` defines the synthesis boundary:

- exact register writes in
- six isolated FM channel stems out
- reset + null-output state advancement required

`ym2612_fm_timeline` applies writes at exact output-sample boundaries and is backend-agnostic.

The intended first FM backend is a mature Yamaha synthesis engine with channel output exposed **before final stereo summation**. `ymfm` is the leading architecture under investigation because it separates engine/channel/operator logic cleanly. Do not replace this with a simplistic four-sine approximation merely to make sound sooner.

The first FM milestone is not yet “remove YM2612 character.” It is:

> same patch + same automation + mature synthesis semantics + six isolated source stems

Only after that is verified should selected hardware restrictions be removed or reinterpreted experimentally.

## Mixing boundary

`source_stem_mixer` performs only explicit linear source routing with double-precision accumulation.

It does **not**:

- compress
- limit
- normalize
- widen
- infer source roles
- invent spatial positions

`ym2612_authored_route()` preserves the chip's original L/R enable decisions for the first enhanced listening baseline.

A later QSound-informed spatial layer may expand source extent before final summation, but synthesis quality and spatial reinterpretation should not be changed in the same first listening experiment.

## Validation without Codex or foobar SDK

`tools/run_core_tests.py`

- discovers every `tests/vgm/*_test.cpp`
- compiles against the complete dependency-free enhancement core
- C++17
- optimization enabled
- warnings as errors
- runs every produced executable

`tools/check_libvgm_patches.py <libvgm-checkout>`

- requires pinned libvgm commit `61fc6725644886abc3168e240e4e51588d74bdf7`
- creates a detached worktree
- checks and applies the full patch series in order

GitHub-hosted Actions are currently manual-only because the account runner is rejected before execution by the platform billing/spending-limit state. Do not interpret the resulting absence of CI as a code failure or a code pass.

## Next audible sequence

Do not enable every enhancement at once.

1. finish/validate live YM2612 FM timeline capture in the wrapper
2. add a mature six-stem FM backend
3. build source-family substitution behind a reversible experimental mode
4. first A/B: improved source realization while preserving authored stereo routing
5. retain only source families that beat the reference in listening tests
6. establish fixed headroom after source mixing, without limiter/compressor/AGC
7. then add source-domain spatial expansion informed by QSound and test again
8. Omniphony receives the resulting modern source-aware stereo master and remains responsible for the full headphone sphere

The product target is not a cleaner emulator. It is the highest-quality plausible realtime realization of the musical information still encoded in the source.
