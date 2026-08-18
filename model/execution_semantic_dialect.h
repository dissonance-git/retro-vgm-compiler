#pragma once

#include "execution_semantic_provenance.h"

#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace vgmtooling::model {

enum class execution_artifact_role : std::uint8_t {
    authoring_source = 0,
    compiled_sequence,
    runtime_sequence,
    transformed_runtime_sequence,
    register_capture,
    unknown,
};

enum class execution_semantic_capability : std::uint8_t {
    note_attack = 0,
    note_release,
    articulation_link,
    pitch_without_retrigger,
    detune,
    transpose,
    modulation,
    volume_control,
    panning,
    instrument_selection,
    sample_selection,
    pcm_arbitration,
    sfx_priority,
    phrase_flow,
    raw_register_escape,
    special_channel_mode,
};

enum class execution_capability_state : std::uint8_t {
    supported = 0,
    unsupported,
    unknown,
};

struct execution_dialect_identity {
    std::string family;
    std::string revision;
    execution_artifact_role artifact_role = execution_artifact_role::unknown;
    execution_semantic_origin origin = execution_semantic_origin::external_document;
    evidence_status status = evidence_status::derived;
    double confidence = 1.0;
    std::string source;
    std::string detail;
    provenance_flags flags = to_flags(provenance_flag::none);
};

struct execution_dialect_capability_observation {
    node_id dialect = 0;
    semantic_layer layer = semantic_layer::driver_execution;
    execution_semantic_capability capability = execution_semantic_capability::note_attack;
    execution_capability_state state = execution_capability_state::unknown;
    execution_semantic_origin origin = execution_semantic_origin::external_document;
    evidence_status status = evidence_status::derived;
    double confidence = 1.0;
    std::string source;
    std::string native_token;
    std::string detail;
    provenance_flags flags = to_flags(provenance_flag::none);
};

inline const char* execution_artifact_role_name(execution_artifact_role role) noexcept {
    switch (role) {
    case execution_artifact_role::authoring_source: return "authoring_source";
    case execution_artifact_role::compiled_sequence: return "compiled_sequence";
    case execution_artifact_role::runtime_sequence: return "runtime_sequence";
    case execution_artifact_role::transformed_runtime_sequence: return "transformed_runtime_sequence";
    case execution_artifact_role::register_capture: return "register_capture";
    case execution_artifact_role::unknown: return "unknown";
    }
    return "unknown";
}

inline const char* execution_semantic_capability_name(execution_semantic_capability capability) noexcept {
    switch (capability) {
    case execution_semantic_capability::note_attack: return "note_attack";
    case execution_semantic_capability::note_release: return "note_release";
    case execution_semantic_capability::articulation_link: return "articulation_link";
    case execution_semantic_capability::pitch_without_retrigger: return "pitch_without_retrigger";
    case execution_semantic_capability::detune: return "detune";
    case execution_semantic_capability::transpose: return "transpose";
    case execution_semantic_capability::modulation: return "modulation";
    case execution_semantic_capability::volume_control: return "volume_control";
    case execution_semantic_capability::panning: return "panning";
    case execution_semantic_capability::instrument_selection: return "instrument_selection";
    case execution_semantic_capability::sample_selection: return "sample_selection";
    case execution_semantic_capability::pcm_arbitration: return "pcm_arbitration";
    case execution_semantic_capability::sfx_priority: return "sfx_priority";
    case execution_semantic_capability::phrase_flow: return "phrase_flow";
    case execution_semantic_capability::raw_register_escape: return "raw_register_escape";
    case execution_semantic_capability::special_channel_mode: return "special_channel_mode";
    }
    return "unknown";
}

inline const char* execution_capability_state_name(execution_capability_state state) noexcept {
    switch (state) {
    case execution_capability_state::supported: return "supported";
    case execution_capability_state::unsupported: return "unsupported";
    case execution_capability_state::unknown: return "unknown";
    }
    return "unknown";
}

inline execution_semantic_kind execution_capability_semantic_kind(
    execution_semantic_capability capability) noexcept {
    switch (capability) {
    case execution_semantic_capability::note_attack:
    case execution_semantic_capability::note_release:
    case execution_semantic_capability::articulation_link:
        return execution_semantic_kind::articulation_control;
    case execution_semantic_capability::pitch_without_retrigger:
    case execution_semantic_capability::detune:
    case execution_semantic_capability::transpose:
        return execution_semantic_kind::pitch_control;
    case execution_semantic_capability::modulation:
        return execution_semantic_kind::modulation_control;
    case execution_semantic_capability::instrument_selection:
        return execution_semantic_kind::instrument_reference;
    case execution_semantic_capability::sample_selection:
        return execution_semantic_kind::sample_reference;
    case execution_semantic_capability::phrase_flow:
        return execution_semantic_kind::phrase_control;
    case execution_semantic_capability::pcm_arbitration:
    case execution_semantic_capability::sfx_priority:
    case execution_semantic_capability::special_channel_mode:
        return execution_semantic_kind::channel_mode;
    default:
        return execution_semantic_kind::driver_command;
    }
}

inline const std::string* execution_string_attribute(const node& value, const std::string& key) noexcept {
    for (const auto& item : value.attributes) {
        if (item.key != key)
            continue;
        return std::get_if<std::string>(&item.value);
    }
    return nullptr;
}

inline std::optional<execution_artifact_role> execution_artifact_role_from_node(const node& value) noexcept {
    const auto* role = execution_string_attribute(value, "artifact_role");
    if (role == nullptr)
        return std::nullopt;
    for (const auto candidate : {
             execution_artifact_role::authoring_source,
             execution_artifact_role::compiled_sequence,
             execution_artifact_role::runtime_sequence,
             execution_artifact_role::transformed_runtime_sequence,
             execution_artifact_role::register_capture,
             execution_artifact_role::unknown,
         }) {
        if (*role == execution_artifact_role_name(candidate))
            return candidate;
    }
    return std::nullopt;
}

