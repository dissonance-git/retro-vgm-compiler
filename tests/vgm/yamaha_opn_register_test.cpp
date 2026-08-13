#include "yamaha_opn_register.h"

#include <cassert>

using namespace gameaudio::vgm;

int main() {
    constexpr auto ym2203 = traits_for(opn_chip_variant::ym2203);
    constexpr auto ym2608 = traits_for(opn_chip_variant::ym2608);
    constexpr auto ym2610 = traits_for(opn_chip_variant::ym2610);
    constexpr auto ym2612 = traits_for(opn_chip_variant::ym2612);

    // Three-channel OPN ignores the later-family bank-select bit in key data.
    static_assert(opn_key_register_channel(ym2203, 0x00).value() == 0);
    static_assert(opn_key_register_channel(ym2203, 0x01).value() == 1);
    static_assert(opn_key_register_channel(ym2203, 0x02).value() == 2);
    static_assert(!opn_key_register_channel(ym2203, 0x03).has_value());
    static_assert(opn_key_register_channel(ym2203, 0x04).value() == 0);
    static_assert(opn_key_register_channel(ym2203, 0x05).value() == 1);
    static_assert(opn_key_register_channel(ym2203, 0x06).value() == 2);
    static_assert(!opn_key_register_channel(ym2203, 0x07).has_value());

    // Six-slot OPN-family parts use bit 2 to select the upper three channels.
    static_assert(opn_key_register_channel(ym2612, 0x00).value() == 0);
    static_assert(opn_key_register_channel(ym2612, 0x02).value() == 2);
    static_assert(opn_key_register_channel(ym2612, 0x04).value() == 3);
    static_assert(opn_key_register_channel(ym2612, 0x06).value() == 5);
    static_assert(!opn_key_register_channel(ym2612, 0x03).has_value());
    static_assert(!opn_key_register_channel(ym2612, 0x07).has_value());
    static_assert(opn_key_operator_mask(0xA4) == 0x0A);

    // Register geometry and active-channel topology are separate facts.
    static_assert(opn_channel_from_port_register(ym2610, 0, 0xA0).value() == 0);
    static_assert(opn_channel_from_port_register(ym2610, 1, 0xA0).value() == 3);
    static_assert(!fm_channel_is_active(ym2610, 0));
    static_assert(!fm_channel_is_active(ym2610, 3));
    static_assert(fm_channel_is_active(ym2610, 1));
    static_assert(fm_channel_is_active(ym2610, 5));

    // YM2203 has one FM register port; later six-slot parts have two.
    static_assert(opn_channel_from_port_register(ym2203, 0, 0xB2).value() == 2);
    static_assert(!opn_channel_from_port_register(ym2203, 1, 0xB0).has_value());
    static_assert(opn_channel_from_port_register(ym2608, 1, 0xB2).value() == 5);
    static_assert(!opn_channel_from_port_register(ym2608, 0, 0xB3).has_value());

    // Operator register order is a family register-layout fact, not channel order.
    static_assert(opn_operator_register(0x30));
    static_assert(opn_operator_from_register(0x30) == 0);
    static_assert(opn_operator_from_register(0x34) == 2);
    static_assert(opn_operator_from_register(0x38) == 1);
    static_assert(opn_operator_from_register(0x3C) == 3);
    static_assert(!opn_operator_register(0x33));
    static_assert(!opn_operator_register(0xA0));

    // High writes latch; low writes commit. The reserved fourth register in each
    // group must not be mistaken for a channel.
    static_assert(opn_frequency_low_register(0xA0));
    static_assert(opn_frequency_low_register(0xA2));
    static_assert(!opn_frequency_low_register(0xA3));
    static_assert(opn_frequency_high_register(0xA4));
    static_assert(opn_frequency_high_register(0xA6));
    static_assert(!opn_frequency_high_register(0xA7));
    static_assert(opn_ch3_frequency_low_register(0xA8));
    static_assert(opn_ch3_frequency_low_register(0xAA));
    static_assert(!opn_ch3_frequency_low_register(0xAB));
    static_assert(opn_ch3_frequency_high_register(0xAC));
    static_assert(opn_ch3_frequency_high_register(0xAE));
    static_assert(!opn_ch3_frequency_high_register(0xAF));

    constexpr auto pitch = decode_opn_block_fnum(0x23, 0xAB);
    static_assert(pitch.fnum == 0x03AB);
    static_assert(pitch.block == 4);

    static_assert(opn_algorithm_feedback_register(0xB0));
    static_assert(opn_algorithm_feedback_register(0xB2));
    static_assert(!opn_algorithm_feedback_register(0xB3));
    static_assert(opn_algorithm(0x2D) == 5);
    static_assert(opn_feedback(0x2D) == 5);

    assert(true);
    return 0;
}
