#pragma once

#include <cstdint>

namespace gameaudio::vgm {

// Enhanced FM must preserve the six-channel OPN2 musical surface before it is
// allowed to improve implementation ceilings. This contract deliberately keeps
// "a better Yamaha FM chip" separate from an unrelated modern synthesizer.
enum class ym2612_enhanced_realization : std::uint8_t {
    reference_ym2612 = 0,
    ym3438_opn2c,
    ymf276_opn2l,
    studio_precision_opn2,
};

struct ym2612_enhanced_realization_traits {
    std::uint8_t fm_channels = 6;
    std::uint8_t operators_per_channel = 4;
    std::uint8_t algorithms = 8;
    std::uint8_t nominal_intermediate_bits = 0;
    bool opn2_register_compatible = true;
    bool ym2612_dac_discontinuity = false;
    bool proper_channel_mix = false;
    bool hardware_descendant = false;
    bool experimental = false;
};

constexpr ym2612_enhanced_realization_traits traits_for(
    const ym2612_enhanced_realization realization) noexcept {
    switch (realization) {
    case ym2612_enhanced_realization::reference_ym2612:
        return {6, 4, 8, 9, true, true, false, true, false};
    case ym2612_enhanced_realization::ym3438_opn2c:
        // Same OPN2 family and register/program surface, without the YM2612
        // DAC discontinuity. Intermediate FM clipping remains 9-bit.
        return {6, 4, 8, 9, true, false, false, true, false};
    case ym2612_enhanced_realization::ymf276_opn2l:
        // ymfm models YMF276 as an YM2612-derived OPN2L realization with
        // 14-bit intermediate clipping and proper mixing. This is the current
        // hardware-descendant Enhanced baseline because it improves the
        // implementation ceiling without changing the six-channel/4-op score.
        return {6, 4, 8, 14, true, false, true, true, false};
    case ym2612_enhanced_realization::studio_precision_opn2:
        // Future mathematical descendant: preserve all programmed OPN2 patch,
        // timing, operator, LFO, feedback and channel semantics while relaxing
        // arithmetic/reconstruction ceilings only after independent validation.
        return {6, 4, 8, 0, true, false, true, false, true};
    }
    return {};
}

constexpr bool preserves_ym2612_musical_surface(
    const ym2612_enhanced_realization realization) noexcept {
    const auto traits = traits_for(realization);
    return traits.opn2_register_compatible
        && traits.fm_channels == 6
        && traits.operators_per_channel == 4
        && traits.algorithms == 8;
}

// The first normal Enhanced target is a real Yamaha OPN2 descendant rather
// than an invented synth architecture. A studio-precision renderer can later
// outrank it only after it wins the project's identity and listening tests.
inline constexpr ym2612_enhanced_realization default_ym2612_enhanced_realization =
    ym2612_enhanced_realization::ymf276_opn2l;

static_assert(preserves_ym2612_musical_surface(
    ym2612_enhanced_realization::reference_ym2612));
static_assert(preserves_ym2612_musical_surface(
    ym2612_enhanced_realization::ym3438_opn2c));
static_assert(preserves_ym2612_musical_surface(
    ym2612_enhanced_realization::ymf276_opn2l));
static_assert(preserves_ym2612_musical_surface(
    ym2612_enhanced_realization::studio_precision_opn2));

} // namespace gameaudio::vgm
