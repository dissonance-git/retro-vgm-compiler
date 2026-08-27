# SPC cross-voice handoff null

Status: **31-cue real-runtime null + global data-association falsification + pinned N-SPC driver boundary**

## Question

Can a persistent musical part be recovered across two different S-DSP physical
voice episodes from runtime evidence alone?

The starting candidate used evidence that is individually legitimate:

```text
same event-time BRR source
+ temporal adjacency
+ source-relative pitch continuity
```

but physical voice identity was deliberately absent.

The question was not whether those facts make a handoff plausible. The question
was whether they discriminate a true handoff from competing assignments in real
polyphonic SPC execution.

## Evidence boundary

The experiment keeps these layers separate:

```text
physical S-DSP voice episode
!= N-SPC logical sequence track
!= persistent musical part
!= auditory stream
```

The control never reads ID666 creator metadata, catalog labels, or answer-bearing
track names. The 31-cue panel is the existing preregistered creator-blind panel:

- `research/projects/sonic3/spc-cube-blind-panel.json`

The runtime path is the pinned accurate SNESAPU forensic continuation owned by
`tools/spc/forensic/`.

## Research projections

### Musical voice separation

The adjacent MIR quarry was symbolic musical voice separation, especially the
link-prediction / multi-trajectory framing used by:

- Manos Karystinaios, Francesco Foscarin, and Gerhard Widmer,
  "Musical Voice Separation as Link Prediction: Modeling a Musical Perception
  Task as a Multi-Trajectory Tracking Problem" (2023);
- the public implementation observatory `manoskary/vocsep_ijcai2023`.

The useful transfer was structural, not evidential:

```text
pairwise similarity
-> candidate link

persistent identity
-> constrained trajectory / competing-link problem
```

This did not add a new SPC evidence domain.

### Multi-target tracking

The distant quarry used data-association / track-switch machinery, including the
public `IzouGend/MCMCDA` implementation. Its useful warning was that a locally
plausible source-target link can be wrong when another global assignment explains
the same observations.

Again, this changed the falsifier, not the evidence confidence.

### Other field projections

No chemistry, quantum, or unrelated physical analogy supplied a lawful
identity-bearing projection. Those routes were treated as `no_valid_projection`
rather than forced into the model.

## Pairwise ablation

The first real-corpus ablation showed that the source/timing/pitch bundle was
not selective across physical voices. Some fixtures produced a 100% apparent
cross-voice strong-link rate before the cross-voice ceiling was applied.

That failure earned the semantic repair in:

- `4cb60812a45900ef5af1863d5599369f6f6abaf3`
  `test(spc): ablate cross-voice part evidence`
- `9ede6c82f703921ce97e3f487ce6c2c8edc761b8`
  `fix(spc): cap unresolved cross-voice handoffs`

Unresolved cross-voice hypotheses are capped at `0.74`, below the shared
trajectory-link threshold `0.75`, unless a genuinely independent handoff
witness is recovered.

The rest of this experiment tried to falsify that conservative ceiling. It did
not relax it.

## Recursive reduction

The canonical executable falsifier is:

- `tests/spc/spc_part_selectivity_control.cpp`
- `.github/workflows/spc-runtime-corpus-pressure.yml`

The 31-cue panel produced this reduction:

```text
2,078
pairwise cross-voice bundles with
source identity + temporal adjacency + pitch continuity

        ↓ one-in / one-out competition among cross-voice links

34
bidirectionally unique cross-voice associations

        ↓ require strong same-voice trajectory context on both sides

14
two-sided unique associations

        ↓ require the complete 0.5 s association horizon
          to be visible on both sides of the capture

14
boundary-safe two-sided unique associations

        ↓ detect synchronized directed assignment cycles

12
belong to synchronized voice-swap cycles

2
remain non-cyclic

        ↓ admit strong ordinary same-voice predecessor/successor
          edges into the same data-association problem

0
uncontested cross-voice handoffs
```

The final full-graph result was measured on commit:

- `f8eb27c6eb9d321e12f28ec762b9864d1d41f372`
  `test(spc): measure full-graph handoff competition`

Hosted run:

- `spc-runtime-corpus-pressure` run `33038738914`

Aggregate final observatory result:

```text
cue_count = 31
cues_with_bundle_candidates = 19
total_bundle_candidates = 2078
total_bidirectional_unique = 34
total_two_sided_unique = 14
total_boundary_safe_two_sided_unique = 14
total_synchronized_cycle_two_sided_unique = 12
total_noncyclic_two_sided_unique = 2
total_same_voice_competed_two_sided_unique = 14
total_uncontested_two_sided_unique = 0
total_boundary_safe_uncontested_two_sided_unique = 0
```

