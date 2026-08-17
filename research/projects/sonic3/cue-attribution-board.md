# Sonic 3 & Knuckles cue attribution board

This board is the project’s conservative cue-level attribution surface for the committed `tests/corpus/sonic-3-knuckles` corpus.

It is **not** a replacement for the blind attribution experiment. Its purpose is to separate what is already historically grounded from what remains a target for the musical model.

## Evidence vocabulary

- **strong** — direct creator statement, sufficiently specific contemporary/official attribution, or a later source that resolves the exact cue with strong provenance.
- **derived** — the musical work is grounded, while this particular realization inherits it through an Act 2 / prototype / version relationship.
- **candidate** — historically constrained hypothesis worth testing, but not composer ground truth.
- **unresolved** — no individual composer is currently admitted.
- **source-family** — a team/company/source lineage is supported even though an individual composer is not.

Roles are separate throughout:

```text
composer
!= arranger / realization author
!= sequence / sound-data programmer
!= driver / toolchain author
!= patch / sample designer
```

Old fan tags, broad soundtrack artist fields, GD3, and the revisable `curated-attribution-hypotheses.json` are not ground truth.

## Retail / final corpus

| # | Cue | Composer evidence | Arrangement / implementation evidence | Current experimental use |
|---:|---|---|---|---|
| 01 | Angel Island Zone Act 1 | **unresolved** | Sega realization; exact arranger unresolved | blind target |
| 02 | Angel Island Zone Act 2 | **derived from Act 1 composition; individual composer unresolved** | exact Act 2 arranger unresolved | blind target + Act1↔Act2 control |
| 03 | Hydrocity Zone Act 1 | **unresolved** | Sega realization | blind target |
| 04 | Hydrocity Zone Act 2 | **derived from Hydrocity composition** | **Masayuki Nagao — strong arranger** | composition/arrangement separation control |
| 05 | Marble Garden Zone Act 1 | **Miyoko Takaoka — strong** | final Mega Drive realization not automatically Takaoka | primary known-Cube anchor |
| 06 | Marble Garden Zone Act 2 | **Miyoko Takaoka — derived/strong work attribution** | Act 2 arranger unresolved | Takaoka held-out target + Act1↔Act2 control |
| 07 | Carnival Night Zone Act 1 | **source-family: Jackson/Buxer team; individual unresolved** | Sega final realization | non-Cube source-family control |
| 08 | Carnival Night Zone Act 2 | **source-family: Jackson/Buxer team; individual unresolved** | exact Act 2 arranger unresolved | non-Cube source-family control |
| 09 | IceCap Zone Act 1 | **source-family: Brad Buxer/Bruce Connole lineage; individual Sonic cue authorship not collapsed further here** | Sega final realization | source-lineage control |
| 10 | IceCap Zone Act 2 | **derived from IceCap work lineage** | exact Act 2 arranger unresolved | Act1↔Act2 control |
| 11 | Launch Base Zone Act 1 | **source-family: Jackson/Buxer team; individual unresolved** | Sega final realization | non-Cube source-family control |
| 12 | Launch Base Zone Act 2 | **source-family: Jackson/Buxer team; individual unresolved** | exact Act 2 arranger unresolved | non-Cube source-family control |
| 13 | Sub-Boss (S3) | **unresolved** | final implementation unresolved | blind target / boss-family decoy |
| 14 | Staff Roll (S3) | **source-family: Jackson/Buxer-team final credits lineage; individual unresolved** | final realization separate | non-Cube source-family control |
| 15 | Mushroom Hill Zone Act 1 | **CUBE source-family strong; Masanori Hikichi candidate** | final Sega/Cube realization unresolved | high-priority Cube blind target |
| 16 | Mushroom Hill Zone Act 2 | **derived from Mushroom Hill work; Hikichi candidate** | Act 2 realization unresolved | high-priority Cube target + Act1↔Act2 control |
| 17 | Flying Battery Zone Act 1 | **unresolved** | unresolved | primary blind target |
| 18 | Flying Battery Zone Act 2 | **derived from Flying Battery work; individual composer unresolved** | Act 2 realization unresolved | primary target + Act1↔Act2 control |
| 19 | Sandopolis Zone Act 1 | **unresolved** | unresolved | primary blind target |
| 20 | Sandopolis Zone Act 2 | **derived from Sandopolis work; individual composer unresolved** | Act 2 realization unresolved | primary target + Act1↔Act2 control |
| 21 | Lava Reef Zone Act 1 | **unresolved** | Sega realization | blind target |
| 22 | Lava Reef Zone Act 2 | **derived from Lava Reef composition** | **Masayuki Nagao — strong arranger** | composition/arrangement separation control |
| 23 | Sky Sanctuary Zone | **outside-Sega composer lineage — strong constraint; individual unresolved; CUBE is a candidate source** | **Masaru Setsumaru — strong arrangement/programming evidence in project archive** | primary external-composer target |
| 24 | Death Egg Zone Act 1 | **unresolved** | unresolved | primary blind target |
| 25 | Death Egg Zone Act 2 | **derived from Death Egg work; individual composer unresolved** | Act 2 realization unresolved | primary target + Act1↔Act2 control |
| 26 | Sub-Boss (S&K) | **unresolved** | related/reworked boss realization; do not infer composer from implementation similarity | boss-family target |
| 27 | Boss Theme / Boss 2 / Major Boss | **Masanori Hikichi — strong** | final Mega Drive realization separate | primary known-Cube anchor |
| 28 | Big Arms / Final Eggman | **unresolved** | unresolved | blind target |
| 29 | The Doomsday Zone / Last Boss | **unresolved; Hikichi remains a research candidate, not an admission** | unresolved | high-priority blind target |
| 30 | Staff Roll (S&K) | **unresolved at exact-cue level** | S&K / Howard Drossin production context; do not convert broad S&K credit into exact composition | blind target |
| 31 | Data Select | **unresolved** | **Masaru Setsumaru — strong arrangement/programming evidence in project archive** | implementation-vs-composition firewall control |
| 32 | Competition Menu | **source-family constrained; exact individual unresolved** | Opus/Sega development lineage requires version-sensitive treatment | source/version target |
| 33 | Azure Lake | **unresolved** | competition-mode implementation family | blind target |
| 34 | Balloon Park | **unresolved; Jun Senoue stylistic candidate only** | competition-mode implementation family | blind target / Senoue decoy |
| 35 | Chrome Gadget | **unresolved** | competition-mode implementation family | blind target |
| 36 | Desert Palace | **unresolved** | competition-mode implementation family | blind target |
| 37 | Endless Mine | **unresolved** | competition-mode implementation family | blind target |
| 38 | Knuckles' Theme (S3) | **unresolved** | S3 realization | lineage target |
| 39 | Knuckles' Theme (S&K) | **Howard Drossin lineage — strong source/context; exact composition claim retained as derived unless separately sourced** | **Masaru Setsumaru Mega Drive realization/programming context** | Drossin/Setsumaru role-separation control |
| 40 | Gumball Machine | **Jun Senoue — strong** | final realization separate | known-Sega composer control |
| 41 | Magnetic Orbs | **Jun Senoue — strong** | final realization separate | known-Sega composer control |
| 42 | Slot Machine | **Jun Senoue — strong** | final realization separate | known-Sega composer control |
| 43 | Blue Spheres | **Yoshiaki Kashima — strong** | final realization separate | known-Sega composer control |
| 44 | Invincibility (S&K) | **Howard Drossin lineage candidate/derived** | final realization separate | S&K lineage control; not exact ground truth yet |

