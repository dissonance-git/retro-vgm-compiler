#pragma once

#include "harmonic_transition_hypothesis.h"
#include "motif_transformation_hypothesis.h"
#include "persistent_part_trajectory.h"
#include "phrase_boundary_hypothesis.h"
#include "phrase_role_evidence.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

// Continuation is a phrase-role hypothesis, not a cadence label. These adapters
// admit only evidence that independently demonstrates musical process across a
// candidate arrival. They do not establish the role by themselves.
inline bool phrase_continuation_scope_contains(
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

inline std::vector<node_id> unique_phrase_continuation_support_nodes(
    std::vector<node_id> nodes) {
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    return nodes;
}

inline const node& phrase_continuation_timed_node(
    const musical_execution_graph& graph,
    node_id id,
    const time_span& role_scope,
    const time_coordinate& arrival) {
    const node* value = graph.find_node(id);
    if (value == nullptr || !value->active.has_value())
        throw std::invalid_argument(
            "phrase continuation evidence requires timed support nodes");
    if (!compatible_phrase_role_time_basis(value->active->start, arrival))
        throw std::invalid_argument(
            "phrase continuation support must share the arrival time basis");
    if (!phrase_continuation_scope_contains(role_scope, value->active->start))
        throw std::invalid_argument(
            "phrase continuation support node lies outside the role scope");
    if (value->active->end.has_value() &&
        !compatible_phrase_role_time_basis(value->active->start, *value->active->end)) {
        throw std::invalid_argument(
            "phrase continuation support node has an incompatible active span");
    }
    return *value;
}

inline phrase_role_evidence make_persistent_part_continuation_evidence(
    const musical_execution_graph& graph,
    const persistent_part_trajectory& trajectory,
    time_coordinate arrival,
    time_span role_scope,
    phrase_role_formal_scale formal_scale,
    std::string source = "persistent-part-continuation-analysis") {
    validate_phrase_role_scope(role_scope);
    if (source.empty())
        throw std::invalid_argument(
            "persistent-part continuation evidence requires a source");
    if (!phrase_continuation_scope_contains(role_scope, arrival))
        throw std::invalid_argument(
            "persistent-part continuation arrival lies outside the role scope");

    // Rebuild from retained transitions so a manually modified trajectory cannot
    // smuggle an ungrounded subject list or confidence into phrase syntax.
    const auto verified = make_persistent_part_trajectory(trajectory.transitions);

    bool before_arrival = false;
    bool after_arrival = false;
    for (node_id subject_id : verified.subject_nodes) {
        const node& subject = phrase_continuation_timed_node(
            graph, subject_id, role_scope, arrival);
        const auto& active = *subject.active;
        const time_coordinate terminal = active.end.value_or(active.start);
        if (!compatible_phrase_role_time_basis(terminal, arrival))
            throw std::invalid_argument(
                "persistent-part continuation terminal time is incompatible");
        before_arrival = before_arrival || active.start.tick < arrival.tick;
        after_arrival = after_arrival ||
            active.start.tick > arrival.tick ||
            terminal.tick > arrival.tick;
    }
    if (!before_arrival || !after_arrival)
        throw std::invalid_argument(
            "persistent-part continuation must have grounded material on both sides of the arrival");

    phrase_role_evidence result;
    result.role = phrase_role_kind::continuation;
    result.scope = std::move(role_scope);
    result.formal_scale = formal_scale;
    result.origin = phrase_role_evidence_origin::persistent_part_continuation;
    result.polarity = phrase_role_evidence_polarity::supports;
    result.status = evidence_status::hypothesis;
    result.confidence = verified.confidence;
    result.source = std::move(source);
    result.detail =
        "a grounded persistent-part trajectory has material before and after the candidate arrival";
    result.support_nodes = unique_phrase_continuation_support_nodes(
        verified.subject_nodes);
    return result;
}

inline bool motif_transformation_supports_sequence_continuation(
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

struct phrase_continuation_occurrence_bounds {
    bool initialized = false;
    std::int64_t earliest_tick = 0;
    std::int64_t latest_tick = 0;
};

inline phrase_continuation_occurrence_bounds phrase_continuation_occurrence_time_bounds(
    const musical_execution_graph& graph,
    const std::vector<node_id>& nodes,
    const time_span& role_scope,
    const time_coordinate& arrival) {
    if (nodes.empty())
        throw std::invalid_argument(
            "motif-sequence continuation requires non-empty motif occurrences");

    phrase_continuation_occurrence_bounds result;
    for (node_id id : nodes) {
        const node& value = phrase_continuation_timed_node(
            graph, id, role_scope, arrival);
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

inline phrase_role_evidence make_motif_sequence_continuation_evidence(
    const musical_execution_graph& graph,
    const motif_transformation_hypothesis& motif,
    time_coordinate arrival,
    time_span role_scope,
    phrase_role_formal_scale formal_scale,
    std::string source = "motif-sequence-continuation-analysis") {
    validate_phrase_role_scope(role_scope);
    if (source.empty())
        throw std::invalid_argument(
            "motif-sequence continuation evidence requires a source");
    if (!std::isfinite(motif.confidence) ||
        motif.confidence < 0.0 || motif.confidence > 1.0) {
        throw std::invalid_argument(
            "motif-sequence continuation confidence must be in [0, 1]");
    }
    if (!motif_transformation_supports_sequence_continuation(motif.kind))
        throw std::invalid_argument(
            "weak or rhythm-only motif relation cannot establish sequence-continuation evidence");
    if (!phrase_continuation_scope_contains(role_scope, arrival))
        throw std::invalid_argument(
            "motif-sequence continuation arrival lies outside the role scope");

    const auto first = phrase_continuation_occurrence_time_bounds(
        graph, motif.first_nodes, role_scope, arrival);
    const auto second = phrase_continuation_occurrence_time_bounds(
        graph, motif.second_nodes, role_scope, arrival);

    if (first.latest_tick >= second.earliest_tick ||
        first.latest_tick > arrival.tick ||
        second.earliest_tick < arrival.tick) {
        throw std::invalid_argument(
            "motif-sequence continuation requires ordered occurrences spanning the candidate arrival");
    }

    phrase_role_evidence result;
    result.role = phrase_role_kind::continuation;
    result.scope = std::move(role_scope);
    result.formal_scale = formal_scale;
    result.origin = phrase_role_evidence_origin::motif_analysis;
    result.polarity = phrase_role_evidence_polarity::supports;
    result.status = evidence_status::hypothesis;
    result.confidence = motif.confidence;
    result.source = std::move(source);
    result.detail =
        std::string{"ordered motif transformation crosses the candidate arrival: "} +
        to_string(motif.kind);
    result.support_nodes = motif.first_nodes;
    result.support_nodes.insert(
        result.support_nodes.end(),
        motif.second_nodes.begin(),
        motif.second_nodes.end());
    result.support_nodes = unique_phrase_continuation_support_nodes(
        std::move(result.support_nodes));
    return result;
}

inline phrase_role_evidence make_harmonic_process_continuation_evidence(
    const musical_execution_graph& graph,
    const std::vector<harmonic_transition_hypothesis>& transitions,
    std::vector<node_id> support_nodes,
    time_coordinate arrival,
    time_span role_scope,
    phrase_role_formal_scale formal_scale,
    std::string source = "harmonic-process-continuation-analysis") {
    validate_phrase_role_scope(role_scope);
    if (source.empty())
        throw std::invalid_argument(
            "harmonic-process continuation evidence requires a source");
    if (transitions.size() < 2)
        throw std::invalid_argument(
            "sustained harmonic-process continuation requires at least two transitions");
    if (support_nodes.empty())
        throw std::invalid_argument(
            "harmonic-process continuation requires support nodes");
    if (!phrase_continuation_scope_contains(role_scope, arrival))
        throw std::invalid_argument(
            "harmonic-process continuation arrival lies outside the role scope");

    double confidence = 1.0;
    for (std::size_t index = 0; index < transitions.size(); ++index) {
        const auto& current = transitions[index];
        if (!std::isfinite(current.confidence) ||
            current.confidence < 0.0 || current.confidence > 1.0) {
            throw std::invalid_argument(
                "harmonic-process continuation confidence must be in [0, 1]");
        }
        if (!current.root_motion_reliable)
            throw std::invalid_argument(
                "harmonic-process continuation requires reliable root motion");
        if (!compatible_phrase_role_time_basis(current.first_time, arrival) ||
            !compatible_phrase_role_time_basis(current.second_time, arrival) ||
            current.second_time.tick <= current.first_time.tick) {
            throw std::invalid_argument(
                "harmonic-process continuation requires ordered transitions on one time basis");
        }
        if (!phrase_continuation_scope_contains(role_scope, current.first_time) ||
            !phrase_continuation_scope_contains(role_scope, current.second_time)) {
            throw std::invalid_argument(
                "harmonic-process transition lies outside the role scope");
        }
        if (index > 0) {
            const auto& previous = transitions[index - 1];
            if (previous.second_time != current.first_time ||
                previous.second_root_pitch_class != current.first_root_pitch_class ||
                previous.second_quality != current.first_quality) {
                throw std::invalid_argument(
                    "harmonic-process continuation requires a contiguous harmonic chain");
            }
        }
        confidence = std::min(confidence, current.confidence);
    }

    if (transitions.front().first_time.tick >= arrival.tick ||
        transitions.back().second_time.tick <= arrival.tick) {
        throw std::invalid_argument(
            "harmonic-process continuation must extend across the candidate arrival");
    }

    support_nodes = unique_phrase_continuation_support_nodes(
        std::move(support_nodes));
    for (node_id id : support_nodes) {
        if (graph.find_node(id) == nullptr)
            throw std::invalid_argument(
                "harmonic-process continuation references an unknown support node");
    }

    phrase_role_evidence result;
    result.role = phrase_role_kind::continuation;
    result.scope = std::move(role_scope);
    result.formal_scale = formal_scale;
    result.origin = phrase_role_evidence_origin::harmonic_process;
    result.polarity = phrase_role_evidence_polarity::supports;
    result.status = evidence_status::hypothesis;
    result.confidence = confidence;
    result.source = std::move(source);
    result.detail =
        "a contiguous reliable harmonic-transition chain crosses the candidate arrival without using a cadence or Roman-numeral label";
    result.support_nodes = std::move(support_nodes);
    return result;
}

struct phrase_boundary_continuation_projection {
    std::vector<phrase_role_evidence> evidence;
    bool cross_boundary_continuity_grounded = false;
    bool cadence_derived_evidence_present = false;
    std::size_t noncontinuity_boundary_observations_ignored = 0;
};

inline phrase_boundary_continuation_projection
project_phrase_boundary_continuation_evidence(
    const phrase_boundary_hypothesis& boundary,
    time_span role_scope,
    phrase_role_formal_scale formal_scale) {
    validate_phrase_role_scope(role_scope);
    if (!phrase_continuation_scope_contains(role_scope, boundary.boundary))
        throw std::invalid_argument(
            "phrase-boundary continuation projection lies outside the role scope");

    phrase_boundary_continuation_projection result;
    for (const auto& item : boundary.evidence) {
        validate_phrase_boundary_evidence(item);

        // Cadence-derived boundary evidence is explicitly visible but cannot be
        // reused to prove continuation. This prevents phrase syntax from becoming
        // circular with cadence classification.
        if (item.kind == phrase_boundary_evidence_kind::cadence_or_resolution) {
            result.cadence_derived_evidence_present = true;
            continue;
        }

        // A boundary detector's positive evidence does not itself prove an
        // ending, and is therefore not inverted into a continuation counter here.
        // The only direct continuation witness is explicit continuity that
        // counters the proposed boundary.
        if (item.kind != phrase_boundary_evidence_kind::cross_boundary_continuity ||
            item.polarity != phrase_boundary_evidence_polarity::counters) {
            ++result.noncontinuity_boundary_observations_ignored;
            continue;
        }

        phrase_role_evidence projected;
        projected.role = phrase_role_kind::continuation;
        projected.scope = role_scope;
        projected.formal_scale = formal_scale;
        projected.origin = phrase_role_evidence_origin::phrase_boundary_analysis;
        projected.polarity = phrase_role_evidence_polarity::supports;
        projected.status = item.status;
        projected.confidence = item.confidence;
        projected.source = item.source;
        projected.detail =
            std::string{"cross-boundary continuity independently counters a phrase boundary: "} +
            item.detail;
        projected.support_nodes = item.support_nodes;
        result.evidence.push_back(std::move(projected));
    }

    result.cross_boundary_continuity_grounded = !result.evidence.empty();
    return result;
}

} // namespace vgmtooling::model
