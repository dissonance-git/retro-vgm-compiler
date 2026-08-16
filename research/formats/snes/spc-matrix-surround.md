# SNES matrix-surround routing evidence

Status: **implementation-relevant historical constraint**

## Question

When an SNES/SFC soundtrack deliberately routes a voice with opposite left/right polarity, should that be treated merely as an odd stereo-pan state, or can it carry authored surround intent that a modern source-aware renderer ought to preserve?

The safe answer is:

```text
opposite-polarity native routing
=
exact authored phase/opposition evidence

not automatically
=
proven discrete rear coordinate
```

For a source-aware renderer, that distinction is strong enough to matter.

## Device fact

The S-DSP voice-volume registers `VxVOLL` and `VxVOLR` are signed. The same is true for main and echo volume registers.

The editable SNESAPU authority used by this project preserves those signs and exposes the pre-volume voice signal separately from the effective L/R mixer coefficients.

Therefore the device can realize, for one causal source:

```text
L = +g * x(t)
R = -g * x(t)
```

without changing the source waveform itself.

This is not equivalent to ordinary centered stereo:

```text
L = +g * x(t)
R = +g * x(t)
```

The absolute route magnitudes are identical but the phase/polarity relation is different.

This is the concrete device case anticipated by `signed-routing-phase-separability.md`.

## Matrix-surround relevance

Dolby MP / Dolby Surround is a 4:2 matrix system. A conventional surround contribution is encoded into the two transmitted channels with opposite polarity, together with a relative phase relationship and additional surround-path processing.

A representative Dolby-assigned patent describes the conventional matrix as:

```text
Lt = L + 0.707 C + 0.707 jS'
Rt = R + 0.707 C - 0.707 jS'
```

where the surround contribution enters Lt/Rt with opposite polarity and `j` denotes the relative phase shift.

Primary technical reference:

- Dolby Laboratories Licensing Corp., `US5862228A`, *Audio matrix encoding*
  - https://patents.google.com/patent/US5862228

The exact historical Dolby encoder therefore contains more machinery than a bare `L=-R` assignment. Do **not** claim that every opposite-polarity S-DSP voice is a complete Dolby MP encode operation.

But the opposite-polarity relation is precisely the kind of directional cue a matrix decoder can use to recover surround/rear energy. It must not be discarded as a cosmetic sign bit.

## Historical leads

Two community references specifically document SNES games using matrix-surround playback and provide candidate titles for controlled corpus work:

- ConsoleMods SNES audio information:
  - https://consolemods.org/wiki/SNES:Audio_Information
- `r/miniSNESmods` discussion and candidate game list:
  - https://www.reddit.com/r/miniSNESmods/comments/hyajts/snes_dolby_surround/

These are **leads, not authority for individual games**. The list contains both officially Dolby-branded and community-identified titles, and the thread itself contains uncertainty about how unofficial cases were verified.

High-value initial positive-control candidates with reported official Dolby branding include:

- `Jurassic Park`
- `Super Turrican`
- `Super Turrican 2`
- `Fatal Fury Special`

Candidate negatives should include ordinary stereo titles whose S-DSP route trajectories do not sustain balanced opposite-polarity states.

Do not import ROMs or copyrighted game data into the repository. Use lawful local/user-provided SPC corpus material and store only derived evidence.

## Renderer consequence

The source-aware pipeline must preserve three distinct routing facts:

```text
magnitude
side
polarity relation
```

A pan scalar preserves only part of this information.

For one source with signed route `(L,R)`, define a rebuildable matrix-surround-compatible phase cue:

```text
if sign(L) == sign(R):
    cue = 0
else:
    cue = 1 - abs(|L|-|R|) / (|L|+|R|)
```

subject to finite/nonzero guards.

Interpretation:

```text
(+1,+1)  -> 0.0
(-1,-1)  -> 0.0
(-1,+1)  -> 1.0
(-1,+0.1)-> weak cue
```

This quantity is not an authored coordinate. It is an authored route relationship compatible with matrix-surround steering.

## Authority ordering

For Omniphony presentation:

```text
explicit authored 3-D position
    >
authored matrix-surround-compatible phase opposition
    >
authored ordinary stereo route
    >
inferred musical role / diffuse / width evidence
    >
identity-only stable separation
```

This ordering matters because an inferred `foundation` or `foreground` label must not silently drag an intentionally matrix-surrounded source back to the front.

Likewise, phase opposition must not create elevation. Historical matrix surround is horizontal/rear evidence, not height metadata.

## Current implementation

Omniphony commit `8c156dd0973ef01dec878def35c74145db1e0546` derives `matrix_surround_phase_cue()` directly from the existing signed `NativeStereoRoute`.

Behavior:

- equal balanced opposite polarity earns a strong broad rear presentation;
- same-sign routing earns no matrix rear weight;
- one-sided/lopsided opposite polarity is attenuated by route imbalance;
- the cue can outrank inferred foundation/foreground geometry;
- the cue adds no vertical displacement;
- authored position still passes through untouched.

No new ABI field was needed because signed route evidence was already preserved.

## Remaining closure issue: time variation

The current foobar SPC projection summarizes signed L/R route trajectory over a render block for pose evidence while preserving sample-exact gain trajectories for source energy.

That is sufficient when route-sign topology is stable across the block, which is expected for many deliberate matrix-surround uses.

It is **not** yet a proof for a block in which a voice changes between same-sign and opposite-sign routing internally.

The closure path is:

```text
sample-exact effective L/R coefficient planes
    -> detect route sign-topology transition
    -> split presentation slice at exact sample
    -> derive matrix-surround cue per slice
    -> render contiguous slices
```

This is the SPC analogue of the VGM exact route-change segmentation rule.

## Corpus experiment

For every admitted SPC track, record per physical voice and later per persistent part:

```text
active duration
same-sign duration
opposite-sign duration
balanced-opposition duration
phase-cue distribution
route transition count
EON state
shared-echo state
driver family where known
work/title provenance
```

Then compare:

1. officially surround-branded positive controls;
2. community-reported unofficial surround titles;
3. ordinary stereo negatives.

Useful questions:

- Do licensed titles show sustained balanced opposition more often than controls?
- Is the cue concentrated in effects, ambience, music, or all three?
- Does a driver expose a specific surround command that maps to signed route writes?
- Do opposite-polarity states correlate with echo-send or FIR choices?
- Does the same logical musical part preserve the matrix cue when it migrates between S-DSP voice slots?

## Claim boundary

The project may claim:

> SNES S-DSP signed routing preserves phase/opposition information that can be intentionally relevant to matrix-surround reproduction, and source-aware rendering should preserve and exploit that evidence conservatively.

The project may **not** infer from one negative route sign alone:

- Dolby licensing;
- a complete Dolby MP encoding chain;
- a specific rear loudspeaker coordinate;
- composer intent;
- height information.

Those require independent work/driver/documentation evidence.

Correction outranks coherence.
