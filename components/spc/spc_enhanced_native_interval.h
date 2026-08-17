#pragma once

#include "spc_enhanced_reconstruction.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

// Exact sampled-voice state for one native 32 kHz S-DSP interval. In the
// hardware/reference path, one output sample is reconstructed at start_q12 and
// the voice's source position then advances by pitch_step_q12 before the next
// native output. Enhanced rendering can evaluate that same source trajectory at
// sub-interval host phases instead of first collapsing it to a 32 kHz waveform.
//
// This is the key distinction between genuine source-domain 96 kHz rendering
// and ordinary 32 kHz -> 96 kHz output resampling.
struct spc_enhanced_native_interval {
    static constexpr std::size_t decoded_sample_count = 16;

    // Exact decoded BRR sample neighborhood after the native step has supplied
    // any samples needed to cover this interval. Index zero corresponds to
    // decoded_origin_sample in the local sample-coordinate system.
    std::array<std::int16_t, decoded_sample_count> decoded{};
    std::int32_t decoded_origin_sample = 0;

    // Source position and pitch use the S-DSP's q12 sample-coordinate unit.
    // start_q12 is the coordinate used for the current native output. The next
    // native output is nominally start_q12 + pitch_step_q12, subject to the
    // producer's exact PMON/KON/clamp behavior before this snapshot is emitted.
    std::int32_t start_q12 = 0;
    std::int32_t pitch_step_q12 = 0;

    std::uint16_t envelope = 0;
    bool noise_enabled = false;
    std::int16_t noise_sample = 0;
};

struct spc_enhanced_interval_sample {
    double pre_envelope = 0.0;
    std::int16_t post_envelope = 0;
    bool valid = false;
};

// Evaluate one host sample inside the native interval. `phase` is normalized to
// [0,1): 0 is the exact current native source coordinate; 1 would be the next
// native output and belongs to the next interval. Noise remains native-rate and
// is held because inventing sub-32k noise evolution would be false source data.
inline spc_enhanced_interval_sample reconstruct_spc_native_interval(
    const spc_enhanced_native_interval& interval,
    double phase) noexcept
{
    spc_enhanced_interval_sample result;
    if (!std::isfinite(phase) || phase < 0.0 || phase >= 1.0)
        return result;
    if (interval.envelope > 0x07FFu || interval.pitch_step_q12 < 0)
        return result;

    if (interval.noise_enabled) {
        result.pre_envelope = static_cast<double>(interval.noise_sample);
        result.post_envelope = apply_spc_reference_envelope_quantization(
            result.pre_envelope,
            interval.envelope);
        result.valid = true;
        return result;
    }

    const double position_q12 = static_cast<double>(interval.start_q12)
        + static_cast<double>(interval.pitch_step_q12) * phase;
    if (!std::isfinite(position_q12))
        return result;

    const double position = position_q12 / 4096.0;
    const double local_position = position
        - static_cast<double>(interval.decoded_origin_sample);
    if (!std::isfinite(local_position))
        return result;

    const std::ptrdiff_t center = static_cast<std::ptrdiff_t>(std::floor(local_position));
    const double fraction = local_position - static_cast<double>(center);

    // Lanczos-4 needs integer samples center-3 ... center+4. A realtime producer
    // is expected to delay publication of the interval until those exact decoded
    // neighbors exist. Missing neighbors fail closed instead of edge-padding the
    // live BRR trajectory with invented values.
    if (center < 3 || center + 4 >= static_cast<std::ptrdiff_t>(interval.decoded.size()))
        return result;

    double weighted = 0.0;
    double weight_sum = 0.0;
    for (std::ptrdiff_t offset = -3; offset <= 4; ++offset) {
        const std::ptrdiff_t index = center + offset;
        const double distance = static_cast<double>(offset) - fraction;
        const double weight = spc_lanczos4_weight(distance);
        weighted += static_cast<double>(interval.decoded[static_cast<std::size_t>(index)])
            * weight;
        weight_sum += weight;
    }

    if (!std::isfinite(weighted) || !std::isfinite(weight_sum)
        || std::abs(weight_sum) < 1.0e-12)
        return result;

    result.pre_envelope = weighted / weight_sum;
    if (!std::isfinite(result.pre_envelope))
        return result;
    result.post_envelope = apply_spc_reference_envelope_quantization(
        result.pre_envelope,
        interval.envelope);
    result.valid = true;
    return result;
}

// Render one native S-DSP interval directly at an integer output-rate multiple.
// 96 kHz therefore produces phases 0, 1/3, 2/3 from the source trajectory
// itself, not three samples interpolated from an already-final 32 kHz waveform.
template <std::size_t MaxOversample = 8>
inline bool render_spc_native_interval_multiple(
    const spc_enhanced_native_interval& interval,
    std::size_t output_samples_per_native_interval,
    std::array<std::int16_t, MaxOversample>& output) noexcept
{
    if (output_samples_per_native_interval == 0
        || output_samples_per_native_interval > MaxOversample)
        return false;

    for (std::size_t sample = 0; sample < output_samples_per_native_interval; ++sample) {
        const double phase = static_cast<double>(sample)
            / static_cast<double>(output_samples_per_native_interval);
        const auto reconstructed = reconstruct_spc_native_interval(interval, phase);
        if (!reconstructed.valid)
            return false;
        output[sample] = reconstructed.post_envelope;
    }
    return true;
}

} // namespace gameaudio::spc
