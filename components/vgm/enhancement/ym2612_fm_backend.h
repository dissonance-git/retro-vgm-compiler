#pragma once

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Synthesis boundary for source-native YM2612 enhancement.
//
// A backend owns Yamaha synthesis semantics. The surrounding realtime engine
// owns VGM timing and source routing. Keeping those responsibilities separate
// lets us evaluate mature OPN2 engines without coupling foobar playback to one
// implementation or watering the source down to mixed stereo.
class ym2612_fm_backend {
public:
    static constexpr std::size_t channel_count = 6;

    virtual ~ym2612_fm_backend() = default;

    virtual void reset() noexcept = 0;
    virtual void write(std::uint8_t port, std::uint8_t reg, std::uint8_t data) noexcept = 0;

    // Render six isolated mono FM channel stems. Any output pointer may be null.
    // The backend must still advance synthesis state when all outputs are null.
    virtual void render(float* const outputs[channel_count], std::size_t frames) noexcept = 0;
};

} // namespace gameaudio::vgm
