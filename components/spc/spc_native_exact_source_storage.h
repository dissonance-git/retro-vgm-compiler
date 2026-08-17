#pragma once

#include "spc_native_source_capture.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::spc {

enum class spc_native_exact_source_error : std::uint8_t {
    none = 0,
    invalid_capture,
    unsupported_output_rate,
    frame_count_mismatch,
    native_timeline_mismatch,
    native_timeline_overflow,
    reference_timeline_overflow,
    capacity_exceeded,
};

// Planar float storage for the one case where host/source alignment is exact by
// construction: the protected SPC renderer is itself running at the S-DSP
// native 32 kHz rate. No FIR phase, interpolation, or reconstructed time map is
// involved in this path.
template <std::size_t CapacityFrames = 4096>
class spc_native_exact_source_storage {
    static_assert(CapacityFrames > 0, "CapacityFrames must be non-zero");

public:
    void reset() noexcept {
        frame_count_ = 0;
        native_start_ = 0;
        reference_start_ = 0;
        valid_ = false;
        last_error_ = spc_native_exact_source_error::none;
    }

    bool build(
        const spc_native_source_capture& capture,
        std::uint32_t output_sample_rate,
        std::uint64_t reference_frame_start,
        std::uint64_t expected_native_sample_start,
        std::size_t reference_frame_count) noexcept
    {
        reset();

        if (!capture.valid())
            return fail(spc_native_exact_source_error::invalid_capture);
        if (output_sample_rate != spc_native_sample_rate)
            return fail(spc_native_exact_source_error::unsupported_output_rate);
        if (reference_frame_count > CapacityFrames)
            return fail(spc_native_exact_source_error::capacity_exceeded);
        if (capture.count() != reference_frame_count)
            return fail(spc_native_exact_source_error::frame_count_mismatch);
        if (reference_frame_count != 0 &&
            capture.first_native_sample() != expected_native_sample_start)
            return fail(spc_native_exact_source_error::native_timeline_mismatch);
        if (reference_frame_count != 0 &&
            static_cast<std::uint64_t>(reference_frame_count - 1u) >
                std::numeric_limits<std::uint64_t>::max() - expected_native_sample_start)
            return fail(spc_native_exact_source_error::native_timeline_overflow);
        if (reference_frame_count >
            std::numeric_limits<std::uint64_t>::max() - reference_frame_start)
            return fail(spc_native_exact_source_error::reference_timeline_overflow);

        constexpr float int16_scale = 1.0f / 32768.0f;
        const spc_native_source_frame* frames = capture.frames();
        for (std::size_t frame = 0; frame < reference_frame_count; ++frame) {
            const std::uint64_t expected_native =
                expected_native_sample_start + static_cast<std::uint64_t>(frame);
            if (frames[frame].native_sample != expected_native)
                return fail(spc_native_exact_source_error::native_timeline_mismatch);

            availability_[frame] = 1u;
            for (std::size_t voice = 0; voice < spc_native_voice_count; ++voice) {
                lane_[voice][frame] =
                    static_cast<float>(frames[frame].source[voice]) * int16_scale;
            }
        }

        frame_count_ = reference_frame_count;
        native_start_ = expected_native_sample_start;
        reference_start_ = reference_frame_start;
        valid_ = true;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    std::size_t frame_count() const noexcept { return frame_count_; }
    std::uint64_t native_start() const noexcept { return native_start_; }
    std::uint64_t reference_start() const noexcept { return reference_start_; }
    spc_native_exact_source_error last_error() const noexcept { return last_error_; }

    const float* lane(std::size_t voice) const noexcept {
        return voice < spc_native_voice_count ? lane_[voice].data() : nullptr;
    }
    const std::uint8_t* availability() const noexcept {
        return availability_.data();
    }

private:
    bool fail(spc_native_exact_source_error error) noexcept {
        last_error_ = error;
        valid_ = false;
        return false;
    }

    std::array<std::array<float, CapacityFrames>, spc_native_voice_count> lane_{};
    std::array<std::uint8_t, CapacityFrames> availability_{};
    std::size_t frame_count_ = 0;
    std::uint64_t native_start_ = 0;
    std::uint64_t reference_start_ = 0;
    bool valid_ = false;
    spc_native_exact_source_error last_error_ = spc_native_exact_source_error::none;
};

} // namespace gameaudio::spc
