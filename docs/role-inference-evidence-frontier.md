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

A second bounded bridge exists for perceptual evidence:

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

A third bridge now connects source-backed part-local phrase evidence directly into the same role descriptor:

```text
persistent-part gestures
        ↓
part-local timing + motif phrase discovery
        ↓ independent phrase grounding
phrase_boundary_participation
        +
structural_motif_prominence
        ↓
bounded musical-role inference
```

Phrase participation is admitted as independent role corroboration only when the phrase hypothesis is cross-domain grounded or exactly authored-grounded. A motif-derived phrase boundary by itself is not allowed to corroborate the same motif a second time under a different field name. Timing-only phrase evidence remains below the role-use threshold and is not promoted into an independently grounded participation signal.

The same bridge is now exercised through both Genesis and SPC source-backed gesture adapters. Their physical clocks and native pitch representations remain distinct, but both can feed persistent-part gesture evidence through motif, phrase, and shared role inference without a source-family role shortcut.

Genesis route transport also preserves higher evidence across route-only YM2612 pan and Game Gear PSG stereo writes. A pan-register update changes `stereo_route`; it is not evidence that persistent-part identity, presentation state, effect membership, or other source semantics disappeared.

## Evidence rules

`part_role_gesture_descriptor` separates recurrence and phrase claims:

- `rhythmic_repetition` remains bounded by motif identity confidence. Rhythm-only recurrence therefore inherits the motif layer's rhythm-only identity ceiling instead of being promoted by a perfect timing match.
- `structural_motif_prominence` is emitted only when motif identity is grounded by comparable pitch evidence or comparable resolved performed-trajectory shape.
- `phrase_boundary_participation` is generated from the same persistent part's ordered gestures only when the phrase layer has independent cross-domain or authored grounding.
- motif-only phrase boundaries may remain valid phrase hypotheses, but they cannot serve as an independent discriminator for the same motif's role.
- motif discovery thresholds must respect the persistent-part identity ceiling. A test or caller cannot demand motif identity above the evidence confidence that licensed the persistent part in the first place and still expect a structural motif to exist.

The existing role kernel then applies its own evidence requirements. In particular, melodic foreground requires structural motif evidence plus an independent foreground discriminator such as phrase-boundary participation or auditory salience. Register and activity are corroborative rather than role-creating evidence.

Current realtime acoustic foreground evidence remains deliberately weak: the realtime proposer caps raw-acoustic foreground confidence at 0.15. Binding that evidence to the correct persistent part preserves the cap rather than upgrading it. A high-level foreground role therefore still requires stronger independent perceptual evidence or another independent musical discriminator.

Presentation-prior evidence is rejected by the auditory-to-role adapter. Feeding a role-derived presentation prior back into structural role inference would create a semantic feedback loop rather than a second evidence domain.

This gives the compiler an explicit epistemic ladder:

```text
performed motion
!= motif identity
!= structural prominence
!= phrase participation
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
4. A motif-only phrase boundary cannot self-corroborate the same motif into a melodic-foreground role.
5. A repeated motif aligned with independent performance-timing phrase evidence produces bounded `phrase_boundary_participation`.
6. Structural motif prominence plus that independently grounded phrase participation can enter the existing melodic-foreground inference path without a manually injected phrase flag.
7. Genesis source-backed persistent-part gestures recover the same motif + timing phrase relation and feed the shared role inference path.
8. SPC source-backed persistent-part gestures recover the analogous relation under their native device-time and pitch representation and feed the same role inference path.
9. Motif discovery thresholds in the cross-architecture control now respect the persistent-part identity confidence that bounds motif identity.
10. A spatial source can bind to an already-materialized persistent musical part without changing or conflating its implementation source ID.
11. Auditory foreground evidence attaches only to the matching persistent part and inherits the weaker of auditory and part-identity confidence.
12. Current raw-acoustic salience remains below the high-level role-use threshold even when the acoustic score itself is large.
13. Presentation-prior feedback is rejected as non-independent evidence.
14. Genesis YM2612 and PSG route-only writes preserve an established persistent-part binding instead of reconstructing the source record and erasing it.

The focused validators were intentionally temporary. The permanent semantic suite still needs to register `tests/model/part_role_gesture_descriptor_test.cpp`, `tests/model/part_role_auditory_evidence_adapter_test.cpp`, and `tests/model/cross_architecture_phrase_discovery_test.cpp` so these boundaries cannot silently regress. The Genesis route-preservation regression should likewise be registered with the host/VGM transport tests.

A separate existing test, `tests/model/part_role_window_inference_test.cpp`, is also not registered in the permanent semantic CMake suite and currently exposes a stale standalone counterline-confidence assertion. That test should not be promoted until its expected confidence is reconciled with the current role-evidence policy for a documented semantic reason.

## Next evidence target

The phrase bridge is no longer merely a manually supplied role input: source-backed Genesis and SPC persistent-part gestures can now produce the independent phrase evidence consumed by role inference.

The next useful step is to widen independent evidence convergence rather than add another role classifier.

Preferred order:

1. stronger auditory salience derived independently from the existing auditory/realtime analysis path without presentation-prior feedback;
2. harmonic bass ownership where harmonic evidence is already trustworthy;
3. cross-part register and spacing as corroborative orchestration evidence;
4. longer role continuity across adjacent phrase/section windows so a part can retain or transfer musical function through time.

Genesis and SPC remain the strongest immediate proving grounds because both already expose source-backed persistent-part gesture evidence. The next proving step should require two or more independent musical evidence families to agree on the same real source-derived part window.

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
