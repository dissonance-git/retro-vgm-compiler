#pragma once

#include "genesis_part_evidence.h"
#include "ym2612_performed_pitch_motion.h"
#include "../../../model/persistent_part_performance_trajectory.h"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gameaudio::vgm {

inline bool is_genesis_ym2612_device_pitch_parameter(
    const vgmtooling::model::node& value) noexcept {
    using namespace vgmtooling::model;

    if (value.kind != node_kind::parameter ||
        value.layer != semantic_layer::musical_performance)
        return false;

    const auto* kind_item = find_genesis_part_attribute(value, "parameter_kind");
    const auto* representation_item = find_genesis_part_attribute(value, "representation");
    const auto* family_item = find_genesis_part_attribute(value, "device_family");
    const auto* kind = kind_item == nullptr
        ? nullptr : std::get_if<std::string>(&kind_item->value);
    const auto* representation = representation_item == nullptr
        ? nullptr : std::get_if<std::string>(&representation_item->value);
    const auto* family = family_item == nullptr
        ? nullptr : std::get_if<std::string>(&family_item->value);

    return kind != nullptr && *kind == "pitch" &&
           representation != nullptr && *representation == "device_native" &&
           family != nullptr && *family == "YM2612";
}

inline std::optional<vgmtooling::model::node_id>
unique_genesis_ym2612_pitch_parameter_for_episode(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id episode_id) noexcept {
    using namespace vgmtooling::model;

    std::optional<node_id> result;
    for (const edge* relation : graph.edges_to(episode_id, edge_kind::controls)) {
        const node* parameter = graph.find_node(relation->from);
        if (parameter == nullptr || !is_genesis_ym2612_device_pitch_parameter(*parameter))
            continue;
        if (result.has_value())
            return std::nullopt;
        result = parameter->id;
    }
    return result;
}

