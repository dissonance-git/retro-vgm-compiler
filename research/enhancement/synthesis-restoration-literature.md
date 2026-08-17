# Source-native enhancement literature pass

Date: 2026-08-17

This note records the literature pass used to constrain the two current Enhanced targets:

1. SNES sampled voices should approach the strongest available estimate of the **pre-BRR production source**, not merely a sharpened 32 kHz final bus.
2. Genesis FM should behave like the same six-channel, four-operator composition executed on a **higher-ceiling Yamaha-FM descendant**, not like a preset conversion or an automatic six-operator rewrite.

The literature is used here as mechanism quarry and a source of failure modes. It does not override source-native evidence from the game, emulator, register stream, sample bytes, or documented production lineage.

## SNES: restoration is an inverse problem, but automatic playback must remain evidence-bounded

Relevant literature found through SciSpace:

- Y. Yamamoto, M. Nagahara, and P. P. Khargonekar, *Signal Reconstruction via H-infinity Sampled-Data Control Theory — Beyond the Shannon Paradigm*, IEEE Transactions on Signal Processing 60(2), 2012, DOI `10.1109/TSP.2011.2175223`.
  - Useful mechanism: reconstruction can be formulated around inter-sample behavior and a declared signal class rather than treating a sampled sequence as the only possible continuous realization.
  - Project translation: the BRR-decoded 32 kHz lattice is not automatically the quality ceiling when the underlying source trajectory is known more strongly.

- N. G. Paulter Jr., *A causal regularizing deconvolution filter for optimal waveform reconstruction*, IEEE Transactions on Instrumentation and Measurement 43(5), 1994, DOI `10.1109/19.328893`.
  - Useful mechanism: inverse reconstruction is ill-posed when the degradation operator or noise is uncertain, so stable restoration needs regularization and a stopping/selection rule.
  - Project translation: never treat an inverse-BRR or inverse-filter estimate as exact source merely because the transform returns a plausible waveform.

- E. Moliner, F. Elvander, and V. Välimäki, *Zero-Shot Blind Audio Bandwidth Extension*, 2023, DOI `10.48550/arXiv.2306.01433`.
  - Useful mechanism: generative priors can produce perceptually convincing high-frequency reconstruction from band-limited historical recordings.
  - Critical boundary: those frequencies are inferred by a prior. They are not recovered source truth.
  - Project translation: generative bandwidth extension is allowed only as a reversible listening/research projection. It must never silently become the normal source-supported SNES restoration path.

- M. Caetano and X. Rodet, *A source-filter model for musical instrument sound transformation*, ICASSP 2012, DOI `10.1109/ICASSP.2012.6287836`.
  - Useful mechanism: separating excitation/partials from spectral envelope can support perceptually coherent transformations.
  - Project translation: when an upstream instrument source is independently identified, validation may compare source trajectory and spectral-envelope behavior separately rather than relying on one full-band similarity score.

### SNES admission order

```text
exact identified pre-BRR source + exact preparation lineage
    -> normal Enhanced candidate

exact source after a documented game-preparation step
+ invertible/explicit coordinate map
    -> normal Enhanced candidate

BRR decoded source + deterministic higher-order interpolation
    -> enhanced reconstruction, but not "original source recovered"

regularized inverse estimate from BRR alone
    -> reversible experiment

generative / diffusion bandwidth extension
    -> reversible experiment only
```

The automatic path therefore remains conservative even if a more speculative method sounds better. Better listening quality is a user-facing result; stronger source identity is an evidence claim and needs its own support.

## Yamaha FM: preserve the patch semantics, raise the numerical ceiling

Relevant literature found through SciSpace:

- F. Holm, *Understanding FM Implementations: A Call for Common Standards*, Computer Music Journal 16(1), 1992, DOI `10.2307/3680493`.
  - Useful warning: nominally "FM" implementations can be incompatible enough that migrating a patch changes timbre.
  - Project translation: do not equate a DX7/FM-X/other Yamaha patch with a YM2612 patch merely because all are called FM. Operator graph, modulation convention, envelopes, scaling, feedback, and timing remain source semantics.

- V. Lazzarini and J. Timoney, *Theory and practice of higher-order frequency modulation synthesis*, Journal of New Music Research, 2024, DOI `10.1080/09298215.2024.2312236`.
  - Useful mechanism: higher-order FM/PM topologies and feedback can be formulated explicitly in discrete time, while implementation details matter to stability and spectrum.
  - Project translation: the source four-operator graph can be rendered at higher numerical precision without adding operators or changing algorithm topology.

- F. Caspe, A. McPherson, and M. Sandler, *DDX7: Differentiable FM Synthesis of Musical Instrument Sounds*, ISMIR 2022, DOI `10.48550/arXiv.2208.06169`.
  - Useful mechanism: FM timbre can be represented and resynthesized through compact operator parameters, but optimization can encounter ambiguity and loss-design problems.
  - Project translation: parameter preservation is stronger than fitting a replacement patch to the rendered audio. Learned patch conversion belongs in an experimental lane, not automatic Enhanced.

- F. Esqueda, V. Välimäki, and S. Bilbao, *Rounding corners with BLAMP*, DAFx 2016, and related antiderivative-antialiasing work.
  - Useful mechanism: nonlinear/discontinuous synthesis stages can create audible aliasing; oversampling is useful but is not the only or automatically sufficient antialiasing strategy.
  - Project translation: an oversampled HQ FM engine must include an explicit reconstruction/anti-alias stage rather than claiming that a box average alone makes modulation products safe.

### Automatic FM target

```text
same six physical musical channels
same four operators per channel
same source key events
same FNUM / BLOCK trajectory
same operator ratios + detune program
same TL + envelope program
same 8 algorithm topologies
same feedback program
same source LFO / CH3 semantics when supported
same authored stereo route

BUT

higher phase precision
higher sine/amplitude precision
higher internal accumulation headroom
no mandatory YM2612 output-ladder distortion
explicit anti-alias reconstruction
higher output bandwidth when the host rate permits it
```

This is intentionally closer to "the composer had a studio-grade Yamaha OPN descendant with the Genesis channel budget" than to "convert the song to DX7." The six-channel limitation is part of the composition and is preserved.

## Joint firewall

```text
higher quality != more notes
higher quality != more channels
higher quality != new instruments by default
higher quality != generative missing detail presented as source truth
higher quality != removing authored routing/effects
```

Enhanced changes only an admitted technical ceiling. Spatial remains a separate presentation choice owned by Omniphony.
