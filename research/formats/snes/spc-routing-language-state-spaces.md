# SNES routing languages define different reachable acoustic state spaces

Status: **cross-driver finite/source control + documentary/literature triangulation**

This note extends `spc-driver-routing-reachability.md` from a two-driver
comparison to five independent Super Famicom music-driver lineages.

The receiving question is not merely whether the S-DSP supports signed stereo
voice gains. It is:

> which signed-routing states are reachable through the music language that a
> particular driver exposes, and which subset is considered legal by an
> authoring tool?

The resulting hierarchy is:

```text
silicon-representable state
-> driver-decodable / driver-reachable state
-> authoring-language legal state
-> work-observed state
```

These layers are not interchangeable.

## Pinned code observatories

Primary reverse-engineering source for the original drivers:

- `loveemu/vgm-disasm`
- commit `e96c5b35649f8e814cac3c31b65cedc07b52d76d`

Files used:

- `snes/Square/Minoru Akao/Front Mission - Gun Hazard.s`
- `snes/Wolf Team/Star Ocean.s`
- `snes/Wolf Team/Tales of Phantasia.s`
- `snes/NSPC/Nintendo/Koji Kondo/Legend of Zelda - A Link to the Past.s`
- `snes/Capcom/Mega Man X.s`
- `snes/Konami/Axelay.s`

Independent modern N-SPC authoring observatory:

- `HertzDevil/AddmusicK`
- commit `c6f8f46ff7d1cbdfba62de20d2df6130b772aa62`
- `AddmusicK/Music.cpp`

The original-driver disassemblies establish what the pinned executable code
does. AddmusicK is used only as an independent modern authoring-language
control; it does not establish the exact restrictions of Nintendo's historical
internal editor.

## Cross-driver result

For the identified ordinary music-routing paths:

```text
authored signed per-voice routing reachable

Nintendo N-SPC / A Link to the Past        YES
Wolf Team / Star Ocean + Tales of Phantasia YES

ordinary music pan constrained nonnegative

Square Akao / Front Mission: Gun Hazard     NO
Capcom / Mega Man X                         NO
Konami / Axelay                             NO
```

Every row is deliberately path-scoped. A negative result for ordinary music
pan does not prove that CPU-direct DSP writes, SFX paths, debug code, or some
other command in the game can never create a negative physical gain.

The result is nevertheless enough to reject the model:

```text
same S-DSP hardware
-> same legal stereo state space for every game
```

The legal/reachable acoustic vocabulary depends on the sound program above the
silicon.

## Nintendo N-SPC: pan is a position fibered by phase

In the pinned A Link to the Past build, sequence command `E1` stores the entire
argument byte and also masks its low five bits into the scalar pan coordinate.
The later physical mixer reloads the stored raw byte and uses bits 7 and 6 to
conditionally two's-complement the left/right route respectively.

Thus an E1 byte has the semantic shape:

```text
bit 7      left-route polarity
bit 6      right-route polarity
bit 5      unused by the audited pan/mixer state
bits 0..4  scalar pan index
```

The signed-route coordinate is therefore not hidden hardware state. It is part
of the ordinary sequence pan command.

For a nonzero base L/R route, the command can reach all four sign quadrants:

```text
(+,+)
(+,-)
(-,+)
(-,-)
```

This gives a concrete source-language realization of the signed-routing phase
fiber measured in `signed-routing-phase-geometry.md`.

### Exact safe quotient: bit 5

A full reference audit of `$0351+x` in the pinned build finds only:

1. the E1 raw-byte write;
2. the physical mixer read, where only bits 7/6 matter;
3. an internal initialization write of a fixed value.

The low five bits had already been copied into the scalar pan coordinate.
There is no downstream read of bit 5.

`tools/nspc_pan_behavioral_equivalence.py` therefore computes an exact finite
quotient:

```text
raw E1 byte values              256
driver-semantic pan/phase states 128
class size                         2
```

Every equivalence class is exactly:

```text
{ b, b XOR 0x20 }
```

Subsequent E1 commands overwrite the raw pan state, so 32,768 exact
representative/action checks confirm that a future E1 assignment cannot reveal
which bit-5 representative preceded it.

This is the complementary finite result to the earlier NES nonlinear-mixer
horizon control:

```text
NES
currently hidden distinction
-> becomes observable after two admitted interventions
-> unsafe to merge

pinned Zelda N-SPC E1 bit 5
raw distinction
-> no audited consequence reads it
-> remains merged under declared E1 continuations
-> safe to quotient in a derived semantic view
```

