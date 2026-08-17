#pragma once

#include "spc_upstream_sample_reconstruction.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

struct spc_sample_lineage_metrics {
    std::size_t compared_frames = 0;
    double rmse = 0.0;
    double normalized_rmse = 0.0;
    double peak_absolute_error = 0.0;
    double correlation = 0.0;
    double gain_ratio = 0.0;
    bool valid = false;
};

// Compare a candidate pre-BRR source, after its explicit historical preparation
// map, against a decoded game-sample waveform. This is deliberately a metric,
// not a provenance oracle: a strong waveform match can validate a proposed
// lineage, but cannot by itself turn an unrelated library variant into an exact
// historical source.
inline spc_sample_lineage_metrics measure_spc_sample_lineage(
    const spc_sample_restoration_candidate& candidate,
    const std::int16_t* decoded_game_pcm,
    std::size_t frame_count,
    double game_start_position = 0.0) noexcept
{
    spc_sample_lineage_metrics result;
    if (decoded_game_pcm == nullptr || frame_count == 0
        || !std::isfinite(game_start_position)
        || !candidate.upstream.valid()
        || !candidate.coordinate_map.valid())
        return result;

    long double error_energy = 0.0L;
    long double game_energy = 0.0L;
    long double candidate_energy = 0.0L;
    long double dot = 0.0L;
    long double game_sum = 0.0L;
    long double candidate_sum = 0.0L;
    double peak_error = 0.0;

    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        const double game_position = game_start_position + static_cast<double>(frame);
        const auto reconstructed = reconstruct_spc_upstream_candidate_sample(
            candidate,
            game_position);
        if (!reconstructed.valid)
            return {};

        const double game = static_cast<double>(decoded_game_pcm[frame]);
        const double source = reconstructed.sample;
        if (!std::isfinite(source))
            return {};

        const double error = source - game;
        error_energy += static_cast<long double>(error) * error;
        game_energy += static_cast<long double>(game) * game;
        candidate_energy += static_cast<long double>(source) * source;
        dot += static_cast<long double>(game) * source;
        game_sum += game;
        candidate_sum += source;
        peak_error = std::fmax(peak_error, std::abs(error));
    }

    const long double count = static_cast<long double>(frame_count);
    const double rmse = std::sqrt(static_cast<double>(error_energy / count));
    const double game_rms = std::sqrt(static_cast<double>(game_energy / count));
    const double source_rms = std::sqrt(static_cast<double>(candidate_energy / count));

    // Pearson correlation after removing DC. This prevents a constant offset
    // from masquerading as strong source identity.
    const long double centered_dot = dot - (game_sum * candidate_sum / count);
    const long double game_centered = game_energy - (game_sum * game_sum / count);
    const long double candidate_centered =
        candidate_energy - (candidate_sum * candidate_sum / count);
    double correlation = 0.0;
    if (game_centered > 0.0L && candidate_centered > 0.0L) {
        correlation = static_cast<double>(
            centered_dot / std::sqrt(game_centered * candidate_centered));
        if (!std::isfinite(correlation))
            correlation = 0.0;
        if (correlation > 1.0) correlation = 1.0;
        if (correlation < -1.0) correlation = -1.0;
    }

    result.compared_frames = frame_count;
    result.rmse = rmse;
    result.normalized_rmse = game_rms > 1.0e-12 ? rmse / game_rms : 0.0;
    result.peak_absolute_error = peak_error;
    result.correlation = correlation;
    result.gain_ratio = game_rms > 1.0e-12 ? source_rms / game_rms : 0.0;
    result.valid = std::isfinite(result.rmse)
        && std::isfinite(result.normalized_rmse)
        && std::isfinite(result.peak_absolute_error)
        && std::isfinite(result.correlation)
        && std::isfinite(result.gain_ratio);
    return result;
}

struct spc_sample_lineage_acceptance {
    double maximum_normalized_rmse = 0.08;
    double minimum_correlation = 0.995;
    double minimum_gain_ratio = 0.85;
    double maximum_gain_ratio = 1.15;
    std::size_t minimum_frames = 32;
};

// Bounded project validation for a proposed preparation map. Passing this test
// is intentionally weaker than provenance: it can set the candidate's
// identity_validation_passed bit only when independent historical/source
// evidence has already established an exact lineage relation.
inline bool spc_sample_lineage_metrics_pass(
    const spc_sample_lineage_metrics& metrics,
    const spc_sample_lineage_acceptance& acceptance = {}) noexcept
{
    return metrics.valid
        && metrics.compared_frames >= acceptance.minimum_frames
        && metrics.normalized_rmse <= acceptance.maximum_normalized_rmse
        && metrics.correlation >= acceptance.minimum_correlation
        && metrics.gain_ratio >= acceptance.minimum_gain_ratio
        && metrics.gain_ratio <= acceptance.maximum_gain_ratio;
}

inline bool validate_exact_spc_sample_lineage_candidate(
    spc_sample_restoration_candidate& candidate,
    const std::int16_t* decoded_game_pcm,
    std::size_t frame_count,
    double game_start_position = 0.0,
    const spc_sample_lineage_acceptance& acceptance = {}) noexcept
{
    candidate.identity_validation_passed = false;
    if (!spc_has_exact_upstream_lineage(candidate.relation)
        || !candidate.coordinate_map.preparation_chain_exact)
        return false;

    const auto metrics = measure_spc_sample_lineage(
        candidate,
        decoded_game_pcm,
        frame_count,
        game_start_position);
    candidate.identity_validation_passed =
        spc_sample_lineage_metrics_pass(metrics, acceptance);
    return candidate.identity_validation_passed;
}

} // namespace gameaudio::spc
