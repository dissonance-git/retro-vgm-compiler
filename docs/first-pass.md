# First implementation pass

This is the bootstrap handoff for the first local/Codex pass. It is intentionally **non-audible**. Do not begin enhancement DSP yet.

## Goal

Turn the repository from provenance + architecture into two buildable source trees while preserving the exact upstream starting points.

Work directly on `main` in the local repository, then push `main` when the pass is complete.

## 1. VGM source import

The exact supplied source archive is committed at:

`imports/foo_input_vgm.7z`

SHA-256:

`93d71695fdad062dee47aefa3f857683e4a057302d1a069958eecf5dd18c60ff`

Extract it **losslessly** into:

`components/vgm/foo_input_vgm/`

The archive contains a `src/foobar2000/foo_input_vgm/` prefix. The component root should be the directory containing `foo_input_vgm.sln`, `foo_input_vgm.vcxproj`, `LICENSE`, and `src/`.

Do not convert resource-file encodings merely to normalize the repository. Preserve bytes/line endings where practical on this initial import.

Verify that the extracted tree contains the MPL-2.0 `LICENSE` and the existing VGM, GYM, DRO, and S98 source files. VGM/VGZ remains the only active enhancement design center; the other handlers are compatibility code.

Record the imported tree hash/file count in `imports/manifest.md` after extraction.

## 2. SPC source import

There are two different things to preserve:

### Editable implementation base

Import the editable improved SNESAPU source from the current dgrfactory/spcplay `develop` lineage into a clearly named subtree under:

`components/spc/`

Do not replace it with the supplied DLL. Preserve GPL-2.0 and Alpha-II/degrade-factory attribution/provenance.

The source currently includes the APU, DSP, SPC700, SNESAPU interface, headers/includes, build support, and third-party compatibility implementation. Keep the upstream layout recognizable on first import.

### Foobar wrapper

Import the existing `foo_snesapu` foobar2000 wrapper source into its own recognizable subtree under:

`components/spc/`

Preserve its original license and provenance. Do not merge wrapper and emulator source into one undifferentiated tree.

### Newer behavioral reference

The separately supplied package `spcplay-2.21.3.9130.zip` is **not** the source baseline. It is the newer behavior/version reference.

Reference SHA-256:

`bd778800295dac01297934ade357a01b4326510ac335822e84615cdbe735123c`

Its contents are:

- `readme.txt` — 16,005 bytes
- `snesapu.dll` — 63,488 bytes
- `spcplay.exe` — 206,848 bytes

Do not architect the component around that binary DLL. The next SPC engineering obligation is to determine what the editable source lacks relative to the v2.21.3 reference and port those changes forward in source form.

If the exact supplied reference ZIP is available in the local working material, place it under `references/spcplay-2.21.3.9130/` or preserve a local reference outside the build tree; do not make the executable/player binary a runtime dependency of the foobar component.

## 3. Dependency provenance

For VGM, identify the exact libvgm revision expected by the supplied component project and document it in `docs/upstreams.md`.

Do not silently copy a random newer libvgm snapshot into the tree. If updating libvgm is desirable, first establish the supplied component's baseline and then make the update a separate reversible commit.

For SPC, record the exact dgrfactory/spcplay source commit imported.

## 4. Build discovery

Document the actual build requirements for each component:

- Visual Studio/toolset
- foobar2000 SDK version/range
- architecture targets
- WTL/PPUI or other support libraries
- libvgm/zlib requirements
- assembler/tooling required by SNESAPU

Do not restructure the projects merely to make them prettier before they build.

## 5. Baseline build

Attempt reproducible reference builds for both components.

If a dependency prevents building in the current environment, record the exact missing dependency and continue all non-blocked bootstrap work. Do not invent replacement code merely to force a green build.

No audible enhancement changes belong in this pass.

## 6. Baseline diagnostics only

Once builds are understood, identify but do not yet redesign the narrow realtime seams where future source state can be observed:

### VGM

Locate:

- file-command processing
- device creation/configuration
- register writes
- PCM/DAC stream control
- device/channel muting and panning
- emulator output/resampling
- final player render/mix boundary

### SPC

Locate:

- the eight DSP voice states
- BRR/sample identity and decode path
- pitch/interpolation state
- envelope/key state
- per-voice L/R level
- echo input/output/FIR/feedback state
- noise/pitch-modulation state
- final DSP mix/output boundary

Document these seams with file/function references in `docs/rendering-seams.md`.

Do **not** create a generalized shared source-state abstraction yet. First record what each engine actually exposes.

## 7. Completion state

This pass is complete when:

- both source trees are present with provenance intact
- exact imported revisions are recorded
- build requirements are known
- baseline build status is recorded honestly
- no enhancement audio behavior has been introduced
- `docs/rendering-seams.md` maps the future realtime intervention points
- the repository still follows `AGENTS.md`

Then the next pass can start the actual enhanced renderer from a trustworthy baseline.
