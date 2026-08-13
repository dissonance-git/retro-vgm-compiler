#include "yamaha_opll_register.h"

#include <cassert>

using namespace gameaudio::vgm;

int main() {
    static_assert(opll_frequency_low_register(0x10));
    static_assert(opll_frequency_low_register(0x18));
    static_assert(!opll_frequency_low_register(0x19));
    static_assert(opll_frequency_high_key_register(0x20));
    static_assert(opll_frequency_high_key_register(0x28));
    static_assert(!opll_frequency_high_key_register(0x29));
    static_assert(opll_instrument_volume_register(0x30));
    static_assert(opll_instrument_volume_register(0x38));
    static_assert(!opll_instrument_volume_register(0x39));

    static_assert(opll_channel_register(0x10, 0x10).value() == 0);
    static_assert(opll_channel_register(0x18, 0x10).value() == 8);
    static_assert(opll_channel_register(0x35, 0x30).value() == 5);
    static_assert(!opll_channel_register(0x39, 0x30).has_value());

    constexpr auto pitch = decode_opll_programmed_pitch(0xAA, 0x37);
    static_assert(pitch.fnum == 0x01AA);
    static_assert(pitch.block == 3);
    static_assert(pitch.key_on);
    static_assert(pitch.sustain_on);

    constexpr auto key_off = decode_opll_programmed_pitch(0x55, 0x0E);
    static_assert(key_off.fnum == 0x0055);
    static_assert(key_off.block == 7);
    static_assert(!key_off.key_on);
    static_assert(!key_off.sustain_on);

    static_assert(opll_user_instrument_register(0x00));
    static_assert(opll_user_instrument_register(0x07));
    static_assert(!opll_user_instrument_register(0x08));

    constexpr auto user_patch = decode_opll_patch_selection(0x0C);
    static_assert(user_patch.instrument == 0);
    static_assert(user_patch.volume == 12);
    static_assert(user_patch.source == opll_patch_source::user_registers);

    constexpr auto preset_patch = decode_opll_patch_selection(0x7C);
    static_assert(preset_patch.instrument == 7);
    static_assert(preset_patch.volume == 12);
    static_assert(preset_patch.source == opll_patch_source::preset_instrument_data);

    constexpr auto rhythm = decode_opll_rhythm_state(0x35);
    static_assert(rhythm.rhythm_enabled);
    static_assert(rhythm.percussion_key_mask == 0x15);
    static_assert(opll_channel_uses_melodic_patch(5, rhythm.rhythm_enabled));
    static_assert(!opll_channel_uses_melodic_patch(6, rhythm.rhythm_enabled));
    static_assert(!opll_channel_uses_melodic_patch(8, rhythm.rhythm_enabled));
    static_assert(opll_channel_uses_melodic_patch(8, false));
    static_assert(!opll_channel_uses_melodic_patch(9, false));

    assert(true);
    return 0;
}
