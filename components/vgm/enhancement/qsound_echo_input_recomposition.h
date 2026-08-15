#pragma once

#include "qsound_native_mix_capture.h"
#include "qsound_native_source_capture.h"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

enum class qsound_echo_input_status : std::uint8_t {
    exact = 0,
    accounting_unavailable = 1,
    native_sample_mismatch = 2,
    historical_int32_overflow_domain = 3,
};

struct qsound_echo_input_result {
    qsound_echo_input_status status = qsound_echo_input_status::exact;
    std::int32_t recomposed = 0;
};

inline qsound_echo_input_result qsound_recompose_echo_input(
    const qsound_native_source_frame& source,
    const qsound_native_mix_frame& mix) noexcept
{
    if (!mix.accounting_valid)
        return {qsound_echo_input_status::accounting_unavailable, 0};
    if (source.native_sample != mix.native_sample)
        return {qsound_echo_input_status::native_sample_mismatch, 0};

    constexpr std::int64_t int32_min = std::numeric_limits<std::int32_t>::min();
    constexpr std::int64_t int32_max = std::numeric_limits<std::int32_t>::max();
    std::int64_t accumulator = 0;

    // Recovered superctr relation for each of the 16 PCM voices:
    //   echo_input += (voice_output * voice.echo) << 2
    // Evaluate the intended signed arithmetic in a wide type and refuse to
    // certify samples whose multiply-by-four term or running accumulator leaves
    // the historical INT32 domain. This avoids relying on compiler-specific
    // signed-overflow behavior while preserving negative source/send values.
    for (std::size_t voice = 0; voice < qsound_native_pcm_count; ++voice) {
        const std::int64_t product = static_cast<std::int64_t>(source.source[voice])
            * static_cast<std::int64_t>(mix.pcm_echo_contribution[voice]);
        const std::int64_t scaled = product * 4;
        if (scaled < int32_min || scaled > int32_max)
            return {qsound_echo_input_status::historical_int32_overflow_domain, 0};

        const std::int64_t next = accumulator + scaled;
        if (next < int32_min || next > int32_max)
            return {qsound_echo_input_status::historical_int32_overflow_domain, 0};
        accumulator = next;
    }

    return {
        qsound_echo_input_status::exact,
        static_cast<std::int32_t>(accumulator),
    };
}

inline bool qsound_echo_input_recomposes_exactly(
    const qsound_native_source_frame& source,
    const qsound_native_mix_frame& mix) noexcept
{
    const qsound_echo_input_result result = qsound_recompose_echo_input(source, mix);
    return result.status == qsound_echo_input_status::exact &&
        result.recomposed == mix.echo_input;
}

} // namespace gameaudio::vgm
