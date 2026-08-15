#pragma once

#include "qsound_echo_input_recomposition.h"
#include "qsound_echo_state.h"
#include "qsound_native_mix_capture.h"
#include "qsound_native_source_capture.h"

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

enum class qsound_echo_block_status : std::uint8_t {
    exact = 0,
    source_capture_invalid = 1,
    mix_capture_invalid = 2,
    capture_shape_mismatch = 3,
    seed_invalid = 4,
    echo_input_uncertified = 5,
    echo_input_mismatch = 6,
    echo_step_uncertified = 7,
    echo_output_mismatch = 8,
};

struct qsound_echo_block_result {
    qsound_echo_block_status status = qsound_echo_block_status::exact;
    std::size_t checked_frames = 0;
    std::size_t unavailable_frames = 0;
    std::size_t failure_index = 0;
    qsound_echo_seed final_seed{};
};

inline bool qsound_echo_seed_equal(
    const qsound_echo_seed& lhs,
    const qsound_echo_seed& rhs) noexcept
{
    return lhs.last_sample == rhs.last_sample &&
        lhs.delay_position == rhs.delay_position &&
        lhs.delay_line == rhs.delay_line;
}

// Verifies one native QSound block from an exact reference seed. The two causal
// arrows are checked separately:
//
//   PCM source samples + same-tick echo sends -> echo_input
//   seeded echo memory + echo_input + runtime feedback/length -> echo_output
//
// Init/filter-refresh ticks remain on the native timeline but do not advance the
// shared echo state because the reference core did not execute the normal echo
// transition on those ticks.
inline qsound_echo_block_result qsound_verify_echo_block(
    const qsound_native_source_capture& source_capture,
    const qsound_native_mix_capture& mix_capture,
    const qsound_echo_seed& start_seed) noexcept
{
    qsound_echo_block_result result;
    result.final_seed = start_seed;

    if (!source_capture.valid()) {
        result.status = qsound_echo_block_status::source_capture_invalid;
        return result;
    }
    if (!mix_capture.valid()) {
        result.status = qsound_echo_block_status::mix_capture_invalid;
        return result;
    }
    if (source_capture.count() != mix_capture.count() ||
        source_capture.native_sample_rate() != mix_capture.native_sample_rate() ||
        (source_capture.count() != 0 &&
            source_capture.first_native_sample() != mix_capture.first_native_sample())) {
        result.status = qsound_echo_block_status::capture_shape_mismatch;
        return result;
    }

    qsound_echo_state state;
    if (!state.load_seed(start_seed)) {
        result.status = qsound_echo_block_status::seed_invalid;
        return result;
    }

    const qsound_native_source_frame* source = source_capture.frames();
    const qsound_native_mix_frame* mix = mix_capture.frames();
    for (std::size_t index = 0; index < source_capture.count(); ++index) {
        result.failure_index = index;
        if (source[index].native_sample != mix[index].native_sample) {
            result.status = qsound_echo_block_status::capture_shape_mismatch;
            result.final_seed = state.seed();
            return result;
        }

        if (!mix[index].accounting_valid) {
            ++result.unavailable_frames;
            continue;
        }

        const qsound_echo_input_result input_result =
            qsound_recompose_echo_input(source[index], mix[index]);
        if (input_result.status != qsound_echo_input_status::exact) {
            result.status = qsound_echo_block_status::echo_input_uncertified;
            result.final_seed = state.seed();
            return result;
        }
        if (input_result.recomposed != mix[index].echo_input) {
            result.status = qsound_echo_block_status::echo_input_mismatch;
            result.final_seed = state.seed();
            return result;
        }

        const qsound_echo_step_result echo_result = state.step_runtime(
            mix[index].echo_input,
            mix[index].echo_feedback,
            mix[index].echo_length);
        if (echo_result.status != qsound_echo_step_status::exact) {
            result.status = qsound_echo_block_status::echo_step_uncertified;
            result.final_seed = state.seed();
            return result;
        }
        if (echo_result.output != mix[index].echo_output) {
            result.status = qsound_echo_block_status::echo_output_mismatch;
            result.final_seed = state.seed();
            return result;
        }

        ++result.checked_frames;
    }

    result.failure_index = source_capture.count();
    result.final_seed = state.seed();
    return result;
}

} // namespace gameaudio::vgm
