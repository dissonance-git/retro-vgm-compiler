#pragma once

#include "spc_part_evidence.h"
#include "../../model/persistent_part_performance_trajectory.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gameaudio::spc {

struct spc_relative_pitch_observation {
    vgmtooling::model::pitch_motion_sample sample;
    vgmtooling::model::node_id sample_version_id = 0;
    double confidence = 0.0;
};

inline std::optional<spc_relative_pitch_observation>
spc_episode_relative_pitch_observation(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id episode_id) {
    using namespace vgmtooling::model;

    const node* episode = graph.find_node(episode_id);
    if (episode == nullptr || episode->kind != node_kind::voice_instance ||
        !episode->active.has_value())
        throw std::invalid_argument("SPC relative pitch requires a bounded physical voice episode");

    const auto samples = spc_episode_sample_versions(graph, episode_id);
    if (samples.size() != 1)
        return std::nullopt;
    const node_id sample_version_id = *samples.begin();

    const node* pitch_event = spc_episode_pitch_observation(graph, episode_id);
    if (pitch_event == nullptr || !pitch_event->active.has_value())
        return std::nullopt;

    const auto event_sample = spc_runtime_event_sample(graph, pitch_event->id);
    if (!event_sample.has_value() || *event_sample != sample_version_id)
        return std::nullopt;

    const attribute* pitch_item = find_spc_performance_attribute(*pitch_event, "pitch_rate");
    const auto* pitch_rate = pitch_item == nullptr
        ? nullptr : std::get_if<std::uint64_t>(&pitch_item->value);
    if (pitch_rate == nullptr || *pitch_rate == 0)
        return std::nullopt;

    const attribute* noise_item = find_spc_performance_attribute(*pitch_event, "noise_enabled");
    const auto* noise_enabled = noise_item == nullptr
        ? nullptr : std::get_if<bool>(&noise_item->value);
    if (noise_enabled != nullptr && *noise_enabled)
        return std::nullopt;

    const auto& episode_start = episode->active->start;
    const auto& event_time = pitch_event->active->start;
    if (event_time.domain != episode_start.domain ||
        event_time.tick_rate != episode_start.tick_rate ||
        event_time.loop_iteration != episode_start.loop_iteration ||
        event_time.tick < episode_start.tick)
        return std::nullopt;
    if (episode->active->end.has_value()) {
        const auto& episode_end = *episode->active->end;
        if (episode_end.domain != event_time.domain ||
            episode_end.tick_rate != event_time.tick_rate ||
            episode_end.loop_iteration != event_time.loop_iteration ||
            event_time.tick >= episode_end.tick)
            return std::nullopt;
    }

    const double coordinate = std::log2(static_cast<double>(*pitch_rate));
    if (!std::isfinite(coordinate))
        return std::nullopt;

    spc_relative_pitch_observation result;
    result.sample.source_node = pitch_event->id;
    result.sample.physical_episode_id = episode_id;
    result.sample.time = event_time;
    result.sample.log2_pitch_coordinate = coordinate;
    result.sample.pitch_basis = "snes_sdsp_pitch_rate_relative_to_event_time_brr_source";
    result.sample.interval_semantics = "log2_playback_rate_ratio_octaves";
    result.sample_version_id = sample_version_id;
    result.confidence = pitch_item->confidence;
    return result;
}

inline std::optional<vgmtooling::model::persistent_part_performance_trajectory>
project_spc_persistent_performance(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::persistent_part_trajectory identity,
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    using namespace vgmtooling::model;

    if (identity.subject_nodes.size() < 2 || identity.transitions.empty())
        throw std::invalid_argument("SPC persistent performance requires a multi-episode identity trajectory");

    std::vector<persistent_part_performance_segment> segments;
    segments.reserve(identity.subject_nodes.size());
    std::optional<node_id> common_sample_version;

    for (const node_id episode_id : identity.subject_nodes) {
        const auto observation = spc_episode_relative_pitch_observation(graph, episode_id);
        if (!observation.has_value())
            return std::nullopt;
        if (!common_sample_version.has_value())
            common_sample_version = observation->sample_version_id;
        else if (*common_sample_version != observation->sample_version_id)
            return std::nullopt;

        segments.push_back(make_persistent_part_performance_segment(
            {observation->sample},
            observation->confidence,
            motion_policy));
    }

    std::vector<pitch_motion_articulation_hypothesis> boundaries;
    boundaries.reserve(identity.transitions.size());
    for (std::size_t index = 0; index < identity.transitions.size(); ++index) {
        boundaries.push_back(make_rearticulation_boundary(
            identity.subject_nodes[index],
            identity.subject_nodes[index + 1],
            identity.transitions[index].confidence));
    }

    return make_persistent_part_performance_trajectory(
        std::move(identity),
        std::move(segments),
        std::move(boundaries));
}

