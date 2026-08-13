#include "yamaha_opn_family.h"

#include <cassert>

using namespace gameaudio::vgm;

int main() {
    constexpr auto plain_clock = decode_chip_clock_word(vgm_version_1_51, 8'000'000u);
    constexpr auto variant_clock = decode_chip_clock_word(
        vgm_version_1_51,
        0x80000000u | 8'000'000u);

    static_assert(resolve_opn_chip_variant(yamaha_register_target::ym2203, plain_clock) ==
                  opn_chip_variant::ym2203);
    static_assert(resolve_opn_chip_variant(yamaha_register_target::ym2608, plain_clock) ==
                  opn_chip_variant::ym2608);
    static_assert(resolve_opn_chip_variant(yamaha_register_target::ym2610, plain_clock) ==
                  opn_chip_variant::ym2610);
    static_assert(resolve_opn_chip_variant(yamaha_register_target::ym2610, variant_clock) ==
                  opn_chip_variant::ym2610b);
    static_assert(resolve_opn_chip_variant(yamaha_register_target::ym2612, plain_clock) ==
                  opn_chip_variant::ym2612);
    static_assert(resolve_opn_chip_variant(yamaha_register_target::ym2612, variant_clock) ==
                  opn_chip_variant::ym3438);
    static_assert(!resolve_opn_chip_variant(yamaha_register_target::ym2151, plain_clock).has_value());
    static_assert(!resolve_opn_chip_variant(yamaha_register_target::ym3812, plain_clock).has_value());

    constexpr auto opn = traits_for(opn_chip_variant::ym2203);
    static_assert(opn.fm_register_slots == 3);
    static_assert(opn.active_fm_channel_mask == 0x07);
    static_assert(opn.fm_ports == 1);
    static_assert(opn.has_ssg);
    static_assert(!opn.has_adpcm_a && !opn.has_adpcm_b && !opn.has_dac);

    constexpr auto opna = traits_for(opn_chip_variant::ym2608);
    static_assert(opna.fm_register_slots == 6);
    static_assert(opna.active_fm_channel_mask == 0x3F);
    static_assert(opna.fm_ports == 2);
    static_assert(opna.has_ssg && opna.has_adpcm_a && opna.has_adpcm_b);
    static_assert(!opna.has_dac);

    constexpr auto opnb = traits_for(opn_chip_variant::ym2610);
    static_assert(opnb.fm_register_slots == 6);
    static_assert(opnb.active_fm_channel_mask == 0x36);
    static_assert(!fm_channel_is_active(opnb, 0));
    static_assert(fm_channel_is_active(opnb, 1));
    static_assert(fm_channel_is_active(opnb, 2));
    static_assert(!fm_channel_is_active(opnb, 3));
    static_assert(fm_channel_is_active(opnb, 4));
    static_assert(fm_channel_is_active(opnb, 5));

    constexpr auto opnb2 = traits_for(opn_chip_variant::ym2610b);
    static_assert(opnb2.active_fm_channel_mask == 0x3F);
    for (std::uint8_t channel = 0; channel < 6; ++channel)
        assert(fm_channel_is_active(opnb2, channel));

    constexpr auto opn2 = traits_for(opn_chip_variant::ym2612);
    constexpr auto opn2c = traits_for(opn_chip_variant::ym3438);
    static_assert(opn2.fm_register_slots == 6 && opn2.fm_ports == 2);
    static_assert(opn2.active_fm_channel_mask == 0x3F);
    static_assert(!opn2.has_ssg && !opn2.has_adpcm_a && !opn2.has_adpcm_b && opn2.has_dac);
    static_assert(opn2.fm_register_slots == opn2c.fm_register_slots);
    static_assert(opn2.active_fm_channel_mask == opn2c.active_fm_channel_mask);

    return 0;
}
