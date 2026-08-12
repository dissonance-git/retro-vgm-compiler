#pragma once

#include "ym2612_block_capture.h"
#include "ym2612_fm_backend.h"

#include <cstddef>

namespace gameaudio::vgm {

// Applies a captured YM2612 register timeline to an arbitrary six-stem FM
// backend at exact output-sample boundaries. It contains no synthesis policy.
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
