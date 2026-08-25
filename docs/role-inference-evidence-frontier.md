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

A third bridge connects source-backed part-local phrase evidence directly into the same role descriptor:

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

A fourth bridge earns stronger auditory salience from a genuinely relational perceptual question rather than by increasing the confidence of the old loudness heuristic:

```text
stable causal dry source
        +
simultaneously active dry competitors
        ↓
relative energy + activity + edge structure
        +
coarse spectral distinctiveness
        +
source continuity + measurement coverage
        ↓
relational auditory salience
        ↓ persistent-part identity cap
auditory_salience on the matching part window
```

The original one-source acoustic foreground proposer remains capped at 0.15 confidence. The relational analyzer has its own bounded ceiling of 0.70 because it uses multi-source contrast plus continuity, but it still does not call a source "melody." It can only corroborate independent structural evidence after exact source identity is joined to a persistent musical part.

A fifth bridge now derives harmonic bass ownership from repeated, identity-grounded harmonic interaction rather than from instantaneous register:

```text
successive harmonic verticalities
        +
persistent-part voice correspondence
        ↓
bass / harmony interaction
        ↓ repeated informative transitions
harmonic_bass_ownership
        ↓
bass_foundation role evidence
```

The lowest sounding pitch is not automatically a bass role, and the sounding bass is not automatically the harmonic root. Inversions are therefore first-class evidence rather than exceptions. Static bass continuity under retained harmony can contribute to ownership continuity and coverage, but it does not mature harmonic-role confidence by itself. Harmonic changes, inversions, pedal-bass-under-change, moving bass under retained upper material, and other informative interactions can mature the claim. One transition remains too weak, duplicate transition spans are rejected, and unresolved transitions reduce coverage confidence.

A sixth bridge now assembles source-backed Genesis performed pitch into a changing harmonic surface:

```text
source-backed YM2612 operator-network pitch interpretation
        ↓ absolute performed Hz while interpretation remains valid
persistent-part absolute pitch spans
        ↓ starts + already-sounding overlaps
performed harmonic verticality timeline
```

The timeline does not interpolate or invent notes. It emits a new evidence boundary whenever a bounded performed-pitch span starts or ends. Pitches already sounding continue to participate beside newly attacked pitches. Span ends are exclusive: a pitch ending at tick T is no longer sounding at T, while a successor beginning at T is. Rearticulation can therefore create a new provenance boundary even when the frequency relation is unchanged. A YM2612 state change that invalidates the source-backed operator-network interpretation terminates absolute-pitch authority immediately rather than letting stale harmonic evidence continue.

The phrase bridge is exercised through both Genesis and SPC source-backed gesture adapters. Genesis additionally has a source-backed absolute performed-pitch path into harmonic verticalities. SPC still deliberately stops one rung earlier: S-DSP playback rate is a relative pitch control, not performed acoustic Hz, unless the underlying sample root tuning is independently established.

Genesis route transport also preserves higher evidence across route-only YM2612 pan and Game Gear PSG stereo writes. A pan-register update changes `stereo_route`; it is not evidence that persistent-part identity, presentation state, effect membership, or other source semantics disappeared.

## Evidence rules

`part_role_gesture_descriptor` separates recurrence and phrase claims:

- `rhythmic_repetition` remains bounded by motif identity confidence. Rhythm-only recurrence therefore inherits the motif layer's rhythm-only identity ceiling instead of being promoted by a perfect timing match.
- `structural_motif_prominence` is emitted only when motif identity is grounded by comparable pitch evidence or comparable resolved performed-trajectory shape.
- `phrase_boundary_participation` is generated from the same persistent part's ordered gestures only when the phrase layer has independent cross-domain or authored grounding.
- motif-only phrase boundaries may remain valid phrase hypotheses, but they cannot serve as an independent discriminator for the same motif's role.
- motif discovery thresholds must respect the persistent-part identity ceiling. A test or caller cannot demand motif identity above the evidence confidence that licensed the persistent part in the first place and still expect a structural motif to exist.

The existing role kernel then applies its own evidence requirements. In particular, melodic foreground requires structural motif evidence plus an independent foreground discriminator such as phrase-boundary participation or auditory salience. Register and activity are corroborative rather than role-creating evidence. Bass foundation can use harmonic bass ownership only after that ownership has been earned from repeated, informative, persistent-part-grounded harmonic interaction.

Current raw realtime acoustic foreground evidence remains deliberately weak: the realtime proposer caps one-source acoustic foreground confidence at 0.15. More samples of that same heuristic do not remove the systematic ambiguity between a loud melody and loud accompaniment.

`realtime_relational_auditory_salience` asks a different question. It requires an exact source/generation observation, at least one simultaneously active dry competitor with a usable coarse spectrum, and stable source continuity. Its score combines scene-relative energy ownership, activity, edge structure, and spectral distinctiveness; its confidence is bounded by source age and measurement coverage. A source that is loud but spectrally blended with its competitor remains below the high-level role-use threshold in the current regression control.

Presentation-prior evidence is rejected by the auditory-to-role adapter. Feeding a role-derived presentation prior back into structural role inference would create a semantic feedback loop rather than a second evidence domain.

For harmony, absolute pitch authority remains representation-specific. Genesis YM2612 performed Hz is admitted only while the source-backed operator-network interpretation is valid. SPC `pitch_rate` remains relative to the selected BRR/source sample and cannot be relabeled as Hz without independent sample-root tuning plus source continuity.

