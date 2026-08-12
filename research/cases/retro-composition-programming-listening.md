# Retro composition, sound programming, expressive performance, and holistic listening

## Status

Research input for the provenance-aware musical execution model.

This pass asks a more concrete question than the earlier layer taxonomy:

> If VGM Tooling is handed a VGM, SPC, driver sequence, MML source, or related game-music object, what must it preserve and infer so that it can reason both about how the object was technically made and about the song a listener actually hears?

The answer from practitioner interviews, source-code archaeology, synthesis research, expressive-performance research, music cognition, ludomusicology, and MIR is that retro game music cannot safely be modeled as either:

```text
bytes -> notes
```

or:

```text
score -> neutral renderer -> song
```

For many important 8-bit and 16-bit traditions, composition, arranging, synthesis, driver design, programming, performance-like parameter shaping, looping, and game context are partially entangled. The correct response is not to collapse them. It is to preserve each contribution while allowing a synchronized song-level view across them.

## 1. Practitioner evidence: there was no single retro workflow

### Yuzo Koshiro: composer and sound programmer as one technical-musical practice

In a 2001 interview, Koshiro described his early Falcom role explicitly as `sound programmer`: he wrote music and programmed. He learned FM synthesis by analyzing waveform behavior, wrote and compiled his own programs, helped write Mega Drive drivers for *The Revenge of Shinobi*, and customized MML so that sound construction and phrase construction could be controlled below what ordinary MIDI exposed.

He also described the technical background as a creative advantage: when hardware lacked a capability, software could recreate or approximate it. A nominal three-voice limitation did not necessarily imply a three-part perceived texture if software tricks created additional apparent activity.

Research use:

```text
composer identity
!= sound-programmer identity

but in some historical cases:
composer person == sound-programmer person
```

The roles remain distinct even when one person fills both.

Primary interview:
- Yuzo Koshiro, 2001 composer interview, translated by shmuplations: https://shmuplations.com/yuzokoshiro/

### Hirokazu Tanaka: direct access to the source as part of individual style

Tanaka described early Nintendo sound work as inseparable from electronics and programming. For Famicom work he did not simply accept the standard conversion route from PC/MIDI tools. He wrote his own sequencer in assembly and emphasized directly controlling the sound source and its parameters because that let him extract more detailed behavior from the limited chip.

This is strong evidence against treating low-level control as implementation noise around an independently complete composition.

Primary interview:
- Alexander Brandon, “Shooting from the Hip: An Interview with Hip Tanaka,” Game Developer, 2002: https://www.gamedeveloper.com/audio/shooting-from-the-hip-an-interview-with-hip-tanaka

### Naoki Kodaka and the Sunsoft team: composer-programmer feedback loop

Kodaka described a different organization. He wrote music on paper and could provide demo tapes, while sound programmers realized it on Famicom hardware. But the relationship was not a one-way transcription service. Kodaka listened to selected sounds and requested musical qualities; programmers answered compositional ideas with hardware and software techniques.

The Sunsoft team used techniques including combined triangle/noise percussion, delta-sample bass, and software-created reverberant effects. Kodaka compared the programmers to highly skilled craftsmen and described a feedback loop where a programmer could create a compelling sound and challenge him to write music for it.

Therefore:

```text
sheet composition
        <->
sound-programming possibility
        <->
revised composition / realization
```

The finished audible work can contain joint authorship at different semantic levels without requiring us to collapse the legal or historical credits.

Primary interview:
- Naoki Kodaka, Sunsoft Famicom Music interview, translated by shmuplations: https://shmuplations.com/sunsoftmusic/

### David Wise: composition through memory, waveform, and DSP constraints

Wise has described composing NES music directly as hexadecimal pitch/duration data with routines for controls such as pitch bends. On SNES he chose code rather than a MIDI-centered workflow in part because memory was precious. The SNES sample budget and eight monophonic channels were not merely post-composition constraints: they shaped instrument design and composition.

