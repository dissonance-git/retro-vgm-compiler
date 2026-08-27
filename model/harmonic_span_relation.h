#pragma once

#include "cadential_arrival_hypothesis.h"
#include "harmonic_transition_hypothesis.h"
#include "phrase_role_evidence.h"
#include "tonal_region_relation.h"
#include "voice_leading_hypothesis.h"

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

// A harmonic span preserves every local surface event while allowing a
// non-adjacent structural dependency to be proposed across them. The relation
// is deliberately function-neutral: neither prolongation nor delayed resolution
// is permission to invent a key, Roman numeral, cadence class, or tonal function.
enum class harmonic_span_relation_kind : std::uint8_t {
    unresolved = 0,
    prolongation_candidate,
    delayed_resolution_candidate,
};

enum class harmonic_span_evidence_kind : std::uint8_t {
    cross_span_voice_leading = 0,
    persistent_part_continuity,
    retained_tonal_center,
    unresolved_process,
    later_structural_arrival,
    authored_relation,
    contradiction,
};

enum class harmonic_span_evidence_origin : std::uint8_t {
    voice_leading = 0,
    persistent_part,
    tonal_region,
    phrase_structure,
    cadential_arrival,
    authored_source,
    external_annotation,
};

enum class harmonic_span_evidence_polarity : std::uint8_t {
    supports = 0,
    counters,
};

struct harmonic_span_evidence {
    harmonic_span_evidence_kind kind =
        harmonic_span_evidence_kind::cross_span_voice_leading;
    harmonic_span_evidence_origin origin =
        harmonic_span_evidence_origin::voice_leading;
    harmonic_span_evidence_polarity polarity =
        harmonic_span_evidence_polarity::supports;
    evidence_status status = evidence_status::hypothesis;
    double confidence = 0.0;
    std::string source;
    std::string detail;
    std::vector<node_id> support_nodes;
};

struct harmonic_span_relation_hypothesis {
    harmonic_span_relation_kind kind = harmonic_span_relation_kind::unresolved;
    time_span scope{};
    std::vector<node_id> surface_nodes;
    std::size_t transition_count = 0;
    std::size_t intervening_event_count = 0;
    std::size_t support_domains = 0;
    double surface_chain_confidence = 0.0;
    double independent_support_ceiling = 0.0;
    double proposed_confidence = 0.0;
    double confidence = 0.0;
    bool surface_events_preserved = false;
    bool cross_domain_grounded = false;
    bool strong_conflict_present = false;
    bool tonal_function_named = false;
    bool cadence_class_established = false;
    bool relation_established = false;
    std::vector<harmonic_span_evidence> evidence;
};

constexpr double harmonic_span_strong_conflict_threshold = 0.80;
constexpr double harmonic_span_strong_conflict_ceiling = 0.49;

inline const char* to_string(harmonic_span_relation_kind kind) noexcept {
    switch (kind) {
    case harmonic_span_relation_kind::unresolved: return "unresolved";
    case harmonic_span_relation_kind::prolongation_candidate:
        return "prolongation_candidate";
    case harmonic_span_relation_kind::delayed_resolution_candidate:
        return "delayed_resolution_candidate";
    }
    return "unknown";
}

inline const char* to_string(harmonic_span_evidence_kind kind) noexcept {
    switch (kind) {
    case harmonic_span_evidence_kind::cross_span_voice_leading:
        return "cross_span_voice_leading";
    case harmonic_span_evidence_kind::persistent_part_continuity:
        return "persistent_part_continuity";
    case harmonic_span_evidence_kind::retained_tonal_center:
        return "retained_tonal_center";
    case harmonic_span_evidence_kind::unresolved_process:
        return "unresolved_process";
    case harmonic_span_evidence_kind::later_structural_arrival:
        return "later_structural_arrival";
    case harmonic_span_evidence_kind::authored_relation:
        return "authored_relation";
    case harmonic_span_evidence_kind::contradiction:
        return "contradiction";
    }
    return "unknown";
}

