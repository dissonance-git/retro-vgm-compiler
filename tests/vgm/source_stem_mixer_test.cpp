#include "../../components/vgm/enhancement/source_stem_mixer.h"

#include <array>
#include <cmath>
#include <cstddef>

using gameaudio::vgm::source_stem_mix_input;
using gameaudio::vgm::source_stem_mixer;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    constexpr std::size_t frames = 4;
    const std::array<float, frames> a{{1.0f, 0.5f, -1.0f, -0.5f}};
    const std::array<float, frames> b{{0.25f, -0.25f, 0.75f, -0.75f}};

    const source_stem_mix_input inputs[] = {
        {a.data(), 1.0f, 0.0f},
        {b.data(), 0.5f, 2.0f},
        {nullptr, 100.0f, 100.0f},
    };

    std::array<float, frames> left{};
    std::array<float, frames> right{};
    source_stem_mixer::mix(inputs, std::size(inputs), left.data(), right.data(), frames);

    for (std::size_t frame = 0; frame < frames; ++frame) {
        const float expected_left = a[frame] + 0.5f * b[frame];
        const float expected_right = 2.0f * b[frame];
        CHECK(std::abs(left[frame] - expected_left) < 1e-7f);
        CHECK(std::abs(right[frame] - expected_right) < 1e-7f);
    }

    // Additive mode must preserve an existing direct/master contribution.
    left.fill(0.25f);
    right.fill(-0.5f);
    source_stem_mixer::mix(inputs, 1, left.data(), right.data(), frames, false);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        CHECK(std::abs(left[frame] - (0.25f + a[frame])) < 1e-7f);
        CHECK(std::abs(right[frame] + 0.5f) < 1e-7f);
    }

    // The mixer is deliberately linear and does not hide overload with a
    // limiter. Downstream headroom policy must therefore be explicit/testable.
    const std::array<float, 1> hot{{1.0f}};
    const source_stem_mix_input hot_inputs[] = {
        {hot.data(), 1.0f, 1.0f},
        {hot.data(), 1.0f, 1.0f},
    };
    float hot_left = 0.0f;
    float hot_right = 0.0f;
    source_stem_mixer::mix(hot_inputs, 2, &hot_left, &hot_right, 1);
    CHECK(hot_left == 2.0f);
    CHECK(hot_right == 2.0f);

    return 0;
}
