# NCSF SDAT and paired representation

## Question and boundary

What does NCSF version `0x25` preserve, and what can a same-work Mario Kart DS
NCSF/2SF pair compare without erasing their different representations?

```text
NCSF envelope + dependency order
→ effective SDAT + explicit sequence selection
→ bounded INFO/FAT/SSEQ/SBNK/SWAR/PLAYER structure
→ runtime unavailable
```

NCSF is not treated as 2SF. An NCSF SDAT is not presumed to be the original
complete cartridge SDAT: the current converter deliberately strips and
rewrites SDAT content for selected sequences.

## Pinned observatories

| Observatory | Commit | Evidential role |
| --- | --- | --- |
| `CyberBotX/NCSF` | `fe1b91afec25fe18a10fe1697f95341e8dd5a44d` | Current creator/player, SDAT stripping, selection, INFO/FAT and PLAYER behavior |
| `CyberBotX/in_xsf` | `74fceae1f09f2e42afff4f71fb68b1952f494916` | Independent historical NCSF mapping/control |
| `CyberBotX/SDATStuff` | `bdd988df4644a21c5eab0e6bec9c21de10060cfb` | Archived predecessor and historical structure control |
| `fincs/FSS` | `c0f69d6105e8877ca8ff3b929d230e49e05726c7` | Historical DS sequence/player lineage |
| `pret/pokediamond` | `038cccaed5de8f013875bc5d734f912d1de08e0f` | Reconstructed same-platform sequence implementation observatory |
| `vgmtrans/vgmtrans` | `083f7c71fe773078061eb785573621082c3e0d1c` | Independent SDAT/SSEQ/SBNK/SWAR structural parser |

## Observed contract

Current `CyberBotX/NCSF` creation emits either:

- one `.ncsf` with a four-byte little-endian sequence index and an SDAT;
- or one `.ncsflib` with empty reserved data and an SDAT plus `.minincsf`
  roots with a four-byte index and no program.

The project rejects other reserved lengths. A non-empty program must have
`SDAT FF FE 00 01`, a supported `0x40` header, a declared size equal to the
effective bytes, and bounded INFO/FAT sections. For the selected sequence it
exposes:

- total SEQ record count and selected index;
- selected SSEQ FAT reference;
- INFO volume, priorities, bank index, and player index;
- SBNK and referenced SWAR FAT objects;
- PLAYER presence, authored channel mask, effective/default mask, max-sequence
  count, and heap size.

PLAYER absence remains absent. The current player fallback of `0xFFFF` is
exposed separately as an effective default. An authored mask of zero is also
preserved before the same compatibility fallback.

## Mario Kart DS paired control

No separately published Mario Kart DS NCSF package was found in the inspected
JoshW archive namespace; the available Mario Kart DS package is 2SF. Rather
than substitute another game or call a derived set independent, this change
uses the already admitted Mario Kart DS 2SF control.

`01 - Main Menu.mini2sf` (SHA-256
`2179020a72608df37811c5b3ce73dc4edabb7349511ab47032d5b96e59a14832`)
resolves to one structurally valid SDAT at effective ROM offset `0xE919C`.
That 3,321,568-byte range has SHA-256
`6fce2fe1580d1fbb492475d0a3d8efd3537da55b87ad3e4d46cf601ac2cfa171`
and contains 76 valid SEQ INFO entries. Its exact bytes were placed in one
NCSF library; 76 roots carry only indices `0..75` plus tags.

The paired comparison names observables:

| Observable | 2SF | NCSF | Relation |
| --- | --- | --- | --- |
| Container version | `0x24` | `0x25` | representation-different |
| Effective SDAT bytes | hash above | hash above | exact within bounded range |
| Selected sequence index | not exposed by bounded 2SF loader | explicit `0..75` | not comparable |
| Machine runtime | unavailable | unavailable | same availability, not audio equivalence |

The pair controls same work and exact SDAT bytes. It does not establish equal
selection state, machine execution, driver trajectory, voices, or audio.