inline const char* to_string(harmonic_span_evidence_origin origin) noexcept {
    switch (origin) {
    case harmonic_span_evidence_origin::voice_leading: return "voice_leading";
    case harmonic_span_evidence_origin::persistent_part: return "persistent_part";
    case harmonic_span_evidence_origin::tonal_region: return "tonal_region";
    case harmonic_span_evidence_origin::phrase_structure: return "phrase_structure";
    case harmonic_span_evidence_origin::cadential_arrival: return "cadential_arrival";
    case harmonic_span_evidence_origin::authored_source: return "authored_source";
    case harmonic_span_evidence_origin::external_annotation:
        return "external_annotation";
    }
    return "unknown";
}

inline const char* to_string(harmonic_span_evidence_polarity polarity) noexcept {
    return polarity == harmonic_span_evidence_polarity::supports
        ? "supports"
        : "counters";
}

inline bool compatible_harmonic_span_time_basis(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
        first.tick_rate == second.tick_rate &&
        first.loop_iteration == second.loop_iteration;
}

inline bool finite_harmonic_span(const time_span& scope) noexcept {
    return scope.end.has_value() &&
        compatible_harmonic_span_time_basis(scope.start, *scope.end) &&
        scope.end->tick > scope.start.tick;
}

inline std::vector<node_id> unique_harmonic_span_nodes(
    std::vector<node_id> nodes) {
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    return nodes;
}

inline void validate_harmonic_span_evidence(
    const harmonic_span_evidence& evidence) {
    if (!std::isfinite(evidence.confidence) ||
        evidence.confidence < 0.0 || evidence.confidence > 1.0) {
        throw std::invalid_argument(
            "harmonic-span evidence confidence must be finite and in [0, 1]");
    }
    if (evidence.source.empty())
        throw std::invalid_argument("harmonic-span evidence requires a source");
    if (evidence.support_nodes.empty())
        throw std::invalid_argument(
            "harmonic-span evidence requires provenance support nodes");
}

inline harmonic_span_evidence make_harmonic_span_evidence(
    harmonic_span_evidence_kind kind,
    harmonic_span_evidence_origin origin,
    harmonic_span_evidence_polarity polarity,
    double confidence,
    std::string source,
    std::string detail,
    std::vector<node_id> support_nodes,
    evidence_status status = evidence_status::hypothesis) {
    harmonic_span_evidence result;
    result.kind = kind;
    result.origin = origin;
    result.polarity = polarity;
    result.status = status;
    result.confidence = confidence;
    result.source = std::move(source);
    result.detail = std::move(detail);
    result.support_nodes = unique_harmonic_span_nodes(std::move(support_nodes));
    validate_harmonic_span_evidence(result);
    return result;
}

inline harmonic_span_evidence make_voice_leading_harmonic_span_evidence(
    const voice_leading_hypothesis& voice_leading,
    const time_span& scope,
    std::vector<node_id> support_nodes,
    std::string source = "long-range-voice-leading-analysis") {
    if (!finite_harmonic_span(scope))
        throw std::invalid_argument(
            "long-range voice-leading evidence requires a finite harmonic span");
    if (voice_leading.first_time != scope.start ||
        voice_leading.second_time != *scope.end) {
        throw std::invalid_argument(
            "long-range voice leading must connect the harmonic-span endpoints");
    }
    if (voice_leading.motions.empty() ||
        voice_leading.identity_preserved_voices == 0) {
        throw std::invalid_argument(
            "long-range voice leading requires at least one persistent-identity correspondence");
    }
    return make_harmonic_span_evidence(
        harmonic_span_evidence_kind::cross_span_voice_leading,
        harmonic_span_evidence_origin::voice_leading,
        harmonic_span_evidence_polarity::supports,
        voice_leading.confidence,
        std::move(source),
        "persistent-voice correspondence connects non-adjacent harmonic endpoints",
        std::move(support_nodes));
}

