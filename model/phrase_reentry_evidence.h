#pragma once

#include "motif_transformation_hypothesis.h"
#include "part_motif_profile.h"
#include "phrase_boundary_hypothesis.h"
#include "phrase_relation_hypothesis.h"
#include "phrase_role_evidence.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
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

inline phrase_role_evidence make_grounded_reentry_boundary_evidence(
    const phrase_boundary_hypothesis& boundary,
    time_span role_scope,
    phrase_role_formal_scale formal_scale,
    std::string source = "grounded-reentry-boundary-analysis") {
    validate_phrase_role_scope(role_scope);
    if (source.empty())
        throw std::invalid_argument(
            "grounded re-entry boundary evidence requires a source");
    if (!phrase_boundary_supports_reentry(boundary))
        throw std::invalid_argument(
            "grounded new-phrase onset requires a structural or authored boundary");
    if (role_scope.start != boundary.boundary)
        throw std::invalid_argument(
            "canonical new-phrase scope must begin at its boundary");

    auto support_nodes = phrase_boundary_reentry_support_nodes(boundary);
    if (support_nodes.empty())
        throw std::invalid_argument(
            "grounded re-entry boundary requires provenance support nodes");

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
        "grounded structural boundary licenses a candidate phrase start independently of performed re-onset";
    result.support_nodes = std::move(support_nodes);
    return result;
}

inline phrase_role_evidence make_performed_phrase_reonset_evidence(
    const musical_execution_graph& graph,
    const time_coordinate& boundary,
    const std::vector<part_gesture_observation>& observations,
    std::int64_t onset_tolerance_ticks,
    time_span role_scope,
    phrase_role_formal_scale formal_scale,
    std::string source = "performed-phrase-reonset-analysis") {
    validate_phrase_role_scope(role_scope);
    if (source.empty())
        throw std::invalid_argument(
            "performed phrase re-onset evidence requires a source");
    if (onset_tolerance_ticks < 0)
        throw std::invalid_argument(
            "performed phrase re-onset tolerance must be nonnegative");
    if (observations.empty())
        throw std::invalid_argument(
            "performed phrase re-onset requires at least one observation");
    if (role_scope.start != boundary)
        throw std::invalid_argument(
            "canonical performed re-onset boundary must equal the role-scope start");

    std::map<node_id, double> best_part_strength;
    std::vector<node_id> support_nodes;
    evidence_status weakest_status = observations.front().status;

    for (const auto& observation : observations) {
        if (observation.part_id == 0 || observation.source_node == 0)
            throw std::invalid_argument(
                "performed re-onset observations require persistent-part and source ids");
        if (!std::isfinite(observation.confidence)
            || observation.confidence < 0.0 || observation.confidence > 1.0) {
            throw std::invalid_argument(
                "performed re-onset confidence must be finite and in [0, 1]");
        }
        if (!compatible_phrase_role_time_basis(boundary, observation.onset)
            || observation.onset.tick < boundary.tick
            || observation.onset.tick - boundary.tick > onset_tolerance_ticks
            || !phrase_reentry_scope_contains(role_scope, observation.onset)) {
            throw std::invalid_argument(
                "performed re-onset must occur at or shortly after the grounded boundary");
        }

        const node* part = graph.find_node(observation.part_id);
        if (part == nullptr)
            throw std::invalid_argument(
                "performed re-onset references an unknown persistent part");
        const auto part_identity = read_persistent_part_motif_evidence(*part);

        const node* source_node = graph.find_node(observation.source_node);
        if (source_node == nullptr || !source_node->active.has_value()
            || source_node->active->start != observation.onset) {
            throw std::invalid_argument(
                "performed re-onset timing must be grounded by its source node");
        }

        const double strength = std::min(
            observation.confidence,
            part_identity.confidence);
        best_part_strength[observation.part_id] = std::max(
            best_part_strength[observation.part_id],
            strength);
        support_nodes.push_back(observation.source_node);
        weakest_status = static_cast<evidence_status>(std::max(
            static_cast<std::uint8_t>(weakest_status),
            static_cast<std::uint8_t>(observation.status)));
    }

    std::vector<double> strengths;
    strengths.reserve(best_part_strength.size());
    for (const auto& item : best_part_strength)
        strengths.push_back(item.second);
    std::sort(strengths.begin(), strengths.end(), std::greater<double>{});

    double confidence = strengths.front();
    if (strengths.size() >= 2)
        confidence = strengths[1];
    else
        confidence = std::min(confidence, phrase_role_single_domain_ceiling);

    phrase_role_evidence result;
    result.role = phrase_role_kind::new_phrase_onset;
    result.scope = std::move(role_scope);
    result.formal_scale = formal_scale;
    result.origin = phrase_role_evidence_origin::performance_reonset;
    result.polarity = phrase_role_evidence_polarity::supports;
    result.status = weakest_status;
    result.confidence = confidence;
    result.source = std::move(source);
    result.detail =
        "performed re-onset after the boundary is grounded across " +
        std::to_string(best_part_strength.size()) +
        " independently tracked persistent part(s)";
    result.support_nodes = unique_phrase_reentry_support_nodes(
        std::move(support_nodes));
    return result;
}

