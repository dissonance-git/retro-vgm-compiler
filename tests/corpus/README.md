# Real-music corpus storage

This directory is the permanent repository home for user-supplied real VGM/VGZ/SPC/NSF/NSFe/PSF1/USF/2SF regression fixtures governed by [`tests/CORPUS.md`](../CORPUS.md).

Canonical layout:

```text
tests/corpus/
├── manifest.json
├── <corpus-id>.sha256
└── <corpus-id>/
    └── immutable runnable files
```

The runnable files themselves are the canonical committed evidence objects.
They are preserved byte-for-byte. Do not retag, normalize, recompress, rename
for metadata cleanup, or otherwise rewrite them.

`manifest.json` records corpus-level provenance and whole-set digests. The adjacent `.sha256` inventories record SHA-256 for every runnable file. Each set also records its expected Git tree SHA-1, so verification can detect path changes and byte drift at both the object and directory levels.

Use `tools/corpus_import.py` to verify the committed corpus:

```text
python tools/corpus_import.py --verify
```

NSF and NSFe are admitted as distinct executable/ripped NES source families.
They are not classified as VGM register logs. `tools/nsf_corpus_audit.py`
performs only container-level admission checks; it does not execute the rip or
validate playback.

PSF1, USF, and 2SF share an xSF envelope but not an execution model. The
common audit validates exact bytes, versions, outer compressed-program CRCs,
tags, dependency closure, deterministic overlay order, and byte provenance.
Platform-specific loaders reconstruct PS-X EXE memory, Nintendo 64
ROM/Project64 save-state patches, or Nintendo DS ROM/save maps respectively.
None of those effective objects is reported as a running machine, understood
driver/sequence, recovered voice/part set, or validated playback result.

To index a future direct-file corpus already placed under `tests/corpus/<id>/`:

```text
python tools/corpus_import.py --record-existing --id <id>
```

Archive import remains supported when an archive itself is intended to be preserved:

```text
python tools/corpus_import.py --archive soundtrack.zip --id <id>
```

An archive is a delivery container, not a mandatory duplicate of a direct-file corpus. If the runnable files are the canonical supplied objects, the repository does not need to retain a ZIP as well.

## Current committed pressure surface

| Corpus ID | Work | Source family | Declared device families | Fixtures |
| --- | --- | --- | --- | ---: |
| `antarctic-adventure-ay8910` | Antarctic Adventure | VGZ | AY8910 | 1 |
| `bucket-relay-champ-ymf262` | Bucket Relay Champ | VGZ | YMF262 | 5 |
| `cameltry-ym2610` | Cameltry | VGZ | YM2610 | 5 |
| `chrono-cross-psf1-playstation` | Chrono Cross | PSF1 | PlayStation machine runtime not yet executed | 68 |
| `dark-wizard-rf5c164` | Dark Wizard | VGZ | RF5C164 | 16 |
| `disc-station-ym2413` | Disc Station | VGM | YM2413 | 2 |
| `front-mission-gun-hazard` | Front Mission: Gun Hazard | SPC | SPC700, S-DSP | 61 |
| `fuusen-pentai-scc` | Fuusen Pentai | VGZ | K051649 | 3 |
| `jyangokushi-qsound` | Jyangokushi | VGZ | QSound | 5 |
| `magical-drop-okim6295` | Magical Drop | VGZ | MSM6295 | 3 |
| `mario-kart-ds-2sf-nintendo-ds` | Mario Kart DS | 2SF | Nintendo DS machine runtime not yet executed | 86 |
| `motocross-maniacs-game-boy` | Motocross Maniacs | VGZ | GameBoy DMG | 5 |
| `ocarina-of-time-usf-nintendo-64` | The Legend of Zelda: Ocarina of Time | USF | Nintendo 64 machine runtime not yet executed | 110 |
| `outrun-segapcm` | OutRun | VGZ | YM2151, SegaPCM | 4 |
| `ponpoko-namco-wsg` | Ponpoko | VGZ | C352 (converted from 3-voice Namco WSG) | 12 |
| `raimais-ym2610b` | Raimais | VGZ | YM2610B | 7 |
| `sonic-3-knuckles` | Sonic 3 & Knuckles | VGZ | YM2612, SN76489 | 58 |
| `star-parodier-huc6280` | Star Parodier | VGZ | HuC6280 | 3 |
| `star-soldier-nes-apu-vgm` | Star Soldier | VGZ | NES APU | 3 |
| `star-soldier-nsf` | Star Soldier | NSF | NES APU | 1 |
| `super-world-court-c140` | Super World Court | VGZ | C140 | 4 |
| `super-world-stadium-95-c352` | Super World Stadium '95 | VGZ | C352 | 7 |
| `tetris-s16-ym2151` | Tetris (Sega System 16) | VGM | YM2151 | 7 |
| `thexder-ym2203` | Thexder | VGZ | YM2203 | 2 |
| `title-fight-multipcm` | Title Fight | VGZ | MultiPCM | 2 |
| `truxton-ym3812` | Truxton | VGZ | YM3812 | 7 |
| `wanderers-from-super-scheme-ym2608` | Wanderers from Super Scheme | VGZ | YM2608 | 4 |

