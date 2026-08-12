#include "ym2612_fm_timeline.h"

#include <algorithm>

namespace gameaudio::vgm {

void ym2612_fm_timeline::render_timed(
    const ym2612_timed_write* writes,
    std::size_t write_count,
    float* const outputs[ym2612_fm_backend::channel_count],
    std::size_t frames) noexcept {
    std::size_t cursor = 0;

    for (std::size_t index = 0; index < write_count; ++index) {
        const ym2612_timed_write& event = writes[index];
        const std::size_t offset = std::min(event.sample_offset, frames);
        if (offset < cursor)
            continue;

        const std::size_t segment = offset - cursor;
        if (segment != 0) {
            float* segment_outputs[ym2612_fm_backend::channel_count]{};
            for (std::size_t channel = 0; channel < ym2612_fm_backend::channel_count; ++channel)
                segment_outputs[channel] = outputs[channel] != nullptr ? outputs[channel] + cursor : nullptr;
            backend_.render(segment_outputs, segment);
            cursor = offset;
        }

        backend_.write(event.port, event.reg, event.data);
    }

    if (cursor < frames) {
        float* segment_outputs[ym2612_fm_backend::channel_count]{};
        for (std::size_t channel = 0; channel < ym2612_fm_backend::channel_count; ++channel)
            segment_outputs[channel] = outputs[channel] != nullptr ? outputs[channel] + cursor : nullptr;
        backend_.render(segment_outputs, frames - cursor);
    }
}

void ym2612_fm_timeline::advance_timed(
    const ym2612_timed_write* writes,
    std::size_t write_count,
    std::size_t frames) noexcept {
    float* outputs[ym2612_fm_backend::channel_count]{};
    render_timed(writes, write_count, outputs, frames);
}

} // namespace gameaudio::vgm
