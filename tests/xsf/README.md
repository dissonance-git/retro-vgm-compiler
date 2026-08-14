# xSF test boundaries

These tests protect container/effective-object and bounded structural claims for PSF1, USF, and 2SF.

For PSF1 AKAO work:

```text
literal signature != sequence
structural candidate != exact AKAO version
raw opcode bytes != decoded event
parsed event != runtime use
runtime state != SPU correspondence
```

The permanent Chrono Cross adversarial cues for unresolved AKAO v3.2 behavior are:

- `114 Shadow Forest.psf`
- `119 Hydra Marshes.psf`
- `302 Chronopolis.psf`

VGMTrans explicitly identifies a Chrono Cross-only `FE 13` sub-opcode in those cues while leaving its semantics unknown. Tests and audits must preserve that uncertainty rather than inventing an interpretation.
