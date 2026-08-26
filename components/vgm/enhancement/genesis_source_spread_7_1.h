#pragma once

#include "genesis_selected_source_block.h"
#include "genesis_source_episode_7_1.h"
#include "../../../model/surround_bed_7_1.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace gameaudio::vgm {

// QSound-inspired dry + spatial-send law for exact Genesis source PCM.
//
// Every source keeps a -3 dB front anchor. The remaining half of its power is a
// spatial send distributed between side and back speakers by its current source
// episode depth. YM2612/SN76489 synthesis itself is never modified here.
constexpr float genesis_source_spread_front_gain =
    vgmtooling::model::surround_equal_power_split;
constexpr float genesis_source_spread_surround_gain =
    vgmtooling::model::surround_equal_power_split;
constexpr float genesis_source_spread_half_pi = 1.57079632679489661923f;

struct genesis_source_spread_gains {
    float front = genesis_source_spread_front_gain;
    float side = 0.0f;
    float back = 0.0f;
};

inline genesis_source_spread_gains genesis_source_spread_gains_for_depth(
    float depth) noexcept
{
    if (!std::isfinite(depth))
        depth = genesis_episode_default_depth;
    if (depth < 0.0f)
        depth = 0.0f;
    if (depth > 1.0f)
        depth = 1.0f;

    const float angle = depth * genesis_source_spread_half_pi;
    return {
        genesis_source_spread_front_gain,
        genesis_source_spread_surround_gain * std::cos(angle),
        genesis_source_spread_surround_gain * std::sin(angle),
    };
}

template <
    std::size_t SourceFrames,
    std::size_t BedFrames,
    std::size_t MaxEpisodeEvents,
    typename ReferenceSample>
bool project_genesis_source_spread_7_1(
    const genesis_selected_source_block_storage<SourceFrames>& selected,
    const genesis_source_episode_block<MaxEpisodeEvents>& episodes,
    const ReferenceSample* reference_interleaved,
    std::size_t frame_count,
    vgmtooling::model::surround_7_1_bed_storage<BedFrames>& bed) noexcept
{
    if (!selected.valid()
        || selected.frame_count() != frame_count
        || !episodes.valid
        || reference_interleaved == nullptr
        || frame_count == 0
        || !bed.begin_from_interleaved_stereo(reference_interleaved, frame_count))
        return false;

    std::array<float, genesis_recomposition_source_count> depth =
        episodes.initial_depth;
    std::size_t episode_event = 0u;
    const auto& sources = selected.sources();

    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        while (episode_event < episodes.event_count
            && episodes.events[episode_event].frame_offset == frame) {
            const auto& transition = episodes.events[episode_event];
            if (transition.source_index >= depth.size())
                return false;
            depth[transition.source_index] = transition.depth;
            ++episode_event;
        }

        for (std::size_t source_index = 0;
             source_index < genesis_recomposition_source_count;
             ++source_index) {
            if (!selected.source_present(source_index))
                continue;

            const auto& source = sources[source_index];
            if (!source.exact || source.left == nullptr || source.right == nullptr)
                return false;

            const auto gains =
                genesis_source_spread_gains_for_depth(depth[source_index]);
            if (!bed.redistribute_stereo_frame_to_depth(
                    frame,
                    source.left[frame],
                    source.right[frame],
                    gains.front,
                    gains.side,
                    gains.back))
                return false;
        }
    }

    return episode_event == episodes.event_count && bed.valid();
}

} // namespace gameaudio::vgm
