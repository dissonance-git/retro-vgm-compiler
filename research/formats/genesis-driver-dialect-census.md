# Genesis driver dialect census: software above the YM2612

Status: research evidence for generic semantic ancestry, not runtime game/composer classification.

This census widens the driver-semantic work beyond SMPS and Terpsichorean. The Mega Drive/Genesis is especially useful because many unrelated authoring systems and sound engines converge on nearly the same YM2612 + SN76489 + DAC hardware. That makes the platform a natural control for separating **hardware capability** from **software vocabulary** and **authored intent**.

The compiler must not turn driver names, composers, games, studios, or genres into renderer controls. Named material here exists to discover generic distinctions that can later be emitted from source-native evidence.

## Direct technical observatories

### Echo / ESF

Repository: https://github.com/sikthehedgehog/Echo

Exact format documentation: https://github.com/sikthehedgehog/Echo/blob/master/doc/esf.txt

Echo Stream Format explicitly describes itself as BGM/SFX arrangement data. Its event vocabulary includes per-channel note attack/release, volume, frequency, instrument selection, PCM sample selection, delays, BGM loop control, SFX channel locking, FM parameter changes, flags, and direct FM-register writes.

Especially useful distinctions:

- frequency events can change pitch without starting a new note and are documented as useful for slides;
- PCM playback occupies the DAC path and disables FM6 until the sample ends;
- SFX can lock physical FM/PSG channels so BGM execution is temporarily suppressed;
- raw FM-register writes coexist with higher-level note/instrument events.

This is evidence that one driver can expose both musical abstractions and hardware escape hatches. A raw-register escape therefore does not imply that every effect produced through it has a unique higher-level semantic label.

### MDSDRV + ctrmml

Repositories:

- https://github.com/superctr/MDSDRV
- https://github.com/superctr/ctrmml

MDSDRV uses a composer-facing MML pipeline that compiles to MDS sequence and PCM data. The documented MML contains tie, slur/legato, quantized articulation, early release, shuffle, grace notes, loops/subroutines, echo macros, transpose, detune, pitch/pan/envelope selection, portamento, drum mode, tempo, FM3 special mode, hardware LFO control, PCM mode/rate selection, and direct FM-register writes.

The important architectural fact is not the command spelling. It is that an authored source can make distinctions such as `tie`, `slur`, `portamento`, `detune`, and `direct register write` before the sequence is lowered to the driver and eventually to chip writes.

MDSDRV also makes resource competition explicit: PCM takes priority over FM6, and its 16 logical monophonic tracks can be assigned to physical channels with priority levels. Logical part identity and physical channel identity therefore cannot be assumed to be the same object.

### GEMS research tooling

Repository: https://github.com/realmonster/GEMS

This repository preserves reverse-engineering tools for the commercial GEMS ecosystem, including sequence splitting/combining, instrument decoding, MIDI conversion, and a player. It is useful as a decoding observatory, but it is not treated here as original GEMS source or as proof of the original composer-facing workflow.

The epistemic distinction is explicit:

```text
reverse-engineered format behavior
!=
original authoring-tool semantics
```

A recovered GEMS sequence can still provide exact driver-level evidence while its upstream authoring ancestry remains unknown.

### XGM2 / SGDK

Repository: https://github.com/Stephane-D/SGDK

Implementation: https://github.com/Stephane-D/SGDK/blob/master/src/snd/xgm2.c

XGM2 exposes separate FM and PSG streams, multiple PCM channels with priorities, global FM/PSG volume, tempo/frame state, elapsed/missed-frame state, DMA coordination, and PCM ring buffering. XGM-family workflows can also consume VGM-derived material.

That makes XGM useful as a transformed-runtime control. When the input lineage begins from flattened VGM chip execution, the resulting runtime sequence must not be promoted to source-native authored intent merely because the new driver has a higher-level container.

### Mega PCM 2

Repository: https://github.com/vladikcomper/MegaPCM

Mega PCM 2 is explicitly a DAC-only driver intended to run beside a separate M68K music driver. It provides sample formats, pitch/volume effects, looping, normal-vs-SFX priority classes, panning, DMA protection, and a tested Z80 runtime.

This is a strong counterexample to the assumption that one game has one indivisible sound driver. The semantic graph should be able to represent cooperating driver subsystems whose arbitration changes audible execution without pretending that the PCM subsystem owns FM/PSG musical semantics.

### SMPS family

Repositories:

- https://github.com/sonicretro/smps-rips
- https://github.com/sonicretro/skdisasm

SMPS remains the strongest historical real-game family for source/driver reconstruction, but it is intentionally treated as a family of revisions rather than one language. Prototype variants, tempo algorithms, coordination flags, modulation envelopes, PCM arrangements, bug-compatible/fixed behavior and game-specific extensions all make revision identity material evidence.

## Composer / programmer evidence

These sources are documentary context. They can justify what distinctions to search for, but they do not become fixture-specific runtime facts without source-native linkage.

### Matt Furniss + Shaun Hollingworth

Interview: https://www.sega-16.com/2010/04/interview-matt-furniss/

