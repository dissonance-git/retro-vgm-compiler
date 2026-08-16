#pragma once

#include "persistent_part_analysis_feature.h"

#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct persistent_part_subject_resolution {
    std::optional<node_id> subject_id{};
    std::vector<edge_id> bridge_edges;
    bool ambiguous = false;
};

inline bool is_physical_voice_episode(const node& value) noexcept {
    if (value.kind != node_kind::voice_instance)
        return false;

    const attribute* scope = persistent_part_attribute(value, "identity_scope");
    if (scope == nullptr)
        return true;
    const auto* text = std::get_if<std::string>(&scope->value);
    return text != nullptr && *text == "physical_voice_episode";
}

inline persistent_part_subject_resolution resolve_persistent_part_subject(
    const musical_execution_graph& graph,
    node_id observation_id) {
    const node* observation = graph.find_node(observation_id);
    if (observation == nullptr)
        throw std::invalid_argument("persistent part observation requires a known node");

    if (is_physical_voice_episode(*observation))
        return {observation_id, {}, false};

    std::set<node_id> subjects;
    std::vector<edge_id> bridges;
    for (edge_kind kind : {
             edge_kind::realizes,
             edge_kind::causes,
             edge_kind::contributes_to,
         }) {
        for (const edge* relation : graph.edges_from(observation_id, kind)) {
            const node* target = graph.find_node(relation->to);
            if (target == nullptr || !is_physical_voice_episode(*target))
                continue;
            subjects.insert(target->id);
            bridges.push_back(relation->id);
        }
    }

    if (subjects.size() != 1)
        return {std::nullopt, std::move(bridges), subjects.size() > 1};

    return {*subjects.begin(), std::move(bridges), false};
}

inline analysis_feature persistent_part_identity_from_observation(
    const musical_execution_graph& graph,
    node_id observation_id,
    std::string source) {
    const auto resolved = resolve_persistent_part_subject(graph, observation_id);
    if (!resolved.subject_id.has_value()) {
        return unresolved_feature(
            "persistent_part_identity",
            semantic_layer::musical_performance,
            feature_availability::unknown,
            resolved.ambiguous
                ? "observation maps to multiple physical voice episodes, so persistent-part identity remains ambiguous"
                : "observation has no unique bounded physical voice episode from which persistent-part identity can be resolved",
            std::move(source));
    }

    analysis_feature result = persistent_part_identity_feature(
        graph,
        *resolved.subject_id,
        std::move(source));

    if (observation_id != *resolved.subject_id)
        result.support_nodes.push_back(observation_id);

    for (edge_id bridge_id : resolved.bridge_edges) {
        result.support_edges.push_back(bridge_id);
        const edge* bridge = graph.find_edge(bridge_id);
        if (bridge != nullptr) {
            result.provenance.insert(
                result.provenance.end(),
                bridge->provenance.begin(),
                bridge->provenance.end());
        }
    }

    return result;
}

} // namespace vgmtooling::model
