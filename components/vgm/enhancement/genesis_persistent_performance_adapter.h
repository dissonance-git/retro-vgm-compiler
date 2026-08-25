#pragma once

#include "genesis_part_evidence.h"
#include "ym2612_performed_pitch_motion.h"
#include "../../../model/persistent_part_performance_trajectory.h"

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

} // namespace gameaudio::vgm
