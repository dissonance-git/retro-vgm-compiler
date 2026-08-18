# Omniphony realtime spatial path

## Completed runtime boundary

Retro VGM Compiler can hand a source-aware musical scene to Omniphony without a whole-track prepass or cached soundtrack automation.

The canonical runtime entry point is now:

```text
realtime_musical_omniphony_pipeline::process_block(raw block, ...)
```

Internally it enforces:

```text
source-specific decoder / renderer
→ raw spatial_source_block_view
→ realtime_musical_spatial_frontend::prepare_block()
→ past-only musical role projection
→ handoff.projected_view()
→ omniphony_source_transport_storage::build()
→ caller-owned interleaved source scratch
→ Omniphony source_ffi ABI 0.3 timed-event call
→ binaural output
→ realtime_musical_spatial_frontend::complete_block(raw block)
```

The order is part of the API rather than a convention a host must remember. `prepare_block()` may consult only already-completed musical history. `complete_block()` runs only after the renderer accepts the block and receives the original raw source block, never the projected view.

That makes this invariant executable:

```text
same observed history through frame N
→ same spatial decision through frame N
```

Current-block PCM cannot retroactively spatialize itself.

A renderer failure does not advance musical memory. The caller can retry or fall back without the semantic state moving ahead of the audio that actually sounded.

## Canonical source-to-scene contract

The game-music path does **not** redefine Omniphony's product scene.

Omniphony currently owns a canonical **8.1.4.4 semantic scene** with 17 static lane names:

```text
L R C LFE Ls Rs Lb Rb Cb
Tfl Tfr Tbl Tbr
Bfl Bfr Bbl Bbr
```

and a separate current 22-direction full-sphere render shell above that scene.

Those are presentation structures owned by Omniphony. They are not a requirement that a VGM, SPC, PSF-family, tracker, driver or chip frontend manufacture seventeen PCM lanes.

The game-music boundary is instead:

```text
EMULATED / EXECUTED SOURCE TRUTH
registers • voices • channels • samples • driver parts • routing • timing
        ↓
PERSISTENT SOURCE OBJECTS
stable episode identity where earned
        ↓
AUTHORED EVIDENCE
native route • timing • send state • supplied position • source identity
        ↓
DERIVED EVIDENCE
musical role • directness/diffuseness • width tendency • vertical affinity • confidence
        ↓
OMNIPHONY SOURCE ABI
causal source objects + timed evidence
        ↓
OMNIPHONY PRESENTATION
8.1.4.4 semantic scene + dynamic objects
        ↓
22-DIRECTION RENDER SHELL
        ↓
BINAURAL
```

The architectural rule is:

> **Retro VGM Compiler tells Omniphony what the instrument actually did. Omniphony decides how the unauthored dimensions of that musical object inhabit its 8.1.4.4 world.**

A richer source therefore causes **less inference**, not more synthetic metadata.

### 8.1.4.4 is vocabulary, not forced source width

A source-aware chip render may expose six FM channels, eight OPM channels, four PSG voices, eight S-DSP voices, one shared wet return, or some other source-native topology. Preserve that topology.

Do not convert a source into static `L/R/C/...` lanes merely because Omniphony has those names.

```text
source object count
!= Omniphony canonical lane count
```

```text
hardware channel
!= speaker channel
!= canonical scene lane
!= render-shell direction
```

Dynamic source objects can be mapped into or around the canonical scene by Omniphony's presentation policy while the static 8.1.4.4 vocabulary remains intact.

If Omniphony later changes its internal shell, the compiler should not need to change merely because the render lattice changed. A compiler change is required only when the actual cross-project source contract changes.

## Provenance law: AUTHORED / DERIVED / EMPTY

Spatial information has authority, not just values.

Use three conceptual states:

```text
AUTHORED
preserved by the source / driver / device / format itself

DERIVED
inferred by musical, acoustic or perceptual reasoning

EMPTY
not supplied and not earned strongly enough to infer
```

Retro VGM Compiler's `spatial_evidence_authority` is the source-side evidence vocabulary. Omniphony's `AUTHORED`, `DERIVED` and `EMPTY` scene states are the presentation-side provenance vocabulary. They should agree semantically even when their concrete representations differ.

Examples:

- a YM2612 left/right enable is authored route evidence;
- a stock Genesis PSG voice number is source identity, not authored azimuth;
- an inferred bass/foundation role may justify conservative presentation behavior but is not authored depth;
- an S-DSP echo send is authored send state, not a license to clone one independent reverberant stem per voice;
- an actual source-supplied 3-D coordinate may be marked authored position;
- absence of evidence remains absence of evidence.

