#pragma once

#include "analysis_feature.h"
#include "harmonic_verticality.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

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

} // namespace vgmtooling::model
