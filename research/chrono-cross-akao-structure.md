# Chrono Cross AKAO structural frontier

## Question

What can Game Music Interpreter establish about *Chrono Cross* AKAO data directly from reconstructed PSF1 memory before CPU/SPU runtime execution exists?

This pass narrows the claim boundary rather than treating VGMTrans labels as hardware truth.

## Independent structural observatory

Pinned VGMTrans revision used for the initial comparison:

- `vgmtrans/vgmtrans` `083f7c71fe773078061eb785573621082c3e0d1c`

Its AKAO parser supplies exact byte-structure hypotheses that can be tested against reconstructed PSF1 memory without adopting VGMTrans's MIDI projection.

For later AKAO sequences the observable structural surface includes:

```text
"AKAO" signature
+0x04 sequence ID
+0x06 declared sequence length
+0x14 associated sample-set ID
+0x20 track-allocation bitmap
+0x28/+0x2C/+0x38/+0x3C zero-valued structural fields
+0x30 instrument-data relative pointer
+0x34 drumkit relative pointer
+0x40 track-pointer table
```

VGMTrans's tagless `guessVersion()` is weaker than exact version attribution: a zero word at `+0x2C` is sufficient for its `VERSION_3_2` guess. Because later structural validation already expects that field to be zero, Game Music Interpreter must not turn this heuristic into exact version proof.

The new bounded probe therefore reports:

```text
v3-compatible; VGMTrans-style 3.2 heuristic only
```

rather than:

```text
proven AKAO 3.2
```

Game/title provenance remains separate supporting evidence.

## Executable probe

`components/psf/akao_probe.py` now scans a reconstructed `Psf1EffectiveImage` for literal AKAO signatures and classifies them without CPU or SPU execution.

It distinguishes:

```text
literal AKAO signature
sequence candidate
zero-length/non-sequence signature
rejected structural candidate
```

A zero declared sequence length is not promoted to a sequence because VGMTrans also uses AKAO signatures for sample collections.

Accepted sequence candidates preserve:

- reconstructed memory address;
- declared length;
- sequence ID;
- associated sample-set ID;
- track bitmap/count;
- bounded track targets;
- instrument/drumkit targets;
- warnings;
- raw `FE 13` byte-pair count.

The probe explicitly does not claim:

- exact AKAO version;
- complete event decoding;
- runtime use of the object;
- driver allocation behavior;
- SPU correspondence;
- authored source recovery.

## Important correction: AKAO event name != SPU mechanism

VGMTrans labels AKAO opcode `0xC6` as:

```text
FM (Pitch LFO) On
```

and later exposes separate events named:

```text
Pitch Side Chain On/Off
Pitch-Volume Side Chain On/Off
```

The latter are currently emitted as generic state events rather than demonstrated SPU operations.

Independent PlayStation SPU evidence shows a hardware pitch-modulation mechanism in which the preceding physical voice can modulate the current voice's sample step.

The similarity of names is insufficient to equate these layers.

Therefore:

```text
AKAO pitch-related event
!= proven SPU PMON transition
```

until runtime tracing demonstrates the mapping.

This is a correction to the tempting earlier hypothesis, not a dead end. It makes the future test sharper:

```text
AKAO event occurrence
-> driver state transition
-> voice allocation
-> SPU register transition
-> evolving effective sample step
```

Each arrow must be observed.

## Chrono Cross-specific unresolved semantics

VGMTrans itself preserves useful uncertainty in later AKAO.

For version 3.2 it marks sub-opcode `FE 13` as a Chrono Cross-specific unknown and explicitly cites three tracks:

- `114 Shadow Forest`
- `119 Hydra Marshes`
- `302 Chronopolis`

All three are present in the permanent Chrono Cross PSF corpus.

VGMTrans also notes that the operation behind later `FE 0B` appears to differ in *Chrono Cross* from earlier implementations.

These are unusually valuable adversarial fixtures because the strongest available parser does not pretend to know their semantics.

The project should preserve this state as:

```text
bytes known
structural location potentially recoverable
semantic operation unknown
runtime effect unobserved
```

not as an invented MIDI/control event.

## Raw byte-pair warning

The structural probe counts raw `FE 13` pairs only as a search clue.

```text
raw bytes FE 13 inside a declared sequence span
!= decoded FE 13 event
```

The pair may occur inside arguments, data reached under different control flow, or other non-event positions.

A future event walker must follow AKAO's actual instruction widths, pattern calls, loops, conditional branches, and track boundaries before promoting a raw pair to an event occurrence.

## Tests earned so far

Synthetic regression coverage protects these boundaries:

1. accept a bounded v3-compatible sequence structure without claiming exact version;
2. do not promote a zero-length AKAO signature to a sequence;
3. reject nonzero later-v3 reserved fields;
4. reject track targets outside the declared sequence span;
5. count `FE 13` only as a raw clue;
6. retain suspicious track targets into the pointer-table region as warnings rather than silently repairing them;
7. keep the corpus audit's structural/runtime/SPU claim states separate.

The corrected structural probe suite passed 6/6 in the local synthetic check performed during this pass. GitHub-hosted CI remains unavailable because the repository workflow is manually disabled from normal pushes while the account runner spending limit blocks jobs.

## Next bounded experiment

Run `tools/psf_akao_audit.py` over the 68 committed Chrono Cross PSF roots from a checkout with direct binary access.

The output should answer only:

- how many literal AKAO signatures are visible in each reconstructed effective image;
- how many satisfy the bounded v3 structural invariants;
- sequence/sample-set IDs and track counts;
- whether the three adversarial cues contain raw `FE 13` clues;
- which signatures are rejected and why.

After that, build a bounded event walker around one accepted object, beginning with known-width control flow and preserving every unimplemented opcode.

Do not jump directly to MIDI.

## Stop conditions

Stop rather than guess if:

- a VGMTrans version heuristic is reported as exact provenance;
- a literal `AKAO` signature is automatically called a sequence;
- raw `FE 13` is automatically called an executed event;
- an unknown Chrono-specific opcode is assigned a musical meaning by analogy;
- `0xC6` or a side-chain label is equated with SPU pitch modulation without runtime evidence;
- a structurally valid object is assumed to have executed in the PSF runtime;
- parser output erases unresolved control flow or game-specific behavior.

Correction outranks coherence.
