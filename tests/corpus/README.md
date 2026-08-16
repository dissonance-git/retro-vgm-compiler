# Real-music corpus storage

This directory is the permanent repository home for real VGM/VGZ/SPC/NSF/NSFe/PSF1/GSF/USF/2SF/NCSF regression fixtures governed by [`tests/CORPUS.md`](../CORPUS.md).

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

KSS and SGC are admitted as distinct executable-rip source families. Extended
M3U files are retained as canonical subsong-routing sidecars: they are hashed
and included in the Git tree identity but are not counted as runnable fixtures.
`tools/z80_rip_corpus_audit.py` checks observed container signatures and exact
playlist targets/selectors only. It does not execute the rip or validate
playback, timing, device behavior, or attribution.

PSF1, GSF, USF, 2SF, and NCSF share an xSF envelope but not an execution model. The
common audit validates exact bytes, versions, outer compressed-program CRCs,
tags, dependency closure, deterministic overlay order, and byte provenance.
Platform-specific loaders reconstruct PS-X EXE memory, GBA uploads, Nintendo 64
ROM/Project64 save-state patches, Nintendo DS ROM/save maps, or selected SDAT
structures respectively.
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

## Earlier committed pressure surface (through 2026-08-13)

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
| `mario-kart-ds-ncsf-nintendo-ds` | Mario Kart DS | NCSF | Derived selected-SDAT control; runtime not executed | 77 |
| `motocross-maniacs-game-boy` | Motocross Maniacs | VGZ | GameBoy DMG | 5 |
| `ocarina-of-time-usf-nintendo-64` | The Legend of Zelda: Ocarina of Time | USF | Nintendo 64 machine runtime not yet executed | 110 |
| `outrun-segapcm` | OutRun | VGZ | YM2151, SegaPCM | 4 |
| `ponpoko-namco-wsg` | Ponpoko | VGZ | C352 (converted from 3-voice Namco WSG) | 12 |
| `pokemon-emerald-gsf-game-boy-advance` | Pokemon Emerald | GSF | Game Boy Advance runtime not yet executed | 17 |
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

The earlier surface contains 29 sets and 585 immutable real-music fixtures. The 2026-08-13
heterogeneous expansion added 22 sets and 108 fixtures. `manifest.json` records
the exact header clocks, VGM versions, command bytes, loop checks, whole-set
SHA-256 values, and Git tree identities for the new VGM/VGZ controls.

The xSF expansion added 264 exact runnable objects. Chrono Cross and Ocarina
of Time were downloaded with explicit user authorization; their source ZIP
hashes and URLs are recorded in `manifest.json`, while the ZIPs themselves are
not retained. Mario Kart DS came from the user's authorized local set.

The 2026-08-13 GSF/NCSF expansion added 94 objects. The Pokémon Emerald ZIP
was downloaded with explicit user authorization, hashed, inspected, and not
retained. No separately published Mario Kart DS NCSF set was found in the
inspected public archive, so the NCSF set was derived deterministically from
the exact SDAT bytes in the already admitted Mario Kart DS 2SF effective ROM.
That paired control asserts byte identity only for the bounded SDAT range; it
does not assert container, selection-state, runtime, driver-state, or audio
equivalence.

## 2026-08-15 Sonic 3 cross-soundtrack controls

The following 34 complete sets add 434 runnable objects. The 31 locally sourced
sets contribute 316 VGM/VGZ and 115 SPC files. Three explicitly authorized
downloads contribute one KSS or SGC executable-rip object each plus 70 retained
extended-M3U subsong routes.

