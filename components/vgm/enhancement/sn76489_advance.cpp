#include "sn76489_enhanced.h"

#include <cmath>

namespace gameaudio::vgm {

void sn76489_enhanced::advance(std::size_t frames) noexcept {
    if (!supported_ || frames == 0 || !(cfg_.sample_rate_hz > 0.0))
        return;

    const double divider = 4.0 * static_cast<double>(cfg_.clock_divider);
    const double frame_count = static_cast<double>(frames);

    // Tone channels are deterministic phase accumulators, so skipped time can
    // be advanced analytically rather than generating discarded samples.
    for (std::size_t channel = 0; channel < tone_phase_.size(); ++channel) {
        const std::uint16_t period = normalized_period(tone_periods_[channel]);
        const double frequency = cfg_.chip_clock_hz / (divider * static_cast<double>(period));
        const double cycles = frequency * frame_count / cfg_.sample_rate_hz;
        tone_phase_[channel] = std::fmod(tone_phase_[channel] + cycles, 1.0);
        if (tone_phase_[channel] < 0.0)
            tone_phase_[channel] += 1.0;
    }

    // Noise cannot be skipped by phase alone because every shift changes the
    // LFSR state. Still, advancing one LFSR step is far cheaper than rendering
    // an oversampled audio block, so seeks scale with source transitions rather
    // than output samples.
    const double shifts = noise_shift_rate_hz() * frame_count / cfg_.sample_rate_hz;
    double total_phase = noise_phase_ + shifts;
    const std::uint64_t whole_shifts = static_cast<std::uint64_t>(std::floor(total_phase));
    noise_phase_ = total_phase - static_cast<double>(whole_shifts);

    for (std::uint64_t shift = 0; shift < whole_shifts; ++shift)
        clock_noise_lfsr();
}

} // namespace gameaudio::vgm