inline harmonic_span_evidence make_retained_center_harmonic_span_evidence(
    const tonal_region_relation_hypothesis& relation,
    const time_span& scope,
    std::vector<node_id> support_nodes,
    std::string source = "retained-tonal-center-analysis") {
    if (!finite_harmonic_span(scope))
        throw std::invalid_argument(
            "retained-center evidence requires a finite harmonic span");
    if (relation.kind != tonal_region_relation_kind::retained_center ||
        !relation.centers_equivalent) {
        throw std::invalid_argument(
            "prolongational retained-center evidence requires an equivalent retained tonal center");
    }
    if (!finite_valid_tonal_region(relation.source_region) ||
        !finite_valid_tonal_region(relation.target_region) ||
        !compatible_harmonic_span_time_basis(
            relation.source_region.start, scope.start) ||
        relation.source_region.start.tick > scope.start.tick ||
        relation.target_region.end->tick < scope.end->tick) {
        throw std::invalid_argument(
            "retained tonal regions must bracket the proposed harmonic span");
    }
    return make_harmonic_span_evidence(
        harmonic_span_evidence_kind::retained_tonal_center,
        harmonic_span_evidence_origin::tonal_region,
        harmonic_span_evidence_polarity::supports,
        relation.confidence,
        std::move(source),
        "independently inferred tonal regions retain an equivalent center across the span",
        std::move(support_nodes));
}

inline harmonic_span_evidence make_unresolved_process_harmonic_span_evidence(
    const phrase_role_evidence& continuation,
    const time_span& scope,
    std::vector<node_id> support_nodes = {},
    std::string source = "unresolved-process-analysis") {
    if (!finite_harmonic_span(scope))
        throw std::invalid_argument(
            "unresolved-process evidence requires a finite harmonic span");
    validate_phrase_role_evidence(continuation);
    if (continuation.role != phrase_role_kind::continuation ||
        continuation.polarity != phrase_role_evidence_polarity::supports ||
        !phrase_role_scope_overlaps(continuation.scope, scope)) {
        throw std::invalid_argument(
            "delayed resolution requires positive continuation evidence inside the harmonic span");
    }
    if (support_nodes.empty())
        support_nodes = continuation.support_nodes;
    return make_harmonic_span_evidence(
        harmonic_span_evidence_kind::unresolved_process,
        harmonic_span_evidence_origin::phrase_structure,
        harmonic_span_evidence_polarity::supports,
        continuation.confidence,
        std::move(source),
        "independent phrase evidence shows the earlier harmonic process remains active",
        std::move(support_nodes),
        continuation.status);
}

inline harmonic_span_evidence make_later_arrival_harmonic_span_evidence(
    const cadential_arrival_hypothesis& arrival,
    const time_span& scope,
    std::vector<node_id> support_nodes,
    std::string source = "later-structural-arrival-analysis") {
    if (!finite_harmonic_span(scope))
        throw std::invalid_argument(
            "later-arrival evidence requires a finite harmonic span");
    if (arrival.arrival_time != *scope.end)
        throw std::invalid_argument(
            "later structural arrival must terminate the proposed harmonic span");
    if (!arrival.cross_part_phrase_grounded ||
        !arrival.harmonic_root_motion_reliable ||
        arrival.confidence <= 0.0) {
        throw std::invalid_argument(
            "later structural arrival requires grounded phrase and reliable harmonic evidence");
    }
    return make_harmonic_span_evidence(
        harmonic_span_evidence_kind::later_structural_arrival,
        harmonic_span_evidence_origin::cadential_arrival,
        harmonic_span_evidence_polarity::supports,
        arrival.confidence,
        std::move(source),
        "a later independently grounded structural arrival terminates the observed span without naming tonal function",
        std::move(support_nodes));
}

inline double harmonic_span_independent_support_ceiling(
    const std::map<harmonic_span_evidence_origin, double>& support) {
    if (support.empty())
        return 0.0;
    std::vector<double> strengths;
    strengths.reserve(support.size());
    for (const auto& item : support)
        strengths.push_back(item.second);
    std::sort(strengths.begin(), strengths.end(), std::greater<double>{});
    return strengths.size() >= 2 ? strengths[1] : strengths[0];
}

