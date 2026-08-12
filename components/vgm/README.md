# VGM component

This directory is the development home for the foobar2000 VGM/VGZ input component.

## Bootstrap

The exact supplied source archive is preserved at:

`../../imports/foo_input_vgm.7z`

Expand that archive here while preserving its original `LICENSE`, Visual Studio project files, and source layout. Do not redesign the wrapper before it has a reproducible baseline build.

The supplied tree contains the active VGM input plus legacy GYM, DRO, and S98 handlers. Keep the legacy handlers working where practical, but VGM/VGZ is the enhancement design center.

## First engineering sequence

1. Expand the supplied source unchanged.
2. Establish a clean reference build.
3. Pin/document the libvgm revision actually used by that build.
4. Verify reference playback before changing audible behavior.
5. Identify the narrowest realtime seam that exposes per-device/per-channel state before final stereo summation.
6. Add diagnostics/A-B capture at that seam.
7. Begin enhanced rendering by source family, not with a bus-level effect.

## Enhancement order

Initial priority:

1. YM2612 / OPN-family FM state and rendering.
2. Genesis DAC + PSG path, with Sonic 3-class material as an early stress case.
3. General PCM/ADPCM/DAC source handling across VGM devices.
4. Source-aware mixing and masking control.
5. QSound as both native authored spatial DSP and a research substrate for generalized source-domain spatial rendering.
6. Broader FM/PSG/wavetable families.

The goal is still ordinary realtime foobar2000 playback. No song compilation or offline reconstruction stage.
