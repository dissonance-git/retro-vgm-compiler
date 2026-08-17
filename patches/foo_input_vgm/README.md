# foo_input_vgm shell patches

The foobar shell keeps synthesis enhancement and Omniphony presentation as two independent user choices.

Apply the complete shell set with:

```text
python patches/foo_input_vgm/apply_enhanced_component.py <foo_input_vgm-src>
```

Before building the component, its libvgm checkout must also receive:

```text
python patches/libvgm/apply_source_capture.py <libvgm-root>
```

## Audible Enhanced paths

### YM2612 FM: six channels stay six channels

The normal Enhanced FM path is now an **exact-state lift**, not a MIDI conversion and not a modern preset substitution.

```text
one authoritative Nuked OPN2 state
        ↓
exact register/write timing
exact pitch + detune + operator ratios
exact LFO / PM / AM
exact CH3 special mode / CSM
exact SSG-EG + key/envelope state
exact OPN modulation history
exact authored pan
        ↓
continuous carrier sine reconstruction
floating carrier accumulation
no 9-bit channel clamp
no YM2612 FM sign-leak / DAC-ladder artifact
no optional MD1 output low-pass
        ↓
six HQ FM source lanes
```

Those lanes currently traverse the same outer libvgm `RSMODE_LINEAR` timing and device-volume coordinate as the six exact reference FM lanes. PlayerA can therefore perform the source-native replacement directly:

```text
protected reference mix
- exact FM1..FM6
+ HQ-lift FM1..FM6
```

DAC is a separate seventh YM2612 source identity and is not subtracted by the FM replacement. When DAC owns channel 6's hardware bus slot, the exact FM6 and HQ FM6 source contributions are both silent.

This first automatic FM rung deliberately keeps the original quantized OPN modulation history as its teacher. That preserves difficult semantics while improving the final carrier/channel/output ceiling. The separate `ym2612_hq_fm_backend` explores a deeper all-floating OPN descendant, but it is not required for this safer automatic path.

### The next FM ceiling: source-rate conversion

libvgm's `RSMODE_LINEAR` is a useful exact timing control, but linear interpolation/box-like downsampling is not the intended quality ceiling for a studio-grade Enhanced source.

The repository now contains:

```text
components/vgm/foo_input_vgm/src/studio_source_resampler.h
```

This is an Enhanced-only 64-tap Kaiser-windowed polyphase FIR kernel. It is rate-aware: when the destination rate is lower than the source rate, the kernel lowers its cutoff before the destination Nyquist boundary instead of letting high-frequency source energy alias into the output. Coefficients are prepared outside the realtime callback; reconstruction itself is a bounded dot product.

It is **not yet substituted into the audible PlayerA FM path**. A symmetric 64-tap FIR needs 31 source samples of history and 32 of lookahead. Applying it to FM alone without compensating the whole Enhanced candidate would shift FM relative to DAC, PSG and untouched chips. That would violate musical timing to improve frequency response, which is not an acceptable trade.

The integration obligation is therefore explicit:

```text
capture exact source-rate HQ FM
        ↓
bandlimited FIR SRC
        +
known FIR latency
        ↓
delay/align the whole Enhanced candidate by the same amount
        ↓
subtract aligned exact FM
+ add aligned HQ FM
        ↓
verify FM/DAC/PSG transient and phase relationships
```

Until that alignment is implemented and tested, the audible path retains the current linear timing bridge. The new FIR kernel is executable/tested infrastructure for the next rung, not a claim that the audible foobar DLL has already crossed it.

### SN76489/96 PSG

The primary default-MAME SN76496/SN76489 path uses its independently synthesized descendant:

```text
ordinary VGMPlayer render
        +
exact four PSG reference source contributions
        +
exact command timing
        ↓
source-aware PlayerA pre-volume seam
        ↓
render higher-quality PSG descendants from the same timed writes
        ↓
reference mix
- exact historical PSG sources
+ enhanced PSG sources
```

## Transaction and fallback policy

Enhancement is transactional per source family. FM can succeed while PSG remains reference, or vice versa. Within a family, incomplete capture, source-timing failure, missing source lanes, non-finite arithmetic, or output overflow keeps that entire family on the protected reference for the block.

Unrelated VGM chips remain untouched. There is no requirement that every device in a mixed-chip VGM have an Enhanced renderer before already-proven source families can improve.

## Four modes

```text
Enhanced OFF + Spatial OFF -> protected reference stereo
Enhanced OFF + Spatial ON  -> source-aware Omniphony presentation
Enhanced ON  + Spatial OFF -> admitted source replacements in stereo
Enhanced ON  + Spatial ON  -> same enhanced sources through Omniphony
```

The old VGM output-resampling and chip-sample-rate controls remain separate from `Enhanced`.
