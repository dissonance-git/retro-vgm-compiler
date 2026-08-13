#include "vgm_chip_clock.h"

#include <cassert>
#include <optional>

using namespace gameaudio::vgm;

int main() {
    constexpr auto plain = decode_chip_clock_word(vgm_version_1_51, 7'670'453u);
    static_assert(plain.clock_hz == 7'670'453u);
    static_assert(!plain.dual_chip);
    static_assert(!plain.flag31);
    static_assert(decode_ym2612_variant(plain) == ym2612_variant::ym2612);

    constexpr auto dual = decode_chip_clock_word(
        vgm_version_1_51,
        0x40000000u | 8'000'000u);
    static_assert(dual.clock_hz == 8'000'000u);
    static_assert(dual.dual_chip);
    static_assert(!dual.flag31);

    constexpr auto variant = decode_chip_clock_word(
        vgm_version_1_51,
        0x80000000u | 4'000'000u);
    static_assert(variant.clock_hz == 4'000'000u);
    static_assert(!variant.dual_chip);
    static_assert(variant.flag31);
    static_assert(decode_ym2151_variant(variant) == ym2151_variant::ym2164);
    static_assert(decode_ym2610_variant(variant) == ym2610_variant::ym2610b);

    constexpr auto dual_variant = decode_chip_clock_word(
        vgm_version_1_51,
        0xC0000000u | 7'670'453u);
    static_assert(dual_variant.clock_hz == 7'670'453u);
    static_assert(dual_variant.dual_chip);
    static_assert(dual_variant.flag31);
    static_assert(decode_ym2612_variant(dual_variant) == ym2612_variant::ym3438);

    // Dual-chip/variant flag semantics do not exist before VGM 1.51. A parser
    // must not silently strip those bits from an older clock word.
    constexpr auto pre_dual = decode_chip_clock_word(
        0x00000150u,
        0x40000000u | 8'000'000u);
    static_assert(pre_dual.clock_hz == (0x40000000u | 8'000'000u));
    static_assert(!pre_dual.dual_chip);
    static_assert(!pre_dual.flag31);

    constexpr auto absent = decode_chip_clock_word(vgm_version_1_51, 0u);
    static_assert(absent.clock_hz == 0u);
    static_assert(!absent.dual_chip);
    static_assert(!absent.flag31);

    // VGM 1.00/1.01 overloaded the later YM2413 clock field.
    static_assert(resolve_ym2413_clock_role(0x00000101u, 7'670'453u) ==
                  legacy_ym2413_clock_role::ym2612);
    static_assert(resolve_ym2413_clock_role(0x00000101u, 4'000'000u) ==
                  legacy_ym2413_clock_role::ym2151);
    static_assert(resolve_ym2413_clock_role(0x00000101u, 5'000'000u) ==
                  legacy_ym2413_clock_role::ambiguous_5mhz);
    static_assert(resolve_ym2413_clock_role(vgm_version_1_10, 3'579'545u) ==
                  legacy_ym2413_clock_role::ym2413);
    static_assert(resolve_ym2413_clock_role(0x00000101u, 0u) ==
                  legacy_ym2413_clock_role::absent);

    // VGM 1.70 extra headers can give the second chip a distinct clock.
    constexpr auto inherited_pair = resolve_chip_clocks(
        0x00000161u,
        dual,
        std::optional<std::uint32_t>{7'987'000u});
    static_assert(inherited_pair.dual_chip);
    static_assert(inherited_pair.primary_clock_hz == 8'000'000u);
    static_assert(inherited_pair.secondary_clock_hz == 8'000'000u);
    static_assert(!inherited_pair.distinct_secondary_clock);

    constexpr auto split_pair = resolve_chip_clocks(
        vgm_version_1_70,
        dual,
        std::optional<std::uint32_t>{7'987'000u});
    static_assert(split_pair.dual_chip);
    static_assert(split_pair.primary_clock_hz == 8'000'000u);
    static_assert(split_pair.secondary_clock_hz == 7'987'000u);
    static_assert(split_pair.distinct_secondary_clock);

    constexpr auto same_pair = resolve_chip_clocks(
        vgm_version_1_70,
        dual,
        std::optional<std::uint32_t>{8'000'000u});
    static_assert(same_pair.dual_chip);
    static_assert(same_pair.secondary_clock_hz == 8'000'000u);
    static_assert(!same_pair.distinct_secondary_clock);

    constexpr auto single_with_irrelevant_extra = resolve_chip_clocks(
        vgm_version_1_70,
        plain,
        std::optional<std::uint32_t>{8'000'000u});
    static_assert(!single_with_irrelevant_extra.dual_chip);
    static_assert(single_with_irrelevant_extra.secondary_clock_hz == 0u);

    assert(true);
    return 0;
}
