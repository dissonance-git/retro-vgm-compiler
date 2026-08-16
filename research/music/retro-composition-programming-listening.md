# Retro composition, realization practice, and holistic listening

## Status

Current research input for the provenance-aware musical execution model.

This case asks:

> If VGM Tooling is handed a VGM, SPC, driver sequence, MML source, or related executable game-music object, what must it preserve and infer so that it can reason both about how the object was technically made and about the song a listener actually hears?

The combined practitioner, source-code, synthesis, expressive-performance, MIR, music-cognition and ludomusicology evidence rejects both of these reductions:

```text
bytes -> notes
```

and:

```text
score -> neutral renderer -> song
```

In important 8-bit and 16-bit traditions, composition, arrangement, sound programming, synthesis design, driver behavior, looping and machine-specific performance are partially entangled. The correct response is not to collapse them into one layer. It is to preserve the different evidence routes while allowing a synchronized listening-level account over the whole song.

## Practitioner evidence: there was no single retro workflow

### Yuzo Koshiro

Koshiro described his early Falcom role as `sound programmer`: he wrote music and programmed, learned FM synthesis by analyzing waveform behavior, wrote and compiled his own programs, helped write Mega Drive drivers, and customized MML so sound construction and phrase construction could be controlled below ordinary MIDI abstractions.

Research consequence:

```text
composer identity
!= sound-programmer identity

but one person may occupy both
```

More importantly, musical arrangement and low-level realization can be one continuous practice rather than a clean handoff.

Primary interview:
- Yuzo Koshiro, 2001 composer interview, translated by shmuplations: https://shmuplations.com/yuzokoshiro/

### Hirokazu Tanaka

Tanaka described early Nintendo sound work as inseparable from electronics and programming. He wrote his own Famicom sequencer in assembly and emphasized direct control of the sound source because standard conversion routes hid parameters he wanted to shape.

This is strong evidence against treating low-level control as implementation noise around an independently complete score.

Primary interview:
- Alexander Brandon, “Shooting from the Hip: An Interview with Hip Tanaka,” Game Developer, 2002: https://www.gamedeveloper.com/audio/shooting-from-the-hip-an-interview-with-hip-tanaka

### Naoki Kodaka and the Sunsoft team

Kodaka represents a more distributed workflow. He could compose on paper and provide demo tapes while dedicated sound programmers realized the music on Famicom hardware. But the relationship was iterative rather than one-way: Kodaka requested musical qualities, programmers created hardware/software techniques, and those new sounds could in turn provoke new composition.

```text
composition
        <->
arrangement / sound-programming possibilities
        <->
revised composition / realization
```

Primary interview:
- Naoki Kodaka, Sunsoft Famicom Music interview, translated by shmuplations: https://shmuplations.com/sunsoftmusic/

### David Wise

Wise has described NES composition directly as hexadecimal pitch/duration data with programmed controls such as pitch bends. On SNES, sample memory, waveform choice and eight monophonic channels shaped both instrument design and composition. His Donkey Kong Country recollections connect the desired musical result to Wavestation-style wave sequencing, short source waveforms and active experimentation with the SNES sound system.

This is a reconstruction warning: a short or compressed waveform is not automatically damage to reverse. It may be part of the authored/programmed timbre and arrangement.

Interviews:
- https://comicbook.com/gaming/news/donkey-kong-country-david-wise-interview/
- https://www.nintendojo.com/features/interviews/interview-david-wise

### Martin Galway

Galway released original C64 music-player assembly specifically so people can study the players and understand how he worked. The repository contains source for works including *Wizball*, *Arkanoid*, *Green Beret* and others, and distinguishes generations of his player design.

This creates an unusually strong control corpus:

```text
composer/programmer testimony
+
actual player source
+
renderable execution
+
known released music
```

Repository:
- https://github.com/MartinGalway/C64_music

### Barry Leitch

Leitch described cross-platform workflows where musical data could remain related while machine-specific instruments and effects changed. The same underlying music could move between machines without implying identical realization.

```text
shared musical material
!= identical machine realization

different machine realization
!= unrelated composition
```

Interview:
- https://www.gamedeveloper.com/audio/interviewing-veteran-composer-barry-leitch-part-i-sound-chips-from-zx-81-to-the-snes-

## Source-code Rosetta stones

Two observatories are especially valuable because they expose practitioner-side source rather than only finished audio.

### `MartinGalway/C64_music`

