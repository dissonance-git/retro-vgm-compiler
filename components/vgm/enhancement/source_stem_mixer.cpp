#include "source_stem_mixer.h"

namespace gameaudio::vgm {

void source_stem_mixer::mix(
    const source_stem_mix_input* sources,
    std::size_t source_count,
    float* left,
    float* right,
    std::size_t frames,
    bool clear_output) noexcept {
    if (left == nullptr || right == nullptr)
        return;

    for (std::size_t frame = 0; frame < frames; ++frame) {
        double sum_left = clear_output ? 0.0 : static_cast<double>(left[frame]);
        double sum_right = clear_output ? 0.0 : static_cast<double>(right[frame]);

        if (sources != nullptr) {
            for (std::size_t source = 0; source < source_count; ++source) {
                if (sources[source].samples == nullptr)
                    continue;
                const double sample = static_cast<double>(sources[source].samples[frame]);
                sum_left += sample * static_cast<double>(sources[source].left_gain);
                sum_right += sample * static_cast<double>(sources[source].right_gain);
            }
        }

        left[frame] = static_cast<float>(sum_left);
        right[frame] = static_cast<float>(sum_right);
    }
}

} // namespace gameaudio::vgm
