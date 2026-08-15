#pragma once

#include "qsound_block_capture.h"
#include "qsound_control_state.h"

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

struct qsound_source_spatial_segment {
    std::size_t begin_frame = 0;
    std::size_t end_frame = 0;
    qsound_spatial_source source{};
};

struct qsound_source_spatial_segment_result {
    std::size_t segment_count = 0;
    qsound_control_state final_state{};
    bool valid_source = true;
    bool valid_order = true;
    bool structurally_valid = true;
    bool overflowed = false;
};

// Convert one block of sample-timed QSound source controls into constant-routing
// intervals for one physical source lane. The mono source audio remains separate;
// these segments describe the source-facing spatial/effect state that applies to
// that audio over the same output-sample interval.
//
// Pan and PCM echo-contribution writes are both retained because QSound's wet
// pan-table route and its signed source-to-shared-echo contribution are distinct
// causal controls. Writes for other physical sources do not split this source's
// timeline. Multiple writes at one sample collapse naturally into the state that
// applies after that sample boundary.
inline qsound_source_spatial_segment_result build_qsound_source_spatial_segments(
    const qsound_control_state& initial_state,
    std::uint8_t instance,
    std::uint8_t physical_slot,
    std::uint32_t episode_generation,
    const qsound_timed_source_control* writes,
    std::size_t write_count,
    std::size_t frames,
    qsound_source_spatial_segment* segments,
    std::size_t segment_capacity) noexcept {
    qsound_source_spatial_segment_result result;
    result.final_state = initial_state;

    if (physical_slot >= qsound_source_count) {
        result.valid_source = false;
        result.structurally_valid = false;
        return result;
    }
    if (write_count != 0 && writes == nullptr) {
        result.structurally_valid = false;
        return result;
    }

    std::size_t cursor = 0;
    std::size_t previous_offset = 0;
    bool have_previous = false;

    auto append = [&](std::size_t begin, std::size_t end) noexcept {
        if (begin >= end)
            return;
        if (segments == nullptr || result.segment_count >= segment_capacity) {
            result.overflowed = true;
            return;
        }
        segments[result.segment_count++] = qsound_source_spatial_segment{
            begin,
            end,
            result.final_state.source(instance, physical_slot, episode_generation),
        };
    };

    for (std::size_t index = 0; index < write_count; ++index) {
        const qsound_timed_source_control& timed = writes[index];
        if (have_previous && timed.sample_offset < previous_offset) {
            result.valid_order = false;
            result.structurally_valid = false;
            return result;
        }
        previous_offset = timed.sample_offset;
        have_previous = true;

        if (timed.write.physical_slot != physical_slot)
            continue;

        const std::size_t offset = timed.sample_offset < frames ? timed.sample_offset : frames;
        if (offset > cursor) {
            append(cursor, offset);
            cursor = offset;
        }

        if (!result.final_state.apply(timed.write)) {
            result.structurally_valid = false;
            return result;
        }
    }

    if (cursor < frames)
        append(cursor, frames);

    return result;
}

} // namespace gameaudio::vgm