Original assembly music-player source from the composer/programmer. This can test whether trace-derived technical fingerprints correspond to known implementation idioms.

### `onitama/mucom88`

MUCOM88 preserves an MML composition/driver environment originally created by Yuzo Koshiro for PC-8801, creating a bridge across authored MML, compiler/driver behavior, FM synthesis and released game-music lineage.

Repository:
- https://github.com/onitama/mucom88

Existing observatories such as `vgmtrans/vgmtrans`, `kuma4649/mml2vgm`, `libgme/game-music-emu`, VGM/SPC tooling, native drivers and tracker systems remain useful for other strata.

## Programmed expression is not implementation residue

Expressive-performance research consistently shows that timing, dynamics, articulation and intonation carry musical information beyond nominal notation.

Useful literature:
- Cancino-Chacón et al., “Computational Models of Expressive Music Performance: A Comprehensive and Critical Review,” 2018. DOI: 10.3389/fdigh.2018.00025
- Joan Serrà et al., “Note Onset Deviations as Musical Piece Signatures,” PLOS ONE, 2013. DOI: 10.1371/journal.pone.0069268
- Alf Gabrielsson, “Timing in music: Performance and experience,” JASA, 1987. DOI: 10.1121/1.2024469
- Panayotis Mavromatis, “A Multi-tiered Approach for Analyzing Expressive Timing in Music Performance,” 2009. DOI: 10.1007/978-3-642-02394-1_18

Executable game music has analogous exact controls:

- gate length;
- attack/release shaping;
- per-note/per-frame volume trajectory;
- vibrato and delayed vibrato;
- portamento and detune;
- pitch envelopes and arpeggiation;
- duty/pulse-width changes;
- FM operator/envelope changes;
- waveform changes;
- sample start/loop behavior;
- retrigger policy and allocation/stealing behavior;
- rhythmic echo/delay;
- deliberate machine-timed offsets.

The safe hierarchy is:

```text
exact programmed control
!= derived musical gesture
!= higher expressive interpretation
```

A driver-level pitch envelope may be exact. Calling it a scoop or expressive accent is a higher musical interpretation supported by that control.

## Synthesis research: perceptual similarity does not recover unique source identity

Synthesizer parameter spaces are high-dimensional, nonlinear and often many-to-many with perceptual timbre. Perceptual sound matching therefore cannot by itself prove one unique historical patch, synthesis algorithm or acoustic instrument.

Useful literature:
- Stefano Fasciani, “TSAM: a tool for analyzing, modeling, and mapping the timbre of sound synthesizers,” 2016. DOI: 10.5281/zenodo.851209
- Hoffman & Cook, “Feature-Based Synthesis: Mapping Acoustic and Perceptual Features onto Synthesis Parameters,” ICMC 2006
- Han, Lostanlen & Lagrange, “Learning to Solve Inverse Problems for Perceptual Sound Matching,” 2023. DOI: 10.48550/arxiv.2311.14213
- Esling, Chemla-Romeu-Santos & Bitton, “Bridging Audio Analysis, Perception and Synthesis with Perceptually-regularized Variational Timbre Spaces,” ISMIR 2018

Durable law:

```text
perceptually similar timbre
!= same synthesis parameters
!= same historical instrument
!= same synthesis algorithm
```

A reconstruction may be musically excellent without being historical recovery.

## Ludomusicology: the song can extend beyond a linear file

Game music can participate in rule structure, player action, repetition, death/respawn, state changes and adaptive form. Looping may have musical and ludic function rather than being merely a storage artifact.

Useful literature:
- Karen Collins, “In the Loop: Creativity and Constraint in 8-bit Video Game Audio,” *Twentieth-Century Music*, 2008. DOI: 10.1017/S1478572208000510
- Mathew Arnold, “Inside the Loop: The Audio Functionality of Inside,” *The Computer Games Journal*, 2018. DOI: 10.1007/s40869-018-0071-x
- Patrick Hutchings & Jon McCormack, “Adaptive Music Composition for Games,” *IEEE Transactions on Games*, 2020. DOI: 10.1109/TG.2019.2921979
- Ailbhe Warde-Brown, “Waltzing on Rooftops and Cobblestones,” *Journal of Sound and Music in Games*, 2021. DOI: 10.1525/JSMG.2021.2.3.34

For a static VGM/SPC object:

```text
track loop point                  source/execution fact
repetition in one capture         capture fact
intended gameplay repetition      contextual/historical claim
musical/ludic effect of repetition analytical/perceptual claim
```

Do not hallucinate missing game state.

## Holistic listening is a synchronized representation problem

Music perception constructs and relates multiple levels at once: pitch, timbre, loudness and timing; voices/streams/chords; phrases and sections; form; memory and expectation.

Useful literature:
- Marcus Pearce, “Music Perception,” *Oxford Research Encyclopedia of Psychology*, 2023. DOI: 10.1093/acrefore/9780190236557.013.890
- Stephen McAdams, “Timbre as a structuring force in music,” JASA, 2013. DOI: 10.1121/1.4806102
- Stefan Koelsch, “Neural Basis of Music Perception: Melody, Harmony, and Timbre,” 2019. DOI: 10.1093/oxfordhb/9780198804123.013.9

The eventual song-level analysis should answer questions such as:

- what enters and leaves the texture;
- which events fuse into one apparent gesture;
- which physical voices sustain one musical part;
- where melody migrates among timbres or channels;
- how bass and percussion interlock;
- what changes at phrase/section boundaries;
- what returns and what is transformed;
- how a loop closes musically, not only byte-wise;
- which timbral changes are structural;
- where density, register, loudness, spectral balance or rhythmic activity change;
- what creates contrast, arrival, suspension, propulsion, release or stasis under a declared analysis/model.

At one aligned time span, VGM Tooling should be able to expose:

```text
source bytes / commands
+ driver state
+ synthesis objects / modulation
+ physical voice episodes
+ realized performance events
+ rendered audio
+ auditory events / streams
+ musical roles / motifs / phrases / sections
+ game-context annotations when externally known
+ attribution evidence
```

No new permanent `holistic_song` ontology is justified. The existing graph plus analysis features should first support this as a synchronized projection.

## Attribution must be role-relative

A single `composer fingerprint` is unsafe.

The current coordinate set is:

```text
COMPOSITION
melodic • rhythmic • harmonic • formal • motivic tendencies

ARRANGEMENT / SOUND PROGRAMMING
register • voicing • texture • channel-role choices • modulation idioms • effects
articulation • envelopes • control tricks • machine-specific realization choices

DRIVER / TOOLCHAIN
command grammar • allocation behavior • scheduler idioms • data layout
compiler / driver artifacts

PATCH / SAMPLE DESIGN
FM parameter/topology habits • waveforms • sample preparation • loop strategy

RENDERING
level relationships • echo/reverb strategy • mixing • hardware-specific realization
```

`ARRANGEMENT / SOUND PROGRAMMING` is intentionally merged. In retro executable music, register/voicing/texture decisions and modulation/effects/control decisions frequently form one continuous realization practice. Historical credits may still distinguish arranger, sound programmer, sound designer or composer, but the analytical fingerprint coordinate should not pretend there is a universal technical boundary between arrangement and programming.

The same person may occupy several coordinates; several people may contribute to one cue. A strong match in one coordinate must not promote another.

```text
sound-programming / arrangement match
!= composer proof

driver match
!= composition match

patch/sample match
!= authorship proof
```

This is compatible with the existing regression `tests/model/creative_role_attribution_test.cpp`, which protects the narrower law that technical realization evidence cannot silently become composer attribution.

## Current implementation consequence

The project already has enough vocabulary for the first controls:

- exact/derived/hypothesis evidence states;
- source-relative analysis availability;
- `musical_performance`, `musical_structure`, `auditory_interpretation`, `listener_response`, and cross-cutting `musicological_context`;
- persistent-part hypotheses distinct from physical episodes and auditory streams;
- exact programmed controls distinct from derived musical gestures;
- timbre/source identity distinct from instrument interpretation;
- separate technical and composition-style attribution hypotheses.

No new core node/edge taxonomy is justified by this pass.

## Next experiment

The strongest next test is an end-to-end real-song control using unusually well-documented source, preferably the Galway C64 source corpus or a MUCOM88/Koshiro route.

The test should traverse:

```text
authored/programmed source
↓
driver / control flow
↓
device execution
↓
synthesis / waveform
↓
performed gestures
↓
persistent parts / streams
↓
phrases / sections / form
↓
coherent whole-song account
```

Success is not “the notes were extracted correctly.”

Success is:

> The system can produce a musically recognizable account of the piece while every technical, structural, perceptual and historical assertion remains linked to evidence and uncertainty.
