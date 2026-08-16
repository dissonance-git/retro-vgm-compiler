#pragma once

#include "genesis_part_evidence.h"
#include "../../../model/part_motif_profile.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace gameaudio::vgm {

inline std::optional<vgmtooling::model::part_gesture_observation>
make_genesis_part_gesture_observation(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    vgmtooling::model::node_id episode_id) {
    using namespace vgmtooling::model;

    const node* part = graph.find_node(part_id);
    const node* episode = graph.find_node(episode_id);
    if (part == nullptr || part->kind != node_kind::part)
        throw std::invalid_argument("Genesis motif adapter requires a persistent-part node");
    if (episode == nullptr || episode->kind != node_kind::voice_instance)
        throw std::invalid_argument("Genesis motif adapter requires a physical voice episode");

    bool member = false;
    for (const edge* relation : graph.edges_from(episode_id, edge_kind::groups_into)) {
        if (relation->to == part_id) {
            member = true;
            break;
        }
    }
    if (!member)
        throw std::invalid_argument("Genesis episode is not a member of the requested persistent part");

    const node* onset = genesis_episode_onset_event(graph, episode_id);
    if (onset == nullptr || !onset->active.has_value())
        return std::nullopt;

    const auto relative_pitch = genesis_relative_pitch_coordinate(*onset);
    if (!relative_pitch.has_value() || !std::isfinite(*relative_pitch) || *relative_pitch <= 0.0)
        return std::nullopt;

    const auto* family_item = find_genesis_part_attribute(*onset, "device_family");
    const auto* family = family_item == nullptr
        ? nullptr
        : std::get_if<std::string>(&family_item->value);
    if (family == nullptr)
        return std::nullopt;

    std::string pitch_basis;
    if (*family == "YM2612")
        pitch_basis = "genesis_ym2612_relative_frequency_code";
    else if (*family == "SN76489")
        pitch_basis = "genesis_sn76489_reciprocal_period";
    else
        return std::nullopt;

    return part_gesture_observation{
        onset->id,
        part_id,
        onset->active->start,
        std::log2(*relative_pitch),
        std::move(pitch_basis),
        "log2_frequency_ratio_octaves",
    };
}

inline std::vector<vgmtooling::model::part_gesture_observation>
collect_genesis_part_gestures(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id) {
    using namespace vgmtooling::model;

    const node* part = graph.find_node(part_id);
    if (part == nullptr || part->kind != node_kind::part)
        throw std::invalid_argument("Genesis motif collection requires a persistent-part node");

    std::vector<part_gesture_observation> observations;
    for (const edge* membership : graph.edges_to(part_id, edge_kind::groups_into)) {
        const auto observation = make_genesis_part_gesture_observation(
            graph,
            part_id,
            membership->from);
        if (observation.has_value())
            observations.push_back(*observation);
    }

    std::sort(observations.begin(), observations.end(), [](const auto& first, const auto& second) {
        if (first.onset.domain != second.onset.domain)
            return static_cast<int>(first.onset.domain) < static_cast<int>(second.onset.domain);
        if (first.onset.tick_rate != second.onset.tick_rate)
            return first.onset.tick_rate < second.onset.tick_rate;
        if (first.onset.loop_iteration != second.onset.loop_iteration)
            return first.onset.loop_iteration < second.onset.loop_iteration;
        return first.onset.tick < second.onset.tick;
    });
    return observations;
}

inline std::optional<vgmtooling::model::part_motif_profile>
make_genesis_part_motif_profile(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id) {
    auto observations = collect_genesis_part_gestures(graph, part_id);
    if (observations.size() < 3)
        return std::nullopt;
    return vgmtooling::model::make_part_motif_profile(observations);
}

} // namespace gameaudio::vgm
