#pragma once

#include "musical_execution_graph.h"

#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

enum class execution_semantic_kind : std::uint8_t {
    authored_command = 0,
    driver_command,
    driver_state,
    articulation_control,
    pitch_control,
    timing_control,
    modulation_control,
    instrument_reference,
    sample_reference,
    phrase_control,
    channel_mode,
    implementation_quirk,
    information_gap,
};

enum class execution_semantic_origin : std::uint8_t {
    source_native = 0,
    runtime_observed,
    deterministic_reconstruction,
    external_document,
    hypothesis,
    unavailable,
};

enum class execution_semantic_gap_kind : std::uint8_t {
    source_representation_absent = 0,
    driver_definition_absent,
    transformed_capture,
    timing_ambiguous,
    sample_provenance_unknown,
    implementation_quirk_unknown,
    intent_underdetermined,
};

struct execution_semantic_observation {
    semantic_layer layer = semantic_layer::driver_execution;
    execution_semantic_kind kind = execution_semantic_kind::driver_state;
    execution_semantic_origin origin = execution_semantic_origin::runtime_observed;
    evidence_status status = evidence_status::exact;
    double confidence = 1.0;
    std::string source;
    std::string native_token;
    std::string detail;
    std::optional<time_span> active{};
    provenance_flags flags = to_flags(provenance_flag::none);
};

struct execution_semantic_gap {
    semantic_layer layer = semantic_layer::driver_execution;
    execution_semantic_gap_kind kind = execution_semantic_gap_kind::intent_underdetermined;
    std::string source;
    std::string detail;
    std::optional<time_span> active{};
    provenance_flags flags = to_flags(provenance_flag::none);
};

inline bool is_execution_semantic_ancestry_layer(semantic_layer layer) noexcept {
    return layer == semantic_layer::source_representation ||
           layer == semantic_layer::authored_program ||
           layer == semantic_layer::driver_execution;
}

inline const char* execution_semantic_kind_name(execution_semantic_kind kind) noexcept {
    switch (kind) {
    case execution_semantic_kind::authored_command: return "authored_command";
    case execution_semantic_kind::driver_command: return "driver_command";
    case execution_semantic_kind::driver_state: return "driver_state";
    case execution_semantic_kind::articulation_control: return "articulation_control";
    case execution_semantic_kind::pitch_control: return "pitch_control";
    case execution_semantic_kind::timing_control: return "timing_control";
    case execution_semantic_kind::modulation_control: return "modulation_control";
    case execution_semantic_kind::instrument_reference: return "instrument_reference";
    case execution_semantic_kind::sample_reference: return "sample_reference";
    case execution_semantic_kind::phrase_control: return "phrase_control";
    case execution_semantic_kind::channel_mode: return "channel_mode";
    case execution_semantic_kind::implementation_quirk: return "implementation_quirk";
    case execution_semantic_kind::information_gap: return "information_gap";
    }
    return "unknown";
}

inline const char* execution_semantic_origin_name(execution_semantic_origin origin) noexcept {
    switch (origin) {
    case execution_semantic_origin::source_native: return "source_native";
    case execution_semantic_origin::runtime_observed: return "runtime_observed";
    case execution_semantic_origin::deterministic_reconstruction: return "deterministic_reconstruction";
    case execution_semantic_origin::external_document: return "external_document";
    case execution_semantic_origin::hypothesis: return "hypothesis";
    case execution_semantic_origin::unavailable: return "unavailable";
    }
    return "unknown";
}

inline const char* execution_semantic_gap_kind_name(execution_semantic_gap_kind kind) noexcept {
    switch (kind) {
    case execution_semantic_gap_kind::source_representation_absent: return "source_representation_absent";
    case execution_semantic_gap_kind::driver_definition_absent: return "driver_definition_absent";
    case execution_semantic_gap_kind::transformed_capture: return "transformed_capture";
    case execution_semantic_gap_kind::timing_ambiguous: return "timing_ambiguous";
    case execution_semantic_gap_kind::sample_provenance_unknown: return "sample_provenance_unknown";
    case execution_semantic_gap_kind::implementation_quirk_unknown: return "implementation_quirk_unknown";
    case execution_semantic_gap_kind::intent_underdetermined: return "intent_underdetermined";
    }
    return "unknown";
}

inline flow_kind execution_semantic_flow(execution_semantic_kind kind) noexcept {
    switch (kind) {
    case execution_semantic_kind::driver_state:
    case execution_semantic_kind::instrument_reference:
    case execution_semantic_kind::sample_reference:
        return flow_kind::value;
    case execution_semantic_kind::timing_control:
    case execution_semantic_kind::phrase_control:
    case execution_semantic_kind::channel_mode:
    case execution_semantic_kind::implementation_quirk:
        return flow_kind::control;
    case execution_semantic_kind::information_gap:
        return flow_kind::none;
    default:
        return flow_kind::event;
    }
}

inline provenance_flags normalized_execution_semantic_flags(
    execution_semantic_origin origin,
    provenance_flags flags) noexcept {
    if (origin == execution_semantic_origin::external_document)
        flags = flags | provenance_flag::external_annotation;
    if (origin == execution_semantic_origin::unavailable)
        flags = flags | provenance_flag::incomplete;
    return flags;
}

