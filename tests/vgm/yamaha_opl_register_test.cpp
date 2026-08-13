#include "yamaha_four_op_fm.h"
#include "yamaha_opl_register.h"

#include <cassert>

using namespace gameaudio::vgm;

int main() {
    constexpr auto opl2 = traits_for(opl_chip_variant::ym3812);
    constexpr auto opl3 = traits_for(opl_chip_variant::ymf262);

    static_assert(opl_frequency_low_register(0xA0));
    static_assert(opl_frequency_low_register(0xA8));
    static_assert(!opl_frequency_low_register(0xA9));
    static_assert(opl_frequency_high_key_register(0xB0));
    static_assert(opl_frequency_high_key_register(0xB8));
    static_assert(!opl_frequency_high_key_register(0xB9));
    static_assert(opl_connection_feedback_register(0xC0));
    static_assert(opl_connection_feedback_register(0xC8));
    static_assert(!opl_connection_feedback_register(0xC9));

    static_assert(opl_channel_from_port_register(opl2, 0, 0xA0, 0xA0).value() == 0);
    static_assert(opl_channel_from_port_register(opl2, 0, 0xA8, 0xA0).value() == 8);
    static_assert(!opl_channel_from_port_register(opl2, 1, 0xA0, 0xA0).has_value());
    static_assert(opl_channel_from_port_register(opl3, 0, 0xB8, 0xB0).value() == 8);
    static_assert(opl_channel_from_port_register(opl3, 1, 0xB0, 0xB0).value() == 9);
    static_assert(opl_channel_from_port_register(opl3, 1, 0xB8, 0xB0).value() == 17);

    constexpr auto pitch = decode_opl_block_fnum(0xAA, 0x2D);
    static_assert(pitch.fnum == 0x01AA);
    static_assert(pitch.block == 3);
    static_assert(pitch.key_on);

    constexpr auto key_off = decode_opl_block_fnum(0xFF, 0x1B);
    static_assert(key_off.fnum == 0x03FF);
    static_assert(key_off.block == 6);
    static_assert(!key_off.key_on);

    constexpr auto opl_cf = decode_opl_connection_feedback(0x07);
    static_assert(opl_cf.connection == 1);
    static_assert(opl_cf.feedback == 3);

    // Negative control: the same byte has different meaning under the OPN/OPM
    // 3+3 algorithm/feedback packing. OPL must not use the four-op helper.
    constexpr auto four_op_af = decode_yamaha_algorithm_feedback(0x07);
    static_assert(four_op_af.algorithm == 7);
    static_assert(four_op_af.feedback == 0);
    static_assert(four_op_af.algorithm != opl_cf.connection);
    static_assert(four_op_af.feedback != opl_cf.feedback);

    static_assert(opl3_four_op_enable_register(1, 0x04));
    static_assert(!opl3_four_op_enable_register(0, 0x04));
    static_assert(opl3_new_mode_register(1, 0x05));
    static_assert(!opl3_new_mode_register(0, 0x05));
    static_assert(opl3_four_op_pair_mask(0xE5) == 0x25);
    static_assert(opl3_new_mode_enabled(0x01));
    static_assert(!opl3_new_mode_enabled(0x00));
    static_assert(opl3_output_mask(0xD3) == 0x0D);

    constexpr auto rhythm = decode_opl_rhythm_state(0x35);
    static_assert(rhythm.rhythm_enabled);
    static_assert(rhythm.percussion_key_mask == 0x15);

    assert(true);
    return 0;
}
