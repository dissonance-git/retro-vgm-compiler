#pragma once

#include "qsound_block_capture.h"
#include "qsound_environment_control_state.h"

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

enum class qsound_environment_replay_status : std::uint8_t {
    replayed = 0,
    capture_overflow = 1,
    non_monotonic_timeline = 2,
};

struct qsound_environment_replay_result {
    qsound_environment_replay_status status = qsound_environment_replay_status::replayed;
    std::size_t applied = 0;
};

inline qsound_environment_replay_result qsound_replay_environment_controls(
    const qsound_block_capture& capture,
    std::size_t rendered_samples,
    qsound_environment_control_state& state) noexcept
{
    if (capture.environment_overflowed())
        return {qsound_environment_replay_status::capture_overflow, 0};

    const qsound_timed_environment_control* controls = capture.environment_controls();
    const std::size_t count = capture.environment_count();

    // Validate the complete bounded stream before mutating state. A malformed
    // timeline must not leave a plausible-looking partially replayed shadow.
    std::size_t previous_offset = 0;
    bool have_previous = false;
    for (std::size_t index = 0; index < count; ++index) {
        const std::size_t offset = controls[index].sample_offset > rendered_samples
            ? rendered_samples
            : controls[index].sample_offset;
        if (have_previous && offset < previous_offset)
            return {qsound_environment_replay_status::non_monotonic_timeline, 0};
        previous_offset = offset;
        have_previous = true;
    }

    for (std::size_t index = 0; index < count; ++index)
        state.apply(controls[index].write);

    return {qsound_environment_replay_status::replayed, count};
}

} // namespace gameaudio::vgm
