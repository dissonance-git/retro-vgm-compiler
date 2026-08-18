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
→ past-only soundtrack mix budget
→ handoff.projected_view()
→ Omniphony ABI 0.4 mix-budget setter
→ omniphony_source_transport_storage::build()
→ caller-owned interleaved source scratch
→ Omniphony ABI 0.4 timed-event call
→ binaural output
→ realtime_musical_spatial_frontend::complete_block(raw block)
```

`prepare_block()` may consult only already-completed musical and scene history. `complete_block()` runs only after the renderer accepts the block and receives the original raw source block, never the projected view.

```text
same observed history through frame N
→ same source presentation and scene budget through frame N
```

A renderer failure does not advance musical memory or the adaptive scene budget.

## Product intent: source-native immersive remix

The source-aware Surround path is not limited to forensic reconstruction of the speaker dimensions that old hardware could encode.

Its audible target is:

> **Recover the real musical sources, then present them as though the soundtrack had always been mixed for a larger immersive format.**

That is a modern presentation choice, not a historical-authorship claim.

Retro VGM Compiler remains strict about what the source actually did. Omniphony is deliberately free to use otherwise unauthored width, rear depth, elevation, extent and distance when `FullSphere` presentation is selected.

The closest production analogy is an immersive remix made from multitracks. Here the "multitracks" are reconstructed causal chip/DSP source objects rather than studio stems.

The protected reference path remains available underneath the enhancement.

## Making it work across different soundtracks

A universal fixed geometry is the wrong abstraction.

```text
universality
!= every soundtrack sounds spatially the same
```

The stable target is the quality law:

```text
preserve musical identity / hierarchy / impact / clarity
+ recover trustworthy source structure
+ spend only the spatial capacity the current arrangement can support
```

Different soundtracks can therefore reach the same "large immersive mix" goal by different routes.

```text
sparse dry FM / PSG
→ individual sources may be wider and farther apart
→ more depth / height capacity
→ more optional Omniphony externalization

dense layered soundtrack
→ tighter objects
→ use depth hierarchy rather than indiscriminate width
→ preserve transient separation

echo-heavy SPC
→ S-DSP echo carries much of the envelopment
→ reduce additional Omniphony room
→ keep dry voices legible

bass / percussion heavy
→ keep foundation compact and stable
→ spend more scale on accompaniment / environmental layers
```

This is **adaptive allocation, not adaptive taste**. The runtime does not need to classify a cue as ambient, orchestral, techno, battle music, or anything else before making first-order safety decisions.

## Causal scene mix budget

`model/realtime_spatial_mix_budget.h` converts source-agnostic scene measurements into a slowly varying renderer intervention budget.

The current observer already measures completed audio in terms of:

```text
observed source count
active source count
mean activity
energy concentration
low-band energy ratio
edge / transient ratio
shared-effect energy share
```

From those values the budget exposes:

```text
dry_width_scale
dry_diffuse_scale
depth_scale
height_scale
shared_wet_strength
shared_wet_extent
added_externalization_scale
```

The first two are per-dry-object presentation scalars. Depth, height, shared-wet treatment and added room live on the separate ABI 0.4 renderer-control plane.

No genre, game, composer or soundtrack identity participates in this calculation.

### Time behavior

The budget is tracked in seconds rather than host calls.

Current defaults are approximately:

```text
contraction: 0.30 s
expansion:   1.50 s
```

If the scene abruptly gets denser, wetter, bass-heavier or more transient-rich, added treatment can contract promptly. When the scene opens, the presentation expands more slowly.

That asymmetry prevents a quiet gap or one sparse callback from making the whole world breathe outward and inward like an effect pedal.

### Causality

Block N uses only the budget learned through blocks before N.

```text
past completed scene
→ mix budget for block N
→ render block N
→ if render succeeds, observe raw block N
→ update budget for block N+1
```

Current-block PCM never spatializes itself retroactively.

A failed render does not advance the budget. A seek, track change or decoder reset returns it to neutral.

## Canonical source-to-scene contract

The game-music path does **not** redefine Omniphony's product scene.

Omniphony owns a canonical **8.1.4.4 semantic scene** with 17 static lane names:

```text
L R C LFE Ls Rs Lb Rb Cb
Tfl Tfr Tbl Tbr
Bfl Bfr Bbl Bbr
```

and a separate 22-direction full-sphere render shell above that scene.

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
DERIVED PER-SOURCE EVIDENCE
musical role • directness/diffuseness • width tendency • vertical affinity • confidence
        ↓
PAST-DERIVED SCENE BUDGET
how much immersive capacity is safe to spend
        ↓
OMNIPHONY ABI 0.4
causal source objects + exact timed evidence + scene control
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

> **Retro VGM Compiler tells Omniphony what the instrument actually did and how the completed mix is behaving. Omniphony decides how those real sources inhabit the immersive world.**

Richer source truth reduces uncertainty about the source. It does not remove Omniphony's permission to make an explicit creative mix decision.

## 8.1.4.4 is vocabulary, not forced source width

A source-aware render may expose six FM channels, eight OPM channels, four PSG voices, eight S-DSP voices, one shared wet field, or another source-native topology. Preserve that topology.

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

The scene mix budget is also `DERIVED`, but it is not source metadata at all. It is renderer intervention state.

Examples:

- YM2612 left/right enable is authored route evidence;
- stock Genesis PSG voice identity is source truth but not authored azimuth;
- bass/foundation classification is derived musical evidence;
- S-DSP echo send is authored send state;
- S-DSP final echo return is a real recovered shared field;
- actual source-supplied 3-D coordinates may be authored position;
- a FullSphere position seeded from stable source identity is a derived immersive mix decision;
- "reduce added room because source-native echo is already strong" is a derived scene-budget decision.

Never promote a derived coordinate or budget to authored source truth.

## Presentation modes

```text
NativeRouting
→ recovered real source objects
→ native laterality + source identity
→ no creative rear / elevation / extra depth