inline phrase_role_hypothesis make_grounded_new_phrase_onset_hypothesis(
    phrase_role_evidence boundary,
    phrase_role_evidence reonset,
    double proposed_confidence = 0.95) {
    validate_phrase_role_evidence(boundary);
    validate_phrase_role_evidence(reonset);
    if (boundary.role != phrase_role_kind::new_phrase_onset
        || reonset.role != phrase_role_kind::new_phrase_onset
        || boundary.polarity != phrase_role_evidence_polarity::supports
        || reonset.polarity != phrase_role_evidence_polarity::supports) {
        throw std::invalid_argument(
            "canonical new-phrase onset requires positive new-phrase evidence");
    }
    if (boundary.origin != phrase_role_evidence_origin::phrase_boundary_analysis
        || reonset.origin != phrase_role_evidence_origin::performance_reonset) {
        throw std::invalid_argument(
            "canonical new-phrase onset requires independent boundary and performed re-onset domains");
    }
    if (!same_phrase_role_scope(boundary.scope, reonset.scope)
        || boundary.formal_scale != reonset.formal_scale) {
        throw std::invalid_argument(
            "canonical new-phrase evidence must share scope and formal scale");
    }

    return make_phrase_role_hypothesis(
        phrase_role_kind::new_phrase_onset,
        boundary.scope,
        boundary.formal_scale,
        proposed_confidence,
        {std::move(boundary), std::move(reonset)},
        {phrase_role_kind::continuation});
}

inline bool phrase_relation_supports_canonical_return(
    const phrase_relation_hypothesis& relation) noexcept {
    const bool recurrence =
        relation.kind == phrase_relation_kind::recurrence
        || relation.kind == phrase_relation_kind::varied_recurrence;
    return recurrence
        && phrase_relation_from_motif_transformation(
               relation.motif_transformation) == relation.kind;
}

