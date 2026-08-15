#pragma once

#include "qsound_native_mix_capture.h"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

enum class qsound_recomposition_status : std::uint8_t {
    exact = 0,
    historical_int32_overflow_domain = 1,
};

struct qsound_recomposition_result {
    qsound_recomposition_status status = qsound_recomposition_status::exact;
    std::int16_t output = 0;
};

inline qsound_recomposition_result qsound_recompose_reference_channel(
    std::int32_t wet_post_delay,
    std::int32_t dry_post_delay) noexcept
{
    // superctr performs these operations in signed INT32:
    //   output = wet + dry;
    //   output = (output + 0x2000) >> 14;
    // Audit in 64 bits and refuse to call an edge case exact when either
    // historical INT32 operation would overflow in portable C/C++ semantics.
    const std::int64_t sum = static_cast<std::int64_t>(wet_post_delay)
        + static_cast<std::int64_t>(dry_post_delay);
    constexpr std::int64_t int32_min = std::numeric_limits<std::int32_t>::min();
    constexpr std::int64_t int32_max = std::numeric_limits<std::int32_t>::max();
    constexpr std::int64_t rounding_bias = 0x2000;
    constexpr std::int64_t q14 = 1ll << 14;

    if (sum < int32_min || sum > int32_max - rounding_bias)
        return {qsound_recomposition_status::historical_int32_overflow_domain, 0};

    const std::int64_t biased = sum + rounding_bias;
    std::int64_t rounded = biased / q14;
    if (biased < 0 && (biased % q14) != 0)
        --rounded; // explicit arithmetic right-shift semantics

    if (rounded > std::numeric_limits<std::int16_t>::max())
        rounded = std::numeric_limits<std::int16_t>::max();
    else if (rounded < std::numeric_limits<std::int16_t>::min())
        rounded = std::numeric_limits<std::int16_t>::min();

    return {
        qsound_recomposition_status::exact,
        static_cast<std::int16_t>(rounded),
    };
}

inline bool qsound_reference_frame_recomposes_exactly(
    const qsound_native_mix_frame& frame) noexcept
{
    for (std::size_t ch = 0; ch < 2; ++ch) {
        const qsound_recomposition_result recomposed = qsound_recompose_reference_channel(
            frame.wet_post_delay[ch], frame.dry_post_delay[ch]);
        if (recomposed.status != qsound_recomposition_status::exact ||
            recomposed.output != frame.reference_output[ch])
            return false;
    }
    return true;
}

} // namespace gameaudio::vgm
