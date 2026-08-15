# Omniphony realtime spatial path

## Completed runtime boundary

Game Music Interpreter can now hand a source-aware musical scene to Omniphony without a whole-track prepass or cached soundtrack automation.

The runtime sequence is:

```text
source-specific decoder / renderer
→ spatial_source_block_view
→ realtime_musical_spatial_frontend::prepare_block()
→ past-only musical role projection
→ handoff.projected_view()
→ omniphony_source_transport_storage::build()
→ caller-owned interleaved source scratch
→ Omniphony source_ffi ABI 0.3 timed-event call
→ binaural output
→ realtime_musical_spatial_frontend::complete_block(raw block)
```

The order matters. `prepare_block()` is called before rendering and may consult only already-completed musical history. `complete_block()` is called after rendering and receives the original raw source block, never the projected view.

That makes this invariant executable:

```text
same observed history through frame N
→ same spatial decision through frame N
```

Current-block PCM cannot retroactively spatialize itself.

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

A confidently near-zero role classification is also not treated as positive permission to move a source. Renderer confidence is driven by positive presentation support.

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

Never feed `handoff.projected_view()` into `complete_block()`. Doing so would let a previous hypothesis become evidence for itself.

## Timed events

GMI source evidence already supports exact intra-block events:

```text
frame_offset + lane_index + new evidence
```

Omniphony source ABI 0.3 now exposes the corresponding timed event form. A host renders only up to the next event boundary, applies every change at that boundary, then continues.

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

GMI retains exact source provenance internally. Omniphony ABI 0.3 has one 64-bit runtime source token, so the transport derives a renderer-local episode token from GMI `source_id + generation`. That token is presentation identity only and must never be used as a GMI provenance key.

## Identity continuity

Omniphony now distinguishes physical channel reuse from presentation identity.

```text
same persistent musical part
→ ordinary smooth spatial continuity may continue

unrelated source reuses same lane
→ do not ramp from the old source's pose
```

This prevents a stolen/reused hardware channel from making a new instrument appear to fly through space from the outgoing instrument's last location.

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

## Validation states

Keep these separate:

```text
code compiles / tests pass
!= reference parity
!= auditory mechanism validated
!= listening quality improved
```

The transport and timing work establishes the realtime mechanism. Whether a particular musical-role policy or spatial mapping sounds better still requires reference comparisons and physical listening.
