# Omniphony realtime spatial path

## Completed runtime boundary

Retro VGM Compiler can hand a source-aware musical scene to Omniphony without a whole-track prepass or cached soundtrack automation.

The canonical runtime entry point is:

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

`prepare_block()` may consult only already-completed musical history. `complete_block()` runs only after the renderer accepts the block and receives the original raw source block, never the projected view.

```text
same observed history through frame N
→ same source evidence through frame N
```

A renderer failure does not advance musical memory.

## Product intent: source-native immersive remix

The source-aware Surround path is not limited to forensic reconstruction of the speaker dimensions that old hardware could encode.

Its audible target is:

> **Recover the real musical sources, then present them as though the soundtrack had always been mixed for a larger immersive format.**

That is a modern presentation choice, not a historical-authorship claim.

Retro VGM Compiler remains strict about what the source actually did. Omniphony is deliberately free to use otherwise unauthored width, rear depth, elevation, extent and distance when `FullSphere` presentation is selected.

The closest production analogy is an immersive remix made from multitracks. Here the "multitracks" are reconstructed causal chip/DSP source objects rather than studio stems.

```text
historical / executable source truth
        ↓
recovered real source objects
        ↓
authored route + timing + identity constraints
        ↓
musical / perceptual presentation evidence
        ↓
Omniphony FullSphere immersive mix
        ↓
8.1.4.4 semantic world + dynamic objects
        ↓
22-direction shell
        ↓
binaural
```

The protected reference path remains available underneath the enhancement.

## Canonical source-to-scene contract

The game-music path does **not** redefine Omniphony's product scene.

Omniphony owns a canonical **8.1.4.4 semantic scene** with 17 static lane names:

```text
L R C LFE Ls Rs Lb Rb Cb
Tfl Tfr Tbl Tbr
Bfl Bfr Bbl Bbr
```

and a separate current 22-direction full-sphere render shell above that scene.

Those are presentation structures owned by Omniphony. They do not require a VGM, SPC, PSF-family, tracker, driver or chip frontend to manufacture seventeen PCM lanes.

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
NativeRouting or FullSphere
        ↓
8.1.4.4 semantic world + dynamic objects
        ↓
22-DIRECTION RENDER SHELL
        ↓
BINAURAL
```

The ownership rule is:

> **Retro VGM Compiler tells Omniphony what the instrument actually did. Omniphony decides how that musical object inhabits the immersive world.**

Richer source truth reduces uncertainty about the source. It does not remove Omniphony's permission to make an explicit creative mix decision.

### 8.1.4.4 is vocabulary, not forced source width

A source-aware render may expose six FM channels, eight OPM channels, four PSG voices, eight S-DSP voices, one shared wet return, or another source-native topology. Preserve that topology.

```text
source object count
!= Omniphony canonical lane count
!= render-shell direction count
```

```text
hardware channel
!= speaker channel
!= canonical scene lane
!= render-shell direction
```

Do not convert source objects into static `L/R/C/...` PCM lanes merely because Omniphony uses those names.

## Provenance law: AUTHORED / DERIVED / EMPTY

Spatial information has authority, not just values.

```text
AUTHORED
preserved by source / driver / device / format

DERIVED
chosen or inferred by musical / acoustic / perceptual presentation policy

EMPTY
no authored source fact exists for that dimension
```

`EMPTY` is a provenance state, **not an instruction that the audible presentation must leave that dimension unused**.

A Genesis PSG voice can have no authored rear or elevation coordinate and still receive a stable `DERIVED` rear/elevation placement in Omniphony `FullSphere`. What must never happen is relabeling that placement as historical source truth.

Retro VGM Compiler's `spatial_evidence_authority` is the source-side evidence vocabulary. Omniphony's `AUTHORED`, `DERIVED` and `EMPTY` states are presentation-side provenance. Their concrete representations may differ, but their meaning must not blur.

Examples:

- YM2612 left/right enable is authored route evidence;
- stock Genesis PSG voice identity is source truth but not authored azimuth;
- bass/foundation classification is derived musical evidence;
- S-DSP echo send is authored send state, not an independent reverberant source position;
- actual source-supplied 3-D coordinates may be authored position;
- a FullSphere position seeded from stable source identity is a derived immersive mix decision.

Never promote a derived coordinate to authored because it is stable, plausible or musically effective.

## Presentation modes

Omniphony owns two useful source-aware modes:

```text
NativeRouting
→ recovered real source objects
→ native laterality + source identity
→ no creative rear / elevation / extra depth

