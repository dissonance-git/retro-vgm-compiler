#pragma once

#include "motif_transformation_hypothesis.h"
#include "phrase_boundary_hypothesis.h"
#include "phrase_role_evidence.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

// Re-entry evidence separates two questions that recurrence alone cannot answer:
// did a formal boundary occur, and does the material after that boundary relate
// to earlier material strongly enough to support a return hypothesis?
inline bool phrase_reentry_scope_contains(
    const time_span& scope,
    const time_coordinate& coordinate) noexcept {
    if (!compatible_phrase_role_time_basis(scope.start, coordinate))
        return false;
    if (coordinate.tick < scope.start.tick)
        return false;
    if (scope.end.has_value() && coordinate.tick > scope.end->tick)
        return false;
    return true;
}

inline std::vector<node_id> unique_phrase_reentry_support_nodes(
    std::vector<node_id> nodes) {
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    return nodes;
}

inline const node& phrase_reentry_timed_node(
    const musical_execution_graph& graph,
    node_id id,
    const time_span& role_scope,
    const time_coordinate& boundary) {
    const node* value = graph.find_node(id);
    if (value == nullptr || !value->active.has_value())
        throw std::invalid_argument(
            "phrase re-entry evidence requires timed support nodes");
    if (!compatible_phrase_role_time_basis(value->active->start, boundary))
        throw std::invalid_argument(
            "phrase re-entry support must share the boundary time basis");
    if (!phrase_reentry_scope_contains(role_scope, value->active->start))
        throw std::invalid_argument(
            "phrase re-entry support node lies outside the role scope");
    return *value;
}

inline bool phrase_boundary_supports_reentry(
    const phrase_boundary_hypothesis& boundary) noexcept {
    return boundary.supporting_observations > 0
        && (boundary.structural_support || boundary.authored_grounded)
        && boundary.confidence > 0.0;
}

inline std::vector<node_id> phrase_boundary_reentry_support_nodes(
    const phrase_boundary_hypothesis& boundary) {
    std::vector<node_id> result;
    for (const auto& item : boundary.evidence) {
        if (item.polarity != phrase_boundary_evidence_polarity::supports)
            continue;
        if (!phrase_boundary_kind_is_structural(item.kind)
            && item.kind != phrase_boundary_evidence_kind::temporal_gap
            && item.kind != phrase_boundary_evidence_kind::part_density_change)
            continue;
        result.insert(
            result.end(),
            item.support_nodes.begin(),
            item.support_nodes.end());
    }
    return unique_phrase_reentry_support_nodes(std::move(result));
}

inline phrase_role_evidence make_post_boundary_new_phrase_evidence(
    const musical_execution_graph& graph,
    const phrase_boundary_hypothesis& boundary,
    std::vector<node_id> onset_nodes,
    time_span role_scope,
    phrase_role_formal_scale formal_scale,
    std::string source = "post-boundary-new-phrase-analysis") {
    validate_phrase_role_scope(role_scope);
    if (source.empty())
        throw std::invalid_argument(
            "new-phrase onset evidence requires a source");
    if (!phrase_reentry_scope_contains(role_scope, boundary.boundary))
        throw std::invalid_argument(
            "new-phrase boundary lies outside the role scope");
    if (!phrase_boundary_supports_reentry(boundary))
        throw std::invalid_argument(
            "new-phrase onset requires a grounded structural boundary");
    if (onset_nodes.empty())
        throw std::invalid_argument(
            "new-phrase onset requires post-boundary material");

    for (node_id id : onset_nodes) {
        const node& value = phrase_reentry_timed_node(
            graph, id, role_scope, boundary.boundary);
        if (value.active->start.tick < boundary.boundary.tick)
            throw std::invalid_argument(
                "new-phrase onset material cannot begin before its boundary");
    }

    auto boundary_nodes = phrase_boundary_reentry_support_nodes(boundary);
    onset_nodes.insert(
        onset_nodes.end(),
        boundary_nodes.begin(),
        boundary_nodes.end());

    phrase_role_evidence result;
    result.role = phrase_role_kind::new_phrase_onset;
    result.scope = std::move(role_scope);
    result.formal_scale = formal_scale;
    result.origin = phrase_role_evidence_origin::phrase_boundary_analysis;
    result.polarity = phrase_role_evidence_polarity::supports;
    result.status = evidence_status::hypothesis;
    result.confidence = boundary.confidence;
    result.source = std::move(source);
    result.detail =
        "grounded post-boundary material supports a new-phrase onset candidate without identifying recurrence";
    result.support_nodes = unique_phrase_reentry_support_nodes(
        std::move(onset_nodes));
    return result;
}