## Prototype / alternate corpus

The prototype tracks are version-lineage objects, not duplicate files to be normalized away.

| # | Cue | Attribution treatment |
|---:|---|---|
| 45 | Angel Island Zone Act 1 (Beta) | same Angel Island composition family; individual composer unresolved; prototype realization control |
| 46 | Angel Island Zone Act 2 (Beta) | same work family; individual composer unresolved; prototype Act2 control |
| 47 | Carnival Night Zone Act 1 (Beta) | **pre-final Sega prototype composition**, not the Jackson/Buxer final Carnival Night work; exact individual unresolved |
| 48 | Carnival Night Zone Act 2 (Beta) | derived from prototype Carnival Night work; exact individual/arranger unresolved |
| 49 | IceCap Zone Act 1 (Beta) | **pre-final Sega prototype composition**, distinct from final Buxer/Connole lineage; exact individual unresolved |
| 50 | IceCap Zone Act 2 (Beta) | derived from prototype IceCap work; exact individual/arranger unresolved |
| 51 | Launch Base Zone Act 1 (Beta) | **pre-final Sega prototype composition**, distinct from final Jackson/Buxer-family work; exact individual unresolved |
| 52 | Launch Base Zone Act 2 (Beta) | derived from prototype Launch Base work; exact individual/arranger unresolved |
| 53 | Sky Sanctuary Zone (Beta) | same external-composer work family unless future evidence falsifies it; realization/version control |
| 54 | Mushroom Valley Zone Act 1 (Beta) | Mushroom Hill precursor/work family; **CUBE source-family**, Hikichi candidate; powerful prototype↔final control |
| 55 | Mushroom Valley Zone Act 2 (Beta) | same precursor/work family; CUBE source-family; Act2/version control |
| 56 | Competition Menu (Beta) | **Sega/Opus prototype lineage**; exact individual unresolved; important negative/source control |
| 57 | Staff Roll (Beta) | **pre-final Sega prototype credits composition**, distinct from final S3 Jackson/Buxer-team credits lineage; exact individual unresolved |
| 58 | Unused | **Sega-side prototype/unused object; not CUBE by default** | negative/source control |