His recollections of *Donkey Kong Country* also connect the desired musical result to Korg Wavestation-style wave sequencing, sample choice, short waveforms, and explicit experimentation with the SNES sound system.

This matters for later “restoration.” A compressed or short waveform is not automatically damage to reverse. It can be a component of the authored timbre and arrangement.

Interviews:
- ComicBook.com, David Wise interview: https://comicbook.com/gaming/news/donkey-kong-country-david-wise-interview/
- Nintendojo, David Wise interview: https://www.nintendojo.com/features/interviews/interview-david-wise

### Martin Galway: the music player itself survives as primary musical evidence

Galway has released original C64 music source material in `MartinGalway/C64_music`, specifically so people can read and understand the music players and how he worked. The repository contains assembly for works including *Wizball*, *Arkanoid*, *Green Beret*, and others, and distinguishes generations of his player design.

This is unusually strong evidence because the practitioner interview tradition and surviving implementation can be compared directly.

For VGM Tooling, this is an ideal future control corpus:

```text
composer/programmer testimony
+
actual music-player source
+
renderable executable behavior
+
known released soundtrack
```

Repository:
- https://github.com/MartinGalway/C64_music

### Barry Leitch: common musical data, machine-specific realization

Leitch described a cross-platform workflow in which music could originate in a tracker-like environment, share a data structure across machines, and then receive machine-specific instrument work and effects. The same musical data could move among platforms while superior machine capabilities were exploited where available. His *Top Gear* account is especially useful: the SNES echo was musically selected but bounded by memory cost, yielding a specific rhythmic delay relationship.

This creates a clean version/reconstruction lesson:

```text
shared musical data
!= identical realization

machine-specific realization
!= unrelated composition
```

Interview:
- Barry Leitch, “Sound chips (from ZX-81 to the SNES),” Game Developer, 2015: https://www.gamedeveloper.com/audio/interviewing-veteran-composer-barry-leitch-part-i-sound-chips-from-zx-81-to-the-snes-

## 2. Source-code observatories are more valuable when paired with testimony

Two current GitHub observatories are unusually useful because they expose authored/programmer-side evidence rather than only finished audio.

### `MartinGalway/C64_music`

The repository exposes assembly music-player source from the original composer/programmer. It gives VGM Tooling a future way to test whether low-level driver idioms can be recovered from traces and whether an inferred technical fingerprint actually corresponds to a known authored implementation.

### `onitama/mucom88`

MUCOM88 preserves an MML composition/driver environment originally created by Yuzo Koshiro for PC-8801. The open project preserves source and tooling lineage, while the official MUCOM88 distribution also provides licensed sample music data by Koshiro from multiple games.

This is a rare bridge across:

```text
authored MML
-> compiler/driver environment
-> FM synthesis behavior
-> released game-music lineage
```

Repository:
- https://github.com/onitama/mucom88

Official project/distribution:
- https://onitama.tv/mucom88/index_en.html

### Existing source-side observatories retained

The earlier observatories remain valuable for other portions of the path:

- `vgmtrans/vgmtrans`: extracted sequence/instrument/sample structures from many game formats;
- `kuma4649/mml2vgm`: authored MML -> executable/logged game-chip music workflows;
- `libgme/game-music-emu`: reference playback across executable legacy music formats;
- VGM/SPC native repositories already recorded elsewhere in this project.

The new conclusion is that these should not all be evaluated as “ways to get notes.” They expose different pieces of historical authorship and execution.

## 3. Expressive performance exists inside programmed music too

Expressive-performance research consistently shows that a performed piece contains systematic timing, dynamics, articulation, and intonation choices that are not reducible to the nominal score.

Cancino-Chacón et al.'s review of computational expressive-performance models emphasizes tempo, timing, dynamics, intonation, and articulation as shaped performance parameters. Gabrielsson's timing work likewise treats timing variation as a major carrier of perceived structure and motion. Work on onset deviations has even found short timing-deviation sequences predictive of the piece being played across performers, showing that expressive timing can reflect composition-level structure as well as performer individuality.

