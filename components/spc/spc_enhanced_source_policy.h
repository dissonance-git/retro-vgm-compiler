#pragma once

#include "spc_sample_restoration.h"

#include <cstdint>

namespace gameaudio::spc {

// Ordered synthesis choices for the independent SNES `Enhanced` option.
// Higher enum value does not imply historical authority; `select_*` is the only
// admission function and always prefers the earliest proven source boundary.
enum class spc_enhanced_source_rung : std::uint8_t {
    protected_reference = 0,
    high_rate_brr_reconstruction,
    verified_prebrr_game_grid,
    verified_original_source,
};

struct spc_enhanced_source_capabilities {
    bool high_rate_brr_reconstruction = false;
    bool prebrr_game_grid_available = false;
    const spc_sample_restoration_candidate* original_source = nullptr;
};

// Normal Enhanced never treats a candidate-search hit or generative bandwidth
// extension as an original source. The top rung requires the same automatic
// evidence gate used by the upstream sample restoration machinery.
inline spc_enhanced_source_rung select_spc_enhanced_source_rung(
    const spc_enhanced_source_capabilities& capabilities) noexcept
{
    if (capabilities.original_source != nullptr
        && may_use_spc_sample_restoration_automatically(
            *capabilities.original_source)) {
        return spc_enhanced_source_rung::verified_original_source;
    }
    if (capabilities.prebrr_game_grid_available)
        return spc_enhanced_source_rung::verified_prebrr_game_grid;
    if (capabilities.high_rate_brr_reconstruction)
        return spc_enhanced_source_rung::high_rate_brr_reconstruction;
    return spc_enhanced_source_rung::protected_reference;
}

constexpr bool spc_enhanced_rung_removes_brr_loss(
    spc_enhanced_source_rung rung) noexcept
{
    return rung == spc_enhanced_source_rung::verified_prebrr_game_grid
        || rung == spc_enhanced_source_rung::verified_original_source;
}

constexpr bool spc_enhanced_rung_restores_preparation_lost_bandwidth(
    spc_enhanced_source_rung rung) noexcept
{
    // Only direct evaluation of the proven upstream waveform can recover real
    // source samples that were already removed by a historical downsample or
    // other lossy preparation *before* BRR encoding.
    return rung == spc_enhanced_source_rung::verified_original_source;
}

} // namespace gameaudio::spc
