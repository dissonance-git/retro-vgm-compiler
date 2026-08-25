#include "ym2612_pcm_stream.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gameaudio::vgm {

namespace {
constexpr double pi = 3.141592653589793238462643383279502884;
constexpr double lanczos_radius = 4.0;
constexpr std::uint8_t length_mode_mask = 0x0F;
constexpr std::uint8_t length_mode_ignore = 0x00;
constexpr std::uint8_t length_mode_commands = 0x01;
constexpr std::uint8_t length_mode_msec = 0x02;
constexpr std::uint8_t length_mode_to_end = 0x03;
constexpr std::uint8_t length_mode_bytes = 0x0F;
constexpr std::uint8_t reverse_flag = 0x10;
constexpr std::uint8_t loop_flag = 0x80;
}

ym2612_pcm_stream::ym2612_pcm_stream() noexcept
    : ym2612_pcm_stream(48000.0) {}

ym2612_pcm_stream::ym2612_pcm_stream(double output_sample_rate) noexcept {
    configure_output_rate(output_sample_rate);
    reset();
}

void ym2612_pcm_stream::configure_output_rate(double sample_rate) noexcept {
    output_sample_rate_ = sample_rate > 0.0 ? sample_rate : 48000.0;
}

void ym2612_pcm_stream::reset() noexcept {
    data_ = nullptr;
    data_length_ = 0;
    step_size_ = 1;
    step_base_ = 0;
    frequency_ = 0;
    data_start_ = 0;
    command_count_ = 0;
    position_ = 0.0;
    active_ = false;
    valid_ = false;
    loop_ = false;
    reverse_ = false;
}

float ym2612_pcm_stream::byte_to_level(std::uint8_t value) noexcept {
    return static_cast<float>(static_cast<int>(value) - 128) / 128.0f;
}

double ym2612_pcm_stream::sinc(double x) noexcept {
    if (std::abs(x) < 1.0e-12)
        return 1.0;
    const double pix = pi * x;
    return std::sin(pix) / pix;
}

double ym2612_pcm_stream::lanczos(double x, double radius) noexcept {
    const double ax = std::abs(x);
    if (ax >= radius)
        return 0.0;
    return sinc(x) * sinc(x / radius);
}

std::size_t ym2612_pcm_stream::maximum_command_count() const noexcept {
    if (data_ == nullptr || data_length_ == 0 || data_start_ >= data_length_ || step_size_ == 0)
        return 0;
    return 1 + (data_length_ - 1 - data_start_) / step_size_;
}

std::size_t ym2612_pcm_stream::source_index(std::int64_t logical_index) const noexcept {
    if (command_count_ == 0)
        return data_start_;

    std::int64_t resolved = logical_index;
    if (loop_) {
        const std::int64_t count = static_cast<std::int64_t>(command_count_);
        resolved %= count;
        if (resolved < 0)
            resolved += count;
    } else {
        resolved = std::max<std::int64_t>(0,
            std::min<std::int64_t>(resolved, static_cast<std::int64_t>(command_count_ - 1)));
    }

    if (reverse_)
        resolved = static_cast<std::int64_t>(command_count_ - 1) - resolved;

    const std::uint64_t index = static_cast<std::uint64_t>(data_start_) +
        static_cast<std::uint64_t>(resolved) * step_size_;
    return index < data_length_ ? static_cast<std::size_t>(index) : data_length_ - 1;
}

float ym2612_pcm_stream::source_sample(std::int64_t logical_index) const noexcept {
    if (!valid_ || data_ == nullptr || data_length_ == 0 || command_count_ == 0)
        return 0.0f;
    return byte_to_level(data_[source_index(logical_index)]);
}

float ym2612_pcm_stream::interpolate(double logical_position) const noexcept {
    if (!valid_ || command_count_ == 0)
        return 0.0f;

    const std::int64_t center = static_cast<std::int64_t>(std::floor(logical_position));
    double weighted = 0.0;
    double weight_sum = 0.0;

    for (int tap = -3; tap <= 4; ++tap) {
        const std::int64_t index = center + tap;
        const double distance = logical_position - static_cast<double>(index);
        const double weight = lanczos(distance, lanczos_radius);
        weighted += static_cast<double>(source_sample(index)) * weight;
        weight_sum += weight;
    }

    if (std::abs(weight_sum) < 1.0e-12)
        return source_sample(center);
    return static_cast<float>(weighted / weight_sum);
}