This gives the compiler an explicit epistemic ladder:

```text
performed motion
!= motif identity
!= structural prominence
!= phrase participation
!= musical role

physical/source id
!= persistent musical-part identity
!= raw acoustic salience
!= relational auditory salience
!= musical role

lowest sounding pitch
!= harmonic bass ownership
!= chord root

relative playback rate
!= sample root tuning
!= performed absolute pitch
!= harmonic function
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
9. Motif discovery thresholds in the cross-architecture control respect the persistent-part identity confidence that bounds motif identity.
10. A spatial source can bind to an already-materialized persistent musical part without changing or conflating its implementation source ID.
11. Auditory foreground evidence attaches only to the matching persistent part and inherits the weaker of auditory and part-identity confidence.
12. Raw one-source acoustic salience remains below the high-level role-use threshold even when its score is large.
13. Presentation-prior feedback is rejected as non-independent evidence.
14. Genesis YM2612 and PSG route-only writes preserve an established persistent-part binding instead of reconstructing the source record and erasing it.
15. A stable, energetic source that is spectrally distinct from an active dry competitor can earn bounded relational auditory salience strong enough to corroborate structural motif evidence.
16. An equally energetic source with the same broad spectral profile as its competitor remains below the role-use threshold.
17. A newly appeared source remains weak until source continuity earns measurement confidence.
18. A solo source cannot claim relational salience because there is no contemporaneous competitor against which to establish contrast.
19. The same persistent part can own the sounding bass through root position, first inversion, second inversion, and a harmonic change without equating bass pitch class to chord root.
20. One grounded bass/harmony transition is too little history to establish a bass-foundation role.
21. Repeated retained/static-harmony continuity alone does not mature harmonic-bass role confidence; informative harmonic transitions are required.
22. Duplicate harmonic transition spans cannot manufacture evidence maturity, and unresolved transitions reduce coverage confidence.
23. Source-backed YM2612 absolute performed-pitch spans assemble into a dynamic harmonic verticality timeline that includes both newly attacked and already-sounding parts.
24. A performed pitch change on one part changes the harmonic verticality while another part sustains.
25. Rearticulation can preserve the same frequency relation while creating a distinct source/provenance boundary.
26. A YM2612 topology change terminates the affected absolute-pitch interpretation; later two-part harmony is not emitted from stale evidence.
27. Simultaneous contradictory pitch claims for one persistent part fail closed during harmonic-timeline construction.

The known-green role evidence guards are permanently registered in the root CMake/CTest suite: `tests/model/part_role_gesture_descriptor_test.cpp`, `tests/model/part_role_auditory_evidence_adapter_test.cpp`, `tests/model/cross_architecture_phrase_discovery_test.cpp`, and `tests/vgm/genesis_spatial_semantic_preservation_test.cpp`. Hosted validation configured the normal CMake graph, built those targets through the repository's strict compiler policy, and ran their registered CTest entries successfully. The relational-auditory change was then validated through the same registered auditory/role target alongside the phrase-role controls.

The newer harmonic-bass and Genesis performed-harmony controls have also passed strict hosted focused validation, including the retained-harmony negative control, but `tests/model/part_role_harmonic_bass_evidence_adapter_test.cpp`, `tests/model/part_role_harmonic_bass_evidence_adapter_research_test.cpp`, and `tests/vgm/genesis_performed_harmony_adapter_test.cpp` still need permanent CMake/CTest registration.

A separate existing test, `tests/model/part_role_window_inference_test.cpp`, remains intentionally unregistered because it currently exposes a stale standalone counterline-confidence assertion. That test should not be promoted until its expected confidence is reconciled with the current role-evidence policy for a documented semantic reason.

## Next evidence target

Genesis can now carry source-backed absolute performed pitch into a dynamic harmonic surface, and repeated informative harmonic interactions can contribute to bass-foundation role inference. The sharpest remaining cross-architecture gap is SPC absolute pitch.

SPC currently preserves exact runtime sample identity, device-native `pitch_rate`, persistent musical-part identity, and relative performed-pitch trajectories. What it does not yet have is permission to turn those ratios into performed Hz across different samples. The missing evidence is the root/fundamental tuning of the exact sample version being played.

Preferred order:

1. introduce an explicit sample-root tuning evidence contract that separates authored/exact tuning from auditory-estimated tuning and unresolved cases;
2. combine validated sample-root tuning with exact S-DSP playback rate and persistent-part identity to produce bounded absolute performed-pitch observations;
3. admit only those absolute observations into the generic harmonic verticality timeline;
4. add confidence-bearing sample F0 estimation for cases with no authored tuning, with voiced/unvoiced and ambiguity controls rather than a one-shot spectral peak;
5. then require real SPC windows to converge across phrase, harmonic, relational-auditory, and role evidence.

## Validation rule

A successful next SPC step should prove an end-to-end chain of the form:

```text
exact BRR/sample version identity
        +
independently earned sample-root tuning
        +
exact S-DSP playback rate
        +
persistent musical-part identity
        ↓
bounded absolute performed pitch
        ↓
performed harmonic verticality
```

If sample tuning is absent, ambiguous, non-pitched, contradicted, or too weak, absolute harmony must remain unresolved. Relative pitch evidence may still support motif and performed-shape reasoning, but it may not impersonate Hz.

For every source family, if an independent signal is absent, incompatible, circular, or too weak, the higher claim must remain absent or weak. No source family gets a special shortcut.
