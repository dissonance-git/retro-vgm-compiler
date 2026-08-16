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

// Relations between cues are the bridge between one-track musical analysis and
// soundtrack-level interpretation. Domain-specific relations may live in
// musical_structure; broader identity/family claims live in
// musicological_context.
enum class soundtrack_relation_kind : std::uint8_t {
    motif_recurrence = 0,
    harmonic_fingerprint,
    rhythmic_fingerprint,
    orchestration_fingerprint,
    formal_parallel,
    arrangement_relation,
    cue_family,
};

enum class soundtrack_evidence_domain : std::uint8_t {
    melody = 0,
    harmony,
    rhythm,
    form,
    orchestration,
    timbre,
    engine_identity,
};

enum class soundtrack_evidence_origin : std::uint8_t {
    musical_analysis = 0,
    sequence_or_engine,
    external_annotation,
};

enum class musical_transformation_kind : std::uint8_t {
    transposition = 0,
    register_shift,
    rhythmic_augmentation,
    rhythmic_diminution,
    tempo_shift,
    reharmonization,
    orchestration_remap,
    truncation_or_extension,
    metric_reinterpretation,
};

struct soundtrack_relation_evidence {
    soundtrack_evidence_domain domain = soundtrack_evidence_domain::melody;
    soundtrack_evidence_origin origin = soundtrack_evidence_origin::musical_analysis;
    evidence_status status = evidence_status::derived;
    double confidence = 0.0;
    std::string source;
    std::string detail;
    std::optional<musical_transformation_kind> transformation{};
    std::vector<node_id> support_nodes;
};

struct soundtrack_relation_hypothesis {
    soundtrack_relation_kind relation = soundtrack_relation_kind::motif_recurrence;
    evidence_status status = evidence_status::hypothesis;
    double proposed_confidence = 0.0;
    double confidence = 0.0;
    bool cross_domain_grounded = false;
    std::vector<node_id> subject_nodes;
    std::vector<soundtrack_relation_evidence> evidence;
};

constexpr double soundtrack_single_domain_context_ceiling = 0.74;

inline const char* to_string(soundtrack_relation_kind kind) noexcept {
    switch (kind) {
    case soundtrack_relation_kind::motif_recurrence:
        return "motif_recurrence";
    case soundtrack_relation_kind::harmonic_fingerprint:
        return "harmonic_fingerprint";
    case soundtrack_relation_kind::rhythmic_fingerprint:
        return "rhythmic_fingerprint";
    case soundtrack_relation_kind::orchestration_fingerprint:
        return "orchestration_fingerprint";
    case soundtrack_relation_kind::formal_parallel:
        return "formal_parallel";
    case soundtrack_relation_kind::arrangement_relation:
        return "arrangement_relation";
    case soundtrack_relation_kind::cue_family:
        return "cue_family";
    }
    return "unknown";
}

inline const char* to_string(soundtrack_evidence_domain domain) noexcept {
    switch (domain) {
    case soundtrack_evidence_domain::melody:
        return "melody";
    case soundtrack_evidence_domain::harmony:
        return "harmony";
    case soundtrack_evidence_domain::rhythm:
        return "rhythm";
    case soundtrack_evidence_domain::form:
        return "form";
    case soundtrack_evidence_domain::orchestration:
        return "orchestration";
    case soundtrack_evidence_domain::timbre:
        return "timbre";
    case soundtrack_evidence_domain::engine_identity:
        return "engine_identity";
    }
    return "unknown";
}

inline const char* to_string(soundtrack_evidence_origin origin) noexcept {
    switch (origin) {
    case soundtrack_evidence_origin::musical_analysis:
        return "musical_analysis";
    case soundtrack_evidence_origin::sequence_or_engine:
        return "sequence_or_engine";
    case soundtrack_evidence_origin::external_annotation:
        return "external_annotation";
    }
    return "unknown";
}

inline const char* to_string(musical_transformation_kind transform) noexcept {
    switch (transform) {
    case musical_transformation_kind::transposition:
        return "transposition";
    case musical_transformation_kind::register_shift:
        return "register_shift";
    case musical_transformation_kind::rhythmic_augmentation:
        return "rhythmic_augmentation";
    case musical_transformation_kind::rhythmic_diminution:
        return "rhythmic_diminution";
    case musical_transformation_kind::tempo_shift:
        return "tempo_shift";
    case musical_transformation_kind::reharmonization:
        return "reharmonization";
    case musical_transformation_kind::orchestration_remap:
        return "orchestration_remap";
    case musical_transformation_kind::truncation_or_extension:
        return "truncation_or_extension";
    case musical_transformation_kind::metric_reinterpretation:
        return "metric_reinterpretation";
    }
    return "unknown";
}

inline semantic_layer soundtrack_relation_layer(soundtrack_relation_kind kind) noexcept {
    switch (kind) {
    case soundtrack_relation_kind::motif_recurrence:
    case soundtrack_relation_kind::harmonic_fingerprint:
    case soundtrack_relation_kind::rhythmic_fingerprint:
    case soundtrack_relation_kind::orchestration_fingerprint:
    case soundtrack_relation_kind::formal_parallel:
        return semantic_layer::musical_structure;
    case soundtrack_relation_kind::arrangement_relation:
    case soundtrack_relation_kind::cue_family:
        return semantic_layer::musicological_context;
    }
    return semantic_layer::musicological_context;
}