void ym2612_pcm_stream::start(const dac_stream_source_event& event) noexcept {
    if (event.start_offset != (std::numeric_limits<std::uint32_t>::max)()) {
        const std::uint64_t start = static_cast<std::uint64_t>(event.start_offset) + step_base_;
        if (start > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)())) {
            valid_ = false;
            active_ = false;
            return;
        }
        data_start_ = static_cast<std::size_t>(start);
    }

    const std::size_t max_count = maximum_command_count();
    const std::uint8_t length_mode = static_cast<std::uint8_t>(event.play_mode & length_mode_mask);
    std::uint64_t requested = command_count_;

    switch (length_mode) {
    case length_mode_ignore:
        break;
    case length_mode_commands:
        requested = event.length;
        break;
    case length_mode_msec:
        // VGM 0x93 mode 02 is milliseconds. Convert time to source commands
        // using the authored stream frequency, not the inverse relationship.
        requested = frequency_ == 0
            ? 0
            : (static_cast<std::uint64_t>(frequency_) * event.length) / 1000u;
        break;
    case length_mode_to_end:
        requested = max_count;
        break;
    case length_mode_bytes:
        requested = step_size_ == 0 ? 0 : event.length / step_size_;
        break;
    default:
        requested = 0;
        break;
    }

    command_count_ = static_cast<std::size_t>(
        std::min<std::uint64_t>(requested, static_cast<std::uint64_t>(max_count)));
    loop_ = (event.play_mode & loop_flag) != 0;
    reverse_ = (event.play_mode & reverse_flag) != 0;
    position_ = 0.0;
    valid_ = data_ != nullptr && data_length_ != 0 && frequency_ != 0 && command_count_ != 0;
    active_ = valid_;
}

void ym2612_pcm_stream::apply(const dac_stream_source_event& event) noexcept {
    if (event.data != nullptr || event.data_length == 0) {
        data_ = event.data;
        data_length_ = event.data_length;
    }
    if (event.step_size != 0)
        step_size_ = event.step_size;
    step_base_ = event.step_base;
    if (event.frequency != 0)
        frequency_ = event.frequency;

    switch (event.kind) {
    case dac_stream_source_event_kind::setup:
        break;
    case dac_stream_source_event_kind::set_data:
        valid_ = data_ != nullptr && data_length_ != 0;
        break;
    case dac_stream_source_event_kind::set_frequency:
        valid_ = data_ != nullptr && data_length_ != 0 && frequency_ != 0;
        break;
    case dac_stream_source_event_kind::start:
        start(event);
        break;
    case dac_stream_source_event_kind::stop:
        active_ = false;
        break;
    }
}

void ym2612_pcm_stream::advance_position(double command_delta) noexcept {
    if (!active_ || !valid_ || command_count_ == 0 || !(command_delta > 0.0))
        return;

    position_ += command_delta;
    if (position_ < static_cast<double>(command_count_))
        return;

    if (loop_) {
        position_ = std::fmod(position_, static_cast<double>(command_count_));
        if (position_ < 0.0)
            position_ += static_cast<double>(command_count_);
    } else {
        position_ = static_cast<double>(command_count_);
        active_ = false;
    }
}

void ym2612_pcm_stream::render(float* output, std::size_t frames) noexcept {
    if (output != nullptr)
        std::fill(output, output + frames, 0.0f);
    if (frames == 0 || !active_ || !valid_ || frequency_ == 0 || !(output_sample_rate_ > 0.0))
        return;

    const double command_step = static_cast<double>(frequency_) / output_sample_rate_;
    for (std::size_t frame = 0; frame < frames; ++frame) {
        if (!active_)
            break;
        if (output != nullptr)
            output[frame] = interpolate(position_);
        advance_position(command_step);
    }
}

void ym2612_pcm_stream::advance(std::size_t frames) noexcept {
    if (frames == 0 || !active_ || !valid_ || frequency_ == 0 || !(output_sample_rate_ > 0.0))
        return;
    const double command_delta = static_cast<double>(frequency_) *
        static_cast<double>(frames) / output_sample_rate_;
    advance_position(command_delta);
}

} // namespace gameaudio::vgm
