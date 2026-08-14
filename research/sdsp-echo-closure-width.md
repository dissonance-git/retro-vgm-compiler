# S-DSP echo feedback has topology-dependent causal closure width

Status: **exact symbolic closure control + work-observed negative control + driver-level positive controls**

This note transfers one bounded Helix result into S-DSP audio execution:

```text
small direct change
!= necessarily small semantic closure
```

but keeps the complementary control that Helix also required:

```text
closure amplification is not automatic
```

For S-DSP echo, the deciding structure is the FIR relation inside the feedback
loop.

## 1. Device mechanism

Independent S-DSP implementations agree on the causal path.

Pinned observatories:

- `snes9xgit/snes9x` commit
  `cf95f09a3799c1e58b539109a4afee6c5f710c9c`,
  `apu/bapu/dsp/SPC_DSP.cpp`
- Mednafen/bsnes lineage as vendored by Provenance commit
  `4634bfd35b425d3be5873aef0bba71d85719bc09`,
  `Cores/Mednafen/Sources/mednafen/mednafen/src/snes/src/sdsp/echo.cpp`

A voice first produces its post-envelope sample. Signed per-voice L/R gain is
then applied. If the voice is selected by EON, that same signed contribution is
added to the shared echo-write accumulator as well as the main mix.

The echo subsystem then:

```text
reads delayed echo RAM
-> updates eight-sample history
-> applies the eight FIR coefficients
-> mixes wet output
-> adds scaled FIR result to new echo-send input through EFB
-> writes the result back to the circular echo buffer
```

Therefore a one-sample routing intervention can alter state that survives after
the originating voice event ends.

This was already enough to reject naive independent wet-stem reconstruction.
The new question here is how *wide* the future causal support can become.

## 2. Exact symbolic support control

Represent the eight FIR coefficients as the integer polynomial

```text
F(x) = c0 + c1 x + ... + c7 x^7.
```

A one-position perturbation has support `{0}`.

Ignoring only later fixed-point scaling/clipping/quantization while preserving
the exact integer FIR coefficients, the causal-path coefficient polynomial at
feedback generation `k` is:

```text
F(x)^k.
```

`tools/sdsp_echo_closure_control.py` computes those powers using exact integer
convolution and counts their nonzero coefficients.

This is not an audible-waveform approximation. It is an exact symbolic control
for the linear FIR path relation before later device arithmetic can cancel or
prune contributions.

For a full eight-tap support with no polynomial cancellation, the maximum span
at generation `k` is:

```text
7k + 1 positions.
```

## 3. Real-work negative control: Gun Hazard / Naval Fortress

The committed corpus fixture

```text
tests/corpus/front-mission-gun-hazard/1.33 - Naval Fortress.spc
```

contains the following physical DSP state at its saved snapshot:

```text
EFB  = 43
EON  = 0x6f
EDL  = 7
FIR  = [126,0,0,0,0,0,0,0]
```

With nominal S-DSP output at 32 kHz:

```text
EDL 7
-> 3584 stereo sample frames
-> approximately 112 ms echo-buffer period
```

This is an especially useful adversary because feedback is genuinely enabled
and several voices are admitted to the echo send, yet the FIR relation is
single-tap.

Exact symbolic feedback support remains:

```text
generation             1 2 3 4 5 6 7
nonzero support count  1 1 1 1 1 1 1
```

So:

```text
feedback present
!= support-width amplification
```

A perturbation may persist and recur, but this observed FIR topology does not
branch one temporal position into many symbolic paths.

The eventual numerical tail still depends on EFB scaling, fixed-point
rounding, saturation, the baseline signal, and when quantization drives the
residual to zero. This control does not replace a waveform-level replay.

## 4. N-SPC driver-level positive controls

The pinned A Link to the Past N-SPC driver contains four FIR coefficient
presets:

```text
preset 0  [127,  0,  0,  0,  0,  0,  0,  0]
preset 1  [ 88,-65,-37,-16, -2,  7, 12, 12]
preset 2  [ 12, 33, 43, 43, 19, -2,-13, -7]
preset 3  [ 52, 51,  0,-39,-27,  1, -4,-21]
```

Pinned source:

- `loveemu/vgm-disasm`
- commit `e96c5b35649f8e814cac3c31b65cedc07b52d76d`
- `snes/NSPC/Nintendo/Koji Kondo/Legend of Zelda - A Link to the Past.s`

Exact polynomial support counts are:

```text
generation     1  2  3  4  5  6  7

preset 0       1  1  1  1  1  1  1
preset 1       8 15 22 29 36 43 50
preset 2       8 15 22 29 36 43 50
preset 3       7 15 22 29 36 43 50
```

Presets 1 and 2 attain the full `7k+1` symbolic support immediately. Preset 3
has one zero tap initially, but its second convolution fills the missing
position and thereafter also reaches the full span.

This is a driver-level reachable-structure result, not evidence that every
A Link to the Past cue uses all four presets.

## 5. Helix transfer

The paired controls give a concrete audio realization of Helix's closure
accounting distinction:

```text
DIRECT CHANGE
one routed sample / one route-sign intervention

RELATION STRUCTURE
single-tap or multi-tap FIR

SEMANTIC CLOSURE BURDEN
how many future feedback-path positions can depend on that intervention
```

For the same one-position perturbation:

```text
single-tap FIR
-> one symbolic path per feedback generation

multi-tap FIR
-> expanding causal support
-> up to 7k+1 positions at generation k for the tested full-support presets
```

