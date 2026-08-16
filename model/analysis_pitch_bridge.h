#pragma once

#include "analysis_feature.h"
#include "harmonic_verticality.h"
#include "persistent_part_analysis_feature.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

inline evidence_status weakest_evidence_status(
    evidence_status first,
    evidence_status second) noexcept {
    return static_cast<evidence_status>(std::max(
        static_cast<std::uint8_t>(first),
        static_cast<std::uint8_t>(second)));
}

inline std::optional<absolute_musical_pitch_observation>
absolute_musical_pitch_from_analysis_features(
    const musical_execution_graph& graph,
    node_id observation_id,
    const analysis_feature_set& features,
    const std::string& pitch_feature_name,
    musical_pitch_role role,
    std::string source) {
    if (observation_id == 0 || graph.find_node(observation_id) == nullptr)
        throw std::invalid_argument("absolute pitch bridge requires a known observation node");
    if (source.empty())
        throw std::invalid_argument("absolute pitch bridge requires provenance source");

    const analysis_feature* pitch = features.find(pitch_feature_name);
    const analysis_feature* part = features.find("persistent_part_identity");
    const analysis_feature* episode = features.find("physical_voice_episode_id");
    if (pitch == nullptr || part == nullptr || episode == nullptr)
        throw std::invalid_argument("absolute pitch bridge requires pitch, part, and physical-episode features");

    if (pitch->availability != feature_availability::present ||
        part->availability != feature_availability::present ||
        episode->availability != feature_availability::present) {
        return std::nullopt;
    }
    if (!pitch->value.has_value() || !part->value.has_value() || !episode->value.has_value() ||
        !pitch->status.has_value() || !part->status.has_value() ||
        !pitch->confidence.has_value() || !part->confidence.has_value()) {
        throw std::logic_error("present pitch/part features are missing validated evidence fields");
    }

    const auto* frequency = std::get_if<double>(&*pitch->value);
    const auto* part_id = std::get_if<std::uint64_t>(&*part->value);
    const auto* episode_id = std::get_if<std::uint64_t>(&*episode->value);
    if (frequency == nullptr || part_id == nullptr || episode_id == nullptr)
        throw std::invalid_argument("absolute pitch bridge received incompatible feature value types");
    if (*part_id == 0 || *episode_id == 0)
        return std::nullopt;

    const node* part_node = graph.find_node(static_cast<node_id>(*part_id));
    const node* episode_node = graph.find_node(static_cast<node_id>(*episode_id));
    if (part_node == nullptr || part_node->kind != node_kind::part ||
        part_node->layer != semantic_layer::musical_performance ||
        episode_node == nullptr || episode_node->kind != node_kind::voice_instance ||
        !episode_node->active.has_value()) {
        return std::nullopt;
    }

    absolute_musical_pitch_observation result;
    result.source_node = observation_id;
    result.part_id = static_cast<node_id>(*part_id);
    result.active = *episode_node->active;
    result.frequency_hz = *frequency;
    result.role = role;
    result.status = weakest_evidence_status(*pitch->status, *part->status);
    result.confidence = std::min(*pitch->confidence, *part->confidence);
    result.source = std::move(source);
    validate_absolute_musical_pitch_observation(result);
    return result;
}

// Format-neutral absolute pitch state before persistent musical identity is
// admitted. A frontend may emit these only when it has earned an absolute Hz
// interpretation at a particular time. The state still belongs to a bounded
// physical performance episode; it is not yet a musical part or note.
struct absolute_pitch_state_sample {
    node_id source_node = 0;
    node_id physical_episode_id = 0;
    time_coordinate time{};
    double frequency_hz = 0.0;
    evidence_status status = evidence_status::hypothesis;
    double confidence = 0.0;
};

inline bool compatible_pitch_state_time(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
           first.tick_rate == second.tick_rate &&
           first.loop_iteration == second.loop_iteration;
}