FullSphere
→ same recovered real source objects
→ preserve authored route / authored position constraints
→ stable identity-aware creative placement
→ width + rear depth + height + distance + extent
→ 8.1.4.4 world → 22-direction shell → binaural
```

`FullSphere` is a production mode, not an evidence-confidence threshold.

In FullSphere:

- stable source or persistent-part identity may seed repeatable creative placement;
- native L/R routing constrains side and must not be casually inverted;
- foundation material is strongly anchored;
- foreground material resists excessive rear/depth displacement;
- diffuse/support evidence may increase rear, depth and extent;
- vertical-affinity evidence may strengthen elevation;
- shared wet material may occupy a broad environmental field;
- otherwise neutral real sources are still allowed useful spatial separation.

The listening criterion is not "did every position exist in the original hardware?" It is "does this sound coherently mixed into a larger format while preserving the music?"

## Source lane semantics

`model/spatial_source.h` distinguishes:

```text
dry_source
shared_effect_return
reference_mix
```

### Dry source

A dry source lane is a causal localizable source witness when the source family can actually expose one.

It may carry physical slot identity, source episode identity, native stereo route, effect-send state, persistent musical-part identity, inferred presentation evidence and genuinely authored position.

A dry lane is not automatically an additive stem. Cross-source coupling, feedback, nonlinear arithmetic or hidden shared state may prevent exact recomposition even when useful isolated source audio exists.

### Shared effect return

A shared effect return is a distinct observable wet field produced by common processing.

Keep it shared. Omniphony may make it broad, diffuse, rearward or elevated as presentation, but must not duplicate one historical return into N invented per-source wet stems.

### Reference mix

The protected reference mix is the scientific and audible control. It is not an object lane and must never receive object geometry or persistent-object memory.

## Device-family authority examples

These examples describe the architectural boundary, not a promise that every runtime path is already fully implemented.

| Source family / device | Source-native truth to preserve | Authored spatial/routing evidence | FullSphere may derive |
| --- | --- | --- | --- |
| YM2612 / Genesis FM | six complete FM channels; channel-6/DAC distinction where applicable; causal state/audio | native L/R enables and timing | width, rear depth, elevation, distance, extent |
| Genesis PSG | three tone voices + noise identity/timing | ordinary stock Genesis PSG has no independent stereo pan register | essentially the complete immersive placement |
| Game Gear PSG | three tone voices + noise identity/timing | explicit per-channel L/R routing | remaining immersive dimensions |
| YM2151 / OPM | eight complete FM channels preserving operator network and channel evolution | authored channel L/R enables | width, rear depth, elevation, distance, extent |
| SNES S-DSP dry path | eight bounded dry voice witnesses where proven | signed per-voice L/R route + echo-send state | unauthored width/depth/height/extent |
| SNES S-DSP shared echo | final shared wet return where observable | shared-wet identity + DSP provenance | diffuse/environmental immersive field |

### FM operators are not spatial objects by default

For YM2612, YM2151 and related FM systems, the source object is the **complete audible channel**, not an individual operator.

```text
FM operator
!= independent musical source
```

Algorithms, modulation and feedback make operators synthesis internals of one audible voice unless a future source representation explicitly proves otherwise.

### Exact source witness is renderer-specific

A higher-fidelity whole-chip renderer does not automatically provide exact independent stems.

```text
whole-chip fidelity improvement
!= proven independent source decomposition
```

Shared mixer/DAC paths, clamps or other coupling require an additivity / causal-decomposition witness before independent enhanced lanes can be called exact.

## Musical projection

`model/realtime_musical_spatial_projection.h` maps already-learned role support into:

```text
foundation
foreground
diffuse
width
vertical_affinity
confidence
```

The projection supplies **evidence and constraints**, not final geometry.

- foundation can anchor a source;
- foreground can preserve near/front importance;
- environmental evidence can increase diffusion and extent;
- transient-accent evidence is not itself a role or coordinate;
- vertical affinity is a derived tendency, not historical elevation;
- authored 3-D coordinates remain separate and untouched.

Crucially, absence of a positive role classification does not veto FullSphere. The user's explicit immersive-mix choice is itself permission for Omniphony to use stable creative geometry. Musical evidence then shapes that geometry.

## Direct/localizable versus diffuse/shared energy

```text
localizable direct source objects
!= shared / diffuse historical wet field
!= Omniphony externalization field
```

Do not collapse everything to stereo before spatial rendering, and do not pretend shared reverberation is a collection of independent point sources.

This distinction is consistent with spatial-audio work separating direct/localizable energy from diffuse/late fields. Relevant anchors include Menzies et al. (2021, DOI `10.1109/TASLP.2020.3036781`), Landschoot & Jot (2023, DOI `10.1121/10.0018389`) and Greco et al. (2025, DOI `10.1186/s13636-025-00437-y`).

## Immersive source extent

Source width/extent is a valid production dimension, not merely a localization error.

Research on apparent source width and spatially extended sources links perceived extent to interaural coherence/correlation and supports controlled decorrelation or spatial covariance methods for making sources perceptually broader while retaining a stable center.

Useful anchors include:

- Ziemer, *Source Width in Music Production* (2017), DOI `10.1007/978-3-319-47292-8_10`;
- Potard & Burnett, *Decorrelation techniques for the rendering of apparent sound source width in 3D audio displays* (2004);
- McCormack, Politis & Pulkki, *Rendering of Source Spread for Arbitrary Playback Setups Based on Spatial Covariance Matching* (2021), DOI `10.1109/WASPAA52581.2021.9632724`;
- Anemüller, Thiergart & Habets, *Binaural Rendering of Heterogeneous Sound Sources with Extent* (2024), DOI `10.1109/ICASSP48485.2024.10448024`.

The implementation consequence is important: carrying a `size` field into object metadata is not enough if the active direct-binaural path ignores it. Source extent is complete only when the binaural renderer turns that extent into an audible, controlled spatial-width mechanism.

## Temporal stability and confidence

Spatial presentation is tracked state, not a new random coordinate every callback.

```text
stable source / persistent-part identity
→ stable creative baseline

