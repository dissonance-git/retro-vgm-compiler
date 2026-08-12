#pragma once

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

enum class ym2612_dac_event_kind : std::uint8_t {
    data = 0,
    enable = 1,
};

struct ym2612_dac_timed_event {
    std::size_t sample_offset = 0;
    ym2612_dac_event_kind kind = ym2612_dac_event_kind::data;
    std::uint8_t value = 0;
};

// Realtime high-quality realization of the YM2612 DAC source stream.
//
// The source bytes are still authoritative. The enhanced renderer removes the
// historical zero-order-hold staircase between known PCM samples by joining
// consecutive source points in floating point. It does not invent percussion,
// replace samples, or touch FM channels.
class ym2612_dac_enhanced {
public:
    void reset() noexcept;
    void set_enabled(bool enabled) noexcept { enabled_ = enabled; }
    void write(std::uint8_t value) noexcept;

    bool enabled() const noexcept { return enabled_; }
    std::uint8_t last_byte() const noexcept { return last_byte_; }
    float last_level() const noexcept { return last_level_; }

    // Render a block from already-sorted, exact source events. Data events are
    // linearly reconstructed to the following data point when one is visible
    // in this block. Enable events remain hard control boundaries.
    void render_timed(
        const ym2612_dac_timed_event* events,
        std::size_t event_count,
        float* output,
        std::size_t frames) noexcept;

private:
    static float byte_to_level(std::uint8_t value) noexcept;

    bool enabled_ = false;
    std::uint8_t last_byte_ = 0x80;
    float last_level_ = 0.0f;
};

} // namespace gameaudio::vgm
