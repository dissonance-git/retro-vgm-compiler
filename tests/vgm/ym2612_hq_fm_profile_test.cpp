#include "components/vgm/enhancement/ym2612_hq_fm_profile.h"

#include <cassert>

int main() {
    using namespace gameaudio::vgm;

    ym2612_hq_fm_profile profile;
    static_assert(ym2612_hq_fm_profile::physical_channel_count == 6);
    static_assert(ym2612_hq_fm_profile::source_operator_count == 4);
    assert(profile.valid());
    assert(profile.automatic_enhanced_safe_shape());
    assert(profile.relax_phase_quantization);
    assert(profile.relax_sine_table_quantization);
    assert(profile.relax_internal_amplitude_quantization);
    assert(profile.relax_output_dac_ladder);
    assert(profile.anti_alias_above_host_nyquist);

    auto expanded = make_experimental_expanded_yamaha_fm_profile();
    assert(expanded.valid());
    assert(expanded.mode == ym2612_enhanced_fm_mode::experimental_expanded_yamaha_fm);
    assert(!expanded.preserve_operator_count);
    assert(!expanded.preserve_algorithm_topology);
    assert(!expanded.automatic_enhanced_safe_shape());

    profile.internal_oversample = 0;
    assert(!profile.valid());
    assert(!profile.automatic_enhanced_safe_shape());

    return 0;
}
