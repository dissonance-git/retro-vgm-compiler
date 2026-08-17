#pragma once

#include "ym2612_dac_enhanced.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Converts arbitrary direct 8-bit YM2612 DAC writes into the same outer device
// gain coordinate used by source-aware libvgm replacement.
//
// Direct-write timing remains exact, including authored holds between writes.
// This path removes the YM2612 DAC ladder/sign/output coloration but does not
// invent intermediate PCM samples. Source-bank streams use ym2612_pcm_stream
// instead, where the bank and authored frequency justify bandlimited recovery.
template <std::size_t Capacity>
class ym2612_dac_enhanced_source_block_storage {
public:
    static constexpr double ideal_source_scale = 8448.0;

    bool render(
        ym2612_dac_enhanced& state,
        const ym2612_dac_timed_event* events,
        std::size_t event_count,
        std::size_t frames,
        bool pan_left,
        bool pan_right,
        std::int16_t volume_left,
        std::int16_t volume_right) noexcept {
        valid_ = false;
        frames_ = 0;
        if (frames > Capacity || (event_count != 0 && events == nullptr))
            return false;

        state.render_exact_hold(events, event_count, mono_.data(), frames);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            const double level = static_cast<double>(mono_[frame]);
            if (!std::isfinite(level))
                return false;
            left_[frame] = pan_left
                ? level * ideal_source_scale * static_cast<double>(volume_left)
                : 0.0;
            right_[frame] = pan_right
                ? level * ideal_source_scale * static_cast<double>(volume_right)
                : 0.0;
            if (!std::isfinite(left_[frame]) || !std::isfinite(right_[frame]))
                return false;
        }

        frames_ = frames;
        valid_ = true;
        return true;
    }

    [[nodiscard]] bool valid() const noexcept { return valid_; }
    [[nodiscard]] std::size_t frames() const noexcept { return frames_; }
    [[nodiscard]] const double* left() const noexcept { return left_.data(); }
    [[nodiscard]] const double* right() const noexcept { return right_.data(); }

private:
    std::array<float, Capacity> mono_{};
    std::array<double, Capacity> left_{};
    std::array<double, Capacity> right_{};
    std::size_t frames_ = 0;
    bool valid_ = false;
};

} // namespace gameaudio::vgm