FullSphere
→ same recovered real source objects
→ preserve authored route / position constraints
→ stable identity-aware creative placement
→ adaptive width + rear depth + height + distance + extent
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
- shared wet material occupies its own environmental layer;
- the scene budget limits how aggressively the renderer uses those freedoms;
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

## SPC: echo as its own spatial layer

SNES is the clearest example of why dry and wet should remain separate.

The current source-native model can preserve:

```text
8 dry S-DSP voice witnesses
+ authored signed per-voice L/R routing
+ authored echo-send state
+ final post-EVOL shared echo L/R
```

The final echo is represented as **two linked left/right components of one shared stereo feedback field**. It is not two independent effect objects and it must not be copied into eight fabricated wet stems.

This gives the immersive mixer independent control over:

```text
dry voice placement / hierarchy
shared echo rear bias
shared echo elevation
shared echo radial depth
shared echo presentation strength
shared echo eventual audible extent
Omniphony added externalization room
```

That last distinction is important:

```text
historical S-DSP echo
!= modern Omniphony externalization
```

A soundtrack with a strong, characteristic S-DSP echo field can use that field as much of its immersive glue and ask for less generic added room. A dry SPC can do the opposite.

The source record itself remains untouched. `diffuse=1,width=1` on the recovered wet field can remain a statement about what kind of source lane it is; the adaptive amount applied to that field lives separately in the ABI 0.4 scene budget.

## Device-family authority examples

| Source family / device | Source-native truth to preserve | Authored spatial/routing evidence | FullSphere may derive |
| --- | --- | --- | --- |
| YM2612 / Genesis FM | six complete FM channels; channel-6/DAC distinction where applicable | native L/R enables and timing | width, rear depth, elevation, distance, extent |
| Genesis PSG | three tone voices + noise identity/timing | ordinary stock Genesis PSG has no independent stereo pan register | essentially the complete immersive placement |
| Game Gear PSG | three tone voices + noise identity/timing | explicit per-channel L/R routing | remaining immersive dimensions |
| YM2151 / OPM | eight complete FM channels preserving operator network and channel evolution | authored channel L/R enables | width, rear depth, elevation, distance, extent |
| SNES S-DSP dry | eight bounded dry voice witnesses where proven | signed per-voice L/R route + echo-send state | unauthored width/depth/height/extent |
| SNES S-DSP shared echo | final shared stereo wet field where observed | shared field identity + native L/R + DSP provenance | environmental rear/height/depth/extent presentation |

## FM operators are not spatial objects by default

For YM2612, YM2151 and related FM systems, the source object is the **complete audible channel**, not an individual operator.

```text
FM operator
!= independent musical source
```

