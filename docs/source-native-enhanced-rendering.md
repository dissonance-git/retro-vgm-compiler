# Source-native enhanced rendering

VGM Compiler's enhanced renderer is not a generic remaster, MIDI/SoundFont conversion, or new arrangement.

Its target is:

> **the highest-fidelity realization of the same executable musical idea that surviving evidence can support after relaxing implementation ceilings that are not part of the music's adopted identity.**

The accurate/reference renderer remains the scientific control. Historical precedents and literature belong under `research/rendering/`; this document owns only the durable rendering contract.

## 1. enhanced and spatial are orthogonal

User-facing playback keeps **enhanced** and **spatial** independently controllable.

```text
enhanced off + spatial off
  reference synthesis/reconstruction + protected historical stereo

enhanced off + spatial on
  reference synthesis/reconstruction + source-aware Omniphony presentation

enhanced on + spatial off
  source-native enhanced synthesis/reconstruction + protected stereo

enhanced on + spatial on
  source-native enhanced synthesis/reconstruction + source-aware Omniphony
```

```text
enhanced
  changes how a known source object is synthesized/reconstructed

spatial
  changes how causal source objects are presented after source rendering
```

Neither control may silently enable the other.

## 2. Evidence ladder

Use the strongest available basis for a less-constrained realization:

```text
A  documented creator intention / approved less-constrained realization
B  same-production higher-quality source/master/prototype/version
C  exact upstream sample/patch + known transformation into game asset
D  deterministic hardware limitation + identity-preservation test
E  cross-source/corpus inference
F  aesthetic enhancement hypothesis
```

Only `A` supports strong creator-intent wording by itself. `B` and `C` still require checking whether historical preparation became musically meaningful. `D` through `F` remain reversible experiments.

## 3. Preserve versus relax

Preserve unless source evidence establishes otherwise:

- note/event identity;
- authored/source timing, groove, and phrasing;
- logical-part relationships;
- patch/sample/instrument identity;
- articulation and modulation;
- source-specific expressive gestures;
- deliberate effect timing/function;
- structural density/arrangement;
- nonlinear/degraded behavior adopted into instrument identity.

Potential ceilings to relax independently when evidence permits:

- storage-driven sample degradation/quantization;
- low-quality interpolation;
- avoidable aliasing/imaging;
- limited arithmetic precision;
- DAC/reconstruction ceilings;
- output bandwidth;
- final summation precision/headroom;
- memory-limited echo/reverb realization;
- storage-driven truncation or crude loop preparation;
- physical-output defects not musically adopted.

Historical limitation is not synonymous with unwanted defect. A constraint can be a burden, compromise, compositional boundary, technique source, or part of the resulting instrument.

## 4. Device-native enhancement

Do not replace a native source family with a generic modern synth merely to improve fidelity.

### FM / PSG / PCM and related VGM devices

Preserve the source-described synthesis system and programmed identity. For YM2612, conceptually:

```text
same algorithm
same operator relationships
same envelopes / feedback / modulation
same notes / timing / articulation
same patch identity
→ candidate higher-quality realization
```

Relax numerical precision, bandwidth, DAC behavior, summation, or other ceilings one at a time. If a device artifact materially defines a patch, it remains part of that instrument until controlled evidence earns a safe replacement.

The intended result is a higher-quality descendant of the same instrument, not an unrelated pristine preset.

### SNES / S-DSP

The useful intervention occurs upstream of the finished 32 kHz bus when exact source voices are available.

```text
exact SPC/BRR source identity
+ pitch/envelope/loop/articulation history
+ routing/echo evidence
→ source-native reconstruction
→ optional proven upstream sample relation
→ preserve intentional preparation
+ relax unwanted loss
```

Preserve fractional pitch/phase trajectories, envelopes, loops, noise/pitch modulation, signed routing, and shared echo semantics.

When a proven pre-BRR source exists, do not automatically substitute it. Compare upstream source, game-prepared source, BRR representation, and shipped realization. The question is which historical changes were technical loss and which became part of the instrument.

Normal product playback uses 48 kHz for the live enhanced reconstruction/final playback path. This does not imply that original S-DSP output contained missing ultrasonic information. A 96 kHz route is an offline/research comparison unless controlled evidence shows a product-relevant benefit that survives the final output condition.

## 5. Source-native boundary

When exact native voices are observable, improve them before the constrained historical final mixer rather than trying to repair only finished stereo.

Do not route game music through MIDI/SoundFont/VST as the canonical intermediate merely because those ecosystems provide high-quality synthesis features. Such formats may remain interoperability/analysis projections.

Source-family memory/storage constraints are title/system-specific evidence. Do not universalize one budget across machines or games.

## 6. Validation

Every relaxed ceiling is a separate experiment.

For one candidate:

1. retain the exact/reference control;
2. state the changed historical ceiling;
3. state the evidence that relaxing it is allowed;
4. state identity features locked by the test;
5. render control and candidate under the same musical execution;
6. measure structural/acoustic differences;
7. listen under route-clean, level-aware conditions;
8. include a control where the historical artifact should remain when applicable;
9. keep only improvements that preserve musical/instrument identity and function.

Test the four playback quadrants independently:

```text
reference + stereo
reference + spatial
enhanced  + stereo
enhanced  + spatial
```

A regression in one quadrant cannot be hidden by an improvement in another.

## 7. Wording

Prefer `less-constrained realization`, `source-supported reconstruction`, `higher-fidelity realization`, `counterfactual source-native render`, or `candidate closer to documented creator intent` when those phrases match the evidence.

Do not claim `this is what the composer intended`, `this is the original uncompressed version`, or `this is how the music was meant to sound` unless direct evidence supports that exact statement.

> **Recover intention where evidence exists, relax unwanted implementation ceilings where identity survives, and preserve constraints that the music adopted as part of itself.**