| Corpus ID | Work | Family | External-tag target route | Runnable fixtures |
| --- | --- | --- | --- | ---: |
| `aa-harimanada-vgz` | Aa Harimanada | VGZ | Masayuki Nagao | 22 |
| `america-oudan-ultra-quiz-spc` | America Oudan Ultra Quiz | SPC | Miyoko Takaoka | 18 |
| `ancient-magic-spc` | Ancient Magic | SPC | Miyoko Takaoka; Masanori Hikichi | 19 |
| `battle-golfer-yui-vgz` | Battle Golfer Yui | VGZ | Masayuki Nagao | 18 |
| `battle-master-spc` | Battle Master | SPC | Masanori Hikichi | 22 |
| `dr-robotniks-mean-bean-machine-vgz` | Dr. Robotnik's Mean Bean Machine | VGZ | Masayuki Nagao | 15 |
| `dumbo-pico-vgz` | Dumbo | VGZ | Tatsuyuki Maeda | 5 |
| `ecology-magic-ym2608` | Ecology Magic | VGZ | Miyoko Takaoka | 7 |
| `gg-doraemon-game-gear-vgm` | GG Doraemon: Nora no Suke no Yabou | VGM | Masayuki Nagao | 7 |
| `ghox-ym2151` | Ghox | VGZ | Miyoko Takaoka | 8 |
| `golden-axe-iii-genesis-vgz` | Golden Axe III | VGZ | Tatsuyuki Maeda; Tomonori Sawada | 21 |
| `j-league-pro-striker-vgz` | J.League Pro Striker | VGZ | Tomonori Sawada | 4 |
| `j-league-pro-striker-2-vgz` | J.League Pro Striker 2 | VGZ | Tatsuyuki Maeda | 6 |
| `kakinoki-shougi-spc` | Kakinoki Shougi | SPC | Miyoko Takaoka | 5 |
| `kangofu-san-pico-vgz` | Kangofu-san Pico | VGZ | Masaru Setsumaru | 15 |
| `kuni-chan-game-tengoku-part-2-sgc` | Kuni-chan no Game Tengoku Part 2 | SGC | Tatsuyuki Maeda (user-requested candidate) | 1 |
| `ninku-kss-game-gear` | Ninku | KSS | Tatsuyuki Maeda (user-requested candidate) | 1 |
| `sanrio-puroland-pico-vgz` | Sanrio Puroland | VGZ | Tatsuyuki Maeda | 6 |
| `segasonic-bros-vgz` | SegaSonic Bros. | VGZ | Masaru Setsumaru | 10 |
| `shinobi-iii-vgz` | Shinobi III | VGZ | Masayuki Nagao | 19 |
| `sonic-3d-blast-genesis-vgm` | Sonic 3D Blast | VGM | Tatsuyuki Maeda; Masaru Setsumaru | 24 |
| `sonic-chaos-vgm` | Sonic Chaos | VGM | Masayuki Nagao | 20 |
| `sonic-drift-vgz` | Sonic Drift | VGZ | Masayuki Nagao | 10 |
| `sonic-drift-2-vgm` | Sonic Drift 2 | VGM/VGZ | Masayuki Nagao | 17 |
| `sonic-eraser-vgz` | Sonic Eraser | VGZ | Masaru Setsumaru | 2 |
| `sonic-the-hedgehog-2-genesis-vgz` | Sonic the Hedgehog 2 (Mega Drive) | VGM/VGZ | matched different-person control | 24 |
| `stellar-assault-vgz` | Stellar Assault | VGZ | Masaru Setsumaru | 10 |
| `super-black-bass-spc` | Super Black Bass | SPC | Miyoko Takaoka | 4 |
| `super-columns-vgm` | Super Columns | VGM | Tatsuyuki Maeda | 12 |
| `terranigma-spc` | Terranigma | SPC | Miyoko Takaoka; Masanori Hikichi | 47 |
| `toki-vgm` | Toki | VGM | Masayuki Nagao | 6 |
| `torarete-tamaruka-sgc` | Torarete Tamaruka!? | SGC | Masayuki Nagao (user-requested candidate) | 1 |
| `toy-story-2-pico-vgz` | Toy Story 2: Woody Sousaku Daisakusen!! | VGZ | Masaru Setsumaru | 17 |
| `wizardry-iii-iv-huc6280` | Wizardry III + IV | VGZ | Miyoko Takaoka | 11 |

The corpus now contains 63 sets, 1,019 runnable fixtures, and 70 playlist
sidecars: 1,089 canonical hashed files in total.

The complete-set policy is deliberate. It retains other externally tagged
artists in mixed soundtracks such as Golden Axe III, Sonic 3D Blast, Ancient
Magic, Terranigma, and SegaSonic Bros. as same-toolchain or same-soundtrack
negative controls. It does not cherry-pick only the cues carrying a target name.

`sonic3-attribution-control-external-tags.jsonl` contains 431 exact fixture-hash
joins to Helix `FOOBAR-TAG-*` records. The artist values come only from the
user's foobar external tags ingested by Helix; GD3 and SPC internal artist tags
are not used. These values route candidate controls but do not establish a
composition, arrangement, sequence-programming, driver, patch-design, or
authorship role.

Ninku, Kuni-chan no Game Tengoku Part 2, and Torarete Tamaruka!? were absent as
local executable-rip sets and were downloaded with explicit user authorization.
Their source ZIPs were hashed but not retained:

| Work | Source page | ZIP SHA-256 | Retained object/routes |
| --- | --- | --- | --- |
| Ninku | `https://www.zophar.net/music/sega-game-gear-sgc/ninku` | `9605ae0e6db2ac6ecdde7b7825ea7490e6394f3f8b3eda07807083e2f94c3a76` | 1 KSS / 37 M3U |
| Kuni-chan no Game Tengoku Part 2 | `https://www.zophar.net/music/sega-game-gear-sgc/kuni-chan-no-game-tengoku-part-2` | `7888ec75b912741dd7e6600c93bcc05d12ed57ce98e2d2e0d5041bed4b8c2f7e` | 1 SGC / 16 M3U |
| Torarete Tamaruka!? | `https://www.zophar.net/music/sega-game-gear-sgc/torarete-tamaruka` | `fb80e49607f5a868c94b416374333beda8821cd3c15c92153b7bc50b2d575408` | 1 SGC / 17 M3U |

The executable-rip bytes are not claimed byte-identical to the user's local
rendered files. Their user-requested person associations remain candidate
selection context, not role-scoped attribution evidence.

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
