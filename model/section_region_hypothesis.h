#pragma once

#include "cadential_arrival_hypothesis.h"
#include "phrase_relation_hypothesis.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct section_region_hypothesis {
    time_span span{};
    std::vector<node_id> phrase_ids;
    std::size_t internal_relation_count = 0;
    bool phrase_family_grounded = false;
    bool closing_arrival_grounded = false;
    double weakest_phrase_confidence = 0.0;
    double strongest_internal_relation_confidence = 0.0;
    double closing_arrival_confidence = 0.0;
    double confidence = 0.0;
};

constexpr double unstructured_phrase_group_ceiling = 0.55;
constexpr double related_phrase_section_ceiling = 0.72;
constexpr double arrival_only_section_ceiling = 0.69;
constexpr double related_closed_section_ceiling = 0.85;

inline bool strong_phrase_family_relation(phrase_relation_kind kind) noexcept {
    return kind == phrase_relation_kind::recurrence ||
        kind == phrase_relation_kind::varied_recurrence;
}

inline section_region_hypothesis infer_section_region(
    const musical_execution_graph& graph,
    std::vector<node_id> phrase_ids,
    const std::vector<phrase_relation_hypothesis>& relations = {},
    const std::optional<cadential_arrival_hypothesis>& closing_arrival = std::nullopt,
    std::int64_t closing_alignment_tolerance_ticks = 0) {
    if (phrase_ids.size() < 2)
        throw std::invalid_argument("section region requires at least two phrase regions");
    if (closing_alignment_tolerance_ticks < 0)
        throw std::invalid_argument("section closing-alignment tolerance must be nonnegative");

    std::set<node_id> unique_phrases(phrase_ids.begin(), phrase_ids.end());
    if (unique_phrases.size() != phrase_ids.size())
        throw std::invalid_argument("section region cannot contain duplicate phrase ids");

    std::sort(phrase_ids.begin(), phrase_ids.end(), [&](node_id first_id, node_id second_id) {
        const node* first = graph.find_node(first_id);
        const node* second = graph.find_node(second_id);
        if (first == nullptr || second == nullptr ||
            !first->active.has_value() || !second->active.has_value()) {
            return first_id < second_id;
        }
        return first->active->start.tick < second->active->start.tick;
    });

    section_region_hypothesis result;
    result.phrase_ids = phrase_ids;
    result.weakest_phrase_confidence = 1.0;

    const node* first_phrase = nullptr;
    const node* last_phrase = nullptr;
    for (std::size_t index = 0; index < phrase_ids.size(); ++index) {
        const node* phrase = graph.find_node(phrase_ids[index]);
        if (phrase == nullptr || !is_phrase_region_node(*phrase) || !phrase->active.has_value() ||
            !phrase->active->end.has_value()) {
            throw std::invalid_argument("section region requires bounded phrase-region nodes");
        }
        if (index == 0)
            first_phrase = phrase;
        if (index != 0) {
            const node* previous = graph.find_node(phrase_ids[index - 1]);
            if (previous == nullptr || !previous->active.has_value() || !previous->active->end.has_value())
                throw std::logic_error("validated previous phrase disappeared");
            if (!compatible_phrase_boundary_time_basis(
                    previous->active->start,
                    phrase->active->start)) {
                throw std::invalid_argument("section region phrases require one compatible time basis");
            }
            if (phrase->active->start.tick < previous->active->end->tick)
                throw std::invalid_argument("section region phrases must not overlap");
        }
        last_phrase = phrase;
        result.weakest_phrase_confidence = std::min(
            result.weakest_phrase_confidence,
            phrase_region_confidence(*phrase));
    }

    if (first_phrase == nullptr || last_phrase == nullptr)
        throw std::logic_error("section region lost validated phrase endpoints");
    result.span = time_span{first_phrase->active->start, last_phrase->active->end};

    for (const auto& relation : relations) {
        if (unique_phrases.count(relation.first_phrase) == 0 ||
            unique_phrases.count(relation.second_phrase) == 0 ||
            !strong_phrase_family_relation(relation.kind)) {
            continue;
        }
        ++result.internal_relation_count;
        result.strongest_internal_relation_confidence = std::max(
            result.strongest_internal_relation_confidence,
            relation.confidence);
    }
    result.phrase_family_grounded = result.internal_relation_count != 0;

    if (closing_arrival.has_value()) {
        if (!result.span.end.has_value() ||
            !compatible_phrase_boundary_time_basis(
                *result.span.end,
                closing_arrival->arrival_time) ||
            std::llabs(result.span.end->tick - closing_arrival->arrival_time.tick) >
                closing_alignment_tolerance_ticks) {
            throw std::invalid_argument("section closing arrival does not align with the final phrase boundary");
        }
        result.closing_arrival_grounded = true;
        result.closing_arrival_confidence = closing_arrival->confidence;
    }

    if (result.phrase_family_grounded && result.closing_arrival_grounded) {
        result.confidence = std::min({
            result.weakest_phrase_confidence,
            result.strongest_internal_relation_confidence,
            result.closing_arrival_confidence,
            related_closed_section_ceiling,
        });
    } else if (result.phrase_family_grounded) {
        result.confidence = std::min({
            result.weakest_phrase_confidence,
            result.strongest_internal_relation_confidence,
            related_phrase_section_ceiling,
        });
    } else if (result.closing_arrival_grounded) {
        result.confidence = std::min({
            result.weakest_phrase_confidence,
            result.closing_arrival_confidence,
            arrival_only_section_ceiling,
        });
    } else {
        result.confidence = std::min(
            result.weakest_phrase_confidence,
            unstructured_phrase_group_ceiling);
    }
    return result;
}

