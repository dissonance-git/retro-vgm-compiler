#pragma once

#include "musical_execution_graph.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

// Phrase role is deliberately independent of cadence class. These are syntax
// hypotheses about what a bounded musical region does at a declared formal
// scale, not names inferred from local harmonic morphology.
enum class phrase_role_kind : std::uint8_t {
    ending = 0,
    continuation,
    new_phrase_onset,
    reroute,
    return_role,
    prolongation,
    delayed_resolution,
    nested_local_close_inside_global_continuation,
};

enum class phrase_role_formal_scale : std::uint8_t {
    local_phrase = 0,
    phrase_group,
    section,
    whole_work,
};

enum class phrase_role_evidence_origin : std::uint8_t {
    persistent_part_continuation = 0,
    motif_analysis,
    harmonic_process,
    phrase_boundary_analysis,
    recurrence_analysis,
    authored_source,
    external_annotation,
    performance_reonset,
    harmonic_dependency,
};

enum class phrase_role_evidence_polarity : std::uint8_t {
    supports = 0,
    counters,
};

struct phrase_role_evidence {
    phrase_role_kind role = phrase_role_kind::continuation;
    time_span scope{};
    phrase_role_formal_scale formal_scale = phrase_role_formal_scale::local_phrase;
    phrase_role_evidence_origin origin =
        phrase_role_evidence_origin::persistent_part_continuation;
    phrase_role_evidence_polarity polarity =
        phrase_role_evidence_polarity::supports;
    evidence_status status = evidence_status::hypothesis;
    double confidence = 0.0;
    std::string source;
    std::string detail;
    std::vector<node_id> support_nodes;
};

struct phrase_role_hypothesis {
    phrase_role_kind role = phrase_role_kind::continuation;
    time_span scope{};
    phrase_role_formal_scale formal_scale = phrase_role_formal_scale::local_phrase;
    double proposed_confidence = 0.0;
    double confidence = 0.0;
    double independent_support_ceiling = 0.0;
    std::size_t supporting_observations = 0;
    std::size_t counter_observations = 0;
    std::size_t support_domains = 0;
    bool cross_domain_grounded = false;
    bool strong_conflict_present = false;
    bool role_established = false;
    std::vector<phrase_role_kind> incompatible_alternatives;
    std::vector<phrase_role_evidence> evidence;
};

struct phrase_role_evidence_set {
    std::vector<phrase_role_hypothesis> alternatives;
    bool incompatible_alternatives_preserved = false;
    bool cross_scale_coexistence_preserved = false;
};

constexpr double phrase_role_no_support_ceiling = 0.25;
constexpr double phrase_role_single_domain_ceiling = 0.69;
constexpr double phrase_role_strong_conflict_ceiling = 0.49;

inline const char* to_string(phrase_role_kind kind) noexcept {
    switch (kind) {
    case phrase_role_kind::ending: return "ending";
    case phrase_role_kind::continuation: return "continuation";
    case phrase_role_kind::new_phrase_onset: return "new_phrase_onset";
    case phrase_role_kind::reroute: return "reroute";
    case phrase_role_kind::return_role: return "return";
    case phrase_role_kind::prolongation: return "prolongation";
    case phrase_role_kind::delayed_resolution: return "delayed_resolution";
    case phrase_role_kind::nested_local_close_inside_global_continuation:
        return "nested_local_close_inside_global_continuation";
    }
    return "unknown";
}

inline const char* to_string(phrase_role_formal_scale scale) noexcept {
    switch (scale) {
    case phrase_role_formal_scale::local_phrase: return "local_phrase";
    case phrase_role_formal_scale::phrase_group: return "phrase_group";
    case phrase_role_formal_scale::section: return "section";
    case phrase_role_formal_scale::whole_work: return "whole_work";
    }
    return "unknown";
}

inline const char* to_string(phrase_role_evidence_origin origin) noexcept {
    switch (origin) {
    case phrase_role_evidence_origin::persistent_part_continuation:
        return "persistent_part_continuation";
    case phrase_role_evidence_origin::motif_analysis: return "motif_analysis";
    case phrase_role_evidence_origin::harmonic_process: return "harmonic_process";
    case phrase_role_evidence_origin::phrase_boundary_analysis:
        return "phrase_boundary_analysis";
    case phrase_role_evidence_origin::recurrence_analysis: return "recurrence_analysis";
    case phrase_role_evidence_origin::authored_source: return "authored_source";
    case phrase_role_evidence_origin::external_annotation: return "external_annotation";
    case phrase_role_evidence_origin::performance_reonset: return "performance_reonset";
    case phrase_role_evidence_origin::harmonic_dependency: return "harmonic_dependency";
    }
    return "unknown";
}

