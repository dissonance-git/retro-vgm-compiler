#pragma once

#include "qsound_native_linear_projection.h"
#include "qsound_native_time_map.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

struct qsound_consumer_source_target {
    std::array<float*, qsound_native_source_count> lane{};
    std::uint8_t* availability = nullptr;
    std::size_t capacity = 0;
};

struct qsound_consumer_source_result {
    bool structurally_valid = true;
    std::size_t available_frames = 0;
    std::size_t unavailable_frames = 0;
};

// Allocation-free consumer block projection. The same absolute QSound time map
// and the same native bracket are used for every lane in each output frame.
inline qsound_consumer_source_result qsound_render_consumer_source_block(
    const qsound_native_time_map& time_map,
    const qsound_native_source_window& window,
    std::uint64_t output_start_sample,
    std::size_t frame_count,
    const qsound_consumer_source_target& target) noexcept
{
    qsound_consumer_source_result result;
    if (!time_map.supported() || target.capacity < frame_count ||
        (frame_count != 0 && target.availability == nullptr)) {
        result.structurally_valid = false;
        return result;
    }

    for (float* lane : target.lane) {
        if (frame_count != 0 && lane == nullptr) {
            result.structurally_valid = false;
            return result;
        }
    }

    std::array<float, qsound_native_source_count> projected{};
    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        if (output_start_sample > std::numeric_limits<std::uint64_t>::max() - frame) {
            result.structurally_valid = false;
            return result;
        }

        const std::uint64_t output_sample =
            output_start_sample + static_cast<std::uint64_t>(frame);
        qsound_native_time_point point;
        qsound_native_source_bracket bracket;
        const bool available =
            time_map.project(output_sample, point) &&
            window.find_bracket(point, bracket) &&
            qsound_project_native_linear(bracket, projected);

        target.availability[frame] = available ? 1u : 0u;
        if (available)
            ++result.available_frames;
        else {
            ++result.unavailable_frames;
            projected.fill(0.0f);
        }

        for (std::size_t lane = 0; lane < qsound_native_source_count; ++lane)
            target.lane[lane][frame] = projected[lane];
    }

    return result;
}

} // namespace gameaudio::vgm