inline std::vector<absolute_musical_pitch_observation>
absolute_musical_pitch_trajectory_from_states(
    const musical_execution_graph& graph,
    const std::vector<absolute_pitch_state_sample>& states,
    std::optional<time_coordinate> interpretation_end,
    musical_pitch_role role,
    std::string source) {
    if (states.empty())
        return {};
    if (source.empty())
        throw std::invalid_argument("absolute pitch trajectory bridge requires provenance source");

    const node_id episode_id = states.front().physical_episode_id;
    if (episode_id == 0)
        throw std::invalid_argument("absolute pitch trajectory requires a bounded physical episode");
    const node* episode = graph.find_node(episode_id);
    if (episode == nullptr || episode->kind != node_kind::voice_instance ||
        !episode->active.has_value()) {
        throw std::invalid_argument("absolute pitch trajectory references an invalid physical episode");
    }

    const auto part_feature = persistent_part_identity_feature(
        graph,
        episode_id,
        source + ":persistent-part");
    if (part_feature.availability != feature_availability::present)
        return {};
    if (!part_feature.value.has_value() || !part_feature.status.has_value() ||
        !part_feature.confidence.has_value()) {
        throw std::logic_error("present persistent-part feature is missing validated evidence fields");
    }
    const auto* part_value = std::get_if<std::uint64_t>(&*part_feature.value);
    if (part_value == nullptr || *part_value == 0)
        throw std::logic_error("persistent-part feature did not contain a part node id");
    const node_id part_id = static_cast<node_id>(*part_value);
    const node* part = graph.find_node(part_id);
    if (part == nullptr || part->kind != node_kind::part ||
        part->layer != semantic_layer::musical_performance) {
        throw std::logic_error("persistent-part feature references an invalid part node");
    }

    const auto& episode_start = episode->active->start;
    std::optional<time_coordinate> final_end = episode->active->end;
    if (interpretation_end.has_value()) {
        if (!compatible_pitch_state_time(*interpretation_end, episode_start))
            throw std::invalid_argument("pitch-trajectory interpretation end uses an incompatible time coordinate");
        if (interpretation_end->tick < episode_start.tick)
            throw std::invalid_argument("pitch-trajectory interpretation ends before the physical episode starts");
        if (!final_end.has_value() || interpretation_end->tick < final_end->tick)
            final_end = interpretation_end;
    }

    for (std::size_t index = 0; index < states.size(); ++index) {
        const auto& state = states[index];
        if (state.source_node == 0 || graph.find_node(state.source_node) == nullptr)
            throw std::invalid_argument("absolute pitch state requires a known source node");
        if (state.physical_episode_id != episode_id)
            throw std::invalid_argument("one absolute pitch trajectory cannot merge physical episodes");
        if (!compatible_pitch_state_time(state.time, episode_start))
            throw std::invalid_argument("absolute pitch state changed time coordinate semantics");
        if (state.time.tick < episode_start.tick)
            throw std::invalid_argument("absolute pitch state precedes its physical episode");
        if (!std::isfinite(state.frequency_hz) || state.frequency_hz <= 0.0)
            throw std::invalid_argument("absolute pitch state frequency must be finite and positive");
        if (state.confidence < 0.0 || state.confidence > 1.0)
            throw std::invalid_argument("absolute pitch state confidence must be in [0, 1]");
        if (index != 0) {
            if (!compatible_pitch_state_time(state.time, states[index - 1].time) ||
                state.time.tick <= states[index - 1].time.tick) {
                throw std::invalid_argument("absolute pitch states require strictly increasing compatible times");
            }
        }
        if (final_end.has_value() && state.time.tick >= final_end->tick)
            throw std::invalid_argument("absolute pitch state lies at or beyond the interpretation boundary");
    }

    std::vector<absolute_musical_pitch_observation> result;
    result.reserve(states.size());
    for (std::size_t index = 0; index < states.size(); ++index) {
        std::optional<time_coordinate> state_end;
        if (index + 1 < states.size())
            state_end = states[index + 1].time;
        else
            state_end = final_end;

        absolute_musical_pitch_observation observation;
        observation.source_node = states[index].source_node;
        observation.part_id = part_id;
        observation.active = time_span{states[index].time, state_end};
        observation.frequency_hz = states[index].frequency_hz;
        observation.role = role;
        observation.status = weakest_evidence_status(
            states[index].status,
            *part_feature.status);
        observation.confidence = std::min(
            states[index].confidence,
            *part_feature.confidence);
        observation.source = source;
        validate_absolute_musical_pitch_observation(observation);
        result.push_back(std::move(observation));
    }

    return result;
}

} // namespace vgmtooling::model