inline const char* to_string(phrase_role_evidence_polarity polarity) noexcept {
    return polarity == phrase_role_evidence_polarity::supports ? "supports" : "counters";
}

inline bool compatible_phrase_role_time_basis(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
        first.tick_rate == second.tick_rate &&
        first.loop_iteration == second.loop_iteration;
}

inline void validate_phrase_role_scope(const time_span& scope) {
    if (!scope.end.has_value())
        return;
    if (!compatible_phrase_role_time_basis(scope.start, *scope.end))
        throw std::invalid_argument("phrase-role scope must use one compatible time basis");
    if (scope.end->tick < scope.start.tick)
        throw std::invalid_argument("phrase-role scope end cannot precede its start");
}

inline bool same_phrase_role_scope(const time_span& first, const time_span& second) noexcept {
    return first.start == second.start && first.end == second.end;
}

inline bool phrase_role_scope_overlaps(const time_span& first, const time_span& second) noexcept {
    if (!compatible_phrase_role_time_basis(first.start, second.start))
        return false;
    const auto first_end = first.end.has_value() ? first.end->tick : first.start.tick;
    const auto second_end = second.end.has_value() ? second.end->tick : second.start.tick;
    return first.start.tick <= second_end && second.start.tick <= first_end;
}

inline void validate_phrase_role_evidence(const phrase_role_evidence& item) {
    validate_phrase_role_scope(item.scope);
    if (item.confidence < 0.0 || item.confidence > 1.0)
        throw std::invalid_argument("phrase-role evidence confidence must be in [0, 1]");
    if (item.source.empty())
        throw std::invalid_argument("phrase-role evidence requires a source");
}

inline double phrase_role_independent_support_ceiling(
    const std::map<phrase_role_evidence_origin, double>& support) {
    if (support.empty())
        return 0.0;
    std::vector<double> strengths;
    strengths.reserve(support.size());
    for (const auto& item : support)
        strengths.push_back(item.second);
    std::sort(strengths.begin(), strengths.end(), std::greater<double>{});
    return strengths.size() >= 2 ? strengths[1] : strengths[0];
}

inline phrase_role_hypothesis make_phrase_role_hypothesis(
    phrase_role_kind role,
    time_span scope,
    phrase_role_formal_scale formal_scale,
    double proposed_confidence,
    std::vector<phrase_role_evidence> evidence,
    std::vector<phrase_role_kind> incompatible_alternatives = {}) {
    validate_phrase_role_scope(scope);
    if (proposed_confidence < 0.0 || proposed_confidence > 1.0)
        throw std::invalid_argument("phrase-role confidence must be in [0, 1]");
    if (evidence.empty())
        throw std::invalid_argument("phrase-role hypothesis requires evidence");

    std::map<phrase_role_evidence_origin, double> supporting_domains;
    std::size_t support_count = 0;
    std::size_t counter_count = 0;
    bool strong_conflict = false;

    for (const auto& item : evidence) {
        validate_phrase_role_evidence(item);
        if (item.role != role)
            throw std::invalid_argument("phrase-role evidence cannot silently support a different role");
        if (!phrase_role_scope_overlaps(scope, item.scope))
            throw std::invalid_argument("phrase-role evidence must overlap the role scope");
        if (item.polarity == phrase_role_evidence_polarity::supports) {
            ++support_count;
            supporting_domains[item.origin] = std::max(
                supporting_domains[item.origin],
                item.confidence);
        } else {
            ++counter_count;
            if (item.confidence >= 0.80)
                strong_conflict = true;
        }
    }

    std::sort(incompatible_alternatives.begin(), incompatible_alternatives.end());
    incompatible_alternatives.erase(
        std::unique(incompatible_alternatives.begin(), incompatible_alternatives.end()),
        incompatible_alternatives.end());
    if (std::find(
            incompatible_alternatives.begin(),
            incompatible_alternatives.end(),
            role) != incompatible_alternatives.end()) {
        throw std::invalid_argument("phrase role cannot be incompatible with itself");
    }

    phrase_role_hypothesis result;
    result.role = role;
    result.scope = scope;
    result.formal_scale = formal_scale;
    result.proposed_confidence = proposed_confidence;
    result.supporting_observations = support_count;
    result.counter_observations = counter_count;
    result.support_domains = supporting_domains.size();
    result.cross_domain_grounded = supporting_domains.size() >= 2;
    result.strong_conflict_present = strong_conflict;
    result.independent_support_ceiling =
        phrase_role_independent_support_ceiling(supporting_domains);
    result.incompatible_alternatives = std::move(incompatible_alternatives);
    result.evidence = std::move(evidence);

    double confidence = proposed_confidence;
    if (support_count == 0) {
        confidence = std::min(confidence, phrase_role_no_support_ceiling);
    } else {
        confidence = std::min(confidence, result.independent_support_ceiling);
        if (!result.cross_domain_grounded)
            confidence = std::min(confidence, phrase_role_single_domain_ceiling);
    }
    if (strong_conflict)
        confidence = std::min(confidence, phrase_role_strong_conflict_ceiling);
    result.confidence = confidence;

    // Evidence objects describe candidates. Establishment belongs to a later
    // arbitration layer with style- and scale-appropriate discriminators.
    result.role_established = false;
    return result;
}

