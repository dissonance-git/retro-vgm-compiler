#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// High-quality realtime SN76489 source renderer.
//
// This is intentionally a stem renderer, not a stereo mixer. Each PSG source
// remains isolated so later source-aware mixing/spatialization can make its own
// decisions without reverse-engineering a summed waveform.
class sn76489_enhanced {
public:
    static constexpr std::size_t stem_count = 4;

    struct config {
        double chip_clock_hz = 3579545.0;
        double sample_rate_hz = 48000.0;
        std::uint32_t white_noise_feedback = 0x0009;
        std::uint8_t shift_register_width = 16;
        std::uint8_t oversample = 4;
    };

    sn76489_enhanced() noexcept;
    explicit sn76489_enhanced(const config& cfg) noexcept;

    void configure(const config& cfg) noexcept;
    void reset() noexcept;

    // Accepts the same latch/data byte stream as the hardware PSG.
    void write(std::uint8_t data) noexcept;
    void write_stereo_mask(std::uint8_t mask) noexcept { stereo_mask_ = mask; }

    // Render four mono floating-point stems. Any output pointer may be null.
    // No allocation or locking occurs in this function.
    void render(float* const outputs[stem_count], std::size_t frames) noexcept;

    std::uint16_t tone_period(std::size_t channel) const noexcept;
    std::uint8_t attenuation(std::size_t channel) const noexcept;
    std::uint8_t noise_control() const noexcept { return noise_control_; }
    std::uint16_t noise_lfsr() const noexcept { return noise_lfsr_; }
    std::uint8_t stereo_mask() const noexcept { return stereo_mask_; }

private:
    static double poly_blep(double phase, double phase_step) noexcept;
    static double attenuation_gain(std::uint8_t attenuation) noexcept;

    double render_tone(std::size_t channel, double internal_rate) noexcept;
    double render_noise(double internal_rate) noexcept;
    void clock_noise_lfsr() noexcept;
    double noise_shift_rate_hz() const noexcept;

    config cfg_{};
    std::array<std::uint16_t, 3> tone_periods_{{1, 1, 1}};
    std::array<std::uint8_t, 4> attenuation_{{15, 15, 15, 15}};
    std::array<double, 3> tone_phase_{};

    std::uint8_t latched_channel_ = 0;
    bool latched_volume_ = false;
    std::uint8_t noise_control_ = 0;
    std::uint8_t stereo_mask_ = 0xFF;

    std::uint16_t noise_lfsr_ = 0x8000;
    double noise_phase_ = 0.0;
};

} // namespace gameaudio::vgm
