# Historical constraint friction and counterfactual rendering

Status: active research input  
Purpose: distinguish hardware/toolchain limitations that creators treated as unwanted production friction from limitations they deliberately incorporated into the musical identity, and use that distinction to bound source-native enhanced rendering

## Why this case exists

The enhancement target is not generic modernization and not a conversion to SoundFont, MIDI, VST, orchestral instrumentation, or a different arrangement.

The target is:

> **the highest-fidelity realization of the same executable musical idea that the surviving evidence can support, after relaxing limitations that were implementation ceilings rather than part of the intended musical identity.**

This is a counterfactual problem.

```text
surviving authored / driver / synthesis evidence
        ↓
identify what the music is trying to do
        ↓
identify which hardware/toolchain constraints shaped the result
        ↓
separate integral constraints from unwanted ceilings
        ↓
relax only the latter
        ↓
source-native enhanced realization
```

The project must not assume that every limitation was unwanted. Some composers explicitly exploited artifacts, compression, aliasing, channel scarcity, chip nonlinearities, crude echoes, or tiny samples because those behaviors became part of the instrument they were composing for.

The historical record contains both kinds of evidence.

## This is different from the earlier practitioner pass

The earlier research asked:

- how composers and sound programmers actually worked;
- how composition and programming fed back into one another;
- how machine-specific controls became musical expression;
- how human practitioners described their creative process.

This pass asks a narrower question:

> **Which parts of that workflow did creators describe as burdens, compromises, unwanted restrictions, or things they were happy to leave behind?**

That negative evidence is important because it gives enhanced rendering permission to improve some aspects without pretending every low-fidelity artifact was sacred.

## Strong practitioner evidence of unwanted friction

### Harumi Fujita: manual sound-data programming was a burden

In a 2011 interview, Fujita contrasted earlier dedicated-chip work with later sampled/PCM production and described not having to program all sound data manually as a major burden removed from the workflow.

She also described the cartridge version of `Pulstar` as a sound-quality problem that Yasuaki Fujita spent substantial time trying to improve, while higher-quality source material existed for the CD version and later `Blazing Star` work.

Durable inference:

```text
manual chip-data entry
can be workflow friction

cartridge/sample-rate degradation
can be a quality ceiling rather than musical intent
```

Source:

- https://shmuplations.com/harumifujita/

### David Wise: assemble / wait / edit loops consumed enormous time

Wise has described his NES/SNES process as typing pitch/length/control data in hexadecimal, assembling it, waiting for playback, editing, and repeating the cycle potentially hundreds of times for one tune.

He explicitly says this saved memory but took a great deal of time.

That is strong evidence that the **workflow cost itself** was not the artistic goal.

At the same time, Wise is also a crucial counterexample later in this document because he deliberately used some of the resulting SNES artifacts as timbral material.

Sources:

- https://comicbook.com/gaming/news/donkey-kong-country-david-wise-interview/
- https://www.musicconnection.com/feature-story-flashback-video-game-composers-speak/

### Koji Kondo: sound programming eventually became too complex

Kondo described Super Famicom composition as continuously constrained by memory and said he used programming tricks to make songs compressible.

By the N64 era, he no longer handled sound programming himself because the work had become too complex; a specialist took over so he could focus on composition.

Durable inference:

```text
composer-programmer integration
can be creatively powerful
!= evidence that every low-level production task was desirable
```

Source:

- https://shmuplations.com/kojikondo/

### Sega of America: pre-GEMS tooling was effectively unusable

David Javelosa described Sega of America's pre-GEMS tool situation as relying on Japanese tools that local staff could not access plus contractor tools that were, in his characterization, essentially unusable.

GEMS was developed partly to leverage MIDI and make Genesis music production accessible to a much broader group of musicians and sound designers.

This matters because GEMS's compromises cannot be interpreted only as aesthetic preference. Part of its historical function was to escape an extremely hostile toolchain.

Source:

- https://segaretro.org/Interview%3A_David_Javelosa_%282025-05-07%29_by_Alexander_Rojas

### Tommy Tallarico: low-level NES sound-driver workflow was hostile to musicians

In surviving interview material quoted by preservation sources, Tallarico called the NES sound driver extremely difficult from a musician's perspective and emphasized that, before tools such as GEMS, game audio often required the composer to be a programmer.

Even when the exact wording is informal, the underlying workflow fact is consistent with the broader historical record: MIDI/editor front ends were adopted partly because raw driver entry imposed a large non-musical burden.

Navigation source:

- https://forums.nesdev.org/viewtopic.php?p=72806