Never promote a derived coordinate to authored because it is stable, plausible or musically useful.

Never fill an unsupported dimension merely because the downstream scene has a slot for it.

## Source lane semantics

`model/spatial_source.h` distinguishes three audio-lane kinds:

```text
dry_source
shared_effect_return
reference_mix
```

They have different jobs.

### Dry source

A dry source lane is a causal localizable source witness when the source family can actually expose one.

It may carry:

- physical slot / channel identity;
- source episode identity;
- native stereo-route evidence;
- effect-send state;
- persistent musical-part identity when earned;
- inferred presentation evidence;
- authored position only when genuinely supplied.

A dry lane is not automatically an additive stem. Cross-source coupling, feedback, nonlinear arithmetic or hidden shared state may prevent exact recomposition even when useful isolated source audio exists.

### Shared effect return

A shared effect return is a distinct observable wet field produced by common processing.

Treat it as shared environmental/diffuse evidence unless the source proves a more specific structure.

Do not duplicate one shared return into N per-source wet stems. Doing so invents separability that the hardware or program never provided.

### Reference mix

The protected reference mix is the scientific and audible control.

It is not an object lane and must never receive object geometry or persistent-object memory.

Reference playback remains available underneath source-aware enhancement so spatial progress cannot silently redefine the musical object.

## Device-family authority examples

These examples describe the current architectural boundary, not a promise that every runtime path is already fully implemented.

| Source family / device | Source-native truth to preserve | Authored spatial/routing evidence | What Omniphony may derive |
| --- | --- | --- | --- |
| YM2612 / Genesis FM | six complete FM channel identities; channel-6/DAC distinction where applicable; causal channel audio/state | native L/R enables and their timing | unsupported elevation, depth, radial placement, extent and other presentation dimensions |
| Genesis PSG | three tone voices + noise identity and timing | stock Genesis provides no independent authored stereo pan register for ordinary PSG voices | essentially all spatial placement beyond source identity; do not fabricate authored pan |
| Game Gear PSG | three tone voices + noise identity and timing | explicit per-channel L/R speaker routing register | remaining unsupported dimensions |
| YM2151 / OPM | eight **complete FM channels**, preserving operator network, algorithm, feedback, envelopes, modulation and channel-local time evolution | authored channel L/R enables | unsupported elevation, depth, extent and full-sphere placement |
| SNES S-DSP dry path | eight bounded voice episodes / dry voice witnesses where the selected implementation proves them | signed per-voice L/R routing and echo-send state | unauthored vertical/depth/extent/semantic presentation |
| SNES S-DSP shared echo | final shared wet return where directly observable | shared wet identity and DSP provenance | diffuse/environmental presentation rather than a fabricated point source |

### FM operators are not spatial objects by default

For YM2612, YM2151 and related FM systems, the default source object is the **complete audible channel**, not its individual operators.

Operators can be mutually coupled through algorithms and feedback and together define one instrument voice. Splitting them into spatial objects would confuse synthesis internals with musical-source identity unless a future source format explicitly authored them as separate audible objects.

```text
FM operator
!= independent musical source
```

### Exact source witness is renderer-specific

A higher-fidelity whole-chip renderer does not automatically provide exact independent stems.

For example, a candidate whole-chip YM2151 renderer with a shared serial mixer, DAC path, clamp, or other coupling must not be promoted to exact per-channel enhanced stems until additivity / causal decomposition is independently demonstrated.

Fail closed:

```text
whole-chip fidelity improvement
!= proven independent source decomposition
```

## Musical projection

`model/realtime_musical_spatial_projection.h` maps only already-learned positive role support into the compact presentation vocabulary:

```text
foundation
foreground
diffuse
width
vertical_affinity
confidence
```

The projection deliberately does not invent geometry.

- foundation can make a source harder to dislodge;
- foreground can preserve near/front importance;
- environmental-layer evidence can contribute diffusion and bounded extent;
- transient-accent evidence is not by itself a musical role or authored position;
- no current role hypothesis invents elevation;
- authored 3-D coordinates, when genuinely present, remain separate and untouched.

A confidently near-zero role classification is not positive permission to move a source. Renderer confidence is driven by positive presentation support.

## Direct/localizable versus diffuse/shared energy

The source contract preserves a distinction that is both technically and perceptually important:

```text
localizable direct source objects
!= shared / diffuse reverberant field
```

This prevents two opposite failures:

1. collapsing everything into one stereo bus before spatial reasoning;
2. pretending shared reverberation is a set of independent point sources.