stable evidence
→ stable evidence-shaped presentation
```

FullSphere may use stable identity to seed creative placement even before a strong role classifier exists. Callback size, transient spectral noise and weak role fluctuations must not reshuffle the mix.

Continuity must not become glue. Authored route/position changes, source replacement or strong new evidence may legitimately move an object.

Recommended behavior:

- preserve persistent-part identity when earned;
- keep confidence separate from position;
- let evidence age rather than disappear instantly;
- use onsets/transients as high-information update moments only when other evidence supports a change;
- use bounded inertia for derived motion;
- keep authored timed route/position evidence exact;
- reset pose continuity when an unrelated source reuses a physical lane.

## No semantic feedback loop

```text
raw source evidence
→ past-only projection
→ Omniphony

raw source evidence + completed PCM
→ observer / role update
→ future memory
```

Never feed `handoff.projected_view()` into `complete_block()`. A previous presentation hypothesis must not become evidence for itself.

## Timed events

Retro VGM Compiler supports exact intra-block evidence changes:

```text
frame_offset + lane_index + new evidence
```

Omniphony ABI 0.3 carries the same event form. A change at frame 137 remains a change at frame 137. Derived motion may be perceptually ramped afterward, but authored evidence timing is not quantized for convenience.

## Transport

`model/omniphony_source_transport.h` is the allocation-free compiler-side ABI mirror.

It:

- rejects protected reference-mix lanes;
- carries native stereo-route evidence without relabeling it as 3-D geometry;
- carries strong persistent-part identity when earned;
- carries authored position only when supplied;
- carries musical presentation evidence;
- preserves ordered intra-block evidence events;
- interleaves causal source lanes into caller-owned scratch;
- refuses to manufacture source PCM where source frames are unavailable.

The C++ transport and Rust `repr(C)` records pin ABI 0.3. The existence of 8.1.4.4 or FullSphere does not require ABI expansion by itself.

Retro VGM Compiler retains exact provenance internally. The ABI's 64-bit runtime token is renderer-local presentation identity, not a compiler provenance key.

## Identity continuity

```text
same persistent musical part
→ ordinary smooth presentation continuity may continue