inline std::optional<vgmtooling::model::persistent_part_performance_trajectory>
infer_spc_persistent_performance(
    const vgmtooling::model::musical_execution_graph& graph,
    const std::vector<vgmtooling::model::node_id>& episode_ids,
    std::string source,
    spc_part_continuity_policy continuity_policy = {},
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    using namespace vgmtooling::model;

    if (episode_ids.size() < 2)
        throw std::invalid_argument("SPC persistent performance inference requires at least two episodes");
    if (source.empty())
        throw std::invalid_argument("SPC persistent performance inference requires a source label");

    std::vector<persistent_part_hypothesis> transitions;
    transitions.reserve(episode_ids.size() - 1);
    for (std::size_t index = 0; index + 1 < episode_ids.size(); ++index) {
        const node* episode = graph.find_node(episode_ids[index]);
        if (episode == nullptr)
            throw std::invalid_argument("SPC persistent performance references an unknown episode");
        if (!spc_episode_allows_part_successor(*episode))
            return std::nullopt;

        persistent_part_hypothesis transition;
        try {
            transition = infer_spc_persistent_part(
                graph,
                episode_ids[index],
                episode_ids[index + 1],
                source,
                continuity_policy);
        } catch (const std::invalid_argument&) {
            return std::nullopt;
        }
        if (!strong_persistent_part_transition(transition))
            return std::nullopt;
        transitions.push_back(std::move(transition));
    }

    auto identity = make_persistent_part_trajectory(std::move(transitions));
    if (identity.subject_nodes != episode_ids)
        throw std::logic_error("SPC persistent-part inference changed requested episode order");

    return project_spc_persistent_performance(
        graph,
        std::move(identity),
        motion_policy);
}

inline bool is_strong_spc_persistent_part_node(
    const vgmtooling::model::node& value) noexcept {
    using namespace vgmtooling::model;

    if (value.kind != node_kind::part ||
        value.layer != semantic_layer::musical_performance)
        return false;
    const auto* scope_item = find_spc_performance_attribute(value, "identity_scope");
    const auto* scope = scope_item == nullptr
        ? nullptr : std::get_if<std::string>(&scope_item->value);
    return scope != nullptr && *scope == "persistent_musical_part" &&
           scope_item->confidence >= persistent_part_trajectory_link_threshold;
}

inline std::vector<vgmtooling::model::node_id>
ordered_spc_persistent_part_episodes(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id) {
    using namespace vgmtooling::model;

    const node* part = graph.find_node(part_id);
    if (part == nullptr)
        throw std::invalid_argument("SPC persistent performance references an unknown part");
    if (!is_strong_spc_persistent_part_node(*part))
        return {};

    std::vector<const node*> episodes;
    std::optional<std::uint64_t> physical_voice;
    std::optional<time_coordinate> basis;
    for (const edge* membership : graph.edges_to(part_id, edge_kind::groups_into)) {
        const node* episode = graph.find_node(membership->from);
        if (episode == nullptr || episode->kind != node_kind::voice_instance ||
            !episode->active.has_value())
            return {};

        const attribute* voice_item = find_spc_performance_attribute(*episode, "physical_voice");
        const auto* voice = voice_item == nullptr
            ? nullptr : std::get_if<std::uint64_t>(&voice_item->value);
        if (voice == nullptr)
            return {};
        if (!physical_voice.has_value())
            physical_voice = *voice;
        else if (*physical_voice != *voice)
            return {};

        const auto& start = episode->active->start;
        if (start.domain != time_domain::device || start.tick_rate == 0)
            return {};
        if (!basis.has_value())
            basis = start;
        else if (start.domain != basis->domain ||
                 start.tick_rate != basis->tick_rate ||
                 start.loop_iteration != basis->loop_iteration)
            return {};
        episodes.push_back(episode);
    }
    if (episodes.size() < 2)
        return {};

    std::sort(episodes.begin(), episodes.end(), [](const node* first, const node* second) {
        if (first->active->start.tick != second->active->start.tick)
            return first->active->start.tick < second->active->start.tick;
        return first->id < second->id;
    });

    for (std::size_t index = 0; index + 1 < episodes.size(); ++index) {
        if (!spc_episode_allows_part_successor(*episodes[index]))
            return {};
    }

    std::vector<node_id> result;
    result.reserve(episodes.size());
    for (const node* episode : episodes)
        result.push_back(episode->id);
    return result;
}

inline std::optional<vgmtooling::model::persistent_part_performance_trajectory>
project_spc_part_performance(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    std::string source,
    spc_part_continuity_policy continuity_policy = {},
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    using namespace vgmtooling::model;

    const node* part = graph.find_node(part_id);
    if (part == nullptr)
        throw std::invalid_argument("SPC persistent performance references an unknown part");
    if (!is_strong_spc_persistent_part_node(*part))
        return std::nullopt;

    const auto episode_ids = ordered_spc_persistent_part_episodes(graph, part_id);
    if (episode_ids.size() < 2)
        return std::nullopt;

    auto performance = infer_spc_persistent_performance(
        graph,
        episode_ids,
        std::move(source),
        continuity_policy,
        motion_policy);
    if (!performance.has_value())
        return std::nullopt;

    const auto* scope_item = find_spc_performance_attribute(*part, "identity_scope");
    if (scope_item == nullptr)
        throw std::logic_error("strong SPC persistent part lost its identity scope");
    performance->identity.confidence =
        std::min(performance->identity.confidence, scope_item->confidence);
    performance->confidence =
        std::min(performance->confidence, scope_item->confidence);
    return performance;
}

inline std::vector<vgmtooling::model::persistent_part_performance_trajectory>
discover_spc_persistent_performances(
    const vgmtooling::model::musical_execution_graph& graph,
    const std::string& source,
    spc_part_continuity_policy continuity_policy = {},
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    using namespace vgmtooling::model;

    if (source.empty())
        throw std::invalid_argument("SPC persistent performance discovery requires a source label");

    std::vector<persistent_part_performance_trajectory> result;
    for (const auto& candidate : graph.nodes()) {
        if (!is_strong_spc_persistent_part_node(candidate))
            continue;
        auto performance = project_spc_part_performance(
            graph,
            candidate.id,
            source,
            continuity_policy,
            motion_policy);
        if (performance.has_value())
            result.push_back(std::move(*performance));
    }
    return result;
}

} // namespace gameaudio::spc
