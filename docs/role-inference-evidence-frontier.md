# Role-inference evidence frontier

## Status

Retro VGM Compiler now has a bounded evidence path from performed pitch motion into musical-role reasoning:

```text
source-backed performed pitch trajectory
        ↓
resolved per-gesture performance shape
        ↓
motif identity comparison
        ↓
structural_motif_prominence
        ↓
role inference, only with independent corroborating evidence
```

This bridge deliberately does not treat performed motion as a role label. A glide, modulation shape, or other resolved trajectory can strengthen the identity of recurring musical material, but it cannot by itself establish foreground melody, accompaniment, counterline, or another role.

A second bounded bridge now exists for perceptual evidence:

```text
materialized persistent musical part
        ↓ explicit identity binding
spatial/source evidence
        +
independent realtime auditory foreground evidence
        ↓ confidence intersection
auditory_salience on the same part-role window
```

The source implementation ID is never treated as musical-part identity. The binding must come from an already-materialized `node_kind::part` whose `identity_scope` is `persistent_musical_part`, and the attached auditory confidence cannot exceed either the auditory hypothesis or that part-identity confidence.

Genesis route transport now preserves that higher evidence across route-only YM2612 pan and Game Gear PSG stereo writes. A pan-register update changes `stereo_route`; it is not evidence that persistent-part identity, presentation state, effect membership, or other source semantics disappeared.

## Evidence rules

`part_role_gesture_descriptor` currently separates two recurrence claims:

- `rhythmic_repetition` remains bounded by motif identity confidence. Rhythm-only recurrence therefore inherits the motif layer's rhythm-only identity ceiling instead of being promoted by a perfect timing match.
- `structural_motif_prominence` is emitted only when motif identity is grounded by comparable pitch evidence or comparable resolved performed-trajectory shape.

The existing role kernel then applies its own evidence requirements. In particular, melodic foreground requires structural motif evidence plus an independent foreground discriminator such as phrase-boundary participation or auditory salience. Register and activity are corroborative rather than role-creating evidence.

Current realtime acoustic foreground evidence remains deliberately weak: the realtime proposer caps raw-acoustic foreground confidence at 0.15. Binding that evidence to the correct persistent part preserves the cap rather than upgrading it. A high-level foreground role therefore still requires stronger independent perceptual evidence or another independent musical discriminator.

Presentation-prior evidence is rejected by the auditory-to-role adapter. Feeding a role-derived presentation prior back into structural role inference would create a semantic feedback loop rather than a second evidence domain.

This gives the compiler an explicit epistemic ladder:

```text
performed motion
!= motif identity
!= structural prominence
!= musical role

physical/source id
!= persistent musical-part identity
!= auditory role evidence
```

Each arrow must be earned by evidence.

## Current regression coverage

Focused validation has established the following behaviors:

1. Pitch-grounded repeated material produces structural motif prominence.
2. Rhythm-only recurrence does not impersonate structural motif evidence and remains under the rhythm-only identity ceiling.
3. Comparable performed-trajectory shape can ground structural motif prominence even when onset pitch is unavailable.
4. Performed-shape motif evidence alone does not create a melodic-foreground candidate.
5. The same structural motif evidence, combined with independent phrase-boundary participation, can enter the existing melodic-foreground inference path.
6. A spatial source can bind to an already-materialized persistent musical part without changing or conflating its implementation source ID.
7. Auditory foreground evidence attaches only to the matching persistent part and inherits the weaker of auditory and part-identity confidence.
8. Current raw-acoustic salience remains below the high-level role-use threshold even when the acoustic score itself is large.
9. Presentation-prior feedback is rejected as non-independent evidence.
10. Genesis YM2612 and PSG route-only writes preserve an established persistent-part binding instead of reconstructing the source record and erasing it.

The focused validators were intentionally temporary. The permanent semantic suite still needs to register `tests/model/part_role_gesture_descriptor_test.cpp` and `tests/model/part_role_auditory_evidence_adapter_test.cpp` so these boundaries cannot silently regress. The Genesis route-preservation regression should likewise be registered with the host/VGM transport tests.

A separate existing test, `tests/model/part_role_window_inference_test.cpp`, is also not registered in the permanent semantic CMake suite and currently exposes a stale standalone counterline-confidence assertion. That test should not be promoted until its expected confidence is reconciled with the current role-evidence policy for a documented semantic reason.

## Next evidence target

The next useful step is not another role classifier. The infrastructure can now carry a persistent-part identity through live Genesis route changes and attach independent perceptual evidence to the correct role window. The next target is to produce stronger independent evidence from real source-backed material.

Preferred order:

1. phrase-boundary participation derived from persistent-part phrase evidence;
2. stronger auditory salience derived independently from the existing auditory/realtime analysis path without presentation-prior feedback;
3. harmonic bass ownership where harmonic evidence is already trustworthy;
4. cross-part register and spacing as corroborative orchestration evidence.

Genesis and SPC remain the strongest immediate proving grounds because both already have source-backed persistent-part performed-trajectory adapters. The goal is to demonstrate converging evidence on real source-derived observations rather than synthetic role labels.

## Validation rule

A successful next step should prove an end-to-end chain of the form:

```text
native/source execution evidence
        ↓
persistent part + performed gesture evidence
        ↓
structural motif prominence
        +
independent phrase / salience / harmony evidence
        ↓
bounded role hypothesis
```

Identity must also survive ordinary runtime state changes:

```text
persistent-part binding
        ↓
route / pan / stereo writes
        ↓
same persistent-part binding + updated route
```

If the independent signal is absent, incompatible, circular, or too weak, the role claim must remain absent or weak. No source family gets a special shortcut.
