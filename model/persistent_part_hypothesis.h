#pragma once

#include "musical_execution_graph.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

// Persistent-part identity is intentionally separate from physical voice,
// authored driver track, and acoustic stream identity. It is the musical
// hypothesis that multiple bounded observations belong to one continuing part
// such as a bass line, melody line, accompaniment strand, or percussion role.
enum class persistent_part_evidence_kind : std::uint8_t {
    physical_slot_continuity = 0,
    source_identity,
    instrument_program_identity,
    pitch_trajectory_continuity,
    temporal_adjacency,
    articulation_continuity,
    rhythmic_role_continuity,
    authored_track_identity,
    driver_track_identity,
    external_identity,
    simultaneous_conflict,
    identity_discontinuity,
};

enum class persistent_part_evidence_origin : std::uint8_t {
    synthesis_runtime = 0,
    driver_execution,
    authored_program,
    musical_analysis,
    external_annotation,
};

enum class persistent_part_evidence_polarity : std::uint8_t {
    supports = 0,
    counters,
};

struct persistent_part_evidence {
    persistent_part_evidence_kind kind =
        persistent_part_evidence_kind::physical_slot_continuity;
    persistent_part_evidence_origin origin =
        persistent_part_evidence_origin::synthesis_runtime;
    persistent_part_evidence_polarity polarity =
        persistent_part_evidence_polarity::supports;
    evidence_status status = evidence_status::derived;
    double confidence = 0.0;
    std::string source;
    std::string detail;
    std::vector<node_id> support_nodes;
};

struct persistent_part_hypothesis {
    evidence_status status = evidence_status::hypothesis;
    double proposed_confidence = 0.0;
    double confidence = 0.0;
    bool identity_bearing_support = false;
    bool cross_domain_grounded = false;
    bool documentary_grounded = false;
    bool strong_conflict_present = false;
    std::vector<node_id> subject_nodes;
    std::vector<persistent_part_evidence> evidence;
};

// These are epistemic ceilings, not calibrated probabilities.
constexpr double persistent_part_slot_only_confidence_ceiling = 0.35;
constexpr double persistent_part_no_identity_confidence_ceiling = 0.64;
constexpr double persistent_part_single_domain_confidence_ceiling = 0.74;
constexpr double persistent_part_strong_conflict_confidence_ceiling = 0.49;

inline const char* to_string(persistent_part_evidence_kind kind) noexcept {
    switch (kind) {
    case persistent_part_evidence_kind::physical_slot_continuity:
        return "physical_slot_continuity";
    case persistent_part_evidence_kind::source_identity:
        return "source_identity";
    case persistent_part_evidence_kind::instrument_program_identity:
        return "instrument_program_identity";
    case persistent_part_evidence_kind::pitch_trajectory_continuity:
        return "pitch_trajectory_continuity";
    case persistent_part_evidence_kind::temporal_adjacency:
        return "temporal_adjacency";
    case persistent_part_evidence_kind::articulation_continuity:
        return "articulation_continuity";
    case persistent_part_evidence_kind::rhythmic_role_continuity:
        return "rhythmic_role_continuity";
    case persistent_part_evidence_kind::authored_track_identity:
        return "authored_track_identity";
    case persistent_part_evidence_kind::driver_track_identity:
        return "driver_track_identity";
    case persistent_part_evidence_kind::external_identity:
        return "external_identity";
    case persistent_part_evidence_kind::simultaneous_conflict:
        return "simultaneous_conflict";
    case persistent_part_evidence_kind::identity_discontinuity:
        return "identity_discontinuity";
    }
    return "unknown";
}

inline const char* to_string(persistent_part_evidence_origin origin) noexcept {
    switch (origin) {
    case persistent_part_evidence_origin::synthesis_runtime:
        return "synthesis_runtime";
    case persistent_part_evidence_origin::driver_execution:
        return "driver_execution";
    case persistent_part_evidence_origin::authored_program:
        return "authored_program";
    case persistent_part_evidence_origin::musical_analysis:
        return "musical_analysis";
    case persistent_part_evidence_origin::external_annotation:
        return "external_annotation";
    }
    return "unknown";
}

inline const char* to_string(persistent_part_evidence_polarity polarity) noexcept {
    switch (polarity) {
    case persistent_part_evidence_polarity::supports:
        return "supports";
    case persistent_part_evidence_polarity::counters:
        return "counters";
    }
    return "unknown";
}