inline phrase_role_evidence make_boundary_return_evidence(
    const phrase_boundary_hypothesis& boundary,
    time_span role_scope,
    phrase_role_formal_scale formal_scale,
    std::string source = "boundary-return-analysis") {
    validate_phrase_role_scope(role_scope);
    if (source.empty())
        throw std::invalid_argument("boundary return evidence requires a source");
    if (!phrase_reentry_scope_contains(role_scope, boundary.boundary))
        throw std::invalid_argument("return boundary lies outside the role scope");
    if (!phrase_boundary_supports_reentry(boundary))
        throw std::invalid_argument(
            "return requires a grounded structural boundary");

    phrase_role_evidence result;
    result.role = phrase_role_kind::return_role;
    result.scope = std::move(role_scope);
    result.formal_scale = formal_scale;
    result.origin = phrase_role_evidence_origin::phrase_boundary_analysis;
    result.polarity = phrase_role_evidence_polarity::supports;
    result.status = evidence_status::hypothesis;
    result.confidence = boundary.confidence;
    result.source = std::move(source);
    result.detail =
        "a grounded formal boundary supports re-entry but does not by itself identify what returned";
    result.support_nodes = phrase_boundary_reentry_support_nodes(boundary);
    return result;
}

inline bool motif_transformation_supports_return(
    motif_transformation_kind kind) noexcept {
    switch (kind) {
    case motif_transformation_kind::near_recurrence:
    case motif_transformation_kind::rhythmic_variant:
    case motif_transformation_kind::interval_variant:
    case motif_transformation_kind::contour_preserving_variant:
    case motif_transformation_kind::transformed_relation:
        return true;
    case motif_transformation_kind::rhythm_only_echo:
    case motif_transformation_kind::weak_relation:
        return false;
    }
    return false;
}

struct phrase_reentry_occurrence_bounds {
    bool initialized = false;
    std::int64_t earliest_tick = 0;
    std::int64_t latest_tick = 0;
};

inline phrase_reentry_occurrence_bounds phrase_reentry_occurrence_time_bounds(
    const musical_execution_graph& graph,
    const std::vector<node_id>& nodes,
    const time_span& role_scope,
    const time_coordinate& boundary) {
    if (nodes.empty())
        throw std::invalid_argument(
            "return recurrence requires non-empty motif occurrences");

    phrase_reentry_occurrence_bounds result;
    for (node_id id : nodes) {
        const node& value = phrase_reentry_timed_node(
            graph, id, role_scope, boundary);
        const std::int64_t tick = value.active->start.tick;
        if (!result.initialized) {
            result.initialized = true;
            result.earliest_tick = tick;
            result.latest_tick = tick;
        } else {
            result.earliest_tick = std::min(result.earliest_tick, tick);
            result.latest_tick = std::max(result.latest_tick, tick);
        }
    }
    return result;
}

inline phrase_role_evidence make_motif_return_recurrence_evidence(
    const musical_execution_graph& graph,
    const motif_transformation_hypothesis& motif,
    const phrase_boundary_hypothesis& boundary,
    time_span role_scope,
    phrase_role_formal_scale formal_scale,
    std::string source = "motif-return-recurrence-analysis") {
    validate_phrase_role_scope(role_scope);
    if (source.empty())
        throw std::invalid_argument("motif return evidence requires a source");
    if (!std::isfinite(motif.confidence)
        || motif.confidence < 0.0 || motif.confidence > 1.0)
        throw std::invalid_argument(
            "motif return confidence must be in [0, 1]");
    if (!motif_transformation_supports_return(motif.kind))
        throw std::invalid_argument(
            "weak or rhythm-only motif relation cannot identify a return");
    if (!phrase_boundary_supports_reentry(boundary))
        throw std::invalid_argument(
            "motif recurrence cannot become a return without a grounded boundary");
    if (!phrase_reentry_scope_contains(role_scope, boundary.boundary))
        throw std::invalid_argument("return boundary lies outside the role scope");

    const auto first = phrase_reentry_occurrence_time_bounds(
        graph, motif.first_nodes, role_scope, boundary.boundary);
    const auto second = phrase_reentry_occurrence_time_bounds(
        graph, motif.second_nodes, role_scope, boundary.boundary);

    if (first.latest_tick >= boundary.boundary.tick
        || second.earliest_tick < boundary.boundary.tick
        || first.latest_tick >= second.earliest_tick) {
        throw std::invalid_argument(
            "return recurrence requires an earlier occurrence and a post-boundary re-entry");
    }

    phrase_role_evidence result;
    result.role = phrase_role_kind::return_role;
    result.scope = std::move(role_scope);
    result.formal_scale = formal_scale;
    result.origin = phrase_role_evidence_origin::recurrence_analysis;
    result.polarity = phrase_role_evidence_polarity::supports;
    result.status = evidence_status::hypothesis;
    result.confidence = std::min(motif.confidence, boundary.confidence);
    result.source = std::move(source);
    result.detail =
        std::string{"motif recurrence re-enters after a grounded boundary: "}
        + to_string(motif.kind);
    result.support_nodes = motif.first_nodes;
    result.support_nodes.insert(
        result.support_nodes.end(),
        motif.second_nodes.begin(),
        motif.second_nodes.end());
    result.support_nodes = unique_phrase_reentry_support_nodes(
        std::move(result.support_nodes));
    return result;
}

} // namespace vgmtooling::model
