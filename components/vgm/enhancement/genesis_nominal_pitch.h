#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace gameaudio::vgm {

// Explicit clock context for converting Genesis device-native pitch coordinates
// into a common frequency coordinate. Values are source-relative facts, normally
// read from the VGM header or another validated source. Zero means unavailable.
struct genesis_pitch_clock_context {
    std::uint32_t ym2612_clock_hz = 0;
    std::uint32_t sn76489_clock_hz = 0;
    std::string source;
};

// YM2612 FNUM/BLOCK defines the channel's nominal frequency basis. This is not
// automatically the acoustic/perceptual fundamental of an FM patch: operator
// multipliers, detune, LFO/PM and algorithm topology remain separate evidence.
//
// ymfm's OPN phase generator represents BLOCK+FNUM as BBBFFFFFFFFFFF, computes
// the ordinary phase step from FNUM and BLOCK, and advances a 10.10 phase
// accumulator once per FM sample. For YM2612 the FM sample rate is master/144.
// Combining those mechanics yields:
//
//   nominal_hz = clock * FNUM * 2^BLOCK / (144 * 2^21)
//
// This function deliberately returns Hz, not a MIDI note or pitch class.
inline std::optional<double> ym2612_nominal_pitch_frequency_hz(
    std::uint16_t fnum,
    std::uint8_t block,
    std::uint32_t clock_hz) noexcept {
    if (clock_hz == 0 || fnum == 0 || fnum > 0x07ffu || block > 7u)
        return std::nullopt;

    constexpr double denominator = 144.0 * 2097152.0; // 144 * 2^21
    const double octave_scale = static_cast<double>(std::uint32_t{1} << block);
    return static_cast<double>(clock_hz) * static_cast<double>(fnum) * octave_scale /
           denominator;
}

// The Sega PSG core divides the master clock by 16 before the tone counters.
// A tone output changes sign once per programmed period, so one complete square
// wave takes two periods:
//
//   nominal_hz = clock / (32 * period)
//
// Chip-specific treatment of very small periods remains a synthesis/audibility
// question. This helper only exposes the nominal programmed tone coordinate.
inline std::optional<double> sn76489_nominal_pitch_frequency_hz(
    std::uint16_t tone_period,
    std::uint32_t clock_hz) noexcept {
    if (clock_hz == 0 || tone_period == 0 || tone_period > 0x03ffu)
        return std::nullopt;

    return static_cast<double>(clock_hz) /
           (32.0 * static_cast<double>(tone_period));
}

} // namespace gameaudio::vgm
