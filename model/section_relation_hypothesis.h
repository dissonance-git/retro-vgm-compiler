#pragma once

#include "harmonic_rhythm_profile.h"
#include "section_region_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

enum class section_relation_kind : std::uint8_t {
    recurrence = 0,
    varied_recurrence,
    developmental_relation,
    weak_relation,
};

struct section_relation_hypothesis {
    node_id first_section = 0;
    node_id second_section = 0;
    section_relation_kind kind = section_relation_kind::weak_relation;
    std::size_t cross_phrase_relation_count = 0;
    double first_phrase_coverage = 0.0;
    double second_phrase_coverage = 0.0;
    double harmonic_rhythm_similarity = 0.0;
    bool harmonic_rhythm_compared = false;
    bool multi_phrase_grounded = false;
    double confidence = 0.0;
};

constexpr double single_phrase_section_relation_ceiling = 0.60;
constexpr double developmental_section_relation_ceiling = 0.72;
constexpr double section_recurrence_ceiling = 0.88;

inline const char* to_string(section_relation_kind kind) noexcept {
    switch (kind) {
    case section_relation_kind::recurrence:
        return "recurrence";
    case section_relation_kind::varied_recurrence:
        return "varied_recurrence";
    case section_relation_kind::developmental_relation:
        return "developmental_relation";
    case section_relation_kind::weak_relation:
        return "weak_relation";
    }
    return "unknown";
}

inline const attribute* section_region_scope_attribute(const node& value) noexcept {
    for (const auto& item : value.attributes) {
        if (item.key == "identity_scope")
            return &item;
    }
    return nullptr;
}

inline bool is_section_region_node(const node& value) noexcept {
    if (value.kind != node_kind::section || value.layer != semantic_layer::musical_structure)
        return false;
    const attribute* scope = section_region_scope_attribute(value);
    if (scope == nullptr)
        return false;
    const auto* text = std::get_if<std::string>(&scope->value);
    return text != nullptr && *text == "section_region_hypothesis";
}

inline double section_region_confidence(const node& value) {
    const attribute* scope = section_region_scope_attribute(value);
    if (scope == nullptr)
        throw std::invalid_argument("section region is missing identity scope evidence");
    return scope->confidence;
}