inline phrase_role_hypothesis make_grounded_phrase_return_hypothesis(
    const musical_execution_graph& graph,
    const phrase_role_hypothesis& onset,
    const phrase_relation_hypothesis& relation,
    double proposed_confidence = 0.95,
    std::string source = "grounded-phrase-return-analysis") {
    if (source.empty())
        throw std::invalid_argument("grounded phrase return requires a source");
    if (onset.role != phrase_role_kind::new_phrase_onset
        || !onset.cross_domain_grounded || onset.support_domains < 2) {
        throw std::invalid_argument(
            "canonical return requires a cross-domain grounded new-phrase onset");
    }

    bool has_boundary = false;
    bool has_reonset = false;
    for (const auto& item : onset.evidence) {
        has_boundary = has_boundary
            || (item.polarity == phrase_role_evidence_polarity::supports
                && item.origin == phrase_role_evidence_origin::phrase_boundary_analysis);
        has_reonset = has_reonset
            || (item.polarity == phrase_role_evidence_polarity::supports
                && item.origin == phrase_role_evidence_origin::performance_reonset);
    }
    if (!has_boundary || !has_reonset)
        throw std::invalid_argument(
            "canonical return requires boundary plus performed re-onset grounding");
    if (!phrase_relation_supports_canonical_return(relation))
        throw std::invalid_argument(
            "canonical return requires recurrence or varied recurrence between phrase regions");
    if (!std::isfinite(relation.confidence)
        || relation.confidence < 0.0 || relation.confidence > 1.0) {
        throw std::invalid_argument(
            "canonical phrase-return recurrence confidence must be finite and in [0, 1]");
    }

    const node* first = graph.find_node(relation.first_phrase);
    const node* second = graph.find_node(relation.second_phrase);
    if (first == nullptr || second == nullptr
        || relation.first_phrase == relation.second_phrase
        || !is_phrase_region_node(*first) || !is_phrase_region_node(*second)
        || !first->active.has_value() || !second->active.has_value()) {
        throw std::invalid_argument(
            "canonical return requires two distinct materialized phrase regions");
    }
    if (!first->active->end.has_value()
        || first->active->end->tick > second->active->start.tick) {
        throw std::invalid_argument(
            "canonical return source phrase must precede the returning phrase");
    }
    if (!same_phrase_role_scope(*second->active, onset.scope))
        throw std::invalid_argument(
            "canonical return onset scope must equal the returning phrase region");
    if (!source_nodes_inside_phrase(
            graph, relation.first_motif_nodes, *first)
        || !source_nodes_inside_phrase(
            graph, relation.second_motif_nodes, *second)) {
        throw std::invalid_argument(
            "canonical return recurrence support must live inside its phrase regions");
    }

    std::vector<phrase_role_evidence> evidence;
    evidence.reserve(onset.evidence.size() + 1);
    for (auto item : onset.evidence) {
        item.role = phrase_role_kind::return_role;
        item.detail =
            "return inherits grounded new-phrase onset: " + item.detail;
        evidence.push_back(std::move(item));
    }

    phrase_role_evidence recurrence;
    recurrence.role = phrase_role_kind::return_role;
    recurrence.scope = onset.scope;
    recurrence.formal_scale = onset.formal_scale;
    recurrence.origin = phrase_role_evidence_origin::recurrence_analysis;
    recurrence.polarity = phrase_role_evidence_polarity::supports;
    recurrence.status = evidence_status::hypothesis;
    recurrence.confidence = relation.confidence;
    recurrence.source = std::move(source);
    recurrence.detail =
        std::string{"materialized earlier-to-later phrase relation is "} +
        to_string(relation.kind) + " via " +
        to_string(relation.motif_transformation);
    recurrence.support_nodes = {
        relation.first_phrase,
        relation.second_phrase,
    };
    recurrence.support_nodes.insert(
        recurrence.support_nodes.end(),
        relation.first_motif_nodes.begin(),
        relation.first_motif_nodes.end());
    recurrence.support_nodes.insert(
        recurrence.support_nodes.end(),
        relation.second_motif_nodes.begin(),
        relation.second_motif_nodes.end());
    recurrence.support_nodes = unique_phrase_reentry_support_nodes(
        std::move(recurrence.support_nodes));
    evidence.push_back(std::move(recurrence));

    // Recurrence is a required claim, not a decorative third witness. The
    // canonical return can never outrun the earlier-to-later phrase relation.
    const double bounded_proposal = std::min({
        proposed_confidence,
        onset.confidence,
        relation.confidence,
    });
    return make_phrase_role_hypothesis(
        phrase_role_kind::return_role,
        onset.scope,
        onset.formal_scale,
        bounded_proposal,
        std::move(evidence),
        {phrase_role_kind::continuation});
}

} // namespace vgmtooling::model
