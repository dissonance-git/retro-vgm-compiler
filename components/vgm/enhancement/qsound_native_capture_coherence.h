#pragma once

#include "qsound_native_mix_capture.h"
#include "qsound_native_source_capture.h"

#include <cstdint>

namespace gameaudio::vgm {

enum class qsound_native_coherence : std::uint8_t {
    coherent = 0,
    source_capture_invalid = 1,
    mix_capture_invalid = 2,
    frame_count_mismatch = 3,
    sample_rate_mismatch = 4,
    first_sample_mismatch = 5,
};

inline qsound_native_coherence qsound_compare_native_captures(
    const qsound_native_source_capture& source,
    const qsound_native_mix_capture& mix) noexcept
{
    if (!source.valid())
        return qsound_native_coherence::source_capture_invalid;
    if (!mix.valid())
        return qsound_native_coherence::mix_capture_invalid;

    // Both observers may legitimately see an empty decode block. No timeline
    // exists to compare until at least one native frame was emitted.
    if (source.count() == 0 && mix.count() == 0)
        return qsound_native_coherence::coherent;

    if (source.count() != mix.count())
        return qsound_native_coherence::frame_count_mismatch;
    if (source.native_sample_rate() != mix.native_sample_rate())
        return qsound_native_coherence::sample_rate_mismatch;
    if (source.first_native_sample() != mix.first_native_sample())
        return qsound_native_coherence::first_sample_mismatch;

    // Each capture independently rejects gaps and reordering. Equal nonzero
    // count plus equal first index therefore proves the complete native index
    // sequence agrees, without scanning the block a second time.
    return qsound_native_coherence::coherent;
}

} // namespace gameaudio::vgm
