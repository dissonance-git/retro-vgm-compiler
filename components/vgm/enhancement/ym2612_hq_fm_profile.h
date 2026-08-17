#pragma once

#include "ym2612_enhanced_realization.h"

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Enhanced YM2612 does not mean "pick a nicer preset". It means replay the
// exact six-channel composition and the exact OPN patch/control program on a
// higher-ceiling Yamaha-FM descendant while preserving the source topology.
//
// The automatic profile deliberately keeps four operators per physical channel.
// A modern Yamaha FM engine may expose more operators and algorithms, but
// automatically adding operators would be a timbre-remapping experiment rather
// than a transparent ceiling relaxation.
enum class ym2612_enhanced_fm_mode : std::uint8_t {
    reference_opn2 = 0,
    high_fidelity_opn_descendant,
    experimental_expanded_yamaha_fm,
};

struct ym2612_hq_fm_profile {
    ym2612_enhanced_fm_mode mode =
        ym2612_enhanced_fm_mode::high_fidelity_opn_descendant;

    // The first normal Enhanced target is a real six-channel Yamaha OPN2
    // descendant. ymfm models YMF276/OPN2L as YM2612-derived but with 14-bit
    // intermediate clipping and proper channel mixing, making it the cleanest
    // concrete hardware-descendant baseline before a mathematical studio engine.
    ym2612_enhanced_realization realization =
        default_ym2612_enhanced_realization;

    static constexpr std::size_t physical_channel_count = 6;
    static constexpr std::size_t source_operator_count = 4;

    // Render the OPN graph at a higher internal rate, then low-pass/downsample to
    // the host rate. Eight is a bounded realtime default rather than a claim that
    // one fixed factor is optimal for every patch.
    std::uint8_t internal_oversample = 8;

    // Automatic Enhanced preserves the source graph and timing semantics.
    bool preserve_operator_count = true;
    bool preserve_algorithm_topology = true;
    bool preserve_operator_ratios = true;
    bool preserve_operator_levels = true;
    bool preserve_envelopes = true;
    bool preserve_feedback = true;
    bool preserve_lfo_program = true;
    bool preserve_channel3_special_mode = true;
    bool preserve_source_event_timing = true;
    bool preserve_authored_stereo_route = true;

    // Ceilings the candidate renderer is explicitly allowed to relax.
    bool relax_phase_quantization = true;
    bool relax_sine_table_quantization = true;
    bool relax_internal_amplitude_quantization = true;
    bool relax_channel_accumulator_clipping = true;
    bool relax_output_dac_ladder = true;
    bool relax_output_bandwidth = true;
    bool anti_alias_above_host_nyquist = true;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return internal_oversample != 0
            && internal_oversample <= 32
            && preserves_ym2612_musical_surface(realization);
    }

    [[nodiscard]] constexpr bool automatic_enhanced_safe_shape() const noexcept {
        return valid()
            && mode == ym2612_enhanced_fm_mode::high_fidelity_opn_descendant
            && realization == ym2612_enhanced_realization::ymf276_opn2l
            && preserve_operator_count
            && preserve_algorithm_topology
            && preserve_operator_ratios
            && preserve_operator_levels
            && preserve_envelopes
            && preserve_feedback
            && preserve_lfo_program
            && preserve_channel3_special_mode
            && preserve_source_event_timing
            && preserve_authored_stereo_route;
    }
};

// Expanded-operator Yamaha FM remains useful as a research/creative projection,
// especially against modern FM-X-class engines, but it is not automatically the
// same instrument. Keep it out of the normal Enhanced checkbox until a specific
// patch has passed an identity-preservation comparison.
constexpr ym2612_hq_fm_profile make_experimental_expanded_yamaha_fm_profile() noexcept {
    ym2612_hq_fm_profile profile;
    profile.mode = ym2612_enhanced_fm_mode::experimental_expanded_yamaha_fm;
    profile.realization = ym2612_enhanced_realization::studio_precision_opn2;
    profile.preserve_operator_count = false;
    profile.preserve_algorithm_topology = false;
    return profile;
}

} // namespace gameaudio::vgm