This is a negative result with positive architectural value: local runtime
geometry did not earn one uncontested cross-voice persistent-part transfer.

## Why local uniqueness failed

Exact edge diagnostics were preserved before the final reduction.

Twelve of the fourteen strongest candidates occurred as synchronized directed
voice permutations, including reciprocal pairs such as:

```text
1 -> 6
6 -> 1

2 -> 3
3 -> 2

4 -> 5
5 -> 4
```

and repeated `0 <-> 1` exchanges.

Those are exactly the kind of global assignment structures that pairwise link
logic can misread as independent handoffs.

The two non-cyclic residuals both occurred in one opaque Terranigma cue and both
had strong same-voice alternatives at their endpoints. Once those ordinary
continuations entered the same association graph, neither cross-voice edge
remained uncontested.

## Capture-boundary control

One-in/one-out uniqueness could have been fabricated by a short three-second
observation window. The experiment therefore required the complete association
horizon before trusting uniqueness.

The policy horizon is `0.5 s`. A candidate is boundary-safe only when:

```text
target start >= capture start + 0.5 s

and

source end <= capture end - 0.5 s
```

All 14 two-sided unique candidates survived that control, so capture truncation
did not explain the later null.

## Independent driver boundary

Runtime graph pressure was then checked against pinned driver source instead of
inventing a new runtime feature.

Pinned source observatory:

- `loveemu/vgm-disasm`
- commit `e96c5b35649f8e814cac3c31b65cedc07b52d76d`

The surviving cues span two documented N-SPC lineages:

- Ancient Magic and America Oudan Ultra Quiz: Cube N-SPC lineage;
- Terranigma / Tenchi Souzou: Quintet N-SPC lineage.

### Quintet

Pinned `snes/NSPC/Quintet/Tenchi Souzou.s` copies sixteen section bytes into
eight two-byte voice pointers, iterates those eight lanes with `x = 0,2,...14`,
and uses a DSP-register base table:

```text
$0000
$0010
$0020
$0030
$0040
$0050
$0060
$0070
```

The ordinary music path therefore ties the eight logical sequence lanes to the
eight S-DSP voice-register bases.

### Cube

Pinned `snes/NSPC/Cube/Ys IV - Mask of the Sun.s` independently loads eight
voice pointers and iterates `x = 0,2,...14`. Its instrument-register path
derives the S-DSP register base from that lane index, producing SRCN registers:

```text
$04
$14
$24
$34
$44
$54
$64
$74
```

The Cube ordinary music path therefore has the same fixed-lane property.

This independent source evidence prevents a tempting but invalid rescue:

```text
cross-voice runtime candidate
!= one recovered N-SPC logical track dynamically reassigned to another voice
```

A genuine cross-voice musical handoff could still be authored between two
distinct driver tracks, but that is a different claim and needs independent
evidence.

The broader parser quarry `vgmtrans/vgmtrans` likewise represents N-SPC as
eight sequence tracks. It is useful corroborating implementation context, not
proof of a musical handoff in any particular work.

## Result

Established by this pass:

- same BRR identity + timing + relative pitch is non-selective for cross-voice
  persistent identity;
- two-sided trajectory flanking is also common and correlated;
- local one-in/one-out graph uniqueness sharply reduces candidates but does not
  make them independent evidence;
- short capture boundaries do not explain the final 14;
- synchronized assignment cycles explain 12 of 14 strongest candidates;
- strong same-voice alternatives explain the remaining two;
- the 31-cue panel contains zero uncontested cross-voice handoffs under the
  current runtime evidence;
- Cube and Quintet ordinary N-SPC paths use fixed logical-track-to-voice lanes,
  so driver-track identity cannot be used to promote cross-voice candidates;
- the existing cross-voice confidence ceiling remains supported.

Not established:

- that cross-voice musical handoffs never occur in SPC music;
- that a fixed driver lane is the same thing as a persistent musical part;
- that two distinct authored sequence tracks can never participate in one
  perceptual or formal musical line;
- a generic driver-level logical-track provenance extractor;
- an authored relation between distinct N-SPC tracks for any of the rejected
  candidates;
- any basis for lifting the current `0.74` cross-voice ceiling.

## Re-entry handle

Do not stack another correlated runtime similarity feature onto the rejected
links.

The next positive route must cross an evidence-domain boundary. Suitable
candidates include:

1. a recovered authored sequence relation between distinct fixed driver tracks;
2. an independently grounded motif/phrase relation that predicts the transfer
   before looking at the cross-voice runtime link;
3. source or documentary evidence that explicitly encodes role transfer.

Any such witness must be tested against the same full-graph competitor null
before it can raise persistent-part confidence.

Correction outranks coherence.
