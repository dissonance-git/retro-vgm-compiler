#pragma once

#include "yamaha_four_op_fm.h"

#include <cstdint>
#include <optional>

namespace gameaudio::vgm {

inline constexpr std::uint8_t opm_fm_channels = 8;

constexpr std::uint8_t opm_key_channel(const std::uint8_t data) noexcept {
    return static_cast<std::uint8_t>(data & 0x07u);
}

constexpr std::uint8_t opm_key_operator_mask(const std::uint8_t data) noexcept {
    return static_cast<std::uint8_t>((data >> 3) & 0x0Fu);
}

constexpr std::optional<std::uint8_t> opm_channel_register(
    const std::uint8_t reg,
    const std::uint8_t base) noexcept {
    if (reg < base || reg >= static_cast<std::uint16_t>(base) + opm_fm_channels)
        return std::nullopt;
    return static_cast<std::uint8_t>(reg - base);
}

constexpr bool opm_algorithm_feedback_register(const std::uint8_t reg) noexcept {
    return reg >= 0x20u && reg <= 0x27u;
}

constexpr bool opm_key_code_register(const std::uint8_t reg) noexcept {
    return reg >= 0x28u && reg <= 0x2Fu;
}

constexpr bool opm_key_fraction_register(const std::uint8_t reg) noexcept {
    return reg >= 0x30u && reg <= 0x37u;
}

constexpr bool opm_lfo_sensitivity_register(const std::uint8_t reg) noexcept {
    return reg >= 0x38u && reg <= 0x3Fu;
}

constexpr bool opm_operator_register(const std::uint8_t reg) noexcept {
    return reg >= 0x40u;
}

constexpr std::uint8_t opm_operator_channel(const std::uint8_t reg) noexcept {
    return static_cast<std::uint8_t>(reg & 0x07u);
}

constexpr std::uint8_t opm_operator_from_register(const std::uint8_t reg) noexcept {
    return yamaha_four_op_logical_operator(static_cast<std::uint8_t>((reg >> 3) & 0x03u));
}

struct opm_programmed_pitch {
    std::uint8_t key_code = 0;
    std::uint8_t octave = 0;
    std::uint8_t note_code = 0;
    std::uint8_t key_fraction = 0;
    std::uint16_t packed_block_frequency = 0;
};

// OPM uses a seven-bit key code plus a six-bit key fraction, not OPN FNUM.
// Preserve the raw four-bit note code because its mapping is an OPM-specific
// pitch encoding and must not be silently replaced with a chromatic note name.
constexpr opm_programmed_pitch decode_opm_programmed_pitch(
    const std::uint8_t key_code_register_data,
    const std::uint8_t key_fraction_register_data) noexcept {
    const std::uint8_t key_code = static_cast<std::uint8_t>(key_code_register_data & 0x7Fu);
    const std::uint8_t fraction = static_cast<std::uint8_t>((key_fraction_register_data >> 2) & 0x3Fu);
    return opm_programmed_pitch{
        key_code,
        static_cast<std::uint8_t>((key_code >> 4) & 0x07u),
        static_cast<std::uint8_t>(key_code & 0x0Fu),
        fraction,
        static_cast<std::uint16_t>((static_cast<std::uint16_t>(key_code) << 6) | fraction),
    };
}

constexpr yamaha_algorithm_feedback opm_algorithm_feedback(
    const std::uint8_t data) noexcept {
    return decode_yamaha_algorithm_feedback(data);
}

struct opm_stereo_route {
    bool left = false;
    bool right = false;
};

constexpr opm_stereo_route decode_opm_stereo_route(const std::uint8_t data) noexcept {
    return opm_stereo_route{
        (data & 0x40u) != 0,
        (data & 0x80u) != 0,
    };
}

} // namespace gameaudio::vgm
