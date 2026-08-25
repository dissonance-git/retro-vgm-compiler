# PlayStation SPU voice semantics

## Question

What physical voice-state semantics can VGM Compiler safely claim for the original PlayStation SPU before any AKAO/driver event is mapped to hardware?

This pass triangulates independent emulator, FPGA, hardware-oriented documentation, and game-specific reverse-engineering evidence. It deliberately keeps driver-level AKAO semantics separate from physical SPU semantics.

## Evidence matrix

Pinned implementation observatories:

- DuckStation `stenzek/duckstation` `5fd366809053fe287291de7a39752c4d5d5b146b`
- Beetle/Mednafen lineage `libretro/beetle-psx-libretro` `12bdb844261f3d8f43d470f569eb3ace0dcad049`
- MiSTer PSX FPGA core `MiSTer-devel/PSX_MiSTer` `0a9338405cee97d8a416af0a8a8879ff89784975`

Hardware-oriented documentation:

- PSX-SPX PlayStation SPU documentation

Game/driver observatories:

- VGMTrans AKAO parser/history
- ValleyBell `MidiConverters/AKAO2MID.bas`
- permanent *Chrono Cross* PSF1 corpus

The academic literature is useful for general dynamic voice-allocation concepts but is sparse on exact PS1 SPU PMON arithmetic. Exact chip claims therefore weight the device-oriented sources above generic papers.

## Physical resources

Independent implementations agree that the original PlayStation SPU exposes 24 physical voices.

A physical voice carries or participates in state including:

- sample/noise source mode;
- ADPCM decode address/history;
- programmed pitch (`VxPitch`);
- interpolation/sample phase;
- ADSR parameters and live ADSR state;
- left/right volume or autonomous sweep state;
- key-on/key-off lifecycle;
- reverb participation;
- optional pitch-modulation relationship to the immediately preceding physical voice.

Therefore:

```text
physical SPU voice number
!= sample identity
!= note identity
!= logical sequence track
!= persistent musical part
```

A voice number is a physical resource coordinate whose source, envelope, routing, and causal relationships evolve over time.

## Source mode can change without changing the physical slot

DuckStation and Beetle/Mednafen both model a per-voice noise-mode bit. When enabled, the voice uses the shared SPU noise generator instead of ordinary interpolated ADPCM sample output.

Thus:

```text
voice slot
!= permanently sampled voice
```

The active source class is time-bearing physical state.

## ADSR and stereo volume are evolved state

Both emulator lineages model ADSR as a live state machine rather than a static parameter block. Left/right voice volume can also be sweep-controlled rather than a fixed scalar.

Therefore:

```text
ADSR register values != ADSR trajectory
volume register values != instantaneous left/right gain trajectory
```

A register snapshot is below the performed physical voice trajectory.

## PMON creates an inter-voice causal graph

Pitch modulation is not an isolated per-voice parameter.

For physical voice `n > 0`, PMON can derive voice `n`'s effective sample step from the immediately preceding physical voice `n-1`.

The strongest cross-implementation agreement is on the signal stage used as the modulator:

```text
voice[n-1] sample source
    -> ADPCM interpolation OR noise substitution
    -> ADSR
    -> mono pre-L/R voice signal
    -> PMON factor for voice[n]
```

Only after this are voice `n-1`'s left/right volume/sweep gains applied for stereo output.

DuckStation stores this signal as `last_volume`. Beetle/Mednafen stores the corresponding signal as `PreLRSample`. MiSTer computes `sample * adsrVolume` and stores the result as `voice_lastVolume` before using it in the next voice's pitch calculation.

Therefore the PMON source is not:

```text
the previous voice's final audible stereo output
```

It is a much more specific coordinate:

```text
previous physical voice's post-source/post-ADSR, pre-left/right-gain mono signal
```

This distinction matters for interpretation and future enhancement. A renderer that derives PMON from a stem or final panned voice would already be in the wrong representation.

## Common signed-carrier PMON candidate

DuckStation, Beetle/Mednafen, and PSX-SPX agree on a high-bit-sensitive PMON model equivalent to:

```text
factor = previous_pre_lr_sample + 0x8000
raw_step = (signed16(VxPitch) * factor) >> 15
raw_step = wrap16(raw_step)
```

Beetle/Mednafen writes the same arithmetic in an algebraically different form:

```text
VxPitch + ((signed16(VxPitch) * previous_pre_lr_sample) >> 15)
```

Modulo 16-bit wrap these expressions are equivalent because the `+0x8000` contribution contributes exactly `signed16(VxPitch)`.

This model preserves the documented high-bit PMON glitch for `VxPitch >= 0x8000`.

## MiSTer high-bit arithmetic disagreement

The pinned MiSTer FPGA RTL agrees on the PMON source signal stage but multiplies:

```text
unsigned(VxPitch) * unsigned(voice_lastVolume + 0x8000)
```

before shifting.

For ordinary `VxPitch < 0x8000`, signed and unsigned carrier interpretations agree.

For high-bit VxPitch values they can diverge sharply.

Example candidate boundary:

```text
VxPitch = 0x8000
previous pre-L/R sample = 0x7FFF

signed-carrier candidate -> raw step near 0x0001
MiSTer unsigned candidate -> raw step near 0xFFFF
```

After the maximum-step clamp these can still produce radically different physical trajectories.

No MiSTer issue or later commit located in this pass documents the unsigned arithmetic as a deliberate hardware measurement. The feature entered in January 2022 in a broader SPU implementation pass adding envelope, noise, previous-channel pitch modulation, and ADPCM fixes.

Therefore:

```text
MiSTer unsigned high-bit PMON
```