inline harmonic_span_relation_hypothesis make_harmonic_span_relation_hypothesis(
    const musical_execution_graph& graph,
    harmonic_span_relation_kind kind,
    const std::vector<harmonic_transition_hypothesis>& transitions,
    std::vector<node_id> surface_nodes,
    std::vector<harmonic_span_evidence> evidence,
    double proposed_confidence = 0.95) {
    if (kind == harmonic_span_relation_kind::unresolved)
        throw std::invalid_argument(
            "harmonic-span constructor requires a candidate relation kind");
    if (!std::isfinite(proposed_confidence) ||
        proposed_confidence < 0.0 || proposed_confidence > 1.0) {
        throw std::invalid_argument(
            "harmonic-span proposed confidence must be finite and in [0, 1]");
    }
    if (transitions.size() < 2)
        throw std::invalid_argument(
            "harmonic-span relation requires at least one intervening harmonic event");
    if (surface_nodes.size() != transitions.size() + 1)
        throw std::invalid_argument(
            "harmonic-span surface nodes must materialize every transition endpoint");

    const time_span scope{
        transitions.front().first_time,
        transitions.back().second_time,
    };
    if (!finite_harmonic_span(scope))
        throw std::invalid_argument(
            "harmonic-span transitions require a finite ordered time basis");

    double surface_confidence = 1.0;
    std::vector<time_coordinate> expected_times;
    expected_times.reserve(surface_nodes.size());
    expected_times.push_back(transitions.front().first_time);

    for (std::size_t index = 0; index < transitions.size(); ++index) {
        const auto& current = transitions[index];
        if (!std::isfinite(current.confidence) ||
            current.confidence < 0.0 || current.confidence > 1.0) {
            throw std::invalid_argument(
                "harmonic-span transition confidence must be finite and in [0, 1]");
        }
        if (!compatible_harmonic_span_time_basis(scope.start, current.first_time) ||
            !compatible_harmonic_span_time_basis(scope.start, current.second_time) ||
            current.second_time.tick <= current.first_time.tick) {
            throw std::invalid_argument(
                "harmonic-span transitions must be ordered on one compatible time basis");
        }
        if (index > 0) {
            const auto& previous = transitions[index - 1];
            if (previous.second_time != current.first_time ||
                previous.second_root_pitch_class != current.first_root_pitch_class ||
                previous.second_quality != current.first_quality) {
                throw std::invalid_argument(
                    "harmonic-span surface chain must preserve every contiguous local transition");
            }
        }
        surface_confidence = std::min(surface_confidence, current.confidence);
        expected_times.push_back(current.second_time);
    }

    surface_nodes = unique_harmonic_span_nodes(std::move(surface_nodes));
    if (surface_nodes.size() != expected_times.size())
        throw std::invalid_argument(
            "harmonic-span surface event identities must be distinct");

    // Preserve chronological node order after uniqueness validation.
    std::vector<node_id> ordered_surface_nodes;
    ordered_surface_nodes.reserve(expected_times.size());
    for (const auto& expected : expected_times) {
        node_id match = 0;
        for (node_id id : surface_nodes) {
            const node* value = graph.find_node(id);
            if (value != nullptr && value->active.has_value() &&
                value->active->start == expected) {
                if (match != 0)
                    throw std::invalid_argument(
                        "harmonic-span surface time maps to multiple supplied nodes");
                match = id;
            }
        }
        if (match == 0)
            throw std::invalid_argument(
                "harmonic-span surface node is missing or does not match its transition time");
        ordered_surface_nodes.push_back(match);
    }

    std::map<harmonic_span_evidence_origin, double> support_by_origin;
    bool continuity = false;
    bool retained_anchor = false;
    bool unresolved_process = false;
    bool later_arrival = false;
    bool strong_conflict = false;

    for (auto& item : evidence) {
        validate_harmonic_span_evidence(item);
        for (node_id support_node : item.support_nodes) {
            if (graph.find_node(support_node) == nullptr)
                throw std::invalid_argument(
                    "harmonic-span evidence references an unknown support node");
        }

        if (item.polarity == harmonic_span_evidence_polarity::supports) {
            support_by_origin[item.origin] = std::max(
                support_by_origin[item.origin],
                item.confidence);
            continuity = continuity ||
                item.kind == harmonic_span_evidence_kind::cross_span_voice_leading ||
                item.kind == harmonic_span_evidence_kind::persistent_part_continuity;
            retained_anchor = retained_anchor ||
                item.kind == harmonic_span_evidence_kind::retained_tonal_center ||
                item.kind == harmonic_span_evidence_kind::authored_relation;
            unresolved_process = unresolved_process ||
                item.kind == harmonic_span_evidence_kind::unresolved_process;
            later_arrival = later_arrival ||
                item.kind == harmonic_span_evidence_kind::later_structural_arrival;
        } else if (item.confidence >= harmonic_span_strong_conflict_threshold) {
            strong_conflict = true;
        }
    }

    if (support_by_origin.size() < 2)
        throw std::invalid_argument(
            "harmonic-span relation requires support from at least two independent evidence domains");

    if (kind == harmonic_span_relation_kind::prolongation_candidate) {
        if (!continuity || !retained_anchor)
            throw std::invalid_argument(
                "prolongation requires both cross-span continuity and an independently retained structural anchor");
    } else if (!continuity || !unresolved_process || !later_arrival) {
        throw std::invalid_argument(
            "delayed resolution requires continuity, an unresolved earlier process, and a later structural arrival");
    }

    harmonic_span_relation_hypothesis result;
    result.kind = kind;
    result.scope = scope;
    result.surface_nodes = std::move(ordered_surface_nodes);
    result.transition_count = transitions.size();
    result.intervening_event_count = result.surface_nodes.size() - 2;
    result.support_domains = support_by_origin.size();
    result.surface_chain_confidence = surface_confidence;
    result.independent_support_ceiling =
        harmonic_span_independent_support_ceiling(support_by_origin);
    result.proposed_confidence = proposed_confidence;
    result.surface_events_preserved = true;
    result.cross_domain_grounded = true;
    result.strong_conflict_present = strong_conflict;
    result.tonal_function_named = false;
    result.cadence_class_established = false;
    result.relation_established = false;
    result.evidence = std::move(evidence);
    result.confidence = std::min({
        proposed_confidence,
        surface_confidence,
        result.independent_support_ceiling,
    });
    if (strong_conflict)
        result.confidence = std::min(
            result.confidence,
            harmonic_span_strong_conflict_ceiling);
    return result;
}

