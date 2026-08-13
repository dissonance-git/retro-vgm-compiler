#pragma once

#include "yamaha_opl_family.h"

#include <cstdint>
#include <optional>

namespace gameaudio::vgm {

// Convert the OPL-family channel frequency basis into Hz without assigning a
// musical note identity. YM3526/Y8950/YM3812 advance their FM engine at
// master/72; YMF262 advances at master/288. Operator multiples, PM, rhythm
// behavior, four-operator pairing, and perceptual pitch remain separate.
inline std::optional<double> opl_nominal_pitch_frequency_hz(
    const std::uint16_t fnum,
    const std::uint8_t block,
    const std::uint32_t clock_hz,
    const opl_chip_variant chip) noexcept {
    if (clock_hz == 0 || fnum == 0 || fnum > 0x03FFu || block > 7u)
        return std::nullopt;

    const double clock_divider = chip == opl_chip_variant::ymf262 ? 288.0 : 72.0;
    constexpr double phase_denominator = 1048576.0; // 2^20
    const double octave_scale = static_cast<double>(std::uint32_t{1} << block);
    return static_cast<double>(clock_hz) * static_cast<double>(fnum) * octave_scale /
           (clock_divider * phase_denominator);
}

// OPLL/YM2413 uses a 9-bit FNUM. ymfm reuses the OPL phase calculation after
// shifting the OPLL block/FNUM coordinate left by one bit, which contributes
// one additional factor of two relative to OPL's 10-bit FNUM representation.
// This is still only the nominal channel basis, not a heard fundamental or note.
inline std::optional<double> opll_nominal_pitch_frequency_hz(
    const std::uint16_t fnum,
    const std::uint8_t block,
    const std::uint32_t clock_hz) noexcept {
    if (clock_hz == 0 || fnum == 0 || fnum > 0x01FFu || block > 7u)
        return std::nullopt;

    constexpr double clock_divider = 72.0;
    constexpr double phase_denominator = 1048576.0; // 2^20
    const double octave_scale = static_cast<double>(std::uint32_t{1} << (block + 1u));
    return static_cast<double>(clock_hz) * static_cast<double>(fnum) * octave_scale /
           (clock_divider * phase_denominator);
}

} // namespace gameaudio::vgm
