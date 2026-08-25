#pragma once

#include "dac_stream_source.h"

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// High-quality source-bank renderer for VGM DAC streams targeting YM2612 $2A.
// It consumes the original PCM bank directly at the stream's authored write
// frequency, bypassing the historical DAC staircase while retaining source
// bytes, stepping, start/length, reverse and loop semantics.
class ym2612_pcm_stream {
public:
    ym2612_pcm_stream() noexcept;
    explicit ym2612_pcm_stream(double output_sample_rate) noexcept;

    void configure_output_rate(double sample_rate) noexcept;
    void reset() noexcept;
    void apply(const dac_stream_source_event& event) noexcept;

    void render(float* output, std::size_t frames) noexcept;
    void advance(std::size_t frames) noexcept;

    bool active() const noexcept { return active_; }
    bool valid() const noexcept { return valid_; }
    bool looping() const noexcept { return loop_; }
    bool reverse() const noexcept { return reverse_; }
    std::uint32_t frequency() const noexcept { return frequency_; }
    std::size_t command_count() const noexcept { return command_count_; }
    double position() const noexcept { return position_; }

private:
    static float byte_to_level(std::uint8_t value) noexcept;
    static double sinc(double x) noexcept;
    static double lanczos(double x, double radius) noexcept;

    std::size_t source_index(std::int64_t logical_index) const noexcept;
    float source_sample(std::int64_t logical_index) const noexcept;
    float interpolate(double logical_position) const noexcept;
    std::size_t maximum_command_count() const noexcept;
    void start(const dac_stream_source_event& event) noexcept;
    void advance_position(double command_delta) noexcept;

    double output_sample_rate_ = 48000.0;
    const std::uint8_t* data_ = nullptr;
    std::size_t data_length_ = 0;
    std::uint8_t step_size_ = 1;
    std::uint8_t step_base_ = 0;
    std::uint32_t frequency_ = 0;

    std::size_t data_start_ = 0;
    std::size_t command_count_ = 0;
    double position_ = 0.0;
    bool active_ = false;
    bool valid_ = false;
    bool loop_ = false;
    bool reverse_ = false;
};

} // namespace gameaudio::vgm
