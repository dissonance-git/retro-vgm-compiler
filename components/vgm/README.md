# VGM component

`components/vgm/` owns VGM/VGZ source-family execution, device-state recovery, source capture, and source-native rendering machinery.

It does not own repository status, the shared musical ontology, or Omniphony presentation policy. Those live in [`../../docs/architecture.md`](../../docs/architecture.md), [`../../docs/source-native-enhanced-rendering.md`](../../docs/source-native-enhanced-rendering.md), [`../../docs/omniphony-realtime-spatial-path.md`](../../docs/omniphony-realtime-spatial-path.md), and the active frontier in [`../../docs/vgm-compiler-roadmap.md`](../../docs/vgm-compiler-roadmap.md).

## Source law

```text
VGM command stream
→ version / timing / data-block semantics
→ chip-family-specific execution
→ exact or bounded source evidence
→ source-relative performance evidence
→ shared musical model only where independently earned
```

The ordered source trace remains canonical evidence. Replayed device state is a rebuildable projection.

Keep these identities separate:

```text
device transition
!= physical voice episode
!= musical note
!= persistent musical part
!= authored spatial object
```

Chip-family pitch/register coordinates may project into common frequency or performance relations without becoming one native encoding.

Capture gaps, unsupported commands, ambiguous source contribution, or incomplete state fail closed rather than fabricating continuity.

## Implementation owners

```text
enhancement/
  project-owned VGM command/device/source semantics
  source capture and source-family recomposition
  analysis/performance adapters
  reversible enhanced-source machinery

foo_input_vgm/
  foobar2000 host integration
  exact reference playback foundation
  source sidecars / realtime bridge into project-owned semantics
```

The tree is the inventory. Do not mirror every supported device, test, patch, or frontier item here.

## Reference and enhancement boundary

Reference playback, source-state infrastructure, an enhancement core compiling, an admitted source replacement, and a retained listening improvement are different evidence states.

Enhanced playback must not add a second synthesizer on top of the protected mix. The lawful replacement shape is:

```text
protected reference mix
+ admitted replacement for one proven source
- exact historical contribution of that same source
= source-replaced mix
```

Unknown or unsupported contributions remain in the protected reference path.

A source-family backend may relax a historical quality ceiling only when the intervention preserves source identity, timing, synthesis relationships, and other identity-bearing behavior required by the durable rendering contract.

## Spatial handoff

VGM Compiler owns source truth and source-quality admission. Omniphony owns final spatial realization.

Device-authored routing may constrain presentation, but a hardware pan value or chip lane does not become authored 3-D geometry without evidence.

For realtime handoff semantics see [`../../docs/omniphony-realtime-spatial-path.md`](../../docs/omniphony-realtime-spatial-path.md).

## External dependency boundary

Pinned upstream libvgm behavior remains a reference and dependency boundary. Project-owned patches live under [`../../patches/libvgm/`](../../patches/libvgm/) and must fail closed when the expected upstream source shape drifts.

Format facts come from the VGM specification. Device behavior comes from chip documentation, drivers, and mature implementations. One does not silently substitute for the other.

## Validation

Use the narrowest executable owner first, then broader repository suites when the change crosses component boundaries.

Keep separate:

```text
format/container correctness
device/source correctness
compile/link success
unit/integration tests
real-corpus behavior
private foobar delivery
physical listening
```

Current unresolved VGM work belongs only in [`../../docs/vgm-compiler-roadmap.md`](../../docs/vgm-compiler-roadmap.md). Git history owns completed frontier narrative.
