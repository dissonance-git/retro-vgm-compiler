#pragma once

#include "musical_execution_graph.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

// Ludic-function claims describe what music appears to do in the game, rather
// than only what the music contains. They therefore live above musical
// structure in musicological_context and must preserve the evidence route that
// supports them.
enum class ludic_function_kind : std::uint8_t {
    game_state_signal = 0,
    affordance_cue,
    place_identity,
    character_identity,
    narrative_frame,
    tension_regulation,
    continuity_bridge,
    action_coupling,
    transition_management,
    reward_feedback,
    failure_reset,
    menu_meta,
};

// Evidence origin is intentionally separate from evidence_status. An exact
// musical observation can still be insufficient to establish an exact claim
// about gameplay function.
enum class ludic_evidence_origin : std::uint8_t {
    musical_intrinsic = 0,
    sequence_or_engine,
    soundtrack_relational,
    runtime_game_context,
    external_annotation,
};

enum class ludic_evidence_polarity : std::uint8_t {
    supports = 0,
    counters,
};

struct ludic_function_evidence {
    ludic_evidence_origin origin = ludic_evidence_origin::musical_intrinsic;
    ludic_evidence_polarity polarity = ludic_evidence_polarity::supports;
    evidence_status status = evidence_status::hypothesis;
    double confidence = 0.0;
    std::string source;
    std::string detail;
    std::vector<node_id> support_nodes;
};

struct ludic_function_hypothesis {
    ludic_function_kind function = ludic_function_kind::game_state_signal;

    // This remains a hypothesis even when its supporting observations are
    // exact. Direct documentary facts should be represented separately as
    // exact/derived analysis features and may then support this inference.
    evidence_status status = evidence_status::hypothesis;

    // Confidence supplied by the inference system before the evidence-origin
    // guardrail is applied.
    double proposed_confidence = 0.0;

    // Confidence after epistemic policy. Structure-only evidence is allowed to
    // suggest a function, but not to become a strong gameplay-context claim.
    double confidence = 0.0;
    bool context_grounded = false;
    std::vector<ludic_function_evidence> evidence;
};

constexpr double ludic_intrinsic_only_confidence_ceiling = 0.64;

inline const char* to_string(ludic_function_kind kind) noexcept {
    switch (kind) {
    case ludic_function_kind::game_state_signal:
        return "game_state_signal";
    case ludic_function_kind::affordance_cue:
        return "affordance_cue";
    case ludic_function_kind::place_identity:
        return "place_identity";
    case ludic_function_kind::character_identity:
        return "character_identity";
    case ludic_function_kind::narrative_frame:
        return "narrative_frame";
    case ludic_function_kind::tension_regulation:
        return "tension_regulation";
    case ludic_function_kind::continuity_bridge:
        return "continuity_bridge";
    case ludic_function_kind::action_coupling:
        return "action_coupling";
    case ludic_function_kind::transition_management:
        return "transition_management";
    case ludic_function_kind::reward_feedback:
        return "reward_feedback";
    case ludic_function_kind::failure_reset:
        return "failure_reset";
    case ludic_function_kind::menu_meta:
        return "menu_meta";
    }
    return "unknown";
}

inline const char* to_string(ludic_evidence_origin origin) noexcept {
    switch (origin) {
    case ludic_evidence_origin::musical_intrinsic:
        return "musical_intrinsic";
    case ludic_evidence_origin::sequence_or_engine:
        return "sequence_or_engine";
    case ludic_evidence_origin::soundtrack_relational:
        return "soundtrack_relational";
    case ludic_evidence_origin::runtime_game_context:
        return "runtime_game_context";
    case ludic_evidence_origin::external_annotation:
        return "external_annotation";
    }
    return "unknown";
}

inline const char* to_string(ludic_evidence_polarity polarity) noexcept {
    switch (polarity) {
    case ludic_evidence_polarity::supports:
        return "supports";
    case ludic_evidence_polarity::counters:
        return "counters";
    }
    return "unknown";
}