inline bool is_execution_dialect_identity_node(const node& value) noexcept {
    return value.layer == semantic_layer::source_representation &&
           execution_string_attribute(value, "dialect_family") != nullptr &&
           execution_artifact_role_from_node(value).has_value();
}

inline void validate_execution_dialect_identity(const execution_dialect_identity& identity) {
    if (identity.family.empty() || identity.revision.empty())
        throw std::invalid_argument("execution dialect identity requires family and revision");
    if (!std::isfinite(identity.confidence) || identity.confidence < 0.0 || identity.confidence > 1.0)
        throw std::invalid_argument("execution dialect confidence must be finite and in [0, 1]");
    if (identity.source.empty() || identity.detail.empty())
        throw std::invalid_argument("execution dialect identity requires source and detail");
    if (identity.origin == execution_semantic_origin::unavailable)
        throw std::invalid_argument("an unavailable dialect identity must be represented as a gap");
    if (identity.origin == execution_semantic_origin::hypothesis && identity.status != evidence_status::hypothesis)
        throw std::invalid_argument("hypothetical dialect identity requires hypothesis status");
}

inline node_id append_execution_dialect_identity(
    musical_execution_graph& graph,
    execution_dialect_identity identity) {
    validate_execution_dialect_identity(identity);
    node value;
    value.kind = node_kind::source_object;
    value.layer = semantic_layer::source_representation;
    value.flow = flow_kind::none;
    value.label = "execution dialect: " + identity.family;
    value.attributes.push_back({"dialect_family", identity.family, identity.status, identity.confidence, ""});
    value.attributes.push_back({"dialect_revision", identity.revision, identity.status, identity.confidence, ""});
    value.attributes.push_back({"artifact_role", std::string{execution_artifact_role_name(identity.artifact_role)}, identity.status, identity.confidence, ""});
    value.attributes.push_back({"semantic_origin", std::string{execution_semantic_origin_name(identity.origin)}, identity.status, identity.confidence, ""});
    value.provenance.push_back({
        identity.status,
        identity.confidence,
        std::move(identity.source),
        std::nullopt,
        std::move(identity.detail),
        normalized_execution_semantic_flags(identity.origin, identity.flags),
    });
    return graph.add_node(std::move(value));
}

inline void validate_execution_dialect_capability(
    const musical_execution_graph& graph,
    const execution_dialect_capability_observation& observation) {
    const node* dialect = graph.find_node(observation.dialect);
    if (dialect == nullptr || !is_execution_dialect_identity_node(*dialect))
        throw std::invalid_argument("execution dialect capability requires a dialect identity node");
    if (observation.layer != semantic_layer::authored_program && observation.layer != semantic_layer::driver_execution)
        throw std::invalid_argument("execution dialect capability belongs to authored or driver semantics");
    if (!std::isfinite(observation.confidence) || observation.confidence < 0.0 || observation.confidence > 1.0)
        throw std::invalid_argument("execution dialect capability confidence must be finite and in [0, 1]");
    if (observation.source.empty() || observation.detail.empty())
        throw std::invalid_argument("execution dialect capability requires source and detail");
    if (observation.state != execution_capability_state::supported && !observation.native_token.empty())
        throw std::invalid_argument("unsupported or unknown dialect capability cannot invent a native token");
    if (observation.origin == execution_semantic_origin::unavailable && observation.state != execution_capability_state::unknown)
        throw std::invalid_argument("unavailable capability evidence must remain unknown");
    if (observation.origin == execution_semantic_origin::hypothesis && observation.status != evidence_status::hypothesis)
        throw std::invalid_argument("hypothetical capability evidence requires hypothesis status");

    const auto role = execution_artifact_role_from_node(*dialect);
    if (!role.has_value())
        throw std::invalid_argument("execution dialect identity has no artifact role");
    if (observation.layer == semantic_layer::authored_program &&
        observation.origin == execution_semantic_origin::source_native &&
        *role != execution_artifact_role::authoring_source) {
        throw std::invalid_argument("non-authoring artifact cannot claim source-native authored semantics");
    }
}

inline node_id append_execution_dialect_capability(
    musical_execution_graph& graph,
    execution_dialect_capability_observation observation) {
    validate_execution_dialect_capability(graph, observation);

    execution_semantic_observation semantic;
    semantic.layer = observation.layer;
    semantic.kind = execution_capability_semantic_kind(observation.capability);
    semantic.origin = observation.origin;
    semantic.status = observation.status;
    semantic.confidence = observation.confidence;
    semantic.source = observation.source;
    semantic.native_token = observation.native_token;
    semantic.detail = observation.detail;
    semantic.flags = observation.flags;
    const node_id id = append_execution_semantic_observation(graph, std::move(semantic));

    node* stored = graph.find_node(id);
    if (stored == nullptr)
        throw std::logic_error("new execution dialect capability was not stored");
    stored->attributes.push_back({
        "dialect_capability",
        std::string{execution_semantic_capability_name(observation.capability)},
        observation.status,
        observation.confidence,
        "",
    });
    stored->attributes.push_back({
        "capability_state",
        std::string{execution_capability_state_name(observation.state)},
        observation.status,
        observation.confidence,
        "",
    });

    link_execution_semantic_ancestry(
        graph,
        observation.dialect,
        id,
        observation.status,
        observation.confidence,
        std::move(observation.source),
        std::move(observation.detail));
    return id;
}

} // namespace vgmtooling::model
