#pragma once

#include "spc_sample_restoration.h"

#include <cmath>
#include <cstddef>

namespace gameaudio::spc {

struct spc_upstream_reconstruction_result {
    double sample = 0.0;
    bool valid = false;
};

constexpr double spc_upstream_pi = 3.141592653589793238462643383279502884;

inline double spc_upstream_sinc(double x) noexcept {
    if (std::abs(x) < 1.0e-12)
        return 1.0;
    const double pix = spc_upstream_pi * x;
    return std::sin(pix) / pix;
}

inline double spc_upstream_lanczos4(double distance) noexcept {
    constexpr double radius = 4.0;
    if (std::abs(distance) >= radius)
        return 0.0;
    return spc_upstream_sinc(distance)
        * spc_upstream_sinc(distance / radius);
}

// Reconstruct an evidence-approved higher-quality upstream source at the exact
// historical game-sample coordinate. The caller still owns the game's sample
// traversal, pitch, loop and articulation state. This function changes only the
// source waveform/reconstruction ceiling.
//
// Near the finite source boundaries, unavailable Lanczos taps are omitted and
// the surviving coefficients are normalized. No invented pre-roll/post-roll
// samples are introduced. A coordinate outside the proven source mapping fails.
inline spc_upstream_reconstruction_result reconstruct_spc_upstream_sample(
    const spc_sample_restoration_candidate& candidate,
    double game_sample_position) noexcept
{
    spc_upstream_reconstruction_result result;
    if (!may_use_spc_sample_restoration_automatically(candidate)
        || !spc_upstream_position_available(candidate, game_sample_position))
        return result;

    const double position = candidate.coordinate_map.map_position(game_sample_position);
    if (!std::isfinite(position))
        return result;

    const double base = std::floor(position);
    const std::ptrdiff_t center = static_cast<std::ptrdiff_t>(base);
    double weighted = 0.0;
    double weight_sum = 0.0;

    // Eight taps spanning integer coordinates center-3 ... center+4.
    for (std::ptrdiff_t offset = -3; offset <= 4; ++offset) {
        const std::ptrdiff_t index = center + offset;
        if (index < 0 || static_cast<std::size_t>(index) >= candidate.upstream.frame_count)
            continue;

        const float source = candidate.upstream.mono_pcm[static_cast<std::size_t>(index)];
        if (!std::isfinite(source))
            return result;
        const double distance = static_cast<double>(index) - position;
        const double weight = spc_upstream_lanczos4(distance);
        weighted += static_cast<double>(source) * weight;
        weight_sum += weight;
    }

    if (!std::isfinite(weighted) || !std::isfinite(weight_sum)
        || std::abs(weight_sum) < 1.0e-12)
        return result;

    result.sample = weighted / weight_sum;
    result.valid = std::isfinite(result.sample);
    return result;
}

} // namespace gameaudio::spc