Useful literature:
- Cancino-Chacón et al., “Computational Models of Expressive Music Performance: A Comprehensive and Critical Review,” Frontiers in Digital Humanities, 2018. DOI: 10.3389/fdigh.2018.00025
- Joan Serrà et al., “Note Onset Deviations as Musical Piece Signatures,” PLOS ONE, 2013. DOI: 10.1371/journal.pone.0069268
- Alf Gabrielsson, “Timing in music: Performance and experience,” JASA, 1987. DOI: 10.1121/1.2024469
- Panayotis Mavromatis, “A Multi-tiered Approach for Analyzing Expressive Timing in Music Performance,” 2009. DOI: 10.1007/978-3-642-02394-1_18

For retro executable music, the analogous expressive variables can include:

- gate length;
- attack/release shaping;
- per-note or per-frame volume trajectory;
- vibrato and delayed vibrato;
- portamento;
- detune;
- pitch envelopes;
- arpeggiation;
- duty/pulse-width changes;
- FM operator/envelope changes;
- waveform changes;
- sample start/loop behavior;
- retrigger policy;
- note stealing/allocation;
- rhythmic echo/delay;
- deliberate micro-offsets between parts where the source/driver permits them.

These are not all “humanization.” They can be exact programmed musical instructions.

Therefore the safe hierarchy is:

```text
nominal event
!= realized event
!= expressive interpretation
```

A driver-level pitch envelope may be exact execution evidence. Calling it a “scoop,” “ornament,” “expressive accent,” or “phrase gesture” is a higher musical interpretation.

## 4. Synthesis research: sound identity is many-to-many

The synthesis literature reinforces the existing timbre/organology pass. Synthesizer parameter spaces are high-dimensional, and the mapping from parameter changes to perceived timbre is nonlinear and often difficult to derive analytically. Work on timbre spaces and perceptual sound matching therefore searches in acoustic/perceptual feature spaces rather than assuming that a perceptually similar target identifies one unique parameter set.

Useful literature:
- Stefano Fasciani, “TSAM: a tool for analyzing, modeling, and mapping the timbre of sound synthesizers,” 2016. DOI: 10.5281/zenodo.851209
- Hoffman & Cook, “Feature-Based Synthesis: Mapping Acoustic and Perceptual Features onto Synthesis Parameters,” ICMC 2006.
- Han, Lostanlen & Lagrange, “Learning to Solve Inverse Problems for Perceptual Sound Matching,” 2023. DOI: 10.48550/arxiv.2311.14213
- Esling, Chemla-Romeu-Santos & Bitton, “Bridging Audio Analysis, Perception and Synthesis with Perceptually-regularized Variational Timbre Spaces,” ISMIR 2018.

GitHub observatories such as `surge-synthesizer/surge`, `BespokeSynth/BespokeSynth`, and sampler/synthesis projects are useful as implementation laboratories for modulation topology, voice structure, parameter automation, and candidate high-resolution realization.

The durable law remains:

```text
perceptually similar timbre
!= same synthesis parameters
!= same historical instrument
!= same synthesis algorithm
```

For reconstruction, a candidate may be musically excellent without being historical recovery. Those claims must remain separate.

## 5. Ludomusicology: the song can extend beyond a linear file

Ludomusicology adds a dimension that ordinary music analysis misses: game music can participate in rule structure, spatial context, player action, repetition, death/respawn, state transitions, and adaptive form.

Research on looping and adaptive game audio shows that repetition is not merely a storage artifact. It can create continuity, situational memory, immersion, pacing, and formal behavior specific to play. Work on open-world and adaptive music further treats music as tied to game-world context and player state rather than as a passive linear soundtrack.

