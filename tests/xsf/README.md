# xSF test boundaries

These tests protect container/effective-object and bounded structural claims for PSF1, USF, and 2SF.

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
