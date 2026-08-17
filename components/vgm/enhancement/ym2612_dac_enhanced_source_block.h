#pragma once

#include "ym2612_dac_enhanced.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Converts the exact timed 8-bit YM2612 DAC stream into the same outer device
// gain coordinate used by source-aware libvgm replacement.
//
// The byte stream remains authoritative. The enhanced descendant removes the
// zero-order hold and the YM2612 DAC ladder/sign-leak/output-filter artifacts;
// it does not invent higher-resolution source bytes. The scale below follows
// the ideal no-ladder OPN2 path before libvgm's device volume:
//   signed DAC code: (byte - 128) * 2
//   OPN2 output pin gain: * 3
//   Nuked no-filter source scale: * 11
// Since ym2612_dac_enhanced normalizes by 128, the corresponding normalized
// full-scale multiplier is 128 * 2 * 3 * 11 = 8448.
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

        state.render_timed(events, event_count, mono_.data(), frames);
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