inline phrase_role_evidence project_harmonic_span_phrase_role_evidence(
    const harmonic_span_relation_hypothesis& relation,
    phrase_role_formal_scale formal_scale,
    std::string source = "harmonic-span-phrase-role-projection") {
    if (relation.kind == harmonic_span_relation_kind::unresolved ||
        !relation.surface_events_preserved ||
        !relation.cross_domain_grounded) {
        throw std::invalid_argument(
            "phrase-role projection requires a grounded harmonic-span candidate");
    }
    if (source.empty())
        throw std::invalid_argument(
            "harmonic-span phrase-role projection requires a source");

    phrase_role_evidence result;
    result.role = relation.kind == harmonic_span_relation_kind::prolongation_candidate
        ? phrase_role_kind::prolongation
        : phrase_role_kind::delayed_resolution;
    result.scope = relation.scope;
    result.formal_scale = formal_scale;
    result.origin = phrase_role_evidence_origin::harmonic_dependency;
    result.polarity = phrase_role_evidence_polarity::supports;
    result.status = evidence_status::hypothesis;
    result.confidence = relation.confidence;
    result.source = std::move(source);
    result.detail =
        std::string{"non-adjacent harmonic dependency preserves "} +
        std::to_string(relation.intervening_event_count) +
        " intervening surface event(s): " + to_string(relation.kind);

    result.support_nodes = relation.surface_nodes;
    for (const auto& item : relation.evidence) {
        if (item.polarity != harmonic_span_evidence_polarity::supports)
            continue;
        result.support_nodes.insert(
            result.support_nodes.end(),
            item.support_nodes.begin(),
            item.support_nodes.end());
    }
    result.support_nodes = unique_harmonic_span_nodes(
        std::move(result.support_nodes));
    return result;
}

