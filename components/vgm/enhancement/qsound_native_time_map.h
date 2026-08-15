#pragma once

#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

struct qsound_native_time_point {
    std::uint64_t native_floor = 0;
    std::uint32_t fraction_numerator = 0;
    std::uint32_t fraction_denominator = 0;
};

// Project-owned absolute-time map for one coherent QSound source bus.
// It does not claim to reproduce libvgm RESMPL_STATE interpolation history.
class qsound_native_time_map {
public:
    bool configure(std::uint32_t native_rate, std::uint32_t output_rate) noexcept {
        native_rate_ = native_rate;
        output_rate_ = output_rate;
        return native_rate_ != 0 && output_rate_ != 0 && output_rate_ >= native_rate_;
    }

    bool supported() const noexcept {
        return native_rate_ != 0 && output_rate_ >= native_rate_;
    }

    std::uint32_t native_rate() const noexcept { return native_rate_; }
    std::uint32_t output_rate() const noexcept { return output_rate_; }

    bool project(std::uint64_t output_sample, qsound_native_time_point& out) const noexcept {
        if (!supported())
            return false;

        const std::uint64_t whole_seconds = output_sample / output_rate_;
        const std::uint64_t within_second = output_sample % output_rate_;
        if (whole_seconds > std::numeric_limits<std::uint64_t>::max() / native_rate_)
            return false;

        const std::uint64_t second_base = whole_seconds * native_rate_;
        const std::uint64_t within_product = within_second * native_rate_;
        const std::uint64_t within_base = within_product / output_rate_;
        if (second_base > std::numeric_limits<std::uint64_t>::max() - within_base)
            return false;

        out.native_floor = second_base + within_base;
        out.fraction_numerator = static_cast<std::uint32_t>(within_product % output_rate_);
        out.fraction_denominator = output_rate_;
        return true;
    }

private:
    std::uint32_t native_rate_ = 0;
    std::uint32_t output_rate_ = 0;
};

} // namespace gameaudio::vgm
