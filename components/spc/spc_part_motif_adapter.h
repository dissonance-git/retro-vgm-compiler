#pragma once

#include "spc_part_evidence.h"
#include "../../model/part_motif_discovery.h"
#include "../../model/part_motif_profile.h"
#include "../../model/part_phrase_boundary_discovery.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace gameaudio::spc {

inline std::optional<vgmtooling::model::part_gesture_observation>
make_spc_part_gesture_observation(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    vgmtooling::model::node_id episode_id) {
    using namespace vgmtooling::model;

    const node* part = graph.find_node(part_id);
    const node* episode = graph.find_node(episode_id);
    if (part == nullptr || part->kind != node_kind::part)
        throw std::invalid_argument("SPC motif adapter requires a persistent-part node");
    if (episode == nullptr || episode->kind != node_kind::voice_instance)
        throw std::invalid_argument("SPC motif adapter requires a physical voice episode");

    bool member = false;
    for (const edge* relation : graph.edges_from(episode_id, edge_kind::groups_into)) {
        if (relation->to == part_id) {
            member = true;
            break;
        }
    }
    if (!member)
        throw std::invalid_argument("SPC episode is not a member of the requested persistent part");

    const node* pitch_event = spc_episode_pitch_observation(graph, episode_id);
    if (pitch_event == nullptr || !pitch_event->active.has_value())
        return std::nullopt;

    const attribute* pitch_item = find_spc_performance_attribute(*pitch_event, "pitch_rate");
    const auto* pitch = pitch_item == nullptr
        ? nullptr
        : std::get_if<std::uint64_t>(&pitch_item->value);
    if (pitch == nullptr || *pitch == 0)
        return std::nullopt;

    std::string pitch_basis;
    const auto samples = spc_episode_sample_versions(graph, episode_id);
    if (samples.size() == 1)
        pitch_basis = "spc_brr_runtime_version:" + std::to_string(*samples.begin());

    return part_gesture_observation{
        pitch_event->id,
        part_id,
        pitch_event->active->start,
        std::log2(static_cast<double>(*pitch)),
        std::move(pitch_basis),
        "log2_frequency_ratio_octaves",
    };
}

inline std::vector<vgmtooling::model::part_gesture_observation>
collect_spc_part_gestures(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id) {
    using namespace vgmtooling::model;

    const node* part = graph.find_node(part_id);
    if (part == nullptr || part->kind != node_kind::part)
        throw std::invalid_argument("SPC motif collection requires a persistent-part node");

    std::vector<part_gesture_observation> observations;
    for (const edge* membership : graph.edges_to(part_id, edge_kind::groups_into)) {
        const auto observation = make_spc_part_gesture_observation(
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
make_spc_part_motif_profile(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id) {
    auto observations = collect_spc_part_gestures(graph, part_id);
    if (observations.size() < 3)
        return std::nullopt;
    return vgmtooling::model::make_part_motif_profile(observations);
}

inline std::vector<vgmtooling::model::repeated_part_motif_hypothesis>
discover_spc_part_motifs(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const vgmtooling::model::part_motif_discovery_policy& policy = {}) {
    return vgmtooling::model::discover_repeated_part_motifs(
        collect_spc_part_gestures(graph, part_id),
        policy);
}

inline std::vector<vgmtooling::model::phrase_boundary_hypothesis>
discover_spc_part_phrase_boundaries(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const vgmtooling::model::part_motif_discovery_policy& motif_policy = {},
    double minimum_gap_ratio = 2.0) {
    return vgmtooling::model::discover_part_phrase_boundaries(
        collect_spc_part_gestures(graph, part_id),
        motif_policy,
        minimum_gap_ratio,
        0.95,
        "spc-part-phrase-discovery");
}

} // namespace gameaudio::spc
