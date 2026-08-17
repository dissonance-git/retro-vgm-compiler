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

// Source-faithful realization of direct YM2612 DAC writes.
//
// The 8-bit source codes and their exact write times remain authoritative. For
// arbitrary direct $2A writes, absence of another write means "hold this code",
// not permission to invent intermediate PCM. Source-bank VGM DAC streams have a
// separate ym2612_pcm_stream renderer that may reconstruct between source bytes
// because the bank and authored playback frequency make that semantics explicit.
class ym2612_dac_enhanced {
public:
    void reset() noexcept;
    void set_enabled(bool enabled) noexcept { enabled_ = enabled; }
    void write(std::uint8_t value) noexcept;

    bool enabled() const noexcept { return enabled_; }
    std::uint8_t last_byte() const noexcept { return last_byte_; }
    float last_level() const noexcept { return last_level_; }

    // Exact causal hold renderer for arbitrary direct writes. It removes the
    // hardware ladder/sign/output coloration later in the source-scaled mixing
    // path, but never smooths a gap whose source semantics are unknown.
    void render_exact_hold(
        const ym2612_dac_timed_event* events,
        std::size_t event_count,
        float* output,
        std::size_t frames) noexcept;

    // Legacy bounded reconstruction helper retained for experiments/tests where
    // consecutive events are independently known to be samples of one PCM
    // stream. Normal direct-write playback must use render_exact_hold().
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