inline bool phrase_role_lists_incompatible(
    const phrase_role_hypothesis& hypothesis,
    phrase_role_kind alternative) noexcept {
    return std::find(
        hypothesis.incompatible_alternatives.begin(),
        hypothesis.incompatible_alternatives.end(),
        alternative) != hypothesis.incompatible_alternatives.end();
}

inline phrase_role_evidence_set make_phrase_role_evidence_set(
    std::vector<phrase_role_hypothesis> alternatives) {
    if (alternatives.empty())
        throw std::invalid_argument("phrase-role evidence set requires at least one candidate");

    phrase_role_evidence_set result;
    result.alternatives = std::move(alternatives);

    for (std::size_t first_index = 0; first_index < result.alternatives.size(); ++first_index) {
        const auto& first = result.alternatives[first_index];
        for (std::size_t second_index = first_index + 1;
             second_index < result.alternatives.size();
             ++second_index) {
            const auto& second = result.alternatives[second_index];
            if (!phrase_role_scope_overlaps(first.scope, second.scope))
                continue;

            if (first.formal_scale != second.formal_scale) {
                result.cross_scale_coexistence_preserved = true;
                continue;
            }

            const bool first_marks_second =
                phrase_role_lists_incompatible(first, second.role);
            const bool second_marks_first =
                phrase_role_lists_incompatible(second, first.role);
            if (first_marks_second != second_marks_first) {
                throw std::invalid_argument(
                    "incompatible phrase-role alternatives must be declared symmetrically");
            }
            result.incompatible_alternatives_preserved =
                result.incompatible_alternatives_preserved || first_marks_second;
        }
    }
    return result;
}

inline node_id add_phrase_role_hypothesis(
    musical_execution_graph& graph,
    const phrase_role_hypothesis& hypothesis) {
    for (const auto& item : hypothesis.evidence) {
        for (node_id support_node : item.support_nodes) {
            if (graph.find_node(support_node) == nullptr)
                throw std::invalid_argument("phrase-role evidence references an unknown node");
        }
    }

    node value;
    value.kind = node_kind::musical_relation;
    value.layer = semantic_layer::musical_structure;
    value.flow = flow_kind::stream;
    value.label = "phrase role hypothesis";
    value.active = hypothesis.scope;
    value.attributes.push_back({
        "identity_scope",
        std::string{"phrase_role_hypothesis"},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    value.attributes.push_back({
        "phrase_role_kind",
        std::string{to_string(hypothesis.role)},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    value.attributes.push_back({
        "formal_scale",
        std::string{to_string(hypothesis.formal_scale)},
        evidence_status::derived,
        1.0,
        "",
    });
    value.attributes.push_back({
        "cross_domain_grounded",
        hypothesis.cross_domain_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    value.attributes.push_back({
        "strong_conflict_present",
        hypothesis.strong_conflict_present,
        evidence_status::derived,
        1.0,
        "",
    });
    value.attributes.push_back({
        "role_established",
        hypothesis.role_established,
        evidence_status::derived,
        1.0,
        "",
    });
    for (phrase_role_kind alternative : hypothesis.incompatible_alternatives) {
        value.attributes.push_back({
            "incompatible_phrase_role",
            std::string{to_string(alternative)},
            evidence_status::hypothesis,
            hypothesis.confidence,
            "",
        });
    }
    for (const auto& item : hypothesis.evidence) {
        value.provenance.push_back({
            item.status,
            item.confidence,
            item.source,
            std::nullopt,
            std::string{to_string(item.polarity)} + " " +
                to_string(item.origin) + " for " + to_string(item.role) + ": " + item.detail,
        });
    }

    const node_id role_id = graph.add_node(std::move(value));
    for (const auto& item : hypothesis.evidence) {
        for (node_id support_node : item.support_nodes) {
            edge support;
            support.kind = edge_kind::derived_from;
            support.from = support_node;
            support.to = role_id;
            support.attributes.push_back({
                "phrase_role",
                std::string{to_string(item.role)},
                item.status,
                item.confidence,
                "",
            });
            support.attributes.push_back({
                "evidence_origin",
                std::string{to_string(item.origin)},
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
            support.provenance.push_back({
                item.status,
                item.confidence,
                item.source,
                std::nullopt,
                item.detail,
            });
            graph.add_edge(std::move(support));
        }
    }
    return role_id;
}

} // namespace vgmtooling::model