inline void validate_execution_semantic_observation(
    const execution_semantic_observation& observation) {
    if (!is_execution_semantic_ancestry_layer(observation.layer))
        throw std::invalid_argument("execution semantic evidence must describe source, authored, or driver ancestry");
    if (!std::isfinite(observation.confidence) ||
        observation.confidence < 0.0 || observation.confidence > 1.0)
        throw std::invalid_argument("execution semantic confidence must be finite and in [0, 1]");
    if (observation.source.empty())
        throw std::invalid_argument("execution semantic evidence requires provenance source");
    if (observation.detail.empty())
        throw std::invalid_argument("execution semantic evidence requires a bounded detail");
    if (observation.origin == execution_semantic_origin::unavailable &&
        !observation.native_token.empty())
        throw std::invalid_argument("unavailable execution semantics cannot invent a native token");
    if (observation.origin == execution_semantic_origin::hypothesis &&
        observation.status != evidence_status::hypothesis)
        throw std::invalid_argument("hypothetical execution semantics require hypothesis status");
}

inline node_id append_execution_semantic_observation(
    musical_execution_graph& graph,
    execution_semantic_observation observation) {
    validate_execution_semantic_observation(observation);

    node value;
    value.kind = observation.kind == execution_semantic_kind::information_gap
        ? node_kind::logical_process
        : node_kind::trace_event;
    value.layer = observation.layer;
    value.flow = execution_semantic_flow(observation.kind);
    value.label = std::string{"execution semantic: "} + execution_semantic_kind_name(observation.kind);
    value.active = observation.active;
    value.attributes.push_back({
        "semantic_kind",
        std::string{execution_semantic_kind_name(observation.kind)},
        observation.status,
        observation.confidence,
        "",
    });
    value.attributes.push_back({
        "semantic_origin",
        std::string{execution_semantic_origin_name(observation.origin)},
        observation.status,
        observation.confidence,
        "",
    });
    value.attributes.push_back({
        "semantic_available",
        observation.origin != execution_semantic_origin::unavailable,
        evidence_status::exact,
        1.0,
        "",
    });
    if (!observation.native_token.empty()) {
        value.attributes.push_back({
            "native_token",
            observation.native_token,
            observation.status,
            observation.confidence,
            "",
        });
    }
    value.provenance.push_back({
        observation.status,
        observation.confidence,
        std::move(observation.source),
        std::nullopt,
        std::move(observation.detail),
        normalized_execution_semantic_flags(observation.origin, observation.flags),
    });
    return graph.add_node(std::move(value));
}

inline node_id append_execution_semantic_gap(
    musical_execution_graph& graph,
    execution_semantic_gap gap) {
    if (!is_execution_semantic_ancestry_layer(gap.layer))
        throw std::invalid_argument("execution semantic gap must belong to source, authored, or driver ancestry");
    if (gap.source.empty())
        throw std::invalid_argument("execution semantic gap requires provenance source");
    if (gap.detail.empty())
        throw std::invalid_argument("execution semantic gap requires a bounded reason");

    execution_semantic_observation observation;
    observation.layer = gap.layer;
    observation.kind = execution_semantic_kind::information_gap;
    observation.origin = execution_semantic_origin::unavailable;
    observation.status = evidence_status::exact;
    observation.confidence = 1.0;
    observation.source = std::move(gap.source);
    observation.detail = std::move(gap.detail);
    observation.active = gap.active;
    observation.flags = gap.flags;
    const node_id id = append_execution_semantic_observation(graph, std::move(observation));
    auto* stored = graph.find_node(id);
    if (stored == nullptr)
        throw std::logic_error("new execution semantic gap was not stored");
    stored->attributes.push_back({
        "gap_kind",
        std::string{execution_semantic_gap_kind_name(gap.kind)},
        evidence_status::exact,
        1.0,
        "",
    });
    return id;
}

inline edge_id link_execution_semantic_ancestry(
    musical_execution_graph& graph,
    node_id upstream,
    node_id downstream,
    evidence_status status,
    double confidence,
    std::string source,
    std::string detail) {
    const node* upstream_node = graph.find_node(upstream);
    const node* downstream_node = graph.find_node(downstream);
    if (upstream_node == nullptr || downstream_node == nullptr)
        throw std::invalid_argument("execution semantic ancestry references an unknown node");
    if (!is_execution_semantic_ancestry_layer(upstream_node->layer))
        throw std::invalid_argument("execution semantic ancestor must be source, authored, or driver evidence");
    if (static_cast<std::uint8_t>(upstream_node->layer) >
        static_cast<std::uint8_t>(downstream_node->layer))
        throw std::invalid_argument("execution semantic ancestry cannot point backward down the semantic ladder");
    if (!std::isfinite(confidence) || confidence < 0.0 || confidence > 1.0)
        throw std::invalid_argument("execution semantic ancestry confidence must be finite and in [0, 1]");
    if (source.empty() || detail.empty())
        throw std::invalid_argument("execution semantic ancestry requires source and detail");

    edge ancestry;
    ancestry.kind = edge_kind::derived_from;
    ancestry.from = downstream;
    ancestry.to = upstream;
    ancestry.provenance.push_back({
        status,
        confidence,
        std::move(source),
        std::nullopt,
        std::move(detail),
        to_flags(provenance_flag::none),
    });
    return graph.add_edge(std::move(ancestry));
}

inline std::vector<const node*> direct_execution_semantic_ancestors(
    const musical_execution_graph& graph,
    node_id downstream) {
    if (graph.find_node(downstream) == nullptr)
        throw std::invalid_argument("execution semantic ancestry query references an unknown node");

    std::vector<const node*> result;
    for (const auto* relation : graph.edges_from(downstream, edge_kind::derived_from)) {
        const node* candidate = graph.find_node(relation->to);
        if (candidate != nullptr && is_execution_semantic_ancestry_layer(candidate->layer))
            result.push_back(candidate);
    }
    return result;
}

} // namespace vgmtooling::model
