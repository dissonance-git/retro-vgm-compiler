#include "components/vgm/enhancement/vgm_chip_clock.h"

#include <cassert>

using namespace gameaudio::vgm;

int main() {
    constexpr auto plain = decode_chip_clock_word(7'670'453u);
    static_assert(plain.clock_hz == 7'670'453u);
    static_assert(!plain.dual_chip);
    static_assert(!plain.flag31);
    static_assert(decode_ym2612_variant(plain) == ym2612_variant::ym2612);

    constexpr auto dual = decode_chip_clock_word(0x40000000u | 8'000'000u);
    static_assert(dual.clock_hz == 8'000'000u);
    static_assert(dual.dual_chip);
    static_assert(!dual.flag31);

    constexpr auto variant = decode_chip_clock_word(0x80000000u | 4'000'000u);
    static_assert(variant.clock_hz == 4'000'000u);
    static_assert(!variant.dual_chip);
    static_assert(variant.flag31);
    static_assert(decode_ym2151_variant(variant) == ym2151_variant::ym2164);
    static_assert(decode_ym2610_variant(variant) == ym2610_variant::ym2610b);

    constexpr auto dual_variant = decode_chip_clock_word(
        0xC0000000u | 7'670'453u);
    static_assert(dual_variant.clock_hz == 7'670'453u);
    static_assert(dual_variant.dual_chip);
    static_assert(dual_variant.flag31);
    static_assert(decode_ym2612_variant(dual_variant) == ym2612_variant::ym3438);

    constexpr auto absent = decode_chip_clock_word(0u);
    static_assert(absent.clock_hz == 0u);
    static_assert(!absent.dual_chip);
    static_assert(!absent.flag31);

    assert(true);
    return 0;
}
