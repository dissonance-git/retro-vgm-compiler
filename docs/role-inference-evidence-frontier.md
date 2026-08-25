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

## Evidence rules

`part_role_gesture_descriptor` currently separates two recurrence claims:

- `rhythmic_repetition` remains bounded by motif identity confidence. Rhythm-only recurrence therefore inherits the motif layer's rhythm-only identity ceiling instead of being promoted by a perfect timing match.
- `structural_motif_prominence` is emitted only when motif identity is grounded by comparable pitch evidence or comparable resolved performed-trajectory shape.

The existing role kernel then applies its own evidence requirements. In particular, melodic foreground requires structural motif evidence plus an independent foreground discriminator such as phrase-boundary participation or auditory salience. Register and activity are corroborative rather than role-creating evidence.

This gives the compiler an explicit epistemic ladder:

```text
performed motion
!= motif identity
!= structural prominence
!= musical role
```

Each arrow must be earned by evidence.

## Current regression coverage

Focused validation has established the following behaviors:

1. Pitch-grounded repeated material produces structural motif prominence.
2. Rhythm-only recurrence does not impersonate structural motif evidence and remains under the rhythm-only identity ceiling.
3. Comparable performed-trajectory shape can ground structural motif prominence even when onset pitch is unavailable.
4. Performed-shape motif evidence alone does not create a melodic-foreground candidate.
5. The same structural motif evidence, combined with independent phrase-boundary participation, can enter the existing melodic-foreground inference path.

The focused bridge validator was intentionally temporary and was removed after a successful run. The permanent semantic suite still needs to register `tests/model/part_role_gesture_descriptor_test.cpp` so this evidence boundary cannot silently regress.

A separate existing test, `tests/model/part_role_window_inference_test.cpp`, is also not registered in the permanent semantic CMake suite and currently exposes a stale standalone counterline-confidence assertion. That test should not be promoted until its expected confidence is reconciled with the current role-evidence policy for a documented semantic reason.

## Next evidence target

The next useful step is not another role classifier. It is to feed a second independently grounded musical signal into the existing role descriptor on source-backed material.

Preferred order:

1. phrase-boundary participation derived from persistent-part phrase evidence;
2. auditory salience derived from the existing auditory/realtime analysis path;
3. harmonic bass ownership where harmonic evidence is already trustworthy;
4. cross-part register and spacing as corroborative orchestration evidence.

Genesis and SPC are the strongest immediate proving grounds because both already have source-backed persistent-part performed-trajectory adapters. The goal is to demonstrate converging evidence on real source-derived observations rather than synthetic role labels.

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

If the independent signal is absent or incompatible, the role claim must remain absent or weak. No source family gets a special shortcut.
