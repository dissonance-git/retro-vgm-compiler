#include "yamaha_opl_family.h"

#include <cassert>

using namespace gameaudio::vgm;

int main() {
    static_assert(resolve_opl_chip_variant(yamaha_register_target::ym3526) ==
                  opl_chip_variant::ym3526);
    static_assert(resolve_opl_chip_variant(yamaha_register_target::y8950) ==
                  opl_chip_variant::y8950);
    static_assert(resolve_opl_chip_variant(yamaha_register_target::ym3812) ==
                  opl_chip_variant::ym3812);
    static_assert(resolve_opl_chip_variant(yamaha_register_target::ymf262) ==
                  opl_chip_variant::ymf262);
    static_assert(!resolve_opl_chip_variant(yamaha_register_target::ym2612).has_value());
    static_assert(!resolve_opl_chip_variant(yamaha_register_target::ym2151).has_value());

    constexpr auto opl = traits_for(opl_chip_variant::ym3526);
    static_assert(opl.fm_channels == 9);
    static_assert(opl.fm_ports == 1);
    static_assert(opl.output_count == 1);
    static_assert(opl.waveform_count == 1);
    static_assert(opl.four_op_pair_count == 0);
    static_assert(opl.has_rhythm_mode);
    static_assert(!opl.has_dynamic_four_op);
    static_assert(!opl.has_adpcm_b);

    constexpr auto y8950 = traits_for(opl_chip_variant::y8950);
    static_assert(y8950.fm_channels == 9);
    static_assert(y8950.waveform_count == 1);
    static_assert(y8950.has_adpcm_b);

    constexpr auto opl2 = traits_for(opl_chip_variant::ym3812);
    static_assert(opl2.fm_channels == 9);
    static_assert(opl2.fm_ports == 1);
    static_assert(opl2.output_count == 1);
    static_assert(opl2.waveform_count == 4);
    static_assert(!opl2.has_dynamic_four_op);

    constexpr auto opl3 = traits_for(opl_chip_variant::ymf262);
    static_assert(opl3.fm_channels == 18);
    static_assert(opl3.fm_ports == 2);
    static_assert(opl3.output_count == 4);
    static_assert(opl3.waveform_count == 8);
    static_assert(opl3.four_op_pair_count == 6);
    static_assert(opl3.has_rhythm_mode);
    static_assert(opl3.has_dynamic_four_op);
    static_assert(!opl3.has_adpcm_b);

    assert(true);
    return 0;
}
