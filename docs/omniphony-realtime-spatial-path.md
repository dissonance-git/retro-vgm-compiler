# Omniphony realtime spatial path

## Completed runtime boundary

Game Music Interpreter can hand a source-aware musical scene to Omniphony without a whole-track prepass or cached soundtrack automation.

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
- transient-accent evidence is not yet mapped because onset alone does not determine musical role;
- no current role hypothesis invents elevation;
- authored 3-D coordinates, when genuinely present, remain separate and untouched.

A confidently near-zero role classification is not positive permission to move a source. Renderer confidence is driven by positive presentation support.

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

GMI source evidence supports exact intra-block events:

```text
frame_offset + lane_index + new evidence
```

Omniphony source ABI 0.3 exposes the corresponding timed event form. A host renders only up to the next event boundary, applies every change at that boundary, then continues.

Therefore a change at frame 137 remains a change at frame 137. It does not need to be quantized to the next host block and it does not need a precomputed song timeline.

## Transport

`model/omniphony_source_transport.h` is the allocation-free GMI-side ABI mirror.

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

GMI retains exact source provenance internally. Omniphony ABI 0.3 has one 64-bit runtime source token, so the transport derives a renderer-local episode token from GMI `source_id + generation`. That token is presentation identity only and must never be used as a GMI provenance key.

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
GMI acoustic observer state
GMI musical-role / time memory
pending projected handoff state
Omniphony binaural/spatial runtime state
Omniphony source-presentation identity state
```

The reset function is part of the bound source ABI client. This prevents a fresh track or seek target from inheriting either an old musical hypothesis or an old renderer pose/identity decision.

## DSP ownership

There is one audible spatial-motion owner in the canonical path: Omniphony.

GMI supplies causal musical/perceptual target evidence. Omniphony maps that evidence into presentation geometry and performs the actual sample-domain/ramped binaural rendering. This avoids stacking two independent spatial smoothers and keeps the existing Omniphony perceptual field as the renderer baseline.

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

## deepSTRF boundary

The active artificial-hearing research reference is `dissonance-git/deepSTRF`; retired `libaural` findings were consolidated there.

DeepSTRF remains a research teacher, not a playback dependency:

```text
GMI source-authoritative fixtures
→ deepSTRF auditory obligations / adversarial experiments
→ compress the mechanism
→ only causal bounded survivors enter this realtime path
```

The current runtime already reflects its strongest state laws: source identity, acoustic continuity and perceptual object identity stay separate; ambiguous re-entry can remain ambiguous; durable memory can freeze under weak evidence; continuity and precision need not share one confidence clock.

## What is finished versus what remains empirical

The spatial engineering path is closed when a format exposes causal source lanes:

```text
source lanes
→ causal musical state
→ exact timed renderer evidence
→ ABI-safe transport
→ Omniphony binaural DSP
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

The transport, timing, identity, reset and causality work establishes the realtime mechanism. Numeric role-to-space policy and personalized tuning still require controlled reference comparisons and physical listening.
