#pragma once

#include "sn76489_enhanced.h"

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

struct sn76489_route_segment {
    std::size_t begin_frame = 0;
    std::size_t end_frame = 0;
    std::uint8_t stereo_mask = 0xFF;
};

struct sn76489_route_segment_result {
    std::size_t segment_count = 0;
    std::uint8_t final_stereo_mask = 0xFF;
    bool valid_order = true;
    bool overflowed = false;
};

// Convert sample-timed Game Gear stereo-mask writes into constant-routing
// intervals for one rendered block. Register writes are ignored here because
// they affect source synthesis, not the source's authored output route.
//
// Multiple mask writes at the same output sample collapse naturally: no
// zero-length segment is emitted and the last write at that sample determines
// routing for the following sample. A mask write at offset == frames affects
// only final_stereo_mask and therefore the next block.
inline sn76489_route_segment_result build_sn76489_route_segments(
    std::uint8_t initial_stereo_mask,
    const sn76489_timed_write* writes,
    std::size_t write_count,
    std::size_t frames,
    sn76489_route_segment* segments,
    std::size_t segment_capacity) noexcept {
    sn76489_route_segment_result result;
    result.final_stereo_mask = initial_stereo_mask;

    std::size_t cursor = 0;
    std::size_t previous_offset = 0;
    bool have_previous = false;

    auto append = [&](std::size_t begin, std::size_t end, std::uint8_t mask) noexcept {
        if (begin >= end)
            return;
        if (segments == nullptr || result.segment_count >= segment_capacity) {
            result.overflowed = true;
            return;
        }
        segments[result.segment_count++] = sn76489_route_segment{begin, end, mask};
    };

    for (std::size_t index = 0; index < write_count; ++index) {
        const sn76489_timed_write& write = writes[index];
        if (have_previous && write.sample_offset < previous_offset) {
            result.valid_order = false;
            return result;
        }
        previous_offset = write.sample_offset;
        have_previous = true;

        if (write.kind != sn76489_write_kind::stereo_mask)
            continue;

        const std::size_t offset = write.sample_offset < frames ? write.sample_offset : frames;
        if (offset > cursor) {
            append(cursor, offset, result.final_stereo_mask);
            cursor = offset;
        }
        result.final_stereo_mask = write.data;
    }

    if (cursor < frames)
        append(cursor, frames, result.final_stereo_mask);

    return result;
}

} // namespace gameaudio::vgm