### Barry Leitch: documentation and memory competed directly with musical effects

Leitch described working with SNES material whose manuals were largely inaccessible to him because they were in Japanese, forcing trial and error.

He discovered the SNES echo system empirically and then had to constrain the delay to one sixteenth-note because the echo buffer consumed precious audio RAM.

The musical timing of the echo was an artistic solution; the memory scarcity that forced the maximum available delay was an implementation constraint.

Source:

- https://www.gamedeveloper.com/audio/interviewing-veteran-composer-barry-leitch-part-i-sound-chips-from-zx-81-to-the-snes-

## Strong evidence of music being cut or altered to fit

### Yuzo Koshiro: shorten the song rather than degrade the sampling

In a Streets of Rage interview, Koshiro described songs being shortened because of memory limits. He explicitly preferred shortening material to reducing sample quality.

He also expressed strong enthusiasm for Mega CD because it would allow him to create game music with his home synthesizers instead of depending entirely on onboard chips.

This is unusually useful counterfactual evidence because it reveals a creator's **priority ordering under constraint**:

```text
preserve sample realization quality
> preserve every section of the longer arrangement
```

It does not mean every Mega Drive chip sound was unwanted. Koshiro elsewhere praised direct chip control and the distinctive expressive possibilities of the hardware.

Source:

- https://shmuplations.com/sormusic/

### ActRaiser team: sample quality versus memory was a direct production problem

The ActRaiser interviews describe keeping samples both small and clear as a major challenge. Live-instrument PCM could sound natural, but using more or longer samples rapidly worsened the memory problem.

The team explicitly framed the soundtrack as not representing the ultimate limit of what they wished to do on the machine.

Source:

- https://shmuplations.com/quintet/

### Manabu Namiki: removing notes to fit

Namiki described mobile game work where the available memory was so small that the BGM could only fit by removing notes from the songs.

That is a clean example of an implementation limit directly deleting authored musical information.

Source:

- https://shmuplations.com/basiscape/

### Grant Kirkhope: sample phrases cut apart to fit cartridge memory

Kirkhope described constructing GoldenEye's Bond material from only a few sampled phrases because a longer continuous phrase would not fit the available cartridge memory budget.

Again, the recombination may become part of the shipped identity, but the reason for the fragmentation was storage pressure, not an abstract musical preference for shorter source recordings.

Source:

- https://www.gamedeveloper.com/audio/how-i-goldeneye-i-s-composer-used-tech-limitations-to-create-an-iconic-sound

## Direct evidence for "closer to my original intention"

### Masahiko Ishida / Irem

Ishida described later CD recordings of originally mono PCB music as an opportunity to move the music somewhat closer to his original compositional intention. He used additional available channels for a restrained stereo effect and delay while deliberately avoiding a radical rewrite.

This is one of the strongest historical analogues for VGM Tooling's enhanced mode:

```text
same composition
same recognizable realization
more available technical capacity
        ↓
restrained enhancement toward documented intent
```

Source:

- https://shmuplations.com/rtypeishida/

### Cartridge/CD and preserved higher-quality sources

Harumi Fujita's Pulstar/Blazing Star comments provide another valuable pattern:

```text
constrained cartridge realization
↔ higher-quality CD/source material
```

When both versions come from the same production lineage, the less-constrained version can be evidence about what the constrained realization was approximating.

It still does not automatically authorize replacing every cartridge-specific characteristic.

## Counterexamples: some constraints became the instrument

A useful enhanced renderer must preserve the historical cases where the limitation itself was intentionally exploited.

### David Wise / Donkey Kong Country

Wise adapted Korg Wavestation-style wave sequencing to the SNES specifically because short waveforms could create rich evolving sounds within the tiny audio-memory budget.

He also reported finding **desirable** distortion/harmonic artifacts when truncating single-cycle waveforms to save memory.

Those artifacts are not safely classed as accidental degradation once the composer hears them, values them, and composes around them.

The correct target is therefore not:

```text
identify original synthesizer preset
→ substitute pristine preset
```

It is closer to:

```text
identify source lineage
+ preserve the programmed transformation that made the SNES instrument distinctive
+ improve only the unwanted reconstruction ceiling
```

Sources:

- https://www.squareenixmusic.com/features/interviews/davidwise.shtml
- https://www.thesoundarchitect.co.uk/davidwise/

### Howard Drossin / Genesis

Drossin described Genesis synthesis limitations as enjoyable and puzzle-like rather than simply frustrating.

That demonstrates why project policy cannot infer intent from technical scarcity alone.

Source:

- https://www.gamedeveloper.com/game-platforms/interview-game-musician-drossin-from-i-sonic-i-to-i-afro-samurai-i-

### Yuzo Koshiro / direct chip control

Koshiro repeatedly valued the precision and flexibility of direct sound-chip programming and customized MML, sometimes preferring it to MIDI because it exposed deeper control.

For Streets of Rage 3 he described the work less as being trapped within limitations and more as fully exploiting the peculiarities of the Mega Drive synthesizer.

Source:

- https://shmuplations.com/yuzokoshiro/
- https://shmuplations.com/sormusic/

## Restoration is not automatically intention recovery

Community source-sample restorations are extremely useful observatories because they can recover:

- original sample-library provenance;
- pre-BRR or pre-downsampled recordings;
- original synth presets;
- source sequence data;
- transformation chains used to create the game-ready sample.

But replacing a game sample with the pristine source is not automatically a historical restoration.

The SNES music-restoration debate demonstrates why:

```text
original source sample
!= automatically intended final timbre
```

A composer may have selected, truncated, filtered, looped, pitch-shifted, layered, or otherwise manipulated a source specifically for how the **degraded result** behaved on the target hardware.

Useful community/source-restoration observatories include work around Super Mario World, Chrono Trigger, Donkey Kong Country and broader source-sample identification communities.

Navigation sources:

- https://arstechnica.com/gaming/2021/02/super-high-fidelity-mario-the-quest-to-find-original-gaming-audio-samples/
- https://www.nintendolife.com/news/2022/11/random-musician-restores-chrono-trigger-soundtrack-using-uncompressed-samples
- https://www.vice.com/en/article/super-nintendo-music-does-not-need-restoration/

The Ars Technica reporting contains a particularly useful description from restoration practitioners: using original sequence data and uncompressed instruments can approximate a pre-console/demo realization, but the result is intentionally not always one-to-one with the shipped console mix.

That is exactly the distinction VGM Tooling must represent explicitly.

## Constraint classification

For enhancement decisions, classify an observed limitation by evidence rather than by intuition.

### 1. Documented unwanted constraint

Examples:

- creator says the workflow was burdensome;
- creator says sound quality was degraded by the platform;
- creator says notes/sections had to be removed;
- creator explicitly prefers a less-constrained version;
- creator describes later mix as closer to original intention.

Enhancement permission: **strong**, within the scope of the documented complaint.

### 2. Paired less-constrained source evidence

Examples:

- same-team CD version;
- surviving studio/render source;
- original pre-compression sample;
- original synth patch plus exact transformation chain;
- prototype or development render before final compression.

Enhancement permission: **strong to moderate**, depending on whether the relation to the shipped work is proven.

### 3. Mechanically identifiable implementation ceiling

Examples:

- BRR quantization;
- low source sample rate;
- poor interpolation;
- DAC quantization;
- limited accumulator precision;
- aliasing caused by a specific historical numerical path;
- storage-driven loop truncation;
- fixed low-resolution parameter steps.

Enhancement permission: **conditional**. First test whether the artifact materially participates in the instrument identity.

### 4. Adapted constraint

The creator clearly composed around the limitation but there is no evidence they valued the resulting artifact itself.

Enhancement permission: **careful**. Preserve the musical adaptation while testing whether the physical ceiling can be relaxed.

### 5. Integral constraint / adopted artifact

Evidence shows the creator deliberately used the artifact or limitation as part of the sound.

Examples:

- intentionally useful waveform truncation artifacts;
- characteristic programmed fake echo;
- channel scarcity deliberately used for rhythmic interruption;
- chip nonlinearity deliberately exploited in patch design.

Enhancement permission: **preserve identity first**. Improve fidelity around the feature rather than erase it.

### 6. Unknown

No reliable evidence about intention.

Enhancement permission: **reversible experiment only**.

## Counterfactual-realization evidence ladder

When VGM Tooling proposes a less-constrained realization, record the strongest basis available.

```text
A  documented creator intention / paired creator-approved realization
B  same-production less-constrained source or master
C  exact upstream sample/patch + known game transformation chain
D  deterministic hardware limitation with stable perceptual identity test
E  cross-source statistical inference
F  purely aesthetic enhancement hypothesis
```

Higher letters are not automatically bad. They simply require weaker wording and stronger A/B discipline.

Never label `E` or `F` as "what the composer intended".

## Genesis / YM2612 implication

The target is **not** to replace YM2612 music with generic modern instruments.

The target is:

```text
same FM patch topology and musical controls
+ same operator relationships
+ same algorithm / envelopes / modulation / automation
+ same note / timing / articulation behavior
        ↓
higher-quality FM realization
```

