# foo_input_vgm shell patches

The foobar shell keeps synthesis enhancement and Omniphony presentation as two independent user choices.

## Naming contract

The **enhanced** option is the one source-native quality-improvement option. `enhanced` is descriptive, not a proper name, brand, product identity, or quality tier. The protected **reference** path is the accuracy/control renderer, likewise not a brand name.

Some recently introduced implementation files and symbols still carry a `studio_*` prefix from development. Treat that prefix as legacy internal naming only. It does **not** define another user-facing mode or quality level, and new work should use `enhanced_*` or a neutral algorithmic name instead.

Apply the complete shell set with:

```text
python patches/foo_input_vgm/apply_enhanced_component.py <foo_input_vgm-src>
```

Before building the component, its libvgm checkout must also receive:

```text
python patches/libvgm/apply_source_capture.py <libvgm-root>
```

## Audible enhanced paths

### YM2612 FM: six channels stay six channels

The normal enhanced FM path is now an **exact-state lift**, not a MIDI conversion and not a modern preset substitution.

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

Those lanes originate from the same live Nuked OPN2 state and authoritative source timing as the six exact reference FM lanes. The enhanced deferred reconstruction path can therefore perform the source-native replacement without creating a second musical timeline:

```text
protected reference mix
- exact FM1..FM6
+ HQ-lift FM1..FM6
```

DAC is a separate seventh YM2612 source identity and is not subtracted by the FM replacement. When DAC owns channel 6's hardware bus slot, the exact FM6 and HQ FM6 source contributions are both silent.

This first automatic FM rung deliberately keeps the original quantized OPN modulation history as its teacher. That preserves difficult semantics while improving the final carrier/channel/output ceiling. The separate `ym2612_hq_fm_backend` explores a deeper all-floating OPN descendant, but it is not required for this safer automatic path.

### Enhanced FM source-rate reconstruction

libvgm's `RSMODE_LINEAR` remains the useful exact timing control, but linear interpolation/box-like downsampling is not the intended quality ceiling for the enhanced source.

The repository currently contains the 64-tap Kaiser-windowed polyphase FIR implementation under the legacy internal filename:

```text
components/vgm/foo_input_vgm/src/studio_source_resampler.h
```

This is an **enhanced-only** reconstruction primitive, not a separate mode. It is rate-aware: when the destination rate is lower than the source rate, the kernel lowers its cutoff before the destination Nyquist boundary instead of letting high-frequency source energy alias into the output. Coefficients are prepared outside the realtime callback; reconstruction itself is a bounded dot product.

A symmetric 64-tap FIR needs 31 source samples of history and 32 of lookahead. The enhanced integration therefore treats that latency as explicit scheduling state rather than shifting FM relative to DAC, PSG, or untouched chips. Whole protected frames retain their authoritative output ordinals until the matching enhanced FM reconstruction is available; unsupported or unavailable regions fall back to reference.

A true playback start and a seek are intentionally different. At initial chip attachment, the reset state proves that negative-time FM is silent, so the FIR may use that known-zero prefix and own destination frame 0 once its real future support has arrived. A seek or later reset does **not** inherit that permission: pre-seek history is musical material, so the observer falls back to reference until enough true post-discontinuity history exists. No hidden zero padding crosses a seek.

The protected frame is itself compositional. When the engine-clock PSG descendant is admitted, its exact four-channel replacement is committed to that frame **before** the deferred FM exchange. Thus render-ahead does not force a choice between better FM and better PSG:

```text
reference frame
- exact PSG + enhanced PSG
        ↓
PSG-enhanced protected frame
- exact FM + enhanced FM
        ↓
final enhanced frame
```

DAC and unrelated chips remain untouched through both exchanges. If PSG loses exact source authority, only PSG falls back to its protected reference contribution; valid FM reconstruction remains eligible. If FM loses reconstruction authority, an already-valid PSG-enhanced protected frame can still survive.

The invariant is:

```text
capture exact source-rate HQ FM
        ↓
bandlimited FIR reconstruction + explicit lookahead
        +
whole protected frame at the same absolute output ordinal
        ↓
release only when the matching enhanced FM frame is reconstructable
        ↓
protected frame - exact FM + enhanced FM
```

Seek/session discontinuities start a fresh native-history epoch while output ordinals remain authoritative. This prevents FIR history from crossing a discontinuity without inventing a second playback clock.

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

During ordinary non-deferred playback, the established block renderer consumes the captured timed-write block. When enhanced FM needs PlayerA render-ahead, that host-block clock is no longer authoritative for PSG. The runtime therefore seeds a private PSG descendant from the continuous shadow, advances it between writes on **absolute engine-sample ordinals**, and stores one bounded replacement contribution per rendered frame. The resulting PSG-enhanced protected frame is then handed to the deferred FM transport.

This is a timing representation change, not a second PSG synthesizer design. A regression feeds the same timed write stream through both the established block renderer and the engine-ordinal queue and requires sample-for-sample left/right agreement under the same libvgm source-volume scaling.

## Transaction and fallback policy

Enhancement is transactional per source family. FM can succeed while PSG remains reference, or vice versa. Within a family, incomplete capture, source-timing failure, missing source lanes, non-finite arithmetic, output overflow, or ordinal discontinuity keeps that family on the protected reference.

Dynamic family admission is deliberately independent. The deferred FM path checks current YM/FM evidence rather than the all-family `source_block_complete()` convenience predicate, while deferred PSG requires its own exact four-lane source block. One family's current block failure cannot demote another family's otherwise-valid candidate.

Unrelated VGM chips remain untouched. There is no requirement that every device in a mixed-chip VGM have an enhanced renderer before already-proven source families can improve.

## Four combinations

```text
enhanced off + spatial off -> protected reference stereo
enhanced off + spatial on  -> source-aware Omniphony presentation
enhanced on  + spatial off -> admitted source replacements in stereo
enhanced on  + spatial on  -> same enhanced sources through Omniphony
```

The old VGM output-resampling and chip-sample-rate controls remain separate from the enhanced option.
