#pragma once

#include <cstdint>

namespace gameaudio::vgm {

// VGM clock words reserve bit 30 for dual-chip support and, for selected chips,
// bit 31 for a chip-specific variant. Keep bit 31 uninterpreted here because
// its meaning is defined by the particular header field.
struct chip_clock_word {
    std::uint32_t clock_hz = 0;
    bool dual_chip = false;
    bool flag31 = false;
};

constexpr chip_clock_word decode_chip_clock_word(const std::uint32_t raw) noexcept {
    return chip_clock_word{
        raw & 0x3FFFFFFFu,
        (raw & 0x40000000u) != 0,
        (raw & 0x80000000u) != 0,
    };
}

enum class ym2612_variant : std::uint8_t { ym2612, ym3438 };
enum class ym2151_variant : std::uint8_t { ym2151, ym2164 };
enum class ym2610_variant : std::uint8_t { ym2610, ym2610b };

constexpr ym2612_variant decode_ym2612_variant(const chip_clock_word word) noexcept {
    return word.flag31 ? ym2612_variant::ym3438 : ym2612_variant::ym2612;
}

constexpr ym2151_variant decode_ym2151_variant(const chip_clock_word word) noexcept {
    return word.flag31 ? ym2151_variant::ym2164 : ym2151_variant::ym2151;
}

constexpr ym2610_variant decode_ym2610_variant(const chip_clock_word word) noexcept {
    return word.flag31 ? ym2610_variant::ym2610b : ym2610_variant::ym2610;
}

} // namespace gameaudio::vgm