inline bool is_contextual_ludic_evidence(ludic_evidence_origin origin) noexcept {
    return origin == ludic_evidence_origin::runtime_game_context ||
           origin == ludic_evidence_origin::external_annotation;
}

inline void validate_ludic_function_evidence(const ludic_function_evidence& evidence) {
    if (evidence.confidence < 0.0 || evidence.confidence > 1.0)
        throw std::invalid_argument("ludic evidence confidence must be in [0, 1]");
    if (evidence.source.empty())
        throw std::invalid_argument("ludic evidence requires a non-empty source");
}

inline ludic_function_hypothesis make_ludic_function_hypothesis(
    ludic_function_kind function,
    double proposed_confidence,
    std::vector<ludic_function_evidence> evidence) {
    if (proposed_confidence < 0.0 || proposed_confidence > 1.0)
        throw std::invalid_argument("ludic hypothesis confidence must be in [0, 1]");

    bool has_support = false;
    bool context_grounded = false;
    for (const auto& item : evidence) {
        validate_ludic_function_evidence(item);
        if (item.polarity != ludic_evidence_polarity::supports)
            continue;
        has_support = true;
        if (is_contextual_ludic_evidence(item.origin))
            context_grounded = true;
    }

    if (!has_support)
        throw std::invalid_argument("ludic hypothesis requires supporting evidence");

    ludic_function_hypothesis result;
    result.function = function;
    result.proposed_confidence = proposed_confidence;
    result.context_grounded = context_grounded;
    result.confidence = context_grounded
        ? proposed_confidence
        : std::min(proposed_confidence, ludic_intrinsic_only_confidence_ceiling);
    result.evidence = std::move(evidence);
    return result;
}

// Materialize a ludic-function hypothesis in the existing execution graph.
// Support and counterevidence are kept as separate derived_from edges with an
// explicit polarity attribute rather than being flattened into one score.
inline node_id add_ludic_function_hypothesis(
    musical_execution_graph& graph,
    const ludic_function_hypothesis& hypothesis) {
    node relation;
    relation.kind = node_kind::musical_relation;
    relation.layer = semantic_layer::musicological_context;
    relation.flow = flow_kind::value;
    relation.label = std::string{"ludic function: "} + to_string(hypothesis.function);
    relation.attributes.push_back({
        "ludic_function",
        std::string{to_string(hypothesis.function)},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    relation.attributes.push_back({
        "context_grounded",
        hypothesis.context_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    relation.attributes.push_back({
        "proposed_confidence",
        hypothesis.proposed_confidence,
        evidence_status::hypothesis,
        hypothesis.proposed_confidence,
        "normalized",
    });

    for (const auto& item : hypothesis.evidence) {
        provenance_flags flags = to_flags(provenance_flag::none);
        if (item.origin == ludic_evidence_origin::runtime_game_context)
            flags = flags | provenance_flag::runtime_capture;
        if (item.origin == ludic_evidence_origin::external_annotation)
            flags = flags | provenance_flag::external_annotation;

        relation.provenance.push_back({
            item.status,
            item.confidence,
            item.source,
            std::nullopt,
            std::string{to_string(item.origin)} + ": " + item.detail,
            flags,
        });
    }

    const node_id relation_id = graph.add_node(std::move(relation));

    for (const auto& item : hypothesis.evidence) {
        provenance_flags flags = to_flags(provenance_flag::none);
        if (item.origin == ludic_evidence_origin::runtime_game_context)
            flags = flags | provenance_flag::runtime_capture;
        if (item.origin == ludic_evidence_origin::external_annotation)
            flags = flags | provenance_flag::external_annotation;

        for (node_id support_node : item.support_nodes) {
            edge support;
            support.kind = edge_kind::derived_from;
            support.from = support_node;
            support.to = relation_id;
            support.attributes.push_back({
                "evidence_polarity",
                std::string{to_string(item.polarity)},
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
