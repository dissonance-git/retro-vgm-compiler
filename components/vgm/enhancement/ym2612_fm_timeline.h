#pragma once

#include "ym2612_fm_backend.h"

#include <cstddef>

namespace gameaudio::vgm {

// Binds captured YM2612 source-clock writes to a synthesis backend without
// quantizing them onto consumer output frames. The backend receives the whole
// ordered block so native clocking and streaming rate conversion can respect
// register changes at their original source times.
class ym2612_fm_timeline {
public:
    explicit ym2612_fm_timeline(ym2612_fm_backend& backend) noexcept : backend_(backend) {}

    void reset() noexcept { backend_.reset(); }

    void render_timed(
        const ym2612_timed_write* writes,
        std::size_t write_count,
        float* const outputs[ym2612_fm_backend::channel_count],
        std::size_t frames) noexcept;

    void advance_timed(
        const ym2612_timed_write* writes,
        std::size_t write_count,
        std::size_t frames) noexcept;

private:
    ym2612_fm_backend& backend_;
};

} // namespace gameaudio::vgm
