# Omniphony realtime spatial path

## Purpose

Retro VGM Compiler supplies Omniphony with causal source-native game-music objects before historical stereo collapse.

The audible target is intentionally creative:

> **Recover the real musical sources, then make the soundtrack sound as though those sources had always been mixed for a larger immersive format.**

The compiler does not claim that old hardware historically authored modern rear, height, distance or extent coordinates. It preserves source truth and hands Omniphony enough structure to make those modern `DERIVED` mix decisions coherently.

## Canonical realtime pipeline

The canonical wrapper is:

```text
realtime_musical_omniphony_pipeline::process_block(...)
```

Its causal order is:

```text
source-specific decoder / renderer
→ raw spatial_source_block_view
→ realtime_musical_spatial_frontend::prepare_block()
→ past-only musical-role projection
→ past-only adaptive scene budget
→ handoff.projected_view()
→ Omniphony ABI 0.4 scene-budget setter
→ allocation-free source/event transport
→ Omniphony render
→ only after success: complete_block(raw block)
```

Current PCM never chooses its own presentation retroactively.

A render failure does not advance observer, role or budget memory.

## Ownership boundary

```text
Retro VGM Compiler
  source truth
  source-quality admission
  source / generation identity
  persistent-part evidence
  authored route / send / timing evidence
  source-native shared effects
  completed-scene observation
  source-agnostic adaptive scene budget
        ↓
Omniphony
  NativeRouting / FullSphere presentation
  canonical 8.1.4.4 semantic world
  stable DERIVED geometry
  source extent
  22-direction shell
  cascaded binaural HRTF / ITD
  distance / air / optional room
```

The compiler must not pre-render a second spatial world.

Omniphony must not decide which emulator, reconstruction or source witness is more truthful.

## Source topology remains native

Omniphony's 17-lane 8.1.4.4 vocabulary is not a source-lane requirement.

Examples:

```text
YM2612       six complete FM channels
YM2151       eight complete FM channels
Genesis PSG  three tones + noise
SNES S-DSP   eight dry voices + linked shared wet L/R where proven
```

Therefore:

```text
physical chip channel
!= persistent musical part
!= speaker channel
!= canonical 8.1.4.4 lane
!= shell direction
```

Do not manufacture seventeen PCM lanes merely because Omniphony's scene has seventeen semantic names.

## Source authority

Use the conceptual authority states consistently:

```text
AUTHORED
preserved from source / device / driver / format

DERIVED
musical / perceptual / presentation inference

EMPTY
no authored fact exists
```

`EMPTY` does not require audible silence in `FullSphere`.

Examples:

- YM2612/YM2151 L/R route is authored.
- Stock Genesis PSG has real source identity but no independent authored pan register.
- Game Gear PSG per-channel L/R routing is authored.
- S-DSP echo send is authored send evidence.
- S-DSP final post-EVOL echo is a real shared historical field where captured.
- Foundation/foreground/diffuse/width/vertical affinity are derived musical-presentation evidence.
- The scene budget is derived renderer-control state, not source evidence.

## Omniphony presentation modes

Both source-aware modes now use the same physical Omniphony topology:

```text
same recovered sources
→ same extent-capable 22-direction System-H-derived shell
→ same cascaded binaural renderer
```

### NativeRouting

The source-aware control closes modern added geometry:

```text
creative rear = 0
creative height = 0
creative extra depth = 0
creative source extent = [0,0,0]
shared-wet modern expansion = 0
```

Native route, identity and genuinely authored source centre remain intact.

### FullSphere

The same renderer opens stable modern presentation:

```text
DERIVED azimuth
DERIVED rear depth
DERIVED elevation
DERIVED distance
DERIVED [width, depth, height] extent
```

Authored route and position remain constraints.

Keeping one physical renderer under both modes is important: a runtime NativeRouting ↔ FullSphere A/B should compare presentation policy, not direct-vs-cascaded binaural algorithms.

The protected historical/reference stereo remains the untouched control beneath both source-aware modes.

## Source extent

Retro VGM Compiler supplies width/diffuse/musical evidence. Omniphony converts the final source presentation to 3-D size.

In FullSphere:

```text
source centre + 3-D size
→ Omniphony object event
→ size-aware constant-power VBAP
→ 22-direction shell
→ cascaded binaural
```

The source-aware shell precomputes at least five extent states from zero to full size.

Increasing extent redistributes shell energy; it is not a source-gain control.

