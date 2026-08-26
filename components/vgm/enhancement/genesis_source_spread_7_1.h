#pragma once

#include "genesis_selected_source_block.h"
#include "../../../model/surround_bed_7_1.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace gameaudio::vgm {

// Role-free source spread for the first YM2612 + SN76489 Surround product.
//
// The protected stereo is authoritative. Every exact isolated source keeps a
// -3 dB front anchor and sends the other half of its power into side/back
// speakers on the same left/right hemisphere. The side/back split uses a
// low-discrepancy sequence only to prevent implementation lanes from stacking
// at one depth.
//
// Slot number is therefore a spacing seed, never a statement that FM1 is bass,
// FM4 is lead, noise belongs behind the listener, etc. Every lane gets the same
// power law and every lane retains front energy.
//
// This follows software-FM full-panning / voice-spread practice: synthesize the
// voice once, then apply constant-power gains at the mixer. No delay, phase
// inversion, detune, pseudo-stereo timing, or semantic classifier is introduced.
constexpr float genesis_source_spread_front_gain =
    vgmtooling::model::surround_equal_power_split;
constexpr float genesis_source_spread_surround_gain =
    vgmtooling::model::surround_equal_power_split;

constexpr std::array<float, genesis_recomposition_source_count>
    genesis_source_spread_depth_fraction{{
        0.5000f,
        0.2500f,
        0.7500f,
        0.1250f,
        0.6250f,
        0.3750f,
        0.8750f,
        0.0625f,
        0.5625f,
        0.3125f,
        0.8125f,
    }};

struct genesis_source_spread_gains {
    float front = genesis_source_spread_front_gain;
    float side = 0.0f;
    float back = 0.0f;
};

inline genesis_source_spread_gains genesis_source_spread_gains_for(
    std::size_t source_index) noexcept
{
    if (source_index >= genesis_source_spread_depth_fraction.size())
        return {};

    const float depth = genesis_source_spread_depth_fraction[source_index];
    return {
        genesis_source_spread_front_gain,
        genesis_source_spread_surround_gain * std::sqrt(1.0f - depth),
        genesis_source_spread_surround_gain * std::sqrt(depth),
    };
}

template <
    std::size_t SourceFrames,
    std::size_t BedFrames,
    typename ReferenceSample>
bool project_genesis_source_spread_7_1(
    const genesis_selected_source_block_storage<SourceFrames>& selected,
    const ReferenceSample* reference_interleaved,
    std::size_t frame_count,
    vgmtooling::model::surround_7_1_bed_storage<BedFrames>& bed) noexcept
{
    if (!selected.valid()
        || selected.frame_count() != frame_count
        || reference_interleaved == nullptr
        || frame_count == 0
        || !bed.begin_from_interleaved_stereo(reference_interleaved, frame_count))
        return false;

    const auto& sources = selected.sources();
    for (std::size_t source_index = 0;
         source_index < genesis_recomposition_source_count;
         ++source_index) {
        if (!selected.source_present(source_index))
            continue;

        const auto& source = sources[source_index];
        const auto gains = genesis_source_spread_gains_for(source_index);
        if (!source.exact
            || !bed.redistribute_stereo_to_depth(
                source.left,
                source.right,
                frame_count,
                gains.front,
                gains.side,
                gains.back))
            return false;
    }

    return bed.valid();
}

} // namespace gameaudio::vgm
