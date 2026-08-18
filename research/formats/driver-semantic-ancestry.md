# Driver-semantic ancestry: recovering the cloud around the chip

Status: active architecture input, not a renderer-control specification.

The remaining gap between accurate chip execution and deep musical understanding is often upstream of the chip. A register trace can be exact while still omitting the authored command, driver state, tool convention, sample lineage, timing rule, or implementation quirk that caused the write. The compiler must therefore preserve semantic ancestry when it is available and preserve an explicit gap when it is not.

This is not permission to hard-code games, composers, genres, or named drivers into runtime behavior. Named historical material below is research evidence for generic distinctions.

## Evidence harvested

### Composer-facing drivers and tools are part of the musical production system

Hitoshi Sakimoto describes the sound-control code that became Terpsichorean as growing out of his own hardware-side programming, and later describes being responsible for both the sound driver and the sound data for *Revolter*. That history is useful because it collapses a modern assumption that composition, sequencing, driver programming, and hardware control always belonged to separate people or separate conceptual stages.

Source:

- Time Extension interview with Hitoshi Sakimoto, 2025-12-29: https://www.timeextension.com/features/interview-it-felt-very-computer-y-to-give-english-names-to-things-hitoshi-sakimoto-on-creating-his-famous-terpsichorean-sound-driver

The architectural consequence is narrow: documentary evidence can justify looking for an authored-program or driver-execution layer, but it is not runtime state. Documentary material must remain an external annotation unless source-native evidence connects it to a specific execution.

### SMPS retains semantics that flattened YM2612/SN76489 writes do not

The Sonic & Knuckles sound-driver disassembly exposes persistent track state above the chips, including retrigger suppression ("do not attack next note"), pitch-slide state, sustain-frequency state, transpose, modulation-envelope selection, voice/instrument references, note-fill timers, loop counters, a track stack, and FM3 special-mode state. The same source can optionally compile with sound-driver bug fixes disabled or enabled, making the distinction between historical behavior and repaired behavior explicit.

Sources:

- sonicretro/skdisasm, `Sound/Z80 Sound Driver.asm`: https://github.com/sonicretro/skdisasm/blob/master/Sound/Z80%20Sound%20Driver.asm
- sonicretro/smps-rips: https://github.com/sonicretro/smps-rips

The SMPS rip collection also demonstrates that "SMPS" is a family rather than one immutable language. Its metadata distinguishes driver types, tempo algorithms, instrument formats, available channels, coordination flags, drum maps, modulation envelopes, PCM samples, prototype variants, engine oddities, and in some cases fixed-bug song variants.

Therefore a future SMPS adapter should emit generic semantic evidence while preserving the source-native token or state that justified it. It must not make `SMPS` itself a musical feature.

### SPC700 music engines can contain a command language above S-DSP

Open SNES sound engines demonstrate that SPC700-side music data can explicitly encode note events, key-off, delays, volume, pan, detune, pitch slide, portamento, vibrato, instruments, and other commands. These are not reconstructible in full merely by observing the final S-DSP register state.

Example technical source:

- SNES GSS driver and documented music-data format: https://github.com/nathancassano/snesgss

This does not imply that every commercial SPC uses SNES GSS or one universal N-SPC grammar. It establishes the generic distinction between **driver command/state** and **DSP execution**. Commercial driver families must be recovered separately and provenance-labeled separately.

### Exact synthesis still requires hidden and cycle-level execution state

Blargg's `snes_spc` documents an accurate S-DSP emulator that resolves register and memory accesses at the exact SPC cycle rather than only at sample boundaries. Its implementation also retains internal state not represented by the obvious public register snapshot, including KON setup state, DSP phase, echo history, BRR decode position, and an obscure hidden envelope value used by one GAIN mode.

Source:

- blarggs-audio-libraries/snes_spc: https://github.com/blarggs-audio-libraries/snes_spc

This is a separate layer from driver semantics. The compiler must not confuse "the emulator needs hidden state to reproduce the waveform" with "that hidden state reveals the composer's intention." Both matter, but they answer different questions.

### Scholarship supports treating software constraints and bespoke drivers as musical affordances

James Newman's *Driving the SID chip: assembly language, composition and sound design for the C64* argues that bespoke low-level drivers and exploitation of chip behavior were inseparable from composition and sound design, including pseudo-polyphony, rapid arpeggiation, PWM, portamento, drum synthesis and sample playback.

Karen Collins' *In the Loop: Creativity and Constraint in 8-bit Video Game Audio* treats technical constraints and composers' responses to them as jointly shaping musical decisions rather than reducing the music to hardware specification alone.

Kevin Driscoll and Joshua Diaz's *Endless loop: A brief history of chiptunes* likewise frames early chip music through interacting hardware, software, and social practices rather than a purely technocratic chip description.

Sources:

- Newman: https://www.gamejournal.it/driving-the-sid-chip-assembly-language-composition-and-sound-design-for-the-c64/
- Collins DOI: https://doi.org/10.1017/S1478572208000510
- Driscoll & Diaz DOI: https://doi.org/10.3983/TWC.2009.096

These works are methodological support. They do not provide source-native facts for a particular VGM/SPC fixture and therefore cannot enter fixture-specific runtime control.

## Semantic ancestry contract

The shared execution graph already distinguishes:

```text
source representation
        ↓
authored program
        ↓
driver execution
        ↓
synthesis
        ↓
musical performance
        ↓
musical structure
```

`model/execution_semantic_provenance.h` makes the first three layers explicitly recordable as evidence. The generic vocabulary currently distinguishes:

```text
authored command
 driver command
 driver state
 articulation control
 pitch control
 timing control
 modulation control
 instrument reference
 sample reference
 phrase control
 channel mode
 implementation quirk
 information gap
```

Each observation separately records its origin:

```text
source-native
runtime-observed
deterministically reconstructed
external document
hypothesis
unavailable
```

The native token is preserved when one exists. An unavailable observation is forbidden from inventing one. External documents are automatically marked as external annotations. Missing ancestry is represented as a first-class gap instead of silently filling the hole with a plausible story.

## Why identical chip state is not enough

Two executions may reach the same visible YM2612 or S-DSP state through different upstream events. One note can be a fresh attack while another continues a tie or suppresses retriggering. A pitch value can be the authored note itself, a transposed note, a portamento intermediate, a modulation displacement, or a driver correction. An instrument/sample address can identify the currently selected resource without identifying where that resource originally came from.

The graph must therefore retain this shape when evidence permits it:

```text
upstream semantic A ─┐
                     ├─> synthesis state X
upstream semantic B ─┘
```

and this shape when evidence does not permit disambiguation:

```text
explicit semantic gap ─> synthesis state X
```

It must never silently reduce both cases to `synthesis state X => one musical intention`.

## Source-family implementation agenda

### Genesis / VGM

Use source-native driver artifacts when they exist to recover and attach:

- articulation and retrigger semantics;
- note/gate duration state;
- pitch-slide, detune, transpose and modulation ancestry;
- voice/instrument-bank references;
- loop/subroutine and phrase-control ancestry;
- driver timing/tempo algorithm and tick-domain mappings;
- special channel modes such as FM3 behavior;
- implementation quirks and known historical bugs.

A VGM capture that contains only chip writes should not claim these as observed. It should keep the synthesis truth exact and expose the missing upstream semantics as gaps or hypotheses.

### SPC

Recover driver command/state only where the engine can be identified and decoded from the SPC's executable/data state. Keep that evidence separate from the exact runtime S-DSP observations already used by the spatial observatory.

Priorities:

- note/retrigger/key-off ancestry;
- pitch, detune, slide, portamento and vibrato commands;
- instrument/sample-number references;
- driver tempo/tick rules and loop/subroutine structure;
- per-engine command tables and state machines;
- sample-directory and BRR identities, while keeping **resource identity** separate from **historical sample provenance**;
- implementation quirks that materially alter execution.

Do not assume all SPC engines share one command grammar.

## Sample provenance law

Three claims must stay separate:

```text
this voice references SRCN N
this SRCN resolves to this BRR byte sequence
this BRR sequence historically came from sample/source X
```

The first two may be source/runtime facts. The third generally needs independent provenance research. A byte-identical sample match may support a provenance hypothesis, but it is not automatically authorship or original-source proof.

## Timing law

Do not flatten timing into one global clock. Driver ticks, VGM source ticks, chip cycles, native sample frames, resampled output frames and musical beat positions are different domains. Existing `time_mapping` edges should connect them only when the mapping is known. Timer quirks, PAL/NTSC update differences, double-update behavior, scheduler jitter and tempo algorithms belong in those mappings or in explicit timing-control evidence.

## Bug and exploit law

Historical bugs are evidence, not dirt to scrub away before analysis.

When a driver bug or quirk is known:

1. preserve the observed historical behavior;
2. label the quirk and its provenance;
3. keep any repaired/counterfactual execution separate;
4. test whether musical structure depends on the quirk before deciding it is musically meaningful.

The compiler should be able to say "this pattern depends on historical driver behavior" without converting every bug into a compositional intention.

## Validation without GitHub Actions

`tools/run_core_tests.py` is the repository-owned fallback while hosted Actions are unavailable. It discovers every `*_test.cpp` under `tests/vgm`, `tests/spc`, and `tests/model`, compiles with strict C++17 warnings-as-errors, and executes each binary locally.

The new `tests/model/execution_semantic_provenance_test.cpp` is therefore automatically part of that local gate. It proves that two identical synthesis states can retain different driver-semantic ancestors, documentary evidence remains externally annotated, unknown intent is represented as a gap, and ancestry cannot be linked backward down the semantic ladder.

This validation route is deliberately local and deterministic. Hosted CI can later become another executor of the same contracts, not the owner of them.
