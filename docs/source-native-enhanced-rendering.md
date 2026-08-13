# Source-native enhanced rendering

VGM Tooling's enhanced renderer is not a generic remastering stage, a MIDI/SoundFont conversion path, or a new arrangement of old game music.

Its target is:

> **the highest-fidelity realization of the same executable musical idea that the surviving evidence can support, after relaxing implementation ceilings that are not themselves part of the music's adopted identity.**

The accurate/reference renderer remains the scientific control.

## Counterfactual realization

Historical game music often passed through severe production constraints:

```text
composer / sound programmer
        ↓
authored musical and synthesis intent
        ↓
limited tools / driver grammar / manual data entry
        ↓
limited storage / RAM / channels / sample bandwidth / arithmetic / effects memory
        ↓
shipped executable realization
```

Enhanced rendering asks what can be safely relaxed **without changing the work into another arrangement or instrument system**.

This is not the same as assuming that every historical limitation was bad.

A limitation may have been:

- an unwanted production burden;
- a storage or quality compromise;
- a boundary the composer adapted around;
- a source of useful technique;
- an artifact deliberately adopted into the final instrument identity.

The renderer must determine which case applies at the level of the specific source object.

See `../research/cases/historical-constraint-friction-counterfactual-rendering.md` for practitioner evidence.

## Evidence ladder

Use the strongest available basis for a less-constrained realization:

```text
A  documented creator intention / creator-approved less-constrained realization
B  same-production higher-quality source, master, prototype or CD version
C  exact upstream sample/patch plus known transformation into the game asset
D  deterministic hardware limitation with an identity-preservation test
E  cross-source or corpus inference
F  purely aesthetic enhancement hypothesis
```

Only `A` supports strong wording about creator intention by itself.

`B` and `C` can strongly support reconstruction, but they still require checking whether the transformation into the shipped asset became musically meaningful.

`D` through `F` remain reversible experiments.

## Preserve versus relax

Always preserve unless the source itself proves otherwise:

- note/event identity;
- exact or source-authored timing relationships;
- groove and phrasing;
- logical part relationships;
- patch/sample/instrument identity;
- programmed articulation;
- programmed modulation and automation;
- source-specific expressive gestures;
- deliberate effect timing and musical function;
- structural density and arrangement;
- important nonlinear or degraded behavior that the instrument was designed around.

Possible ceilings to relax when evidence supports it:

- source-sample degradation caused only by storage pressure;
- BRR/PCM quantization losses;
- low-quality interpolation;
- avoidable aliasing or imaging;
- limited arithmetic precision;
- DAC reconstruction quality;
- output bandwidth;
- final summation precision and headroom;
- memory-limited echo/reverb realization;
- storage-driven truncation or crude loop preparation;
- physical-output defects not used as part of patch identity.

## Mega Drive / Genesis

The enhanced YM2612 target is not a generic modern synth.

Conceptually:

```text
same FM algorithm
same operator ratios / levels / envelopes
same modulation / feedback / LFO behavior
same notes / timing / articulation
same programmed patch identity
        ↓
higher-quality source-native FM realization
```

The experiment may relax selected hardware ceilings separately, such as numerical precision, bandwidth, reconstruction, DAC quality, or summation.

But if a patch depends on a YM2612-specific artifact, nonlinearity, quantization behavior, or modulation interaction for its identity, that behavior remains part of the instrument until listening/evidence proves a safe higher-quality equivalent.

The intended perceptual result is closer to:

> the best possible descendant of the same FM instrument

than:

> replace the Genesis patch with a pristine unrelated synthesizer preset.

## SNES / S-DSP

The SNES path is especially suitable for source-native reconstruction because the shipped instruments are already sample based.

The intended route is:

```text
exact SPC / BRR sample identity
+ exact pitch / envelope / loop / articulation history
+ exact echo / routing behavior
        ↓
upstream sample provenance when available
        ↓
identify historical edits / truncation / filtering / looping / BRR encoding
        ↓
preserve intentional transformations
+ relax unwanted degradation
        ↓
higher-quality realization of the same instrument design
```

If an original pre-BRR sample is discovered, do not automatically replace the game sample with it.

Instead compare:

```text
upstream source sample
↔ game-prepared sample
↔ BRR-encoded sample
↔ shipped render
```

The important question is which changes were only technical loss and which changes became part of the instrument.

A modern high-end sampler or VST is a useful **quality analogy**, not the planned backend. The renderer remains source-native.

## SoundFonts, DLS, MIDI and VST ecosystems

These are research observatories.

They help expose established ideas about:

- multisampling;
- key/velocity zones;
- envelopes;
- modulation;
- filters;
- pitch control;
- bank/program identity;
- sample looping;
- realtime synthesis quality;
- effect routing;
- rendering precision.

They are not the canonical intermediate representation and are not the planned foobar playback engine.

```text
VGM / SPC / native driver
→ common musical meaning when justified
→ source-native enhanced renderer

not

VGM / SPC
→ MIDI
→ SoundFont
→ final answer
```

MIDI/SF2/DLS exports may still exist as projections for interoperability or analysis.

## Memory nuance

Do not universalize one memory figure across systems or games.

### SNES

The S-SMP/S-DSP audio subsystem has 64 KiB of local RAM. That space may contain or support:

- driver/code/data;
- sequence state;
- BRR samples;
- sample-directory structures;
- echo buffer/state;
- other engine-specific audio data.

Graphics do not literally consume this local audio RAM. Cartridge ROM is a separate whole-game storage constraint.

### Mega Drive / Genesis

Cartridge ROM is shared across the game and allocations among code, graphics, music data, PCM, tables and other assets are title specific.

Therefore VGM Tooling should record actual per-game resource evidence when available rather than assume one generic soundtrack budget.

The important project principle is broader:

> **historical audio was often forced to fit inside a small resource envelope; enhanced playback need not reproduce that envelope when the musical identity survives its removal.**

## Validation

Every relaxed ceiling is a separate experiment.

Do not simultaneously change sample reconstruction, interpolation, FM precision, DAC behavior, mixing, echo and stereo presentation and then call the result better.

For one proposed relaxation:

1. retain the exact/reference path;
2. state which historical ceiling is changing;
3. state the evidence that the ceiling is unwanted or safely relaxable;
4. state which identity features are locked;
5. render reference and candidate under the same musical execution;
6. measure structural/perceptual differences;
7. listen;
8. test at least one control where the corresponding historical artifact should remain;
9. retain only improvements that preserve recognition, phrasing, instrument identity and musical function.

## Historical-intent wording

Use precise language.

Prefer:

```text
less constrained realization
source-supported reconstruction
higher-fidelity realization
counterfactual source-native render
candidate closer to documented creator intent
```

Avoid claiming:

```text
this is what the composer intended
this is the original uncompressed version
this is how the music was meant to sound
```

unless direct evidence actually supports that claim.

## Project rule

> **Recover intention where evidence exists, relax unwanted implementation ceilings where identity survives, and preserve the constraints that the music actually adopted as part of itself.**
