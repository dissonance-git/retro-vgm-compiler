#pragma once

#include "spc_upstream_sample_reconstruction.h"

#include <cmath>
#include <cstddef>

namespace gameaudio::spc {

// Candidate discovery is intentionally weaker than lineage admission.
// Robust matching can tell us that a library sample is worth investigating;
// it cannot prove that the game actually descended from that object.
struct spc_upstream_candidate_score {
    double zero_mean_correlation = 0.0;
    double fitted_gain = 0.0;
    double fitted_bias = 0.0;
    double rms_residual = 0.0;
    std::size_t frames_compared = 0;
    bool valid = false;
};

// Compare a decoded game-domain sample trajectory with a proposed upstream
// source under an already-declared coordinate map. The affine fit is used only
// for robust *ranking* across candidate libraries, where level/DC changes may
// have occurred during historical preparation. It does not modify the explicit
// preparation scale in spc_upstream_sample_view and therefore cannot upgrade a
// candidate to automatic restoration by itself.
inline spc_upstream_candidate_score score_spc_upstream_candidate(
    const float* decoded_game_pcm,
    std::size_t frame_count,
    const spc_sample_restoration_candidate& candidate,
    double game_start_position = 0.0) noexcept
{
    spc_upstream_candidate_score result;
    if (decoded_game_pcm == nullptr || frame_count < 8u
        || !std::isfinite(game_start_position)
        || !candidate.upstream.valid() || !candidate.coordinate_map.valid())
        return result;

    double sum_x = 0.0;
    double sum_y = 0.0;
    double sum_xx = 0.0;
    double sum_yy = 0.0;
    double sum_xy = 0.0;

    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        const double y = static_cast<double>(decoded_game_pcm[frame]);
        if (!std::isfinite(y))
            return {};

        const auto reconstructed = reconstruct_spc_upstream_candidate_sample(
            candidate,
            game_start_position + static_cast<double>(frame));
        if (!reconstructed.valid || !std::isfinite(reconstructed.sample))
            return {};

        const double x = reconstructed.sample;
        sum_x += x;
        sum_y += y;
        sum_xx += x * x;
        sum_yy += y * y;
        sum_xy += x * y;
    }

    const double n = static_cast<double>(frame_count);
    const double centered_xx = sum_xx - (sum_x * sum_x) / n;
    const double centered_yy = sum_yy - (sum_y * sum_y) / n;
    const double centered_xy = sum_xy - (sum_x * sum_y) / n;
    if (!(centered_xx > 1.0e-18) || !(centered_yy > 1.0e-18))
        return result;

    const double denominator = std::sqrt(centered_xx * centered_yy);
    if (!(denominator > 0.0) || !std::isfinite(denominator))
        return result;

    const double correlation = centered_xy / denominator;
    const double gain = centered_xy / centered_xx;
    const double mean_x = sum_x / n;
    const double mean_y = sum_y / n;
    const double bias = mean_y - gain * mean_x;
    if (!std::isfinite(correlation) || !std::isfinite(gain) || !std::isfinite(bias))
        return result;

    double squared_error = 0.0;
    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        const auto reconstructed = reconstruct_spc_upstream_candidate_sample(
            candidate,
            game_start_position + static_cast<double>(frame));
        if (!reconstructed.valid)
            return {};
        const double predicted = gain * reconstructed.sample + bias;
        const double residual = static_cast<double>(decoded_game_pcm[frame]) - predicted;
        squared_error += residual * residual;
    }

    const double rms = std::sqrt(squared_error / n);
    if (!std::isfinite(rms))
        return result;

    result.zero_mean_correlation = correlation;
    result.fitted_gain = gain;
    result.fitted_bias = bias;
    result.rms_residual = rms;
    result.frames_compared = frame_count;
    result.valid = true;
    return result;
}

// Discovery threshold only. Passing this test means "strong candidate worth a
// forward-lineage check", never "proven original sample".
inline bool strong_spc_upstream_candidate(
    const spc_upstream_candidate_score& score,
    double minimum_absolute_correlation = 0.985) noexcept
{
    return score.valid
        && std::isfinite(minimum_absolute_correlation)
        && minimum_absolute_correlation >= 0.0
        && minimum_absolute_correlation <= 1.0
        && std::abs(score.zero_mean_correlation) >= minimum_absolute_correlation;
}

} // namespace gameaudio::spc