inline bool is_identity_bearing_part_evidence(
    persistent_part_evidence_kind kind) noexcept {
    return kind == persistent_part_evidence_kind::source_identity ||
           kind == persistent_part_evidence_kind::instrument_program_identity ||
           kind == persistent_part_evidence_kind::authored_track_identity ||
           kind == persistent_part_evidence_kind::driver_track_identity ||
           kind == persistent_part_evidence_kind::external_identity;
}

inline bool is_documentary_part_evidence(
    const persistent_part_evidence& evidence) noexcept {
    return evidence.origin == persistent_part_evidence_origin::external_annotation ||
           evidence.kind == persistent_part_evidence_kind::authored_track_identity ||
           evidence.kind == persistent_part_evidence_kind::driver_track_identity ||
           evidence.kind == persistent_part_evidence_kind::external_identity;
}

inline std::uint8_t persistent_part_domain(
    persistent_part_evidence_kind kind) noexcept {
    switch (kind) {
    case persistent_part_evidence_kind::physical_slot_continuity:
        return 0;
    case persistent_part_evidence_kind::source_identity:
    case persistent_part_evidence_kind::instrument_program_identity:
    case persistent_part_evidence_kind::authored_track_identity:
    case persistent_part_evidence_kind::driver_track_identity:
    case persistent_part_evidence_kind::external_identity:
        return 1;
    case persistent_part_evidence_kind::pitch_trajectory_continuity:
        return 2;
    case persistent_part_evidence_kind::temporal_adjacency:
        return 3;
    case persistent_part_evidence_kind::articulation_continuity:
    case persistent_part_evidence_kind::rhythmic_role_continuity:
        return 4;
    case persistent_part_evidence_kind::simultaneous_conflict:
    case persistent_part_evidence_kind::identity_discontinuity:
        return 5;
    }
    return 5;
}

inline void validate_persistent_part_evidence(
    const persistent_part_evidence& evidence) {
    if (evidence.confidence < 0.0 || evidence.confidence > 1.0)
        throw std::invalid_argument("persistent-part evidence confidence must be in [0, 1]");
    if (evidence.source.empty())
        throw std::invalid_argument("persistent-part evidence requires a non-empty source");
}

inline persistent_part_hypothesis make_persistent_part_hypothesis(
    double proposed_confidence,
    std::vector<node_id> subject_nodes,
    std::vector<persistent_part_evidence> evidence) {
    if (proposed_confidence < 0.0 || proposed_confidence > 1.0)
        throw std::invalid_argument("persistent-part confidence must be in [0, 1]");

    std::set<node_id> unique_subjects(subject_nodes.begin(), subject_nodes.end());
    if (unique_subjects.size() < 2)
        throw std::invalid_argument("persistent part requires at least two distinct subject observations");
    if (evidence.empty())
        throw std::invalid_argument("persistent part requires evidence");

    bool has_support = false;
    bool identity_support = false;
    bool documentary_support = false;
    bool slot_support = false;
    bool non_slot_support = false;
    bool strong_conflict = false;
    std::set<std::uint8_t> support_domains;

    for (const auto& item : evidence) {
        validate_persistent_part_evidence(item);
        if (item.polarity == persistent_part_evidence_polarity::supports) {
            has_support = true;
            support_domains.insert(persistent_part_domain(item.kind));
            if (item.kind == persistent_part_evidence_kind::physical_slot_continuity)
                slot_support = true;
            else
                non_slot_support = true;
            if (is_identity_bearing_part_evidence(item.kind))
                identity_support = true;
            if (is_documentary_part_evidence(item))
                documentary_support = true;
        } else if (
            item.confidence >= 0.80 &&
            (item.kind == persistent_part_evidence_kind::simultaneous_conflict ||
             item.kind == persistent_part_evidence_kind::identity_discontinuity)) {
            strong_conflict = true;
        }
    }

    if (!has_support)
        throw std::invalid_argument("persistent part requires supporting evidence");

    persistent_part_hypothesis result;
    result.proposed_confidence = proposed_confidence;
    result.identity_bearing_support = identity_support;
    result.cross_domain_grounded = support_domains.size() >= 2;
    result.documentary_grounded = documentary_support;
    result.strong_conflict_present = strong_conflict;
    result.subject_nodes = std::move(subject_nodes);
    result.evidence = std::move(evidence);

    double confidence = proposed_confidence;
    if (slot_support && !non_slot_support)
        confidence = std::min(confidence, persistent_part_slot_only_confidence_ceiling);
    else if (!identity_support)
        confidence = std::min(confidence, persistent_part_no_identity_confidence_ceiling);
    else if (!result.cross_domain_grounded && !documentary_support)
        confidence = std::min(confidence, persistent_part_single_domain_confidence_ceiling);

    if (strong_conflict && !documentary_support)
        confidence = std::min(confidence, persistent_part_strong_conflict_confidence_ceiling);

    result.confidence = confidence;
    return result;
}