NativeRouting explicitly closes added extent even if a source carries derived width/diffuse evidence. There is currently no authored-extent authority field in the ABI.

## SPC / S-DSP: dry voices and echo must stay separate

Where source capture proves the necessary witnesses, preserve:

```text
8 dry S-DSP voices
+ signed per-voice L/R route
+ echo-send state
+ final post-EVOL shared echo L/R
```

The echo L/R components are **one linked historical stereo feedback field**.

They are not eight independent wet stems and they are not two unrelated reverbs.

This gives Omniphony independent presentation control over:

```text
dry-voice placement / extent
shared echo rear bias
shared echo elevation
shared echo radial depth
shared echo strength
shared echo 3-D extent
modern Omniphony room
```

The historical echo field and Omniphony's optional externalization room are separate layers.

A wet SPC soundtrack can therefore become large primarily through its own S-DSP field while keeping the dry voices sharper and asking for less generic room.

## Universal adaptation

The target is not one universal geometry.

> **Same aesthetic target, different spatial expenditure.**

The current observer measures completed causal audio using:

```text
active-source count / density
mean source activity
energy concentration / distribution
low-band energy share
edge / transient proxy
historical shared-effect energy share
coarse dry-source spectral overlap
```

No game, composer, soundtrack, genre or cue label is required.

## Coarse spectral-overlap observer

`realtime_musical_spatial_observer.h` now maintains a cheap persistent three-band profile per bounded source identity.

The mechanism is:

```text
sample
→ causal low split
→ causal upper split
→ [low, mid, high] instantaneous power
→ time-based persistent power envelope
→ normalized three-band source profile
```

Default splits are approximately:

```text
low / mid: 300 Hz
mid / high: 2500 Hz
profile memory: 0.20 s
```

The scene then compares **active dry sources only**.

For each dry pair:

```text
profile overlap = Σ min(left_band_share, right_band_share)
```

and combines pairs using an energy-derived weight.

The result is:

```text
coarse_spectral_overlap ∈ [0,1]
```

This is deliberately **not** called psychoacoustic masking.

It does not model critical bands, auditory thresholds, temporal masking or source salience with enough fidelity to make that claim.

Its value is that two equally dense scenes can now be distinguished:

```text
many sources occupying different broad bands
!= many sources crowding the same broad bands
```

The persistent filter/power state advances sample by sample, so splitting the same audio into different host callback sizes should converge to the same spectral profile.

Shared historical wet is excluded from this dry-source overlap statistic.

## Crowding response

The first implementation is intentionally conservative.

More coarse spectral crowding asks for:

```text
less dry-source extent
less dry diffuseness
slightly less shared-wet strength / extent
less added Omniphony room
```

It does **not** make objects wider in the name of separation.

It also does not yet modify global depth/height capacity.

Research on masking-aware automatic mixing supports spectral analysis plus spatial decisions, but true source-to-source unmasking deserves an explicit future separation/panning control. Do not smuggle that behavior into unrelated fields.

## Other adaptive pressures

The scene budget also responds to:

```text
source density
energy distribution
low-band weight
transient/edge density
historical wet share
```

Typical behavior:

```text
sparse / dry
→ larger objects
→ more immersive capacity

dense
→ tighter objects
→ protect articulation

bass / transient heavy
→ protect foundation and attacks

echo-heavy SPC
→ let historical wet carry envelopment
→ reduce modern room

spectrally crowded
→ reduce smear and extra ambience
```

The mix budget contracts faster than it expands:

```text
contraction ≈ 0.30 s
expansion   ≈ 1.50 s
```

That keeps sudden dense passages safe while preventing quiet gaps from pumping the world outward.

## Causality

Block N uses only the scene budget learned before block N.

```text
render block N with existing budget
→ if render succeeds
→ observe RAW completed block N
→ update role / overlap / scene budget
→ use result for block N+1
```

Never feed `handoff.projected_view()` back into `complete_block()`.

That would create a semantic feedback loop in which prior presentation decisions become evidence for themselves.

## ABI 0.4 transport

`model/omniphony_source_transport.h` mirrors the source DLL ABI without allocation.

ABI 0.4 preserves timed source evidence and adds:

```text
OmniphonySourceMixBudgetV1
  depth_scale
  height_scale
  shared_wet_strength_scale
  shared_wet_extent_scale
  externalization_scale
```

Dry width/diffuse scaling remains in the per-source presentation sidecar because those describe how an individual recovered object occupies the remix.