inline node_id add_section_region_hypothesis(
    musical_execution_graph& graph,
    const section_region_hypothesis& section) {
    if (section.phrase_ids.size() < 2)
        throw std::invalid_argument("section region is missing phrase members");
    for (node_id phrase_id : section.phrase_ids) {
        const node* phrase = graph.find_node(phrase_id);
        if (phrase == nullptr || !is_phrase_region_node(*phrase))
            throw std::invalid_argument("section region references an unknown phrase region");
    }

    node region;
    region.kind = node_kind::section;
    region.layer = semantic_layer::musical_structure;
    region.flow = flow_kind::stream;
    region.label = "section region hypothesis";
    region.active = section.span;
    region.attributes.push_back({
        "identity_scope",
        std::string{"section_region_hypothesis"},
        evidence_status::hypothesis,
        section.confidence,
        "",
    });
    region.attributes.push_back({
        "phrase_count",
        static_cast<std::uint64_t>(section.phrase_ids.size()),
        evidence_status::derived,
        1.0,
        "phrases",
    });
    region.attributes.push_back({
        "internal_relation_count",
        static_cast<std::uint64_t>(section.internal_relation_count),
        evidence_status::derived,
        1.0,
        "relations",
    });
    region.attributes.push_back({
        "phrase_family_grounded",
        section.phrase_family_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    region.attributes.push_back({
        "closing_arrival_grounded",
        section.closing_arrival_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    region.provenance.push_back({
        evidence_status::hypothesis,
        section.confidence,
        "section-region-analysis",
        std::nullopt,
        "multi-phrase structural unit; does not assign verse/chorus/theme/exposition or other conventional form labels",
    });
    const node_id section_id = graph.add_node(std::move(region));

    for (node_id phrase_id : section.phrase_ids) {
        edge grouping;
        grouping.kind = edge_kind::groups_into;
        grouping.from = phrase_id;
        grouping.to = section_id;
        grouping.attributes.push_back({
            "support_role",
            std::string{"phrase_member"},
            evidence_status::hypothesis,
            section.confidence,
            "",
        });
        graph.add_edge(std::move(grouping));
    }
    return section_id;
}

} // namespace vgmtooling::model
