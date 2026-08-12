#include "sn76489_enhanced.h"

#include <algorithm>
#include <cmath>

namespace gameaudio::vgm {

namespace {

constexpr double pi = 3.141592653589793238462643383279502884;
constexpr double minimum_rate = 1.0;
constexpr std::uint16_t maximum_period = 0x03FF;

std::uint8_t clamp_oversample(std::uint8_t value) noexcept {
    if (value == 0)
        return 1;
    return static_cast<std::uint8_t>(std::min<unsigned>(value, 16));
}

std::uint16_t noise_initial_state(std::uint8_t width) noexcept {
    const std::uint8_t clamped = static_cast<std::uint8_t>(std::max<unsigned>(1, std::min<unsigned>(width, 16)));
    return static_cast<std::uint16_t>(1u << (clamped - 1));
}

} // namespace

sn76489_enhanced::sn76489_enhanced() noexcept {
    configure(config{});
}

sn76489_enhanced::sn76489_enhanced(const config& cfg) noexcept {
    configure(cfg);
}

void sn76489_enhanced::configure(const config& cfg) noexcept {
    cfg_ = cfg;
    if (!(cfg_.chip_clock_hz > 0.0))
        cfg_.chip_clock_hz = 3579545.0;
    if (!(cfg_.sample_rate_hz > 0.0))
        cfg_.sample_rate_hz = 48000.0;
    cfg_.oversample = clamp_oversample(cfg_.oversample);
    cfg_.shift_register_width = static_cast<std::uint8_t>(std::max<unsigned>(1, std::min<unsigned>(cfg_.shift_register_width, 16)));
    reset();
}

void sn76489_enhanced::reset() noexcept {
    tone_periods_ = {{1, 1, 1}};
    attenuation_ = {{15, 15, 15, 15}};
    tone_phase_ = {{0.0, 0.0, 0.0}};
    latched_channel_ = 0;
    latched_volume_ = false;
    noise_control_ = 0;
    stereo_mask_ = 0xFF;
    noise_lfsr_ = noise_initial_state(cfg_.shift_register_width);
    noise_phase_ = 0.0;
}

void sn76489_enhanced::write(std::uint8_t data) noexcept {
    if ((data & 0x80u) != 0) {
        latched_channel_ = static_cast<std::uint8_t>((data >> 5) & 0x03u);
        latched_volume_ = (data & 0x10u) != 0;

        if (latched_volume_) {
            attenuation_[latched_channel_] = static_cast<std::uint8_t>(data & 0x0Fu);
        } else if (latched_channel_ < 3) {
            auto& period = tone_periods_[latched_channel_];
            period = static_cast<std::uint16_t>((period & 0x03F0u) | (data & 0x0Fu));
            if (period == 0)
                period = 1;
        } else {
            noise_control_ = static_cast<std::uint8_t>(data & 0x07u);
            noise_lfsr_ = noise_initial_state(cfg_.shift_register_width);
        }
        return;
    }

    if (latched_volume_) {
        attenuation_[latched_channel_] = static_cast<std::uint8_t>(data & 0x0Fu);
    } else if (latched_channel_ < 3) {
        auto& period = tone_periods_[latched_channel_];
        period = static_cast<std::uint16_t>((period & 0x000Fu) | ((data & 0x3Fu) << 4));
        period &= maximum_period;
        if (period == 0)
            period = 1;
    } else {
        noise_control_ = static_cast<std::uint8_t>(data & 0x07u);
        noise_lfsr_ = noise_initial_state(cfg_.shift_register_width);
    }
}

double sn76489_enhanced::poly_blep(double phase, double phase_step) noexcept {
    if (!(phase_step > 0.0) || !(phase_step < 1.0))
        return 0.0;

    if (phase < phase_step) {
        const double t = phase / phase_step;
        return t + t - t * t - 1.0;
    }
    if (phase > 1.0 - phase_step) {
        const double t = (phase - 1.0) / phase_step;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

double sn76489_enhanced::attenuation_gain(std::uint8_t attenuation) noexcept {
    // The SN76489 ladder is nominally 2 dB per step. Keep the encoded mix
    // relationship while avoiding the integer quantization of the old output
    // table. 15 is hardware silence rather than another 2 dB step.
    if (attenuation >= 15)
        return 0.0;
    return std::pow(10.0, -0.1 * static_cast<double>(attenuation));
}

double sn76489_enhanced::render_tone(std::size_t channel, double internal_rate) noexcept {
    const std::uint16_t period = std::max<std::uint16_t>(1, tone_periods_[channel]);
    const double frequency = cfg_.chip_clock_hz / (32.0 * static_cast<double>(period));
    const double step = frequency / internal_rate;

    // There is no useful audible source above the renderer's internal Nyquist
    // limit. Muting it avoids converting an ultrasonic hardware setting into a
    // false lower alias product.
    if (!(step > 0.0) || step >= 0.5)
        return 0.0;

    double phase = tone_phase_[channel];
    double sample = phase < 0.5 ? 1.0 : -1.0;

    // Band-limit both discontinuities of the 50% duty square wave.
    sample += poly_blep(phase, step);
    double half_phase = phase + 0.5;
    if (half_phase >= 1.0)
        half_phase -= 1.0;
    sample -= poly_blep(half_phase, step);

    phase += step;
    phase -= std::floor(phase);
    tone_phase_[channel] = phase;

    return sample * attenuation_gain(attenuation_[channel]);
}

double sn76489_enhanced::noise_shift_rate_hz() const noexcept {
    const std::uint8_t mode = static_cast<std::uint8_t>(noise_control_ & 0x03u);
    if (mode == 3) {
        const std::uint16_t period = std::max<std::uint16_t>(1, tone_periods_[2]);
        return cfg_.chip_clock_hz / (32.0 * static_cast<double>(period));
    }

    const std::uint32_t noise_period = 16u << mode;
    return cfg_.chip_clock_hz / (32.0 * static_cast<double>(noise_period));
}

void sn76489_enhanced::clock_noise_lfsr() noexcept {
    std::uint32_t feedback = 0;
    if ((noise_control_ & 0x04u) != 0) {
        feedback = static_cast<std::uint32_t>(noise_lfsr_) & cfg_.white_noise_feedback;
        feedback ^= feedback >> 8;
        feedback ^= feedback >> 4;
        feedback ^= feedback >> 2;
        feedback ^= feedback >> 1;
        feedback &= 1u;
    } else {
        feedback = noise_lfsr_ & 1u;
    }

    noise_lfsr_ = static_cast<std::uint16_t>(
        (noise_lfsr_ >> 1) |
        (feedback << (cfg_.shift_register_width - 1)));
}

double sn76489_enhanced::render_noise(double internal_rate) noexcept {
    const bool white_noise = (noise_control_ & 0x04u) != 0;
    double sample = (noise_lfsr_ & 1u) != 0 ? 1.0 : -1.0;

    // The Maxim reference core compensates white-noise energy by 1/2. Preserve
    // that authored balance while retaining a floating-point source stem.
    if (white_noise)
        sample *= 0.5;
    sample *= attenuation_gain(attenuation_[3]);

    const double shift_rate = noise_shift_rate_hz();
    const double step = shift_rate / internal_rate;
    if (step > 0.0) {
        noise_phase_ += step;
        while (noise_phase_ >= 1.0) {
            noise_phase_ -= 1.0;
            clock_noise_lfsr();
        }
    }

    return sample;
}

void sn76489_enhanced::render(float* const outputs[stem_count], std::size_t frames) noexcept {
    const std::uint8_t oversample = cfg_.oversample;
    const double internal_rate = std::max(minimum_rate, cfg_.sample_rate_hz * static_cast<double>(oversample));
    const double inv_oversample = 1.0 / static_cast<double>(oversample);

    for (std::size_t frame = 0; frame < frames; ++frame) {
        std::array<double, stem_count> accumulated{};

        for (std::uint8_t os = 0; os < oversample; ++os) {
            for (std::size_t channel = 0; channel < 3; ++channel)
                accumulated[channel] += render_tone(channel, internal_rate);
            accumulated[3] += render_noise(internal_rate);
        }

        for (std::size_t channel = 0; channel < stem_count; ++channel) {
            if (outputs[channel] != nullptr)
                outputs[channel][frame] = static_cast<float>(accumulated[channel] * inv_oversample);
        }
    }
}

std::uint16_t sn76489_enhanced::tone_period(std::size_t channel) const noexcept {
    if (channel >= tone_periods_.size())
        return 0;
    return tone_periods_[channel];
}

std::uint8_t sn76489_enhanced::attenuation(std::size_t channel) const noexcept {
    if (channel >= attenuation_.size())
        return 15;
    return attenuation_[channel];
}

} // namespace gameaudio::vgm
