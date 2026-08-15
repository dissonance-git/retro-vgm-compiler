#pragma once

#include "qsound_native_source_window.h"

#include <array>
#include <cstddef>

namespace gameaudio::vgm {

// Dependency-free baseline projection used to prove consumer-rate lane
// coherence. This is not the final source-quality ceiling.
inline bool qsound_project_native_linear(
    const qsound_native_source_bracket& bracket,
    std::array<float, qsound_native_source_count>& out) noexcept
{
    out.fill(0.0f);
    if (!bracket.available() ||
        bracket.fraction_numerator >= bracket.fraction_denominator)
        return false;

    const double fraction = static_cast<double>(bracket.fraction_numerator) /
        static_cast<double>(bracket.fraction_denominator);
    const double left_weight = 1.0 - fraction;
    constexpr double int16_scale = 1.0 / 32768.0;

    for (std::size_t lane = 0; lane < qsound_native_source_count; ++lane) {
        const double sample =
            static_cast<double>(bracket.lower->source[lane]) * left_weight +
            static_cast<double>(bracket.upper->source[lane]) * fraction;
        out[lane] = static_cast<float>(sample * int16_scale);
    }
    return true;
}

} // namespace gameaudio::vgm