inline std::optional<time_span> persistent_part_subject_span(
    const musical_execution_graph& graph,
    const std::vector<node_id>& subjects) {
    std::optional<time_coordinate> earliest{};
    std::optional<time_coordinate> latest{};
    time_domain domain = time_domain::source;
    bool domain_set = false;

    for (node_id subject_id : subjects) {
        const node* subject = graph.find_node(subject_id);
        if (subject == nullptr || !subject->active.has_value())
            return std::nullopt;
        const auto& active = *subject->active;
        if (!domain_set) {
            domain = active.start.domain;
            domain_set = true;
        }
        if (active.start.domain != domain ||
            (active.end.has_value() && active.end->domain != domain))
            return std::nullopt;

        if (!earliest.has_value() || active.start.tick < earliest->tick)
            earliest = active.start;
        const time_coordinate end = active.end.value_or(active.start);
        if (!latest.has_value() || end.tick > latest->tick)
            latest = end;
    }

    if (!earliest.has_value() || !latest.has_value())
        return std::nullopt;
    return time_span{*earliest, *latest};
}

inline node_id add_persistent_part_hypothesis(
    musical_execution_graph& graph,
    const persistent_part_hypothesis& hypothesis) {
    for (node_id subject_id : hypothesis.subject_nodes) {
        if (graph.find_node(subject_id) == nullptr)
            throw std::invalid_argument("persistent-part subject references an unknown node");
    }

    node part;
    part.kind = node_kind::part;
    part.layer = semantic_layer::musical_performance;
    part.flow = flow_kind::stream;
    part.label = "persistent musical part hypothesis";
    part.active = persistent_part_subject_span(graph, hypothesis.subject_nodes);
    part.attributes.push_back({
        "identity_scope",
        std::string{"persistent_musical_part"},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    part.attributes.push_back({
        "identity_bearing_support",
        hypothesis.identity_bearing_support,
        evidence_status::derived,
        1.0,
        "",
    });
    part.attributes.push_back({
        "cross_domain_grounded",
        hypothesis.cross_domain_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    part.attributes.push_back({
        "documentary_grounded",
        hypothesis.documentary_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    part.attributes.push_back({
        "strong_conflict_present",
        hypothesis.strong_conflict_present,
        evidence_status::derived,
        1.0,
        "",
    });
    part.attributes.push_back({
        "proposed_confidence",
        hypothesis.proposed_confidence,
        evidence_status::hypothesis,
        hypothesis.proposed_confidence,
        "normalized",
    });

    for (const auto& item : hypothesis.evidence) {
        provenance_flags flags = to_flags(provenance_flag::none);
        if (item.origin == persistent_part_evidence_origin::synthesis_runtime)
            flags = flags | provenance_flag::runtime_capture;
        if (item.origin == persistent_part_evidence_origin::external_annotation)
            flags = flags | provenance_flag::external_annotation;
        part.provenance.push_back({
            item.status,
            item.confidence,
            item.source,
            std::nullopt,
            std::string{to_string(item.polarity)} + " " + to_string(item.kind) + ": " + item.detail,
            flags,
        });
    }

    const node_id part_id = graph.add_node(std::move(part));

    for (node_id subject_id : hypothesis.subject_nodes) {
        edge membership;
        membership.kind = edge_kind::groups_into;
        membership.from = subject_id;
        membership.to = part_id;
        membership.attributes.push_back({
            "relation_role",
            std::string{"part_subject"},
            evidence_status::hypothesis,
            hypothesis.confidence,
            "",
        });
        graph.add_edge(std::move(membership));
    }

    for (const auto& item : hypothesis.evidence) {
        provenance_flags flags = to_flags(provenance_flag::none);
        if (item.origin == persistent_part_evidence_origin::synthesis_runtime)
            flags = flags | provenance_flag::runtime_capture;
        if (item.origin == persistent_part_evidence_origin::external_annotation)
            flags = flags | provenance_flag::external_annotation;

        for (node_id support_node : item.support_nodes) {
            if (graph.find_node(support_node) == nullptr)
                throw std::invalid_argument("persistent-part evidence references an unknown node");
            edge support;
            support.kind = edge_kind::derived_from;
            support.from = support_node;
            support.to = part_id;
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
                flags,
            });
            graph.add_edge(std::move(support));
        }
    }

    return part_id;
}

} // namespace vgmtooling::model
