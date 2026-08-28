# Upstream and reference evidence registry

This research owner records **why external sources matter and what claim classes they can constrain**. It is not the operational dependency lockfile and must not duplicate exact build pins already owned by import manifests, workflows, patch guards, or build tooling.

Durable semantic law lives in [`../../docs/architecture.md`](../../docs/architecture.md). Immutable imported package identities live in [`../../imports/MANIFEST.md`](../../imports/MANIFEST.md). Exact source revisions required by a build belong with the build/patch owner that verifies them.

A reference is not automatically a runtime dependency:

```text
primary specification / source / measurement
        ↓
source-family implementation evidence
        ↓
independent implementation controls
        ↓
project-owned semantic model
```

## Evidence classes

- **Format authority:** primary specifications such as the VGMRips VGM specification constrain byte, header, command, timing, loop, data-block, and version semantics.
- **Hardware authority:** official manuals, application notes, schematics, development material, and measured hardware constrain device behavior and exposed mechanisms.
- **Implementation controls:** libvgm, Nuked cores, ymfm, SPCPlay/SNESAPU, MAME, QSound implementations, and similar projects provide independent executable comparison surfaces.
- **Container/platform controls:** psflib, GSF/USF/2SF/NCSF players and tools constrain xSF envelope/dependency behavior while preserving platform-specific execution boundaries.
- **Driver/source controls:** SMPSPlay, GEMSPlay, VGMTrans, Hoot, preservation tooling, and source ledgers constrain driver identity, sequence/instrument/sample data, and executable behavior.
- **Execution-to-music controls:** SPC/VGM-to-MIDI and related recovery tools expose recoverable musical projections without making MIDI the project ontology.
- **Representation observatories:** see [`music-representation-systems.md`](music-representation-systems.md) and [`openmusic-libraries.md`](openmusic-libraries.md) for upper-layer pressure tests.
- **Literature:** papers contribute established distinctions and falsifiable experiment ideas, not imported ontology.

## Use law

For every external source:

1. Record the question it helps answer.
2. Prefer primary evidence when available.
3. Keep conceptual evidence separate from imported implementation.
4. Inspect licensing before reuse and preserve required attribution.
5. Keep source-specific mechanisms source-specific.
6. Put exact mutable pins in the operational owner that verifies them, not here.
7. Promote a cross-system lesson only after independent evidence supports the same relation.

```text
shared frontend != shared semantic depth
register resemblance != shared musical law
reference implementation != format specification
conceptual usefulness != permission to copy
```

The purpose of this registry is evidence routing and re-entry, not a second dependency database or architecture document.


## Focused source-family re-entry maps

When a task needs implementation-level detail rather than the broad evidence classes above, use the focused maps instead of reconstructing the quarry from conversation history:

- [SNES / SPC upstream reference map](../formats/snes/snes-spc-upstream-reference-map.md)
- [VGM / Genesis upstream reference map](../formats/vgm/vgm-genesis-upstream-reference-map.md)

These maps record what each project can constrain, what it cannot establish, and which older repository commits already contain deeper experiments or integration work. They remain research routing documents, not dependency lockfiles.

## Curated GitHub return shelf

The focused source-family maps above remain the deep quarry owners. This section is the **small project-local shelf** of GitHub repositories worth remembering across sessions because they can save a future deep dive or provide a discriminating control.

The revision below is the revision actually inspected for the stated evidence. It is **not** an operational dependency pin unless a build/import/patch owner separately pins it.

### VGM, Genesis, and executable playback