Useful literature:
- Karen Collins, “In the Loop: Creativity and Constraint in 8-bit Video Game Audio,” Twentieth-Century Music, 2008. DOI: 10.1017/S1478572208000510
- Mathew Arnold, “Inside the Loop: The Audio Functionality of Inside,” Games and Culture / game-audio study, 2018. DOI: 10.1007/s40869-018-0071-x
- Patrick Hutchings & Jon McCormack, “Adaptive Music Composition for Games,” IEEE Transactions on Games, 2020. DOI: 10.1109/TG.2019.2921979
- Ailbhe Warde-Brown, “Waltzing on Rooftops and Cobblestones: Sonic Immersion through Spatiotemporal Involvement in the Assassin’s Creed Series,” Journal of Sound and Music in Games, 2021. DOI: 10.1525/JSMG.2021.2.3.34

For a static VGM/SPC file, VGM Tooling must not hallucinate the missing game state. But it may still preserve exact file-level loop structure and attach externally sourced context when known.

Useful distinction:

```text
track loop point                      exact source/execution fact
repetition count in one capture       exact capture fact
intended indefinite gameplay loop     external/contextual claim
where cue was used in the game        external historical claim
effect on immersion/pacing            ludomusicological/perceptual analysis
```

## 6. Holistic music perception is a synchronized representation problem

Music-perception research directly supports the user's stronger target. A listener does not perceive an unordered list of note tokens. Perception constructs event attributes such as pitch, timbre, loudness, and timing; groups events into chords, voices, streams, and phrases; relates those groups into larger form; and combines them with memory and expectation.

Useful literature:
- Althea F. P. Moore, “Music Perception,” Oxford Research Encyclopedia of Psychology, 2023. DOI: 10.1093/acrefore/9780190236557.013.890
- Stephen McAdams, “Timbre as a structuring force in music,” JASA, 2013. DOI: 10.1121/1.4806102
- Stefan Koelsch, “Neural Basis of Music Perception: Melody, Harmony, and Timbre,” Oxford Handbook chapter, 2019. DOI: 10.1093/oxfordhb/9780198804123.013.9

This means VGM Tooling's eventual song-level reasoning should answer questions such as:

- what enters and leaves the texture;
- which events fuse into one apparent gesture;
- which physical voices sustain one musical part;
- where the melody migrates between timbres or channels;
- how bass and percussion interlock;
- what changes at a phrase or section boundary;
- what returns and what is transformed on return;
- how the loop closes musically, not only byte-wise;
- which timbral changes are structural rather than ornamental;
- where density, register, loudness, spectral balance, or rhythmic activity change;
- what creates contrast, arrival, suspension, propulsion, release, or stasis under a declared analytical/perceptual model;
- which observations are source facts and which are listener/model hypotheses.

The important implementation principle is **time alignment**.

A future analysis should make it possible to point at one time span and inspect, together:

```text
source bytes / commands
+ driver state
+ synthesis objects and modulation
+ physical voice episodes
+ realized performance events
+ rendered audio
+ auditory events / streams
+ musical roles / motifs / phrases / sections
+ game-context annotations when externally known
+ attribution evidence
```

That is much closer to “hearing the song with everything underneath it” than a symbolic-note dump.

No new permanent `holistic_song` ontology is justified yet. The current graph and analysis-feature system can support this as a bounded synchronized projection until a real analysis task proves a missing primitive.

## 7. Attribution must become role-relative

The practitioner corpus makes a single “composer fingerprint” actively unsafe.

A retro file may expose independent evidence for:

```text
COMPOSITION FINGERPRINT
melodic, harmonic, rhythmic, formal tendencies

ARRANGEMENT FINGERPRINT
voicing, register, texture, orchestration, channel-role decisions

SOUND-PROGRAMMING FINGERPRINT
control idioms, effects, modulation macros, timing behavior, instrument realization

DRIVER / TOOLCHAIN FINGERPRINT
command grammar, allocation, data layout, compiler/driver artifacts

PATCH / SAMPLE-DESIGN FINGERPRINT
FM parameter habits, waveforms, envelopes, sample preparation, loop strategy

RENDER / MIX FINGERPRINT
level relationships, echo/reverb strategy, device-specific realization
```

The same person can contribute to several coordinates. Several people can contribute to one finished cue.

A strong match in one coordinate must not automatically promote another.

Example:

