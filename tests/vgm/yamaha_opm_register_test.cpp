#include "yamaha_opm_register.h"

#include <cassert>

using namespace gameaudio::vgm;

int main() {
    // OPM key writes select one of 8 channels directly in bits 0-2 and carry
    // the four-operator key mask in bits 3-6. This is not OPN key encoding.
    static_assert(opm_key_channel(0x5D) == 5);
    static_assert(opm_key_operator_mask(0x5D) == 0x0B);

    static_assert(opm_algorithm_feedback_register(0x20));
    static_assert(opm_algorithm_feedback_register(0x27));
    static_assert(!opm_algorithm_feedback_register(0x28));
    static_assert(opm_channel_register(0x25, 0x20).value() == 5);

    constexpr auto af = opm_algorithm_feedback(0x2D);
    static_assert(af.algorithm == 5);
    static_assert(af.feedback == 5);

    // OPM pitch is key-code + key-fraction, not FNUM + block.
    static_assert(opm_key_code_register(0x28));
    static_assert(opm_key_code_register(0x2F));
    static_assert(!opm_key_code_register(0x30));
    static_assert(opm_key_fraction_register(0x30));
    static_assert(opm_key_fraction_register(0x37));
    static_assert(!opm_key_fraction_register(0x38));

    constexpr auto pitch = decode_opm_programmed_pitch(0x5A, 0xFC);
    static_assert(pitch.key_code == 0x5A);
    static_assert(pitch.octave == 5);
    static_assert(pitch.note_code == 0x0A);
    static_assert(pitch.key_fraction == 63);
    static_assert(pitch.packed_block_frequency == ((0x5Au << 6) | 63u));

    // Bit 7 of the key-code source byte is not part of the seven-bit OPM code.
    constexpr auto masked_pitch = decode_opm_programmed_pitch(0xDA, 0xFF);
    static_assert(masked_pitch.key_code == 0x5A);
    static_assert(masked_pitch.key_fraction == 63);

    static_assert(opm_lfo_sensitivity_register(0x38));
    static_assert(opm_lfo_sensitivity_register(0x3F));
    static_assert(!opm_lfo_sensitivity_register(0x40));

    // OPM operator addressing uses channel bits 0-2 and slot bits 3-4.
    // The physical slot order 1,3,2,4 maps through the same cross-family
    // logical order now used by OPN.
    static_assert(opm_operator_register(0x40));
    static_assert(opm_operator_register(0xFF));
    static_assert(!opm_operator_register(0x3F));
    static_assert(opm_operator_channel(0x45) == 5);
    static_assert(opm_operator_from_register(0x40) == 0);
    static_assert(opm_operator_from_register(0x48) == 2);
    static_assert(opm_operator_from_register(0x50) == 1);
    static_assert(opm_operator_from_register(0x58) == 3);

    // OPM stereo bits are opposite address geometry from OPN but still retain
    // authored left/right routing explicitly.
    constexpr auto left_only = decode_opm_stereo_route(0x40);
    constexpr auto right_only = decode_opm_stereo_route(0x80);
    constexpr auto both = decode_opm_stereo_route(0xC0);
    static_assert(left_only.left && !left_only.right);
    static_assert(!right_only.left && right_only.right);
    static_assert(both.left && both.right);

    assert(true);
    return 0;
}
