#pragma once

#include "phrase_boundary_consensus.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct phrase_region_hypothesis {
    time_span span{};
    double confidence = 0.0;
    double start_boundary_confidence = 0.0;
    double end_boundary_confidence = 0.0;
    bool cross_part_grounded = false;
    std::vector<node_id> supporting_parts;
};

inline phrase_region_hypothesis make_phrase_region_hypothesis(
    const phrase_boundary_consensus& start,
    const phrase_boundary_consensus& end) {
    if (!compatible_phrase_boundary_time_basis(start.representative, end.representative))
        throw std::invalid_argument("phrase region requires boundaries in one compatible time basis");
    if (end.representative.tick <= start.representative.tick)
        throw std::invalid_argument("phrase region end must follow its start boundary");

    phrase_region_hypothesis result;
    result.span = time_span{start.representative, end.representative};
    result.start_boundary_confidence = start.confidence;
    result.end_boundary_confidence = end.confidence;
    result.confidence = std::min(start.confidence, end.confidence);
    result.cross_part_grounded = start.cross_part_grounded && end.cross_part_grounded;

    result.supporting_parts = start.supporting_parts;
    result.supporting_parts.insert(
        result.supporting_parts.end(),
        end.supporting_parts.begin(),
        end.supporting_parts.end());
    std::sort(result.supporting_parts.begin(), result.supporting_parts.end());
    result.supporting_parts.erase(
        std::unique(result.supporting_parts.begin(), result.supporting_parts.end()),
        result.supporting_parts.end());
    return result;
}

inline node_id add_phrase_region_hypothesis(
    musical_execution_graph& graph,
    const phrase_region_hypothesis& phrase) {
    for (node_id part_id : phrase.supporting_parts) {
        const node* part = graph.find_node(part_id);
        if (part == nullptr || part->kind != node_kind::part)
            throw std::invalid_argument("phrase region references an unknown persistent part");
    }

    node region;
    region.kind = node_kind::section;
    region.layer = semantic_layer::musical_structure;
    region.flow = flow_kind::stream;
    region.label = "phrase region hypothesis";
    region.active = phrase.span;
    region.attributes.push_back({
        "identity_scope",
        std::string{"phrase_region_hypothesis"},
        evidence_status::hypothesis,
        phrase.confidence,
        "",
    });
    region.attributes.push_back({
        "start_boundary_confidence",
        phrase.start_boundary_confidence,
        evidence_status::derived,
        1.0,
        "ratio",
    });
    region.attributes.push_back({
        "end_boundary_confidence",
        phrase.end_boundary_confidence,
        evidence_status::derived,
        1.0,
        "ratio",
    });
    region.attributes.push_back({
        "cross_part_grounded",
        phrase.cross_part_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    const node_id phrase_id = graph.add_node(std::move(region));

    for (node_id part_id : phrase.supporting_parts) {
        edge support;
        support.kind = edge_kind::derived_from;
        support.from = part_id;
        support.to = phrase_id;
        support.attributes.push_back({
            "support_role",
            std::string{"persistent_part_phrase_context"},
            evidence_status::derived,
            1.0,
            "",
        });
        graph.add_edge(std::move(support));
    }
    return phrase_id;
}

} // namespace vgmtooling::model