Furniss describes a custom Genesis production system created with Shaun Hollingworth: Atari Mega ST + SNASM development kit + a tracker-style application. Hollingworth wrote most of the early sound engines and, because he was both a programmer and musician, participated in development of the audio system itself. Furniss also describes leaving PSG available for SFX in some work and using FM/PCM for effects in their system.

Consequence: composer/programmer collaboration can shape channel policy and authoring affordances before any chip trace exists.

### Noriyuki Iwadare

Interviews:

- https://rpgamer.com/2020/09/composer-noriyuki-iwadare-interview/
- https://www.sega-16.com/2008/08/interview-noriyuki-iwadare/

Iwadare describes Mega Drive After Burner II conversion as his first work of that type, says unfamiliar FM parameters/development tools caused difficulty, and later emphasizes that exploiting the Genesis hardware required experimentation.

Preservation research additionally attributes an Iwadare-authored driver/conversion workflow and several distinct Cube driver families, but those claims should remain secondary until matched to preserved source or stronger primary documentation.

Consequence: weak realization can reflect tool familiarity and driver affordances rather than only musical composition or chip limits.

### Hiroshi Kawaguchi (Hiro)

Interview translation/archive: https://shmuplations.com/sst/

Kawaguchi says Sega's early Mega Drive sound drivers were not good enough for his needs, so he rewrote them in assembler. He also describes the period's culture as strongly self-contained, with sound staff taking pride in doing much of the work themselves, while FM instrument parameters were among the few things freely shared.

Consequence: two Sega composers on identical hardware can inherit materially different software worlds and timbre libraries. Studio/platform identity is therefore insufficient to infer a driver dialect.

### Chris Huelsbeck

Interview: https://www.amigapd.com/interview-chris-huelsbeck.html

Huelsbeck describes the Mega Drive as challenging but enjoyable because of its mixed FM, PSG and sample possibilities and specifically says their sample-audio support was programmed by the team. Other historical sources associate his Mega Drive work with the Audios Wave Slave lineage.

Consequence: PCM behavior may be bespoke software structure layered beside FM/PSG sequencing, not a generic property of the console.

### Tim Follin + Dean Belfield

Interview translation/archive: https://note.com/aka_obi/n/nbcecdeb83077

Follin describes the unreleased Mega Drive version of *Time Trax* as using a driver programmed with colleague Dean Belfield, which he regarded as exceptionally flexible, but which was apparently used only for that project.

Source quality note: the accessible page is a Japanese translation/archive of the interview rather than the original publication. Treat the attribution as documentary evidence to quarry further, not as source-native proof.

Consequence: one-off drivers matter. A driver census limited to widely reused commercial engines will systematically miss unusual expressive systems.

## Academic controls

The Genesis-specific archival record is richer than the peer-reviewed literature, so broader chip-music scholarship is used as methodological control rather than fixture evidence.

- Kevin R. Burke, **Hard Limitations and Soft Possibilities**, in *The Oxford Handbook of Video Game Music and Sound* (2024), DOI: https://doi.org/10.1093/oxfordhb/9780197556160.013.17
  - explicitly treats both hardware and software systems as determining composers' challenges and affordances.
- James Newman, **Driving the SID chip: assembly language, composition and sound design for the C64** (2017): https://www.gamejournal.it/driving-the-sid-chip-assembly-language-composition-and-sound-design-for-the-c64/
  - shows bespoke low-level drivers creating pseudo-polyphony, PWM, ring modulation, portamento, drum synthesis and sample playback, supporting the separation between chip specification and software-created musical capability.
- Karen Collins, **In the Loop: Creativity and Constraint in 8-bit Video Game Audio**, DOI: https://doi.org/10.1017/S1478572208000510
  - treats technical constraint and composers' responses as jointly shaping aesthetic decisions.
- Kenneth B. McAlpine, **Bits and Pieces: A History of Chiptunes**, DOI: https://doi.org/10.1093/oso/9780190496098.001.0001
  - combines procedural analysis with practitioner history around coding, composition and constraint.

These sources support the general model. They do not identify a specific Genesis fixture's driver command.

## New generic distinctions earned by the census

The evidence now justifies preserving these independently:

```text
driver family
revision / dialect
artifact role
    authoring source
    compiled sequence
    runtime sequence
    transformed runtime sequence
    register capture
capability state
    supported
    unsupported
    unknown
native command / token when actually evidenced
semantic layer
    authored program
    driver execution
    synthesis
```

Three firewalls follow:

```text
not observed != unsupported
unsupported != unknown
runtime sequence != authoring source
```

And one especially important ancestry rule:

> A transformed runtime artifact may preserve or add useful structure, but it cannot manufacture source-native authored semantics that were already absent from its input lineage.

`model/execution_semantic_dialect.h` implements these distinctions without making any named driver a runtime feature. `tests/model/execution_semantic_dialect_test.cpp` uses MDSDRV/Echo-shaped controls to prove that the same abstract capability can retain different native tokens, transformed runtime material cannot claim source-native authored semantics, and explicit `unsupported` remains distinct from `unknown`.
