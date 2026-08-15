# SNESAPU source-bus integration

## Upstream authority

Editable implementation reference:

- repository: `dgrfactory/spcplay`
- pinned commit: `fc770e268ecacb4523699e2edc5c0efdf80957d6`
- date: 2026-07-25
- relevant files:
  - `snesapu.dll/DSP.asm`
  - `snesapu.dll/DSP.h`
  - `snesapu.dll/DSP.inc`
  - `snesapu.dll/APU.asm`
  - `snesapu.dll/APU.h`
  - `snesapu.dll/SNESAPU.h`
  - `snesapu.dll/SNESAPU.def`
- license in the source headers: GNU GPL v2 or later

The previously supplied SPCPlay/SNESAPU binary remains a behavioral reference. This pinned source revision is the implementation authority for the source-bus patch.

## Exact dry-voice tap

`RunDSP` renders each active S-DSP voice through:

```text
interpolation / noise substitution
        -> envelope
        -> mOut
        -> VxVOLL / VxVOLR
        -> main accumulator
        -> optional echo-send accumulator
```

`Voice::mOut` is documented by upstream as the last sample output before channel volume and is also used by pitch modulation. The source-bus dry lane must therefore observe the floating-point value immediately after interpolation/noise plus envelope and before the signed channel-volume multiplication.

This location preserves:

- BRR/interpolation result;
- NON noise substitution;
- envelope behavior;
- source pitch trajectory;
- the exact pre-route signal consumed by S-DSP mixing.

It deliberately excludes:

- VxVOLL/VxVOLR;
- main volume;
- echo volume;
- echo FIR/feedback;
- renderer presentation.

The tap is observational only. It must not modify `mOut`, the FPU stack value used by the normal mixer, pitch modulation, source position, or envelope state.

## Exact shared-wet tap

S-DSP echo is one shared stereo feedback system. `MixEchoDSP`:

1. reads the shared echo history;
2. applies the FIR path;
3. obtains the filtered stereo return;
4. applies `EVOLL` / `EVOLR` when adding wet energy to the main output;
5. combines the filtered return with current echo sends for feedback.

The source bus must expose the filtered shared return without manufacturing per-voice wet stems.

GMI represents it as two linked mono lanes:

```text
shared echo field
    |- left return + signed EVOLL authority
    `- right return + signed EVOLR authority
```

Both lanes share one persistent environmental-field identity but retain independent source IDs and authored side gains. Omniphony may present the pair as one broad field; it may not relabel the field as authored 3-D geometry.

## Realtime source-tap ABI

The source tap is optional and disabled by default.

Desired shape:

```c
#define SNESAPU_SOURCE_VOICES 8

typedef struct SNESAPUSourceBlock {
    uint32_t struct_size;
    uint32_t sample_rate;
    uint32_t frame_count;
    const float *dry_voice[SNESAPU_SOURCE_VOICES];
    const float *echo_left;
    const float *echo_right;
} SNESAPUSourceBlock;

typedef void (__stdcall *SNESAPUSourceCallback)(
    const SNESAPUSourceBlock *block,
    void *user);

void __stdcall SetSNESAPUSourceCallback(
    SNESAPUSourceCallback callback,
    void *user);
```

The exact exported spelling may change during implementation, but the contract must retain these properties:

- no allocation in `RunDSP`;
- no lock in `RunDSP`;
- no per-voice or per-sample function call;
- at most one callback after a completed `RunDSP` chunk;
- source buffers are valid only for the callback duration unless the caller copies them;
- callback disabled means source capture is skipped;
- callback failure or missing Omniphony support cannot alter reference playback;
- seek/reset clears source-capture generation state along with decoder state.

`RunDSP` already processes bounded chunks up to `MIX_SIZE`, making one block notification the natural realtime boundary.

## Buffer layout

Prefer source-major contiguous float buffers because Omniphony consumes mono source lanes:

```text
dry[0][0..frames)
dry[1][0..frames)
...
dry[7][0..frames)
echo_left[0..frames)
echo_right[0..frames)
```

The capture storage can be fixed-size `MIX_SIZE` arrays owned by SNESAPU. No source-bus memory should be allocated by the hot loop.

## Reference invariants

With source capture disabled, output and internal state must be bit-identical to the pinned upstream build under the same compile options.

With source capture enabled:

- reference stereo output must remain unchanged;
- `mOut`, PMON, NON, KON/KOFF, envelope and BRR state must remain unchanged;
- FIR and feedback history must remain unchanged;
- capture must not introduce extra DSP samples or timing changes;
- source lane timestamps must correspond exactly to the reference output chunk.

## Recomposition boundary

Do not claim that summing the ten exposed lanes reconstructs final stereo by simple addition.

Reasons include:

- signed per-voice routing;
- main volume;
- shared echo feedback;
- echo volume;
- PMON cross-voice dependence;
- optional higher-precision/non-hardware-faithful rendering modes;
- later output filtering and finite arithmetic.

The protected reference path remains the control.

## Foobar consequence

The SNESAPU input's existing **Enable "surround" sound** checkbox should become the hard path selector:

```text
unchecked -> protected SNESAPU stereo/reference playback
checked   -> 8 dry voices + linked stereo echo field -> Omniphony
```

Additional shared presentation controls:

- `Externalization` checkbox;
- `Spatial depth: Native | Full`.

The existing SNESAPU interpolation, sample-rate, bit-depth and DSP-quality controls remain format-specific and should continue to describe the source renderer, not Omniphony presentation.
