#pragma once

#include "qsound_consumer_source_block.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

class qsound_consumer_source_storage {
public:
    static constexpr std::size_t capacity = 4096;

    void reset() noexcept {
        frame_count_ = 0;
        valid_ = false;
        all_available_ = false;
    }

    qsound_consumer_source_result render(
        const qsound_native_time_map& time_map,
        const qsound_native_source_window& window,
        std::uint64_t output_start_sample,
        std::size_t frame_count) noexcept
    {
        reset();
        qsound_consumer_source_result result;
        if (frame_count > capacity) {
            result.structurally_valid = false;
            return result;
        }

        qsound_consumer_source_target target;
        for (std::size_t lane = 0; lane < qsound_native_source_count; ++lane)
            target.lane[lane] = lane_[lane].data();
        target.availability = availability_.data();
        target.capacity = capacity;

        result = qsound_render_consumer_source_block(
            time_map, window, output_start_sample, frame_count, target);
        if (!result.structurally_valid)
            return result;

        frame_count_ = frame_count;
        valid_ = true;
        all_available_ = result.unavailable_frames == 0;
        return result;
    }

    bool valid() const noexcept { return valid_; }
    bool all_available() const noexcept { return valid_ && all_available_; }
    std::size_t frame_count() const noexcept { return frame_count_; }

    const float* lane(std::size_t source) const noexcept {
        return source < qsound_native_source_count ? lane_[source].data() : nullptr;
    }

    const std::uint8_t* availability() const noexcept {
        return availability_.data();
    }

private:
    std::array<std::array<float, capacity>, qsound_native_source_count> lane_{};
    std::array<std::uint8_t, capacity> availability_{};
    std::size_t frame_count_ = 0;
    bool valid_ = false;
    bool all_available_ = false;
};

} // namespace gameaudio::vgm
