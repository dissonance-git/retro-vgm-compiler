# Whole-song controls: Wizball and MUCOM88

## Purpose

The previous research passes established the representation/evidence layers and then corrected the human-facing target: VGM Tooling should not merely know musical facts, it should be able to discuss music naturally while preserving the exact machinery underneath.

This pass chooses two complementary empirical controls:

1. **Martin Galway, Wizball title music (C64, 1987)** as a bottom-up control from surviving executable source and driver code into musical and human description;
2. **Yuzo Koshiro / MUCOM88** as a top-down control from authored MML through driver/synthesis semantics into rendered performance.

The point is not to declare either system a dependency. The point is to force the current model to survive a real piece in both directions.

## Control A: Wizball title music

### Why this object is unusually strong

Martin Galway published the source for his 1980s C64 music specifically so people could read, analyze and understand his music players and how he worked:

- https://github.com/MartinGalway/C64_music

The repository README states that `Wizball` used his first-generation player design, in use from 1984 through roughly mid-1987.

The source file itself identifies:

```text
Wizball Audio Source File (SID/65xx system)
Design, code, music & arrangements by Martin Galway
Work started 10th February 1987
```

Source:

- https://github.com/MartinGalway/C64_music/blob/main/wizball.asm

This gives a rare alignment of:

```text
author identity
+ driver source
+ music data
+ synthesis/control code
+ creator interview language
+ later listener/reviewer discourse
```

### Exact execution facts visible in source

The title screen entry is documented as a 200 Hz music path. The tune table assigns the title music three concurrent streams:

```text
TITLE0
TITLE1
TITLE2
```

The source is not simply a note list. The title streams contain, among other things:

- note/rest data;
- loop/for-next control;
- calls and jumps;
- instrument/state loads (`FLoad`);
- filter changes;
- pitch/frequency program changes (`Freq`);
- per-voice state pokes;
- modulation enable/disable;
- frequency-modulation counters/delays/gains;
- pitch-modulation state;
- offset-list frequency behavior;
- envelope/release state;
- repeated shared musical subroutines;
- eventual jumps back to the stream heads.

The player code separately implements per-voice pitch modulation and frequency modulation, including continuous updates to the SID frequency registers. The driver therefore preserves a distinction among:

```text
note / duration program
!= instrument / voice definition
!= continuous pitch/frequency motion
!= envelope / release behavior
!= filter behavior
!= physical SID register realization
```

That distinction matters because Galway's own descriptions of the music emphasize sound and motion, not merely melody.

### The title is structurally more than one repeated phrase

All three title streams change behavior internally before looping.

`TITLE0`, for example, does not retain one static voice configuration from beginning to end. It changes frequency programs, modulation delay/gain state, release/envelope state, and later loads a different voice/state definition before moving into shared phrase material and returning to the head.

`TITLE1` and `TITLE2` likewise change offset-list programs, pitch-modulation values and voice/state definitions over their run before looping.

This means a whole-song description such as:

```text
the piece changes character later
```

can potentially be supported by actual coordinated state changes across the three streams rather than merely by a different sequence of note numbers.

The stronger empirical task is to locate the audible boundary and quantify which musical/synthesis dimensions change together.

### Creator discourse is an answer key, not ground truth for every interpretation

Galway discussed the title music in a C64.com interview. He described a recurring trait in his larger pieces: sections could last a minute or two and then change completely in character, calling the Wizball title music a particularly clear example.

He also described the latter material as something some listeners heard as mysterious, magical and unconventional, while personally judging it weaker than the first part.

Source:

- https://www.c64.com/interviews/galway_part_2.html

In a separate 2001 interview he described Wizball as having a great `sound`, described much of his C64 work as having a sound-design bent, and characterized his style through flowing and smooth tones.

Source:

- https://remix64.com/interviews/interview-martin-galway.html

Important evidence distinction:

```text
Galway says he worked toward a sound
= documentary creator evidence

source contains modulation / filter / envelope behavior
= exact executable evidence

system concludes those controls help create a flowing or breathing impression
= musical/perceptual interpretation
```

The last step must be demonstrated, not silently promoted from the first two.

### Reviewer/listener discourse provides independent vocabulary

Later descriptions repeatedly use natural perceptual/critical language rather than register-level vocabulary.

Examples include descriptions of the title as:

- eerie;
- ethereal;
- mysterious;
- changing dramatically;
- having a clear changeover into a different/mysterious ending.

Useful observatories:

- https://vgmpf.com/Wiki/index.php?title=Title_-_Wizball_%28C64%29
- https://remix64.com/articles/featured-merman-wizball.html
- https://remix64.com/reviews/review-galway-remixed.html
- https://pocketmags.com/retro-gamer-magazine/issue-280/articles/wizball

These descriptions do not prove acoustic mechanisms. Their value is that they provide a human-language target against which a grounded analysis can be evaluated.

### First natural-language target

The project should be able to produce something in the neighborhood of:

```text
The first part feels more like one long, flowing idea. Later it turns into something stranger and more suspended; it is not just a new melody, the voices themselves start behaving differently.
```

That sentence is intentionally not yet a final analytical claim. The empirical job is to split it into supported subclaims:

```text
"one long, flowing idea"
→ recurrence / continuity / phrase and modulation evidence

"later it turns"
→ aligned structural boundary

"stranger / more suspended"
→ listener/reviewer hypothesis requiring perceptual support

"voices themselves start behaving differently"
→ exact changes in voice definitions, modulation, envelope/filter/control state
```

The project should then be able to descend from each phrase into the source.

### What would count as failure

Wizball fails the current model if any of the following happens:

1. the analysis can list note/channel changes but cannot identify the large audible character change Galway and reviewers describe;
2. the analysis calls the character change merely a `section boundary` without explaining what a listener hears changing;
3. the human-facing description sounds like a feature report rather than music discussion;
4. the natural description cannot descend into source/support evidence;
5. the system parrots Galway's words and then reverse-engineers convenient evidence rather than independently locating the behavior;
6. physical SID channels are treated as stable musical-role identities without evidence;
7. continuous modulation/envelope/filter behavior is discarded during note extraction.

## Control B: MUCOM88

### Why this is the inverse control

MUCOM88 was created by Yuzo Koshiro as an MML compiler/music-production environment for NEC PC-8801 systems using Yamaha FM hardware. The open project contains the compiler/player environment and original source lineage.

Sources:

- https://github.com/onitama/mucom88
- https://www.ancient.co.jp/~mucom88/

The English README describes MUCOM88 as:

```text
MML authored source
→ compiler
→ Z80 FM driver
→ playback / composer feedback
```

This is exactly the direction VGM Tooling usually has to reconstruct in reverse.

### Channel capability is explicit

The current open source defines eleven logical channels:

```text
MUCOM_CH_FM1    0
MUCOM_CH_PSG    3
MUCOM_CH_RHYTHM 6
MUCOM_CH_FM2    7
MUCOM_CH_ADPCM  10
MUCOM_MAXCH     11
```

Source:

- https://github.com/onitama/mucom88/blob/master/src/cmucom.h

Under the sequential A-K MML channel naming used by the supplied sample this yields the useful source-relative mapping:

```text
A-C   first FM group
D-F   PSG group
G     rhythm
H-J   second FM group
K     ADPCM
```

This is not a universal MML ontology. It is a validated MUCOM88 adapter fact.

### Koshiro's supplied MML retains authored musical/realization semantics

`package/sampl1.muc` is attributed directly to Yuzo Koshiro and contains the authored MML rather than a reverse-engineered note dump.

Source:

- https://github.com/onitama/mucom88/blob/master/package/sampl1.muc

The file contains all eleven A-K channels and exposes examples of:

- explicit tempo;
- octave and default-length state;
- note/rest/tie material;
- loops and alternate endings;
- instrument/voice numbers;
- volume;
- gate/articulation values;
- modulation commands;
- detune/pitch-related controls;
- channel-specific setup;
- rhythm/ADPCM use;
- repeated authored patterns.

Representative source fragments include controls such as:

```text
@78
v9
l16
q4
M20,1,12,4
S0,0,8,8
H4,4,0
D-18
p2 / p3
```

The exact semantics of each command must be taken from the MUCOM88 dialect/compiler, not guessed from generic MML conventions.

The important representation lesson is already clear:

```text
MML note identity
+ authored channel identity
+ instrument selection
+ articulation / gate state
+ modulation / pitch controls
+ program structure
```

exist together before device execution.

A reverse path that converts the result into only pitch/onset/duration would throw away information the author explicitly wrote.

### Practitioner evidence explains why the language and driver cannot be separated casually

Koshiro has repeatedly described his workflow as programming as well as composing.

In a 2001 interview he explained that he modified MML for his own purposes so he could customize everything from the sounds themselves to phrase construction, and could recreate missing hardware capabilities in software.

Source:

- https://shmuplations.com/yuzokoshiro/