## Strong historical anchors admitted to the interpretation layer

### Miyoko Takaoka

Takaoka identified Marble Garden as her Sonic 3 work. She also remembered submitting a bonus-stage piece but did not recognize the final bonus-stage tracks when they were shown to her. Therefore:

- Marble Garden is the positive composer anchor.
- no final bonus-stage track is promoted to Takaoka from that recollection.

### Masanori Hikichi

A 2024 correspondence record reports Hikichi specifically identifying the **Act 2 Boss theme** as his. Sonic Origins names this musical object **Boss 2**, the regular end-of-zone Eggman boss cue. Therefore the committed `27 - Boss Theme.vgz` is the strong Hikichi anchor.

His recollection that he *might* have done Mushroom Hill is retained as a candidate only.

### Jun Senoue

Senoue has said that the bonus-stage songs were his. The final bonus-stage set provides Gumball Machine, Magnetic Orbs, and Slot Machine as strong composer controls. Historical claims around short jingles are maintained separately and do not need to contaminate the zone-attribution experiment.

### Yoshiaki Kashima

Kashima directly identified the Special Stage as his contribution. The Blue Spheres cue is therefore a strong composer control. Older exclusion-based attempts to expand this into Angel Island or Sky Sanctuary are not accepted as ground truth.

### Tomonori Sawada

Sawada is strongly associated with composition/arrangement of the Sonic 3 title screen. That title cue is not present as a standalone item in the current 58-track corpus, so it is useful as an external known control but does not justify assigning Angel Island, Sky Sanctuary, Flying Battery, Doomsday, or other unresolved cues to him.

### Masayuki Nagao

Nagao’s strongest exact role evidence is **arrangement/programming**, including Hydrocity Act 2 and Lava Reef Act 2. His historical statement that he believed he had not composed original Sonic 3 music is treated as counterevidence against using his implementation fingerprint as composer proof.

### Masaru Setsumaru

Setsumaru is a major implementation/arrangement fingerprint source. In particular, Data Select and Sky Sanctuary carry strong project-archive realization evidence. This is deliberately *not* promoted into composer attribution.

### Howard Drossin

Drossin was the Sonic & Knuckles composer/music director context, while Setsumaru input music into the Mega Drive realization. Drossin himself has warned that web credits often assign him Sonic music he did not write. Therefore broad S&K association is not enough to label Mushroom Hill or every late-game cue as Drossin.

## Current Cube attack board

The next composer-facing model should not waste its first discriminative power on already-grounded tracks. Its key held-out targets are:

1. Mushroom Hill Act 1 / Act 2 — CUBE known, Hikichi candidate.
2. Flying Battery Act 1 / Act 2 — unresolved.
3. Sandopolis Act 1 / Act 2 — unresolved.
4. Sky Sanctuary — external composer known, individual unresolved; CUBE plausible.
5. Death Egg Act 1 / Act 2 — unresolved.
6. The Doomsday Zone — unresolved, Hikichi lead worth pressure-testing.

Anchors / adversarial controls:

- Marble Garden — Takaoka positive anchor.
- Boss 2 — Hikichi positive anchor.
- Gumball / Magnetic Orbs / Slot — Senoue controls.
- Blue Spheres — Kashima control.
- Carnival Night / IceCap / Launch Base final-versus-beta pairs — source/version confound controls.
- Hydrocity 2 / Lava Reef 2 — Nagao arrangement controls.
- Data Select / Sky Sanctuary — Setsumaru implementation controls.

## Promotion rule

A future musical-model result may update a cue from `unresolved` to `candidate` when it survives blinded controls. It may update from `candidate` to a stronger evidence state only when the musical result is coupled to independent historical/documentary evidence or an exceptionally discriminating preregistered cross-work test.

Similarity alone never becomes history.
