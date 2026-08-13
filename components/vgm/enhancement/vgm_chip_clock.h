#pragma once

#include <cstdint>
#include <optional>

namespace gameaudio::vgm {

inline constexpr std::uint32_t vgm_version_1_01 = 0x00000101u;
inline constexpr std::uint32_t vgm_version_1_10 = 0x00000110u;
inline constexpr std::uint32_t vgm_version_1_51 = 0x00000151u;
inline constexpr std::uint32_t vgm_version_1_70 = 0x00000170u;

// Raw VGM clock-word meaning after version gating. VGM 1.51 introduced the
// dual-chip flag in bit 30 and several field-specific bit-31 meanings. Before
// 1.51 those bits are not format flags and therefore remain part of the raw
// clock value rather than being silently stripped.
struct chip_clock_word {
    std::uint32_t clock_hz = 0;
    bool dual_chip = false;
    bool flag31 = false;
};

constexpr bool vgm_clock_flags_supported(const std::uint32_t version) noexcept {
    return version >= vgm_version_1_51;
}

constexpr chip_clock_word decode_chip_clock_word(
    const std::uint32_t version,
    const std::uint32_t raw) noexcept {
    if (!vgm_clock_flags_supported(version))
        return chip_clock_word{raw, false, false};

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

// VGM 1.00/1.01 overloaded the 0x10 clock field that later became the YM2413
// field. The specification distinguishes the legacy YM2612 and YM2151 uses by
// a 5 MHz boundary. Exactly 5 MHz is left explicit as ambiguous rather than
// inventing a chip identity the specification does not state.
enum class legacy_ym2413_clock_role : std::uint8_t {
    absent,
    ym2413,
    ym2151,
    ym2612,
    ambiguous_5mhz,
};

constexpr legacy_ym2413_clock_role resolve_ym2413_clock_role(
    const std::uint32_t version,
    const std::uint32_t clock_hz) noexcept {
    if (clock_hz == 0)
        return legacy_ym2413_clock_role::absent;
    if (version > vgm_version_1_01)
        return legacy_ym2413_clock_role::ym2413;
    if (clock_hz > 5'000'000u)
        return legacy_ym2413_clock_role::ym2612;
    if (clock_hz < 5'000'000u)
        return legacy_ym2413_clock_role::ym2151;
    return legacy_ym2413_clock_role::ambiguous_5mhz;
}

// VGM 1.70 can provide a distinct clock for the second instance in the extra
// header. Before then, or when no extra clock is supplied, a dual chip inherits
// the primary clock. The parser should pass an already-decoded extra-header
// clock value here; zero means no explicit second clock.
struct resolved_chip_clocks {
    std::uint32_t primary_clock_hz = 0;
    std::uint32_t secondary_clock_hz = 0;
    bool dual_chip = false;
    bool distinct_secondary_clock = false;
};

constexpr resolved_chip_clocks resolve_chip_clocks(
    const std::uint32_t version,
    const chip_clock_word primary,
    const std::optional<std::uint32_t> extra_secondary_clock_hz = std::nullopt) noexcept {
    if (!primary.dual_chip)
        return resolved_chip_clocks{primary.clock_hz, 0, false, false};

    const bool separate_clock_valid =
        version >= vgm_version_1_70 &&
        extra_secondary_clock_hz.has_value() &&
        *extra_secondary_clock_hz != 0;

    return resolved_chip_clocks{
        primary.clock_hz,
        separate_clock_valid ? *extra_secondary_clock_hz : primary.clock_hz,
        true,
        separate_clock_valid && *extra_secondary_clock_hz != primary.clock_hz,
    };
}

} // namespace gameaudio::vgm
