#pragma once

#include "spc_sample_restoration.h"
#include "spc_upstream_sample_reconstruction.h"

#include <cmath>
#include <cstdint>

namespace gameaudio::spc {

// Live control interval for the highest SNES Enhanced rung. Unlike the pre-BRR
// block provider, this does not first collapse a proven upstream source back to
// the game's lossy preparation grid. It evaluates the original waveform at the
// exact game-source phase trajectory, then applies the exact game envelope as a
// high-precision scalar. Pitch/PMON/KON/loop control still comes from the game.
struct spc_original_sample_control_interval {
    const spc_sample_restoration_candidate* source = nullptr;
    std::int32_t start_q12 = 0;
    std::int32_t pitch_step_q12 = 0;
    std::uint16_t envelope = 0;
    bool noise_enabled = false;
};

struct spc_original_sample_interval_result {
    double source_normalized = 0.0;
    double post_envelope_normalized = 0.0;
    double game_position = 0.0;
    bool valid = false;
};

// `phase` is normalized to one native S-DSP interval. At 96 kHz playback this
// is naturally 0, 1/3, 2/3 for each historical 32 kHz interval. The upstream
// coordinate map can retain a much denser original sample grid, so these points
// may contain genuine source information that never survived the game's
// resample/BRR preparation.
inline spc_original_sample_interval_result reconstruct_spc_original_sample_interval(
    const spc_original_sample_control_interval& interval,
    double phase) noexcept
{
    spc_original_sample_interval_result result;
    if (interval.source == nullptr
        || !may_use_spc_sample_restoration_automatically(*interval.source)
        || !std::isfinite(phase) || phase < 0.0 || phase >= 1.0
        || interval.pitch_step_q12 < 0 || interval.envelope > 0x07ffu
        || interval.noise_enabled)
        return result;

    const double position_q12 = static_cast<double>(interval.start_q12)
        + static_cast<double>(interval.pitch_step_q12) * phase;
    if (!std::isfinite(position_q12))
        return result;

    result.game_position = position_q12 / 4096.0;
    const auto source = reconstruct_spc_upstream_candidate_sample(
        *interval.source,
        result.game_position);
    if (!source.valid || !std::isfinite(source.sample))
        return result;

    result.source_normalized = source.sample;
    // Preserve the exact eleven-bit game envelope trajectory but remove the
    // S-DSP's final integer multiply/shift truncation from the Enhanced source.
    result.post_envelope_normalized = result.source_normalized
        * (static_cast<double>(interval.envelope) / 2048.0);
    result.valid = std::isfinite(result.post_envelope_normalized);
    return result;
}

} // namespace gameaudio::spc