Scene-wide depth, height, wet and room decisions stay on the scene-control plane.

The transport must:

- reject protected reference-mix lanes as objects;
- preserve authored route without relabeling it as 3-D position;
- preserve strong persistent-part identity where earned;
- carry authored 3-D only when supplied;
- preserve ordered intra-block evidence events;
- refuse to invent missing source PCM;
- require ABI minor 0.4 for the adaptive client.

The ABI test pins binary sizes and offsets for both source evidence and the 0.4 mix-budget record.

## Exact event timing

A timed evidence event contains:

```text
frame_offset
lane_index
new evidence
```

Authored state changes keep their exact sample boundary.

Derived renderer motion may be perceptually ramped after that boundary, but source timing is not quantized for convenience.

## Identity continuity

Presentation identity prefers:

```text
persistent musical part
otherwise bounded source identity
```

An unrelated source reusing the same chip voice must not inherit the outgoing source's pose.

A persistent part that genuinely migrates across hardware slots may retain continuity.

## FM source boundary

For ordinary YM2612/YM2151-style synthesis:

```text
one complete FM channel = one default spatial source object
FM operator != independent spatial object
```

Algorithms, feedback and modulation make operators synthesis internals unless independent authored object identity is proven.

Likewise:

```text
higher-fidelity whole-chip renderer
!= proven independent additive stems
```

Do not spatialize unproven decompositions merely because the whole-chip renderer sounds better.

## Reset / seek

A new track, seek or decoder restart clears the complete causal presentation timeline:

```text
compiler acoustic observer
coarse spectral-profile memory
musical-role memory
adaptive mix-budget tracker
pending projected handoff
Omniphony source-presentation identity
Omniphony spatial / binaural history
Omniphony mix budget → neutral
```

A new soundtrack must not inherit the previous soundtrack's mix personality.

## Research grounding

The adaptive source path borrows obligations from immersive-audio and automatic-mixing literature without treating published methods as proof of our current tuning.

Useful anchors include:

- Jot, Carpentier & Warusfel, 2023, perceptually motivated object-scene rendering, DOI `10.1109/I3DA57090.2023.10289196`
- Landschoot & Jot, 2023, object-aware binaural externalization, DOI `10.1121/10.0018389`
- McCormack, Politis & Pulkki, 2021, covariance-constrained source spread, DOI `10.1109/WASPAA52581.2021.9632724`
- Anemüller, Thiergart & Habets, 2024, binaural rendering of sources with extent, DOI `10.1109/ICASSP48485.2024.10448024`
- Hafezi & Reiss, 2015, multitrack masking reduction
- Tom, Reiss & Depalle, 2019, automatic stereo spatialization using unmasking and panning practice

The literature supports source separation, perceptual width, dry/wet distinction, masking awareness and object-based presentation as useful dimensions.

It does **not** validate our current cutoff frequencies, smoothing constants, FullSphere positions or budget coefficients. Those remain engineering hypotheses requiring corpus and physical-listening tests.

## Validation obligations

At minimum defend:

```text
SOURCE TRUTH
authored route / timing / identity survive

NO AUTHORITY PROMOTION
DERIVED geometry and scene budget never become authored

MODE CONTROL
NativeRouting closes creative rear / height / depth / extent
FullSphere opens stable immersive geometry
both share one 22-direction/cascaded renderer
runtime mode switch does not recreate the processor

EXTENT
source size changes FullSphere headphone audio
changing extent does not move source centre
shell spread stays approximately constant power

SPC ECHO
post-EVOL L/R remain one linked shared field
shared-wet extent changes headphone field without moving centre
historical wet never becomes fabricated per-voice wet stems

ADAPTATION
budget is past-only
render failure does not advance state
reset returns budget/profile memory to neutral
coarse profile is callback-partition invariant
similar dry spectra overlap more than disjoint spectra
shared wet is excluded from dry overlap
more overlap cannot increase dry width/diffuse or added room

SOURCE BOUNDARY
FM operators remain synthesis internals by default
whole-chip fidelity does not imply stem additivity

REFERENCE
protected historical playback remains available
```

## Evidence states

Do not collapse:

```text
code written
compile/test success
source/reference correctness
block-size invariance
perceptual mechanism validity
physical listening quality
```

The architecture and regressions can establish the first several layers. The final question remains audible:

> **Does each soundtrack keep its own identity while gaining as much stable scale as its arrangement can support without losing groove, impact, clarity, timbre, transients or hierarchy?**