The original source byte remains provenance. Safe semantic quotienting is not a
license to rewrite preserved data.

## Driver-decodable is not authoring-legal

The same E1 command exposes another state-space boundary.

The pinned runtime masks five pan bits and therefore has 32 possible scalar
indices at the byte-decoder level.

But its actual pan table contains 21 intended entries. A raw index above 20 is
not rejected before the mixer indexes the table region.

Independent AddmusicK authoring syntax explicitly accepts:

```text
pan 0..20
+ optional left surround boolean
+ optional right surround boolean
```

and packs the two booleans into bits 7/6 without emitting bit 5.

Therefore the finite domains are:

```text
raw command byte space                       256
pinned-driver behavioral pan/phase quotient 128
modern AddmusicK authored semantic states     84
pinned-driver semantic states outside that
  modern authoring contract                   44
```

The 44 extra states are exactly:

```text
pan indices 21..31
x four route-polarity combinations
```

Do not interpret those 44 states as intended Nintendo musical vocabulary. The
result only says that the pinned runtime code can decode them whereas the
independent modern N-SPC-compatible authoring language refuses to generate
them.

So:

```text
runtime-decodable
!= authoring-language legal
```

This distinction should be preserved anywhere Game Music Interpreter attempts
to infer intent or offer source-native editing controls.

## Wolf Team: a separate phase command

The pinned Star Ocean/Tales of Phantasia lineage reaches the same physical
signed-routing capability through a different sequence design.

`vcmd AD` controls two per-voice phase flags. The physical L/R mixer consumes
those flags as conditional sign inversions. For representative nonzero route
magnitudes, arguments 0..3 reach all four sign quadrants.

So Nintendo and Wolf Team expose a related acoustic coordinate through
different source grammars:

```text
Nintendo N-SPC
pan position + phase bits in one E1 byte

Wolf Team
ordinary route magnitudes + separate AD phase command
```

The common semantic coordinate is downstream of the source-language syntax.
The source syntax itself must remain driver-native.

## Three ordinary-pan negative controls

### Square Akao / Gun Hazard

The ordinary C6/C7 scalar pan path derives complementary nonnegative L/R
magnitudes from a bounded nonnegative voice level. Exhausting the conservative
finite coordinate in `spc_driver_routing_reachability.py` checks 32,768
level/pan combinations and never reaches the S-DSP sign bit.

A manual pressure sample of 21 committed Gun Hazard SPC snapshots, covering 168
physical voice slots, likewise found no negative saved VxVOLL/VxVOLR value.
The separate 61-SPC corpus regression is committed but remains unexecuted on
hosted CI while GitHub runners are billing-blocked.

### Capcom / Mega Man X

The pinned Capcom mixer uses a centered scalar pan coordinate and a nonnegative
Nintendo-compatible magnitude curve. Its final physical L/R stage multiplies a
0..127 pan magnitude by an unsigned 8-bit level and keeps the high byte.
Exhausting all 32,768 pairs yields a maximum physical gain of 126, below the
signed-byte boundary.

This establishes the ordinary pan path only.

### Konami / Axelay

The pinned Axelay mixer uses separate nonnegative 21-entry left/right pan
tables. The pre-pan level is constrained below 128 and the final table product
is normalized by 127. Exhausting all 16,384 bounded final-stage combinations
yields physical gain 0..127 and never sets the sign bit.

Again, this is the ordinary pan path rather than every possible DSP write in
the game.

## Human evidence: this layer was part of the creative workflow

### Nintendo / Koji Kondo

In the translated history of Nintendo game music, Kondo explicitly remembers
the Super Famicom as fully stereo and says Nintendo tried to make use of the
stereo effect, including movement in the stereo field.

Source:

- Shmuplations, "The History of Nintendo Game Music (1983-2001)"
  `https://shmuplations.com/nintendogamemusic/`

A contemporary 1990 Super Mario World interview also gives the relevant
workflow boundary. Kondo describes composing on keyboards whose sounds did not
match the SFC, moving material onto the computer/SFC, listening to the real
hardware, and building the piece gradually by adjusting lines and sounds.

Source:

- Shmuplations translation, "Super Mario World – 1990 Developer Interview"
  `https://shmuplations.com/supermarioworld/`

His 2001 retrospective further describes the SFC as a decisive new sound world
and notes that he was still doing sound programming through the end of that
era.

Source:

