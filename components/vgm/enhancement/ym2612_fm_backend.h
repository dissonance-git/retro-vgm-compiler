#pragma once

#include "ym2612_timed_write.h"

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

struct ym2612_fm_backend_config {
    std::uint32_t chip_clock_hz = 0;
    std::uint32_t source_tick_rate_hz = vgm_timeline_tick_rate_hz;
    std::uint32_t output_sample_rate_hz = 0;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return chip_clock_hz != 0 && source_tick_rate_hz != 0 && output_sample_rate_hz != 0;
    }
};

// Synthesis boundary for source-native YM2612 rendering.
//
// The backend receives each realtime block's entire ordered write set in the
// source VGM clock. It owns source-tick -> native-FM-clock mapping and native
// synthesis -> output-rate conversion. The caller must not quantize writes to
// output frames before this boundary.
class ym2612_fm_backend {
public:
    static constexpr std::size_t channel_count = 6;

    virtual ~ym2612_fm_backend() = default;
    virtual bool configure(const ym2612_fm_backend_config& config) noexcept = 0;
    virtual void reset() noexcept = 0;

    // Fixed algorithmic output delay introduced by the backend's streaming
    // reconstruction/rate-conversion path. Other source paths must be aligned
    // to this latency before final source mixing.
    [[nodiscard]] virtual std::size_t latency_frames() const noexcept = 0;

    // Render the next sequential output block. Writes carry absolute source
    // ticks and are ordered exactly as encountered in the VGM stream. Same-tick
    // write order is significant. Any output pointer may be null; synthesis
    // state and clocks must still advance identically.
    virtual void render_timed(
        const ym2612_timed_write* writes,
        std::size_t write_count,
        float* const outputs[channel_count],
        std::size_t frames) noexcept = 0;
};

} // namespace gameaudio::vgm