Algorithms, modulation and feedback make operators synthesis internals of one audible voice unless a future source representation explicitly proves otherwise.

A higher-fidelity whole-chip renderer also does not automatically provide exact independent stems.

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

The projection supplies **per-source evidence and constraints**, not final geometry and not the scene-wide intervention budget.

- foundation can anchor a source;
- foreground can preserve near/front importance;
- environmental evidence can increase diffusion and extent;
- transient-accent evidence is not itself a role or coordinate;
- vertical affinity is a derived tendency, not historical elevation;
- authored 3-D coordinates remain separate and untouched.

Absence of a positive role classification does not veto FullSphere. The explicit immersive-mix choice is permission for Omniphony to use stable creative geometry. Musical evidence and the adaptive scene budget then shape it.

## Direct/localizable versus diffuse/shared energy

```text
localizable direct source objects
!= shared / diffuse historical wet field
!= Omniphony externalization field
```

Do not collapse everything to stereo before spatial rendering, and do not pretend shared reverberation is a collection of independent point sources.

This distinction is consistent with spatial-audio work separating direct/localizable energy from diffuse/late fields. Relevant anchors include Menzies et al. (2021, DOI `10.1109/TASLP.2020.3036781`), Landschoot & Jot (2023, DOI `10.1121/10.0018389`) and Greco et al. (2025, DOI `10.1186/s13636-025-00437-y`).

## Research support for adaptive immersive mixing

The architecture uses research to define obligations rather than to claim that one numeric preset is optimal.

- Jot, Carpentier and Warusfel's perceptually motivated scene work supports independent production dimensions such as position, distance, presence and reverberance.
- Landschoot & Jot (2023, DOI `10.1121/10.0018389`) supports object-aware externalization.
- Ziemer (2017, DOI `10.1007/978-3-319-47292-8_10`) treats source width as a music-production dimension.
- McCormack, Politis & Pulkki (2021, DOI `10.1109/WASPAA52581.2021.9632724`) provides a fidelity-constrained covariance approach for spread sources.
- Anemüller, Thiergart & Habets (2024, DOI `10.1109/ICASSP48485.2024.10448024`) studies binaural rendering of sources with spatial extent.

The common lesson is useful here: localization, width/extent, direct/reverberant balance and environmental envelopment are separable controls. A robust system can allocate them according to scene structure rather than maximizing all of them at once.

## Immersive source extent

Source width/extent is a valid production dimension, but the implementation is not complete merely because `SourcePresentation.size` exists.

The active Omniphony direct-binaural path currently consumes object world position and gain but does not yet convert object `size` into a physical binaural spread mechanism.

Therefore:

```text
azimuth / rear placement / elevation / radial depth
→ audible today

size metadata
→ carried today

true direct-binaural apparent source extent
→ still an implementation frontier
```

A future implementation should preserve a stable localizable center while widening the apparent event, likely with bounded multi-direction HRTF rendering or a fidelity-constrained decorrelation/covariance technique. It must not sacrifice transient impact or introduce obvious combing/colouration merely to make the source wider.

## Temporal stability and confidence

Spatial presentation and the scene budget are tracked states, not fresh guesses every callback.

```text
stable source / persistent-part identity
→ stable creative baseline

stable completed scene
→ slowly varying intervention budget
```

Callback size, transient spectral noise and weak role fluctuations must not reshuffle the mix.

Continuity must not become glue. Authored route/position changes, source replacement or strong new evidence may legitimately move an object.

The scene budget contracts faster than it expands. Authored timed evidence remains exact even when derived renderer motion and the budget are perceptually smoothed.

## No semantic feedback loop

```text
raw source evidence
→ past-only role projection
→ past-only scene budget
→ Omniphony

raw source evidence + completed PCM
→ observer / role / budget update
→ future memory
```

Never feed `handoff.projected_view()` into `complete_block()`. A previous presentation hypothesis or adaptive intervention must not become evidence for itself.

## Timed events

Retro VGM Compiler supports exact intra-block evidence changes:

```text
frame_offset + lane_index + new evidence
```

Omniphony ABI 0.4 retains the ABI 0.3 timed-event form. A change at frame 137 remains a change at frame 137. Derived motion may be perceptually ramped afterward, but authored evidence timing is not quantized for convenience.

## ABI 0.4 transport