Candidate ceilings to test separately include:

- internal numerical precision;
- phase/frequency resolution;
- aliasing and image rejection where not identity-bearing;
- DAC path quality for PCM;
- reconstruction/interpolation quality;
- final summation precision/headroom;
- output bandwidth;
- historically accidental analog/output-stage defects.

But chip-specific behavior that materially defines the patch must survive.

The enhanced target should sound like an extremely capable descendant of the same FM instrument, not like a DX7 preset replacement and not like an orchestral re-arrangement.

## SNES / S-DSP implication

The target is not `SPC -> MIDI -> SoundFont`.

SoundFont/DLS/VST ecosystems remain useful **observatories** for understanding sample-based synthesis, multisampling, envelopes, modulation, bank mapping, interpolation, and high-quality realtime rendering. They are not the planned playback backend.

For SNES, candidate enhancement routes include:

```text
exact BRR/sample identity
+ exact pitch/envelope/control history
+ exact loop / articulation behavior
        ↓
source-provenance recovery when available
        ↓
higher-quality sample reconstruction / interpolation
        ↓
higher-precision per-voice synthesis and mixing
        ↓
higher-quality realization of the same echo/environment intent
```

When an original pre-BRR source sample is identified, it can become a strong enhancement candidate **only after** comparing how the shipped BRR transformation changed its musical identity.

Do not merely swap the pristine sample in.

A useful model is:

```text
UPSTREAM SAMPLE
        ↓ historical edit / truncation / filter / loop / pitch / BRR encode
GAME INSTRUMENT
        ↓ programmed envelope / pitch / echo / mix
SHIPPED SOUND

ENHANCED PATH
upstream evidence
+ preserve intentional transformations
+ relax unwanted degradation
        ↓
COUNTERFACTUAL SOURCE-NATIVE REALIZATION
```

This is closer to what a modern high-end sampler or VST could achieve **while still playing the same SNES instrument design**, rather than importing a generic modern instrument.

## Memory accounting nuance

Do not use one universal historical number such as "the soundtrack had 56 KB" across systems.

For SNES, the S-SMP/S-DSP audio subsystem has 64 KiB of local RAM. That space can be contested by driver/code/data, sample storage, sequence state, and echo buffering. Graphics live elsewhere; they do not literally occupy SPC700 audio RAM.

The cartridge ROM has a separate whole-game storage budget that may also constrain how much music/sample data can be stored or swapped.

For Mega Drive/Genesis, cartridge ROM allocation among code, graphics, music data, samples and other assets is game-specific.

Therefore record actual per-game budgets when known rather than importing a generic "4 MB cart / N KB sound" assumption.

The user's underlying design point still holds:

> **music was often allotted only a small fraction of the total production resources, and the enhanced renderer should not reproduce those allocation ceilings merely because they were historically necessary.**

## Literature context

Research on early game audio consistently treats platform hardware/software as both constraint and affordance rather than one or the other.

Useful references include:

- Karen Collins, `In the Loop: Creativity and Constraint in 8-bit Video Game Audio`;
- Karen Collins, `Game Sound`;
- Kevin R. Burke, `Hard Limitations and Soft Possibilities`;
- Kevin Driscoll and Joshua Diaz, `Endless loop: A brief history of chiptunes`.

The important methodological lesson is compatible with the practitioner evidence:

```text
constraint
can simultaneously be
production burden
+ compositional boundary
+ source of technique
+ eventual aesthetic identity
```

VGM Tooling must decide at the level of the specific musical object and transformation, not at the level of platform nostalgia.

## Validation rule

Every relaxed limitation gets its own test.

Do not change all of these together:

```text
sample quality
interpolation
FM precision
bandwidth
DAC reconstruction
mixing precision
echo realization
stereo presentation
```

For each change:

1. preserve the accurate/reference path;
2. state which historical ceiling is being relaxed;
3. state the evidence that the ceiling was unwanted or safely relaxable;
4. preserve all higher-priority identity constraints;
5. A/B against the reference;
6. test known integral-artifact controls where the change should *not* erase the original behavior;
7. retain the change only if it improves listening quality without making the work feel like a different arrangement/instrument system.

## Project law

The enhanced renderer is not an emulator with prettier output and not a remastering plugin after stereo summation.

It is an alternate source-native realization of the same executable musical work.

The safe formulation is:

> **Recover intention where evidence exists, relax unwanted implementation ceilings where identity survives, and preserve the constraints that the music actually adopted as part of itself.**