inline std::optional<vgmtooling::model::persistent_part_performance_trajectory>
project_genesis_ym2612_persistent_performance(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::persistent_part_trajectory identity,
    const genesis_pitch_clock_context& clocks,
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    using namespace vgmtooling::model;

    if (identity.subject_nodes.size() < 2 || identity.transitions.empty())
        throw std::invalid_argument("Genesis persistent performance requires a multi-episode identity trajectory");

    std::vector<persistent_part_performance_segment> segments;
    segments.reserve(identity.subject_nodes.size());
    for (const node_id episode_id : identity.subject_nodes) {
        const auto pitch_parameter =
            unique_genesis_ym2612_pitch_parameter_for_episode(graph, episode_id);
        if (!pitch_parameter.has_value())
            return std::nullopt;

        const auto projection = project_ym2612_performed_pitch_motion(
            graph,
            *pitch_parameter,
            clocks);
        if (!projection.static_operator_network_grounded || projection.samples.empty())
            return std::nullopt;

        segments.push_back(make_persistent_part_performance_segment(
            projection.samples,
            projection.confidence,
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
infer_genesis_ym2612_persistent_performance(
    const vgmtooling::model::musical_execution_graph& graph,
    const std::vector<vgmtooling::model::node_id>& episode_ids,
    const genesis_pitch_clock_context& clocks,
    std::string source,
    genesis_part_continuity_policy continuity_policy,
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    using namespace vgmtooling::model;

    if (episode_ids.size() < 2)
        throw std::invalid_argument("Genesis persistent performance inference requires at least two episodes");
    if (source.empty())
        throw std::invalid_argument("Genesis persistent performance inference requires a source label");

    std::vector<persistent_part_hypothesis> transitions;
    transitions.reserve(episode_ids.size() - 1);
    for (std::size_t index = 0; index + 1 < episode_ids.size(); ++index) {
        auto transition = infer_genesis_persistent_part(
            graph,
            episode_ids[index],
            episode_ids[index + 1],
            source,
            continuity_policy);
        if (!strong_persistent_part_transition(transition))
            return std::nullopt;
        transitions.push_back(std::move(transition));
    }

    auto identity = make_persistent_part_trajectory(std::move(transitions));
    if (identity.subject_nodes != episode_ids)
        throw std::logic_error("Genesis persistent-part inference changed requested episode order");

    return project_genesis_ym2612_persistent_performance(
        graph,
        std::move(identity),
        clocks,
        motion_policy);
}

inline bool is_strong_genesis_persistent_part_node(
    const vgmtooling::model::node& value) noexcept {
    using namespace vgmtooling::model;

    if (value.kind != node_kind::part ||
        value.layer != semantic_layer::musical_performance)
        return false;
    const auto* scope_item = find_genesis_part_attribute(value, "identity_scope");
    const auto* scope = scope_item == nullptr
        ? nullptr : std::get_if<std::string>(&scope_item->value);
    return scope != nullptr && *scope == "persistent_musical_part" &&
           scope_item->confidence >= persistent_part_trajectory_link_threshold;
}

inline std::vector<vgmtooling::model::node_id>
ordered_genesis_persistent_part_episodes(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id) {
    using namespace vgmtooling::model;

    const node* part = graph.find_node(part_id);
    if (part == nullptr)
        throw std::invalid_argument("Genesis persistent performance references an unknown part");
    if (!is_strong_genesis_persistent_part_node(*part))
        return {};

    std::vector<const node*> episodes;
    for (const edge* membership : graph.edges_to(part_id, edge_kind::groups_into)) {
        const node* episode = graph.find_node(membership->from);
        if (episode == nullptr || episode->kind != node_kind::voice_instance ||
            !episode->active.has_value())
            return {};
        episodes.push_back(episode);
    }
    if (episodes.size() < 2)
        return {};

    const auto& first_time = episodes.front()->active->start;
    for (const node* episode : episodes) {
        const auto& start = episode->active->start;
        if (start.domain != first_time.domain ||
            start.tick_rate != first_time.tick_rate ||
            start.loop_iteration != first_time.loop_iteration)
            return {};
        if (episode->active->end.has_value()) {
            const auto& end = *episode->active->end;
            if (end.domain != first_time.domain ||
                end.tick_rate != first_time.tick_rate ||
                end.loop_iteration != first_time.loop_iteration)
                return {};
        }
    }

    std::sort(episodes.begin(), episodes.end(), [](const node* first, const node* second) {
        if (first->active->start.tick != second->active->start.tick)
            return first->active->start.tick < second->active->start.tick;
        return first->id < second->id;
    });

    std::vector<node_id> result;
    result.reserve(episodes.size());
    for (const node* episode : episodes)
        result.push_back(episode->id);
    return result;
}

inline std::optional<vgmtooling::model::persistent_part_performance_trajectory>
project_genesis_ym2612_part_performance(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const genesis_pitch_clock_context& clocks,
    std::string source,
    genesis_part_continuity_policy continuity_policy,
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    using namespace vgmtooling::model;

    const node* part = graph.find_node(part_id);
    if (part == nullptr)
        throw std::invalid_argument("Genesis persistent performance references an unknown part");
    if (!is_strong_genesis_persistent_part_node(*part))
        return std::nullopt;

    const auto episode_ids = ordered_genesis_persistent_part_episodes(graph, part_id);
    if (episode_ids.size() < 2)
        return std::nullopt;

    auto performance = infer_genesis_ym2612_persistent_performance(
        graph,
        episode_ids,
        clocks,
        std::move(source),
        continuity_policy,
        motion_policy);
    if (!performance.has_value())
        return std::nullopt;

    const auto* scope_item = find_genesis_part_attribute(*part, "identity_scope");
    if (scope_item == nullptr)
        throw std::logic_error("strong Genesis persistent part lost its identity scope");
    performance->identity.confidence =
        std::min(performance->identity.confidence, scope_item->confidence);
    performance->confidence =
        std::min(performance->confidence, scope_item->confidence);
    return performance;
}

inline std::vector<vgmtooling::model::persistent_part_performance_trajectory>
discover_genesis_ym2612_persistent_performances(
    const vgmtooling::model::musical_execution_graph& graph,
    const genesis_pitch_clock_context& clocks,
    const std::string& source,
    genesis_part_continuity_policy continuity_policy,
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    using namespace vgmtooling::model;

    if (source.empty())
        throw std::invalid_argument("Genesis persistent performance discovery requires a source label");

    std::vector<persistent_part_performance_trajectory> result;
    for (const auto& candidate : graph.nodes()) {
        if (!is_strong_genesis_persistent_part_node(candidate))
            continue;
        auto performance = project_genesis_ym2612_part_performance(
            graph,
            candidate.id,
            clocks,
            source,
            continuity_policy,
            motion_policy);
        if (performance.has_value())
            result.push_back(std::move(*performance));
    }
    return result;
}

} // namespace gameaudio::vgm
