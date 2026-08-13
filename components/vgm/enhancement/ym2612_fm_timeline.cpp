#include "ym2612_fm_timeline.h"

namespace gameaudio::vgm {

void ym2612_fm_timeline::render_timed(
    const ym2612_timed_write* writes,
    std::size_t write_count,
    float* const outputs[ym2612_fm_backend::channel_count],
    std::size_t frames) noexcept {
    backend_.render_timed(writes, write_count, outputs, frames);
}

void ym2612_fm_timeline::advance_timed(
    const ym2612_timed_write* writes,
    std::size_t write_count,
    std::size_t frames) noexcept {
    float* outputs[ym2612_fm_backend::channel_count]{};
    backend_.render_timed(writes, write_count, outputs, frames);
}

} // namespace gameaudio::vgm
