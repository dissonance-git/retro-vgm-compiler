#pragma once

#include <cstdint>
#include <optional>

namespace gameaudio::vgm {

inline std::optional<double> ay8910_tone_nominal_frequency_hz(
    const std::uint16_t period,
    const std::uint32_t clock_hz) noexcept {
    if (clock_hz == 0 || period > 0x0FFFu)
        return std::nullopt;
    const std::uint16_t effective_period = period == 0 ? 1u : period;
    return static_cast<double>(clock_hz) /
           (16.0 * static_cast<double>(effective_period));
}

inline std::optional<double> game_boy_square_nominal_frequency_hz(
    const std::uint16_t frequency_code,
    const std::uint32_t clock_hz) noexcept {
    if (clock_hz == 0 || frequency_code > 0x07FFu)
        return std::nullopt;
    return static_cast<double>(clock_hz) /
           (32.0 * static_cast<double>(0x0800u - frequency_code));
}

inline std::optional<double> game_boy_wave_nominal_frequency_hz(
    const std::uint16_t frequency_code,
    const std::uint32_t clock_hz) noexcept {
    if (clock_hz == 0 || frequency_code > 0x07FFu)
        return std::nullopt;
    return static_cast<double>(clock_hz) /
           (64.0 * static_cast<double>(0x0800u - frequency_code));
}

inline std::optional<double> k051649_wave_nominal_frequency_hz(
    const std::uint16_t period,
    const std::uint32_t clock_hz) noexcept {
    if (clock_hz == 0 || period > 0x0FFFu)
        return std::nullopt;
    return static_cast<double>(clock_hz) /
           (16.0 * static_cast<double>(period + 1u));
}

inline bool k051649_frequency_is_halted(const std::uint16_t period) noexcept {
    return period <= 0x0FFFu && period < 9u;
}

inline std::optional<double> huc6280_wave_nominal_frequency_hz(
    const std::uint16_t frequency_code,
    const std::uint32_t clock_hz) noexcept {
    if (clock_hz == 0 || frequency_code > 0x0FFFu)
        return std::nullopt;
    const std::uint16_t effective_period = frequency_code == 0 ? 4096u : frequency_code;
    return static_cast<double>(clock_hz) /
           (32.0 * static_cast<double>(effective_period));
}

inline std::optional<double> nes_apu_square_nominal_frequency_hz(
    const std::uint16_t period_code,
    const std::uint32_t cpu_clock_hz) noexcept {
    if (cpu_clock_hz == 0 || period_code > 0x07FFu)
        return std::nullopt;
    return static_cast<double>(cpu_clock_hz) /
           (16.0 * static_cast<double>(period_code + 1u));
}

inline std::optional<double> nes_apu_triangle_nominal_frequency_hz(
    const std::uint16_t period_code,
    const std::uint32_t cpu_clock_hz) noexcept {
    if (cpu_clock_hz == 0 || period_code > 0x07FFu)
        return std::nullopt;
    return static_cast<double>(cpu_clock_hz) /
           (32.0 * static_cast<double>(period_code + 1u));
}

} // namespace gameaudio::vgm
