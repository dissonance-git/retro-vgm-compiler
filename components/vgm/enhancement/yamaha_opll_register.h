#pragma once

#include <cstdint>
#include <optional>

namespace gameaudio::vgm {

inline constexpr std::uint8_t opll_fm_channels = 9;

constexpr std::optional<std::uint8_t> opll_channel_register(
    const std::uint8_t reg,
    const std::uint8_t base) noexcept {
    if (reg < base || reg > static_cast<std::uint8_t>(base + 8u))
        return std::nullopt;
    return static_cast<std::uint8_t>(reg - base);
}

constexpr bool opll_frequency_low_register(const std::uint8_t reg) noexcept {
    return reg >= 0x10u && reg <= 0x18u;
}

constexpr bool opll_frequency_high_key_register(const std::uint8_t reg) noexcept {
    return reg >= 0x20u && reg <= 0x28u;
}

constexpr bool opll_instrument_volume_register(const std::uint8_t reg) noexcept {
    return reg >= 0x30u && reg <= 0x38u;
}

constexpr bool opll_user_instrument_register(const std::uint8_t reg) noexcept {
    return reg <= 0x07u;
}

struct opll_programmed_pitch {
    std::uint16_t fnum = 0;
    std::uint8_t block = 0;
    bool key_on = false;
    bool sustain_on = false;
};

constexpr opll_programmed_pitch decode_opll_programmed_pitch(
    const std::uint8_t low,
    const std::uint8_t high_key) noexcept {
    return opll_programmed_pitch{
        static_cast<std::uint16_t>(low | ((high_key & 0x01u) << 8)),
        static_cast<std::uint8_t>((high_key >> 1) & 0x07u),
        (high_key & 0x10u) != 0,
        (high_key & 0x20u) != 0,
    };
}

enum class opll_patch_source : std::uint8_t {
    user_registers,
    preset_instrument_data,
};

struct opll_patch_selection {
    std::uint8_t instrument = 0;
    std::uint8_t volume = 0;
    opll_patch_source source = opll_patch_source::user_registers;
};

// For melodic mode, instrument 0 selects the user patch encoded in registers
// 00-07. Nonzero instrument numbers select preset instrument data whose exact
// bytes are external chip/variant context, not recoverable from this write alone.
constexpr opll_patch_selection decode_opll_patch_selection(
    const std::uint8_t data) noexcept {
    const std::uint8_t instrument = static_cast<std::uint8_t>((data >> 4) & 0x0Fu);
    return opll_patch_selection{
        instrument,
        static_cast<std::uint8_t>(data & 0x0Fu),
        instrument == 0
            ? opll_patch_source::user_registers
            : opll_patch_source::preset_instrument_data,
    };
}

struct opll_rhythm_state {
    bool rhythm_enabled = false;
    std::uint8_t percussion_key_mask = 0;
};

constexpr opll_rhythm_state decode_opll_rhythm_state(const std::uint8_t data) noexcept {
    return opll_rhythm_state{
        (data & 0x20u) != 0,
        static_cast<std::uint8_t>(data & 0x1Fu),
    };
}

constexpr bool opll_channel_uses_melodic_patch(
    const std::uint8_t channel,
    const bool rhythm_enabled) noexcept {
    return channel < opll_fm_channels && !(rhythm_enabled && channel >= 6u);
}

} // namespace gameaudio::vgm
