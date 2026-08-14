# S-DSP signed-routing reachability is driver-relative

Status: **cross-driver source result + bounded corpus pressure + documentary context**

This note follows the signed-routing phase control in
`research/signed-routing-phase-geometry.md` and asks a harder question:

> the S-DSP can represent signed left/right voice gains, but can a particular
> game's music driver actually reach those states?

The answer differs across two late Super Famicom driver lineages.

```text
S-DSP hardware capability
!= driver-reachable state
!= state observed in a preserved work
```

That distinction is now directly evidenced rather than architectural caution.

## 1. Hardware coordinate

The S-DSP physical voice registers `VxVOLL` and `VxVOLR` are signed 8-bit
routing gains. The common model therefore preserves them as native routing
coordinates rather than converting them immediately into a scalar pan value.

For two sources A/B routed to L/R, the earlier finite control showed that the
sign product around the complete two-source/two-output cycle

```text
chi = sign(AL) sign(AR) sign(BL) sign(BR)
```

contains real linear information. With equal nonzero magnitudes, balanced
`chi=+1` signings collapse to rank one while unbalanced `chi=-1` signings are
rank two. With arbitrary positive magnitudes, `chi=-1` remains nonsingular.

This is standard signed-matrix/signed-graph algebra. The project-specific
question is whether a work's driver exposes the sign coordinate at all.

## 2. Gun Hazard: ordinary Akao music pan cannot reach the sign bit

Pinned observatories:

- `loveemu/vgm-disasm` commit
  `e96c5b35649f8e814cac3c31b65cedc07b52d76d`,
  `snes/Square/Minoru Akao/Front Mission - Gun Hazard.s`
- `vgmtrans/vgmtrans` commit
  `083f7c71fe773078061eb785573621082c3e0d1c`, Akao SNES v4/Gun Hazard parser

The Gun Hazard sequence vocabulary exposes ordinary scalar panning through
`C6` and pan fading through `C7`. The driver then derives physical left/right
voice magnitudes from that scalar pan after volume/tremolo/global scaling.

The relevant path remains bounded below the S-DSP signed-byte boundary:

```text
programmed pan
+ nonnegative bounded voice level
-> complementary nonnegative L/R coefficients
-> unsigned magnitude products
-> VxVOLL / VxVOLR
```

The ordinary music route therefore cannot produce `0x80..0xff` as a signed
negative L/R gain.

`tools/spc_driver_routing_reachability.py` exhausts the conservative finite
byte-coordinate version of this argument over:

```text
128 level values * 256 pan values = 32,768 route states
```

and confirms that the route remains nonnegative throughout.

This statement is deliberately scoped to the ordinary Akao music pan path. It
does not claim that no CPU/SFX/debug/direct-DSP path could ever write a signed
S-DSP volume register.

### Preserved-work pressure

`tools/spc_signed_routing_audit.py` reads the physical DSP register image from
SPC snapshots without projecting it back to scalar pan.

A manual sample during this research pass inspected 21 committed
Front Mission: Gun Hazard cues, covering 168 physical voice slots, and found
zero negative `VxVOLL`/`VxVOLR` values at the saved snapshot states.

The committed regression extends the same predicate to all 61 corpus SPCs:

```text
snapshot_count == 61
negative_gain_snapshot_count == 0
negative_gain_voice_count == 0
unbalanced_cycle_pair_count == 0
```

At the time of this note the 61-file regression is **committed but not yet
executed on hosted CI**, because the repository's GitHub-hosted runners are
blocked before job start by the account spending limit. Do not upgrade this to
an executed 61/61 result until the test actually runs.

The manual snapshot sample is temporal evidence, not a substitute for the
source-level reachability result. Its role is adversarial pressure: the saved
physical states sampled so far agree with the driver's reachable manifold.

## 3. Star Ocean: the Wolf Team sequence language exposes phase signs

Pinned observatory:

- `loveemu/vgm-disasm` commit
  `e96c5b35649f8e814cac3c31b65cedc07b52d76d`,
  `snes/Wolf Team/Star Ocean.s`

The Star Ocean voice mixer contains explicit conditional two's-complement
operations immediately before writing physical `VOL(L)` and `VOL(R)`.
The conditions are bits 6 and 5 of the per-voice state byte respectively.

More importantly, those bits are not hidden emulator microstate. Sequence
command `AD` writes them:

```text
AD 0  -> neither phase bit
AD 1  -> bit 5   -> right route can be negated
AD 2  -> bit 6   -> left route can be negated
AD 3+ -> bits 5+6 -> both routes can be negated
```

Therefore, for any voice with nonzero base left/right magnitudes, the authored
Wolf Team sequence vocabulary can reach all four sign quadrants:

```text
(+,+)
(+,-)
(-,+)
(-,-)
```

Two simultaneously routed voices can consequently realize an unbalanced
`chi=-1` routing cycle and the rank-two signed-routing geometry from the
previous finite control.

This is not a one-game accident. The pinned `Tales of Phantasia.s` driver from
the same Wolf Team lineage contains the same `AD` command and physical sign
consumption path. This establishes a driver-family mechanism, while Star Ocean
remains the work-specific positive-control target.

### Shared echo return is signed too

Star Ocean also contains global echo-volume presets including signed `0xe0`
(-32) values on left, right, or both echo returns. This is a separate shared-DSP
coordinate and must not be confused with the per-voice `AD` route signs.

So even within one game:

```text
voice phase routing
!= shared echo-return polarity
```

## 4. Human/documentary evidence

The exact code establishes mechanism. Human evidence changes how much semantic
weight we should give the mechanism.