The distinction is consistent with binaural/spatial research that treats direct sound, early reflections and diffuse/late reverberation differently because they carry different localization and spaciousness information. Useful research anchors include Tohyama (2020), Menzies et al. (2021, DOI `10.1109/TASLP.2020.3036781`), Landschoot & Jot (2023, DOI `10.1121/10.0018389`), Greco et al. (2025, DOI `10.1186/s13636-025-00437-y`) and related direct/early/diffuse decomposition literature.

These references justify the architectural separation. They do not prove a particular Omniphony tuning.

## Temporal stability and confidence

Spatial inference is a tracked state, not a fresh coordinate guess on every callback.

```text
stable source identity
+ stable evidence
→ stable presentation tendency
```

A source must not jitter around the sphere because block boundaries, short-term spectra or weak role estimates fluctuate.

At the same time, continuity must not become glue. Genuine authored route changes, source replacement, strong persistent-part transitions or strong new perceptual evidence may move the object.

Recommended inference behavior:

- preserve source/persistent-part identity across blocks when earned;
- carry confidence separately from position;
- let evidence age and weaken rather than instantly disappear;
- use strong onsets/transients as high-information moments for **updating** a derived pose when other evidence supports the update;
- do not infer a role or position from onset alone;
- use bounded inertia / smoothing for derived motion;
- bypass or constrain smoothing where authored position or route changes require exactness;
- reset continuity when a physical channel is reused by an unrelated source episode.

The perceptual motivation is consistent with work showing that reverberation and ambiguous binaural cues increase localization uncertainty, while transient/onset-dominant cues can be disproportionately useful for stable scene formation. Stecker's RESTART account (2023, DOI `10.1121/10.0023479`) is a useful research pressure test for that latter idea.

Any concrete filter, Kalman model, Bayesian tracker or state machine remains an implementation choice to validate. The contract requires the behavior, not one algorithm.

## No semantic feedback loop

The projected block is renderer input only.

```text
raw source evidence
→ past-only projection
→ Omniphony

raw source evidence + completed PCM
→ observer / role update
→ future memory
```

Never feed `handoff.projected_view()` into `complete_block()`. The canonical pipeline prevents that misuse by construction. Doing so manually would let a previous hypothesis become evidence for itself.

## Timed events

Retro VGM Compiler source evidence supports exact intra-block events:

```text
frame_offset + lane_index + new evidence
```

Omniphony source ABI 0.3 exposes the corresponding timed event form. A host renders only up to the next event boundary, applies every change at that boundary, then continues.

Therefore a change at frame 137 remains a change at frame 137. It does not need to be quantized to the next host block and it does not need a precomputed song timeline.

Authored route changes keep their source timing. Derived presentation state may smooth perceptually afterward, but the evidence event itself must never be retimed to make rendering easier.

## Transport

`model/omniphony_source_transport.h` is the allocation-free compiler-side ABI mirror.

It:

- rejects protected reference-mix lanes;
- carries native stereo-route evidence without relabeling it as 3-D geometry;
- carries strong persistent-part identity when earned;
- carries authored position only when the source actually supplied one;
- carries the projected musical presentation values;
- preserves ordered intra-block evidence events;
- interleaves planar causal mono lanes into caller-owned scratch;
- refuses to manufacture zero PCM for unavailable source frames.

The C++ transport and Rust `repr(C)` records pin the ABI 0.3 binary layout from both sides. Field-order or size drift must fail tests rather than silently reinterpret musical evidence across the DLL boundary.

Retro VGM Compiler retains exact source provenance internally. Omniphony ABI 0.3 has one 64-bit runtime source token, so the transport derives a renderer-local episode token from compiler `source_id + generation`. That token is presentation identity only and must never be used as a compiler provenance key.

The existence of Omniphony's 17-lane canonical scene does not by itself require an ABI expansion. Add ABI fields only when new source evidence must cross the boundary and cannot be represented safely by the existing contract.

## Identity continuity

Omniphony distinguishes physical channel reuse from presentation identity.

```text
same persistent musical part
→ ordinary smooth spatial continuity may continue

unrelated source reuses same lane
→ do not ramp from the old source's pose
```

This prevents a stolen/reused hardware channel from making a new instrument appear to fly through space from the outgoing instrument's last location.

Presentation identity is committed only after a render succeeds, so a failed block cannot poison the next block's continuity decision.

## Reset / seek lifecycle

A track change, seek or decoder restart resets the complete causal timeline, not merely one half of it.

`realtime_musical_omniphony_pipeline::reset()` clears:

```text
compiler acoustic observer state
compiler musical-role / time memory
pending projected handoff state
Omniphony binaural/spatial runtime state
Omniphony source-presentation identity state
```

