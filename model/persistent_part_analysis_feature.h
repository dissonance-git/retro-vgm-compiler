#pragma once

#include "analysis_feature.h"

#include <cstdint>
#include <string>
#include <vector>

namespace vgmtooling::model {

constexpr double persistent_part_feature_threshold = 0.75;

inline const attribute* persistent_part_attribute(
    const node& value,
    const char* key) noexcept {
    for (const auto& item : value.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

inline analysis_feature persistent_part_identity_feature(
    const musical_execution_graph& graph,
    node_id subject_id,
    std::string source) {
    const node* subject = graph.find_node(subject_id);
    if (subject == nullptr)
        throw std::invalid_argument("persistent part feature requires a known subject node");

    struct candidate {
        node_id part_id;
        edge_id membership_edge_id;
        evidence_status status;
        double confidence;
    };

    std::vector<candidate> strong;
    bool has_weak_candidate = false;

    for (const edge* relation : graph.edges_from(subject_id, edge_kind::groups_into)) {
        const node* part = graph.find_node(relation->to);
        if (part == nullptr || part->kind != node_kind::part ||
            part->layer != semantic_layer::musical_performance)
            continue;

        const attribute* scope = persistent_part_attribute(*part, "identity_scope");
        const auto* scope_value = scope == nullptr
            ? nullptr
            : std::get_if<std::string>(&scope->value);
        if (scope_value == nullptr || *scope_value != "persistent_musical_part")
            continue;

        if (scope->confidence >= persistent_part_feature_threshold) {
            strong.push_back({part->id, relation->id, scope->status, scope->confidence});
        } else {
            has_weak_candidate = true;
        }
    }

    if (strong.size() != 1) {
        std::string detail;
        if (strong.size() > 1) {
            detail = "multiple strong persistent-part hypotheses remain";
        } else if (has_weak_candidate) {
            detail = "persistent-part hypotheses exist but remain below the strong threshold";
        } else {
            detail = "no persistent-part hypothesis groups this subject";
        }
        return unresolved_feature(
            "persistent_part_identity",
            semantic_layer::musical_performance,
            feature_availability::unknown,
            std::move(detail),
            std::move(source));
    }

    const candidate selected = strong.front();
    const node* part = graph.find_node(selected.part_id);
    const edge* membership = graph.find_edge(selected.membership_edge_id);

    analysis_feature feature = present_feature(
        "persistent_part_identity",
        semantic_layer::musical_performance,
        attribute_value{static_cast<std::uint64_t>(selected.part_id)},
        selected.status,
        selected.confidence,
        "node_id");
    feature.support_nodes.push_back(subject_id);
    feature.support_nodes.push_back(selected.part_id);
    feature.support_edges.push_back(selected.membership_edge_id);
    if (part != nullptr)
        feature.provenance = part->provenance;
    if (membership != nullptr) {
        feature.provenance.insert(
            feature.provenance.end(),
            membership->provenance.begin(),
            membership->provenance.end());
    }
    return feature;
}

} // namespace vgmtooling::model
