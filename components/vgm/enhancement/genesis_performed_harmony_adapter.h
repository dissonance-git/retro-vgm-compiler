#pragma once

#include "genesis_persistent_performance_adapter.h"
#include "ym2612_performed_pitch_motion.h"
#include "../../../model/harmonic_verticality_timeline.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace gameaudio::vgm {

inline std::vector<vgmtooling::model::absolute_musical_pitch_observation>
collect_genesis_ym2612_performed_pitch_observations(
    const vgmtooling::model::musical_execution_graph& graph,
    const genesis_pitch_clock_context& clocks,
    const std::string& source) {
    using namespace vgmtooling::model;

    if (source.empty())
        throw std::invalid_argument(
            "Genesis performed harmony collection requires a provenance source");

    std::vector<absolute_musical_pitch_observation> result;
    for (const auto& candidate : graph.nodes()) {
        if (!is_genesis_ym2612_device_pitch_parameter(candidate))
            continue;

        const auto projected = ym2612_performed_pitch_part_trajectory(
            graph,
            candidate.id,
            clocks,
            source + ":ym2612-performed-pitch");
        result.insert(result.end(), projected.begin(), projected.end());
    }

    std::sort(result.begin(), result.end(), [](const auto& first, const auto& second) {
        if (first.active.start.domain != second.active.start.domain)
            return first.active.start.domain < second.active.start.domain;
        if (first.active.start.tick_rate != second.active.start.tick_rate)
            return first.active.start.tick_rate < second.active.start.tick_rate;
        if (first.active.start.loop_iteration != second.active.start.loop_iteration)
            return first.active.start.loop_iteration < second.active.start.loop_iteration;
        if (first.active.start.tick != second.active.start.tick)
            return first.active.start.tick < second.active.start.tick;
        if (first.part_id != second.part_id)
            return first.part_id < second.part_id;
        return first.source_node < second.source_node;
    });
    return result;
}

inline std::vector<vgmtooling::model::harmonic_verticality>
discover_genesis_ym2612_performed_harmonic_verticalities(
    const vgmtooling::model::musical_execution_graph& graph,
    const genesis_pitch_clock_context& clocks,
    const std::string& source) {
    return vgmtooling::model::make_harmonic_verticality_timeline(
        collect_genesis_ym2612_performed_pitch_observations(
            graph,
            clocks,
            source));
}

} // namespace gameaudio::vgm
