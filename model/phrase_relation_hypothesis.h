#pragma once

#include "motif_transformation_hypothesis.h"
#include "phrase_region_hypothesis.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace vgmtooling::model {

enum class phrase_relation_kind : std::uint8_t {
    recurrence = 0,
    varied_recurrence,
    rhythm_only_echo,
    weak_relation,
};

struct phrase_relation_hypothesis {
    node_id first_phrase = 0;
    node_id second_phrase = 0;
    phrase_relation_kind kind = phrase_relation_kind::weak_relation;
    motif_transformation_kind motif_transformation = motif_transformation_kind::weak_relation;
    double confidence = 0.0;
    std::vector<node_id> first_motif_nodes;
    std::vector<node_id> second_motif_nodes;
};

inline const char* to_string(phrase_relation_kind kind) noexcept {
    switch (kind) {
    case phrase_relation_kind::recurrence:
        return "recurrence";
    case phrase_relation_kind::varied_recurrence:
        return "varied_recurrence";
    case phrase_relation_kind::rhythm_only_echo:
        return "rhythm_only_echo";
    case phrase_relation_kind::weak_relation:
        return "weak_relation";
    }
    return "unknown";
}

inline const attribute* phrase_region_scope_attribute(const node& value) noexcept {
    for (const auto& item : value.attributes) {
        if (item.name == "identity_scope")
            return &item;
    }
    return nullptr;
}

inline bool is_phrase_region_node(const node& value) noexcept {
    if (value.kind != node_kind::section || value.layer != semantic_layer::musical_structure)
        return false;
    const attribute* scope = phrase_region_scope_attribute(value);
    if (scope == nullptr)
        return false;
    const auto* text = std::get_if<std::string>(&scope->value);
    return text != nullptr && *text == "phrase_region_hypothesis";
}

inline double phrase_region_confidence(const node& value) {
    const attribute* scope = phrase_region_scope_attribute(value);
    if (scope == nullptr)
        throw std::invalid_argument("phrase region is missing identity scope evidence");
    return scope->confidence;
}

inline bool time_coordinate_inside_phrase(
    const time_coordinate& coordinate,
    const time_span& span) noexcept {
    if (coordinate.domain != span.start.domain ||
        coordinate.tick_rate != span.start.tick_rate ||
        coordinate.loop_iteration != span.start.loop_iteration) {
        return false;
    }
    if (coordinate.tick < span.start.tick)
        return false;
    if (!span.end.has_value())
        return true;
    return coordinate.tick < span.end->tick;
}

inline bool source_nodes_inside_phrase(
    const musical_execution_graph& graph,
    const std::vector<node_id>& sources,
    const node& phrase) {
    if (!phrase.active.has_value())
        return false;
    if (sources.empty())
        return false;
    for (node_id source_id : sources) {
        const node* source = graph.find_node(source_id);
        if (source == nullptr || !source->active.has_value())
            return false;
        if (!time_coordinate_inside_phrase(source->active->start, *phrase.active))
            return false;
    }
    return true;
}

inline phrase_relation_kind phrase_relation_from_motif_transformation(
    motif_transformation_kind kind) noexcept {
    switch (kind) {
    case motif_transformation_kind::near_recurrence:
        return phrase_relation_kind::recurrence;
    case motif_transformation_kind::rhythmic_variant:
    case motif_transformation_kind::interval_variant:
    case motif_transformation_kind::contour_preserving_variant:
    case motif_transformation_kind::transformed_relation:
        return phrase_relation_kind::varied_recurrence;
    case motif_transformation_kind::rhythm_only_echo:
        return phrase_relation_kind::rhythm_only_echo;
    case motif_transformation_kind::weak_relation:
        return phrase_relation_kind::weak_relation;
    }
    return phrase_relation_kind::weak_relation;
}

inline phrase_relation_hypothesis infer_phrase_relation(
    const musical_execution_graph& graph,
    node_id first_phrase_id,
    node_id second_phrase_id,
    const motif_transformation_hypothesis& motif) {
    const node* first_phrase = graph.find_node(first_phrase_id);
    const node* second_phrase = graph.find_node(second_phrase_id);
    if (first_phrase == nullptr || !is_phrase_region_node(*first_phrase))
        throw std::invalid_argument("phrase relation requires a valid first phrase region");
    if (second_phrase == nullptr || !is_phrase_region_node(*second_phrase))
        throw std::invalid_argument("phrase relation requires a valid second phrase region");
    if (!source_nodes_inside_phrase(graph, motif.first_nodes, *first_phrase))
        throw std::invalid_argument("first motif occurrence does not lie inside the first phrase");
    if (!source_nodes_inside_phrase(graph, motif.second_nodes, *second_phrase))
        throw std::invalid_argument("second motif occurrence does not lie inside the second phrase");

    phrase_relation_hypothesis result;
    result.first_phrase = first_phrase_id;
    result.second_phrase = second_phrase_id;
    result.kind = phrase_relation_from_motif_transformation(motif.kind);
    result.motif_transformation = motif.kind;
    result.confidence = std::min({
        motif.confidence,
        phrase_region_confidence(*first_phrase),
        phrase_region_confidence(*second_phrase),
    });
    result.first_motif_nodes = motif.first_nodes;
    result.second_motif_nodes = motif.second_nodes;
    return result;
}

inline edge_id add_phrase_relation_hypothesis(
    musical_execution_graph& graph,
    const phrase_relation_hypothesis& relation) {
    const node* first = graph.find_node(relation.first_phrase);
    const node* second = graph.find_node(relation.second_phrase);
    if (first == nullptr || !is_phrase_region_node(*first) ||
        second == nullptr || !is_phrase_region_node(*second)) {
        throw std::invalid_argument("phrase relation references unknown phrase regions");
    }

    edge value;
    value.kind = relation.kind == phrase_relation_kind::recurrence
        ? edge_kind::repeats
        : edge_kind::transforms;
    value.from = relation.first_phrase;
    value.to = relation.second_phrase;
    value.attributes.push_back({
        "identity_scope",
        std::string{"phrase_relation_hypothesis"},
        evidence_status::hypothesis,
        relation.confidence,
        "",
    });
    value.attributes.push_back({
        "phrase_relation_kind",
        std::string{to_string(relation.kind)},
        evidence_status::hypothesis,
        relation.confidence,
        "",
    });
    value.attributes.push_back({
        "motif_transformation_kind",
        std::string{to_string(relation.motif_transformation)},
        evidence_status::hypothesis,
        relation.confidence,
        "",
    });
    value.provenance.push_back({
        evidence_status::hypothesis,
        relation.confidence,
        "phrase-relation-analysis",
        std::nullopt,
        std::string{"phrase relation supported by motif transformation: "} +
            to_string(relation.motif_transformation),
    });
    return graph.add_edge(std::move(value));
}

} // namespace vgmtooling::model