In a 2016 Dengeki Online interview, Motoi Sakuraba recalled that the original
Star Ocean Super Famicom workflow required him to work at tri-Ace's offices,
compose/prepare material using another sound source, convert it, listen to the
result, discover that it was different, and repeatedly adjust it. He
specifically said that volume balance changed in conversion and had to be
retuned.

Source:

- Dengeki Online, 2016-03-28,
  `https://dengekionline.com/elem/000/001/242/1242446/`

This supports a conservative historical interpretation:

```text
source composition
-> platform conversion / driver realization
-> listening
-> corrective adjustment
```

was part of the actual creative/technical workflow for Star Ocean.

It does **not** prove that Sakuraba personally selected every `AD` phase command,
that he designed the driver, or that signed routing was always an aesthetic
choice rather than an implementation choice.

The first Star Ocean credits list Motoi Sakuraba for music and Kouichi Imazaki
as sound engineer; Hiroya Hatsushiba is credited for field programming on the
first game and is explicitly identified as the sound programmer responsible
for music and character-voice work in the 1998 Star Ocean: Second Story
interview. Later Star Ocean credits repeatedly identify Hatsushiba in
sound-programming/directing roles. This lineage is useful provenance context,
not proof of authorship for the exact 1996 SPC700 routine.

A 2020 HCS thread independently records that the Star Ocean in-game sound
player offered separate output and DSP choices, and that the circulating SPC
rip used `surround` output with the `arena` DSP setting:

- `https://www.hcs64.com/mboard/forum.php?showthread=61156`

That is practitioner/community evidence for a user-visible surround state. The
exact causal route from that UI option to `AD`, the global bypass flags, or the
signed echo presets has **not yet been proven**. Keep that gap visible.

## 5. Psychoacoustic boundary

Signed routing is not synonymous with perceived stereo width.

Relevant established audio literature includes:

- Juha Vilkamo and Ville Pulkki,
  "Adaptive Optimization of Interchannel Coherence with Stereo and Surround
  Audio Content," *Journal of the Audio Engineering Society* 62(12), 2014,
  DOI `10.17743/jaes.2014.0046`.
- Miyoung Kim, Eunmi Oh, and Hwan Shim,
  "Stereo Audio Coding Improved by Phase Parameters," AES 129th Convention,
  Paper 8289, 2010.

The first treats interchannel coherence as an important contributor to
perceived width. The second explicitly addresses cancellation problems in
strongly out-of-phase stereo material when left and right are downmixed by
summation.

Those results support preserving phase/correlation information, but they do not
license this shortcut:

```text
routing chi / matrix rank == perceived width
```

The defensible path is:

```text
native signed route state
-> realized L/R signals
-> time/frequency-dependent interchannel relation
-> listening geometry / reproduction system
-> perceived width, localization, envelopment, mono compatibility
```

## 6. New project result: device state space has a driver-relative reachable subset

The comparison earns a stronger representation law than the original phase
experiment:

```text
DEVICE STATE SPACE
all states the silicon can represent

DRIVER-REACHABLE STATE SPACE
states a particular sound program can generate under its admitted commands

WORK-OBSERVED STATE SPACE
states actually witnessed for a specific preserved execution/corpus
```

These sets must remain distinct.

For the current control:

```text
same physical S-DSP signed VxVOL registers

Gun Hazard / Square Akao ordinary music
    -> nonnegative route manifold

Star Ocean / Wolf Team
    -> authored sequence command reaches all four sign quadrants
```

This matters beyond stereo routing. The same idea can be applied to:

- FM operator/channel modes;
- hardware rhythm/DAC repurposing;
- SPU pitch modulation;
- noise modes;
- interpolation/filter bypasses;
- reverb/echo modes;
- sample-memory address regions;
- timer/clock states.

A hardware feature should not automatically become part of a work's legal
source-native transformation space merely because the chip supports it.

Conversely, a state that looks exotic at the hardware layer should not be
flagged as anomalous when the source driver explicitly exposes it.

## 7. Preservation and validation consequence

Driver reachability can become a validation surface.

For example, a controlled Gun Hazard music-only trace that suddenly contains a
negative physical voice gain should trigger investigation:

```text
unmodeled driver path?
SFX/direct DSP interaction?
transformed rip?
corrupt capture?
incorrect adapter?
```

It should not be silently accepted merely because the S-DSP register permits
that byte value.

Likewise, negative Star Ocean routing should not be "fixed" into conventional
positive stereo panning. The driver deliberately contains a sequence-level
route-sign coordinate.

This gives source-native enhancement a useful constraint:

> improve within the work's evidenced transformation vocabulary unless a
> deliberate experiment explicitly crosses that boundary.

## Executable controls

- `tools/signed_routing_phase_control.py`
- `tools/spc_signed_routing_audit.py`
- `tools/spc_driver_routing_reachability.py`
- `tests/spc/test_spc_signed_routing_audit.py`
- `tests/spc/test_spc_driver_routing_reachability.py`

## Claim boundary

Established here:

- S-DSP signed per-voice routing is physically meaningful state;
- ordinary Gun Hazard Akao music panning cannot reach negative route gains;
- a 21-cue manual snapshot sample agrees with that restriction;
- Star Ocean's Wolf Team driver can negate either voice route through an
  authored sequence command;
- Tales of Phantasia independently confirms that mechanism within the same
  driver family;
- Star Ocean also has separate signed shared-echo return presets;
- human testimony confirms that platform conversion and volume rebalance were
  actively iterated during the original Star Ocean workflow.

Not established here:

- a complete dynamic audit of all 61 Gun Hazard cues;
- which Star Ocean songs execute `AD` and at what musical moments;
- the exact UI `surround` option -> driver-state causal path;
- that route-sign topology alone predicts perceived width;
- that Sakuraba personally authored every low-level route-sign event;
- a universal law about all SNES music drivers.
