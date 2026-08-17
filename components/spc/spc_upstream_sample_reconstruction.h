#pragma once

#include "spc_sample_restoration.h"
#include "spc_studio_sample_reconstruction.h"

#include <cmath>

namespace gameaudio::spc {

struct spc_upstream_reconstruction_result {
    double sample = 0.0;
    bool valid = false;
};

// Reconstruct a proposed upstream source at the historical game-sample
// coordinate without yet deciding whether the candidate is allowed into normal
// Enhanced playback. This is the comparison primitive used by lineage
// verification: admission must not be required before the evidence test that
// establishes admission can run.
//
// The proven/upstream PCM itself is sampled with the same studio-grade
// bandlimited reconstruction intended for normal Enhanced playback. This keeps
// candidate validation and eventual playback on one reconstruction model rather
// than validating an 8-point approximation and later rendering something else.
inline spc_upstream_reconstruction_result reconstruct_spc_upstream_candidate_sample(
    const spc_sample_restoration_candidate& candidate,
    double game_sample_position) noexcept
{
    spc_upstream_reconstruction_result result;
    if (!candidate.upstream.valid()
        || !candidate.coordinate_map.valid()
        || !spc_upstream_position_available(candidate, game_sample_position))
        return result;

    const double position = candidate.coordinate_map.map_position(game_sample_position);
    if (!std::isfinite(position))
        return result;

    const auto reconstructed = reconstruct_spc_studio_sample(
        candidate.upstream.mono_pcm,
        candidate.upstream.frame_count,
        position);
    if (!reconstructed.valid || !std::isfinite(reconstructed.sample))
        return result;

    const double scaled = reconstructed.sample
        * candidate.upstream.game_pcm_units_per_source_unit;
    if (!std::isfinite(scaled))
        return result;

    result.sample = scaled;
    result.valid = true;
    return result;
}

// Reconstruct an evidence-approved higher-quality upstream source at the exact
// historical game-sample coordinate. The caller still owns the game's sample
// traversal, pitch, loop and articulation state. This function changes only the
// source waveform/reconstruction ceiling.
inline spc_upstream_reconstruction_result reconstruct_spc_upstream_sample(
    const spc_sample_restoration_candidate& candidate,
    double game_sample_position) noexcept
{
    if (!may_use_spc_sample_restoration_automatically(candidate))
        return {};
    return reconstruct_spc_upstream_candidate_sample(candidate, game_sample_position);
}

} // namespace gameaudio::spc
