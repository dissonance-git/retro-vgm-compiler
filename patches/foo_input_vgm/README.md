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

## First audible Enhanced path

The first admitted VGM replacement is the primary default-MAME SN76496/SN76489 device:

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
        ↓
PlayerA's unchanged volume/fade/output path
```

The operation is transactional. If source accounting, capture completeness, sample alignment, finite arithmetic, or renderer support fails, that whole block remains protected reference audio.

YM2612 FM, DAC and unrelated chips are not changed merely because `Enhanced` is checked. They remain reference until their own exact subtraction + validated replacement path exists.

## Four modes

```text
Enhanced OFF + Spatial OFF -> protected reference stereo
Enhanced OFF + Spatial ON  -> source-aware Omniphony presentation
Enhanced ON  + Spatial OFF -> admitted source replacements in stereo
Enhanced ON  + Spatial ON  -> same enhanced sources through Omniphony
```

The old VGM output resampling and chip-sample-rate controls remain separate from `Enhanced`.