The reset function is part of the bound source ABI client. This prevents a fresh track or seek target from inheriting either an old musical hypothesis or an old renderer pose/identity decision.

## DSP ownership

There is one audible spatial-motion owner in the canonical path: Omniphony.

Retro VGM Compiler supplies causal musical/perceptual target evidence. Omniphony maps that evidence into its canonical semantic world and presentation geometry, then performs the actual sample-domain/ramped binaural rendering. This avoids stacking two independent spatial smoothers and keeps the existing Omniphony perceptual field as the renderer baseline.

`model/realtime_spatial_scene_dsp.h` remains useful as an executable reference for causal control trajectories, smoothing laws and tests, but it is not inserted as a second audible motion stage in front of Omniphony.

The path is still a DSP path:

```text
streaming source audio
+ bounded causal state
+ sample-timed evidence events
→ continuously updated Omniphony renderer state
→ audio out
```

It is not a precached soundtrack script.

## Cross-project ownership

The boundary is intentionally asymmetric:

```text
Retro VGM Compiler
  owns source truth
  owns source-native identity / timing / route / send evidence
  owns source-quality selection and reference-vs-enhanced validation
        ↓
Omniphony
  owns presentation geometry
  owns canonical 8.1.4.4 semantic scene
  owns 22-direction render shell
  owns binaural / room / distance presentation
```

Omniphony must not decide which emulator or enhanced source renderer is more truthful. Retro VGM Compiler must not reproduce Omniphony's spatial world internally and feed it a second already-spatialized scene.

The source-selection decision happens upstream. Spatial consumes the already-selected causal source witnesses.

## deepSTRF boundary

The active artificial-hearing research reference is `dissonance-git/deepSTRF`; retired `libaural` findings were consolidated there.

DeepSTRF remains a research teacher, not a playback dependency:

```text
Retro VGM Compiler source-authoritative fixtures
→ deepSTRF auditory obligations / adversarial experiments
→ compress the mechanism
→ only causal bounded survivors enter this realtime path
```

The current runtime already reflects its strongest state laws: source identity, acoustic continuity and perceptual object identity stay separate; ambiguous re-entry can remain ambiguous; durable memory can freeze under weak evidence; continuity and precision need not share one confidence clock.

## Validation obligations

The source-to-Omniphony foundation should be defended by regressions that target the boundary itself.

At minimum:

```text
AUTHORED ROUTE SURVIVAL
source-authored L/R routing reaches Omniphony unchanged as authored evidence

NO AUTHORITY PROMOTION
inferred geometry never becomes authored geometry

EMPTY PRESERVATION
unsupported dimensions remain unsupported rather than being filled to satisfy 8.1.4.4

SHARED-WET INTEGRITY
one shared effect return never becomes N fabricated independent wet stems

IDENTITY CONTINUITY
stable persistent source evidence does not jitter across callback/block sizes

GENUINE TRANSITION
new source identity or strong timed evidence can change pose without inheriting stale motion

SOURCE-OBJECT BOUNDARY
FM operators are not exposed as independent spatial objects by default

FAIL-CLOSED DECOMPOSITION
whole-chip enhanced renderers are not admitted as exact independent stems without an additivity/causal witness

REFERENCE PROTECTION
source-aware rendering cannot erase the protected reference control
```

Useful source-family adversarial fixtures include:

- Genesis PSG versus Game Gear PSG, because one lacks and one has explicit stereo routing;
- YM2612 authored pan changes at exact intra-block times;
- YM2151 channel output versus operator-internal state;
- SNES dry voices plus one shared wet return;
- physical-channel reuse by an unrelated persistent part;
- stable source evidence under several host block sizes;
- onset-supported derived position update followed by steady-state evidence that must not thrash the pose.

## What is finished versus what remains empirical

The spatial engineering path is closed when a format exposes causal source lanes:

```text
source-native truth
→ causal source objects
→ authored + derived evidence with provenance
→ causal musical state
→ exact timed renderer evidence
→ ABI-safe transport
→ Omniphony 8.1.4.4 semantic world / dynamic-object presentation
→ 22-direction shell
→ binaural DSP
→ future-state learning
```

Format/source-extraction coverage is a separate upstream problem. A format that does not yet expose trustworthy isolated source audio cannot be promoted by guessing stems.

Likewise, engineering completion is not a listening claim:

```text
code compiles / tests pass
!= reference parity
!= auditory mechanism validated
!= personalized listening quality improved
```

The transport, timing, identity, reset, authority and causality work establishes the realtime mechanism. Numeric role-to-space policy, object-placement tuning and personalized listening still require controlled reference comparisons and physical listening.