- Shmuplations, "Koji Kondo – 2001 Composer Interview"
  `https://shmuplations.com/kojikondo/`

These sources support treating platform stereo realization as part of
Nintendo's actual sound-making workflow. They do not prove that Kondo
personally selected every E1 phase bit in A Link to the Past.

### Star Ocean / Motoi Sakuraba

Sakuraba recalls the first Star Ocean workflow as compose/prepare using another
sound source, convert, listen, notice that the result changed, and retune it;
he specifically mentions changed volume balance.

Source:

- Dengeki Online, 2016-03-28
  `https://dengekionline.com/elem/000/001/242/1242446/`

That testimony makes the platform conversion/realization layer historically
creative rather than merely an emulator implementation detail. It still does
not prove that Sakuraba personally authored every low-level phase event.

## Literature boundary

The psychoacoustic literature supports preserving interchannel phase and
coherence while strongly warning against turning a route-sign bit into a
perceptual label.

Useful controls include:

- Juha Vilkamo and Ville Pulkki,
  "Adaptive Optimization of Interchannel Coherence with Stereo and Surround
  Audio Content," *Journal of the Audio Engineering Society* 62(12), 2014,
  DOI `10.17743/jaes.2014.0046`.
- Chungeun Kim, Emad M. Grais, Russell Mason, and Mark D. Plumbley,
  "Perception of phase changes in the context of musical audio source
  separation," JAES/AES, 2018.
- Johannes Käsbach et al.,
  "Assessing and modeling apparent source width perception," 2016.
- Ping Wang, Z. Lin, and Xiaojun Qiu,
  work on interaural phase difference, localization, and auditory source width,
  2018.

The safe causal order remains:

```text
native route sign / gain
-> realized stereo waveform
-> time/frequency interchannel phase + level + coherence
-> reproduction/listening geometry
-> perceived location / width / envelopment / mono compatibility
```

Therefore:

```text
signed-routing cycle product
!= perceived surround amount
```

The cycle invariant is a structural signal-coordinate, not a listening metric.

## New general control surface

The combined pass suggests the following evidence ladder for any device
feature, not just SNES stereo:

```text
1. DEVICE CAPABILITY
   silicon can represent state S

2. DRIVER REACHABILITY
   this sound program can generate S

3. AUTHORING LEGALITY
   the relevant tool/language exposes S as an intended source operation

4. WORK OBSERVATION
   controlled execution of this work actually reaches S

5. HUMAN / DOCUMENTARY INTERPRETATION
   available evidence explains whether/how creators treated the coordinate

6. PERCEPTUAL CONSEQUENCE
   listening/acoustic evidence establishes what S does for a listener
```

Skipping levels creates characteristic errors:

```text
hardware supports it
-> assume composer intended it             [too strong]

driver decodes it
-> assume authoring tool generated it       [too strong]

tool exposes it
-> assume this work used it                 [too strong]

work used it
-> infer perceptual effect directly         [too strong]
```

This ladder is a useful target for future source-native enhancement and
attribution controls.

## Executable controls

- `tools/signed_routing_phase_control.py`
- `tools/spc_signed_routing_audit.py`
- `tools/spc_driver_routing_reachability.py`
- `tools/nspc_pan_behavioral_equivalence.py`
- `tests/spc/test_spc_signed_routing_audit.py`
- `tests/spc/test_spc_driver_routing_reachability.py`
- `tests/spc/test_nspc_pan_behavioral_equivalence.py`

## Next falsifiers

1. Add signed VxVOLL/VxVOLR to the controlled dynamic SPC capture boundary so
   saved-snapshot evidence can be upgraded to trajectories.
2. Acquire or authorize a Star Ocean SPC control and determine which cues
   actually execute signed per-voice routing.
3. Trace the Star Ocean user-facing surround option to the exact SPC700 mode
   state instead of inferring the linkage from community labels plus plausible
   handlers.
4. Test more independent SNES driver families, but stop adding families once
   the reachable-state partition stops changing the common model.
5. Reuse the same ladder on FM rhythm/DAC modes, SPU PMON, sample interpolation
   switches, and driver-specific clock modes.

## Claim boundary

This is a bounded cross-driver control, not a census of SNES music engines.
The positive/negative labels apply only to the identified ordinary music
routing mechanisms at the pinned revisions. Human testimony establishes
workflow context, not byte-level authorship. Psychoacoustic literature
establishes the importance of phase/coherence to perception, not a direct map
from one driver bit to perceived width.