is retained as a competing implementation hypothesis, not promoted to hardware truth and not silently discarded.

## Maximum-step boundary disagreement

Three implementation lineages currently agree on a `0x3FFF` maximum effective step:

- DuckStation
- Beetle/Mednafen
- MiSTer FPGA

The MiSTer RTL explicitly advances the ADPCM position by `step` only when `step < 0x3FFF`; otherwise it advances by `0x3FFF`.

PSX-SPX instead documents an over-range step as becoming `0x4000`.

This one-LSB disagreement is small as a normal pitch difference but scientifically useful because it is a precise hardware discriminator.

VGM Compiler therefore preserves both candidates:

```text
implementation candidate: 0x3FFF
hardware-documentation candidate: 0x4000
```

until stronger direct hardware evidence resolves the boundary.

Do not average them, choose one by majority vote, or hide the discrepancy behind floating-point Hz.

## Executable representation

`components/psf/spu_pitch.py` now exposes:

- the 24-voice physical-resource count;
- unity pitch coordinate `0x1000`;
- the signed-carrier PMON raw-step candidate;
- the MiSTer unsigned-carrier PMON raw-step candidate;
- the implementation `0x3FFF` clamp candidate;
- the PSX-SPX `0x4000` clamp candidate;
- explicit arithmetic/clamp disagreement flags.

This helper is intentionally below AKAO and musical pitch.

```text
AKAO note/tuning
!= VxPitch
!= raw PMON step
!= clamped effective step
!= heard pitch
```

Every conversion between these layers still needs to be earned.

## Chrono Cross forces a separate allocation layer

Independent AKAO tooling reports that *Chrono Cross: The Brink of Death* uses 31 logical AKAO channels/tracks, while the physical SPU exposes only 24 voices.

The bounded conclusion is:

```text
AKAO logical track capacity > physical SPU voice count
```

for a real same-work control.

This does not prove all 31 logical tracks are simultaneously active or audible. It does prove that an interpreter cannot identify sequence-track index with physical voice index.

The missing transformation is some form of runtime allocation/ownership mapping:

```text
AKAO logical track/event
-> driver note/voice request
-> allocation / reuse / stealing / reservation
-> one of 24 physical SPU voices
```

The exact AKAO allocation policy remains a driver question, not a chip question.

## AKAO event labels remain above the SPU

VGMTrans exposes AKAO labels such as:

- `FM (Pitch LFO)`
- pitch side-chain
- pitch-to-volume side-chain

but its parser does not demonstrate that these correspond directly to SPU PMON writes.

Therefore:

```text
AKAO pitch-related event label
!= proven SPU PMON transition
```

A valid correspondence requires observing:

```text
same AKAO event occurrence
-> same driver update
-> same allocated physical voice relationship
-> same SPU PMON/register transition
-> same effective physical voice trajectory
```

The new exact SPU model makes this future test possible without prejudging its answer.

## Tuning/export boundary

VGMTrans's historical Chrono Cross `Dragon God` tuning bug is another useful representation warning.

Later AKAO tuning can exceed the range representable by a destination SoundFont fine-tune field. Correct export required separating a larger source tuning displacement into coarse unity-key movement plus residual cents.

Therefore:

```text
AKAO source tuning coordinate
!= SoundFont fine-tune field
!= SPU VxPitch
```

Reference playback and extracted/exported representation can disagree even inside one mature tool. VGM Compiler must test parity at each transformation boundary rather than only checking that one final renderer sounds plausible.

## Literature boundary

The scholarly search performed during this pass returned useful general work on real-time dynamic voice allocation and broader game-audio constraints, but little peer-reviewed work specifying the original PlayStation SPU's exact PMON arithmetic.

That absence is itself informative:

- use peer-reviewed literature for general allocation/performance concepts where relevant;
- use hardware documentation, independent implementations, FPGA cores, game-specific reverse engineering, and eventual hardware tests for exact PS1 device semantics;
- do not inflate weakly related papers into chip-level evidence.

## Highest-information next tests

### 1. Direct-hardware PMON discriminator

Build a tiny PS1 test that exercises:

```text
high-bit VxPitch + PMON
```

and records whether real hardware follows the signed-carrier or unsigned-carrier path.

Also separately distinguish effective maximum step `0x3FFF` vs `0x4000`.

### 2. Chrono logical-to-physical allocation trace

For one bounded cue, preferably a high-pressure case such as *The Brink of Death*:

```text
AKAO track event
-> driver request
-> allocated SPU voice
-> key-on/off lifetime
-> source/tuning/ADSR
-> physical trajectory
```

Measure reuse and reservation rather than assuming one policy.

### 3. AKAO-to-PMON correspondence test

Find a cue containing a candidate AKAO pitch-side-chain/pitch-LFO event and observe whether PMON changes on the allocated SPU voice at the corresponding runtime boundary.

A negative result is valuable.

### 4. Dragon God tuning vertical slice

Follow one known tuning-sensitive articulation through:

```text
AKAO articulation tuning
-> driver pitch calculation
-> VxPitch
-> effective sample step
-> rendered pitch
```

Compare against VGMTrans's corrected export coordinate without assuming either representation is canonical.

## Stop conditions

Stop rather than guess if:

- logical AKAO track count is capped to 24 because the SPU has 24 voices;
- the previous voice's final stereo output is used as the PMON modulator;
- high-bit PMON arithmetic is called settled despite MiSTer's competing implementation;
- the `0x3FFF`/`0x4000` maximum-step disagreement is erased;
- an AKAO side-chain label is called PMON without runtime evidence;
- source tuning is copied directly into a destination fine-tune field;
- successful final playback is treated as proof that every intermediate representation is correct.

Correction outranks coherence.