unrelated source reuses same lane
→ do not ramp from outgoing source pose
```

Presentation identity is committed only after successful rendering.

## Reset / seek lifecycle

`realtime_musical_omniphony_pipeline::reset()` clears:

```text
compiler acoustic observer state
compiler musical-role / time memory
pending projected handoff state
Omniphony binaural/spatial runtime state
Omniphony source-presentation identity state
```

A track change, seek or decoder restart is one causal-timeline boundary across both projects.

## DSP ownership

There is one audible spatial owner: Omniphony.

Retro VGM Compiler supplies causal source truth plus musical/perceptual evidence. Omniphony chooses the final presentation geometry and performs the binaural rendering.

`model/realtime_spatial_scene_dsp.h` remains an executable reference for control laws and tests, not a second audible spatializer in front of Omniphony.

## Cross-project ownership

```text
Retro VGM Compiler
  owns source truth
  owns source-native identity / timing / route / send evidence
  owns source-quality selection and reference-vs-enhanced validation
        ↓
Omniphony
  owns creative presentation geometry
  owns NativeRouting / FullSphere policy
  owns canonical 8.1.4.4 semantic world
  owns 22-direction render shell
  owns binaural / externalization / distance / extent rendering
```

Omniphony must not decide which emulator/source reconstruction is more truthful. Retro VGM Compiler must not pre-render a competing spatial world.

## deepSTRF boundary

The active artificial-hearing reference is `dissonance-git/deepSTRF`. It remains a research teacher, not a playback dependency.

```text
Retro VGM Compiler source-authoritative fixtures
→ deepSTRF auditory obligations / adversarial experiments
→ compress mechanism
→ only causal bounded survivors enter realtime playback
```

## Validation obligations

At minimum defend:

```text
SOURCE TRUTH
recovered source identity / timing / route remains intact

NO AUTHORITY PROMOTION
creative/inferred geometry never becomes authored geometry

NATIVE CONTROL
NativeRouting disables creative rear / height / extra depth

IMMERSIVE EXPANSION
FullSphere gives neutral real sources deterministic larger width/depth/height

AUTHORED ROUTE SURVIVAL
native L/R constraints survive into presentation

SHARED-WET INTEGRITY
one shared return never becomes N fabricated wet stems

IDENTITY CONTINUITY
persistent parts remain spatially stable across physical voice reuse

BLOCK INVARIANCE
creative placement does not depend on host callback size

SOURCE-OBJECT BOUNDARY
FM operators are not independent spatial objects by default

FAIL-CLOSED DECOMPOSITION
whole-chip fidelity does not imply independent additive stems

REFERENCE PROTECTION
the protected reference control remains available

EXTENT AUDIBILITY
source `size` must reach an actual binaural extent/spread mechanism before extent is called implemented
```

Useful adversarial fixtures include Genesis PSG versus Game Gear PSG, YM2612 exact pan events, YM2151 channel-vs-operator boundaries, SNES dry voices plus one shared wet return, source-lane reuse by unrelated musical parts and the same source scene rendered at several block sizes.

## What is finished versus what remains empirical

```text
source-native truth
→ causal source objects
→ authored + derived evidence with provenance
→ exact timed transport
→ Omniphony NativeRouting / FullSphere
→ 8.1.4.4 world + dynamic objects
→ 22-direction shell
→ binaural DSP
```

Format/source-extraction coverage remains upstream work. A format without trustworthy causal source audio cannot be promoted by guessing stems.

Engineering success, perceptual success and listening preference remain separate evidence states:

```text
code compiles / tests pass
!= source/reference correctness
!= block invariance
!= perceptual mechanism validated
!= physical listening quality improved
```

The transport, timing, identity, reset and source-authority mechanisms are established. Creative mix tuning, coordinated ensemble placement, audible direct-binaural source extent and personalized listening remain active empirical work.
