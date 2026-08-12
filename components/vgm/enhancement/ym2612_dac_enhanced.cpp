#include "ym2612_dac_enhanced.h"

#include <algorithm>

namespace gameaudio::vgm {

void ym2612_dac_enhanced::reset() noexcept {
    enabled_ = false;
    last_byte_ = 0x80;
    last_level_ = 0.0f;
}

float ym2612_dac_enhanced::byte_to_level(std::uint8_t value) noexcept {
    // YM2612 interprets the DAC byte around 0x80. Keep the asymmetric endpoint
    // of the original 8-bit source rather than renormalizing its code values.
    return static_cast<float>(static_cast<int>(value) - 128) / 128.0f;
}

void ym2612_dac_enhanced::write(std::uint8_t value) noexcept {
    last_byte_ = value;
    last_level_ = byte_to_level(value);
}

void ym2612_dac_enhanced::render_timed(
    const ym2612_dac_timed_event* events,
    std::size_t event_count,
    float* output,
    std::size_t frames) noexcept {
    if (output == nullptr)
        return;

    std::size_t cursor = 0;

    for (std::size_t index = 0; index < event_count; ++index) {
        const ym2612_dac_timed_event& event = events[index];
        const std::size_t offset = std::min(event.sample_offset, frames);
        if (offset < cursor)
            continue;

        const std::size_t segment = offset - cursor;
        if (segment != 0) {
            if (!enabled_) {
                std::fill(output + cursor, output + offset, 0.0f);
            } else if (event.kind == ym2612_dac_event_kind::data) {
                // The next DAC byte is known because libvgm has already parsed
                // this realtime block. Reconstruct the source between the two
                // known PCM points instead of holding the previous 8-bit code.
                const float target = byte_to_level(event.value);
                const float delta = target - last_level_;
                const float inv_segment = 1.0f / static_cast<float>(segment);
                for (std::size_t frame = 0; frame < segment; ++frame) {
                    const float t = static_cast<float>(frame) * inv_segment;
                    output[cursor + frame] = last_level_ + delta * t;
                }
            } else {
                std::fill(output + cursor, output + offset, last_level_);
            }
            cursor = offset;
        }

        if (event.kind == ym2612_dac_event_kind::enable)
            enabled_ = (event.value & 0x80u) != 0;
        else
            write(event.value);
    }

    if (cursor < frames) {
        std::fill(output + cursor, output + frames, enabled_ ? last_level_ : 0.0f);
    }
}

} // namespace gameaudio::vgm
