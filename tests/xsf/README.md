# xSF test boundaries

These tests protect container/effective-object and bounded structural claims for PSF1, GSF, USF, 2SF, and NCSF.

GSF tests treat the 12-byte little-endian entry/load/size header as a GBA
upload contract, preserve root-selected entry state and byte provenance, and
keep zero-filled gaps unknown. They do not equate GSF with MP2K or the
effective upload image with an original ROM.

NCSF tests require an empty or four-byte reserved sequence selector and an
empty or structurally valid SDAT program. They expose INFO/SSEQ/PLAYER/bank/
wave-archive references without claiming NCSF equals 2SF or that an NCSF SDAT
is an original complete cartridge SDAT. Same-work comparisons name each
observable and preserve non-comparability instead of asserting state identity.

For PSF1 AKAO work:

```text
literal signature != sequence
structural candidate != exact AKAO version
raw opcode bytes != decoded event
parsed event != runtime use
driver state != physical voice state unless the mapping is demonstrated
```

Permanent Chrono Cross adversarial cues for late AKAO behavior include:

- `114 Shadow Forest.psf`
- `119 Hydra Marshes.psf`
- `302 Chronopolis.psf`
- `102 The Brink of Death.psf`
- tuning-sensitive `Dragon God` material
- sequence/instrument edge cases such as `Dream's Creation`

Historical VGMTrans evidence left the Chrono-specific `FE 13` operation unknown. The pinned same-game `jdperos/chrono-cross-decomp` reconstruction now gives a concrete driver behavior: `FE 13` marks the logical channel so its saved active note is not re-keyed when suspended music resumes.

The test boundary therefore changes from `FE 13 semantics unknown` to:

```text
raw FE 13 bytes != decoded FE13 event
known FE13 driver semantics != proof that the event executed on a runtime path
```

The same-game driver also distinguishes:

```text
C6/C7 -> logical FM flags -> allocated physical voice mask -> SPU PMON
D4/D5 -> software playback-rate sidechain using previous logical channel SampleRate
D6/D7 -> software pitch-to-volume sidechain using that logical source coordinate
```

Tests must not collapse those mechanisms merely because all can affect pitch or modulation.