In a later interview he described the original MML as heavily modified toward something more assembly-like and said he used it for the Bare Knuckle/Streets of Rage games.

Source:

- https://games.kikizo.com/features/yuzo_koshiro_iv_oct05_p2.asp

In a Korg interview he described game-music programming as not very different from programming a game because he needed to create a sound driver, and discussed FM synthesis directly in terms of operators, carriers/modulators, envelopes and waveform behavior.

Source:

- https://www.korg.com/uk/features/artists/2022/0131/

In a PC-88/Ys interview he described FM as having personality and described native computer output as harder/cooler than contemporary CD transfers.

Source:

- https://shmuplations.com/ys/

This supports an important project rule:

```text
composition discourse
+
sound-programming discourse
+
tool/driver discourse
```

may all describe one person's continuous creative process without becoming one undifferentiated data layer.

### Why the supplied sample is not enough by itself

`sampl1.muc` is an excellent authored-source control, but it is not itself evidence for the historical source of a particular 1980s/1990s game cue.

The Ancient MUCOM88 site separately publishes a sample-MML archive containing historical/example material. That archive should be treated as a next acquisition target when its contents can be retrieved and licensed/provenanced precisely.

Until then:

```text
MUCOM88 environment / compiler facts
= exact project/source facts

sampl1.muc authored semantics
= exact relative to that supplied sample

claims about a specific historical game cue's original MML
= not established by this pass
```

## Paired-direction validation

These two objects provide complementary pressure:

```text
WIZBALL
surviving machine/program source
→ recover performance / structure
→ recover heard musical behavior
→ natural discourse

MUCOM88
surviving authored musical source
→ compile / execute
→ device realization
→ audio
```

The long-term validation loop should meet in the middle:

```text
AUTHORED-SOURCE CONTROL
known authored semantics
→ compile / execute
→ capture lower evidence
→ run inverse analysis
→ compare recovered claims against authored truth

EXECUTABLE-SOURCE CONTROL
known driver / music program
→ execute / render
→ analyze song-level behavior
→ compare natural description with independent creator/reviewer/listener discourse
```

This is stronger than testing either a transcription model or a language model alone.

## Human discourse validation matrix

A single supported passage should be expressible in several registers without changing the evidence underneath it.

Example target shape:

```text
LISTENER
It gets stranger here and feels less grounded.

REVIEWER
The second stretch drifts away from the more direct first section into something eerier and less settled.

COMPOSER / MUSICIAN
The later section gets its contrast from different voice behavior and longer, more suspended gestures.

PRODUCER
The change works because the arrangement stops giving you the same kind of forward push and lets the voices hang differently.

ENGINEER / SOUND PROGRAMMER
The voice programs and modulation/envelope behavior change, so the contrast is in the sound and articulation as well as the notes.

FORENSIC
At the aligned boundary, enumerate the exact TITLE0/TITLE1/TITLE2 program, voice, modulation, envelope/filter and device-state changes with provenance.
```

These sentences are **test targets**, not assertions that the current implementation has already measured every listed behavior.

## What this pass changes

No new generic graph primitive is justified yet.

The pressure instead strengthens four existing obligations:

1. **whole-song alignment** must compare several layers at the same musical time;
2. **programmed expression** must survive note/event extraction;
3. **authored semantics** must remain available when a source exposes them;
4. **human discourse** must be generated from support bundles and discourse context rather than feature-to-adjective lookup rules.

## Next empirical step

For Wizball:

1. obtain/replay a trusted SID or assemble the published source;
2. capture the three SID voices plus filter/global state at high temporal resolution;
3. align execution positions to `TITLE0`, `TITLE1`, `TITLE2` and shared subroutines;
4. detect the major first-part/second-part transition independently of Galway's description;
5. compare note-role, register, density, articulation, modulation, envelope, waveform, filter and repetition behavior across the boundary;
6. write the natural listener-level description **before** looking again at creator/reviewer language;
7. then compare the independently generated description with Galway and critical/listener descriptions;
8. make every phrase descend into its support bundle.

For MUCOM88:

1. compile a supplied `.muc` with the original/open compiler path;
2. retain MML source spans and compiled/executed events together;
3. capture FM/PSG/rhythm/ADPCM device state;
4. recover the musical-performance view from execution alone;
5. compare recovered events, parts, instrument/control changes and loops against the authored MML;
6. record exactly which authored distinctions are unrecoverable from lower layers.

That is the next useful pressure boundary. More ontology work should wait for a failure in one of these two controls.
