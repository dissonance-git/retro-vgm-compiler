#pragma once

#include "musical_execution_graph.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

enum class phrase_boundary_evidence_kind : std::uint8_t {
    temporal_gap = 0,
    motif_completion,
    repeated_motif_alignment,
    part_density_change,
    bass_harmonic_change,
    cadence_or_resolution,
    authored_boundary,
    driver_loop_boundary,
    external_annotation,
    cross_boundary_continuity,
};

enum class phrase_boundary_evidence_origin : std::uint8_t {
    performance_timing = 0,
    motif_analysis,
    harmonic_analysis,
    authored_source,
    driver_execution,
    external_annotation,
};

enum class phrase_boundary_evidence_polarity : std::uint8_t {
    supports = 0,
    counters,
};

struct phrase_boundary_evidence {
    phrase_boundary_evidence_kind kind = phrase_boundary_evidence_kind::temporal_gap;
    phrase_boundary_evidence_origin origin = phrase_boundary_evidence_origin::performance_timing;
    phrase_boundary_evidence_polarity polarity = phrase_boundary_evidence_polarity::supports;
    evidence_status status = evidence_status::hypothesis;
    double confidence = 0.0;
    std::string source;
    std::string detail;
    std::vector<node_id> support_nodes;
};

struct phrase_boundary_hypothesis {
    time_coordinate boundary{};
    double proposed_confidence = 0.0;
    double confidence = 0.0;
    double independent_support_ceiling = 0.0;
    std::size_t supporting_observations = 0;
    std::size_t counter_observations = 0;
    std::size_t support_domains = 0;
    bool structural_support = false;
    bool timing_only = false;
    bool cross_domain_grounded = false;
    bool authored_grounded = false;
    bool strong_conflict_present = false;
    std::vector<phrase_boundary_evidence> evidence;
};

constexpr double phrase_boundary_no_support_ceiling = 0.25;
constexpr double phrase_boundary_timing_only_ceiling = 0.45;
constexpr double phrase_boundary_nonstructural_ceiling = 0.60;
constexpr double phrase_boundary_single_domain_ceiling = 0.69;
constexpr double phrase_boundary_strong_conflict_ceiling = 0.49;

inline bool phrase_boundary_kind_is_structural(
    phrase_boundary_evidence_kind kind) noexcept {
    switch (kind) {
    case phrase_boundary_evidence_kind::motif_completion:
    case phrase_boundary_evidence_kind::repeated_motif_alignment:
    case phrase_boundary_evidence_kind::bass_harmonic_change:
    case phrase_boundary_evidence_kind::cadence_or_resolution:
    case phrase_boundary_evidence_kind::authored_boundary:
    case phrase_boundary_evidence_kind::external_annotation:
        return true;
    case phrase_boundary_evidence_kind::temporal_gap:
    case phrase_boundary_evidence_kind::part_density_change:
    case phrase_boundary_evidence_kind::driver_loop_boundary:
    case phrase_boundary_evidence_kind::cross_boundary_continuity:
        return false;
    }
    return false;
}

inline const char* to_string(phrase_boundary_evidence_kind kind) noexcept {
    switch (kind) {
    case phrase_boundary_evidence_kind::temporal_gap:
        return "temporal_gap";
    case phrase_boundary_evidence_kind::motif_completion:
        return "motif_completion";
    case phrase_boundary_evidence_kind::repeated_motif_alignment:
        return "repeated_motif_alignment";
    case phrase_boundary_evidence_kind::part_density_change:
        return "part_density_change";
    case phrase_boundary_evidence_kind::bass_harmonic_change:
        return "bass_harmonic_change";
    case phrase_boundary_evidence_kind::cadence_or_resolution:
        return "cadence_or_resolution";
    case phrase_boundary_evidence_kind::authored_boundary:
        return "authored_boundary";
    case phrase_boundary_evidence_kind::driver_loop_boundary:
        return "driver_loop_boundary";
    case phrase_boundary_evidence_kind::external_annotation:
        return "external_annotation";
    case phrase_boundary_evidence_kind::cross_boundary_continuity:
        return "cross_boundary_continuity";
    }
    return "unknown";
}

inline const char* to_string(phrase_boundary_evidence_polarity polarity) noexcept {
    return polarity == phrase_boundary_evidence_polarity::supports ? "supports" : "counters";
}

inline void validate_phrase_boundary_evidence(const phrase_boundary_evidence& item) {
    if (item.confidence < 0.0 || item.confidence > 1.0)
        throw std::invalid_argument("phrase-boundary evidence confidence must be in [0, 1]");
    if (item.source.empty())
        throw std::invalid_argument("phrase-boundary evidence requires a source");
}

inline double phrase_boundary_independent_support_ceiling(
    const std::map<phrase_boundary_evidence_origin, double>& support) {
    if (support.empty())
        return 0.0;
    std::vector<double> strengths;
    strengths.reserve(support.size());
    for (const auto& item : support)
        strengths.push_back(item.second);
    std::sort(strengths.begin(), strengths.end(), std::greater<double>{});
    return strengths.size() >= 2 ? strengths[1] : strengths[0];
}

