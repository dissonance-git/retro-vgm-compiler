#pragma once

#include "spc_persistent_performance_adapter.h"
#include "../../model/analysis_pitch_bridge.h"
#include "../../model/sample_root_tuning_evidence.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace gameaudio::spc {

constexpr std::uint64_t spc_sdsp_unity_pitch_rate = 0x1000u;

inline std::optional<vgmtooling::model::absolute_pitch_state_sample>
spc_episode_absolute_pitch_state(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id episode_id,
    const vgmtooling::model::sample_root_tuning_evidence& tuning) {
    using namespace vgmtooling::model;

    const node* sample = graph.find_node(tuning.sample_version_id);
    if (sample == nullptr || sample->kind != node_kind::sample_buffer)
        throw std::invalid_argument("SPC absolute pitch tuning references an invalid sample version");
    if (!usable_sample_root_tuning(tuning))
        return std::nullopt;

    const auto relative = spc_episode_relative_pitch_observation(graph, episode_id);
    if (!relative.has_value())
        return std::nullopt;
    if (relative->sample_version_id != tuning.sample_version_id)
        return std::nullopt;

    const node* pitch_event = graph.find_node(relative->sample.source_node);
    if (pitch_event == nullptr)
        throw std::logic_error("SPC relative pitch observation lost its source event");
    const attribute* pitch_item = find_spc_performance_attribute(*pitch_event, "pitch_rate");
    const auto* pitch_rate = pitch_item == nullptr
        ? nullptr : std::get_if<std::uint64_t>(&pitch_item->value);
    if (pitch_rate == nullptr || *pitch_rate == 0)
        throw std::logic_error("SPC relative pitch observation lost its validated pitch rate");

    const double frequency_hz = tuning.unity_playback_fundamental_hz *
        static_cast<double>(*pitch_rate) /
        static_cast<double>(spc_sdsp_unity_pitch_rate);
    if (!std::isfinite(frequency_hz) || frequency_hz <= 0.0)
        throw std::logic_error("SPC sample tuning and pitch rate produced invalid performed Hz");

    absolute_pitch_state_sample result;
    result.source_node = pitch_event->id;
    result.physical_episode_id = episode_id;
    result.time = relative->sample.time;
    result.frequency_hz = frequency_hz;
    result.status = weakest_evidence_status(tuning.status, pitch_item->status);
    result.confidence = std::min(tuning.confidence, relative->confidence);
    return result;
}

inline std::vector<vgmtooling::model::absolute_musical_pitch_observation>
spc_episode_absolute_pitch_observations(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id episode_id,
    const vgmtooling::model::sample_root_tuning_evidence& tuning,
    const std::string& source) {
    using namespace vgmtooling::model;

    if (source.empty())
        throw std::invalid_argument("SPC absolute pitch bridge requires provenance source");
    const node* episode = graph.find_node(episode_id);
    if (episode == nullptr || episode->kind != node_kind::voice_instance ||
        !episode->active.has_value()) {
        throw std::invalid_argument("SPC absolute pitch bridge requires a bounded physical episode");
    }

    const auto state = spc_episode_absolute_pitch_state(graph, episode_id, tuning);
    if (!state.has_value())
        return {};

    return absolute_musical_pitch_trajectory_from_states(
        graph,
        {*state},
        episode->active->end,
        musical_pitch_role::performed,
        source + ":sample-root-tuning+sdsp-pitch-rate");
}

inline std::vector<vgmtooling::model::absolute_musical_pitch_observation>
collect_spc_absolute_performed_pitch_observations(
    const vgmtooling::model::musical_execution_graph& graph,
    const std::vector<vgmtooling::model::sample_root_tuning_evidence>& tunings,
    const std::string& source) {
    using namespace vgmtooling::model;

    if (source.empty())
        throw std::invalid_argument("SPC absolute pitch collection requires provenance source");

    std::unordered_map<node_id, const sample_root_tuning_evidence*> by_sample;
    for (const auto& tuning : tunings) {
        const node* sample = graph.find_node(tuning.sample_version_id);
        if (sample == nullptr || sample->kind != node_kind::sample_buffer)
            throw std::invalid_argument("SPC absolute pitch collection contains unknown sample tuning");
        if (!by_sample.emplace(tuning.sample_version_id, &tuning).second)
            throw std::invalid_argument("SPC absolute pitch collection contains competing tuning claims for one sample version");
    }

    std::vector<absolute_musical_pitch_observation> result;
    for (const auto& candidate : graph.nodes()) {
        if (candidate.kind != node_kind::voice_instance || !candidate.active.has_value())
            continue;
        const auto relative = spc_episode_relative_pitch_observation(graph, candidate.id);
        if (!relative.has_value())
            continue;
        const auto found = by_sample.find(relative->sample_version_id);
        if (found == by_sample.end())
            continue;

        const auto projected = spc_episode_absolute_pitch_observations(
            graph,
            candidate.id,
            *found->second,
            source);
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

} // namespace gameaudio::spc
