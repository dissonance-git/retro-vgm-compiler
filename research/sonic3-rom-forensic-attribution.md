# Sonic 3 ROM forensic attribution

Status: active evidence lane for the Sonic 3 primary testbed  
Related: `docs/sonic3-primary-testbed.md`, `research/sonic3-composer-programmer-attribution.md`

## Question

Can Sonic 3 / Sonic & Knuckles preserve authorship or production provenance **inside the ROM or its binary organization**, independently of musical-style inference?

Some games retain filenames, symbolic labels, source paths, build strings, tool signatures, author initials, chunk names, or other development residue that can expose who created an asset. Even when literal names are stripped, the binary layout can preserve weaker provenance signals such as contiguous import batches, bank boundaries, pointer-table ordering, duplicated assets, alignment/padding conventions, shared patch/sample libraries, or compiler/exporter fingerprints.

This is worth testing directly rather than assuming either outcome.

## Critical distinction: reconstructed source names are not ROM evidence

The Sonic Retro SMPS research repository states that its song data are raw binaries extracted from ROMs, while the repository itself applies filenames following a pattern such as `01 Song Title...`. Therefore those filenames are research/archive labels, not evidence that the original retail ROM contained those names.

Likewise, reconstructed source projects expose labels such as:

```text
Music01: include "Sound/Music/AIZ1.asm"
Music02: include "Sound/Music/AIZ2.asm"
Music03: include "Sound/Music/HCZ1.asm"
...
```

and symbolic IDs such as `mus_AIZ1`, `mus_HCZ2`, etc. These are useful disassembly labels but must not be mistaken for literal strings or symbols preserved in the shipped ROM unless byte-level evidence independently proves that they were embedded.

Current searches of the major Sonic 3/S&K disassembly repositories found no obvious literal strings containing candidate composer names such as Tatsuyuki Maeda, Masayuki Nagao, Masaru Setsumaru, Brad Buxer, Jun Senoue, Howard Drossin, Miyoko Takaoka, Masanori Hikichi, Yoshiaki Kashima, or Tomonori Sawada.

That is **not yet proof of absence**. It only means the visible reconstructed source does not currently expose an obvious composer-name residue.

## Forensic evidence hierarchy

Treat possible ROM clues in descending evidential strength.

### A. Direct embedded provenance

Strongest possible evidence:

```text
literal composer / arranger / programmer name
initials with independent contextual support
original filename or source path
asset label naming a creator
build manifest residue
tool-generated author field
```

These should be preserved byte-for-byte with ROM offsets and surrounding context.

### B. Tool/export signatures

Potential evidence:

```text
converter signature
compiler/exporter marker
version string
source-format residue
characteristic padding or serialization header
embedded path fragment
```

A tool signature may identify a workflow or programmer, not the composer. Historical role provenance remains mandatory.

### C. Structural production provenance

When names are stripped, binary topology may still preserve how music entered the build:

```text
music pointer-table order
contiguous song blocks
bank boundaries
alignment / padding runs
shared local instrument banks
Universal Voice Bank versus local voices
sample-bank grouping
modulation / envelope-table grouping
song IDs and duplicated slots
prototype/final relocation
identical binary regions across games/builds
```

This evidence can support hypotheses such as “these assets were imported or processed together” without proving that they were composed by the same person.

### D. Statistical binary fingerprints

Lowest-level candidate evidence:

```text
compression choices
padding byte distributions
pointer locality
sequence packing density
call/loop factoring patterns
local table ordering
unused bytes / dead data
```

These are especially vulnerable to toolchain and programmer confounds and should never become direct composer evidence by themselves.

## Sonic 3-specific opportunities

### Music table ordering

The reconstructed driver exposes a stable numbered music table (`Music01`, `Music02`, ...), with zone music and miscellaneous cues occupying identifiable slots.

The important forensic question is not the modern labels. It is whether the **underlying pointer order and storage order in the ROM** preserve historical batches.

Examples to test:

- Are tracks now attributed to the same contributor unusually contiguous in pointer or storage order?
- Do prototype replacements occupy old slots while new data move elsewhere?
- Are Buxer/Jackson-team replacements clustered physically even when their sound-test IDs are not?
- Do Act 2 arrangements added by Sega staff sit near other work by the same arranger/programmer?
- Does Sonic & Knuckles append or reorganize material in a way that reflects production chronology?

Any apparent cluster must be compared against simpler explanations such as stage order, sound-test order, bank-size constraints, or build-system packing.

### Bank topology

Sonic 3/S&K music crosses banked Z80 storage and a shared sound-driver environment.

Test whether:

```text
same creator hypothesis
↔ same ROM bank / import block
```

is stronger than expected by chance after controlling for song ID, game half, zone order, and size.

If so, the interpretation is initially **production-batch evidence**, not composer proof.

### Shared voice / envelope resources

Universal Voice Bank use, local FM voices, PSG envelopes, DAC/sample organization, and modulation tables may reveal which songs entered through the same realization workflow.

This can be useful for separating:

```text
composition provenance
from
arrangement / sound-data programming provenance
from
shared project library
```

### Prototype/final residue

Prototype and retail ROMs may preserve lineage through:

- unchanged pointers;
- overwritten slots;
- orphaned/unused song data;
- duplicated cues;
- retained old instruments/envelopes;
- changed bank placement;
- new data appended rather than replacing old data in place.

These are natural clues about development chronology.

### Cross-game binary matches

Compare Sonic 3/S&K regions against source-available or ripped data from other candidate-contributor soundtracks.

Exact or near-exact matches can identify:

- reused patches;
- reused envelopes;
- reused driver tables;
- reused sequences;
- reused source material;
- shared tooling.

A binary match must be typed by what matched. A shared driver table is not a shared composition. A copied sequence is much stronger work-lineage evidence.

## Blind-attribution leakage rule

ROM-forensic evidence is valuable historically but dangerous during a musical-understanding benchmark.

Therefore the testbed must run two modes:

```text
MUSICAL BLIND MODE
exclude filenames, tags, disassembly labels, ROM textual residue,
known song IDs where they leak cue identity, and attribution metadata

FORENSIC MODE
allow byte layout, strings, paths, tool signatures, pointer topology,
bank placement, and other provenance evidence
```

Do not let forensic shortcuts inflate the composer-understanding score.

A model that correctly identifies a composer because an embedded filename says their initials has solved a provenance problem, not a musical-understanding problem.

But the same clue is extremely useful as **independent validation evidence** after the musical result is frozen.

## Proposed ROM-forensics audit

Create a deterministic tool that consumes legally supplied / locally available ROM images or exact extracted binary regions and emits only derived metadata, never redistributing copyrighted ROM bytes.

Suggested output:

```json
{
  "rom_sha256": "...",
  "strings": [
    {"offset": "0x...", "text": "...", "classification": "path|name|tool|other"}
  ],
  "music_table": [
    {"id": 1, "pointer": "0x...", "length": 0, "bank": 0}
  ],
  "regions": [
    {
      "start": "0x...",
      "end": "0x...",
      "kind": "sequence|voice_bank|envelope|driver|unknown",
      "hash": "..."
    }
  ],
  "adjacency": [],
  "exact_cross_build_matches": [],
  "near_match_candidates": [],
  "claim_boundary": "binary provenance only; no automatic composer inference"
}
```

## Statistical controls

If binary layout seems to correlate with the curated attribution hypotheses, compare it against null models that preserve obvious build structure.

Useful permutations:

```text
shuffle labels within zone-music slots
shuffle within prototype/final strata
shuffle within Sonic 3 vs Sonic & Knuckles halves
shuffle within ROM bank
shuffle within similar sequence-size bins
```

A composer cluster that vanishes under a bank-preserving null is probably a bank/toolchain artifact.

## Evidence integration

ROM evidence enters the attribution model through typed provenance coordinates:

```text
embedded_name
source_path_residue
tool_signature
binary_region_identity
storage_adjacency
bank_membership
shared_resource_identity
prototype_lineage
```

Each coordinate must state the historical role it can support.

Examples:

```text
embedded source filename naming person
→ potentially strong documentary/provenance evidence

shared local FM bank
→ arrangement/programming/workflow evidence

same ROM bank
→ weak production-batch evidence

same pointer neighborhood
→ weak production chronology evidence
```

None of these silently converts to `composer = X`.

## Immediate status

Current evidence does **not** establish that retail Sonic 3/S&K contains literal original song filenames or composer names.

What is established so far:

1. the visible song filenames in `smps-rips` are repository naming conventions applied to raw ROM-extracted binaries;
2. the visible `AIZ1.asm`, `HCZ2.asm`, `mus_AIZ1`, etc. names are reconstructed source labels and cannot be treated as embedded ROM strings without byte evidence;
3. no obvious candidate-composer name strings were found in the major reconstructed Sonic 3/S&K source repositories during the initial search;
4. the numbered music table, banked storage, prototype/final lineage, resource sharing, and physical binary organization remain promising forensic surfaces worth auditing.

The absence of an obvious filename leak actually makes the subtler question more interesting: **does the topology of the ROM preserve the topology of the production process?**
