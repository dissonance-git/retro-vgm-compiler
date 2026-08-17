#pragma once

#include "genesis_analysis_features.h"
#include "genesis_part_evidence.h"
#include "ym2612_episode_pitch_analysis.h"
#include "../../../model/part_motif_discovery.h"
#include "../../../model/part_motif_profile.h"
#include "../../../model/part_phrase_boundary_discovery.h"

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
    const auto part_evidence = read_persistent_part_motif_evidence(*part);

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
        weaker_part_motif_evidence_status(evidence_status::derived, part_evidence.status),
        part_evidence.confidence,
    };
}

inline std::optional<vgmtooling::model::part_gesture_observation>
make_genesis_performed_part_gesture_observation(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    vgmtooling::model::node_id episode_id,
    const genesis_pitch_clock_context& clocks) {
    using namespace vgmtooling::model;

    const node* part = graph.find_node(part_id);
    const node* episode = graph.find_node(episode_id);
    if (part == nullptr || part->kind != node_kind::part)
        throw std::invalid_argument("Genesis performed motif adapter requires a persistent-part node");
    if (episode == nullptr || episode->kind != node_kind::voice_instance)
        throw std::invalid_argument("Genesis performed motif adapter requires a physical voice episode");
    const auto part_evidence = read_persistent_part_motif_evidence(*part);

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

    const auto* family_item = find_genesis_part_attribute(*onset, "device_family");
    const auto* family = family_item == nullptr
        ? nullptr
        : std::get_if<std::string>(&family_item->value);
    if (family == nullptr)
        return std::nullopt;

    analysis_feature_set features;
    if (*family == "YM2612") {
        features = extract_ym2612_episode_pitch_features(
            graph,
            episode_id,
            clocks);
    } else if (*family == "SN76489") {
        features = extract_genesis_performance_analysis_features(
            graph,
            onset->id,
            &clocks);
    } else {
        return std::nullopt;
    }

    const analysis_feature* performed = features.find("performed_pitch_frequency_hz");
    if (performed == nullptr ||
        performed->availability != feature_availability::present) {
        return std::nullopt;
    }
    if (!performed->value.has_value() || !performed->status.has_value() ||
        !performed->confidence.has_value()) {
        throw std::logic_error("present Genesis performed-pitch feature lacks evidence fields");
    }
    const auto* frequency = std::get_if<double>(&*performed->value);
    if (frequency == nullptr || !std::isfinite(*frequency) || *frequency <= 0.0)
        throw std::logic_error("present Genesis performed-pitch feature lacks a valid frequency");

    return part_gesture_observation{
        onset->id,
        part_id,
        onset->active->start,
        std::log2(*frequency),
        "absolute_performed_frequency_hz",
        "log2_frequency_ratio_octaves",
        weaker_part_motif_evidence_status(*performed->status, part_evidence.status),
        std::min(*performed->confidence, part_evidence.confidence),
    };
}

inline void sort_genesis_part_gestures(
    std::vector<vgmtooling::model::part_gesture_observation>& observations) {
    std::sort(observations.begin(), observations.end(), [](const auto& first, const auto& second) {
        if (first.onset.domain != second.onset.domain)
            return static_cast<int>(first.onset.domain) < static_cast<int>(second.onset.domain);
        if (first.onset.tick_rate != second.onset.tick_rate)
            return first.onset.tick_rate < second.onset.tick_rate;
        if (first.onset.loop_iteration != second.onset.loop_iteration)
            return first.onset.loop_iteration < second.onset.loop_iteration;
        return first.onset.tick < second.onset.tick;
    });
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

    sort_genesis_part_gestures(observations);
    return observations;
}

// Prefer one absolute performed-frequency basis only when every physical episode
// grouped into this already-grounded persistent part can support it. YM2612 and
// SN76489 tone episodes may therefore coexist in one performed profile once part
// continuity has independently been earned. If any episode cannot support the
// common coordinate, fall back wholesale to the native-relative projection.
// This keeps fallback a part-level choice and never mixes incompatible pitch
// semantics event by event.
inline std::vector<vgmtooling::model::part_gesture_observation>
collect_genesis_part_gestures(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const genesis_pitch_clock_context& clocks) {
    using namespace vgmtooling::model;

    const auto native = collect_genesis_part_gestures(graph, part_id);
    const node* part = graph.find_node(part_id);
    if (part == nullptr || part->kind != node_kind::part)
        throw std::invalid_argument("Genesis performed motif collection requires a persistent-part node");

    std::vector<part_gesture_observation> performed;
    std::size_t membership_count = 0;
    for (const edge* membership : graph.edges_to(part_id, edge_kind::groups_into)) {
        ++membership_count;
        const auto observation = make_genesis_performed_part_gesture_observation(
            graph,
            part_id,
            membership->from,
            clocks);
        if (!observation.has_value())
            return native;
        performed.push_back(*observation);
    }

    if (membership_count == 0 || performed.size() != membership_count)
        return native;

    sort_genesis_part_gestures(performed);
    return performed;
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

inline std::optional<vgmtooling::model::part_motif_profile>
make_genesis_part_motif_profile(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const genesis_pitch_clock_context& clocks) {
    auto observations = collect_genesis_part_gestures(graph, part_id, clocks);
    if (observations.size() < 3)
        return std::nullopt;
    return vgmtooling::model::make_part_motif_profile(observations);
}

inline std::vector<vgmtooling::model::repeated_part_motif_hypothesis>
discover_genesis_part_motifs(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const vgmtooling::model::part_motif_discovery_policy& policy = {}) {
    return vgmtooling::model::discover_repeated_part_motifs(
        collect_genesis_part_gestures(graph, part_id),
        policy);
}

inline std::vector<vgmtooling::model::repeated_part_motif_hypothesis>
discover_genesis_part_motifs(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const genesis_pitch_clock_context& clocks,
    const vgmtooling::model::part_motif_discovery_policy& policy = {}) {
    return vgmtooling::model::discover_repeated_part_motifs(
        collect_genesis_part_gestures(graph, part_id, clocks),
        policy);
}

inline std::vector<vgmtooling::model::phrase_boundary_hypothesis>
discover_genesis_part_phrase_boundaries(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const vgmtooling::model::part_motif_discovery_policy& motif_policy = {},
    double minimum_gap_ratio = 2.0) {
    return vgmtooling::model::discover_part_phrase_boundaries(
        collect_genesis_part_gestures(graph, part_id),
        motif_policy,
        minimum_gap_ratio,
        0.95,
        "genesis-part-phrase-discovery");
}

inline std::vector<vgmtooling::model::phrase_boundary_hypothesis>
discover_genesis_part_phrase_boundaries(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const genesis_pitch_clock_context& clocks,
    const vgmtooling::model::part_motif_discovery_policy& motif_policy = {},
    double minimum_gap_ratio = 2.0) {
    return vgmtooling::model::discover_part_phrase_boundaries(
        collect_genesis_part_gestures(graph, part_id, clocks),
        motif_policy,
        minimum_gap_ratio,
        0.95,
        "genesis-performed-part-phrase-discovery");
}

} // namespace gameaudio::vgm
