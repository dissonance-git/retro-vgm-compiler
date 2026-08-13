#pragma once

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

struct ym2612_fm_backend_config {
    std::uint32_t chip_clock_hz = 0;
    std::uint32_t output_sample_rate_hz = 0;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return chip_clock_hz != 0 && output_sample_rate_hz != 0;
    }
};

// Synthesis boundary for source-native YM2612 enhancement.
//
// A backend owns Yamaha synthesis semantics and any rate conversion needed to
// produce the requested realtime output rate. The surrounding realtime engine
// owns VGM timing and source routing. Keeping those responsibilities separate
// lets us evaluate mature OPN2 engines without coupling foobar playback to one
// implementation or watering the source down to mixed stereo.
class ym2612_fm_backend {
public:
    static constexpr std::size_t channel_count = 6;

    virtual ~ym2612_fm_backend() = default;

    // Configure the exact source chip clock and the consumer/output frame rate.
    // A mature OPN2 core may synthesize at a different native rate internally;
    // translating that native clock into output frames is backend-owned so the
    // timeline remains an exact output-frame scheduler rather than a hidden
    // resampler. Return false for unsupported/invalid configurations.
    virtual bool configure(const ym2612_fm_backend_config& config) noexcept = 0;

    virtual void reset() noexcept = 0;
    virtual void write(std::uint8_t port, std::uint8_t reg, std::uint8_t data) noexcept = 0;

    // Render six isolated mono FM channel stems at output_sample_rate_hz. Any
    // output pointer may be null. The backend must still advance synthesis state
    // when all outputs are null.
    virtual void render(float* const outputs[channel_count], std::size_t frames) noexcept = 0;
};

} // namespace gameaudio::vgm