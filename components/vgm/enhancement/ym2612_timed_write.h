#pragma once

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

inline constexpr std::uint32_t vgm_timeline_tick_rate_hz = 44100;

// One YM2612 register write on the source VGM clock. `tick` is absolute in
// VGM timeline ticks, not an output-frame index. Same-tick writes retain source
// order and must be applied in that order by the renderer.
struct ym2612_timed_write {
    std::uint64_t tick = 0;
    std::uint8_t port = 0;
    std::uint8_t reg = 0;
    std::uint8_t data = 0;
};

} // namespace gameaudio::vgm