inline std::vector<node_id> section_phrase_members(
    const musical_execution_graph& graph,
    node_id section_id) {
    std::vector<node_id> result;
    for (const edge* relation : graph.edges_to(section_id, edge_kind::groups_into)) {
        const node* phrase = graph.find_node(relation->from);
        if (phrase != nullptr && is_phrase_region_node(*phrase))
            result.push_back(relation->from);
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

inline section_relation_hypothesis infer_section_relation(
    const musical_execution_graph& graph,
    node_id first_section_id,
    node_id second_section_id,
    const std::vector<phrase_relation_hypothesis>& cross_phrase_relations,
    std::optional<double> rhythm_similarity = std::nullopt) {
    const node* first_section = graph.find_node(first_section_id);
    const node* second_section = graph.find_node(second_section_id);
    if (first_section == nullptr || !is_section_region_node(*first_section) ||
        second_section == nullptr || !is_section_region_node(*second_section)) {
        throw std::invalid_argument("section relation requires materialized section-region nodes");
    }
    if (rhythm_similarity.has_value() &&
        (!std::isfinite(*rhythm_similarity) || *rhythm_similarity < 0.0 || *rhythm_similarity > 1.0)) {
        throw std::invalid_argument("harmonic-rhythm similarity must be in [0, 1]");
    }

    const auto first_phrases = section_phrase_members(graph, first_section_id);
    const auto second_phrases = section_phrase_members(graph, second_section_id);
    if (first_phrases.empty() || second_phrases.empty())
        throw std::invalid_argument("section relation requires phrase membership evidence");

    const std::set<node_id> first_set(first_phrases.begin(), first_phrases.end());
    const std::set<node_id> second_set(second_phrases.begin(), second_phrases.end());
    std::set<node_id> covered_first;
    std::set<node_id> covered_second;
    std::vector<double> relation_confidences;
    bool any_varied = false;

    for (const auto& relation : cross_phrase_relations) {
        if (first_set.count(relation.first_phrase) == 0 ||
            second_set.count(relation.second_phrase) == 0 ||
            !strong_phrase_family_relation(relation.kind)) {
            continue;
        }
        covered_first.insert(relation.first_phrase);
        covered_second.insert(relation.second_phrase);
        relation_confidences.push_back(relation.confidence);
        any_varied = any_varied || relation.kind == phrase_relation_kind::varied_recurrence;
    }

    section_relation_hypothesis result;
    result.first_section = first_section_id;
    result.second_section = second_section_id;
    result.cross_phrase_relation_count = relation_confidences.size();
    result.first_phrase_coverage = static_cast<double>(covered_first.size()) /
        static_cast<double>(first_phrases.size());
    result.second_phrase_coverage = static_cast<double>(covered_second.size()) /
        static_cast<double>(second_phrases.size());
    result.multi_phrase_grounded = covered_first.size() >= 2 && covered_second.size() >= 2;
    if (rhythm_similarity.has_value()) {
        result.harmonic_rhythm_compared = true;
        result.harmonic_rhythm_similarity = *rhythm_similarity;
    }

    const double minimum_coverage = std::min(
        result.first_phrase_coverage,
        result.second_phrase_coverage);
    if (result.multi_phrase_grounded && minimum_coverage >= 0.75) {
        result.kind = any_varied
            ? section_relation_kind::varied_recurrence
            : section_relation_kind::recurrence;
    } else if (!relation_confidences.empty() && minimum_coverage >= 0.25) {
        result.kind = section_relation_kind::developmental_relation;
    } else {
        result.kind = section_relation_kind::weak_relation;
    }

    double relation_ceiling = 0.0;
    if (!relation_confidences.empty()) {
        std::sort(relation_confidences.begin(), relation_confidences.end(), std::greater<double>{});
        relation_ceiling = relation_confidences.size() >= 2
            ? relation_confidences[1]
            : std::min(relation_confidences.front(), single_phrase_section_relation_ceiling);
    }

    result.confidence = std::min(
        section_region_confidence(*first_section),
        section_region_confidence(*second_section));
    if (result.kind == section_relation_kind::recurrence ||
        result.kind == section_relation_kind::varied_recurrence) {
        result.confidence = std::min({
            result.confidence,
            relation_ceiling,
            section_recurrence_ceiling,
        });
    } else if (result.kind == section_relation_kind::developmental_relation) {
        result.confidence = std::min({
            result.confidence,
            relation_ceiling,
            developmental_section_relation_ceiling,
        });
    } else {
        result.confidence = std::min(result.confidence, single_phrase_section_relation_ceiling);
    }

    // Harmonic-rhythm agreement can corroborate a relation but cannot create
    // section identity by itself. It therefore only acts as an additional
    // ceiling once phrase-family evidence already exists.
    if (result.harmonic_rhythm_compared && !relation_confidences.empty())
        result.confidence = std::min(result.confidence, result.harmonic_rhythm_similarity);

    return result;
}

inline edge_id add_section_relation_hypothesis(
    musical_execution_graph& graph,
    const section_relation_hypothesis& relation) {
    const node* first = graph.find_node(relation.first_section);
    const node* second = graph.find_node(relation.second_section);
    if (first == nullptr || !is_section_region_node(*first) ||
        second == nullptr || !is_section_region_node(*second)) {
        throw std::invalid_argument("section relation references unknown section regions");
    }

    edge value;
    value.kind = relation.kind == section_relation_kind::recurrence
        ? edge_kind::repeats
        : edge_kind::transforms;
    value.from = relation.first_section;
    value.to = relation.second_section;
    value.attributes.push_back({
        "identity_scope",
        std::string{"section_relation_hypothesis"},
        evidence_status::hypothesis,
        relation.confidence,
        "",
    });
    value.attributes.push_back({
        "section_relation_kind",
        std::string{to_string(relation.kind)},
        evidence_status::hypothesis,
        relation.confidence,
        "",
    });
    value.attributes.push_back({
        "cross_phrase_relation_count",
        static_cast<std::uint64_t>(relation.cross_phrase_relation_count),
        evidence_status::derived,
        1.0,
        "relations",
    });
    value.attributes.push_back({
        "first_phrase_coverage",
        relation.first_phrase_coverage,
        evidence_status::derived,
        1.0,
        "ratio",
    });
    value.attributes.push_back({
        "second_phrase_coverage",
        relation.second_phrase_coverage,
        evidence_status::derived,
        1.0,
        "ratio",
    });
    value.provenance.push_back({
        evidence_status::hypothesis,
        relation.confidence,
        "section-relation-analysis",
        std::nullopt,
        "section-level relation grounded in cross-phrase recurrence/transformation; conventional form labels remain unresolved",
    });
    return graph.add_edge(std::move(value));
}

} // namespace vgmtooling::model