inline bool soundtrack_relation_requires_cross_domain_support(
    soundtrack_relation_kind kind) noexcept {
    return kind == soundtrack_relation_kind::arrangement_relation ||
           kind == soundtrack_relation_kind::cue_family;
}

inline void validate_soundtrack_relation_evidence(const soundtrack_relation_evidence& evidence) {
    if (evidence.confidence < 0.0 || evidence.confidence > 1.0)
        throw std::invalid_argument("soundtrack relation evidence confidence must be in [0, 1]");
    if (evidence.source.empty())
        throw std::invalid_argument("soundtrack relation evidence requires a non-empty source");
}

inline soundtrack_relation_hypothesis make_soundtrack_relation_hypothesis(
    soundtrack_relation_kind relation,
    double proposed_confidence,
    std::vector<node_id> subject_nodes,
    std::vector<soundtrack_relation_evidence> evidence) {
    if (proposed_confidence < 0.0 || proposed_confidence > 1.0)
        throw std::invalid_argument("soundtrack relation confidence must be in [0, 1]");

    std::set<node_id> unique_subjects(subject_nodes.begin(), subject_nodes.end());
    if (unique_subjects.size() < 2)
        throw std::invalid_argument("soundtrack relation requires at least two distinct subjects");

    std::set<soundtrack_evidence_domain> domains;
    bool has_external_annotation = false;
    for (const auto& item : evidence) {
        validate_soundtrack_relation_evidence(item);
        domains.insert(item.domain);
        if (item.origin == soundtrack_evidence_origin::external_annotation)
            has_external_annotation = true;
    }
    if (evidence.empty())
        throw std::invalid_argument("soundtrack relation requires evidence");

    const bool cross_domain_grounded = domains.size() >= 2 || has_external_annotation;
    const bool needs_cross_domain = soundtrack_relation_requires_cross_domain_support(relation);

    soundtrack_relation_hypothesis result;
    result.relation = relation;
    result.status = soundtrack_relation_layer(relation) == semantic_layer::musical_structure
        ? evidence_status::derived
        : evidence_status::hypothesis;
    result.proposed_confidence = proposed_confidence;
    result.cross_domain_grounded = cross_domain_grounded;
    result.confidence = needs_cross_domain && !cross_domain_grounded
        ? std::min(proposed_confidence, soundtrack_single_domain_context_ceiling)
        : proposed_confidence;
    result.subject_nodes = std::move(subject_nodes);
    result.evidence = std::move(evidence);
    return result;
}

inline node_id add_soundtrack_relation_hypothesis(
    musical_execution_graph& graph,
    const soundtrack_relation_hypothesis& hypothesis) {
    node relation;
    relation.kind = node_kind::musical_relation;
    relation.layer = soundtrack_relation_layer(hypothesis.relation);
    relation.flow = flow_kind::value;
    relation.label = std::string{"soundtrack relation: "} + to_string(hypothesis.relation);
    relation.attributes.push_back({
        "soundtrack_relation",
        std::string{to_string(hypothesis.relation)},
        hypothesis.status,
        hypothesis.confidence,
        "",
    });
    relation.attributes.push_back({
        "cross_domain_grounded",
        hypothesis.cross_domain_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    relation.attributes.push_back({
        "proposed_confidence",
        hypothesis.proposed_confidence,
        hypothesis.status,
        hypothesis.proposed_confidence,
        "normalized",
    });

    std::size_t transform_index = 0;
    for (const auto& item : hypothesis.evidence) {
        provenance_flags flags = to_flags(provenance_flag::none);
        if (item.origin == soundtrack_evidence_origin::external_annotation)
            flags = flags | provenance_flag::external_annotation;

        relation.provenance.push_back({
            item.status,
            item.confidence,
            item.source,
            std::nullopt,
            std::string{to_string(item.domain)} + "/" + to_string(item.origin) + ": " + item.detail,
            flags,
        });

        if (item.transformation.has_value()) {
            relation.attributes.push_back({
                "transformation_" + std::to_string(transform_index++),
                std::string{to_string(*item.transformation)},
                item.status,
                item.confidence,
                "",
            });
        }
    }

    const node_id relation_id = graph.add_node(std::move(relation));

    for (node_id subject : hypothesis.subject_nodes) {
        edge subject_edge;
        subject_edge.kind = edge_kind::derived_from;
        subject_edge.from = subject;
        subject_edge.to = relation_id;
        subject_edge.attributes.push_back({
            "relation_role",
            std::string{"subject"},
            hypothesis.status,
            hypothesis.confidence,
            "",
        });
        graph.add_edge(std::move(subject_edge));
    }

    for (const auto& item : hypothesis.evidence) {
        provenance_flags flags = to_flags(provenance_flag::none);
        if (item.origin == soundtrack_evidence_origin::external_annotation)
            flags = flags | provenance_flag::external_annotation;

        for (node_id support_node : item.support_nodes) {
            edge support;
            support.kind = edge_kind::derived_from;
            support.from = support_node;
            support.to = relation_id;
            support.attributes.push_back({
                "relation_role",
                std::string{"evidence"},
                item.status,
                item.confidence,
                "",
            });
            support.attributes.push_back({
                "evidence_domain",
                std::string{to_string(item.domain)},
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

    return relation_id;
}

} // namespace vgmtooling::model