Thus:

```text
intervention size
!= causal closure width
```

and:

```text
feedback existence
!= closure amplification
```

The relation topology is load-bearing.

This is standard FIR/convolution mathematics applied as a game-audio execution
control. No new DSP theorem is claimed.

## 6. Literature context

Artificial-reverberation literature independently treats echo density and its
growth as structural properties of recursive delay/filter networks rather than
as a binary consequence of "reverb on."

Useful controls include:

- Sebastian J. Schlecht and Emanuel A. P. Habets,
  "Feedback Delay Networks: Echo Density and Mixing Time,"
  *IEEE/ACM Transactions on Audio, Speech, and Language Processing*, 2017,
  DOI `10.1109/TASLP.2016.2635027`.
- Sebastian J. Schlecht and Emanuel A. P. Habets,
  "Dense Reverberation with Delay Feedback Matrices," WASPAA 2019,
  DOI `10.1109/WASPAA.2019.8937284`.
- Sebastian J. Schlecht and Emanuel A. P. Habets,
  "Scattering in Feedback Delay Networks,"
  *IEEE/ACM Transactions on Audio, Speech, and Language Processing*, 2020,
  DOI `10.1109/TASLP.2020.3001395`.

Those papers concern much more general feedback-delay networks than S-DSP.
Their role here is conceptual calibration: recursive-filter topology controls
how impulse responses become dense or diffuse over time.

They do not establish the exact finite S-DSP support counts above; those come
from the pinned FIR coefficients and exact polynomial calculation.

## 7. Human evidence: echo was a musical and resource decision

Barry Leitch's retrospective on Top Gear is unusually direct historical
context for the S-DSP echo coordinate. He describes discovering the SNES's
real-time hardware echo as a striking platform-specific capability, choosing
to exploit it, and setting Top Gear's echo to exactly a sixteenth-note because
longer delay consumed more SNES audio memory than the project could afford.

Source:

- GameDeveloper interview,
  "Interviewing veteran composer Barry Leitch (Part I). Sound chips (from
  ZX-81 to the SNES)."
  `https://www.gamedeveloper.com/audio/interviewing-veteran-composer-barry-leitch-part-i-sound-chips-from-zx-81-to-the-snes-`

This gives a creator-side relationship:

```text
hardware effect topology
+ tempo / musical subdivision
+ memory budget
-> chosen echo realization
```

That is much closer to how the interpreter should eventually explain an echo
than simply reporting `EDL=...`.

Yasunori Mitsuda independently describes Square's strong SNES sound as the
product of sound-programming/manipulation work by Minoru Akao and himself, and
says they created a variety of techniques to improve game sound. In another
interview he distinguishes composer-controlled layering/timbre choices from
later tonal processing such as reverb/effects/EQ.

Sources:

- VGMO / Square Enix Music Online Mitsuda interview, 2005
- Shmuplations, "Yasunori Mitsuda – 2003 Composer Interview"

These statements support treating the sound-programming and effect-realization
layers as historically meaningful work, while also warning against assigning a
specific low-level DSP choice to a composer without direct provenance.

## 8. Interpreter consequence

A useful future description should be able to distinguish statements like:

```text
"this cue uses echo"
```

from the more informative:

```text
"the driver sends these voices into a ~112 ms feedback loop whose saved FIR
state is essentially single-tap, so the effect repeats without the temporal
smearing available from the driver's broader multi-tap filters"
```

and then, only with sufficient human/perceptual evidence, translate that into
musician/listener language such as:

```text
short slap-like repeat
broader diffuse tail
space / depth
rhythmic echo
```

The exact device state licenses the mechanism. The human and psychoacoustic
layers license the musical words.

## Executable controls

- `tools/sdsp_echo_closure_control.py`
- `tests/spc/test_sdsp_echo_closure_control.py`

The SPC test suite is already included in the manual `core-tests` workflow.
Hosted execution remains blocked by the account spending limit; synthetic
calculations in this research pass were replayed locally, while corpus-level CI
claims remain pending until runners execute them.

## Next falsifiers

1. Add dynamic signed-route and echo-send observations to the SPC runtime
   capture boundary so one actual route-sign intervention can be followed into
   echo RAM and back out.
2. Find or authorize a preserved N-SPC work that actually selects one of the
   multi-tap FIR presets and compare symbolic closure with exact waveform
   difference propagation.
3. Separate causal support width from audible echo density; small coefficients
   may create mathematically nonzero paths below perceptual relevance.
4. Measure how S-DSP clipping/quantization can collapse or merge causal paths
   that remain distinct in the ideal integer polynomial.
5. Keep a single-tap control in every future closure experiment so "feedback"
   itself is never mistaken for the cause of diffusion.

## Claim boundary

Established here:

- exact work-observed Gun Hazard Naval Fortress echo registers at the saved SPC
  state;
- exact integer-polynomial support of that FIR and the pinned N-SPC FIR table;
- the single-tap versus multi-tap closure-width separation;
- independent implementation agreement on S-DSP echo's shared FIR/feedback
  causal structure;
- historical evidence that SNES echo could be an intentional musical/time and
  memory-budget decision.

Not established here:

- exact audible support after every S-DSP fixed-point operation;
- that A Link to the Past uses every listed FIR preset in retained music;
- that a larger symbolic support necessarily sounds wider or more reverberant;
- that the quoted creators personally chose the exact low-level states in the
  controls above;
- any novel theorem about feedback networks.