`model/omniphony_source_transport.h` is the allocation-free compiler-side ABI mirror.

ABI 0.4 keeps the existing source-evidence record and timed-event model and adds a separate scene-control record:

```text
OmniphonySourceMixBudgetV1
  depth_scale
  height_scale
  shared_wet_strength_scale
  shared_wet_extent_scale
  externalization_scale
```

The transport/client:

- rejects protected reference-mix lanes;
- carries native stereo-route evidence without relabeling it as 3-D geometry;
- carries strong persistent-part identity when earned;
- carries authored position only when supplied;
- carries musical per-source presentation evidence;
- preserves ordered intra-block evidence events;
- carries scene-wide mix intervention separately from source evidence;
- interleaves causal source lanes into caller-owned scratch;
- refuses to manufacture source PCM where source frames are unavailable.

The adaptive client requires ABI minor 0.4. An older 0.3 DLL is rejected rather than silently dropping the soundtrack-adaptive scene control.

Before every render block the client sends the already-smoothed past-derived budget. If the setter fails, that block does not render under stale geometry.

Retro VGM Compiler retains exact provenance internally. The ABI's 64-bit runtime source token is renderer-local presentation identity, not a compiler provenance key.

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
compiler soundtrack mix-budget tracker
pending projected handoff state
Omniphony binaural/spatial runtime state
Omniphony source-presentation identity state
Omniphony scene mix budget → neutral
```

A track change, seek or decoder restart is one causal-timeline boundary across both projects.

## DSP ownership

There is one audible spatial owner: Omniphony.

Retro VGM Compiler supplies causal source truth, musical/perceptual evidence and the past-derived scene intervention budget. Omniphony chooses final presentation geometry and performs binaural rendering.

`model/realtime_spatial_scene_dsp.h` remains an executable reference for control laws and tests, not a second audible spatializer in front of Omniphony.

## Cross-project ownership

```text
Retro VGM Compiler
  owns source truth
  owns source-native identity / timing / route / send evidence
  owns source-quality selection and reference-vs-enhanced validation
  owns causal completed-scene observation
  owns the source-agnostic intervention budget
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
creative/inferred geometry and scene budget never become authored geometry

NATIVE CONTROL
NativeRouting disables creative rear / height / extra depth

IMMERSIVE EXPANSION
FullSphere gives neutral real sources deterministic larger width/depth/height

SOUNDTRACK ADAPTATION
same aesthetic target can spend spatial capacity differently by completed scene structure

NO LOOKAHEAD
block N cannot alter its own scene budget

FAILURE TRANSACTION
render failure does not advance role or budget memory

RESET ISOLATION
new track/seek returns the adaptive budget to neutral

AUTHORED ROUTE SURVIVAL
native L/R constraints survive into presentation

SHARED-WET INTEGRITY
one shared return never becomes N fabricated wet stems

SPC ECHO LAYER
post-EVOL L/R stay linked components of one historical shared field

ROOM COMPETITION
source-native wet can reduce added Omniphony externalization without changing source evidence

IDENTITY CONTINUITY
persistent parts remain spatially stable across physical voice reuse

BLOCK INVARIANCE
creative placement and budget trajectory do not depend on host callback size

SOURCE-OBJECT BOUNDARY
FM operators are not independent spatial objects by default

FAIL-CLOSED DECOMPOSITION
whole-chip fidelity does not imply independent additive stems

REFERENCE PROTECTION
the protected reference control remains available

EXTENT AUDIBILITY
source `size` must reach an actual binaural extent/spread mechanism before extent is called implemented
```

Useful adversarial fixtures include Genesis PSG versus Game Gear PSG, YM2612 exact pan events, YM2151 channel-vs-operator boundaries, SNES dry voices plus linked shared wet L/R, dry versus echo-heavy SPC, source-lane reuse by unrelated musical parts and the same source scene rendered at several block sizes.

## What is finished versus what remains empirical

```text
source-native truth
→ causal source objects
→ authored + derived per-source evidence
→ past-derived adaptive mix budget
→ ABI 0.4 exact timed transport + scene control
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

The transport, timing, identity, reset, source-authority and adaptive-budget mechanisms are now represented in code. Creative mix tuning, direct-binaural source extent, coordinated ensemble placement and physical listening across a broad soundtrack corpus remain empirical work.
