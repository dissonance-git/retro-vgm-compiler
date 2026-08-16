# vgm2mid as an interoperability influence

## Status

Research input for symbolic projection and eventual external-tool feedback.

`vgm2mid` is useful to this project, but not as an architectural center. It is a strong historical example of a narrower transformation:

```text
VGM device execution
        ↓
chip-specific pitch / gate / timing reconstruction
        ↓
MIDI approximation
```

Game Music Interpreter is trying to solve a larger problem:

```text
native source / execution
        ↓
format-specific semantic truth
        ↓
persistent musical parts
        ↓
motifs / phrases / harmony / form / creator grammar
        ↓
optional projections
MIDI • notation • tracker rows • analysis JSON • tool-specific annotations
```

The important relationship is therefore:

> **learn from vgm2mid's hard-won chip conversion knowledge, but treat MIDI as one lossy projection of a richer musical model.**

---

## What vgm2mid teaches us

The maintained `vgm2mid` lineage is historically valuable because it accumulated practical conversion knowledge for many VGM chips. Public documentation and maintainer discussion describe especially useful work on SN76489, OPL, OPN/YM2612 and YM2151 conversion, along with more limited support for several other devices.

Useful lessons include:

1. **Chip clocks and native pitch formulae matter.**
   Paul Jensen's early work explicitly derived real-frequency formulae from chip clock and F-number state rather than relying on coarse hand transposition.

2. **A register-frequency estimate is not always the musical fundamental.**
   Later maintainer discussion notes that FM operator multipliers and modulation can make two identical nominal frequency settings sound at different effective octaves. This is exactly why Game Music Interpreter keeps device-native pitch, nominal device frequency, programmed pitch, performed pitch and heard pitch as separate claims.

3. **PCM/DAC percussion is structurally hostile to naive MIDI conversion.**
   A VGM can contain streamed sample writes rather than authored drum-note events. Turning individual DAC writes into MIDI events can create dense garbage-note clouds. Recovering the underlying percussion event requires source/sample segmentation or higher musical evidence.

4. **MIDI tempo is a projection choice.**
   `vgm2mid` can choose a BPM and scale note lengths so playback duration remains compatible. That is useful for editing, but it demonstrates that exported MIDI meter/tempo need not be original authored meter/tempo.

5. **Different chips deserve different conversion logic.**
   A generic `frequency -> MIDI` converter is not enough. Device semantics, envelopes, key state, channel modes, samples, software vibrato and driver behavior alter what can safely be inferred.

These are all compatible with the current project's evidence-first model.

---

## Where the current project is already deeper

`vgm2mid` asks approximately:

> What MIDI notes can reproduce this captured chip activity usefully?

Game Music Interpreter asks:

> What musical object best explains this source, what evidence supports that interpretation, what survives representation change, and which information would be lost by projecting it into MIDI?

That distinction matters in several places.

### Persistent parts

A physical YM2612 or S-DSP voice is not automatically a musical part. The current model can carry identity across physical-slot reuse only when evidence supports it.

### Relative musical grammar

Motifs can be compared by interval, contour and normalized rhythm without first inventing absolute MIDI note names.

### Phrase and form

Repeated/transformed material and converging part boundaries can support phrase structure before a complete score transcription exists.

### Harmony

The harmonic layer requires absolute musical pitch evidence and an explicit tuning projection. It does not call raw FNUM, S-DSP pitch rate or simultaneous register state a chord.

### Creator grammar

Composer attribution operates above these musical relations and remains separated from arrangement/programming/toolchain fingerprints.

---

## The correct role for MIDI

MIDI should be an **export adapter and cross-representation witness**, not the master representation.

A symbolic projection should carry at least:

- source node(s);
- persistent-part identity when known;
- onset and duration with source time provenance;
- pitch role: programmed / performed / heard;
- absolute frequency evidence when known;
- MIDI note only when an explicit tuning projection earns one;
- residual tuning error in cents;
- articulation / velocity / controller mappings when supported;
- instrument/timbre correspondence type rather than false equality;
- confidence;
- loss declarations.

Examples of loss declarations:

```text
pitch_quantized_to_12tet
timing_quantized
meter_imposed_for_export
timbre_collapsed_to_program
fm_operator_structure_lost
sample_identity_lost
physical_channel_identity_lost
persistent_part_unknown
percussion_event_uncertain
software_modulation_simplified
loop_or_branch_flattened
```

The exported MIDI can then be very useful without being mistaken for the source truth.

---

## vgm2mid as an oracle/control

When both systems can process the same VGM, `vgm2mid` is useful as a historical comparison oracle.

For each supported chip/work:

```text
VGM
 ├─> vgm2mid output
 └─> Game Music Interpreter symbolic projection
```

Compare:

- note onset agreement;
- note-off/duration agreement;
- octave disagreements;
- pitch residuals;
- channel/part splits;
- percussion recovery;
- tempo/meter projection;
- instrument changes;
- controller/modulation behavior.

Disagreement is not automatically a Game Music Interpreter failure. It becomes an experiment:

```text
which system is closer to the authored/native semantics,
and what evidence decides it?
```

Source code, driver data, known transcriptions or authored sequence formats can serve as hidden oracles where available.

---

## The feedback loop

The long-term ecosystem should be bidirectional.

### Phase 1: other tools teach us

Use existing tools as observatories and controls:

- vgm2mid / vgm2midi;
- smps2mid and driver-specific converters;
- VGM analysis/editing tools;
- tracker importers/exporters;
- disassemblies and driver source;
- sequence players/editors;
- emulator instrumentation.

Extract generalizable chip knowledge and failure cases without importing their representational assumptions wholesale.

### Phase 2: Game Music Interpreter becomes the richer semantic engine

Recover:

```text
parts
motifs
phrases
harmony
form
arrangement roles
creator-facing grammar
```

while preserving provenance and uncertainty.

### Phase 3: feed the research back outward

Adapters can then improve existing workflows:

```text
Game Music Interpreter
        ├─> richer MIDI export
        ├─> tracker reconstruction
        ├─> annotated VGM playback
        ├─> SMPS/GEMS analysis hints
        ├─> notation / MusicXML-like export
        ├─> DAW markers and part labels
        ├─> disassembly annotations
        └─> external tool libraries / patches
```

The goal is not to replace every specialist tool. It is to let specialist tools consume a musical interpretation deeper than any one source representation can provide.

---

## Design law

```text
IMPORT KNOWLEDGE, NOT ASSUMPTIONS.
EXPORT INTERPRETATION, WITH LOSSES DECLARED.
```

A converter may be excellent at one projection while still being unable to represent the full musical object. The project should preserve that asymmetry explicitly.

## Sources consulted

- VGMRips wiki, `vgm2mid`: historical authorship/maintenance, supported VGM revisions, operational caveats.
- SMS Power forum, Paul Jensen, *Sound chip frequencies, FNum and Hz, vgm2mid v0.35 later today*: early YM2413/PSG/YM2612 frequency derivation discussion.
- VGMRips forum, *Updated vgm2mid?*: ValleyBell and other developers discussing FM frequency modifiers, octave errors and tool updates.
- VGMRips forum, *vgm2mid Messed Up Drum Track*: PCM/DAC-to-MIDI failure mode.
- VGMRips forum, *VGM Tracker Import*: ValleyBell discussion of per-chip conversion quality and BPM projection behavior.
- `mechanicheart/vgm2mid`: Visual Basic port exposing OPN register/FNUM/BLOCK mapping and current documented conversion limitations.