inline phrase_boundary_hypothesis make_phrase_boundary_hypothesis(
    time_coordinate boundary,
    double proposed_confidence,
    std::vector<phrase_boundary_evidence> evidence) {
    if (proposed_confidence < 0.0 || proposed_confidence > 1.0)
        throw std::invalid_argument("phrase-boundary confidence must be in [0, 1]");
    if (evidence.empty())
        throw std::invalid_argument("phrase-boundary hypothesis requires evidence");

    std::map<phrase_boundary_evidence_origin, double> supporting_domains;
    std::size_t support_count = 0;
    std::size_t counter_count = 0;
    bool structural_support = false;
    bool timing_only = true;
    bool authored_grounded = false;
    bool strong_conflict = false;

    for (const auto& item : evidence) {
        validate_phrase_boundary_evidence(item);
        if (item.polarity == phrase_boundary_evidence_polarity::supports) {
            ++support_count;
            supporting_domains[item.origin] = std::max(
                supporting_domains[item.origin],
                item.confidence);
            structural_support = structural_support || phrase_boundary_kind_is_structural(item.kind);
            timing_only = timing_only && item.kind == phrase_boundary_evidence_kind::temporal_gap;
            if (item.kind == phrase_boundary_evidence_kind::authored_boundary &&
                item.status == evidence_status::exact && item.confidence >= 0.90) {
                authored_grounded = true;
            }
        } else {
            ++counter_count;
            if (item.confidence >= 0.80)
                strong_conflict = true;
        }
    }

    phrase_boundary_hypothesis result;
    result.boundary = boundary;
    result.proposed_confidence = proposed_confidence;
    result.supporting_observations = support_count;
    result.counter_observations = counter_count;
    result.support_domains = supporting_domains.size();
    result.structural_support = structural_support;
    result.timing_only = support_count > 0 && timing_only;
    result.cross_domain_grounded = supporting_domains.size() >= 2;
    result.authored_grounded = authored_grounded;
    result.strong_conflict_present = strong_conflict;
    result.independent_support_ceiling = phrase_boundary_independent_support_ceiling(supporting_domains);
    result.evidence = std::move(evidence);

    double confidence = proposed_confidence;
    if (support_count == 0) {
        confidence = std::min(confidence, phrase_boundary_no_support_ceiling);
    } else {
        confidence = std::min(confidence, result.independent_support_ceiling);
        if (result.timing_only)
            confidence = std::min(confidence, phrase_boundary_timing_only_ceiling);
        else if (!structural_support)
            confidence = std::min(confidence, phrase_boundary_nonstructural_ceiling);
        else if (!result.cross_domain_grounded && !authored_grounded)
            confidence = std::min(confidence, phrase_boundary_single_domain_ceiling);
    }

    if (strong_conflict && !authored_grounded)
        confidence = std::min(confidence, phrase_boundary_strong_conflict_ceiling);

    result.confidence = confidence;
    return result;
}

inline node_id add_phrase_boundary_hypothesis(
    musical_execution_graph& graph,
    const phrase_boundary_hypothesis& hypothesis) {
    for (const auto& item : hypothesis.evidence) {
        for (node_id support_node : item.support_nodes) {
            if (graph.find_node(support_node) == nullptr)
                throw std::invalid_argument("phrase-boundary evidence references an unknown node");
        }
    }

    node boundary;
    boundary.kind = node_kind::musical_relation;
    boundary.layer = semantic_layer::musical_structure;
    boundary.flow = flow_kind::event;
    boundary.label = "phrase boundary hypothesis";
    boundary.active = time_span{hypothesis.boundary, std::nullopt};
    boundary.attributes.push_back({
        "identity_scope",
        std::string{"phrase_boundary_hypothesis"},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    boundary.attributes.push_back({
        "structural_support",
        hypothesis.structural_support,
        evidence_status::derived,
        1.0,
        "",
    });
    boundary.attributes.push_back({
        "cross_domain_grounded",
        hypothesis.cross_domain_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    boundary.attributes.push_back({
        "authored_grounded",
        hypothesis.authored_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    boundary.attributes.push_back({
        "strong_conflict_present",
        hypothesis.strong_conflict_present,
        evidence_status::derived,
        1.0,
        "",
    });

    for (const auto& item : hypothesis.evidence) {
        boundary.provenance.push_back({
            item.status,
            item.confidence,
            item.source,
            std::nullopt,
            std::string{to_string(item.polarity)} + " " + to_string(item.kind) + ": " + item.detail,
        });
    }

    const node_id boundary_id = graph.add_node(std::move(boundary));
    for (const auto& item : hypothesis.evidence) {
        for (node_id support_node : item.support_nodes) {
            edge support;
            support.kind = edge_kind::derived_from;
            support.from = support_node;
            support.to = boundary_id;
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
    return boundary_id;
}

} // namespace vgmtooling::model