```text
driver idiom strongly matches programmer A
        does not imply
composer = programmer A

melodic style strongly matches composer B
        does not imply
sound programmer = composer B
```

This is directly useful for vague or incomplete credits.

The 2025 systematic survey of symbolic composer attribution also supplies a general warning: attribution research often suffers from imbalanced corpora, weak validation, and accuracy-only reporting. Any future VGM attribution model should use held-out evaluation, source-family controls, balanced metrics where appropriate, and explicit confound tests.

Useful literature:
- Federico Simonetta, “Style-based Composer Identification and Attribution of Symbolic Music Scores: a Systematic Survey,” 2025. arXiv:2506.12440
- McKay, Cumming & Fujinaga, “jSymbolic 2.2,” ISMIR 2018.

## 8. GitHub observatories for the next implementation frontier

### Source / authoring / execution

- `MartinGalway/C64_music`
- `onitama/mucom88`
- `kuma4649/mml2vgm`
- `vgmtrans/vgmtrans`
- `libgme/game-music-emu`
- source-family emulators and players already catalogued elsewhere in VGM Tooling

### Musical structure / score-performance relation

- `CPJKU/partitura`
- `urinieto/msaf`
- `MTG/essentia`
- `CPJKU/madmom`
- previously catalogued music21 / Humdrum / MEI tools

`partitura` is especially useful because it preserves score-like symbolic structure and supports performance-oriented research. `msaf` is useful as a comparative laboratory for section-boundary and structure-analysis methods. Their outputs should remain analyses over VGM Tooling truth, not replacement truth.

### Synthesis / timbre search

- `surge-synthesizer/surge`
- `surge-synthesizer/shortcircuit-xt`
- `BespokeSynth/BespokeSynth`

These are most useful as implementation observatories for synthesis graphs, modulation, sampling, parameter automation, and search spaces rather than as new playback dependencies.

## 9. Resulting laws

### Law A: programmed expression is first-class musical evidence

If the source or validated driver explicitly controls articulation, modulation, gate, dynamics, waveform, sample behavior, or timing, preserve that control as musical-performance evidence. Do not normalize it away to a nominal note list.

### Law B: a technical fingerprint has a role scope

A technical or statistical match must state what role it can support: composition, arrangement, sound programming, driver/toolchain, patch/sample design, or rendering. It may not silently promote across roles.

### Law C: reconstruction is obligation-relative

A reconstruction may target several different obligations:

```text
historical source fidelity
perceptual similarity
musical-role preservation
structural preservation
higher-resolution source-native realization
hypothetical acoustic/orchestral realization
```

One candidate need not optimize all of them. The target and tradeoffs must be explicit.

### Law D: game context is evidence, not presumed metadata

Loop points and command behavior can come from the executable object. Gameplay meaning, scene use, adaptive triggers, and historical intention require game/runtime or external evidence.

### Law E: holistic listening is a projection across aligned truths

The song-level view should not replace source truth. It should make source, execution, acoustics, musical organization, perception, context, and attribution jointly reachable for the same time span.

## 10. Concrete next controls

The first executable regression earned by this pass is not a new ontology. It is a role-separation control:

1. exact driver identity can be present;
2. a strong sound-programmer match can be supported;
3. composer attribution can remain unknown;
4. an independent compositional-style hypothesis can point elsewhere;
5. all four facts coexist without one overwriting another.

A later representative-file experiment should use a source with unusually rich provenance, ideally one of:

```text
Koshiro authored MML / MUCOM88 lineage
Galway C64 source + release
known VGM trace with documented composer/programmer split
known SPC with surviving sequence/driver source
```

and test the complete path:

```text
source
-> execute/render
-> align source and waveform time
-> recover physical/performance activity
-> infer parts/streams/sections
-> produce a song-level explanation
-> descend from each explanation back to exact evidence
```

The acceptance target is not “correct notes.” It is whether the system can say something musically recognizable about the actual track while every statement remains traceable to what the file, renderer, model, or external source can legitimately support.