| Repository | Inspected revision | Inspected surface | Return for | Not authority for | Revisit when |
| --- | --- | --- | --- | --- | --- |
| [`ValleyBell/libvgm`](https://github.com/ValleyBell/libvgm) | [`e41ca80220cbcbaac0e7d77bf57c689a395c5f97`](https://github.com/ValleyBell/libvgm/commit/e41ca80220cbcbaac0e7d77bf57c689a395c5f97) | player and device integration; current main; VGM timing and replay architecture | VGM and VGZ playback architecture; device/core integration; timing, resampling, and controlled source observers | VGM format specification; original driver or authored-source truth | libvgm changes VGM timing, observer, resampling, or device integration; VGM Compiler adds a new VGM source observer |
| [`nukeykt/Nuked-OPN2`](https://github.com/nukeykt/Nuked-OPN2) | [`335747d78cb0abbc3b55b004e62dad9763140115`](https://github.com/nukeykt/Nuked-OPN2/commit/335747d78cb0abbc3b55b004e62dad9763140115) | README.md; YM3438 and YM2612-compatible clocked core | cycle-level OPN2 disputes; undocumented YM3438 behavior; accuracy differential against higher-level FM cores | SMPS or GEMS command semantics; authored music structure | OPN2 timing or output behavior changes; a YM2612 disagreement needs a die-level control |
| [`aaronsgiles/ymfm`](https://github.com/aaronsgiles/ymfm) | [`81aec25ccbb98f4873a255f7551ac4dadac59b4a`](https://github.com/aaronsgiles/ymfm/commit/81aec25ccbb98f4873a255f7551ac4dadac59b4a) | README.md; OPN family implementation including YM2612, YM3438, and YMF276 | cross-family Yamaha comparisons; readable FM operator/register abstractions; Enhanced OPN2 descendant experiments | full digital pipeline accuracy; driver semantics | ymfm changes OPN-family behavior; VGM Compiler reopens Enhanced OPN2 or cross-chip abstraction work |
| [`mamedev/mame`](https://github.com/mamedev/mame) | [`49e9a1f7c999ea1cbe8a715cb1cea2c2ed4f7c78`](https://github.com/mamedev/mame/commit/49e9a1f7c999ea1cbe8a715cb1cea2c2ed4f7c78) | README.md; machine-level emulation framework; current 2026 main | bus and machine context around sound hardware; cross-system hardware comparisons; full-machine controls when isolated cores are insufficient | independent Yamaha arithmetic when it reuses ymfm; creator intent | a sound question depends on machine integration rather than an isolated device; MAME changes the relevant machine or sound integration |
| [`ValleyBell/SMPSPlay`](https://github.com/ValleyBell/SMPSPlay) | [`2d7deabb284345eb919a8f215d556bf257707d9b`](https://github.com/ValleyBell/SMPSPlay/commit/2d7deabb284345eb919a8f215d556bf257707d9b) | Readme.txt; driver configuration and command tables; VGM logging path | known-answer SMPS forward execution; driver-specific command, modulation, envelope, drum, and DAC behavior; source-to-VGM blind reconstruction controls | universal Genesis driver semantics; composer identity | SMPSPlay adds or fixes a driver dialect; VGM Compiler tests a new SMPS source family or inverse-recovery claim |
| [`ValleyBell/GEMSPlay`](https://github.com/ValleyBell/GEMSPlay) | [`8b2e490942e9e8c0cc8d6f9132f15c5b8507a5db`](https://github.com/ValleyBell/GEMSPlay/commit/8b2e490942e9e8c0cc8d6f9132f15c5b8507a5db) | Readme.txt; instrument/envelope/sequence/sample bank loading; VGM logging | same-hardware negative control against SMPS; GEMS source-bank semantics; GEMS forward execution to VGM | SMPS semantics; general Genesis musical ontology | GEMS parsing or playback changes; VGM Compiler adds a GEMS corpus or driver-generalization test |
| [`sonicretro/smps-rips`](https://github.com/sonicretro/smps-rips) | [`d7a49cb91932c99db013b880639f9d9eecb287a5`](https://github.com/sonicretro/smps-rips/commit/d7a49cb91932c99db013b880639f9d9eecb287a5) | README.md; raw ROM-ripped songs, samples, instruments, and driver configuration files | source-native Genesis corpus pressure; driver dialect identity; prototype and revision comparisons; answer keys for source reconstruction | composer attribution by itself; universal SMPS semantics | new games or prototype/source material are added; VGM Compiler widens source-level Genesis corpus pressure |
| [`sonicretro/skdisasm`](https://github.com/sonicretro/skdisasm) | [`044fa46725c71187399e13f5ddb70e11d32dc024`](https://github.com/sonicretro/skdisasm/commit/044fa46725c71187399e13f5ddb70e11d32dc024) | README.md; sound directory; byte-perfect build checks | Sonic 3 driver and sound-data archaeology; revision-specific source truth; attribution implementation-layer controls | composition authorship without independent evidence; other SMPS revisions | sound-driver or sound-data labels materially change; Sonic 3 attribution needs a new implementation discriminator |
| [`Ivan-YO/vgm2smps`](https://github.com/Ivan-YO/vgm2smps) | [`c943601073fac66477e81e5c8307aae20e67734d`](https://github.com/Ivan-YO/vgm2smps/commit/c943601073fac66477e81e5c8307aae20e67734d) | README.md; VGM to Sonic 1 SMPS conversion behavior | inverse-lowering competition; failure cases for note, duration, DAC, and instrument reconstruction; plausible-source versus exact-source boundary tests | exact authored-source recovery; general SMPS dialect support | VGM Compiler starts source reconstruction from VGM; a newer inverse-SMPS implementation appears |
| [`vgmtrans/vgmtrans`](https://github.com/vgmtrans/vgmtrans) | [`3e16daae49d42246f2d1b302b04f6e80a8037342`](https://github.com/vgmtrans/vgmtrans/commit/3e16daae49d42246f2d1b302b04f6e80a8037342) | README.md; format scanner; sequence/instrument/sample conversion surface | driver-aware recovery across SNES, PlayStation, GBA, DS, arcade, and other families; negative tests against pure device-log inference; sequence/instrument/sample representation design | universal source ontology; historical truth beyond supported parser evidence | VGMTrans adds a source family relevant to the VGM Compiler frontier; a new driver-aware recovery layer is designed |
| [`tildearrow/furnace`](https://github.com/tildearrow/furnace) | [`426dba17d52830374cb3489d6878547cb04cf053`](https://github.com/tildearrow/furnace/commit/426dba17d52830374cb3489d6878547cb04cf053) | README.md; multi-chip tracker architecture; large supported-chip surface | authored tracker program versus device execution comparisons; cross-chip control semantics; shared-frontend negative controls; chip-state rendering experiments | historical source for unrelated games; one universal game-music semantic model | Furnace adds relevant source or export behavior; VGM Compiler needs an authored multi-chip control |
| [`Wohlstand/libOPNMIDI`](https://github.com/Wohlstand/libOPNMIDI) | [`200260e83339294ab7d1d9761151b94d63e269bf`](https://github.com/Wohlstand/libOPNMIDI/commit/200260e83339294ab7d1d9761151b94d63e269bf) | README.md; multi-core OPN synthesis; full-panning option | fixed musical workload across several OPN emulator cores; FM patch-bank experiments; continuous panning and source-native enhancement controls | original game driver or patch provenance; historical YM2612 source truth | VGM Compiler reopens Enhanced OPN2 or Genesis surround work; libOPNMIDI changes panning or supported OPN cores |
| [`libgme/game-music-emu`](https://github.com/libgme/game-music-emu) | [`fe8da4b6d3876d7542c2fb69d94487e19836d678`](https://github.com/libgme/game-music-emu/commit/fe8da4b6d3876d7542c2fb69d94487e19836d678) | README.md; common API across NSF, SPC, VGM and other executable music formats | common-frontend versus source-family-specific semantics; portable playback architecture; voice-muting and timing controls across multiple systems | deep driver semantics for every supported format; shared musical ontology | VGM Compiler adds a common playback facade; Game Music Emu materially changes a relevant backend or common contract |

### SNES / SPC execution and archaeology

| Repository | Inspected revision | Inspected surface | Return for | Not authority for | Revisit when |
| --- | --- | --- | --- | --- | --- |
| [`dgrfactory/spcplay`](https://github.com/dgrfactory/spcplay) | [`5f1774a999c5c66192c2836071a3564f9708294a`](https://github.com/dgrfactory/spcplay/commit/5f1774a999c5c66192c2836071a3564f9708294a) | README.md; SNESAPU DSP controls documented in VGM Compiler map; develop branch | S-DSP reference playback; main-only versus echo-only differential renders; Gaussian versus sinc and FIR controls; source-tap validation | proof that optional high-quality modes are historical hardware behavior; musical part identity | SNESAPU DSP behavior or controls change; VGM Compiler changes SPC dry/wet or interpolation ownership |
| [`blarggs-audio-libraries/snes_spc`](https://github.com/blarggs-audio-libraries/snes_spc) | [`ec8ee2bbe30451614c1d02a83f7af1c97d497d45`](https://github.com/blarggs-audio-libraries/snes_spc/commit/ec8ee2bbe30451614c1d02a83f7af1c97d497d45) | README.md; full SPC emulator; standalone accurate DSP | S-DSP timing and mixer oracle; dry-voice tap placement; exact full-state replay; voice-mute differential controls | cleaner interpolation as historical truth; driver-track identity | an SPC timing or mixer disagreement appears; VGM Compiler changes runtime capture boundaries |
| [`gocha/split700`](https://github.com/gocha/split700) | [`1a055bd4496fbdba09ef58498ccfdfb4cabccaef`](https://github.com/gocha/split700/commit/1a055bd4496fbdba09ef58498ccfdfb4cabccaef) | README.md; SRCN selection; BRR loop-point extraction | BRR sample identity; SRCN-to-encoded-sample provenance; sample-library archaeology; decoded-waveform comparison | performed voice identity; pitch, envelope, routing, or echo state | VGM Compiler resumes original-sample provenance work; a stronger BRR provenance extractor appears |
| [`nesdev-org/MesenCE`](https://github.com/nesdev-org/MesenCE) | [`af2342818ca81a93b346146802047760400fc7b1`](https://github.com/nesdev-org/MesenCE/commit/af2342818ca81a93b346146802047760400fc7b1) | README.md; debugger, trace logger, profiler, watch and savestate-capable multi-system emulator | reproducible machine-state bundles; driver archaeology across NES, SNES, GB, GBA, PC Engine and other systems; trace and debugger controls around disputed audio events | source-native musical identity; independent truth for every emulated device | VGM Compiler adds a source family supported by MesenCE; debugger or trace capabilities materially change |
| [`aikiriao/spc2midi-tsuu`](https://github.com/aikiriao/spc2midi-tsuu) | [`501be0e31608e6b297f81c059f1ca06670d113c5`](https://github.com/aikiriao/spc2midi-tsuu/commit/501be0e31608e6b297f81c059f1ca06670d113c5) | README.md; SPC to MIDI conversion; current pitch-bend handling | execution-to-note projection prior art; MIDI information-loss controls; pitch-bend and retrigger boundary tests | MIDI as source ontology; exact persistent musical-part identity | VGM Compiler changes its performed-note projection; spc2midi-tsuu adds a relevant execution-to-symbolic mapping |
| [`Raikaru/ghidra-spc700`](https://github.com/Raikaru/ghidra-spc700) | [`af628ce1a8e86ae1382dcec4c03d4412e013890c`](https://github.com/Raikaru/ghidra-spc700/commit/af628ce1a8e86ae1382dcec4c03d4412e013890c) | README.md; SPC700 SLEIGH language; APU analysis workflow; semantic smoke tests | SPC700 driver disassembly; control-flow and data-flow archaeology; repeatable Ghidra-based source reconstruction | S-DSP audio accuracy; musical meaning of recovered code | VGM Compiler needs a new SPC driver reconstruction; the Ghidra module gains materially stronger analysis semantics |

### NES authored-source control family

| Repository | Inspected revision | Inspected surface | Return for | Not authority for | Revisit when |
| --- | --- | --- | --- | --- | --- |
| [`bbbradsmith/nsfplay`](https://github.com/bbbradsmith/nsfplay) | [`6af5406e3325b5507bea1ae1a57c77d5efe5c7f3`](https://github.com/bbbradsmith/nsfplay/commit/6af5406e3325b5507bea1ae1a57c77d5efe5c7f3) | distribute/nsfplay.txt; CPU logging controls; per-device and per-channel configuration | future NES execution oracle; CPU sound-write traces; APU and expansion-chip differential controls; loop and channel-state observability | authored tracker/source truth; universal NES driver semantics | VGM Compiler opens an NSF/NSFE source-family gate; NSFPlay changes logging or expansion-audio behavior |
| [`Dn-Programming-Core-Management/Dn-FamiTracker`](https://github.com/Dn-Programming-Core-Management/Dn-FamiTracker) | [`6660b0e5fdbebda9742a75cd01116d708d90d1c4`](https://github.com/Dn-Programming-Core-Management/Dn-FamiTracker/commit/6660b0e5fdbebda9742a75cd01116d708d90d1c4) | README.md; text import/export; NSF2/NSFe export; per-channel WAV export | authored pattern-to-driver-to-NSF controls; NES source truth for cross-representation teaching; expansion-audio authored semantics | historical source for games not authored in FamiTracker; general NSF execution truth | VGM Compiler opens an NES authored-source experiment; Dn-FamiTracker changes export or source representation |

### Musical representation, attribution, and evaluation

| Repository | Inspected revision | Inspected surface | Return for | Not authority for | Revisit when |
| --- | --- | --- | --- | --- | --- |
| [`cuthbertLab/music21`](https://github.com/cuthbertLab/music21) | [`93b49debc1b3b6c6548b4d3aedb588a0b216be9f`](https://github.com/cuthbertLab/music21/commit/93b49debc1b3b6c6548b4d3aedb588a0b216be9f) | README.md; Stream object model described in published work | hierarchical and flat symbolic views; context-preserving symbolic transforms; music-theory analysis controls | game-source execution truth; one canonical VGM Compiler representation | VGM Compiler adds or revises hierarchical musical representations; music21 introduces a relevant analysis or representation mechanism |
| [`CPJKU/partitura`](https://github.com/CPJKU/partitura) | [`427ff875bd5a49a0eec894fdd7c6631ed7f597ea`](https://github.com/CPJKU/partitura/commit/427ff875bd5a49a0eec894fdd7c6631ed7f597ea) | README.md; Score object; score-to-performance alignment surface | score versus performance mappings; voice separation controls; symbolic timing and performance alignment; cross-representation teaching | device execution or driver truth; proof that a recovered score is authored source | VGM Compiler adds score/performance alignment; persistent-part or voice-separation work needs an external symbolic control |
| [`urinieto/msaf`](https://github.com/urinieto/msaf) | [`ee0dae1460941cbf677dce7f0c8f5c3e81c5b66c`](https://github.com/urinieto/msaf/commit/ee0dae1460941cbf677dce7f0c8f5c3e81c5b66c) | README.md; structure-analysis framework; systematic algorithm exploration | independent section/segment hypotheses; structure-analysis baselines; audio-derived boundary controls | phrase syntax authority; source-native structure or creator intent | VGM Compiler evaluates section or form segmentation; MSAF gains a relevant structure algorithm |
| [`DDMAL/jSymbolic2`](https://github.com/DDMAL/jSymbolic2) | [`0c465281d836f6ab6c66ca8762fa8811ebac730f`](https://github.com/DDMAL/jSymbolic2/commit/0c465281d836f6ab6c66ca8762fa8811ebac730f) | README.md; dependency-aware feature extraction; published composer-attribution use | creator-grammar and attribution baselines; encoding-bias controls; large symbolic feature inventories as adversarial comparators | authorship proof; relational musical explanation; source-native game execution | VGM Compiler resumes creator attribution or feature-bias testing; a newer jSymbolic implementation materially changes the feature surface |
| [`mir-evaluation/mir_eval`](https://github.com/mir-evaluation/mir_eval) | [`fe73b3533737814f83dbd9739f06e90f5f82f758`](https://github.com/mir-evaluation/mir_eval/commit/fe73b3533737814f83dbd9739f06e90f5f82f758) | README.rst; beat/chord/hierarchy/melody/pattern/segment/separation/transcription metric modules | independent scoring of analysis projections; baseline evaluation definitions; falsifiers for melody, pattern, segmentation, hierarchy, separation, and transcription claims | musical ontology; source truth; a model that generates the analysis | VGM Compiler adds a benchmarkable MIR projection; mir_eval changes a metric used by a receiving experiment |

### Return-shelf law

- Re-enter the focused Genesis/SPC maps when the shelf row points toward a source-family question; do not expand this table into a second deep map.
- Preserve exact/derived/hypothesis boundaries when borrowing upstream behavior.
- A player, emulator, reverse-engineering tool, symbolic toolkit, or MIR metric remains evidence for only the claim class named above.
- Refresh upstream when its revisit trigger fires or when the receiving VGM Compiler contract changes materially.
- Helix may retain a few cross-project mechanisms discovered here, but **VGM Compiler owns this detailed GitHub evidence shelf**.
