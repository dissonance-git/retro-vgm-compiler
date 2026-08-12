#pragma once

#include <cstddef>

namespace gameaudio::vgm {

struct source_stem_mix_input {
    const float* samples = nullptr;
    float left_gain = 0.0f;
    float right_gain = 0.0f;
};

// Linear high-precision summation boundary for source-native rendering.
//
// Spatial policy lives before this class. Loudness control lives after it.
// This mixer never compresses, limits, normalizes, widens or guesses source
// roles. It simply preserves explicit source routing with double accumulation.
class source_stem_mixer {
public:
    static void mix(
        const source_stem_mix_input* sources,
        std::size_t source_count,
        float* left,
        float* right,
        std::size_t frames,
        bool clear_output = true) noexcept;
};

} // namespace gameaudio::vgm