inline node_id add_harmonic_span_relation_hypothesis(
    musical_execution_graph& graph,
    const harmonic_span_relation_hypothesis& relation) {
    if (!finite_harmonic_span(relation.scope) ||
        relation.kind == harmonic_span_relation_kind::unresolved ||
        relation.surface_nodes.size() < 3 ||
        !relation.surface_events_preserved) {
        throw std::invalid_argument(
            "materialized harmonic-span relation requires a valid non-adjacent candidate");
    }

    node value;
    value.kind = node_kind::musical_relation;
    value.layer = semantic_layer::musical_structure;
    value.flow = flow_kind::stream;
    value.label = "harmonic span relation hypothesis";
    value.active = relation.scope;
    value.attributes.push_back({
        "identity_scope",
        std::string{"harmonic_span_relation_hypothesis"},
        evidence_status::hypothesis,
        relation.confidence,
        "",
    });
    value.attributes.push_back({
        "harmonic_span_relation_kind",
        std::string{to_string(relation.kind)},
        evidence_status::hypothesis,
        relation.confidence,
        "",
    });
    value.attributes.push_back({
        "surface_events_preserved",
        relation.surface_events_preserved,
        evidence_status::derived,
        1.0,
        "",
    });
    value.attributes.push_back({
        "intervening_event_count",
        static_cast<std::uint64_t>(relation.intervening_event_count),
        evidence_status::derived,
        1.0,
        "events",
    });
    value.attributes.push_back({
        "cross_domain_grounded",
        relation.cross_domain_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    value.attributes.push_back({
        "strong_conflict_present",
        relation.strong_conflict_present,
        evidence_status::derived,
        1.0,
        "",
    });
    value.attributes.push_back({
        "tonal_function_named",
        relation.tonal_function_named,
        evidence_status::derived,
        1.0,
        "",
    });
    value.attributes.push_back({
        "cadence_class_established",
        relation.cadence_class_established,
        evidence_status::derived,
        1.0,
        "",
    });
    value.attributes.push_back({
        "relation_established",
        relation.relation_established,
        evidence_status::derived,
        1.0,
        "",
    });

    for (const auto& item : relation.evidence) {
        value.provenance.push_back({
            item.status,
            item.confidence,
            item.source,
            std::nullopt,
            std::string{to_string(item.polarity)} + " " +
                to_string(item.kind) + ": " + item.detail,
        });
    }

    const node_id relation_id = graph.add_node(std::move(value));

    for (node_id surface_node : relation.surface_nodes) {
        if (graph.find_node(surface_node) == nullptr)
            throw std::invalid_argument(
                "harmonic-span relation references an unknown surface node");
        edge support;
        support.kind = edge_kind::derived_from;
        support.from = surface_node;
        support.to = relation_id;
        support.attributes.push_back({
            "support_role",
            std::string{"surface_harmonic_event"},
            evidence_status::derived,
            1.0,
            "",
        });
        graph.add_edge(std::move(support));
    }

    for (const auto& item : relation.evidence) {
        for (node_id support_node : item.support_nodes) {
            if (graph.find_node(support_node) == nullptr)
                throw std::invalid_argument(
                    "harmonic-span relation references an unknown evidence node");
            edge support;
            support.kind = edge_kind::derived_from;
            support.from = support_node;
            support.to = relation_id;
            support.attributes.push_back({
                "evidence_kind",
                std::string{to_string(item.kind)},
                item.status,
                item.confidence,
                "",
            });
            support.attributes.push_back({
                "evidence_polarity",
                std::string{to_string(item.polarity)},
                item.status,
                item.confidence,
                "",
            });
            graph.add_edge(std::move(support));
        }
    }
    return relation_id;
}

} // namespace vgmtooling::model