Total: 27 sets and 491 immutable real-music fixtures. The 2026-08-13
heterogeneous expansion added 22 sets and 108 fixtures. `manifest.json` records
the exact header clocks, VGM versions, command bytes, loop checks, whole-set
SHA-256 values, and Git tree identities for the new VGM/VGZ controls.

The xSF expansion added 264 exact runnable objects. Chrono Cross and Ocarina
of Time were downloaded with explicit user authorization; their source ZIP
hashes and URLs are recorded in `manifest.json`, while the ZIPs themselves are
not retained. Mario Kart DS came from the user's authorized local set.

## 2026-08-13 selection boundaries

The expansion searched the user's indexed local collection first and then
validated the selected bytes. Preferred-title substitutions were required:

- The Scheme was present only as rendered Opus audio, so Thexder and Wanderers
  from Super Scheme provide the YM2203 and YM2608 controls respectively.
- No local Tetris YM2610 or Ryu Jin YM2610B set was present; Cameltry and
  Raimais provide those controls.
- Blazeon was present only as SPC, so Tetris (Sega System 16) provides YM2151.
- The local Puyo Puyo Tsuu VGM set declares YM2612 plus SN76489, not YMF262;
  Bucket Relay Champ provides the single-device YMF262 control.
- No local NSF/NSFe object was found. With explicit user authorization, the
  Star Soldier NSF was downloaded from Zophar's Domain and its exact NSF bytes
  were retained; the source ZIP was hashed but not committed.

## Namco WSG representation boundary

`ponpoko-namco-wsg` preserves a compact 12-cue VGM pack whose music was written
for the three-voice Namco WSG on Pac-Man hardware. Standard VGM has no native
Namco WSG command support. The pack author reports extracting the music with a
reverse-engineered driver, checking it frame-for-frame against MAME register
dumps, and converting it to C352 for VGM playback.

The actual retained headers and commands therefore declare and drive C352, not
WSG. This is a source-hardware/converted-representation control, kept separate
from `super-world-court-c140` and the native C352 control
`super-world-stadium-95-c352`. It is not evidence of a native WSG command
timeline and does not establish WSG-to-C352 event or synthesis equivalence.

## Same-work NES cross-representation control

`star-soldier-nsf` and `star-soldier-nes-apu-vgm` preserve the same work as
different representations:

```text
NSF
program/data intended to execute and drive NES audio

VGZ
captured downstream NES APU device-command timeline
```

This is a related-rip control, not an equivalence claim. The corpus does not
assert byte identity, event identity, matching track boundaries/order, or
playback equivalence between the NSF and three VGZ files.

The corpus is intentionally committed to this private repository so future renderer, parser, execution, semantic, attribution-safeguard, and listening passes can exercise real files without depending on chat attachment lifetime.
