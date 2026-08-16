#pragma once

#include "part_motif_profile.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

enum class motif_transformation_kind : std::uint8_t {
    near_recurrence = 0,
    rhythmic_variant,
    interval_variant,
    contour_preserving_variant,
    transformed_relation,
    rhythm_only_echo,
    weak_relation,
};

struct motif_transformation_hypothesis {
    motif_transformation_kind kind = motif_transformation_kind::weak_relation;
    part_motif_similarity similarity;
    double confidence = 0.0;
    std::vector<node_id> first_nodes;
    std::vector<node_id> second_nodes;
    std::string detail;
};

inline const char* to_string(motif_transformation_kind kind) noexcept {
    switch (kind) {
    case motif_transformation_kind::near_recurrence:
        return "near_recurrence";
    case motif_transformation_kind::rhythmic_variant:
        return "rhythmic_variant";
    case motif_transformation_kind::interval_variant:
        return "interval_variant";
    case motif_transformation_kind::contour_preserving_variant:
        return "contour_preserving_variant";
    case motif_transformation_kind::transformed_relation:
        return "transformed_relation";
    case motif_transformation_kind::rhythm_only_echo:
        return "rhythm_only_echo";
    case motif_transformation_kind::weak_relation:
        return "weak_relation";
    }
    return "unknown";
}

inline motif_transformation_hypothesis infer_motif_transformation(
    const part_motif_profile& first,
    const part_motif_profile& second) {
    const part_motif_similarity similarity = compare_part_motif_profiles(first, second);

    motif_transformation_hypothesis result;
    result.similarity = similarity;
    result.first_nodes = first.source_nodes;
    result.second_nodes = second.source_nodes;

    if (!similarity.pitch_comparable) {
        result.kind = similarity.rhythm_similarity >= 0.90
            ? motif_transformation_kind::rhythm_only_echo
            : motif_transformation_kind::weak_relation;
        result.confidence = similarity.identity_confidence;
    } else {
        const double interval = similarity.interval_similarity.value_or(0.0);
        const double contour = similarity.contour_similarity.value_or(0.0);
        const double rhythm = similarity.rhythm_similarity;

        if (interval >= 0.97 && contour >= 0.97 && rhythm >= 0.97) {
            result.kind = motif_transformation_kind::near_recurrence;
            result.confidence = similarity.identity_confidence;
        } else if (interval >= 0.90 && contour >= 0.90 && rhythm < 0.90) {
            result.kind = motif_transformation_kind::rhythmic_variant;
            result.confidence = similarity.combined_similarity;
        } else if (rhythm >= 0.90 && contour >= 0.90 && interval < 0.90) {
            result.kind = motif_transformation_kind::interval_variant;
            result.confidence = similarity.combined_similarity;
        } else if (contour >= 0.90 && similarity.combined_similarity >= 0.65) {
            result.kind = motif_transformation_kind::contour_preserving_variant;
            result.confidence = similarity.combined_similarity;
        } else if (similarity.combined_similarity >= 0.65) {
            result.kind = motif_transformation_kind::transformed_relation;
            result.confidence = similarity.combined_similarity;
        } else {
            result.kind = motif_transformation_kind::weak_relation;
            result.confidence = similarity.combined_similarity;
        }
    }

    result.detail =
        std::string{"kind="} + to_string(result.kind) +
        "; combined_similarity=" + std::to_string(similarity.combined_similarity) +
        "; identity_confidence=" + std::to_string(similarity.identity_confidence) +
        "; pitch_comparable=" + (similarity.pitch_comparable ? "true" : "false");
    return result;
}

inline node_id add_motif_transformation_hypothesis(
    musical_execution_graph& graph,
    const motif_transformation_hypothesis& hypothesis) {
    if (hypothesis.first_nodes.empty() || hypothesis.second_nodes.empty())
        throw std::invalid_argument("motif transformation requires source nodes for both motif occurrences");
    for (node_id id : hypothesis.first_nodes) {
        if (graph.find_node(id) == nullptr)
            throw std::invalid_argument("motif transformation references an unknown first-occurrence node");
    }
    for (node_id id : hypothesis.second_nodes) {
        if (graph.find_node(id) == nullptr)
            throw std::invalid_argument("motif transformation references an unknown second-occurrence node");
    }

    node relation;
    relation.kind = node_kind::musical_relation;
    relation.layer = semantic_layer::musical_structure;
    relation.flow = flow_kind::value;
    relation.label = "motif transformation hypothesis";
    relation.attributes.push_back({
        "identity_scope",
        std::string{"motif_transformation_hypothesis"},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    relation.attributes.push_back({
        "transformation_kind",
        std::string{to_string(hypothesis.kind)},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    relation.attributes.push_back({
        "combined_similarity",
        hypothesis.similarity.combined_similarity,
        evidence_status::derived,
        1.0,
        "ratio",
    });
    relation.attributes.push_back({
        "identity_confidence",
        hypothesis.similarity.identity_confidence,
        evidence_status::derived,
        1.0,
        "ratio",
    });
    relation.attributes.push_back({
        "pitch_comparable",
        hypothesis.similarity.pitch_comparable,
        evidence_status::derived,
        1.0,
        "",
    });
    relation.provenance.push_back({
        evidence_status::hypothesis,
        hypothesis.confidence,
        "motif-transformation-analysis",
        std::nullopt,
        hypothesis.detail,
    });
    const node_id relation_id = graph.add_node(std::move(relation));

    for (const auto& group : {std::pair<const char*, const std::vector<node_id>*>("first", &hypothesis.first_nodes),
                              std::pair<const char*, const std::vector<node_id>*>("second", &hypothesis.second_nodes)}) {
        for (node_id source_id : *group.second) {
            edge support;
            support.kind = edge_kind::derived_from;
            support.from = source_id;
            support.to = relation_id;
            support.attributes.push_back({
                "motif_occurrence",
                std::string{group.first},
                evidence_status::derived,
                1.0,
                "",
            });
            graph.add_edge(std::move(support));
        }
    }
    return relation_id;
}

} // namespace vgmtooling::model
